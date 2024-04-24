#include "socket.hpp"

Socket::Socket(int port)
{
    this->_socket = 0;
    this->_addr_in.sin6_family = AF_INET6;
    this->_addr_in.sin6_port = htons(port);
    memcpy(&this->_addr_in.sin6_addr, &in6addr_any, sizeof(in6addr_any));
}

Socket::~Socket()
{
    close(this->_socket);
    for (auto client : this->_clients)
    {
        close(client);
    }
}

void Socket::createSocket()
{
    this->_socket = socket(AF_INET6, SOCK_STREAM, 0);
    if (this->_socket == -1)
    {
        perror("Socket creation failed");
        Socket::~Socket();
        exit(1);
    }
    // set socket to be reusable
    int opt = 1;
    if (setsockopt(this->_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("setsockopt");
        Socket::~Socket();

        exit(1);
    }
    // set socket to be non-blocking
    /*
    if (fcntl(this->_socket, F_SETFL, O_NONBLOCK) < 0)
    {
        perror("fcntl");
        exit(1);
    }
    */
    // set socket to allow ipv4 and ipv6

    int no = 0;
    if (setsockopt(this->_socket, IPPROTO_IPV6, IPV6_V6ONLY, (void *)&no, sizeof(no)) < 0)
    {
        perror("setsockopt");
        Socket::~Socket();

        exit(1);
    }
}
void Socket::bindSocket()
{
    if (bind(this->_socket, (struct sockaddr *)&this->_addr_in, sizeof(this->_addr_in)) == -1)
    {
        perror("Socket bind failed");
        Socket::~Socket();

        exit(1);
    }
}

void Socket::listenSocket()
{
    if (listen(this->_socket, 10) == -1)
    {
        perror("Socket listen failed");
        Socket::~Socket();

        exit(1);
    }
}
void Socket::acceptSocket()
{
    int new_socket;
    struct sockaddr_in new_addr;
    socklen_t addr_size = sizeof(new_addr);
    new_socket = accept(this->_socket, (struct sockaddr *)&new_addr, &addr_size);
    if (new_socket == -1)
    {
        perror("Socket accept failed");
        Socket::~Socket();

        exit(1);
    }
    this->_clients.push_back(new_socket);
}

void Socket::sendSocket(const void *buf, size_t len, int client)

{
    if (client > this->_clients.size())
    {
        perror("Client not found");
        exit(1);
    }
    if (send(this->_clients[client], buf, len, 0) == -1)
    {
        perror("Socket send failed");
        Socket::~Socket();

        exit(1);
    }
}
void Socket::recvSocket(void *buf, size_t len, int client)
{
    if (client > this->_clients.size())
    {
        perror("Client not found");
        Socket::~Socket();

        exit(1);
    }
    if (recv(this->_clients[client], buf, len, 0) == -1)
    {
        perror("Socket recv failed");
        Socket::~Socket();
        exit(1);
    }
}
std::string Socket::printClientInfo(int client)
{
    struct sockaddr_in6 addr;
    std::stringstream ss;
    socklen_t addr_size = sizeof(addr);
    getpeername(this->_clients[client], (struct sockaddr *)&addr, &addr_size);
    char ipstr[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, &addr.sin6_addr, ipstr, sizeof(ipstr));
    ss << "IP: " << ipstr << " Port: " << ntohs(addr.sin6_port);
    return ss.str();
}