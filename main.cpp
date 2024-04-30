#include "socket.hpp"
#include "parseString.hpp"
#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include "FileTypeDetector.hpp"
#include <format>
std::filesystem::directory_entry findFile(std::map<std::filesystem::directory_entry, std::string> &files, std::string file)
{
    if (file[0] != '/')
    {
        return std::filesystem::directory_entry();
    }
    for (const auto &entry : files)
    {
        if (entry.first.path().filename() == file.substr(1))
        {
            return entry.first;
        }
    }
    return std::filesystem::directory_entry();
}
int main()
{
    Socket s(8080);
    FileTypeDetector ftd;
    ftd.addSingleFileType("html", "text/html");
    ftd.addSingleFileType("css", "text/css");
    ftd.addSingleFileType("js", "text/javascript");
    ftd.addSingleFileType("txt", "");

    std::string dir = "www";
    std::filesystem::path path = std::filesystem::current_path();
    path /= "../";
    path /= dir;
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

    int file_size = 0;
    auto file = std::ifstream(path / "index.html", std::ios::binary);
    std::string response404 = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\nConnection: Close\r\n\r\n";
    // TODO : keep alive !
    s.createSocket();
    s.bindSocket();
    ParseString p("");
    while (true)
    {
        s.listenSocket();
        std::vector<int> clients = s.pollClients(1000);
        for (auto i = 0; i < clients.size(); i++)
        {
            std::cout << s.printClientInfo(clients[i]) << std::endl;
            std::string received = s.receiveSocket(clients[i]);
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
                int file_size = file.file_size();
                std::string response = std::format("HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\nConnection: Keep-Alive\r\nContent-Length: {0} \r\n\r\n", file_size); // we cannot use a variable here :(
                s.sendSocket(response.c_str(), response.size(), clients[i]);
                s.sendFile(file, clients[i]);
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