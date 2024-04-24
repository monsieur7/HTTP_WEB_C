#include "socket.hpp"
#include "parseString.hpp"
#include <iostream>
#include <string>

int main()
{
    Socket s(8080);

    s.createSocket();
    s.bindSocket();
    std::string input = "Première ligne\r\nDeuxième ligne\r\nTroisième ligne\r\n";
    ParseString data(input);
    std::vector<std::string> lines = data.parseHTML();

    for (const auto& line : lines) {
        std::cout << line << std::endl;
    }

    while (true)
    {
        s.listenSocket();
        std::vector<int> clients = s.pollClients(1000);
        for (int i = 0; i < clients.size(); i++)
        {
            std::cout << s.printClientInfo(clients[i]) << std::endl;
            s.sendSocket("Hello from server", 17, clients[i]);
            s.flushRecvBuffer(clients[i]);
        }
    }

    return 0;
}