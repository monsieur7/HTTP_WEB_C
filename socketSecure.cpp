#include "socketSecure.hpp"
// https://wiki.openssl.org/index.php/Simple_TLS_Server
SocketSecure::SocketSecure(int port, std::filesystem::path cert, std::filesystem::path key) : Socket(port)
{
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
}

void SocketSecure::acceptSocket()
{
    Socket::acceptSocket();
    SSL *ssl = SSL_new(_ctx);
    if (!ssl)
    {
        ERR_print_errors_fp(stderr);
        this->~SocketSecure();
    }
    if (!SSL_set_fd(ssl, this->_clients.back()))
    {
        ERR_print_errors_fp(stderr);
        this->~SocketSecure();
    }
    // NO NON BLOCKING
    // FIXME : handle non blocking
    int err;
    if ((err = SSL_accept(ssl)) < 0) // non blocking
    {
        ERR_print_errors_fp(stderr);
        err = SSL_get_error(ssl, err);
        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
        {

            // The operation did not complete; the underlying BIO reported an I/O error
            ;
        }
        else
        {
            // A fatal error occurred, you should handle it and probably close the connection
            ERR_print_errors_fp(stderr);
            // TODO : close connection
            this->~SocketSecure();
        }
    }
    // set non blocking:
    if (!BIO_socket_nbio(this->_clients.back(), 1))
    {
        perror("Error setting non blocking");
        this->~SocketSecure();
    }
    _ssl_clients[this->_clients.back()] = ssl;
}

void SocketSecure::sendSocket(const void *buf, size_t len, int client_fd)
{

    if (SSL_write(_ssl_clients[client_fd], buf, len) <= 0)
    {
        ERR_print_errors_fp(stderr);
    }
}
void SocketSecure::recvSocket(void *buf, size_t len, int client_fd)
{
    if (SSL_read(_ssl_clients[client_fd], buf, len) <= 0)
    {
        ERR_print_errors_fp(stderr);
    }
}
void SocketSecure::flushRecvBuffer(int client_fd)
{
    char buf[1024];
    while (SSL_read(_ssl_clients[client_fd], buf, 1024) > 0)
    {
        ;
    }
}

void SocketSecure::closeSocket(int client_fd)
{
    SSL_shutdown(_ssl_clients[client_fd]);
    SSL_free(_ssl_clients[client_fd]);
    // erase from map
    _ssl_clients.erase(client_fd);
    Socket::closeSocket(client_fd);
}

void SocketSecure::sendFile(std::filesystem::directory_entry file, int client_fd)
{
    std::ifstream file_stream(file.path(), std::ios::binary);
    if (!file_stream.is_open())
    {
        perror("File not found");
        return;
    }
    char buf[1024];
    while (file_stream.read(buf, 1024))
    {
        if (SSL_write(_ssl_clients[client_fd], buf, 1024) <= 0)
        {
            ERR_print_errors_fp(stderr);
        }
    }
    if (SSL_write(_ssl_clients[client_fd], buf, file_stream.gcount()) <= 0)
    {
        ERR_print_errors_fp(stderr);
    }
    file_stream.close();
}
SocketSecure::~SocketSecure()
{
    for (auto ssl : _ssl_clients)
    {
        SSL_shutdown(ssl.second);
        SSL_free(ssl.second);
    }
    SSL_CTX_free(_ctx);
    Socket::~Socket();
    exit(1); // TODO : find a better way to handle this
}

void SocketSecure::sendSocket(const std::string &msg, int client_fd)
{
    if (SSL_write(_ssl_clients[client_fd], msg.c_str(), msg.size()) <= 0)
    {
        ERR_print_errors_fp(stderr);
    }
}
std::string SocketSecure::receiveSocket(int client_fd)
{
    std::stringstream ss;
    char buf[1024];
    ssize_t bytes_read;
    do
    {
        bytes_read = SSL_read(_ssl_clients[client_fd], buf, 1024);
        if (bytes_read <= 0)
        {
            if (SSL_get_error(_ssl_clients[client_fd], bytes_read) != SSL_ERROR_WANT_READ)
            {
                ERR_print_errors_fp(stderr);
            }
        }
        else
        {
            ss << std::string(buf, bytes_read);
        }
    } while (bytes_read > 0);
    return ss.str();
}