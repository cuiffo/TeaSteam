#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>

#define CLIENT_NUM 2

#ifndef SPOTS_Y
#define SPOTS_Y 6
#endif
#ifndef SPOTS_X
#define SPOTS_X 7
#endif



/*
 * Function prototypes.
 */
int initConnections(struct sockaddr_in* clients, int* clientfds, int listenfd);
int open_listenfd(int);
int closeConnections(int* clientfds);
void abortGame(int* fds);
void addSd(int sd, fd_set* set);


fd_set readfds;
int nfsd;
int endval = 0;

int main(int argc, char** argv) {

  int listenfd, clientfds[CLIENT_NUM];
  struct sockaddr_in server, clients[CLIENT_NUM];
  socklen_t sock_size = sizeof(server);

  // create a new TCP server on a random port and get that port number
  listenfd = open_listenfd(0);
  if (listenfd < 0) {
    perror(NULL);
    return -1;
  }
  if (getsockname(listenfd, (struct sockaddr*)&server, &sock_size) < 0) {
    perror(NULL);
    close(listenfd);
    return -1;
  }
  unsigned short port = ntohs(server.sin_port);
  fprintf(stderr, "%hu\n", port);
  write(STDOUT_FILENO, &port, 2);

  // Initialize our connections to the clients.
  FD_ZERO(&readfds);
  if (initConnections(clients, clientfds, listenfd) == 0) {
    close(listenfd);
    return -1;
  }

  // The game is running, keep listening for moves by players.
  int positions[2] = {0,0};
  struct timeval times[2];
  while(1) {
    fd_set readfds_c = readfds;
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
  
    // Perform select.
    int retval = select(nfsd, &readfds_c, NULL, NULL, &tv);

    if (retval == 0) {
      abortGame(clientfds);
      closeConnections(clientfds);
      exit(0);
    }

    int i;
    for (i = 0; i < CLIENT_NUM; ++i) {
      if (FD_ISSET(clientfds[i], &readfds_c)) {
        char msg[2];
        if (read(clientfds[i], msg, 2) == 0) {
          abortGame(clientfds);
          closeConnections(clientfds);
          exit(0);
        }
        positions[i] = (int)msg[1];
        if (positions[i] == endval)
          gettimeofday(&(times[i]), NULL);
        int opponent = (i+1)%2;
        write(clientfds[opponent], msg, 2);
      }
    }
    if (positions[0] + positions[1] == endval*2)
      break;
  }

  struct timeval tv;
  time_t curtime;

  int i, winner;
  double diff = times[0].tv_usec - times[1].tv_usec;
  if (diff < 0)
    winner = 0;
  else
    winner = 1;

  for (i = 0; i < CLIENT_NUM; ++i) {
    char results[2];
    results[0] = 2;
    if (i == winner)
      results[1] = 1;
    else
      results[1] = 2;
    write(clientfds[i], results, 2);
  }

  closeConnections(clientfds);

  exit(0);
}


/*
 * Create the connections to the clients.
 */
int initConnections(struct sockaddr_in* clients, int* clientfds, int listenfd) {
  socklen_t sock_size = sizeof(struct sockaddr_in);

  // Accept connections from both clients.
  int i;
  for (i = 0; i < CLIENT_NUM; ++i) {

    // Find each client from the server (from a pipe of stdin).
    char clientData[1000];
    read(STDIN_FILENO, clientData, 1000);

    // Make accept non-blocking so that we can have a timer on it.
    fcntl(listenfd, F_SETFL, O_NDELAY);

    // Create a connection between us and the current client.
    int clientfd;
    time_t start = time(NULL);
    while ((clientfd = accept(listenfd, (struct sockaddr*)&clients[i], &sock_size)) < 0 ) {
      time_t now = time(NULL);
      if (now - start > 5) {
        return 0;
      }
    }
    clientfds[i] = clientfd;

    // Set this back to blocking.
    fcntl(listenfd, F_SETFL, 0);
  }

  // Tell the clients we're ready to begin the game.
  for (i = 0; i < CLIENT_NUM; ++i) {
    char msg[100] = "  What's up man?";

    // OPCODE 1 tells clients it's time.
    msg[0] = 1;
    msg[1] = strlen(msg)-2;

    endval = msg[1];

    // Send the player number with this OPCODE.
    write(clientfds[i], msg, strlen(msg));

    addSd(clientfds[i], &readfds);
  }

  return 1;
}

void addSd(int sd, fd_set* set) {
  FD_SET(sd, set);
  nfsd = (nfsd-1 > sd) ? nfsd : sd+1;
}


/*
 * Sends both players an error.
 */
void abortGame(int* fds) {
  char msg[1];
  msg[0] = 7;
  int i;
  for (i = 0 ; i < CLIENT_NUM; ++i) {
          fprintf(stderr, "Game Broken\n");
    if (write(fds[i], msg, 1) < 0) {
      perror(NULL);
    }
  }
}


/*
 * Closes the connections to the clients.
 */
int closeConnections(int* clientfds) {

  // Close each client separately.
  int i;
  for (i = 0; i < CLIENT_NUM; ++i)
    if (close(clientfds[i]) < 0)
      return 0;

  return 1;
}


/*
 * Opens a new TCP listening socket on an ephemeral port.
 * Returns an integer for the listening file descriptor,
 * or -1 on error.
 */
int open_listenfd(int portnum) {

  int listenfd, optval=1;
  struct sockaddr_in serveraddr;

  if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    return -1;

  if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
        (const void *) &optval, sizeof(int)) < 0)
    return -1;

  bzero((char *)&serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short) portnum);

  if (bind(listenfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0)
    return -1;

  if (listen(listenfd, 1024) < 0)
    return -1;

  return listenfd;
}
