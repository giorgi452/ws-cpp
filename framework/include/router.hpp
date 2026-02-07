#pragma once
#include "http_request.hpp"
#include <functional>
#include <string>

using Handler = std::function<std::string(const HttpRequest &)>;

using RouteKey = std::pair<std::string, std::string>;

using RouteMap = std::map<RouteKey, Handler>;

class Router {
public:
  void get(std::string path, RouteMap *routes, Handler handler);
  void post(std::string path, RouteMap *routes, Handler handler);
};
