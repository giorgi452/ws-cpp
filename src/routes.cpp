#include "include/routes.hpp"
#include "framework/include/http_request.hpp"
#include "framework/include/http_response.hpp"
#include "framework/include/router.hpp"

using nlohmann::json;

namespace {
HttpResponse handle_root(const HttpRequest &req) {
  HttpResponse res;
  res.set_body("Hello World!");
  return res;
}

HttpResponse handle_login(const HttpRequest &req) {
  HttpResponse res;
  res.set_status(201);
  res.set_body("Logged in as: " + req.body);
  return res;
}

HttpResponse handle_status(const HttpRequest &req) {
  HttpResponse res;

  json data;
  data["status"] = "online";
  data["uptime"] = "24h";
  data["connections"] = 5;
  data["features"] = {"parsing", "routing", "json_support"};

  res.set_json(data);
  return res;
}
} // namespace

HttpResponse setup_router(const HttpRequest &request) {
  Router router;

  router.get("/", handle_root);
  router.post("/login", handle_login);
  router.get("/api/status", handle_status);

  return router.handle(request);
}
