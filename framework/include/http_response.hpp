#pragma once

#include <map>
#include <string>
#include <vector>

/**
 * HttpResponse Class
 * Handles the construction of a valid HTTP/1.1 response string.
 */
class HttpResponse {
public:
  // Basic HTTP components
  int status_code = 200;
  std::string status_message = "OK";
  std::string body;
  std::map<std::string, std::string> headers;

  /**
   * Sets the response body and automatically calculates the Content-Length
   * header. Defaults to text/plain if no content_type is provided.
   */
  void set_body(const std::string &content,
                const std::string &content_type = "text/plain") {
    body = content;
    headers["Content-Type"] = content_type;
    // The Content-Length is essential for the client to know when the message
    // ends
    headers["Content-Length"] = std::to_string(body.length());
  }

  /**
   * Helper to set common status codes quickly
   */
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

  /**
   * Converts the object into a raw string ready to be sent over a socket.
   * Ensures RFC-compliant CRLF (\r\n) line endings.
   */
  std::string to_string() {
    // 1. Start with the Status Line
    std::string res = "HTTP/1.1 " + std::to_string(status_code) + " " +
                      status_message + "\r\n";

    // 2. Add default headers if they haven't been set manually
    if (headers.find("Server") == headers.end()) {
      headers["Server"] = "WebServer++/1.0";
    }
    if (headers.find("Connection") == headers.end()) {
      headers["Connection"] = "close";
    }

    // 3. Append all headers
    for (auto const &[key, val] : headers) {
      res += key + ": " + val + "\r\n";
    }

    // 4. The Critical Blank Line (separates headers from body)
    res += "\r\n";

    // 5. Append the body content
    res += body;

    return res;
  }
};
