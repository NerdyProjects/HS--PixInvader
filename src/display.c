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

/* we want about 2 ms ~ 500 Hz.
 * F = F_OSC / 2 / (65536 - RCAP2 HL)
 * 65536 - RCAP = F_OSC / F / 2
 * RCAP = -F_OSC / F / 2 + 65536
 * -24000000 / 500 / 2 + 65536
 * */
#define DISPLAY_REFRESH_RATE 500

#ifndef F_OSC
#define F_OSC 24000000
#endif

#define DISPLAY_TIMER_RELOAD ((-F_OSC / DISPLAY_REFRESH_RATE / 2) + 65536)

#define DISPLAY_SELECT_OFF (0xF8)


/* timer 1 ISR.
 * This serves display and gameplay timer.
 * Call frequency will be F_OSC / 12 / (65536 - RCAP2 HL)
 *
 */
#ifdef SDCC
void timer1_isr(void) __interrupt (5) __using (2)
#else
void timer1_isr(void)
#endif
{
	static data unsigned char col = 0;
	unsigned char i;
	unsigned char adrIdx = col + DISPLAY_COLS_PER_MATRIX * 8;
	for(i = 7; i; --i)
	{
		DisplaySelectReg = DISPLAY_SELECT_OFF | i;
		adrIdx -= DISPLAY_COLS_PER_MATRIX;
		DisplayDataReg = DisplayRead[adrIdx];
	}
	DisplaySelectReg = col << 3;
	if(++col > 7)
	{
		col = 0;
		/* todo set flag for frame completion, handle grayscale buffer change */
	}

}


/* timer 0 ISR.
 * this ISR serves the sound timer.
 * Call frequency will be F_OSC / 12 / (256 - TH0).
 * F_OSC of 24 MHz leads to 7812 Hz .
 * total execution time must not exceed timer rate to prevent audio jitter!
 * */
#ifdef SDCC
void timer0_isr(void) __interrupt (1) __using (1)
#else
void timer0_isr(void)
#endif
{

}

void main(void)
{
	TMOD = M1_0;	/* 8 bit timer */
	TH0 = TIMER0_RELOAD;
	TR0 = 1;
	ET0 = 1;
	PT0 = 1;		/* high priority IRQ */

	RCAP2H = DISPLAY_TIMER_RELOAD >> 8;
	RCAP2L = DISPLAY_TIMER_RELOAD;
	TR2 = 1;
	ET2 = 1;

	DisplaySelectReg = 0;
	DisplayDataReg = 2;
	EA = 1;
	while(1)
		;
}
