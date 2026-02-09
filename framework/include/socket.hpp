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
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(socket_fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(8080);

    if (bind(socket_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
      return -1;
    if (listen(socket_fd, SOMAXCONN) < 0)
      return -1;

    return socket_fd;
  }

  void handle(int client_fd) {
    struct timeval tv{};
    tv.tv_sec = 60;
    tv.tv_usec = 0;
    setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(client_fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    int ka = 1;
    setsockopt(client_fd, SOL_SOCKET, SO_KEEPALIVE, &ka, sizeof(ka));
    int idle = 75;
    setsockopt(client_fd, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(idle));

    static thread_local std::vector<char> buf(256 * 1024);
    size_t buf_len = 0;
    int req_count = 0;
    constexpr int MAX_REQS = 5000;

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
              usleep(500);
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
