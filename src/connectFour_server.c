#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
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
int playMove(int* board, int col, int player);
int checkWin(int* board);
int open_listenfd(int);
int sendSuccessfulMove(int* fds, int col);
int sendFailedMove(int fd);
int sendWinner(int* fds, int col);
int sendTie(int* fds);
int closeConnections(int* clientfds);
void abortGame(int* fds);


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
  write(STDOUT_FILENO, &port, 2);
  fprintf(stderr, "%hu\n", port);

  // Initialize our connections to the clients.
  if (initConnections(clients, clientfds, listenfd) == 0) {
    close(listenfd);
    return -1;
  }

  // The game is running, keep listening for moves by players.
  int board[SPOTS_X*SPOTS_Y] = {0};
  bzero(board, SPOTS_X*SPOTS_Y);
  int playerTurn = 1;
  int turns = 0;
  while(1) {

    char msg[2];
    ssize_t size = read(clientfds[playerTurn-1], msg, 2);
    if (size == 0) {
      perror(NULL);
      abortGame(clientfds);
      break;
    }

    // If this message is OPCODE 2, we care about it.
    if (msg[0] == 2) {
      if (playMove(board, msg[1], playerTurn)) {
        int win = checkWin(board);
        if (win) {
          fprintf(stderr, "Winner\n");
          sendWinner(clientfds, msg[1]);
          break;
        } else {
          turns++;
          if (turns == SPOTS_X*SPOTS_Y) {
            sendTie(clientfds);
            break;
          }
          fprintf(stderr, "Move piece in col %i\n", (int)msg[1]);
          if (sendSuccessfulMove(clientfds, msg[1]) == 0) {
            abortGame(clientfds);
            break;
          }
        }
        playerTurn = (playerTurn == 1) ? 2 : 1;
      } else {
        fprintf(stderr, "Failed Move\n");
        sendFailedMove(clientfds[playerTurn-1]);
      }
    }
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
    char msg[2];

    // OPCODE 1 tells clients it's time.
    msg[0] = 1;

    // Send the player number with this OPCODE.
    msg[1] = i+1;
    write(clientfds[i], msg, 2);
  }

  return 1;
}


/*
 * Checks if this move is playable and if it is, then add the
 * result on the board.
 */
int playMove(int* board, int col, int player) {
  int i;
  for (i = 5; i >= 0; --i) {
    int cur = i*7+col;
    if (board[cur] == 0) {
      board[cur] = player;
      return 1;
    }
  }
  return 0;
}


/*
 * Checks if there is a winner on the board.
 */
int checkWin(int* board) {

  // Check horizontals
  int row;
  for (row = 0; row < 6; ++row) {
    int piece;
    for (piece = 0; piece < 4; ++piece) {
      int piece2;
      for (piece2 = piece+1; piece2 < piece+4; ++piece2)
        if (board[row*7+piece2] != board[row*7+piece] ||
            board[row*7+piece] == 0)
          break;
      if (piece2 == piece+4)
        return board[row*7+piece];
    }
  }


  for (row = 0; row < 3; ++row) {

    // Check verticals.
    int piece;
    for (piece = 0; piece < 7; ++piece) {
      int row2;
      for (row2 = row+1; row2 < row+4; ++row2)
        if (board[row2*7+piece] != board[row*7+piece] ||
            board[row*7+piece] == 0)
          break;
      if (row2 == row+4)
        return board[row*7+piece];
    }

    // Check negative diagonals
    for (piece = 0; piece < 4; ++piece) {
      int offset;
      for (offset = 1; offset < 4; ++offset) {
        if (board[(row+offset)*7+piece+offset] != board[row*7+piece] ||
            board[row*7+piece] == 0)
          break;
      }
      if (offset == 4)
        return board[row*7+piece];
    }

    // Check positive diagonals
    for (piece = 3; piece < 7; ++piece) {
      int offset;
      for (offset = 1; offset < 4; ++offset) {
        if (board[(row+offset)*SPOTS_X+piece-offset] != board[row*SPOTS_X+piece] ||
            board[row*7+piece] == 0)
          break;
      }
      if (offset == 4)
        return board[row*SPOTS_X+piece];
    }
  }

  return 0;
}


/*
 * Sends the successful column entry to both clients so they may
 * update their screens and the opponent may continue.
 */
int sendSuccessfulMove(int* fds, int col) {
  char msg[2];
  msg[0] = 3;
  msg[1] = (char)col;
  int i;
  for (i = 0 ; i < CLIENT_NUM; ++i) {
    if (write(fds[i], msg, 2) < 0) {
      perror(NULL);
      return 0;
    }
  }
  return 1;
}


/*
 * Sends the tie command to both clients as well as the piece
 * played that resulted in this.
 */
int sendTie(int* fds) {
  char msg[1];
  msg[0] = 6;
  int i;
  for (i = 0 ; i < CLIENT_NUM; ++i) {
    if (write(fds[i], msg, 1) < 0) {
      perror(NULL);
      return 0;
    }
  }
  return 1;
}


/*
 * Send a failed move notification to the current player so they 
 * fix the error of their ways and try again.
 */
int sendFailedMove(int fd) {
  char msg[1];
  msg[0] = 4;
  if (write(fd, msg, 1) < 0) {
    perror(NULL);
    return 0;
  }
  return 1;
}


/*
 * Tells both clients that the current player has won the game.
 */
int sendWinner(int* fds, int col) {
  char msg[2];
  msg[0] = 5;
  msg[1] = (char)col;
  int i;
  for (i = 0 ; i < CLIENT_NUM; ++i) {
    if (write(fds[i], msg, 2) < 0) {
      perror(NULL);
      return 0;
    }
  }
  return 1;
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
