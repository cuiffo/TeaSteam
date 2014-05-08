#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <inttypes.h>
#include <unistd.h>

typedef struct sockaddr SA;

#ifndef BOARD_Y
#define BOARD_Y 6
#endif
#ifndef BOARD_X
#define BOARD_X 1
#endif
#ifndef SPOTS_Y
#define SPOTS_Y 6
#endif
#ifndef SPOTS_X
#define SPOTS_X 7
#endif
#ifndef SPOT_WIDTH
#define SPOT_WIDTH 2
#endif
#ifndef SPOT_HEIGHT
#define SPOT_HEIGHT 1
#endif
#ifndef INNER_SPACING_X
#define INNER_SPACING_X 2
#endif
#ifndef INNER_SPACING_Y
#define INNER_SPACING_Y 1
#endif
#ifndef OUTER_SPACING_X
#define OUTER_SPACING_X 2
#endif
#ifndef OUTER_SPACING_Y
#define OUTER_SPACING_Y 1
#endif


/*
 * Prototype functions
 */
void init();
void initColors();
void fillBoard(WINDOW* window, int width);
void fillHoles(WINDOW* window);
void fillPlayer(WINDOW* window, int player);
void fillCur(WINDOW* window);
void draw(WINDOW* window);
int playMove(int col, int player);
int open_clientfd(char* host, int port);
int waitForReady();
int submitMove(int fd, int col);
int waitForOpponent(int fd, int *col);
void dropDisk();
int opponentTurn(int fd, WINDOW* window);


/*
 * Global variables
 */
int board[SPOTS_X*SPOTS_Y] = {0};
int boardWidth = 32;
int boardHeight = 20;
int curLocation = 3;
int curPlayer = 1;
int playerNum = 0;
int turns = 0;
int win_status = 0;

int main() {

  // Wait until server connection is established and opponent
  // is confirmed.
  int fd = waitForReady();

  // Initialize ncurses and some options.
  init();

  // Initialize some custom colors.
  initColors();

  // Create a window (basically the border).
  boardHeight = SPOT_HEIGHT*SPOTS_Y + INNER_SPACING_Y*(SPOTS_Y - 1) + 2*OUTER_SPACING_Y;
  int height = BOARD_Y + boardHeight + 1 ;
  boardWidth = SPOT_WIDTH*SPOTS_X + INNER_SPACING_X*(SPOTS_X - 1) + 2*OUTER_SPACING_X;
  int width = BOARD_X + boardWidth + 1;
  int x = 0;
  int y = 0;
  WINDOW* window = newwin(height, width, y, x);

  // Draw window with initial settings.
  draw(window);

  int gameInProgress = 1;
  if (curPlayer != playerNum) {
    int gameover = opponentTurn(fd, window);
    if (gameover)
      gameInProgress = 0;
  }

  // Flush the input buffer
  nodelay(window, true);
  while (wgetch(window) != ERR);
  nodelay(window, false);

  // Wait for a character input
  while(gameInProgress) {

    // Get the next key pressed.
    char c = wgetch(window);

    // We only care about left, right and enter.
    if (c == 67) {

      // If we pressed the right key, then move the current piece right
      // unless it can't move anymore.
      curLocation++;
      curLocation = (curLocation > SPOTS_X-1) ? SPOTS_X-1 : curLocation;
      wclear(window);
      draw(window);
    } else if (c == 68) {

      // If we pressed left, move left unless we can't go anymore.
      curLocation--;
      curLocation = (curLocation < 0) ? 0 : curLocation;
      wclear(window);
      draw(window);
    } else if (c == 10) {

      // If we pressed enter, submit the move and see if it is successful.
      int result = submitMove(fd, curLocation);

      // Maybe we failed to hit a valid spot.
      if (result == 4)
        continue;

      // But if we won with it, awesome.
      else if (result == 5)
        win_status = playerNum;

      // Or maybe it ended up with a tie.
      else if (result == 6)
        win_status = 3;

      else if (result == 7)
        win_status = 4;

      // If we're here then our result was 3, meaning a successful move.
      dropDisk(curLocation, curPlayer, window);

      // If the win_status has changed, we're done.
      if (win_status != 0)
        break;

      // Now wait for the other user to go.
      int gameover = opponentTurn(fd, window);
      if (gameover)
        break;

      curLocation = 3;


      // Flush the character buffer.
      nodelay(window, true);
      while (wgetch(window) != ERR);
      nodelay(window, false);
    }
  }

  // Flush the character buffer.
  nodelay(window, true);
  while (wgetch(window) != ERR);
  nodelay(window, false);

  // Wait until user presses a button.
  wgetch(window);

  // Close the connection
  close(fd);

  // Close the screen.
  endwin();
  return 0;
}

