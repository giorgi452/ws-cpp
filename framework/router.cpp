#include "include/router.hpp"
#include <string>

void Router::get(std::string path, RouteMap *routes, Handler handler) {
  (*routes)[{"GET", path}] = handler;
};

void Router::post(std::string path, RouteMap *routes, Handler handler) {
  (*routes)[{"POST", path}] = handler;
};
