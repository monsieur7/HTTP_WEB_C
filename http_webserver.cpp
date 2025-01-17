
#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
// local include
#include "socketSecure.hpp"
#include "parseString.hpp"
#include "FileTypeDetector.hpp"

// #include <format>
#define PORT 8080 // port to listen on
std::filesystem::directory_entry findFile(std::map<std::filesystem::directory_entry, std::string> &files, std::string file)
{
    // convert file to path
    std::filesystem::path path = std::filesystem::path(file);
    if (file[0] != '/')
    {
        return std::filesystem::directory_entry();
    }
    for (const auto &entry : files)
    {

        if (entry.first.path().filename().stem() == path.filename().stem())
        {
            return entry.first;
        }
    }
    return std::filesystem::directory_entry();
}
int main()
{
    // print pid :
    std::cout << "PID : " << getpid() << std::endl;
    FileTypeDetector ftd;
    ftd.addSingleFileType(".html", "text/html");
    ftd.addSingleFileType(".css", "text/css");
    ftd.addSingleFileType(".js", "text/javascript");
    ftd.addSingleFileType(".txt", "");

    std::string dir = "www";
    std::filesystem::path path = std::filesystem::current_path();
    path /= "../";
    path /= dir;

    // key :
    std::filesystem::path keyPath = std::filesystem::current_path();
    keyPath /= "..";
    keyPath /= "raspberrypi-fr.local.key";

    // crt :
    std::filesystem::path certPath = std::filesystem::current_path();
    certPath /= "..";
    certPath /= "raspberrypi-fr.local.crt";

    // list all files in this dir:
    std::vector<std::filesystem::directory_entry> files;
    std::map<std::filesystem::directory_entry, std::string> file_types;
    for (const auto &entry : std::filesystem::directory_iterator(path))
    {
        files.push_back(entry);
    }
    for (const auto &file : files)
    {
        std::cout << file << std::endl;
        std::string extension = file.path().extension();
        std::string type = ftd.getFileType(extension);
        file_types[file] = type;
    }
    SocketSecure s(PORT, certPath, keyPath);
    auto file = std::ifstream(path / "index.html", std::ios::binary);
    std::string response404 = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\nConnection: Close\r\n\r\n";
    // TODO : keep alive !

    s.createSocket();
    s.bindSocket();
    ParseString p("");
    while (true)
    {
        s.listenSocket();
        std::vector<int> clients;
        try
        {
            clients = s.pollClients(1000);
        }
        catch (const std::runtime_error &e)
        {
            std::cerr << e.what() << '\n';
        }
        for (std::size_t i = 0; i < clients.size(); i++)
        {
            std::cout << s.printClientInfo(clients[i]) << std::endl;
            std::string received;
            try
            {
                received = s.receiveSocket(clients[i]);
            }
            catch (const std::runtime_error &e)
            {
                std::cerr << e.what() << '\n';
                s.closeSocket(clients[i]);
                continue;
            }

            p.setData(received);
            std::map<std::string, std::string> headers = p.parseRequest();
            for (auto const &x : headers)
            {
                std::cout << x.first << " : " << x.second << std::endl;
            }
            if (headers["Path"] == "/")
            {
                headers["Path"] = "/index.html"; // default file
            }
            std::filesystem::directory_entry file = findFile(file_types, headers["Path"]);
            if (file.path().filename() != "")
            {
                std::string response = std::string("HTTP/1.1 200 OK\r\nContent-Type: ") + std::string(file_types[file]) + std::string("; charset=UTF-8\r\nTransfer-Encoding: chunked\r\nConnection: Close\r\n\r\n\r\n");
                s.sendSocket(response.c_str(), response.size(), clients[i]);
                s.sendFile(file, clients[i]);
                s.closeSocket(clients[i]); // close the connection
            }
            // search in files if there is a file with this name and send it
            // if not send 404
            else
            {
                s.sendSocket(response404.c_str(), response404.size(), clients[i]);
                s.closeSocket(clients[i]); // close the connection
            }
        }
    }
    file.close();
    return 0;
}
