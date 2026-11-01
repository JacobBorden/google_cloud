#ifndef SSL_SOCKET_H
#define SSL_SOCKET_H

#include "socket.h"
#include <openssl/ssl.h>

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
            SSL_connect(ssl);
        }
        return result;
    }


    ssize_t Send(const std::string &buf, int flags = 0) override
    {
        if (ssl) {
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
        std::string buffer(max_length, '\0');

        if (ssl) {
            ssize_t bytes_received = SSL_read(ssl, &buffer[0], buffer.size());
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

#endif // SSL_SOCKET_H
