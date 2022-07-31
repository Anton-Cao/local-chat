/*
 * Listen on a port.
 * Poll for messages that come in.
 * Print messages to stdout.
 */

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
  remove(socket_file);

  int sock = socket(AF_LOCAL, SOCK_STREAM, 0);
  if (sock == 0) {
    perror("failed to create socket");
    exit(1);
  }

  int opt = 1;
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
    perror("failed to setsockopt");
    exit(1);
  }

  struct sockaddr_un address;
  address.sun_family = AF_LOCAL;
  strncpy(address.sun_path, socket_file, sizeof(address.sun_path));
  address.sun_path[sizeof(address.sun_path) - 1] = '\0';
  int addrlen = sizeof(address);

  if (bind(sock, (struct sockaddr*) &address, sizeof(address)) < 0) {
    perror("failed to bind");
    exit(1);
  }
  if (listen(sock, 3) < 0) {
    perror("failed to listen");
    exit(1);
  }

  printf("listening...\n");

  const int MAX_NFDS = 16;
  struct pollfd pfds[MAX_NFDS];
  int nfds = 1;
  pfds[0].fd = sock;
  pfds[0].events = POLLIN;

  while (true) {
    printf("polling\n");

    int ready = poll(pfds, nfds, -1);
    if (ready == -1) {
      perror("failed to poll");
    }

    for (int i = 0; i < nfds; i++) {
      if (pfds[i].revents & POLLHUP) {
        // connection closed
        close(pfds[i].fd);
        pfds[i].fd = -pfds[i].fd;
      } else if (pfds[i].revents & POLLIN) {
        if (i == 0) { // this means there's a new connection
          printf("accepting\n");
          int new_socket = accept(sock, (struct sockaddr*) &address, (socklen_t*) &addrlen);
          if (new_socket < 0) {
            perror("failed to accept");
            exit(1);
          }
          if (nfds == MAX_NFDS) {
            printf("maximum file descriptor limit reached, exiting\n");
            break;
          }
          pfds[nfds].fd = new_socket;
          pfds[nfds].events = POLLIN;
          nfds++;
        } else { // this means there's a new message
          printf("receiving\n");
          char buffer[1024];
          int valread = read(pfds[i].fd, buffer, sizeof(buffer));
          buffer[valread] = '\0';
          printf("%s\n", buffer);

          // send message to all other clients
          for (int j = 1; j < nfds; j++) {
            if (j != i) {
              send(pfds[j].fd, buffer, valread, 0);
            }
          }
        }
        // only process one at a time
        goto end;
      }
    }
end: {}
  }

  remove(socket_file);

  return 0;
}
