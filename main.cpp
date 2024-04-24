#include "socket.hpp"
#include <iostream>
#include <string>

int main()
{
    Socket s(8080);
    s.createSocket();
    s.bindSocket();
    s.listenSocket();
    s.acceptSocket();

    std::string message = "Hello from server!";
    std::cout << s.printClientInfo(0) << std::endl;
    s.sendSocket(message.c_str(), message.size(), 0);
    char buffer[1024];
    s.recvSocket(buffer, sizeof(buffer), 0);
    std::cout << "Received: " << buffer << std::endl;
    return 0;
}