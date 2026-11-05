#ifndef SSLSOCKET_H
#define SSLSOCKET_H

#include "socket.h"
#include <openssl/ssl.h>
#include <stdexcept>
#include <iostream>

class SSLSocket : public Socket
{
public:
    static SSL_CTX *ssl_ctx;

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
    SSLSocket &operator=(const SSLSocket &) = delete;

    SSLSocket(SSLSocket &&other) noexcept : Socket(std::move(other)), ssl(other.ssl)
    {
        other.ssl = nullptr;
    }

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
            ssl = nullptr;
        }
    }

    int Connect(const std::string &address, const std::string &service) override
    {
        if (ssl != nullptr) { SSL_free(ssl); ssl = nullptr; }

        int success = Socket::Connect(address, service);
        if (success == 0)
        {
            ssl = SSL_new(ssl_ctx);
            SSL_set_fd(ssl, sockfd);
            SSL_connect(ssl); // The original code didn't check return value
        }
        return success;
    }

private:
    SSL *ssl = nullptr;
};

#endif // SSLSOCKET_H
