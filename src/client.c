#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

int main() {
  printf("Please enter your name:\n");
  char* name = NULL;
  size_t namelen;
  if (getline(&name, &namelen, stdin) == -1) {
    perror("failed to read name");
    exit(1);
  }
  namelen = 0;
  while (name[namelen] != '\n') namelen++;
  name[namelen] = '\0'; // remove newline
  printf("Hello %s\n", name);

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

  printf("Connected to chat server\n");

  const int nfds = 2;
  struct pollfd pfds[nfds];
  pfds[0].fd = fileno(stdin);
  pfds[0].events = POLLIN;
  pfds[1].fd = sock;
  pfds[1].events = POLLIN;

  while (true) {
    int ready = poll(pfds, nfds, -1);
    if (ready == -1) {
      perror("failed to poll");
    }

    if (pfds[0].revents & POLLIN) { // stdin
      char* line = NULL;
      size_t len;
      if (getline(&line, &len, stdin) == -1) {
        printf("no line\n");
        break;
      } else {
        len = 0;
        while (line[len] != '\n') len++;
        line[len] = '\0'; // remove newline
        char message[1024];
        memset(message, 0, 1024);
        strncpy(message, name, namelen);
        strcat(message, ": ");
        strcat(message, line);
        message[namelen + len + 2] = '\0';
        send(sock, message, strlen(message), 0);
      }
    }

    if (pfds[1].revents & POLLHUP) { // remote server closed
      printf("server closed\n");
      break;
    }

    if (pfds[1].revents & POLLIN) { // socket
      char buffer[1024];
      int valread = read(pfds[1].fd, buffer, sizeof(buffer));
      buffer[valread] = '\0';
      printf("%s\n", buffer);
    }
  }

  close(sock);

  return 0;
}
