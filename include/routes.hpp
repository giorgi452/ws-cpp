#pragma once

#include "http_request.hpp"
#include <functional>
#include <map>
#include <string>
#include <utility>

using Handler = std::function<std::string(const HttpRequest &)>;

using RouteKey = std::pair<std::string, std::string>;

using RouteMap = std::map<RouteKey, Handler>;

void setup_routes(RouteMap &routes);