void draw(WINDOW* window) {

  // Create a border 
  wattrset(window, COLOR_PAIR(1));
  box(window, 0, 0);
  touchwin(window);

  // Text on top
  int opponent = (playerNum == 1) ? 2 : 1;
  if (win_status == 0) {
    if (curPlayer == playerNum)
      mvwprintw(window, 1, 10, "Your turn!");
    else 
      mvwprintw(window, 1, 6, "Waiting for opponent");
  } else if (win_status == playerNum) {
    wattrset(window, COLOR_PAIR(playerNum+2));
    mvwprintw(window, 1, 10, "YOU WIN!");
    wattrset(window, COLOR_PAIR(1));
    mvwprintw(window, 2, 3, "Press any button to exit.");
  } else if (win_status == opponent) {
    wattrset(window, COLOR_PAIR(opponent+2));
    mvwprintw(window, 1, 9, "You lose :(");
    wattrset(window, COLOR_PAIR(1));
    mvwprintw(window, 2, 3, "Press any button to exit.");
  } else if (win_status == 3) {
    mvwprintw(window, 1, 9, "Yay, a tie!");
    mvwprintw(window, 2, 3, "Press any button to exit.");
  } else if (win_status == 4) {
    mvwprintw(window, 1, 7, "Connection lost...");
    mvwprintw(window, 2, 3, "Press any button to exit.");
  }

  // Draw the board
  wattrset(window, COLOR_PAIR(2));
  fillBoard(window, boardWidth);
  wattrset(window, COLOR_PAIR(1));
  fillHoles(window);
  wattrset(window, COLOR_PAIR(3));
  fillPlayer(window, 1);
  wattrset(window, COLOR_PAIR(4));
  fillPlayer(window, 2);

  if (win_status == 0 && curPlayer == playerNum) {
    wattrset(window, COLOR_PAIR(curPlayer+2));
    fillCur(window);
  }

  // Refresh -- actually draw everything.
  wrefresh(window);

  // Get rid of the cursor.
  curs_set(0);
}


/*
 * Initialize the graphics.
 */
void init() {

  // Initialize our ncurses screen.
  initscr();
  start_color();

  // Set the options.
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
}


/*
 * Initialize a few colors that will help make our screen look
 * nice and purty.
 */
void initColors() {
  short FG = COLOR_WHITE;
  short BG = COLOR_BLACK;
  short colorNum = 1;
  init_pair(colorNum, FG, BG);

  short boardFG = COLOR_BLACK;
  short boardBG = COLOR_YELLOW;
  short boardNum = 2;
  init_pair(boardNum, boardFG, boardBG);

  short p1FG = COLOR_WHITE;
  short p1BG = COLOR_BLUE;
  short p1Num = 3;
  init_pair(p1Num, p1FG, p1BG);

  short p2FG = COLOR_WHITE;
  short p2BG = COLOR_RED;
  short p2Num = 4;
  init_pair(p2Num, p2FG, p2BG);
}


/*
 * Fill in the yellow background for the board.
 */
void fillBoard(WINDOW* window, int width) {
  char *blank = malloc(width+1);
  memset(blank, ' ', width);
  blank[width] = '\0';
  int i;
  for (i = BOARD_Y; i < BOARD_Y+boardHeight; ++i) {
    mvwprintw(window, i, BOARD_X, "%s", blank);
  }
  free(blank);
}


/*
 * Fill in the holes of the board with the color of the
 * background (gray).
 */
void fillHoles(WINDOW* window) {
  char *blank = malloc(SPOT_WIDTH+1);
  memset(blank, ' ', SPOT_WIDTH);
  blank[SPOT_WIDTH] = '\0';
  int i, j;
  for (i = BOARD_Y+1; i < BOARD_Y+boardHeight; i+=2) {
    for (j = BOARD_X+OUTER_SPACING_X; j < boardWidth-OUTER_SPACING_X; j+=SPOT_WIDTH+INNER_SPACING_X)
      mvwprintw(window, i, j, "%s", blank);
  }
  free(blank);
}


/*
 * Fill in all spots in the board that have already been occupied by
 * a player.
 */
void fillPlayer(WINDOW* window, int player) {
  char *blank = malloc(SPOT_WIDTH+1);
  memset(blank, ' ', SPOT_WIDTH);
  blank[SPOT_WIDTH] = '\0';
  int i;
  for (i = 0; i < 42; ++i) {
    if (board[i] == player) {
      mvwprintw(window, (i/SPOTS_X)*2+BOARD_Y+1, (i%SPOTS_X)*(SPOT_WIDTH+INNER_SPACING_X)+BOARD_X+OUTER_SPACING_X, "%s", blank);
    }
  }
  free(blank);
}


/*
 * Fill in the disk that hovers just over the board if we 
 * want it to show.
 */
