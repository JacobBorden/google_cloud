#ifndef SOCKET_H
#define SOCKET_H
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string>
#include <cstring>
#include <cerrno>
#include <stdexcept>
#include <openssl/ssl.h>
#include <iostream>

class Socket
{
public:
    Socket()
    {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
    }
    Socket(const Socket &) = delete;
    Socket(Socket &&other) noexcept : sockfd(other.sockfd), io_timeout_ms(other.io_timeout_ms)
    {
        other.sockfd = -1;
        other.io_timeout_ms = 0;
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
            io_timeout_ms = other.io_timeout_ms;
            other.io_timeout_ms = 0;
        }
        return *this;
    }
    virtual ~Socket()
    {
        if (sockfd != -1)
        {
            close(sockfd);
        }
    }
    bool isValid() const
    {
        return sockfd != -1;
    }
    virtual int Connect(const std::string &address, const std::string &service)
    {
        if (sockfd != -1) { close(sockfd); sockfd = -1; }
        struct addrinfo hints{};
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        struct addrinfo *res = nullptr;
        int connected = getaddrinfo(address.c_str(), service.c_str(), &hints, &res);
        if (connected != 0)
        {
            std::cerr << "Getaddrinfo error: " << gai_strerror(connected) << std::endl;
            if (res != nullptr)
            {
                freeaddrinfo(res);
            }
            return connected; // getaddrinfo failed
        }
        struct addrinfo *p;
        for (p = res; p != nullptr; p = p->ai_next)
        {
            sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
            if (sockfd == -1) continue;

            if (io_timeout_ms > 0)
            {
                if (!ApplyIoTimeout(sockfd, io_timeout_ms))
                {
                    close(sockfd);
                    sockfd = -1;
                    continue;
                }
            }

            int success = connect(sockfd, p->ai_addr, p->ai_addrlen);
            if (success == 0)
            {
                break; // Successfully connected
            }
            if (success == -1)
            {
                std::cerr << "Connect error: " << strerror(errno) << std::endl;
            }
            close(sockfd);
            sockfd = -1;
        }
        freeaddrinfo(res);
        if (p == nullptr)
        {
            return -1; // Connection failed
        }

        return 0; // Success
    }

    // Applies to send and receive. Name resolution and connect remain blocking.
    void SetIoTimeout(int timeout_ms)
    {
        if (timeout_ms < 0)
        {
            throw std::invalid_argument("I/O timeout must be nonnegative");
        }
        if (sockfd != -1)
        {
            if (!ApplyIoTimeout(sockfd, timeout_ms))
            {
                throw std::runtime_error("Failed to set socket I/O timeout");
            }
        }
        io_timeout_ms = timeout_ms;
    }

    virtual ssize_t Send(const std::string &buf, int flags = 0)
    {
        ssize_t bytes_sent = send(sockfd, buf.c_str(), buf.length(), flags);
        return bytes_sent;
    }

    virtual std::string Receive(int flags = 0, size_t max_length = 4096)
    {
        if (max_length == 0)
        {
            return "";
        }
        std::string buffer(max_length, '\0');
        ssize_t bytes_received = recv(sockfd, &buffer[0], buffer.size(), flags);
        if (bytes_received > 0)
        {
            buffer.resize(bytes_received);
            return buffer;
        }
        return "";
    }

protected:
    static bool ApplyIoTimeout(int fd, int timeout_ms)
    {
        const struct timeval tv{timeout_ms / 1000, (timeout_ms % 1000) * 1000};
        return setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == 0 &&
               setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) == 0;
    }
    int sockfd = -1;
    int io_timeout_ms = 0;
};

class SSLSocket : public Socket
{
public:
    SSLSocket() : Socket()
    {
        if (ssl_ctx == nullptr)
        {
            SSL_library_init();
            OpenSSL_add_all_algorithms();
            SSL_load_error_strings();
            ssl_ctx = SSL_CTX_new(TLS_client_method());
        }
    }
    SSLSocket(const SSLSocket &) = delete;
    SSLSocket(SSLSocket &&other) noexcept : Socket(std::move(other)), ssl(other.ssl)
    {
        other.ssl = nullptr;
    }
    SSLSocket &operator=(const SSLSocket &) = delete;
    SSLSocket &operator=(SSLSocket &&other) noexcept
    {
        if (this != &other)
        {
            if (ssl != nullptr)
            {
                SSL_free(ssl);
            }
            Socket::operator=(std::move(other));
            ssl = other.ssl;
            other.ssl = nullptr;
        }
        return *this;
    }
    ~SSLSocket() override
    {
        if (ssl != nullptr)
        {
            SSL_free(ssl);
        }
    }

    int Connect(const std::string &address, const std::string &service) override
    {
        if (ssl != nullptr) { SSL_free(ssl); ssl = nullptr; }
        int result = Socket::Connect(address, service);
        if (result == 0)
        {
            ssl = SSL_new(ssl_ctx);
            SSL_set_fd(ssl, sockfd);
            if (SSL_connect(ssl) != 1) {
                SSL_free(ssl);
                ssl = nullptr;
                close(sockfd);
                sockfd = -1;
                return -1;
            }
        }
        return result;
    }

    ssize_t Send(const std::string &buf, int flags = 0) override
    {
        if (ssl != nullptr)
        {
            (void)flags; // ignoring flags for ssl for now
            return SSL_write(ssl, buf.c_str(), buf.length());
        }
        return Socket::Send(buf, flags);
    }

    std::string Receive(int flags = 0, size_t max_length = 4096) override
    {
        if (max_length == 0)
        {
            return "";
        }
        if (ssl != nullptr)
        {
            (void)flags; // ignoring flags
            std::string buffer(max_length, '\0');
            int bytes_received = SSL_read(ssl, &buffer[0], buffer.size());
            if (bytes_received > 0)
            {
                buffer.resize(bytes_received);
                return buffer;
            }
            return "";
        }
        return Socket::Receive(flags, max_length);
    }

    static SSL_CTX *ssl_ctx;
private:
    SSL *ssl = nullptr;
};

#endif // SOCKET_H
