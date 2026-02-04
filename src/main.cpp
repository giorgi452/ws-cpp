#include <http_request.hpp>
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

    char buffer[4096] = {0};
    int bytes_read = read(new_socket, buffer, 4096);

    if (bytes_read > 0) {
      HttpRequest request;
      request.parse(std::string(buffer));

      std::cout << "User requested: " << request.path << " using "
                << request.method << std::endl;
      std::cout << "User-Agent: " << request.headers["User-Agent"] << std::endl;

      std::string response;
      if (request.method == "GET" && request.path == "/") {
        response = "HTTP/1.1 200 OK\nContent-Length: 13\n\nWelcome Home!";
      } else if (request.method == "POST" && request.path == "/") {
        std::cout << "Received POST data: " << request.body << std::endl;

        response = "HTTP/1.1 201 Created\r\n"
                   "Content-Type: text/plain\r\n"
                   "Content-Length: " +
                   std::to_string(request.body.length()) +
                   "\r\n"
                   "\r\n" +
                   request.body;
      } else if (request.path == "/about") {
        response = "HTTP/1.1 200 OK\nContent-Length: 17\n\nAbout this server";
      } else {
        response = "HTTP/1.1 404 Not Found\nContent-Length: 9\n\nNot Found";
      }

      send(new_socket, response.c_str(), response.length(), 0);
    }

    close(new_socket);
  }
  return 0;
}