void fillCur(WINDOW* window) {
  char *blank = malloc(SPOT_WIDTH+1);
  memset(blank, ' ', SPOT_WIDTH);
  blank[SPOT_WIDTH] = '\0';
  mvwprintw(window, BOARD_Y-1, BOARD_X+OUTER_SPACING_X+(curLocation*(SPOT_WIDTH+INNER_SPACING_X)), "%s", blank);
  free(blank);
}


/*
 * Move a disk onto the board.
 */
int playMove(int col, int player) {
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
 * Wait for the server to establish the game and assign
 * players.
 */
int waitForReady() {

  // Tell the server that we want to play a game.
  int fd = open_clientfd("localhost", 5774);
  if (fd < 0) {
    perror(NULL);
    exit(0);
  }
  char msg[32] = "\0\0\0\0\0\0\0\0connectFour\0\0\0\0\0\0\0\0\0\0\0\0\0";
  write(fd, msg, 32);

  // Wait for the response which includes our port we will talk to later on.
  char res[2];
  read(fd, res, 2);
  printf("%c%c\n", res[0], res[1]);
  read(fd, res, 2);
  uint16_t port = ntohs(*((uint16_t*)res));
  printf("%" PRIu16 "\n", port);

  // Create a new connection with the other port.
  int gamefd = open_clientfd("localhost", (int)port);
  if (gamefd < 0) {
    perror(NULL);
    exit(0);
  }

  // Wait for the server to tell us we're all ready.
  int ready = 0;
  while (!ready) {
    if (read(gamefd, msg, 32) == 0) {
      perror(NULL);
      exit(0);
    }
    if (msg[0] == 1) {
      playerNum = (int)msg[1];
      break;
    }
  }

  // Close the connection to the main server because it's not needed.
  close(fd);

  return gamefd;
}


/*
 * When we have decided on a place we want to drop the token at,
 * this will submit it to the server for approval and checking
 * for win conditions.
 */
int submitMove(int fd, int col) {
  
  // Send the message to the server.
  char msg[2];
  msg[0] = 2;
  msg[1] = (char)col;
  if (write(fd, msg, 2) < 0)
    return 7;

  // Recieve the response.
  char res[10];
  if (read(fd, res, 10) == 0)
    return 7;

  return (int)res[0];
}


/*
 * Wait for the oppent to have made a move.
 */ 
int waitForOpponent(int fd, int *col) {
  
  // Loop incase we recieve some junk.
  char res[32];
  while (1) {
    if (read(fd, res, 32) == 0)
      return 7;

    // If the opcode is 3, then we recieved a move.
    if (res[0] == 3 || res[0] == 5) {
      *col = (int)res[1];
      break;
    } else {
      break;
    }
  }

  return (int)res[0];
}


/*
 * Register that a disk has been dropped by the specified 
 * player. After this is complete, redraw the screen.
 */
void dropDisk(int col, int player, WINDOW* window) {
  playMove(col, player);
  turns++;
  curPlayer = (curPlayer == 1) ? 2 : 1;
  curLocation = 3;
  wclear(window);
  draw(window);
}


/*
 * Gets a move by the opponent and take the appropriate
 * action for the consequences of that move.
 */
int opponentTurn(int fd, WINDOW* window) {
  int col = 0;
  int result = waitForOpponent(fd, &col);

  // They won...
  if (result == 5)
    win_status = (playerNum == 1) ? 2 : 1;
  

  // We tied!
  else if (result == 6)
    win_status = 3;

  // Connection problem
  else if (result == 7) {
    win_status = 4;
    wclear(window);
    draw(window);
    return 1;
  }

  // Our turn! Drop the opponent disk and get outta here.
  dropDisk(col, curPlayer, window);

  // If the win_status has changed, we're done.
  if (win_status != 0)
    return 1;

  return 0;
}


/*
 * open_clientfd - open connection to server at <hostname, port> 
 *   and return a socket descriptor ready for reading and writing.
 *   Returns -1 and sets errno on Unix error. 
 *   Returns -2 and sets h_errno on DNS (gethostbyname) error.
 */
/* $begin open_clientfd */
int open_clientfd(char *hostname, int port) {
  int clientfd;
  struct hostent *hp;
  struct sockaddr_in serveraddr;

  if ((clientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    return -1; /* check errno for cause of error */

  /* Fill in the server's IP address and port */
  if ((hp = gethostbyname(hostname)) == NULL)
    return -2; /* check h_errno for cause of error */
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  bcopy((char *)hp->h_addr_list[0], 
      (char *)&serveraddr.sin_addr.s_addr, hp->h_length);
  serveraddr.sin_port = htons(port);

  /* Establish a connection with the server */
  if (connect(clientfd, (SA *) &serveraddr, sizeof(serveraddr)) < 0)
    return -1;
  return clientfd;
}
