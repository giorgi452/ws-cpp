#include "include/routes.hpp"
#include "framework/include/router.hpp"
#include "http_request.hpp"
#include "http_response.hpp"

using nlohmann::json;

std::string setup_router(HttpRequest request) {
  Router router;

  router.get("/", [](const HttpRequest &req) {
    HttpResponse res;
    res.set_body("Welcome Home!");
    return res;
  });

  router.post("/login", [](const HttpRequest &req) {
    HttpResponse res;
    res.status_code = 201;
    res.set_body("Logged in as: " + req.body);
    return res;
  });

  router.get("/api/status", [](const HttpRequest &req) {
    HttpResponse res;

    json data;
    data["status"] = "online";
    data["uptime"] = "24h";
    data["connections"] = 5;
    data["features"] = {"parsing", "routing", "json_support"};

    res.set_json(data);
    return res;
  });

  HttpResponse response = router.handle(request);
  return response.to_string();
}
