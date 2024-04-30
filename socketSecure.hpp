#include <filesystem>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "socket.hpp"
#pragma once

class SocketSecure : public Socket
{
public:
    SocketSecure(int port, std::filesystem::path cert, std::filesystem::path key);
    ~SocketSecure();
    void acceptSocket() override; // DONE
    void sendSocket(const void *buf, size_t len, int client_fd) override;
    void recvSocket(void *buf, size_t len, int client_fd) override;
    void flushRecvBuffer(int client_fd);
    void closeSocket(int client_fd) override;
    // std::vector<int> pollClients(int timeout); //TODO : check if reusing this is possible
    void sendFile(std::filesystem::directory_entry file, int client_fd);

private:
    std::filesystem::path _cert;
    std::filesystem::path _key;
    SSL_CTX *_ctx;
};