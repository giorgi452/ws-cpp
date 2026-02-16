#include "include/routes.hpp"
#include "controllers/main_controller.hpp"
#include "framework/include/http_request.hpp"
#include "framework/include/http_response.hpp"
#include "framework/include/router.hpp"

using nlohmann::json;

// Initialize router once at startup
Router &get_global_router() {
  static Router router = []() {
    Router r;

    r.get("/", MainController::handle);

    return r;
  }();

  return router;
}

HttpResponse setup_router(HttpRequest &request) {
  return get_global_router().handle(request);
}
