#include "include/not_found.hpp"
#include <string>

std::string not_found() {
  return "HTTP/1.1 404 Not Found\r\nContent-Length: 9\r\n\r\nNot Found";
};
