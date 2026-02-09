#pragma once

#include <algorithm>
#include <map>
#include <sstream>
#include <string>

class HttpRequest {
public:
  std::string method, path, version, body;
  std::map<std::string, std::string> headers;

  void parse(const std::string &raw_request) {
    std::istringstream stream(raw_request);
    std::string line;

    if (std::getline(stream, line)) {
      std::istringstream line_stream(line);
      line_stream >> method >> path >> version;
    }

    while (std::getline(stream, line) && line != "\r") {
      size_t colon_pos = line.find(':');
      if (colon_pos != std::string::npos) {
        std::string key = line.substr(0, colon_pos);
        std::string value = line.substr(colon_pos + 2);

        if (!value.empty() && value.back() == '\r')
          value.pop_back();
        headers[key] = value;
      }
    }

    if (headers.count("Content-Length")) {
      int length = std::stoi(headers["Content-Length"]);
      char *body_buf = new char[length + 1];
      stream.read(body_buf, length);
      body_buf[length] = '\0';
      body = body_buf;
      delete[] body_buf;
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
