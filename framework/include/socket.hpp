#pragma once

#include "http_request.hpp"
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

    if (listen(socket_fd, 1024) < 0) {
      return -1;
    }

    return socket_fd;
  }

  void handle(int client_socket) {
    char buffer[8192] = {0};
    ssize_t bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);

    if (bytes_read <= 0) {
      close(client_socket);
      return;
    }

    HttpRequest request;
    request.parse(std::string(buffer, bytes_read));

    std::string response = setup_router(request);

    size_t total_sent = 0;
    const char *data = response.c_str();
    size_t len = response.length();

    while (total_sent < len) {
      ssize_t sent = send(client_socket, data + total_sent, len - total_sent,
                          MSG_NOSIGNAL);
      if (sent <= 0) {
        break;
      }
      total_sent += sent;
    }

    shutdown(client_socket, SHUT_RDWR);
    close(client_socket);
  }
};
