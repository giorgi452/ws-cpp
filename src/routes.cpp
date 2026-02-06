#include "routes.hpp"

void setup_routes(RouteMap &routes) {
  routes[{"GET", "/"}] = [](const HttpRequest &req) {
    return "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: "
           "13\r\n\r\nWelcome Home!";
  };

  routes[{"GET", "/about"}] = [](const HttpRequest &req) {
    return "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: "
           "17\r\n\r\nAbout this server";
  };

  routes[{"POST", "/"}] = [](const HttpRequest &req) {
    std::string body = "Received your data: " + req.body;
    return "HTTP/1.1 201 Created\r\n"
           "Content-Type: text/plain\r\n"
           "Content-Length: " +
           std::to_string(body.length()) + "\r\n\r\n" + body;
  };
}
