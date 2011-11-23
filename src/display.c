/*
 * display.c
 *
 *  Created on: 23.11.2011
 *      Author: matthias
 */

#include <at89x52.h>

#include "display.h"

unsigned char DisplayDataA[DISPLAY_COLORS*((DISPLAY_ROWS + 7)/8) * DISPLAY_COLS];
unsigned char DisplayDataB[DISPLAY_COLORS*((DISPLAY_ROWS + 7)/8) * DISPLAY_COLS];
xdata unsigned char * data DisplayRead;
xdata unsigned char * data DisplayWrite;

/* IRQ rate: F_OSC / 12 / (256 - RELOAD) */
#define TIMER0_RELOAD 0		/* 7812,5 Hz */
/* F_OSC / 12 / (256 - RELOAD) / DISPLAY_DIVIDER */
#define DISPLAY_DIVIDER 16 /* 488,28 Hz -> ~ 2 ms */


static void refreshDisplay(void)
{

}


/* timer 0 ISR. todo: register bank 0 may be changed.
 * this ISR serves the sound, display and gameplay timer.
 * Call frequency will be F_OSC / 12 / (256 - TH0).
 * F_OSC of 24 MHz leads to 7812 Hz .
 * Division of 16 for display leads to 48,83 Hz picture refresh rate (2 gray steps, 5 columns)*/
#ifdef SDCC
void timer0_isr(void) __interrupt (1) __using (0)
#else
void timer0_isr(void)
#endif
{
	static data unsigned char displayDivider = DISPLAY_DIVIDER;
	if(--displayDivider == 0)
	{
		displayDivider = DISPLAY_DIVIDER;
		refreshDisplay();
	}

}

void main(void)
{
	TMOD = M1_0;
	TH0 = TIMER0_RELOAD;
	TR0 = 1;
	DisplaySelectReg = 0;
	DisplayDataReg = 2;
	while(1)
		;
}
