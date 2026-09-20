#ifndef _HTTPRESPONSE_
#define _HTTPRESPONSE_
#include <string>
#include <map>
#include <cctype>
#include <algorithm>

struct CaseInsensitiveCompare {
    bool operator()(const std::string& a, const std::string& b) const {
        return std::lexicographical_compare(
            a.begin(), a.end(), b.begin(), b.end(),
            [](unsigned char ac, unsigned char bc) { return std::tolower(ac) < std::tolower(bc); }
        );
    }
};

struct HttpResponse {
    int statusCode;
    std::string statusMessage;
    std::map<std::string, std::string, CaseInsensitiveCompare> headers;
    std::string body;

    static HttpResponse Parse(const std::string& raw_response) {
        HttpResponse response;
        response.statusCode = 0;

        size_t pos = 0;
        size_t end = raw_response.find("\r\n");
        if (end == std::string::npos) return response;

        // Parse status line
        std::string statusLine = raw_response.substr(0, end);
        size_t space1 = statusLine.find(' ');
        if (space1 != std::string::npos) {
            size_t space2 = statusLine.find(' ', space1 + 1);
            if (space2 != std::string::npos) {
                try {
                    response.statusCode = std::stoi(statusLine.substr(space1 + 1, space2 - space1 - 1));
                } catch (...) {
                    response.statusCode = 0;
                }
                response.statusMessage = statusLine.substr(space2 + 1);
            }
        }

        pos = end + 2;

        // Parse headers
        while (pos < raw_response.length()) {
            end = raw_response.find("\r\n", pos);
            if (end == std::string::npos) break;

            if (end == pos) {
                // Empty line, end of headers
                pos = end + 2;
                break;
            }

            std::string line = raw_response.substr(pos, end - pos);
            size_t colon = line.find(':');
            if (colon != std::string::npos) {
                std::string name = line.substr(0, colon);
                size_t valStart = colon + 1;
                while (valStart < line.length() && (line[valStart] == ' ' || line[valStart] == '\t')) {
                    valStart++;
                }
                std::string value = line.substr(valStart);
                response.headers[name] = value;
            }
            pos = end + 2;
        }

        // Parse body
        if (pos < raw_response.length()) {
            response.body = raw_response.substr(pos);
        }

        return response;
    }
};
#endif
