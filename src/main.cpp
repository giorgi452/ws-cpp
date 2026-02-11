#include "framework/include/socket.hpp"

int main() {
  Socket server;

  if (!server.init()) {
    return 1;
  }

  server.run();
  return 0;
}
