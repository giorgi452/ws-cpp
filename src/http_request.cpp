#include "http_request.hpp"
#include <sstream>

void HttpRequest::parse(const std::string &raw_request) {
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
      std::string value = line.substr(colon_pos + 2); // Skip ": "

      // Basic cleanup of carriage return
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
