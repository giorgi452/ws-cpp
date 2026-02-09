#pragma once

#include "http_request.hpp"
#include "http_response.hpp"
#include "routes.hpp"
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
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

    if (listen(socket_fd, SOMAXCONN) < 0) {
      return -1;
    }

    return socket_fd;
  }

  void handle(int client_socket) {
    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

    setsockopt(client_socket, SOL_SOCKET, SO_SNDTIMEO, &timeout,
               sizeof(timeout));

    setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout,
               sizeof(timeout));

    int keepalive = 1;
    setsockopt(client_socket, SOL_SOCKET, SO_KEEPALIVE, &keepalive,
               sizeof(keepalive));

    int keepidle = 120;
    setsockopt(client_socket, IPPROTO_TCP, TCP_KEEPIDLE, &keepidle,
               sizeof(keepidle));

    static thread_local char buffer[16384];

    int request_count = 0;
    const int max_requests = 1000; // Increased for better connection reuse
    bool keep_alive = true;

    while (keep_alive && request_count < max_requests) {
      ssize_t bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);

      if (bytes_read <= 0) {
        break;
      }

      buffer[bytes_read] = '\0';

      HttpRequest request;
      request.parse(std::string(buffer, bytes_read));

      HttpResponse res = setup_router(request);
      keep_alive = request.wants_keep_alive();

      res.keep_alive = keep_alive;

      std::string response_str = res.to_string();
      size_t total_sent = 0;

      while (total_sent < response_str.length()) {
        ssize_t sent = send(client_socket, response_str.c_str(),
                            response_str.length() - total_sent, MSG_NOSIGNAL);
        if (sent <= 0) {
          if (errno == EAGAIN || errno == EWOULDBLOCK) {
            usleep(100); // Wait 100 microseconds
            continue;
          }
          keep_alive = false; // Connection error
          break;
        }
        total_sent += sent;
      }

      request_count++;
    }

    shutdown(client_socket, SHUT_RDWR);
    close(client_socket);
  }
};
