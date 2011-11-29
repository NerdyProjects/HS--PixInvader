#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <term.h>
#include <assert.h>

#define bit unsigned char

#include "../display.h"
#include "../keys.h"

#include "../pixinvaders.h"

volatile unsigned char keyPressed;

char PixelData[DISPLAY_COLS][DISPLAY_ROWS];

/* set up the terminal */
void init_terminal(void)
{
  char *term = getenv("TERM");
  int err_ret;

  //initscr();
  if ( !term || ! *term) {
    fprintf(stderr,"ERROR: The TERM environment variable is not set.\n");
    exit(EXIT_FAILURE);
  }
  /* Note:  bsd would use err_ret = setterm(term); */
  setupterm(term, 1, &err_ret);

  switch (err_ret) {
  case -1:
    fprintf(stderr,"ERROR: Can't access terminfo database.\n");
    exit(EXIT_FAILURE);
    break;
  case 0:
    fprintf(stderr, "ERROR: Can't find entry for terminal %s\n",
	    term);
    exit(EXIT_FAILURE);
    break;
  case 1:
    break;
  default:
    fprintf(stderr, "ERROR:  Unknown setupterm() return code %d\n",
            err_ret);
    exit(EXIT_FAILURE);
  }
}

void clrscrn(void) { tputs(clear_screen, 1, putchar); }

/* cursor positioning */
/*  home is 1,1       */
void cursor_move(int col, int row)
{
  /* reject out of hand anything negative */
  if (--row < 0 || --col < 0) {
    return;
  }
  /* move the cursor */
  tputs(tparm(cursor_address, row, col), 1, putchar);
}

void keyInit(void)
{}

void keyRead(void)
{}

bit keyPress(unsigned char keyMask)
{
	/* todo semaphore needed! */
	if((keyPressed & keyMask) == keyMask)
	{
		keyPressed &= ~keyMask;
#ifdef _DEBUG
		fprintf(stderr, "keymask %X succeeded\n", keyMask);
#endif
		return 1;
	}
	return 0;

}

bit KeyIsPressed(unsigned char keyMask)
{
	return 0;

}

void displayInit(void)
{

}

void displayPixel(unsigned char x, unsigned char y, unsigned char color)
{
	char out;
	assert(color <= DISPLAY_COLORS);
	assert(x < DISPLAY_COLS);
	assert(y < DISPLAY_ROWS);
	switch(color)
	{
	case 1:
		out = '-';
		break;
	case 2:
		out = 'X';
		break;
	default:
		out = ' ';
		break;
	}
	PixelData[x][y] = out;
}

static void clearPixelBuffer(void)
{
	int x, y;
	for(y = 0; y < DISPLAY_ROWS; ++y)
	{
		for(x = 0; x < DISPLAY_COLS; ++x)
		{
			PixelData[x][y] = ' ';
		}
	}
}

void displayChangeBuffer(void)
{
	int x, y;
	clear();
	for (y = 0; y < 14; ++y)
	{
		//printf("|\n%d", y%10);
		for (x = 0; x < 20; ++x)
			//printf("%c", PixelData[x][y]);
			mvprintw(y+1,x+1,"%c", PixelData[x][y]);

	}
	refresh();
	clearPixelBuffer();
	usleep(10000);
}


void *timer(void *mydata)
{
	while(1)
	{
		gameTime();
		usleep(20000);
	}
}

void *runGame(void *mydata)
{
	game();
}

void *keys(void *mydata)
{
	char c;
	while(1)
	{
		c = getch();
#ifdef _DEBUG
		fprintf(stderr, "read key %c\n", c);
		fflush(stderr);
#endif
		switch(c)
		{
			case 's':
				keyPressed |= KEY_ENTER;
				break;
			case 'a':
				keyPressed |= KEY_LEFT;
				break;
			case 'd':
				keyPressed |= KEY_RIGHT;
				break;
			case 'q':
				return;
				break;
			default:
				break;
		}

	}
}
int main(int argc, char **argv)
{
#ifdef _DEBUG
fprintf(stderr, "\n\n\t###GAME STARTS###\n");
#endif
	pthread_t threads[3];
	//init_terminal();
	initscr();
	cbreak();
	noecho();
	clearPixelBuffer();
	pthread_create(&threads[0], NULL, runGame, NULL);
	pthread_create(&threads[1], NULL, timer, NULL);
	pthread_create(&threads[2], NULL, keys, NULL);
	pthread_join(threads[2], NULL);
	endwin();
	while(1)
		;

	return 0;
}
