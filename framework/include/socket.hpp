#pragma once

#include "http_request.hpp"
#include "http_response.hpp"
#include "routes.hpp"
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

class Socket {
public:
  int socket_fd;
  sockaddr_in address;

  int init() {
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1)
      return -1;

    int opt = 1;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
      return -1;
    if (setsockopt(socket_fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) < 0)
      return -1;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(DEFAULT_PORT);

    if (bind(socket_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
      return -1;
    if (listen(socket_fd, SOMAXCONN) < 0)
      return -1;

    return socket_fd;
  }

private:
  static constexpr int DEFAULT_PORT = 8080;
  static constexpr int SOCKET_TIMEOUT_SEC = 60;
  static constexpr int TCP_KEEPALIVE_IDLE = 75;
  static constexpr int BUFFER_SIZE = 256 * 1024;
  static constexpr int MAX_REQUESTS_PER_CONNECTION = 5000;
  static constexpr int REQUEST_PROCESSING_DELAY_US = 500;

public:
  void handle(int client_fd) {
    struct timeval tv{};
    tv.tv_sec = SOCKET_TIMEOUT_SEC;
    tv.tv_usec = 0;
    if (setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
      close(client_fd);
      return;
    }
    if (setsockopt(client_fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0) {
      close(client_fd);
      return;
    }

    int ka = 1;
    if (setsockopt(client_fd, SOL_SOCKET, SO_KEEPALIVE, &ka, sizeof(ka)) < 0) {
      close(client_fd);
      return;
    }
    int idle = TCP_KEEPALIVE_IDLE;
    if (setsockopt(client_fd, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(idle)) < 0) {
      close(client_fd);
      return;
    }

    static thread_local std::vector<char> buf(BUFFER_SIZE);
    size_t buf_len = 0;
    int req_count = 0;
    constexpr int MAX_REQS = MAX_REQUESTS_PER_CONNECTION;

    bool alive = true;

    while (alive && req_count < MAX_REQS) {
      if (buf_len >= buf.size() - 8192) {
        buf.resize(buf.size() * 2);
      }

      ssize_t n =
          recv(client_fd, buf.data() + buf_len, buf.size() - buf_len - 1, 0);

      if (n == 0)
        break;
      if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
          continue;
        break;
      }

      buf_len += static_cast<size_t>(n);

      size_t offset = 0;

      while (offset < buf_len) {
        HttpRequest req;
        std::string_view data(buf.data() + offset, buf_len - offset);

        auto [ok, consumed] = req.parse(data);
        if (!ok || consumed == 0)
          break;

        HttpResponse res = setup_router(req);
        alive = req.wants_keep_alive() && res.keep_alive;
        res.keep_alive = alive;

        std::string reply = res.to_string();

        size_t sent_total = 0;
        while (sent_total < reply.size()) {
          ssize_t s = send(client_fd, reply.data() + sent_total,
                           reply.size() - sent_total, MSG_NOSIGNAL);
          if (s <= 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
              usleep(REQUEST_PROCESSING_DELAY_US);
              continue;
            }
            alive = false;
            goto finish;
          }
          sent_total += static_cast<size_t>(s);
        }

        req_count++;
        offset += consumed;
      }

      // Compact remaining data
      if (offset < buf_len) {
        std::memmove(buf.data(), buf.data() + offset, buf_len - offset);
        buf_len -= offset;
      } else {
        buf_len = 0;
      }
    }

  finish:
    shutdown(client_fd, SHUT_RDWR);
    close(client_fd);
  }
};
