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
xdata SAMPLE SampleInfo _at_ ADDR_SAMPLE_INFO;
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

/* auxiliary for all resample-able streams: */			/* for each output sample: */
data unsigned char ASIncr[AUDIO_MAX_PARALLEL];			/* input sample offset += ASIncr */
data unsigned char ASIncrFrac[AUDIO_MAX_PARALLEL];		/* ASIncrFracCnt += ASIncrFrac */
data unsigned char ASIncrFracCnt[AUDIO_MAX_PARALLEL];	/* on overflow: input sample offset += 1 */

/* base pointer to sample */
data unsigned char xdata *AS[AUDIO_MAX_PARALLEL];
bit AS0N;			/* nibble select. 1: high nibble, 0: low nibble */
bit AS1N;			/* (for each stream) */
bit AS2N;
bit AS3N;
bit AS4N;
bit AS5N;

bit AS0R;			/* stream running? */
bit AS1R;
bit AS2R;
bit AS3R;
bit AS4R;
bit AS5R;

/* 16 bit indexed samples: */
data unsigned char xdata *ASEnd[AUDIO_SAMPLE_16];		/* end of sample (pointer!)*/
data unsigned char xdata *ASReload[AUDIO_SAMPLE_16];		/* loop start */
#if 0
/* 8 bit indexed samples:
 * Simplification: 8 bit indexed samples have to be completely in one 256 byte region.
 * There may be more than one, though. (2x128, 4x64, 64+192, 32+32,...) Index will not
 * be carry added to high address byte!
 * */
data unsigned char ASReadIdx[AUDIO_SAMPLE_8];	/* read index */
data unsigned char ASLoopFrom[AUDIO_SAMPLE_8];	/* loop start */
data unsigned char ASEndAt[AUDIO_SAMPLE_8];		/* end of sample (last played sample tone) */

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
	/*TR0 = 1;
	ET0 = 1;
	PT0 = 1;		/* high priority IRQ */
}

void playSample(unsigned char idx, unsigned char channel)
{
}
