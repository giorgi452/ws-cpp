#pragma once
#include <netinet/in.h>

class Socket {
public:
  int socket_fd;
  sockaddr_in address;
  int init();
};
