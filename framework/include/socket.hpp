#pragma once

#include "http_request.hpp"
#include "routes.hpp"
#include <iostream>
#include <netinet/in.h>
#include <thread>
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

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(socket_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
      return -1;
    }

    if (listen(socket_fd, 10) < 0) {
      return -1;
    }

    return socket_fd;
  }

  void handle(int client_socket) {
    char buffer[4096] = {0};
    int bytes_read = read(client_socket, buffer, 4096);

    if (bytes_read > 0) {
      HttpRequest request;
      request.parse(std::string(buffer));

      std::string response = setup_router(request);

      size_t total_sent = 0;
      while (total_sent < response.length()) {
        ssize_t sent = send(client_socket, response.c_str() + total_sent,
                            response.length() - total_sent, 0);
        if (sent < 0)
          break;
        total_sent += sent;
      }

      shutdown(client_socket, SHUT_WR);

      usleep(1000); // 1ms
    }

    close(client_socket);
  }
};
