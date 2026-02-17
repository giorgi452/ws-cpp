#pragma once
#include "middleware.hpp"

#include <atomic>
#include <chrono>
#include <ctime>
#include <iostream>
#include <mutex>
#include <string>
#include <unordered_map>

namespace Middlewares {

// ---------------------------------------------------------------------------
// Logger
// Prints method, path, response status, and wall-clock duration to stdout.
// ---------------------------------------------------------------------------
inline Middleware logger() {
  return [](HttpRequest &req, Next next) -> HttpResponse {
    const auto start = std::chrono::steady_clock::now();
    HttpResponse res = next();
    const auto end = std::chrono::steady_clock::now();
    const auto ms =
        std::chrono::duration_cast<std::chrono::microseconds>(end - start)
            .count();

    // ISO-8601-ish timestamp
    std::time_t now = std::time(nullptr);
    char ts[32];
    std::strftime(ts, sizeof(ts), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&now));

    std::cout << ts << "  " << req.method << "  " << req.raw_path << "  "
              << res.status_code << "  " << ms << "μs\n";
    return res;
  };
}

// ---------------------------------------------------------------------------
// Request-ID
// Stamps every response with a monotonically-increasing X-Request-Id header.
// ---------------------------------------------------------------------------
inline Middleware request_id() {
  return [](HttpRequest &req, Next next) -> HttpResponse {
    static std::atomic<uint64_t> counter{1};
    const uint64_t id = counter.fetch_add(1, std::memory_order_relaxed);

    HttpResponse res = next();
    res.headers["X-Request-Id"] = std::to_string(id);
    return res;
  };
}

// ---------------------------------------------------------------------------
// CORS
// Adds Access-Control-* response headers and handles preflight OPTIONS.
//
// Parameters:
//   allowed_origins  – exact origin string, or "*" for any (default "*")
//   allowed_methods  – comma-separated list (default common verbs)
//   allowed_headers  – comma-separated list (default common headers)
//   max_age_seconds  – preflight cache duration in seconds (default 86400)
// ---------------------------------------------------------------------------
inline Middleware cors(const std::string &allowed_origins = "*",
                       const std::string &allowed_methods =
                           "GET, POST, PUT, DELETE, PATCH, OPTIONS",
                       const std::string &allowed_headers =
                           "Content-Type, Authorization, X-Request-Id",
                       int max_age_seconds = 86400) {
  return [=](HttpRequest &req, Next next) -> HttpResponse {
    // Preflight — respond immediately without hitting the route.
    if (req.method == "OPTIONS") {
      HttpResponse res;
      res.set_status(204);
      res.status_message = "No Content";
      res.headers["Access-Control-Allow-Origin"] = allowed_origins;
      res.headers["Access-Control-Allow-Methods"] = allowed_methods;
      res.headers["Access-Control-Allow-Headers"] = allowed_headers;
      res.headers["Access-Control-Max-Age"] = std::to_string(max_age_seconds);
      res.keep_alive = req.wants_keep_alive();
      return res;
    }

    HttpResponse res = next();
    res.headers["Access-Control-Allow-Origin"] = allowed_origins;
    return res;
  };
}

// ---------------------------------------------------------------------------
// Rate Limiter (in-process, per-IP sliding window)
//
// Parameters:
//   max_requests  – number of requests allowed in the window
//   window_secs   – rolling window length in seconds
//
// The client IP is read from the X-Forwarded-For header first, then falls
// back to a sentinel string when the header is absent (single-server use).
//
// Returns 429 Too Many Requests when the limit is exceeded.
// ---------------------------------------------------------------------------
inline Middleware rate_limit(int max_requests, int window_secs) {
  struct Bucket {
    std::vector<std::chrono::steady_clock::time_point> timestamps;
  };

  // Shared state wrapped in a shared_ptr so the lambda is copyable.
  auto state = std::make_shared<
      std::pair<std::mutex, std::unordered_map<std::string, Bucket>>>();

  return [=](HttpRequest &req, Next next) -> HttpResponse {
    const auto now = std::chrono::steady_clock::now();
    const auto window = std::chrono::seconds(window_secs);

    // Determine client key.
    std::string client_key = "local";
    auto it = req.headers.find("X-Forwarded-For");
    if (it != req.headers.end() && !it->second.empty()) {
      // Take only the first IP in a possibly comma-separated list.
      client_key = it->second.substr(0, it->second.find(','));
      while (!client_key.empty() && client_key.back() == ' ')
        client_key.pop_back();
    }

    auto &[mtx, buckets] = *state;
    {
      std::lock_guard<std::mutex> lock(mtx);
      Bucket &bucket = buckets[client_key];

      // Evict timestamps outside the current window.
      bucket.timestamps.erase(
          std::remove_if(bucket.timestamps.begin(), bucket.timestamps.end(),
                         [&](const auto &tp) { return now - tp > window; }),
          bucket.timestamps.end());

      if (static_cast<int>(bucket.timestamps.size()) >= max_requests) {
        HttpResponse res;
        res.set_status(429);
        res.status_message = "Too Many Requests";
        res.headers["Retry-After"] = std::to_string(window_secs);
        res.set_body("Rate limit exceeded. Try again later.");
        res.keep_alive = req.wants_keep_alive();
        return res;
      }

      bucket.timestamps.push_back(now);
    }

    return next();
  };
}

// ---------------------------------------------------------------------------
// Content-Type Guard
// Rejects non-preflight requests whose body-carrying methods lack a matching
// Content-Type, returning 415 Unsupported Media Type.
//
// Parameter:
//   required_type  – expected media type prefix (e.g. "application/json")
//                    checked only for POST, PUT, PATCH
// ---------------------------------------------------------------------------
inline Middleware require_content_type(const std::string &required_type) {
  return [=](HttpRequest &req, Next next) -> HttpResponse {
    static const std::vector<std::string> body_methods = {"POST", "PUT",
                                                          "PATCH"};
    bool is_body_method = std::find(body_methods.begin(), body_methods.end(),
                                    req.method) != body_methods.end();

    if (is_body_method) {
      auto it = req.headers.find("Content-Type");
      if (it == req.headers.end() ||
          it->second.find(required_type) == std::string::npos) {
        HttpResponse res;
        res.set_status(415);
        res.status_message = "Unsupported Media Type";
        res.set_body("Expected Content-Type: " + required_type);
        res.keep_alive = req.wants_keep_alive();
        return res;
      }
    }

    return next();
  };
}

} // namespace Middlewares
