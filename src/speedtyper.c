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
#include <signal.h>
#include <fcntl.h>

#ifndef MAX_SENTENCE_LENGTH
#define MAX_SENTENCE_LENGTH 78
#endif
#ifndef MAX_SENTENCES
#define MAX_SENTENCES 5
#endif

typedef struct sockaddr SA;

/*
 * Prototype functions
 */
void init();
void initColors();
void draw(WINDOW* window, int cur, int isYou);
int submitMove(int fd, int cur);
int open_clientfd(char* host, int port);
int waitForReady();
int updateOpp(int fd, WINDOW* window);

/*
 * Global variables
 */
char sentences[MAX_SENTENCE_LENGTH];
int youCur = 0;
int youDone = 0;
int oppCur = 0;
int oppDone = 0;
int winner = 0;



int main() {

  // Wait until server connection is established and opponent
  // is confirmed.
  int fd = waitForReady();
  fcntl(fd, F_SETFL, O_NONBLOCK);

  // Initialize ncurses and some options.
  init();

  // Initialize some custom colors.
  initColors();

  // Create two windows -- one for us and one for our opponent.
  int height = 4;
  int width = 80;
  int x = 0;
  int y = 0;
  WINDOW* youWindow = newwin(height, width, y, x);

  y = 5;
  WINDOW* oppWindow = newwin(height, width, y, x);

  // Draw window with initial settings.
  draw(youWindow, youCur, 1);
  draw(oppWindow, oppCur, 0);

  // Flush the input buffer
  nodelay(youWindow, true);
  while (wgetch(youWindow) != ERR);

  // Wait for a character input
  while(1) {

    // Get the next key pressed.
    char c = wgetch(youWindow);

    if (c == ERR) {
      if (updateOpp(fd, oppWindow) == 7) {
        winner = -1;
        wclear(youWindow);
        draw(youWindow, youCur, 1);
        sleep(1);
  
  nodelay(youWindow, false);
        // Wait until user presses a button.
        wgetch(youWindow);
        close(fd);
        endwin();
        return 0;
      }
    }

    if (c == sentences[youCur]) {
      do {
        youCur++;
      } while (sentences[youCur] == ' ');
      wclear(youWindow);
      draw(youWindow, youCur, 1);
      submitMove(fd, youCur);
      if (youCur == strlen(sentences))
        youDone = 1;
    }
    
    if (youDone && oppDone)
      break;
  }

  // Flush the character buffer.
  while (wgetch(youWindow) != ERR);
  nodelay(youWindow, false);

  fcntl(fd, F_SETFL, 0);
  char buf[2];
  read(fd, buf, 2);
  winner = (int)buf[1];
  wclear(youWindow);
  wclear(oppWindow);
  draw(youWindow, 0, 1);
  draw(oppWindow, 0, 0);

  sleep(1);
  
  // Wait until user presses a button.
  wgetch(youWindow);

  // Close the connection
  close(fd);

  // Close the screen.
  endwin();
  return 0;
}


int updateOpp(int fd, WINDOW* window) {
  char msg[2];
  bzero(msg, 2);
  ssize_t size = read(fd, msg, 2);
  if (size > 0 && msg[0] == 2) {
    oppCur = msg[1];
    wclear(window);
    draw(window, oppCur, 0);
    if (oppCur == strlen(sentences))
      oppDone = 1;
  }

  return msg[0];
}



void draw(WINDOW* window, int cur, int isYou) {
  wattrset(window, COLOR_PAIR(1));
  box(window, 0, 0);
  touchwin(window);

  if (winner == 0) {
    if (isYou) {
      mvwprintw(window, 1, 1, "Type this:");
    } else {
      mvwprintw(window, 1, 1, "Opponent progress:");
    }
  } else if (winner == -1) { 
      mvwprintw(window, 1, 1, "Opponent left!");
      mvwprintw(window, 1, 57, "Press any key to exit.");
  } else {
    if ((isYou && winner == 1) || (isYou == 0 && winner == 2))
      mvwprintw(window, 1, 1, "Winner!");
    else
      mvwprintw(window, 1, 1, "Loser!");

    if (isYou)
      mvwprintw(window, 1, 57, "Press any key to exit.");
  } 

  int i;
  for (i = 0; i < cur; ++i) {
    mvwprintw(window, 2, 1+i, "%c", sentences[i]);
  }
  int color = isYou ? 3 : 4;
  wattrset(window, COLOR_PAIR(color));
  for (; i < strlen(sentences); ++i) {
    mvwprintw(window, 2, 1+i, "%c", sentences[i]);
  }
  wattrset(window, COLOR_PAIR(1));

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
  signal(SIGWINCH, SIG_IGN);
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

  short p1FG = COLOR_BLUE;
  short p1BG = COLOR_BLACK;
  short p1Num = 3;
  init_pair(p1Num, p1FG, p1BG);

  short p2FG = COLOR_RED;
  short p2BG = COLOR_BLACK;
  short p2Num = 4;
  init_pair(p2Num, p2FG, p2BG);
}


/*
 * Wait for the server to establish the game and assign
 * players.
 */
int waitForReady() {

  // Tell the server that we want to play a game.
  int fd = open_clientfd("ecuiffo.com", 5774);
  if (fd < 0) {
    perror(NULL);
    exit(0);
  }
  char msg[32] = "\0\0\0\0\0\0\0\0speedtyper\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
  write(fd, msg, 32);

  // Wait for the response which includes our port we will talk to later on.
  char res[2];
  read(fd, res, 2);
  read(fd, res, 2);
  uint16_t port = ntohs(*((uint16_t*)res));

  // Create a new connection with the other port.
  int gamefd = open_clientfd("ecuiffo.com", (int)port);
  if (gamefd < 0) {
    perror(NULL);
    exit(0);
  }

  // Wait for the server to tell us we're all ready.
  int ready = 0;
  while (!ready) {
    if (read(gamefd, msg, 32) == 0) {
      fprintf(stderr, "Opponent has disconnected.\n");
      exit(0);
    }
    if (msg[0] == 1) {
      memcpy(sentences, msg+2, msg[1]);
      sentences[(int)msg[1]] = '\0';
      break;
    }
  }

  // Close the connection to the main server because it's not needed.
  close(fd);

  return gamefd;
}


int submitMove(int fd, int cur) {
  
  // Send the message to the server.
  char msg[2];
  msg[0] = 2;
  msg[1] = (char)cur;
  if (write(fd, msg, 2) < 0)
    return 7;

  return (int)msg[0];
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
