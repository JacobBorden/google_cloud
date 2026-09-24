#ifndef HTTP_UTILS_H
#define HTTP_UTILS_H

#include <string>
#include <cctype>

inline std::string extractHeader(const std::string& response, const std::string& headerName) {
    std::string lowerHeaderName = headerName;
    for (char& c : lowerHeaderName) {
        c = std::tolower(static_cast<unsigned char>(c));
    }

    size_t startPos = 0;
    while (true) {
        size_t lineEnd = response.find("\r\n", startPos);
        if (lineEnd == std::string::npos) break;

        std::string line = response.substr(startPos, lineEnd - startPos);
        startPos = lineEnd + 2;

        if (line.empty()) break; // End of headers

        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string currentHeader = line.substr(0, colonPos);
            for (char& c : currentHeader) {
                c = std::tolower(static_cast<unsigned char>(c));
            }

            if (currentHeader == lowerHeaderName) {
                size_t valueStart = colonPos + 1;
                while (valueStart < line.length() && (line[valueStart] == ' ' || line[valueStart] == '\t')) {
                    valueStart++;
                }
                return line.substr(valueStart);
            }
        }
    }
    return "";
}

#endif // HTTP_UTILS_H
