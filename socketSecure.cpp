#include "socketSecure.hpp"
// https://wiki.openssl.org/index.php/Simple_TLS_Server
SocketSecure::SocketSecure(int port, std::filesystem::path cert, std::filesystem::path key)
{
    Socket::Socket(port);
    Socket::createSocket();
    Socket::bindSocket();

    _cert = cert;
    _key = key;
    _ctx = SSL_CTX_new(SSLv23_server_method());
    if (!_ctx)
    {
        ERR_print_errors_fp(stderr);
        this->~SocketSecure();
    }
    if (SSL_CTX_use_certificate_file(_ctx, _cert.c_str(), SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stderr);
        this->~SocketSecure();
    }
    if (SSL_CTX_use_PrivateKey_file(_ctx, _key.c_str(), SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stderr);
        this->~SocketSecure();
    }
    if (!SSL_CTX_check_private_key(_ctx))

    {
        fprintf(stderr, "Private key does not match the certificate public key\n");
        this->~SocketSecure();
    }
    // set socket to be reusable and non-blocking:
    if (!BIO_socket_nbio(thi, 1)) // see https://www.openssl.org/docs/manmaster/man7/ossl-guide-tls-client-non-block.html
    {
        perror("BIO_socket_nbio");
        this->~SocketSecure();
    }
}

void SocketSecure::acceptSocket()
{
    if (_ctx == nullptr)
    {
        return;
    }

    SSL_set_fd(ssl, this->_socket);

    if (SSL_accept(ssl) <= 0) // TODO : see if we can use SSL_accept
    {
        ERR_print_errors_fp(stderr);
        close(client_fd);
        return;
    }
    _clients.push_back(client_fd);
}

void SocketSecure::sendSocket(const void *buf, size_t len, int client_fd)
{
    if (_ctx == nullptr)
    {
        return;
    }
    SSL_set_fd(ssl, client_fd);
    if (SSL_write(ssl, buf, len) <= 0)
    {
        ERR_print_errors_fp(stderr);
        close(client_fd);
    }
}

void SocketSecure::recvSocket(void *buf, size_t len, int client_fd)
{
    if (_ctx == nullptr)
    {
        return;
    }
    SSL_set_fd(ssl, client_fd);
    if (SSL_read(ssl, buf, len) <= 0)
    {
        ERR_print_errors_fp(stderr);
        close(client_fd);
    }
}

void SocketSecure::flushRecvBuffer(int client_fd)
{
    if (_ctx == nullptr)
    {
        return;
    }
    SSL_set_fd(ssl, client_fd);
    char buf[1024];
    while (SSL_read(ssl, buf, 1024) > 0)
    {
        continue;
    }
}
