#pragma once

#include <algorithm>
#include <cctype>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

struct ParseResult {
  bool success = false;
  size_t bytes_consumed = 0;
};

class HttpRequest {
public:
  std::string method, path, version, body;
  std::map<std::string, std::string> headers;
  std::map<std::string, std::string> query_params;
  std::map<std::string, std::string> path_params;

  std::string raw_path;

  ParseResult parse(std::string_view raw) {
    ParseResult result;
    size_t pos = 0;

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

    raw_path =
        std::string(request_line.substr(space1 + 1, space2 - space1 - 1));
    parse_path_and_query(raw_path);

    version = std::string(request_line.substr(space2 + 1));

    pos = line_end + 2;

    while (pos < raw.size()) {
      const size_t header_line_end = raw.find("\r\n", pos);
      if (header_line_end == std::string_view::npos) {
        return result;
      }

      const std::string_view line = raw.substr(pos, header_line_end - pos);
      if (line.empty()) {
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

    size_t content_length = 0;
    const auto it = headers.find("Content-Length");
    if (it != headers.end()) {
      try {
        content_length = std::stoul(it->second);
      } catch (const std::invalid_argument &) {
        return result;
      } catch (const std::out_of_range &) {
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

  std::string get_query_param(const std::string &name,
                              const std::string &default_value = "") const {
    auto it = query_params.find(name);
    return (it != query_params.end()) ? it->second : default_value;
  }

  bool has_query_param(const std::string &name) const {
    return query_params.find(name) != query_params.end();
  }

  std::string get_path_param(const std::string &name,
                             const std::string &default_value = "") const {
    auto it = path_params.find(name);
    return (it != path_params.end()) ? it->second : default_value;
  }

  bool has_path_param(const std::string &name) const {
    return path_params.find(name) != path_params.end();
  }

private:
  void parse_path_and_query(const std::string &full_path) {
    size_t question_mark = full_path.find('?');

    if (question_mark == std::string::npos) {
      path = url_decode(full_path);
      return;
    }

    path = url_decode(full_path.substr(0, question_mark));
    std::string query_string = full_path.substr(question_mark + 1);

    parse_query_string(query_string);
  }

  void parse_query_string(const std::string &query_string) {
    size_t pos = 0;

    while (pos < query_string.length()) {
      size_t amp_pos = query_string.find('&', pos);
      if (amp_pos == std::string::npos) {
        amp_pos = query_string.length();
      }

      std::string pair = query_string.substr(pos, amp_pos - pos);

      size_t eq_pos = pair.find('=');
      if (eq_pos != std::string::npos) {
        std::string key = url_decode(pair.substr(0, eq_pos));
        std::string value = url_decode(pair.substr(eq_pos + 1));
        query_params[key] = value;
      } else if (!pair.empty()) {
        query_params[url_decode(pair)] = "";
      }

      pos = amp_pos + 1;
    }
  }

  static std::string url_decode(const std::string &str) {
    std::string result;
    result.reserve(str.length());

    for (size_t i = 0; i < str.length(); ++i) {
      if (str[i] == '%' && i + 2 < str.length()) {
        int hex_value;
        std::istringstream(str.substr(i + 1, 2)) >> std::hex >> hex_value;
        result += static_cast<char>(hex_value);
        i += 2;
      } else if (str[i] == '+') {
        result += ' ';
      } else {
        result += str[i];
      }
    }

    return result;
  }
};
