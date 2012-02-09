/**
 * @file main.c
 * @author matthias
 * @author nils
 * @brief The program entry.
 */


#include "main.h"
#include "display.h"
#include "sound.h"
#include "keys.h"
#include "pixinvaders.h"
#include "font.h"
#include "spi_command.h"


data volatile unsigned long waitVar;
/**
 * Waits a short Time. Depends direct on CPU Frequency.
 */
void waitFewMs(void)
{
	waitVar = 0;
	while(waitVar < 2000UL)
		waitVar++;
}

/**
 * displays scrolling texts.
 * length of text 2 MUST be >= length of text1.
 * text1 is taken for string end detection.
 * @param text1 line 1
 * @param text2 line 2
 */
static void showScrollScreen(char *text1, char *text2) {
	unsigned char i = 0;
	do {
		unsigned char j;
		displayString(4 - (i % 5), 0, text1 + i/5, LINEBREAK_NONE);
		displayString(4 - (i % 5), 7, text2 + i/5, LINEBREAK_NONE);
		displayChangeBuffer();
		i++;
		if (text1[i/5] == 0)
					i = 0;

		for (j = 0; (j < 1) && !KeyIsPressed(KEY_ALL); ++j) {
			waitFewMs();
		}
	} while (!KeyIsPressed(KEY_ALL));
}

/**
 * Displays the intro screen and waits for a key input.
 */
static void showIntroScreen(void) {
	char code text[] = "  Play now!   ";
	char code text2[] = "  Press key!   ";
	showScrollScreen(text, text2);
}

/**
 * Displays the lost screen and waits for a key input.
 * This function is calles after the player lost the game.
 */
static void showLostScreen(void) {
	char code text[] = "  Game over!   ";
	char code text2[] = "  Press key!    ";
	showScrollScreen(text, text2);
}

/**
 * Displays the won screen and waits for a key input.
 * This function is called after the player won the game.
 */
static void showWonScreen(void) {
	char code text[] = "  You won!!!   ";
	char code text2[] = "  Press key!   ";
	showScrollScreen(text, text2);
}

/**
 * Main function, program entry.
 */
void main(void)
{
	PAGE_SELECT = PDATA_PAGE;		/* not really neccessary, but so everything is clear... */

	keyInit();
	displayInit();
	soundInit();
	EA = 1;

	while(1)
	{
		unsigned char rc;
		handleSPI();
		LED_OFF();
		EA = 1;

		playSample(0,1,20);
		showIntroScreen();
		while(KeyIsPressed(KEY_ALL))
			;
		rc = game(); //Start a new game

		while(KeyIsPressed(KEY_ALL))
				;

		if(rc){
			playSong(0);
			showWonScreen();
		} else {
			playSong(1);
			showLostScreen();
		}
		while(KeyIsPressed(KEY_ALL))
			;
		stopSong();
	}
}
