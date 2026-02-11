#include "framework/include/io_uring_server.hpp"

int main() {
  IoUringServer server;

  if (!server.init()) {
    return 1;
  }

  server.run();
  return 0;
}
