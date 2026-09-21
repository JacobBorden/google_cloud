#pragma once

#include <optional>
#include <regex>
#include <string>

namespace http {

inline std::optional<std::string> ExtractLocationHeader(
    const std::string &response) {
  static const std::regex location_regex(
      "\r\nLocation:[ \t]*([^ \t\r\n][^\r\n]*)", std::regex::icase);
  std::smatch match;
  if (!std::regex_search(response, match, location_regex))
    return std::nullopt;
  auto location = match[1].str();
  const auto value_end = location.find_last_not_of(" \t");
  location.erase(value_end + 1);
  return location;
}

}  // namespace http
