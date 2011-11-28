/*
 * sound.c
 *
 *  Created on: 24.11.2011
 *      Author: matthias
 */


#include "main.h"
#include "sound.h"

#if defined(__C51__)
/* Keil declaration */
xdata volatile unsigned char SoundReg _at_ 0x6000;
//#define M1_0 (T0_M1_)
#define M1_0 (0x02)

#elif defined(SDCC)
/* sdcc declaration */
static xdata volatile __at (0x6000) unsigned char SoundReg ;
#else
/* befriend other compilers */
static unsigned char SoundReg;
#endif


/* IRQ rate: F_OSC / 12 / (256 - RELOAD) */
#define TIMER0_RELOAD 0		/* 7812,5 Hz */

#ifdef __C51__
data unsigned char xdata *AudioStream[AUDIO_MAX_PARALLEL];
data unsigned char xdata *AudioStreamEnd[AUDIO_MAX_PARALLEL];
#else
xdata unsigned char * data AudioStream[AUDIO_MAX_PARALLEL];
xdata unsigned char * data AudioStreamEnd[AUDIO_MAX_PARALLEL];

#endif


/* timer 0 ISR.
 * this ISR serves the sound timer.
 * Call frequency will be F_OSC / 12 / (256 - TH0).
 * F_OSC of 24 MHz leads to 7812 Hz .
 * total execution time must not exceed timer rate to prevent audio jitter!
 * warning: SDCC compilation result is unusable slow - C code is supplied for reference only.
 * */
#ifdef SDCC
void timer0_isr(void) __interrupt (1) __using (1)
#elif !defined(__C51__)
//void timer0_isr(void) interrupt 1 using 1
void timer0_isr(void)
#endif
#ifndef __C51__
{
	static bit lowNibble = 0;

	static unsigned char audioOut;
	unsigned char audioTemp;
	unsigned char i;

	SoundReg = audioOut;
	audioOut = 0;

	for(i = 0; i < AUDIO_MAX_PARALLEL; ++i)
	{

		if(AudioStream[i] != AudioStreamEnd[i])
		{
			audioTemp = *AudioStream[i];
			if(!lowNibble)
			{
				AudioStream[i]++;
				audioTemp >>= 4;
			}
			audioTemp &= 0x0F;
		} else {
			audioTemp = 7;
		}
		audioOut += audioTemp;
	}

    lowNibble = ~lowNibble;
}
#endif

void soundInit(void)
{
	TMOD = M1_0;	/* 8 bit timer */
	TH0 = TIMER0_RELOAD;
	TR0 = 1;
	ET0 = 1;
	PT0 = 1;		/* high priority IRQ */
}
