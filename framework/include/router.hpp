#pragma once

#include "http_request.hpp"
#include "http_response.hpp"
#include <functional>
#include <regex>
#include <string>
#include <vector>

using Handler = std::function<HttpResponse(const HttpRequest &)>;

struct RoutePattern {
  std::string method;
  std::string pattern;
  std::regex regex_pattern;
  std::vector<std::string> param_names;
  Handler handler;

  RoutePattern(std::string_view m, std::string_view p, Handler h)
      : method(m), pattern(p), handler(std::move(h)) {
    compile_pattern();
  }

private:
  void compile_pattern() {
    std::string regex_str;
    regex_str.reserve(pattern.length() * 2);
    regex_str = "^";
    param_names.clear();

    size_t pos = 0;
    while (pos < pattern.length()) {
      size_t param_start = pattern.find(':', pos);

      if (param_start == std::string::npos) {
        std::string literal = pattern.substr(pos);
        regex_str += regex_escape(literal);
        break;
      }

      if (param_start > pos) {
        std::string literal = pattern.substr(pos, param_start - pos);
        regex_str += regex_escape(literal);
      }

      size_t param_end = param_start + 1;
      while (param_end < pattern.length() &&
             (std::isalnum(pattern[param_end]) || pattern[param_end] == '_')) {
        param_end++;
      }

      std::string param_name =
          pattern.substr(param_start + 1, param_end - param_start - 1);
      param_names.push_back(param_name);

      regex_str += "([^/]+)";

      pos = param_end;
    }

    regex_str += "$";
    regex_pattern = std::regex(regex_str, std::regex::optimize);
  }

  static std::string regex_escape(const std::string &str) {
    static const std::string special_chars = "\\^$.|?*+()[]{}";
    std::string result;
    result.reserve(str.length() * 2);

    for (char c : str) {
      if (special_chars.find(c) != std::string::npos) {
        result += '\\';
      }
      result += c;
    }
    return result;
  }
};

class Router {
public:
  void get(std::string_view path, Handler handler) {
    routes.emplace_back("GET", path, std::move(handler));
  }

  void post(std::string_view path, Handler handler) {
    routes.emplace_back("POST", path, std::move(handler));
  }

  void put(std::string_view path, Handler handler) {
    routes.emplace_back("PUT", path, std::move(handler));
  }

  void del(std::string_view path, Handler handler) {
    routes.emplace_back("DELETE", path, std::move(handler));
  }

  void patch(std::string_view path, Handler handler) {
    routes.emplace_back("PATCH", path, std::move(handler));
  }

  HttpResponse handle(HttpRequest &req) const {
    for (const auto &route : routes) {
      if (route.method != req.method) {
        continue;
      }

      std::smatch match;
      if (std::regex_match(req.path, match, route.regex_pattern)) {
        req.path_params.clear();
        for (size_t i = 0; i < route.param_names.size() && i + 1 < match.size();
             ++i) {
          req.path_params[route.param_names[i]] = match[i + 1].str();
        }

        return route.handler(req);
      }
    }

    HttpResponse res;
    res.set_status(404);
    res.status_message = "Not Found";
    res.set_body("Page not found");
    res.keep_alive = req.wants_keep_alive();
    return res;
  }

private:
  std::vector<RoutePattern> routes;
};
