#ifndef SOCKET_H
#define SOCKET_H
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string>
#include <cstring>
#include <cerrno>
#include <openssl/ssl.h>

class Socket
{
public:
    Socket()
    {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (ssl_ctx == nullptr)
        {
            SSL_library_init();
            OpenSSL_add_all_algorithms();
            SSL_load_error_strings();
            ssl_ctx = SSL_CTX_new(TLS_client_method());
        }
    }
    Socket(const Socket &) = delete;
    Socket(Socket &&other) noexcept : sockfd(other.sockfd)
    {
        other.sockfd = -1;
    }
    Socket &operator=(const Socket &) = delete;
    Socket &operator=(Socket &&other) noexcept
    {
        if (this != &other)
        {
            if (sockfd != -1)
            {
                close(sockfd);
            }
            sockfd = other.sockfd;
            other.sockfd = -1;
        }
        return *this;
    }
    ~Socket()
    {
        if (ssl != nullptr)
        {
            SSL_free(ssl);
        }
        if (sockfd != -1)
        {
            close(sockfd);
        }
    }
    bool isValid() const
    {
        return sockfd != -1;
    }
    int Connect(const std::string &address, const std::string &service)
    {
        struct addrinfo hints{
            .ai_family = AF_UNSPEC,
            .ai_socktype = SOCK_STREAM};
        struct addrinfo *res;
        int connected = getaddrinfo(address.c_str(), service.c_str(), &hints, &res);
        if (connected != 0)
        {
            std::cerr << "Getaddrinfo error: " << gai_strerror(connected) << std::endl;
            return connected; // getaddrinfo failed
        }
        struct addrinfo *p;
        for (p = res; p != nullptr; p = p->ai_next)
        {
            sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
            int success = connect(sockfd, p->ai_addr, p->ai_addrlen);
            if (success == 0)
            {
                ssl = SSL_new(ssl_ctx);
                SSL_set_fd(ssl, sockfd);
                SSL_connect(ssl);
                break; // Successfully connected
            }
            if (success == -1)
            {
                std::cerr << "Connect error: " << strerror(errno) << std::endl;
            }
            close(sockfd);
        }
        freeaddrinfo(res);
        if (p == nullptr)
        {
            return -1; // Connection failed
        }

        return 0; // Success
    }
    ssize_t Send(const std::string &buf, int flags = 0)
    {
        ssize_t bytes_sent = send(sockfd, buf.c_str(), buf.length(), flags);
        return bytes_sent;
    }

    std::string Receive(int flags = 0)
    {
        std::string buffer(4096, '\0');
        ssize_t bytes_received = recv(sockfd, &buffer[0], buffer.size(), flags);
        if (bytes_received > 0)
        {
            buffer.resize(bytes_received);
            return buffer;
        }
        return "";
    }

private:
    int sockfd = -1;
    static SSL_CTX *ssl_ctx;
    SSL *ssl;
};

#endif // SOCKET_H