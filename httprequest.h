#ifndef _HTTPREQUEST_
#define _HTTPREQUEST_
#include <string>
#include <vector>
struct httprequest
{
    httprequest(const std::string& content_type, const std::string& body_content){
        contentType = content_type;
        contentLength = body_content.length();
        body = body_content;
    }

    void AddHeader(const std::string& name, const std::string& value) {
        headers.push_back({name, value});
    }
    std::string ToString(const std::string& host, const std::string& path){
        std::string request;
        request += "POST " + path + " HTTP/1.1\r\n";
        request += "Host: " + host + "\r\n";
        request += "Content-Type: " + contentType + "\r\n";
        request += "Content-Length: " + std::to_string(contentLength) + "\r\n";
        request += "Connection: close\r\n";

        for (const auto& header : headers) {
            request += header.first + ": " + header.second + "\r\n";
        }
        request += "\r\n";

        request += body;
        return request;
    }
    std::string contentType;
    int contentLength;
    std::string body;
    std::vector<std::pair<std::string, std::string>> headers;
};

#endif