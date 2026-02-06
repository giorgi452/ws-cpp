#include "socket.hpp"

int Socket::init() {
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
