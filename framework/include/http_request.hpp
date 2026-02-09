#pragma once

#include <algorithm>
#include <map>
#include <string>
#include <string_view>

class HttpRequest {
public:
  std::string method, path, version, body;
  std::map<std::string, std::string> headers;

  void parse(std::string_view raw_request) {
    size_t pos = 0;
    size_t end = raw_request.size();

    size_t line_end = raw_request.find("\r\n", pos);
    if (line_end == std::string_view::npos)
      return;

    std::string_view request_line = raw_request.substr(pos, line_end - pos);

    size_t space1 = request_line.find(' ');
    if (space1 != std::string_view::npos) {
      method = std::string(request_line.substr(0, space1));

      // Extract path
      size_t space2 = request_line.find(' ', space1 + 1);
      if (space2 != std::string_view::npos) {
        path =
            std::string(request_line.substr(space1 + 1, space2 - space1 - 1));
        version = std::string(request_line.substr(space2 + 1));
      }
    }

    pos = line_end + 2;

    while (pos < end) {
      line_end = raw_request.find("\r\n", pos);
      if (line_end == std::string_view::npos)
        break;

      std::string_view line = raw_request.substr(pos, line_end - pos);

      // Empty line signals end of headers
      if (line.empty()) {
        pos = line_end + 2;
        break;
      }

      // Parse header: "Key: Value"
      size_t colon = line.find(':');
      if (colon != std::string_view::npos) {
        std::string_view key = line.substr(0, colon);
        std::string_view value = line.substr(colon + 1);

        // Trim leading space from value
        while (!value.empty() && value[0] == ' ') {
          value.remove_prefix(1);
        }

        headers[std::string(key)] = std::string(value);
      }

      pos = line_end + 2;
    }

    auto it = headers.find("Content-Length");
    if (it != headers.end() && pos < end) {
      try {
        size_t content_length = std::stoul(it->second);
        if (pos + content_length <= end) {
          body = std::string(raw_request.substr(pos, content_length));
        }
      } catch (...) {
        // Invalid Content-Length, ignore
      }
    }
  }

  bool wants_keep_alive() const {
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
