/*
 * display.c
 *
 *  Created on: 23.11.2011
 *      Author: matthias
 */
#ifdef __C51__
#if __C51__ = 750
#include <atmel/regx52.h>
#else
#include <atmel/at89x52.h>
#endif
#else
#include <at89x52.h>
#endif

#include "display.h"

unsigned char DisplayDataA[DISPLAY_COLORS*((DISPLAY_ROWS + 7)/8) * DISPLAY_COLS];
unsigned char DisplayDataB[DISPLAY_COLORS*((DISPLAY_ROWS + 7)/8) * DISPLAY_COLS];
#ifdef __C51__
data unsigned char xdata *DisplayRead = DisplayDataA;
data unsigned char xdata *DisplayWrite = DisplayDataB;
#else
xdata unsigned char * data DisplayRead = DisplayDataA;
xdata unsigned char * data DisplayWrite = DisplayDataB;
#endif

#ifdef __C51__
data unsigned char xdata *AudioStream1;
data unsigned char xdata *AudioStream2;
data unsigned char xdata *AudioStream3;
data unsigned char xdata *AudioStream4;
#else
xdata unsigned char * data AudioStream[AUDIO_MAX_PARALLEL];
#endif
data unsigned short AudioStreamEnd1;
data unsigned short AudioStreamEnd2;
data unsigned short AudioStreamEnd3;
data unsigned short AudioStreamEnd4;

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
#elif defined(__C51__)
void timer1_isr(void) interrupt 5 using 2
#else
void timer1_isr(void)
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
		DisplayDataReg = DisplayRead[adrIdx];
	}
	DisplaySelectReg = col << 3;
	if(++col >= DISPLAY_COLS_PER_MATRIX)
	{
		col = 0;
		if(++color >= DISPLAY_COLORS)
		{
			color = 0;
		}
		/* todo set flag for frame completion, handle buffer change */
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
#elif defined(__C51__)
void timer0_isr(void) interrupt 1 using 1
#else
void timer0_isr(void)
#endif
{
	static bit highNibble = 0;

	unsigned char audioOut = 0;
	unsigned char audioTemp;

    if(AudioStream1 != AudioStreamEnd1)
    {
        audioTemp = *AudioStream1;
        if(highNibble)
        {
            AudioStream1++;
            audioTemp >>= 4;
        }
        audioOut += audioTemp & 0x0F;
    }

    if(AudioStream2 != AudioStreamEnd2)
    {
        audioTemp = *AudioStream2;
        if(highNibble)
        {
            AudioStream2++;
            audioTemp >>= 4;
        }
        audioOut += audioTemp & 0x0F;
    }

    if(AudioStream3 != AudioStreamEnd3)
    {
        audioTemp = *AudioStream3;
        if(highNibble)
        {
            AudioStream3++;
            audioTemp >>= 4;
        }
        audioOut += audioTemp & 0x0F;
    }

    if(AudioStream4 != AudioStreamEnd4)
    {
        audioTemp = *AudioStream4;
        if(highNibble)
        {
            AudioStream4++;
            audioTemp >>= 4;
        }
        audioOut += audioTemp & 0x0F;
    }

	highNibble = ~highNibble;
	SoundReg = audioOut;
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
