#ifndef SOCKET_H
#define SOCKET_H
#include <unistd.h>
#include <sys/socket.h>

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
private:
    int sockfd = -1;
};

#endif // SOCKET_H