#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H

#include <string>
#include <map>
#include <sstream>
#include <algorithm>
#include <cctype>

struct HttpResponse {
    int statusCode = 0;
    std::map<std::string, std::string> headers;
    std::string body;

    static HttpResponse Parse(const std::string& response) {
        HttpResponse res;
        size_t headerEnd = response.find("\r\n\r\n");
        size_t bodyStartOffset = 4;
        if (headerEnd == std::string::npos) {
            headerEnd = response.find("\n\n");
            bodyStartOffset = 2;
        }

        std::string headerPart;
        if (headerEnd != std::string::npos) {
            headerPart = response.substr(0, headerEnd);
            res.body = response.substr(headerEnd + bodyStartOffset);
        } else {
            headerPart = response;
        }

        std::istringstream stream(headerPart);
        std::string line;

        // Parse status line
        if (std::getline(stream, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            size_t space1 = line.find(' ');
            if (space1 != std::string::npos) {
                size_t space2 = line.find(' ', space1 + 1);
                if (space2 != std::string::npos) {
                    try {
                        res.statusCode = std::stoi(line.substr(space1 + 1, space2 - space1 - 1));
                    } catch (...) {
                        // ignore parse error
                    }
                }
            }
        }

        // Parse headers
        while (std::getline(stream, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.empty()) break;

            size_t colon = line.find(':');
            if (colon != std::string::npos) {
                std::string name = line.substr(0, colon);
                std::string value = line.substr(colon + 1);

                // lowercase the name
                std::transform(name.begin(), name.end(), name.begin(),
                               [](unsigned char c){ return std::tolower(c); });

                // trim leading/trailing whitespace from value
                size_t first = value.find_first_not_of(" \t");
                if (first == std::string::npos) {
                    value = "";
                } else {
                    size_t last = value.find_last_not_of(" \t");
                    value = value.substr(first, (last - first + 1));
                }

                res.headers[name] = value;
            }
        }

        return res;
    }
};

#endif // HTTPRESPONSE_H
