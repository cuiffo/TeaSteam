#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <math.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>


#define CLIENT_NUM 2


/*
 * Function prototypes.
 */
int initConnections(struct sockaddr_in* clients, int* clientfds, int listenfd);
int open_listenfd(int);
int closeConnections(int* clientfds);
void abortGame(int* fds);
void addSd(int sd, fd_set* set);
void randMsg(char* buf);
int getClients(int clientfds[CLIENT_NUM]);
int progressGame(int clientfds[CLIENT_NUM]);


/*
 * Global variables
 */
fd_set readfds;
int nfsd;
int endval = 0;
int positions[2] = {0,0};
struct timeval times[2];

/*
 * Main function
 */
int main(int argc, char** argv) {
  
  // Seed the randomizer.
  srand(time(NULL));

  // Get our client connections.
  int clientfds[CLIENT_NUM];
  if (getClients(clientfds) == 0)
    return -1;

  // The game is running, keep listening for moves by players.
  while(1)
    if (progressGame(clientfds))
      break;

  // Check to see who the winner is
  int i, winner;
  float diff = (times[0].tv_sec + ((float)times[0].tv_usec)/1000000.0) -
    (times[1].tv_sec + ((float)times[1].tv_usec)/1000000.0);
  if (diff < 0)
    winner = 0;
  else
    winner = 1;

  // Send it to both players.
  diff = fabs(diff);
  char str[6];
  snprintf(str, 6, "%.3f", diff);
  for (i = 0; i < CLIENT_NUM; ++i) {
    char results[8];
    results[0] = 3;
    if (i == winner)
      results[1] = 1;
    else
      results[1] = 2;
    memcpy(results+2, str, 6);
    if (write(clientfds[i], results, 8) == 0) {
      perror(NULL);
    } 
  }

  closeConnections(clientfds);

  exit(0);
}


/*
 * Creates a socket and connects to the clients.
 */
int getClients(int clientfds[CLIENT_NUM]) {

  // Setup some variables.
  int listenfd;
  struct sockaddr_in server, clients[CLIENT_NUM];
  socklen_t sock_size = sizeof(server);

  // Create a new TCP server on a random port and get that port number
  listenfd = open_listenfd(0);
  if (listenfd < 0) {
    perror(NULL);
    return 0;
  }
  if (getsockname(listenfd, (struct sockaddr*)&server, &sock_size) < 0) {
    perror(NULL);
    close(listenfd);
    return 0;
  }
  unsigned short port = ntohs(server.sin_port);
  fprintf(stderr, "%hu\n", port);
  if (write(STDOUT_FILENO, &port, 2) == 0) {
    perror(NULL);
    return 0;
  }

  // Initialize our connections to the clients.
  FD_ZERO(&readfds);
  if (initConnections(clients, clientfds, listenfd) == 0) {
    close(listenfd);
    return 0;
  }

  return 1;
}


/*
 * A single iteration in the main function loop -- a single select call
 * along with the logic to perform whatever is necessary with that 
 * select.
 */
int progressGame(int clientfds[CLIENT_NUM]) {

  // Setup variables for select.
  fd_set readfds_c = readfds;
  struct timeval tv;
  tv.tv_sec = 5;
  tv.tv_usec = 0;

  // Perform select.
  select(nfsd, &readfds_c, NULL, NULL, &tv);

  // Handler -- check if either client has a read ready.
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

  // If both players have reached the end, we're done.
  if (positions[0] + positions[1] == endval*2)
    return 0;
  return 1;
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
  char msg[100];
  bzero(msg, 100);

  // Create a random message for the players to type.
  randMsg(msg+2);
  msg[0] = 1;
  msg[1] = 1;
  msg[1] = strlen(msg)-2;

  // Save this global variable for use later.
  endval = msg[1];
  for (i = 0; i < CLIENT_NUM; ++i) {

    // Send the player number with this OPCODE.
    write(clientfds[i], msg, strlen(msg));

    addSd(clientfds[i], &readfds);
  }

  return 1;
}


/*
 * Adds a socket descriptor to the list that we will use
 * select on.
 */
void addSd(int sd, fd_set* set) {
  FD_SET(sd, set);
  nfsd = (nfsd-1 > sd) ? nfsd : sd+1;
}


/*
 * Creates a random message for the players to type in.
 */
void randMsg(char* buf) {
  int num = rand()%10;
  switch (num) {
    case 0:
      strncpy(buf, "Hey there, what's up?", 78);
      break;
    case 1:
      strncpy(buf, "Programming is fun!", 78);
      break;
    case 2:
      strncpy(buf, "Aarg, I forgot a semicolon again.", 78);
      break;
    case 3:
      strncpy(buf, "foo bar baz and all that stuff", 78);
      break;
    case 4:
      strncpy(buf, "for (int i = 0; i < size; ++i) {", 78);
      break;
    case 5:
      strncpy(buf, "I'm not really that good at making up fake sentences.", 78);
      break;
    case 6:
      strncpy(buf, "Where the A's at?", 78);
      break;
    case 7:
      strncpy(buf, "Can you beat your opponent?", 78);
      break;
    case 8:
      strncpy(buf, "Is that the fastest you can type?", 78);
      break;
    case 9:
      strncpy(buf, "I can type this in a millisecond, how about you?", 78);
      break;
  }


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
