#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

int main() {
  const char* socket_file = "/tmp/chat.sock";

  int sock = socket(AF_LOCAL, SOCK_STREAM, 0);
  if (sock == 0) {
    perror("failed to create socket");
    exit(1);
  }

  struct sockaddr_un address;
  address.sun_family = AF_LOCAL;
  strncpy(address.sun_path, socket_file, sizeof(address.sun_path));
  address.sun_path[sizeof(address.sun_path) - 1] = '\0';

  int client = connect(sock, (struct sockaddr*) &address, sizeof(address));
  if (client < 0) {
    perror("failed to connect");
    exit(1);
  }

  const int nfds = 2;
  struct pollfd pfds[nfds];
  pfds[0].fd = fileno(stdin);
  pfds[0].events = POLLIN;
  pfds[1].fd = sock;
  pfds[1].events = POLLIN;

  printf("stdin %d\n", pfds[0].fd);
  printf("socket %d\n", pfds[1].fd);

  while (true) {
    int ready = poll(pfds, nfds, -1);
    if (ready == -1) {
      perror("failed to poll");
    }

    if (pfds[0].revents & POLLIN) { // stdin
      printf("reading from stdin\n");
      char *line = NULL;
      size_t len;
      if (getline(&line, &len, stdin) == -1) {
        printf("no line\n");
        break;
      } else {
        send(sock, line, len, 0);
        printf("Message sent\n");
      }
    }

    if (pfds[1].revents & POLLHUP) { // remote server closed
      printf("server closed\n");
      break;
    }

    if (pfds[1].revents & POLLIN) { // socket
      printf("socket\n");
      char buffer[1024];
      int valread = read(pfds[1].fd, buffer, sizeof(buffer));
      buffer[valread] = '\0';
      printf("other: %s\n", buffer);
    }
  }

  close(sock);

  return 0;
}
