#pragma once

#include "http_request.hpp"
#include "http_response.hpp"
#include <functional>
#include <string>
#include <unordered_map>

using Handler = std::function<HttpResponse(const HttpRequest &)>;

struct RouteKey {
  std::string method;
  std::string path;

  RouteKey(std::string_view m, std::string_view p) : method(m), path(p) {}

  bool operator==(const RouteKey &other) const {
    return method == other.method && path == other.path;
  }
  
  bool operator!=(const RouteKey &other) const {
    return !(*this == other);
  }
};

struct RouteKeyHash {
  std::size_t operator()(const RouteKey &key) const {
    return std::hash<std::string>()(key.method) ^
           (std::hash<std::string>()(key.path) << 1);
  }
};

using RouteMap = std::unordered_map<RouteKey, Handler, RouteKeyHash>;

class Router {
public:
  RouteMap routes;

  void get(std::string_view path, Handler handler) {
    routes.emplace(RouteKey("GET", path), std::move(handler));
  }

  void post(std::string_view path, Handler handler) {
    routes.emplace(RouteKey("POST", path), std::move(handler));
  }

  void put(std::string_view path, Handler handler) {
    routes.emplace(RouteKey("PUT", path), std::move(handler));
  }

  void del(std::string_view path, Handler handler) {
    routes.emplace(RouteKey("DELETE", path), std::move(handler));
  }

  HttpResponse handle(const HttpRequest &req) {
    RouteKey key(req.method, req.path);

    auto it = routes.find(key);
    if (it != routes.end()) {
      return it->second(req);
    }

    // Default 404
    HttpResponse res;
    res.set_status(404);
    res.status_message = "Not Found";
    res.set_body("Page not found");
    res.keep_alive = req.wants_keep_alive(); // Match client preference
    return res;
  }
};
