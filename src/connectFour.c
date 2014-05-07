#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <inttypes.h>

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
int checkWin();
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

  // Wait for a character input
  while(gameInProgress) {

    // Get the next key pressed.
    int c = wgetch(window);

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
        win_status = 1;

      // Or maybe it ended up with a tie.
      else if (result == 6)
        win_status = 3;

      // If we're here then our result was 3, meaning a successful move.
      dropDisk(curLocation, window);

      // If the win_status has changed, we're done.
      if (win_status != 0)
        break;

      // Now wait for the other user to go.
      curPlayer = (curPlayer == 1) ? 2 : 1;
      int gameover = opponentTurn(fd, window);
      if (gameover)
        break;


      // if (playMove() == 0)
      //   continue;
      // win = checkWin();
      //  if (win) {
      //    wclear(window);
      //    draw(window);
      //    break;
      //  }
      //turns++;
      //curPlayer = (curPlayer == 1) ? 2 : 1;
      //curLocation = 3;
      //wclear(window);
      //draw(window);
      //if (turns == SPOTS_X*SPOTS_Y)
      //  break;

    }
  }

  // Wait until user presses a button.
  wgetch(window);

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
  if (win_status == 0) {
    if (curPlayer == playerNum)
      mvwprintw(window, 1, 10, "Your turn!", curPlayer);
    else 
      mvwprintw(window, 1, 6, "Waiting for opponent", curPlayer);
  } else if (win_status == 1) {
    wattrset(window, COLOR_PAIR(playerNum+2));
    mvwprintw(window, 1, 10, "YOU WIN!", curPlayer);
    wattrset(window, COLOR_PAIR(1));
    mvwprintw(window, 2, 3, "Press any button to exit.");
  } else if (win_status == 2) {
    int opponent = (playerNum == 1) ? 2 : 1;
    wattrset(window, COLOR_PAIR(opponent+2));
    mvwprintw(window, 1, 9, "You lose :(", curPlayer);
    wattrset(window, COLOR_PAIR(1));
    mvwprintw(window, 2, 3, "Press any button to exit.");
  } else if (win_status == 3) {
    mvwprintw(window, 1, 9, "Yay, a tie!", curPlayer);
    mvwprintw(window, 2, 3, "Press any button to exit.");
  }

  /*
  if (turns == SPOTS_X*SPOTS_Y) {
    mvwprintw(window, 1, 11, "It's a tie!");
    mvwprintw(window, 2, 3, "Press any button to exit.");
  } else if (win_status == 0) {
    mvwprintw(window, 1, 8, "Player %i's Turn", curPlayer);
  } else {
    wattrset(window, COLOR_PAIR(win+2));
    mvwprintw(window, 1, 9, "Player %i wins!", curPlayer);
    wattrset(window, COLOR_PAIR(1));
    mvwprintw(window, 2, 3, "Press any button to exit.");
  }

  */

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

void init() {

  // Initialize our ncurses screen.
  initscr();
  start_color();

  // Set the options.
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
}

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

void fillCur(WINDOW* window) {
  char *blank = malloc(SPOT_WIDTH+1);
  memset(blank, ' ', SPOT_WIDTH);
  blank[SPOT_WIDTH] = '\0';
  mvwprintw(window, BOARD_Y-1, BOARD_X+OUTER_SPACING_X+(curLocation*(SPOT_WIDTH+INNER_SPACING_X)), "%s", blank);
  free(blank);
}

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

int checkWin() {

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
  char res[4];
  read(fd, res, 4);
  uint16_t port = ntohs(res[2]);
  printf("%c%c%" PRIu16 "\n", res[0], res[1], port);

  // Create a new connection with the other port.
  int gamefd = open_clientfd("localhost", (int)port);
  if (gamefd < 0) {
    perror(NULL);
    exit(0);
  }

  // Wait for the server to tell us we're all ready.
  int ready = 0;
  while (!ready) {
    if (read(gamefd, msg, 32) < 0) {
      perror(NULL);
      exit(0);
    }
    if (msg[0] == 1) {
      playerNum = (int)msg[1];
      break;
    }
  }

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
  write(fd, msg, 2);

  // Recieve the response.
  char res[10];
  read(fd, res, 10);

  return (int)res[0];
}


int waitForOpponent(int fd, int *col) {
  
  // Recieve the move
  char res[10];
  read(fd, res, 10);
  if (res[0] == 3)
    *col = (int)res[1];

  return (int)res[0];
}


void dropDisk(int col, int player, WINDOW* window) {
  playMove(col, player);
  turns++;
  curPlayer = (curPlayer == 1) ? 2 : 1;
  curLocation = 3;
  wclear(window);
  draw(window);
}

int opponentTurn(int fd, WINDOW* window) {
  int col = 0;
  int result = waitForOpponent(fd, &col);

  // They won...
  if (result == 5)
    win_status = 2;

  // We tied!
  else if (result == 6)
    win_status = 3;

  // Our turn! Drop the opponent disk and get outta here.
  dropDisk(col, curPlayer, window);
  curPlayer = (curPlayer == 1) ? 2 : 1;

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
