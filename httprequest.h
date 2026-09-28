#ifndef _HTTPREQUEST_
#define _HTTPREQUEST_
#include <string>
#include <cctype>

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

    static std::string GetHeader(const std::string& response, const std::string& headerName) {
        size_t pos = response.find("\r\n"); // end of request line
        if (pos == std::string::npos) return "";
        pos += 2;

        while (pos < response.length() && response.compare(pos, 2, "\r\n") != 0) {
            size_t colon_pos = response.find(':', pos);
            size_t end_line = response.find("\r\n", pos);
            if (end_line == std::string::npos) end_line = response.length();

            if (colon_pos != std::string::npos && colon_pos < end_line) {
                std::string current_header = response.substr(pos, colon_pos - pos);
                if (current_header.length() == headerName.length()) {
                    bool match = true;
                    for (size_t i = 0; i < current_header.length(); ++i) {
                        if (std::tolower(static_cast<unsigned char>(current_header[i])) != std::tolower(static_cast<unsigned char>(headerName[i]))) {
                            match = false;
                            break;
                        }
                    }
                    if (match) {
                        size_t val_start = colon_pos + 1;
                        while (val_start < end_line && std::isspace(static_cast<unsigned char>(response[val_start]))) {
                            val_start++;
                        }
                        size_t val_end = end_line;
                        while (val_end > val_start && std::isspace(static_cast<unsigned char>(response[val_end - 1]))) {
                            val_end--;
                        }
                        return response.substr(val_start, val_end - val_start);
                    }
                }
            }
            pos = end_line + 2; // move to start of next line
        }
        return "";
    }
};

#endif