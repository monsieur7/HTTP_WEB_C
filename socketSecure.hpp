#include <filesystem>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "socket.hpp"
#include <map>

#pragma once

class SocketSecure : public Socket
{
public:
    SocketSecure(int port, std::filesystem::path cert, std::filesystem::path key);
    ~SocketSecure();
    void acceptSocket() override;
    void sendSocket(const void *buf, size_t len, int client_fd) override;
    void recvSocket(void *buf, size_t len, int client_fd) override;
    void sendSocket(const std::string &msg, int client_fd) override;
    std::string receiveSocket(int client_fd) override;
    void flushRecvBuffer(int client_fd) override;
    void closeSocket(int client_fd) override;
    // std::vector<int> pollClients(int timeout); //TODO : check if reusing this is possible
    void sendFile(std::filesystem::directory_entry file, int client_fd) override;

private:
    std::filesystem::path _cert;
    std::filesystem::path _key;
    SSL_CTX *_ctx;
    std::map<int, SSL *> _ssl_clients;
};