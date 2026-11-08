#ifndef SOCKET_H
#define SOCKET_H
#include <unistd.h>
#include <algorithm>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string>
#include <utility>
#include <cstring>
#include <cerrno>
#include <limits>
#include <stdexcept>
#include <openssl/ssl.h>
#include <iostream>
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
    Socket(Socket &&other) noexcept : sockfd(other.sockfd), ssl(other.ssl), io_timeout_ms(other.io_timeout_ms), tls_host(std::move(other.tls_host)), tls_ca_file(std::move(other.tls_ca_file))
    {
        other.sockfd = -1;
        other.ssl = nullptr;
        other.io_timeout_ms = 0;
    }
    Socket &operator=(const Socket &) = delete;
    Socket &operator=(Socket &&other) noexcept
    {
        if (this != &other)
        {
            if (ssl != nullptr)
            {
                SSL_free(ssl);
            }
            if (sockfd != -1)
            {
                close(sockfd);
            }
            sockfd = other.sockfd;
            other.sockfd = -1;
            ssl = other.ssl;
            other.ssl = nullptr;
            io_timeout_ms = other.io_timeout_ms;
            other.io_timeout_ms = 0;
            tls_host = std::move(other.tls_host);
            tls_ca_file = std::move(other.tls_ca_file);
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
    // Enables certificate chain and hostname checks for subsequent TLS connections.
    // The CA file is an explicit trust anchor; existing callers remain unverified.
    void SetTlsPeerVerification(std::string expected_host, std::string ca_file)
    {
        if (expected_host.empty() || ca_file.empty() ||
            expected_host.find('\0') != std::string::npos ||
            ca_file.find('\0') != std::string::npos)
            throw std::invalid_argument("TLS host and CA file must be nonempty");
        tls_host = std::move(expected_host);
        tls_ca_file = std::move(ca_file);
    }
    // Uses OpenSSL's configured default CA paths, including SSL_CERT_FILE.
    void SetTlsPeerVerification(std::string expected_host)
    {
        if (expected_host.empty() || expected_host.find('\0') != std::string::npos)
            throw std::invalid_argument("TLS host must be nonempty");
        tls_host = std::move(expected_host);
        tls_ca_file.clear();
    }
    int Connect(const std::string &address, const std::string &service, bool use_tls)
    {
        if (ssl != nullptr) { SSL_free(ssl); ssl = nullptr; }
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
                if (use_tls)
                {
                    ssl = SSL_new(ssl_ctx);
                    if (ssl != nullptr && !tls_host.empty())
                    {
                        X509_STORE *store = X509_STORE_new();
                        in_addr ipv4{};
                        in6_addr ipv6{};
                        const bool ip_host = inet_pton(AF_INET, tls_host.c_str(), &ipv4) == 1 ||
                                             inet_pton(AF_INET6, tls_host.c_str(), &ipv6) == 1;
                        const bool configured = store != nullptr &&
                            (tls_ca_file.empty() ? X509_STORE_set_default_paths(store) :
                             X509_STORE_load_file(store, tls_ca_file.c_str())) == 1 &&
                            SSL_set1_verify_cert_store(ssl, store) == 1 &&
                            (ip_host ? X509_VERIFY_PARAM_set1_ip_asc(SSL_get0_param(ssl), tls_host.c_str()) == 1 :
                             (SSL_set1_host(ssl, tls_host.c_str()) == 1 &&
                              SSL_set_tlsext_host_name(ssl, tls_host.c_str()) == 1));
                        X509_STORE_free(store);
                        if (!configured)
                        {
                            SSL_free(ssl); ssl = nullptr;
                        }
                        else SSL_set_verify(ssl, SSL_VERIFY_PEER, nullptr);
                    }
                    if (ssl == nullptr || SSL_set_fd(ssl, sockfd) != 1 ||
                        SSL_connect(ssl) != 1)
                    {
                        if (ssl != nullptr) { SSL_free(ssl); ssl = nullptr; }
                        close(sockfd);
                        sockfd = -1;
                        continue;
                    }
                }
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

    ssize_t Send(const std::string &buf, int flags = 0)
    {
        if (ssl != nullptr)
        {
            if (flags != 0)
            {
                throw std::invalid_argument("TLS Send does not support flags");
            }
            const size_t length = std::min(buf.size(),
                                           static_cast<size_t>(std::numeric_limits<int>::max()));
            return SSL_write(ssl, buf.data(), static_cast<int>(length));
        }
        ssize_t bytes_sent = send(sockfd, buf.c_str(), buf.length(), flags);
        return bytes_sent;
    }

    std::string Receive(int flags = 0, size_t max_length = 4096)
    {
        if (max_length == 0)
        {
            return "";
        }
        std::string buffer(max_length, '\0');
        ssize_t bytes_received;
        if (ssl != nullptr)
        {
            const int length = static_cast<int>(std::min(
                buffer.size(), static_cast<size_t>(std::numeric_limits<int>::max())));
            if (flags == MSG_PEEK)
            {
                bytes_received = SSL_peek(ssl, buffer.data(), length);
            }
            else
            {
                if (flags != 0)
                {
                    throw std::invalid_argument("TLS Receive supports only MSG_PEEK");
                }
                bytes_received = SSL_read(ssl, buffer.data(), length);
            }
        }
        else
        {
            bytes_received = recv(sockfd, buffer.data(), buffer.size(), flags);
        }
        if (bytes_received > 0)
        {
            buffer.resize(bytes_received);
            return buffer;
        }
        return "";
    }
static SSL_CTX *ssl_ctx;
private:
    static bool ApplyIoTimeout(int fd, int timeout_ms)
    {
        const struct timeval tv{timeout_ms / 1000, (timeout_ms % 1000) * 1000};
        return setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == 0 &&
               setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) == 0;
    }
    int sockfd = -1;
    
    SSL *ssl = nullptr;
    int io_timeout_ms = 0;
    std::string tls_host;
    std::string tls_ca_file;
};

#endif // SOCKET_H
