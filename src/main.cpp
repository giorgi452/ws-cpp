#include "framework/include/socket.hpp"
#include "framework/include/thread_pool.hpp"
#include <netinet/in.h>
#include <routes.hpp>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char **argv) {

  Socket server;
  int server_fd = server.init();

  ThreadPool pool(10);

  while (true) {
    int new_socket = accept(server_fd, nullptr, nullptr);

    if (new_socket < 0) {
      continue;
    }

    pool.enqueue([new_socket, &server]() { server.handle(new_socket); });
  }
  return 0;
}
