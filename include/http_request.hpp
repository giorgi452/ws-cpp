#pragma once
#include <map>
#include <string>

class HttpRequest {
public:
  std::string method, path, version, body;
  std::map<std::string, std::string> headers;
  void parse(const std::string &raw_request);
};
