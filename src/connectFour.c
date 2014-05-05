#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>

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
int playMove();
int checkWin();


/*
 * Global variables
 */
int board[SPOTS_X*SPOTS_Y] = {0};
int boardWidth = 32;
int boardHeight = 20;
int curLocation = 3;
int curPlayer = 1;
int turns = 0;
int win = 0;

int main() {

  // Initialize ncurses and some options.
  init();

  // Initialize some custom colors.
  initColors();

  // Create an 32x20 window
  boardHeight = SPOT_HEIGHT*SPOTS_Y + INNER_SPACING_Y*(SPOTS_Y - 1) + 2*OUTER_SPACING_Y;
  int height = BOARD_Y + boardHeight + 1 ;
  boardWidth = SPOT_WIDTH*SPOTS_X + INNER_SPACING_X*(SPOTS_X - 1) + 2*OUTER_SPACING_X;
  int width = BOARD_X + boardWidth + 1;
  int x = 0;
  int y = 0;
  WINDOW* window = newwin(height, width, y, x);

  // Draw window with initial settings.
  draw(window);

  // Wait for a character input
  while(1) {
    int c = wgetch(window);
    if (c == 67) {
      curLocation++;
      curLocation = (curLocation > SPOTS_X-1) ? SPOTS_X-1 : curLocation;
      wclear(window);
      draw(window);
    } else if (c == 68) {
      curLocation--;
      curLocation = (curLocation < 0) ? 0 : curLocation;
      wclear(window);
      draw(window);
    } else if (c == 10) {
      if (playMove()) {
        win = checkWin();
        if (win) {
          wclear(window);
          draw(window);
          break;
        }
        turns++;
        curPlayer = (curPlayer == 1) ? 2 : 1;
        curLocation = 3;
        wclear(window);
        draw(window);
        if (turns == SPOTS_X*SPOTS_Y)
          break;
      }
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
  if (turns == SPOTS_X*SPOTS_Y) {
    mvwprintw(window, 1, 11, "It's a tie!");
    mvwprintw(window, 2, 3, "Press any button to exit.");
  } else if (win == 0) {
    mvwprintw(window, 1, 8, "Player %i's Turn", curPlayer);
  } else {
    wattrset(window, COLOR_PAIR(win+2));
    mvwprintw(window, 1, 9, "Player %i wins!", curPlayer);
    wattrset(window, COLOR_PAIR(1));
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

  if (win == 0 && turns != SPOTS_X*SPOTS_Y) {
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
  for (i = BOARD_Y; i <= BOARD_Y+12; ++i) {
    mvwprintw(window, i, BOARD_X, "%s", blank);
  }
  free(blank);
}

void fillHoles(WINDOW* window) {
  char *blank = malloc(SPOT_WIDTH+1);
  memset(blank, ' ', SPOT_WIDTH);
  blank[SPOT_WIDTH] = '\0';
  int i, j;
  for (i = BOARD_Y+1; i <= BOARD_Y+12; i+=2) {
    for (j = BOARD_X+OUTER_SPACING_X; j < boardWidth-OUTER_SPACING_X; j+=SPOT_WIDTH+INNER_SPACING_X)
      mvwprintw(window, i, j, blank);
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
      mvwprintw(window, (i/SPOTS_X)*2+BOARD_Y+1, (i%SPOTS_X)*(SPOT_WIDTH+INNER_SPACING_X)+BOARD_X+OUTER_SPACING_X, blank);
    }
  }
  free(blank);
}

void fillCur(WINDOW* window) {
  char *blank = malloc(SPOT_WIDTH+1);
  memset(blank, ' ', SPOT_WIDTH);
  blank[SPOT_WIDTH] = '\0';
  mvwprintw(window, BOARD_Y-1, BOARD_X+OUTER_SPACING_X+(curLocation*(SPOT_WIDTH+INNER_SPACING_X)), blank);
  free(blank);
}

int playMove() {
  int i;
  for (i = 5; i >= 0; --i) {
    int cur = i*7+curLocation;
    if (board[cur] == 0) {
      board[cur] = curPlayer;
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
