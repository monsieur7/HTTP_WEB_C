
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <vector>
#include <string>
#include <sstream>
#include <poll.h>

class Socket
{
private:
    int _socket;
    struct sockaddr_in6 _addr_in;
    std::vector<int> _clients;

public:
    Socket(int port);

    ~Socket();
    void createSocket();
    void bindSocket();
    void listenSocket();
    void acceptSocket();
    void sendSocket(const void *buf, size_t len, int client_fd);
    void recvSocket(void *buf, size_t len, int client_fd);
    std::string printClientInfo(int client);
    std::vector<int> pollClients(int timeout);
    int getFdfromClient(int client_num);
    void flushRecvBuffer(int client_fd);
};
