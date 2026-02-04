#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char **argv) {
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);

  sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(8080);

  int _ = bind(server_fd, (struct sockaddr *)&address, sizeof(address));
  listen(server_fd, 10);

  std::cout << "Server listening on port 8080..." << std::endl;

  while (true) {
    int new_socket = accept(server_fd, nullptr, nullptr);

    char buffer[1024] = {0};
    read(new_socket, buffer, 1024);
    std::cout << "Request received:\n" << buffer << std::endl;

    std::string response = "HTTP/1.1 200 OK\n"
                           "Content-Type: text/plain\n"
                           "Content-Length: 12\n\n"
                           "Hello World!";
    send(new_socket, response.c_str(), response.length(), 0);

    close(new_socket);
  }
  return 0;
}
