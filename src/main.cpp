#include <http_request.hpp>
#include <netinet/in.h>
#include <routes.hpp>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char **argv) {
  RouteMap routes;
  setup_routes(routes);

  int server_fd = socket(AF_INET, SOCK_STREAM, 0);

  sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(8080);

  int _ = bind(server_fd, (struct sockaddr *)&address, sizeof(address));
  listen(server_fd, 10);

  while (true) {
    int new_socket = accept(server_fd, nullptr, nullptr);

    char buffer[4096] = {0};
    int bytes_read = read(new_socket, buffer, 4096);

    if (bytes_read > 0) {
      HttpRequest request;
      request.parse(std::string(buffer));

      auto it = routes.find({request.method, request.path});

      std::string response;
      if (it != routes.end()) {
        response = it->second(request);
      } else {
        response =
            "HTTP/1.1 404 Not Found\r\nContent-Length: 9\r\n\r\nNot Found";
      }
      send(new_socket, response.c_str(), response.length(), 0);
    }

    close(new_socket);
  }
  return 0;
}
