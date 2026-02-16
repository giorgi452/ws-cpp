#pragma once

#include "http_request.hpp"
#include "http_response.hpp"

class MainController {
public:
  static HttpResponse handle(const HttpRequest &req) {
    HttpResponse res;
    res.set_body("Hello World!");
    return res;
  }
};
