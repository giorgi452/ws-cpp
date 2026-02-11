#pragma once

#include <algorithm>
#include <cctype>
#include <map>
#include <string>
#include <string_view>
#include <stdexcept>

struct ParseResult {
  bool success = false;
  size_t bytes_consumed = 0;
};

class HttpRequest {
public:
  std::string method, path, version, body;
  std::map<std::string, std::string> headers;

  ParseResult parse(std::string_view raw) {
    ParseResult result;
    size_t pos = 0;

    // 1. Request line
    const size_t line_end = raw.find("\r\n", pos);
    if (line_end == std::string_view::npos) {
      return result;
    }

    const std::string_view request_line = raw.substr(pos, line_end - pos);
    const size_t space1 = request_line.find(' ');
    if (space1 == std::string_view::npos) {
      return result;
    }

    method = std::string(request_line.substr(0, space1));

    const size_t space2 = request_line.find(' ', space1 + 1);
    if (space2 == std::string_view::npos) {
      return result;
    }

    path = std::string(request_line.substr(space1 + 1, space2 - space1 - 1));
    version = std::string(request_line.substr(space2 + 1));

    pos = line_end + 2;

    // 2. Headers
    while (pos < raw.size()) {
      const size_t header_line_end = raw.find("\r\n", pos);
      if (header_line_end == std::string_view::npos) {
        return result;
      }

      const std::string_view line = raw.substr(pos, header_line_end - pos);
      if (line.empty()) {
        // End of headers
        pos = header_line_end + 2;
        break;
      }

      const size_t colon = line.find(':');
      if (colon != std::string_view::npos && colon + 1 < line.size()) {
        std::string_view key = line.substr(0, colon);
        std::string_view value = line.substr(colon + 1);
        while (!value.empty() &&
               std::isspace(static_cast<unsigned char>(value[0]))) {
          value.remove_prefix(1);
        }

        headers[std::string(key)] = std::string(value);
      }

      pos = header_line_end + 2;
    }

    // 3. Body (if present)
    size_t content_length = 0;
    const auto it = headers.find("Content-Length");
    if (it != headers.end()) {
      try {
        content_length = std::stoul(it->second);
      } catch (const std::invalid_argument&) {
        return result;
      } catch (const std::out_of_range&) {
        return result;
      }
    }

    if (content_length > 0) {
      if (pos + content_length > raw.size()) {
        return result;
      }

      body = std::string(raw.substr(pos, content_length));
      pos += content_length;
    }

    result.success = !method.empty();
    result.bytes_consumed = pos;
    return result;
  }

  bool wants_keep_alive() const noexcept {
    auto it = headers.find("Connection");
    if (it != headers.end()) {
      std::string conn = it->second;
      std::transform(conn.begin(), conn.end(), conn.begin(),
                     [](unsigned char c) { return std::tolower(c); });
      return conn.find("keep-alive") != std::string::npos;
    }
    return version == "HTTP/1.1";
  }
};
