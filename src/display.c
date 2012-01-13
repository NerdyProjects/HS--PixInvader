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

/* Buffer layout: Two sets of: One byte per matrix-col, interleaved for all matrices:
 * (8 matrices, 5 cols each)
 * Byte 0: M0 C0, Byte 1: M1 C0,  Byte 7: M7 C0, Byte 8: M0 C1 .. Byte 39: M7 C4
 * Byte 0-3 are y < 7; Byte 4-7 are y >= 7  (x 0,5,10,15)
 * Byte 8-11               12-15            (x 1,6,11,16)
 * Byte 16-19              20-23            (x 2,7,12,17)
 *      24-27              28-31            (  3,8,13,18)
 *      32-35              36-39               4,9,14,19
 * -> Byte is (8*(x%5) + x/5 + (y >= 7) ? 4 : 0)
 * for easy output to display.
 * There may be 4 Colours, but only three will be implemented at first:
 * Off - both bits clear    -> works
 * 1/3 - lower bit set      -> will be 1/2 on
 * 2/3 - higher bit set     -> will be 1/2 on
 * 3/3 - both bits set      -> works
 */
#define DISPLAY_BUFFER_BYTES_PER_COLOR (DISPLAY_COLS_PER_MATRIX * DISPLAY_MATRICES)
pdata unsigned char DisplayDataA[DISPLAY_COLOR_BITS*DISPLAY_BUFFER_BYTES_PER_COLOR];
pdata unsigned char DisplayDataB[DISPLAY_COLOR_BITS*DISPLAY_BUFFER_BYTES_PER_COLOR];
pdata unsigned char DisplayDataBackground[DISPLAY_COLOR_BITS*DISPLAY_BUFFER_BYTES_PER_COLOR];
#ifdef __C51__
data unsigned char pdata *DisplayRead = DisplayDataA;
data unsigned char pdata *DisplayWrite = DisplayDataB;
data unsigned char pdata *DisplayNext = DisplayDataB;
#else
pdata unsigned char * data DisplayRead = DisplayDataA;
pdata unsigned char * data DisplayWrite = DisplayDataB;
pdata unsigned char * data DisplayNext = DisplayDataB;
#endif

static volatile bit BufferSwitchRequest;

/* we want about 2 ms ~ 500 Hz.
 * F = F_OSC / 2 / (65536 - RCAP2 HL)
 * 65536 - RCAP = F_OSC / F / 2
 * RCAP = -F_OSC / F / 2 + 65536
 * -24000000 / 500 / 2 + 65536
 * resulting frame rate will be much lower:
 * F / DISPLAY_COLS_PER_MATRIX / COLORS -> 500 / 5 / 2 -> 50 Hz (fps)
 * */
#define DISPLAY_REFRESH_RATE 500

#define DISPLAY_TIMER_RELOAD ((-F_OSC / DISPLAY_REFRESH_RATE / 12) + 65536UL)

#define DISPLAY_BLANK (0xF8)


/* Display ISR.
 * This serves display and gameplay timer.
 * Call frequency will be F_OSC / 12 / (65536 - RCAP2 HL) -> 2ms
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
	static data unsigned char cnt = 0;

	unsigned char i;
	unsigned char pdata * readPtr = DisplayRead + (color * DISPLAY_MATRICES * DISPLAY_COLS_PER_MATRIX + col * DISPLAY_MATRICES);

	for(i = 0; i < DISPLAY_MATRICES; ++i)
	{
		unsigned char colOut;
		colOut = *(readPtr++);
		DisplaySelectReg = DISPLAY_BLANK | i;
		DisplayDataReg = colOut;
	}

	DisplaySelectReg = col << 3;
	if(++col >= DISPLAY_COLS_PER_MATRIX)
	{	/* all columns outputted */
		col = 0;

		if(++color >= DISPLAY_COLORS)
		{	/* all colors outputted */
			color = 0;
			#if (DISPLAY_REFRESH_RATE / (DISPLAY_COLS_PER_MATRIX * DISPLAY_COLORS) == GAME_TIMEBASE_HZ)
				keyRead();
				gameTime();
			#else
				#error "Game timebase incorrect! see display interrupt code"
			#endif
			if(BufferSwitchRequest)
			{	/* we have some new data to draw */
				void pdata *tmp;
				tmp = DisplayNext;
				DisplayNext = DisplayRead;
				DisplayRead = tmp;
				BufferSwitchRequest = 0;
			}



		}
	}

}

/**
 * draws a pixel at given coordinate.
 * drawing multiple times with different colors darkens the pixel.
 * @param x x coordinate
 * @param y y coordinate
 * @param color color defineed in header file: 0-3 for off . . full
 */
void displayPixel(unsigned char x, unsigned char y, unsigned char color)
{
	unsigned char adrIdx = (DISPLAY_MATRICES*(x%DISPLAY_COLS_PER_MATRIX) + x/DISPLAY_COLS_PER_MATRIX + (y > 6) ? 4 : 0);
			//(x + ((y > 6) ? 20 : 0));	/* byte addressing: line 0-6: byte X, line 7-13: byte X+20 */
	unsigned char bitIdx = y % 7;	/* bit addressing: line % 7 -> 7 bits per byte used */
	if(color & 1)
		DisplayWrite[adrIdx] |= (1 << bitIdx);
	if(color & 2)
		DisplayWrite[adrIdx + DISPLAY_BUFFER_BYTES_PER_COLOR] |= (1 << bitIdx);
}

/**
 * Switches buffers and clears the new one.
 * Drawing target buffer is set to next buffer.
 */
void displayChangeBuffer(void)
{
	unsigned char i;
	BufferSwitchRequest = 1;
	while(BufferSwitchRequest)
		; /* todo: busy wait here is not the best idea... there may be some work to do */

	for(i = 0; i < DISPLAY_COLOR_BITS * DISPLAY_BUFFER_BYTES_PER_COLOR; ++i)
		DisplayNext[i] = 0;
	DisplayWrite = DisplayNext;
}

/**
 * Selects a third buffer as drawing target.
 * @param clear clear that buffer
 */
void displaySelectBackgroundBuffer(bit clear)
{
	unsigned char i;
	DisplayWrite = DisplayDataBackground;
	if(clear)
		for(i = 0; i < DISPLAY_COLOR_BITS * DISPLAY_BUFFER_BYTES_PER_COLOR; ++i)
			DisplayWrite[i] = 0;
}

/**
 * Copies background buffer to next buffer. Also sets drawing target to it.
 */
void displayApplyBackgroundBuffer(void)
{
	unsigned char i;

	for(i = 0; i < DISPLAY_COLOR_BITS * DISPLAY_BUFFER_BYTES_PER_COLOR; ++i)
		DisplayNext[i] = DisplayDataBackground[i];

	DisplayWrite = DisplayNext;
}

void displayInit(void)
{
	RCAP2H = ((DISPLAY_TIMER_RELOAD & 0xFF00) >> 8);
	RCAP2L = DISPLAY_TIMER_RELOAD;
	TR2 = 1;
	ET2 = 1;

	DisplaySelectReg = DISPLAY_BLANK;
	DisplayDataReg = 0;
	DisplayRead = DisplayDataA;
	DisplayWrite = DisplayDataB;
	DisplayNext = DisplayDataB;
}
