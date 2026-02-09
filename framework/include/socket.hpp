#pragma once

#include "http_request.hpp"
#include "http_response.hpp"
#include "routes.hpp"
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <unistd.h>

class Socket {
public:
  int socket_fd;
  sockaddr_in address;

  int init() {
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (socket_fd == -1) {
      return -1;
    }

    int opt = 1;
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    setsockopt(socket_fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(socket_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
      return -1;
    }

    if (listen(socket_fd, 4096) < 0) {
      return -1;
    }

    return socket_fd;
  }

  void handle(int client_socket) {
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout,
               sizeof(timeout));

    int opt = 1;
    setsockopt(client_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    while (true) {
      char buffer[8192];
      ssize_t bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);

      if (bytes_read <= 0) {
        break;
      }

      HttpRequest request;
      request.parse(std::string(buffer, bytes_read));

      HttpResponse res = setup_router(request);
      bool client_wants_alive = request.wants_keep_alive();

      res.keep_alive = client_wants_alive;
      std::string response_str = res.to_string();

      if (send(client_socket, response_str.c_str(), response_str.length(),
               MSG_NOSIGNAL) <= 0) {
        break;
      }

      if (!client_wants_alive)
        break;
    }

    close(client_socket);
  }
};
