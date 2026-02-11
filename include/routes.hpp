#pragma once

#include "http_request.hpp"
#include "http_response.hpp"

HttpResponse setup_router(const HttpRequest& request);
