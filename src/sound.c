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
 * */
#ifdef SDCC
void timer0_isr(void) __interrupt (1) __using (1)
#elif !defined(__C51__)
//void timer0_isr(void) interrupt 1 using 1
void timer0_isr(void)
#endif
#ifndef __C51__
{
	static bit highNibble = 0;

	unsigned char audioOut = 0;
	unsigned char audioTemp;

    if(AudioStream[1] != AudioStreamEnd[1])
    {
        audioTemp = *AudioStream[1];
        if(highNibble)
        {
            AudioStream[1]++;
            audioTemp >>= 4;
        }
        audioOut += audioTemp & 0x0F;
    }

    if(AudioStream[2] != AudioStreamEnd[2])
    {
        audioTemp = *AudioStream[2];
        if(highNibble)
        {
            AudioStream[2]++;
            audioTemp >>= 4;
        }
        audioOut += audioTemp & 0x0F;
    }

    if(AudioStream[3] != AudioStreamEnd[3])
    {
        audioTemp = *AudioStream[3];
        if(highNibble)
        {
            AudioStream[3]++;
            audioTemp >>= 4;
        }
        audioOut += audioTemp & 0x0F;
    }

    if(AudioStream[4] != AudioStreamEnd[4])
    {
        audioTemp = *AudioStream[4];
        if(highNibble)
        {
            AudioStream[4]++;
            audioTemp >>= 4;
        }
        audioOut += audioTemp & 0x0F;
    }

	highNibble = ~highNibble;
	SoundReg = audioOut;
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
