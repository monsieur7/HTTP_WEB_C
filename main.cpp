#include "socket.hpp"
#include <iostream>
#include <string>

int main()
{
    Socket s(8080);

    s.createSocket();
    s.bindSocket();
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