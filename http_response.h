#pragma once

#include <optional>
#include <regex>
#include <string>

namespace http {

inline std::optional<std::string> ExtractLocationHeader(
    const std::string &response) {
  const auto headers_end = response.find("\r\n\r\n");
  if (headers_end == std::string::npos)
    return std::nullopt;
  static const std::regex location_regex(
      "\r\nLocation:[ \t]*([^ \t\r\n][^\r\n]*)", std::regex::icase);
  std::smatch match;
  const auto headers = response.substr(0, headers_end);
  if (!std::regex_search(headers, match, location_regex))
    return std::nullopt;
  auto location = match[1].str();
  const auto value_end = location.find_last_not_of(" \t");
  location.erase(value_end + 1);
  return location;
}

}  // namespace http
