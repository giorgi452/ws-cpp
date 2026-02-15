#include "include/routes.hpp"
#include "framework/include/http_request.hpp"
#include "framework/include/http_response.hpp"
#include "framework/include/router.hpp"

using nlohmann::json;

namespace {

// Root endpoint
HttpResponse handle_root(const HttpRequest &req) {
  HttpResponse res;
  res.set_body("Hello World!");
  return res;
}

// Login endpoint
HttpResponse handle_login(const HttpRequest &req) {
  HttpResponse res;
  res.set_status(201);
  res.set_body("Logged in as: " + req.body);
  return res;
}

// Status endpoint
HttpResponse handle_status(const HttpRequest &req) {
  HttpResponse res;

  json data;
  data["status"] = "online";
  data["uptime"] = "24h";
  data["connections"] = 5;
  data["features"] = {"parsing", "routing", "json_support", "url_params",
                      "query_params"};

  res.set_json(data);
  return res;
}

// GET /users/:id - Path parameter example
HttpResponse handle_get_user(const HttpRequest &req) {
  HttpResponse res;

  std::string user_id = req.get_path_param("id");

  json data;
  data["user_id"] = user_id;
  data["username"] = "user_" + user_id;
  data["email"] = "user" + user_id + "@example.com";

  res.set_json(data);
  return res;
}

// GET /users/:id/posts/:post_id - Multiple path parameters
HttpResponse handle_get_user_post(const HttpRequest &req) {
  HttpResponse res;

  std::string user_id = req.get_path_param("id");
  std::string post_id = req.get_path_param("post_id");

  json data;
  data["user_id"] = user_id;
  data["post_id"] = post_id;
  data["title"] = "Post " + post_id + " by User " + user_id;
  data["content"] = "This is the content of post " + post_id;

  res.set_json(data);
  return res;
}

// GET /search - Query parameter example
HttpResponse handle_search(const HttpRequest &req) {
  HttpResponse res;

  // Get query parameters with defaults
  std::string query = req.get_query_param("q", "");
  std::string page = req.get_query_param("page", "1");
  std::string limit = req.get_query_param("limit", "10");
  std::string sort = req.get_query_param("sort", "relevance");

  json data;
  data["query"] = query;
  data["page"] = page;
  data["limit"] = limit;
  data["sort"] = sort;
  data["results"] = json::array();

  // Example: Add mock results if query is provided
  if (!query.empty()) {
    for (int i = 1; i <= 3; ++i) {
      json item;
      item["id"] = i;
      item["title"] = "Result " + std::to_string(i) + " for '" + query + "'";
      item["score"] = 0.95 - (i * 0.1);
      data["results"].push_back(item);
    }
  }

  res.set_json(data);
  return res;
}

// GET /products/:category/:id - Path params + query params
HttpResponse handle_get_product(const HttpRequest &req) {
  HttpResponse res;

  // Path parameters
  std::string category = req.get_path_param("category");
  std::string product_id = req.get_path_param("id");

  // Query parameters
  std::string include_reviews = req.get_query_param("reviews", "false");
  std::string format = req.get_query_param("format", "full");

  json data;
  data["category"] = category;
  data["product_id"] = product_id;
  data["name"] = "Product " + product_id;
  data["format"] = format;

  if (include_reviews == "true") {
    data["reviews"] =
        json::array({{{"rating", 5}, {"comment", "Great product!"}},
                     {{"rating", 4}, {"comment", "Good value"}}});
  }

  res.set_json(data);
  return res;
}

// POST /users/:id/update - Update with path param
HttpResponse handle_update_user(const HttpRequest &req) {
  HttpResponse res;

  std::string user_id = req.get_path_param("id");

  json data;
  data["user_id"] = user_id;
  data["updated"] = true;
  data["body_received"] = req.body;

  res.set_status(200);
  res.set_json(data);
  return res;
}

// DELETE /users/:id - Delete with path param
HttpResponse handle_delete_user(const HttpRequest &req) {
  HttpResponse res;

  std::string user_id = req.get_path_param("id");

  json data;
  data["user_id"] = user_id;
  data["deleted"] = true;
  data["message"] = "User " + user_id + " deleted successfully";

  res.set_status(200);
  res.set_json(data);
  return res;
}

// GET /api/params - Debug endpoint to show all parameters
HttpResponse handle_debug_params(const HttpRequest &req) {
  HttpResponse res;

  json data;

  // Path parameters
  json path_params_json;
  for (const auto &[key, value] : req.path_params) {
    path_params_json[key] = value;
  }
  data["path_params"] = path_params_json;

  // Query parameters
  json query_params_json;
  for (const auto &[key, value] : req.query_params) {
    query_params_json[key] = value;
  }
  data["query_params"] = query_params_json;

  // Headers
  json headers_json;
  for (const auto &[key, value] : req.headers) {
    headers_json[key] = value;
  }
  data["headers"] = headers_json;

  // Request info
  data["method"] = req.method;
  data["path"] = req.path;
  data["raw_path"] = req.raw_path;
  data["version"] = req.version;
  data["body"] = req.body;

  res.set_json(data);
  return res;
}

// Initialize router once at startup
Router &get_global_router() {
  static Router router = []() {
    Router r;

    // Basic routes
    r.get("/", handle_root);
    r.post("/login", handle_login);
    r.get("/api/status", handle_status);

    // Path parameter routes
    r.get("/users/:id", handle_get_user);
    r.post("/users/:id/update", handle_update_user);
    r.del("/users/:id", handle_delete_user);
    r.get("/users/:id/posts/:post_id", handle_get_user_post);

    // Query parameter routes
    r.get("/search", handle_search);

    // Combined path + query parameters
    r.get("/products/:category/:id", handle_get_product);

    // Debug endpoint
    r.get("/api/params", handle_debug_params);

    return r;
  }();

  return router;
}

} // namespace

HttpResponse setup_router(HttpRequest &request) {
  return get_global_router().handle(request);
}
