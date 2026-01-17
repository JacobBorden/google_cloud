#ifndef SOCKET_H
#define SOCKET_H
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string>

class Socket
{
public:
    Socket()
    {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
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
            return connected; // getaddrinfo failed
        }
        struct addrinfo *p;
        for (p = res; p != nullptr; p = p->ai_next)
        {
            sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
            if (connect(sockfd, p->ai_addr, p->ai_addrlen) == 0)
            {
                break; // Successfully connected
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
};

#endif // SOCKET_H