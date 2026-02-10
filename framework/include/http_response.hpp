#pragma once

#include <map>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>

static constexpr int KEEPALIVE_TIMEOUT = 5;
static constexpr int KEEPALIVE_MAX = 100;

class HttpResponse {
public:
  int status_code = 200;
  std::string status_message = "OK";
  std::string body;
  std::map<std::string, std::string> headers;
  bool keep_alive = true;

  HttpResponse() {}

  void set_body(const std::string &content,
                const std::string &content_type = "text/plain") {
    body = content;
    headers["Content-Type"] = content_type;
    headers["Content-Length"] = std::to_string(body.length());
  }

  void set_json(const nlohmann::json &j) {
    set_body(j.dump(4), "application/json");
  }

  void set_status(int code) {
    status_code = code;
    switch (code) {
    case 200:
      status_message = "OK";
      break;
    case 201:
      status_message = "Created";
      break;
    case 400:
      status_message = "Bad Request";
      break;
    case 404:
      status_message = "Not Found";
      break;
    case 500:
      status_message = "Internal Server Error";
      break;
    default:
      status_message = "Unknown Status";
      break;
    }
  }

  std::string to_string() {
    std::ostringstream oss;

    oss << "HTTP/1.1 " << status_code << " " << status_message << "\r\n";

    if (keep_alive) {
      headers["Connection"] = "keep-alive";
      headers["Keep-Alive"] = "timeout=" + std::to_string(KEEPALIVE_TIMEOUT) + ", max=" + std::to_string(KEEPALIVE_MAX);
    } else {
      headers["Connection"] = "close";
    }

    for (auto const &[key, val] : headers) {
      oss << key << ": " << val << "\r\n";
    }

    oss << "\r\n";

    oss << body;

    return oss.str();
  }
};
