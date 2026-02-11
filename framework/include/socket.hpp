#pragma once

#include <liburing.h>
#include <memory>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string_view>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

#include "http_request.hpp"
#include "http_response.hpp"
#include "routes.hpp"

enum class EventType { ACCEPT, READ, WRITE };

struct ConnectionContext {
  int fd;
  EventType event_type;
  std::vector<char> read_buffer;
  std::string write_data;

  static constexpr size_t DEFAULT_BUFFER_SIZE = 4096;
  
  ConnectionContext()
      : fd(-1), event_type(EventType::READ), read_buffer(DEFAULT_BUFFER_SIZE) {}
};

class Socket {
public:
  static constexpr int DEFAULT_PORT = 8080;
  static constexpr int QUEUE_DEPTH = 4096;
  static constexpr int MAX_FDS = 32768;

  Socket() : server_fd(-1) { fd_table.resize(MAX_FDS); }
  
  ~Socket() {
    cleanup();
  }

  bool init() {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
      return false;
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
      close(server_fd);
      server_fd = -1;
      return false;
    }
    
    if (setsockopt(server_fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) < 0) {
      close(server_fd);
      server_fd = -1;
      return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(DEFAULT_PORT);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
      close(server_fd);
      server_fd = -1;
      return false;
    }
    
    if (listen(server_fd, SOMAXCONN) < 0) {
      close(server_fd);
      server_fd = -1;
      return false;
    }

    // Initialize ring
    int ret = io_uring_queue_init(QUEUE_DEPTH, &ring, 0);
    if (ret != 0) {
      close(server_fd);
      server_fd = -1;
      return false;
    }
    
    return true;
  }

  void run() {
    submit_accept();
    io_uring_submit(&ring);

    struct io_uring_cqe *cqe;
    while (true) {
      int ret = io_uring_wait_cqe(&ring, &cqe);
      if (ret < 0)
        continue;

      unsigned head;
      int count = 0;

      io_uring_for_each_cqe(&ring, head, cqe) {
        handle_completion(cqe);
        count++;
      }

      if (count > 0) {
        io_uring_cq_advance(&ring, count);
        io_uring_submit(&ring);
      }
    }
  }

private:
  struct io_uring ring;
  int server_fd;
  std::vector<std::unique_ptr<ConnectionContext>> fd_table;

  void submit_accept() {
    struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
    if (!sqe)
      return;

    ConnectionContext *ctx = new ConnectionContext();
    ctx->event_type = EventType::ACCEPT;

    io_uring_prep_accept(sqe, server_fd, nullptr, nullptr, 0);
    io_uring_sqe_set_data(sqe, ctx);
  }

  void submit_read(ConnectionContext *ctx) {
    struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
    if (!sqe)
      return;
    ctx->event_type = EventType::READ;
    io_uring_prep_recv(sqe, ctx->fd, ctx->read_buffer.data(),
                       ctx->read_buffer.size(), 0);
    io_uring_sqe_set_data(sqe, ctx);
  }

  void submit_write(ConnectionContext *ctx) {
    struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
    if (!sqe)
      return;
    ctx->event_type = EventType::WRITE;
    io_uring_prep_send(sqe, ctx->fd, ctx->write_data.data(),
                       ctx->write_data.size(), MSG_NOSIGNAL);
    io_uring_sqe_set_data(sqe, ctx);
  }

  void handle_completion(struct io_uring_cqe *cqe) {
    auto *ctx = static_cast<ConnectionContext *>(io_uring_cqe_get_data(cqe));
    int res = cqe->res;

    if (!ctx)
      return;

    if (ctx->event_type == EventType::ACCEPT) {
      if (res >= 0) {
        int client_fd = res;

        int opt = 1;
        setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));

        if (client_fd < MAX_FDS) {
          auto conn = std::make_unique<ConnectionContext>();
          conn->fd = client_fd;
          ConnectionContext *ptr = conn.get();
          fd_table[client_fd] = std::move(conn);
          submit_read(ptr);
        } else {
          close(client_fd);
        }
      }
      delete ctx;      // Important: Clean up the temporary accept object
      submit_accept(); // Re-arm the listener
      return;
    }

    if (res <= 0) {
      clean_conn(ctx);
      return;
    }

    if (ctx->event_type == EventType::READ) {
      HttpRequest req;
      std::string_view data(ctx->read_buffer.data(), res);

      auto [ok, consumed] = req.parse(data);

      if (ok) {
        HttpResponse resp = setup_router(req);
        ctx->write_data = resp.to_string();
        submit_write(ctx);
      } else {
        submit_read(ctx);
      }
    } else if (ctx->event_type == EventType::WRITE) {
      submit_read(ctx);
    }
  }

  void clean_conn(ConnectionContext *ctx) {
    if (ctx && ctx->fd >= 0) {
      int fd = ctx->fd;
      close(fd);
      if (fd < (int)fd_table.size()) {
        fd_table[fd].reset();
      }
    }
  }

  void cleanup() {
    if (server_fd >= 0) {
      close(server_fd);
      server_fd = -1;
    }
    
    for (auto& conn : fd_table) {
      if (conn && conn->fd >= 0) {
        close(conn->fd);
        conn->fd = -1;
      }
    }
    fd_table.clear();
    
    io_uring_queue_exit(&ring);
  }
};
