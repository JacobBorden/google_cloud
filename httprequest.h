#ifndef _HTTPREQUEST_
#define _HTTPREQUEST_
#include <string>
struct httprequest
{
    httprequest(const std::string& content_type, const std::string& body_content){
        contentType = content_type;
        contentLength = body_content.length();
        body = body_content;
    }
    std::string ToString(const std::string& host, const std::string& path){
        std::string request;
        request += "POST " + path + " HTTP/1.1\r\n";
        request += "Host: " + host + "\r\n";
        request += "Content-Type: " + contentType + "\r\n";
        request += "Content-Length: " + std::to_string(contentLength) + "\r\n";
        request += "Connection: close\r\n";
        request += "\r\n";
        request += body;
        return request;
    }
    std::string contentType;
    int contentLength;
    std::string body;
};

#endif