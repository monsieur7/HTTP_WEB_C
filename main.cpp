#include "socket.hpp"
#include "parseString.hpp"
#include <iostream>
#include <string>
#include <fstream>

int main()
{
    Socket s(8080);

    // open index.html :
    std::ifstream file("/home/nolane/Desktop/ESIREM/STAGE_PT/HTTP_WEB_C/index.html");
    std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: Close\r\n\r\n";
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
                s.sendSocket(response.c_str(), response.size(), clients[i]);

                if (file.is_open())
                {
                    std::cout << "File is open" << std::endl;

                    // Create a buffer to hold file data
                    char buffer[1024];
                    while (!file.eof())
                    {
                        // Read data into the buffer
                        file.read(buffer, sizeof(buffer));
                        // Send the buffer over the network
                        s.sendSocket(buffer, file.gcount(), clients[i]);
                    }
                    std::cout << "File sent" << std::endl;
                    // Reset the file stream to the beginning of the file
                    file.clear();
                    file.seekg(0, std::ios::beg);
                    // close the connection
                    s.closeSocket(clients[i]);
                }
                else
                {
                    s.sendSocket(response404.c_str(), response404.size(), clients[i]);
                }
            }
            else
            {
                s.sendSocket(response404.c_str(), response404.size(), clients[i]);
            }
        }
    }
    file.close();
    return 0;
}