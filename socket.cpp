#include "socket.hpp"
// Signal handler function
// TODO : move this to a separate file (main.cpp)
void signalHandler(int signum) // https://stackoverflow.com/questions/343219/is-it-possible-to-use-signal-inside-a-c-class
{
    std::cout << "Caught signal " << signum << std::endl;

    exit(1);
}

Socket::Socket(int port)
{
    this->_socket = 0;
    this->_addr_in.sin6_family = AF_INET6;
    this->_addr_in.sin6_port = htons(port);
    memcpy(&this->_addr_in.sin6_addr, &in6addr_any, sizeof(in6addr_any));
    // sigaction for sigint :
    struct sigaction act;
    sigemptyset(&act.sa_mask);
    act.sa_handler = signalHandler;
    sigaction(SIGINT, &act, NULL);
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
    this->_socket = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (this->_socket == -1)
    {
        perror("Socket creation failed");
        this->~Socket();
        exit(1);
    }
    // set socket to be reusable
    int opt = 1;
    if (setsockopt(this->_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("setsockopt");
        this->~Socket();

        exit(1);
    }
    // set socket to be non-blocking

    if (fcntl(this->_socket, F_SETFL, O_NONBLOCK) < 0)
    {
        perror("fcntl");
        exit(1);
    }

    // set socket to allow ipv4 and ipv6

    int no = 0;
    if (setsockopt(this->_socket, IPPROTO_IPV6, IPV6_V6ONLY, (void *)&no, sizeof(no)) < 0)
    {
        perror("setsockopt");
        this->~Socket();

        exit(1);
    }
}
void Socket::bindSocket()
{
    if (bind(this->_socket, (struct sockaddr *)&this->_addr_in, sizeof(this->_addr_in)) == -1)
    {
        perror("Socket bind failed");
        this->~Socket();

        exit(1);
    }
}

void Socket::listenSocket()
{
    if (listen(this->_socket, 10) == -1)
    {
        perror("Socket listen failed");
        this->~Socket();

        exit(1);
    }
}
void Socket::acceptSocket()
{
    int new_socket;
    struct sockaddr_in new_addr;
    socklen_t addr_size = sizeof(new_addr);
    new_socket = accept(this->_socket, (struct sockaddr *)&new_addr, &addr_size); // blocking
    if (new_socket == -1)
    {
        perror("Socket accept failed");
        this->~Socket();

        exit(1);
    }
    this->_clients.push_back(new_socket);
}

void Socket::sendSocket(const void *buf, size_t len, int client_fd)
{
    if (send(client_fd, buf, len, MSG_NOSIGNAL) == -1)
    {
        perror("Socket send failed");
        close(client_fd);
        // remove client from list
        std::erase(this->_clients, client_fd);
    }
}
void Socket::recvSocket(void *buf, size_t len, int client_fd)
{

    if (recv(client_fd, buf, len, MSG_NOSIGNAL) == -1)
    {
        perror("Socket recv failed");
        close(client_fd);
        // remove client from list
        std::erase(this->_clients, client_fd);
    }
}
std::string Socket::printClientInfo(int client_fd)
{
    struct sockaddr_in6 addr;
    std::stringstream ss;
    socklen_t addr_size = sizeof(addr);
    getpeername(client_fd, (struct sockaddr *)&addr, &addr_size);
    char ipstr[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, &addr.sin6_addr, ipstr, sizeof(ipstr));
    ss << "IP: " << ipstr << " Port: " << ntohs(addr.sin6_port);
    return ss.str();
}

std::vector<int> Socket::pollClients(int timeout)
{
    std::vector<struct pollfd> fds;
    for (auto client : this->_clients)
    {
        struct pollfd fd;
        fd.fd = client;
        fd.events = POLLIN;
        fds.push_back(fd);
    }
    // add a struct for the server socket
    struct pollfd server_fd;
    server_fd.fd = this->_socket;
    server_fd.events = POLLIN;
    fds.push_back(server_fd);
    int res = poll(fds.data(), fds.size(), timeout);
    if (res <= 0)
    {
        return std::vector<int>();
    }
    std::vector<int> ready_clients;
    for (int i = 0; i < fds.size() - 1; i++) //-1 because the last one is the server socket
    {
        if (fds[i].revents & POLLIN)
        {
            ready_clients.push_back(fds[i].fd);
        }
    }
    if (fds[fds.size() - 1].revents & POLLIN)
    {
        acceptSocket();
    }
    return ready_clients;
}
int Socket::getFdfromClient(int client_num)
{
    if (client_num >= this->_clients.size())
    {
        return -1;
    }
    return this->_clients[client_num];
}
void Socket::flushRecvBuffer(int client_fd)
{
    char buf[1024];
    while (recv(client_fd, buf, 1024, MSG_DONTWAIT) > 0)
    {
    }
}

std::string Socket::receiveSocket(int client_fd)
{
    char buf[1024];
    std::stringstream ss;
    ssize_t bytes_read;
    do
    {
        bytes_read = recv(client_fd, buf, 1024, MSG_DONTWAIT);
        if (bytes_read == -1)
        {
            if (errno != EWOULDBLOCK && errno != EAGAIN)
            {

                perror("Socket recv failed");
                this->~Socket();
            }
        }
        else
        {
            ss << std::string(buf, bytes_read);
        }
    } while (bytes_read > 0);
    return ss.str();
}

void Socket::sendSocket(const std::string &msg, int client_fd)
{
    sendSocket(msg.c_str(), msg.size(), client_fd);
}