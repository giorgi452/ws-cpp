#include "framework/include/http_request.hpp"
#include "framework/include/socket.hpp"
#include <iostream>
#include <netinet/in.h>
#include <routes.hpp>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char **argv) {

  Socket server;
  int server_fd = server.init();

  std::cout << "Welcome To Web Server++" << std::endl;
  std::cout << "-----------------------" << std::endl;
  std::cout << "Server URL: http://localhost:8080" << std::endl;

  while (true) {
    int new_socket = accept(server_fd, nullptr, nullptr);

    char buffer[4096] = {0};
    int bytes_read = read(new_socket, buffer, 4096);

    if (bytes_read > 0) {
      HttpRequest request;
      request.parse(std::string(buffer));

      std::string response = setup_router(request);
      send(new_socket, response.c_str(), response.length(), 0);
    }

    close(new_socket);
  }
  return 0;
}
