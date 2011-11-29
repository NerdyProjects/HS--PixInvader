#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <term.h>
#include <assert.h>

    #include <termios.h>

    int getch();

    int getch()

    {

       static int ch = -1, fd = 0;

       struct termios neu, alt;

       fd = fileno(stdin);

       tcgetattr(fd, &alt);

       neu = alt;

       neu.c_lflag &= ~(ICANON|ECHO);

       tcsetattr(fd, TCSANOW, &neu);

       ch = getchar();

       tcsetattr(fd, TCSANOW, &alt);

       return ch;

    }

#define bit unsigned char

#include "../display.h"
#include "../keys.h"

#include "../pixinvaders.h"

char PixelData[DISPLAY_COLS][DISPLAY_ROWS];

/* set up the terminal */
void init_terminal(void)
{
  char *term = getenv("TERM");
  int err_ret;

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
		out = 219;
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
	//cursor_move(1,1);
	clrscrn();

	printf("|");
	for(x = 0; x < 20; ++x)
		printf("%d", x%10);
	for (y = 0; y < 14; ++y)
	{
		printf("|\n%d", y%10);
		for (x = 0; x < 20; ++x)
			printf("%c", PixelData[x][y]);

	}
	printf("\n ");
	for(x = 0; x < 20; ++x)
		printf("%d", x%10);
	printf("\n");
	fflush(stdout);
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

int main(int argc, char **argv)
{
#ifdef _DEBUG
fprintf(stderr, "\n\n\t###GAME STARTS###\n");
#endif
	pthread_t threads[3];
	init_terminal();
	clearPixelBuffer();
	pthread_create(&threads[0], NULL, runGame, NULL);
	pthread_create(&threads[1], NULL, timer, NULL);
	//pthread_create(&threads[2], NULL, display, NULL);
	while(1)
		;

	return 0;
}
