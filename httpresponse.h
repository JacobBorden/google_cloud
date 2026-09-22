#ifndef _HTTPRESPONSE_
#define _HTTPRESPONSE_

#include <string>
#include <map>
#include <algorithm>
#include <cctype>
#include <sstream>

struct httpresponse
{
    std::string version;
    int status_code = 0;
    std::string status_message;
    std::map<std::string, std::string> headers;
    std::string body;

    static httpresponse Parse(const std::string& raw_response) {
        httpresponse response;
        std::istringstream stream(raw_response);
        std::string line;

        if (std::getline(stream, line) && !line.empty()) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            std::istringstream status_stream(line);
            status_stream >> response.version >> response.status_code;
            std::getline(status_stream >> std::ws, response.status_message);
        }

        while (std::getline(stream, line) && line != "\r" && !line.empty()) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            size_t colon_pos = line.find(':');
            if (colon_pos != std::string::npos) {
                std::string key = line.substr(0, colon_pos);
                std::string value = line.substr(colon_pos + 1);

                auto key_start = std::find_if_not(key.begin(), key.end(), [](unsigned char c) { return std::isspace(c); });
                auto key_end = std::find_if_not(key.rbegin(), key.rend(), [](unsigned char c) { return std::isspace(c); }).base();
                key = (key_start < key_end) ? std::string(key_start, key_end) : "";

                auto val_start = std::find_if_not(value.begin(), value.end(), [](unsigned char c) { return std::isspace(c); });
                auto val_end = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char c) { return std::isspace(c); }).base();
                value = (val_start < val_end) ? std::string(val_start, val_end) : "";

                std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c){ return std::tolower(c); });

                response.headers[key] = value;
            }
        }

        std::ostringstream body_stream;
        body_stream << stream.rdbuf();
        response.body = body_stream.str();

        return response;
    }
};

#endif
