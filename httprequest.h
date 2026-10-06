#ifndef _HTTPREQUEST_
#define _HTTPREQUEST_
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
struct httprequest
{
    std::string method = "GET";
    std::vector<std::pair<std::string, std::string>> headers;

    httprequest() = default;

    httprequest(const std::string& content_type, const std::string& body_content){
        method = "POST";
        headers.push_back({"Content-Type", content_type});
        headers.push_back({"Content-Length", std::to_string(body_content.length())});
        body = body_content;
    }

    void AddHeader(const std::string& name, const std::string& value) {
        ValidateHeader(name, value);
        headers.push_back({name, value});
    }
    std::string ToString(const std::string& host, const std::string& path) const {
        ValidateToken(method);
        ValidateHeader("Host", host);
        if (path.empty() || std::any_of(path.begin(), path.end(), [](unsigned char ch) {
                return ch <= 0x20 || ch == 0x7f;
            })) {
            throw std::invalid_argument("Invalid HTTP request target");
        }
        std::string request;
        request += method + " " + path + " HTTP/1.1\r\n";
        request += "Host: " + host + "\r\n";

        bool has_connection = false;
        bool has_content_length = false;
        bool has_transfer_encoding = false;
        const auto is_header = [](const std::string& name, const char* expected,
                                  std::size_t length) {
            return name.size() == length &&
                   std::equal(name.begin(), name.end(), expected,
                              [](unsigned char a, unsigned char b) {
                                  return std::tolower(a) == std::tolower(b);
                              });
        };
        for (const auto& header : headers) {
            ValidateHeader(header.first, header.second);
            request += header.first + ": " + header.second + "\r\n";
            has_connection |= is_header(header.first, "Connection", 10);
            has_content_length |= is_header(header.first, "Content-Length", 14);
            has_transfer_encoding |= is_header(header.first, "Transfer-Encoding", 17);
        }

        if (!has_connection) {
            request += "Connection: close\r\n";
        }
        if (!body.empty() && !has_content_length && !has_transfer_encoding) {
            request += "Content-Length: " + std::to_string(body.size()) + "\r\n";
        }

        request += "\r\n";
        request += body;
        return request;
    }
    std::string body;
private:
    static void ValidateToken(const std::string& token) {
        const std::string punctuation = "!#$%&'*+-.^_`|~";
        if (token.empty() || std::any_of(token.begin(), token.end(),
            [&punctuation](unsigned char ch) {
                return !((ch >= 'A' && ch <= 'Z') ||
                         (ch >= 'a' && ch <= 'z') ||
                         (ch >= '0' && ch <= '9') ||
                         punctuation.find(ch) != std::string::npos);
            })) {
            throw std::invalid_argument("Invalid HTTP token");
        }
    }
    static void ValidateHeader(const std::string& name, const std::string& value) {
        ValidateToken(name);
        if (std::any_of(value.begin(), value.end(), [](unsigned char ch) {
                return (ch < 0x20 && ch != '\t') || ch == 0x7f;
            })) {
            throw std::invalid_argument("Invalid HTTP header value");
        }
    }
};

#endif
