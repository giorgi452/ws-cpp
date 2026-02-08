#include "include/routes.hpp"
#include "framework/include/router.hpp"
#include "http_request.hpp"
#include "http_response.hpp"

std::string setup_router(HttpRequest request) {
  Router router;

  router.get("/", [](const HttpRequest &req) {
    HttpResponse res;
    res.set_body("Welcome Home!");
    return res.to_string();
  });

  router.post("/login", [](const HttpRequest &req) {
    HttpResponse res;
    res.status_code = 201;
    res.set_body("Logged in as: " + req.body);
    return res.to_string();
  });

  HttpResponse response = router.handle(request);
  return response.to_string();
}
