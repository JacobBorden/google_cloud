#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include <string>
#include <string_view>
#include <algorithm>
#include <cctype>

inline std::string extract_header(std::string_view response, std::string_view header_name) {
    auto it = response.begin();
    while (it != response.end()) {
        auto line_end = std::find(it, response.end(), '\n');
        std::string_view line(response.data() + std::distance(response.begin(), it), std::distance(it, line_end));

        if (!line.empty() && line.back() == '\r') {
            line.remove_suffix(1);
        }

        if (line.empty()) {
            break; // End of headers
        }

        auto colon_pos = line.find(':');
        if (colon_pos != std::string_view::npos) {
            std::string_view current_header = line.substr(0, colon_pos);
            if (current_header.size() == header_name.size()) {
                bool match = true;
                for (size_t i = 0; i < current_header.size(); ++i) {
                    if (std::tolower(static_cast<unsigned char>(current_header[i])) !=
                        std::tolower(static_cast<unsigned char>(header_name[i]))) {
                        match = false;
                        break;
                    }
                }
                if (match) {
                    std::string_view value = line.substr(colon_pos + 1);
                    while (!value.empty() && (value.front() == ' ' || value.front() == '\t')) {
                        value.remove_prefix(1);
                    }
                    while (!value.empty() && (value.back() == ' ' || value.back() == '\t')) {
                        value.remove_suffix(1);
                    }
                    return std::string(value);
                }
            }
        }

        if (line_end == response.end()) {
            break;
        }
        it = line_end + 1;
    }
    return "";
}

#endif // HTTP_PARSER_H
