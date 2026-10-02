#ifndef _HTTPREQUEST_
#define _HTTPREQUEST_
#include <algorithm>
#include <cctype>
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
        headers.push_back({name, value});
    }
    std::string ToString(const std::string& host, const std::string& path) const {
        std::string request;
        request += method + " " + path + " HTTP/1.1\r\n";
        request += "Host: " + host + "\r\n";

        bool has_connection = false;
        for (const auto& header : headers) {
            request += header.first + ": " + header.second + "\r\n";
            if (header.first.size() == 10 &&
                std::equal(header.first.begin(), header.first.end(),
                           "Connection", [](unsigned char a, unsigned char b) {
                               return std::tolower(a) == std::tolower(b);
                           })) {
                has_connection = true;
            }
        }

        if (!has_connection) {
            request += "Connection: close\r\n";
        }

        request += "\r\n";
        request += body;
        return request;
    }
    std::string body;
};

#endif
