#ifndef _HTTPRESPONSE_
#define _HTTPRESPONSE_

#include <string>
#include <map>
#include <algorithm>
#include <cctype>

struct httpresponse {
    httpresponse(const std::string& rawResponse) {
        Parse(rawResponse);
    }

    std::string GetHeader(const std::string& name) const {
        std::string lowerName = name;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), [](unsigned char c) { return std::tolower(c); });
        auto it = headers.find(lowerName);
        if (it != headers.end()) {
            return it->second;
        }
        return "";
    }

    std::map<std::string, std::string> headers;
    std::string body;
    std::string version;
    int statusCode = 0;
    std::string statusMessage;

    void Parse(const std::string& rawResponse) {
        size_t start = 0;

        size_t endLine = rawResponse.find('\n', start);
        if (endLine != std::string::npos) {
            std::string line = rawResponse.substr(start, endLine - start);
            if (!line.empty() && line.back() == '\r') line.pop_back();
            // Parse status line
            size_t space1 = line.find(' ');
            if (space1 != std::string::npos) {
                version = line.substr(0, space1);
                size_t space2 = line.find(' ', space1 + 1);
                if (space2 != std::string::npos) {
                    try {
                        statusCode = std::stoi(line.substr(space1 + 1, space2 - space1 - 1));
                    } catch(...) {}
                    statusMessage = line.substr(space2 + 1);
                }
            }
            start = endLine + 1;
        }

        while (true) {
            endLine = rawResponse.find('\n', start);
            if (endLine == std::string::npos) break;

            std::string line = rawResponse.substr(start, endLine - start);
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }

            if (line.empty()) {
                start = endLine + 1;
                break;
            }

            size_t colon = line.find(':');
            if (colon != std::string::npos) {
                std::string key = line.substr(0, colon);

                auto keyStart = std::find_if_not(key.begin(), key.end(), [](unsigned char ch) { return std::isspace(ch); });
                auto keyEnd = std::find_if_not(key.rbegin(), key.rend(), [](unsigned char ch) { return std::isspace(ch); }).base();
                if (keyStart < keyEnd) {
                    key = std::string(keyStart, keyEnd);
                    std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) { return std::tolower(c); });

                    std::string value = line.substr(colon + 1);

                    auto valStart = std::find_if_not(value.begin(), value.end(), [](unsigned char ch) { return std::isspace(ch); });
                    auto valEnd = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) { return std::isspace(ch); }).base();
                    if (valStart < valEnd) {
                        headers[key] = std::string(valStart, valEnd);
                    } else {
                        headers[key] = "";
                    }
                }
            }
            start = endLine + 1;
        }
        if (start < rawResponse.length()) {
            body = rawResponse.substr(start);
        }
    }
};

#endif
