#include "framework/include/socket.hpp"
#include "framework/include/thread_pool.hpp"
#include <netinet/in.h>
#include <routes.hpp>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char **argv) {

  Socket server;
  int server_fd = server.init();

  ThreadPool pool(std::thread::hardware_concurrency());

  while (true) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd =
        accept(server_fd, (struct sockaddr *)&client_addr, &client_len);

    if (client_fd >= 0) {
      pool.enqueue([client_fd, &server] { server.handle(client_fd); });
    }
  }
  return 0;
}
