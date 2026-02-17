#pragma once

#include "http_request.hpp"
#include "http_response.hpp"
#include <functional>
#include <vector>

using Next = std::function<HttpResponse()>;

using Middleware = std::function<HttpResponse(HttpRequest &, Next)>;

class MiddlewareChain {
public:
  MiddlewareChain &use(Middleware mw) {
    middlewares_.push_back(std::move(mw));
    return *this;
  }

  HttpResponse
  execute(HttpRequest &req,
          std::function<HttpResponse(HttpRequest &)> handler) const {
    return dispatch(req, 0, handler);
  }

private:
  std::vector<Middleware> middlewares_;

  HttpResponse
  dispatch(HttpRequest &req, size_t index,
           const std::function<HttpResponse(HttpRequest &)> &handler) const {
    if (index >= middlewares_.size()) {
      return handler(req);
    }

    Next next = [this, &req, index, &handler]() {
      return dispatch(req, index + 1, handler);
    };

    return middlewares_[index](req, std::move(next));
  }
};
