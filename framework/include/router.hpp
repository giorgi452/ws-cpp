#pragma once

#include "http_request.hpp"
#include "http_response.hpp"
#include <functional>
#include <string>

using Handler = std::function<HttpResponse(const HttpRequest &)>;

using RouteKey = std::pair<std::string, std::string>;

using RouteMap = std::map<RouteKey, Handler>;

class Router {
public:
  RouteMap routes;

  void get(std::string path, Handler handler) {
    routes[{"GET", path}] = handler;
  };

  void post(std::string path, Handler handler) {
    routes[{"POST", path}] = handler;
  };

  HttpResponse handle(const HttpRequest &req) {
    auto it = routes.find({req.method, req.path});
    if (it != routes.end()) {
      return it->second(req);
    }

    // Default 404
    HttpResponse res;
    res.set_status(404);
    res.status_message = "Not Found";
    res.set_body("Page not found");
    return res;
  }
};
