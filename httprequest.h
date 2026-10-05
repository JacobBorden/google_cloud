#ifndef _HTTPREQUEST_
#define _HTTPREQUEST_
#include <string>
#include <vector>

struct httprequest
{
    std::string method = "POST";
    std::vector<std::pair<std::string, std::string>> headers;
    std::string body;

    httprequest() = default;

    httprequest(const std::string& content_type, const std::string& body_content){
        SetHeader("Content-Type", content_type);
        SetHeader("Content-Length", std::to_string(body_content.length()));
        body = body_content;
    }

    void SetHeader(const std::string& key, const std::string& value) {
        for (auto& header : headers) {
            if (header.first == key) {
                header.second = value;
                return;
            }
        }
        headers.push_back({key, value});
    }

    std::string ToString(const std::string& host, const std::string& path) const {
        std::string request;
        request += method + " " + path + " HTTP/1.1\r\n";
        request += "Host: " + host + "\r\n";
        for (const auto& header : headers) {
            request += header.first + ": " + header.second + "\r\n";
        }
        bool has_connection = false;
        for (const auto& header : headers) {
            if (header.first == "Connection") {
                has_connection = true;
                break;
            }
        }
        if (!has_connection) {
            request += "Connection: close\r\n";
        }
        request += "\r\n";
        request += body;
        return request;
    }
};

#endif
