/*
 * display.c
 *
 *  Created on: 23.11.2011
 *      Author: matthias
 */

#include "main.h"
#include "display.h"
#include "keys.h"
#include "pixinvaders.h"

#if defined(__C51__)
/* Keil declaration */
xdata volatile unsigned char DisplaySelectReg _at_ ADDR_DISPLAY_SELECT;
xdata volatile unsigned char DisplayDataReg _at_ ADDR_DISPLAY_DATA;
//#define M1_0 (T0_M1_)
#define M1_0 (0x02)

#elif defined(SDCC)
/* sdcc declaration */
static xdata volatile __at (ADDR_DISPLAY_SELECT) unsigned char DisplaySelectReg ;
static xdata volatile __at (ADDR_DISPLAY_DATA) unsigned char DisplayDataReg ;
#else
/* befriend other compilers */
static unsigned char DisplaySelectReg;
static unsigned char DisplayDataReg;
#endif


xdata unsigned char DisplayDataA[DISPLAY_COLORS*((DISPLAY_ROWS + 7)/8) * DISPLAY_COLS];
xdata unsigned char DisplayDataB[DISPLAY_COLORS*((DISPLAY_ROWS + 7)/8) * DISPLAY_COLS];
#ifdef __C51__
data unsigned char xdata *DisplayRead = DisplayDataA;
data unsigned char xdata *DisplayWrite = DisplayDataB;
#else
xdata unsigned char * data DisplayRead = DisplayDataA;
xdata unsigned char * data DisplayWrite = DisplayDataB;
#endif

static volatile bit BufferSwitchRequest;

/* we want about 2 ms ~ 500 Hz.
 * F = F_OSC / 2 / (65536 - RCAP2 HL)
 * 65536 - RCAP = F_OSC / F / 2
 * RCAP = -F_OSC / F / 2 + 65536
 * -24000000 / 500 / 2 + 65536
 * */
#define DISPLAY_REFRESH_RATE 500

#define DISPLAY_TIMER_RELOAD ((-F_OSC / DISPLAY_REFRESH_RATE / 2) + 65536UL)

#define DISPLAY_SELECT_OFF (0xF8)


/* timer 1 ISR.
 * This serves display and gameplay timer.
 * Call frequency will be F_OSC / 12 / (65536 - RCAP2 HL)
 *
 */
#ifdef SDCC
void timer2_isr(void) __interrupt (5) __using (2)
#elif defined(__C51__)
void timer2_isr(void) interrupt 5 using 2
#else
void timer2_isr(void)
#endif
{
	static data unsigned char col = 0;
	static data unsigned char color = 0;
	unsigned char i;
	unsigned char adrIdx = color * (DISPLAY_COLS_PER_MATRIX * 8) +
					col + DISPLAY_COLS_PER_MATRIX * 8;
	for(i = 7; i; --i)
	{
		DisplaySelectReg = DISPLAY_SELECT_OFF | i;
		adrIdx -= DISPLAY_COLS_PER_MATRIX;
		DisplayDataReg = ~DisplayRead[adrIdx];
	}
	DisplaySelectReg = col << 3;
	if(++col >= DISPLAY_COLS_PER_MATRIX)
	{
		col = 0;
		if(++color >= DISPLAY_COLORS)
		{
			color = 0;
			if(BufferSwitchRequest)
			{
				void xdata *tmp;
				tmp = DisplayWrite;
				DisplayWrite = DisplayRead;
				DisplayRead = tmp;
				BufferSwitchRequest = 0;
			}
			#if (DISPLAY_REFRESH_RATE / (DISPLAY_COLS_PER_MATRIX * DISPLAY_COLORS) == GAME_TIMEBASE_HZ)
				keyRead();
				gameTime();
			#else
				#error "Game timebase incorrect! see display interrupt code"
			#endif
		}
	}

}

void displayPixel(unsigned char x, unsigned char y, unsigned char color)
{
	unsigned char adrIdx = (x + ((y > 6) ? 20 : 0));	/* byte addressing: line 0-6: byte X, line 7-13: byte X+20 */
	unsigned char bitIdx = y % 7;	/* bit addressing: line % 7 -> 7 bits per byte used */
	if(color)
		DisplayWrite[adrIdx] |= (1 << bitIdx);
	if(color >= 2)
		DisplayWrite[adrIdx + 40] |= (1 << bitIdx);
}

/**
 * Switches buffers and clears the new one.
 */
void displayChangeBuffer(void)
{
	unsigned char i;
	BufferSwitchRequest = 1;
	while(BufferSwitchRequest)
		; /* todo: busy wait here is not the best idea... there may be some work to do */

	for(i = 0; i < DISPLAY_COLORS * DISPLAY_COLS * 2; ++i)
		DisplayWrite[i] = 0;
}

void displayInit(void)
{
	RCAP2H = (DISPLAY_TIMER_RELOAD >> 8);
	RCAP2L = DISPLAY_TIMER_RELOAD;
	TR2 = 1;
	ET2 = 1;

	DisplaySelectReg = 0;
	DisplayDataReg = 2;
	DisplayRead = DisplayDataA;
	DisplayWrite = DisplayDataB;
}
