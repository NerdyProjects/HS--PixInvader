/*
 * sound.c
 *
 *  Created on: 24.11.2011
 *      Author: matthias
 */


#include "main.h"
#include "sound.h"

/* direct references in code - be careful when changing this!
 * maximum number of parallel played samples */
#define AUDIO_MAX_PARALLEL 4
/* X of them are 16 bit indexed samples: */
#define AUDIO_SAMPLE_16	4
/* and X 8 bit indexed: */
#define AUDIO_SAMPLE_8 0

/* number of total saved audio samples */
#define AUDIO_SAMPLES CNT_SAMPLE_INFO
/* number of total saved songs */
#define AUDIO_SONGS CNT_SONG_INFO


typedef struct {
	void xdata * sample;		/* point to begin of sample data */
	unsigned short length;	/* length in samples */
	unsigned char nibble;	/* 0: sound data in low nibble, 1: sound data in high nibble */
	unsigned short loopEntry;	/* offset from beginning of sample. when loopEntry >= length
	 	 	 	 	 	 	 	   there will be no loop, but DC offset generated at output*/
} SAMPLE;

typedef struct {
	void xdata * pattern;	/* points to begin of 4-ch pattern data */
							/* each pattern must have an infinite loop or
							 * song-end command
							 */
} SONG;

data SONG Song;

#if defined(__C51__)
	/* Keil declaration */
	xdata volatile unsigned char SoundReg _at_ ADDR_SOUND_REG;
	xdata SAMPLE SampleInfo[AUDIO_SAMPLES] _at_ ADDR_SAMPLE_INFO;
	xdata SONG   SongInfo[AUDIO_SONGS]   _at_ ADDR_SONG_INFO;
	//#define M1_0 (T0_M1_)
	#define M1_0 (0x02)

#elif defined(SDCC)
	/* sdcc declaration */
	static xdata volatile __at (ADDR_SOUND_REG) unsigned char SoundReg ;
	static xdata __at (ADDR_SAMPLE_INFO) SAMPLE SampleInfo[AUDIO_SAMPLES];
	static xdata __at (ADDR_SONG_INFO) SONG SongInfo[AUDIO_SONGS];
#else
	/* befriend other compilers */
	static unsigned char SoundReg;
	SAMPLE SampleInfo[AUDIO_SAMPLES];
	SONG SongInfo[AUDIO_SONGS];
#endif


/* IRQ rate: F_OSC / 12 / (256 - RELOAD) */
#define TIMER0_RELOAD 0		/* 7812,5 Hz */

/* auxiliary for all resample-able streams: */			/* for each output sample: */
data unsigned char ASIncr[AUDIO_MAX_PARALLEL];			/* input sample offset += ASIncr */
data unsigned char ASIncrFrac[AUDIO_MAX_PARALLEL];		/* ASIncrFracCnt += ASIncrFrac */
data unsigned char ASIncrFracCnt[AUDIO_MAX_PARALLEL];	/* on overflow: input sample offset += 1 */


#if defined (__C51__)
	/* base pointer to sample */
	data unsigned char xdata *AS[AUDIO_MAX_PARALLEL];
	/* 16 bit indexed samples: */
	data unsigned char xdata *ASEnd[AUDIO_SAMPLE_16];		/* end of sample (this will not be played)*/
	data unsigned char xdata *ASReload[AUDIO_SAMPLE_16];		/* loop start */
#elif defined(SDCC)
	/* base pointer to sample */
	xdata unsigned char  * data AS[AUDIO_MAX_PARALLEL];
	/* 16 bit indexed samples: */
	xdata unsigned char  * data ASEnd[AUDIO_SAMPLE_16];		/* end of sample (this will not be played)*/
	xdata unsigned char  * data ASReload[AUDIO_SAMPLE_16];		/* loop start */
#else
	unsigned char *AS[AUDIO_MAX_PARALLEL];
	unsigned char *ASEnd[AUDIO_SAMPLE_16];		/* end of sample (this will not be played)*/
	unsigned char *ASReload[AUDIO_SAMPLE_16];		/* loop start */
#endif



bit AS0N;			/* nibble select. 1: high nibble, 0: low nibble */
bit AS1N;			/* (for each stream) */
bit AS2N;
bit AS3N;

bit AS0R;			/* stream running? */
bit AS1R;
bit AS2R;
bit AS3R;



#if AUDIO_SAMPLE_8 > 0
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

		if(AS[i] != ASEnd[i])
		{
			audioTemp = *AS[i];
			if(!lowNibble)
			{
				AS[i]++;
				audioTemp >>= 4;
			}
			audioTemp &= 0x0F;
		} else {
			audioTemp = 7;
		}
		audioOut += audioTemp;
	}

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

/**
 * <= 19 cycles including RET
 */
static void setNibbleSelect(unsigned char idx, bit high) {
	switch (idx) {
	case 0:
		AS0N = high;
		break;
	case 1:
		AS1N = high;
		break;
	case 2:
		AS2N = high;
		break;
	case 3:
		AS3N = high;
		break;
	default:
		break;
	}
}

/**
 * <= 19 cycles including RET
 */
static void setStreamRunning(unsigned char idx, bit run) {
	switch (idx) {
	case 0:
		AS0R = run;
		break;
	case 1:
		AS1R = run;
		break;
	case 2:
		AS2R = run;
		break;
	case 3:
		AS3R = run;
		break;
	default:
		break;
	}
}

/*
 * plays  a sound sample.
 * @param idx Sample number
 * @param channel output channel number
 * @param incr integer increment of sample pointer each audio tick
 * @param incrFrac fractional increment of sample pointer each audio tick
 *
 * needs over 250 cycles! *todo* optimize
 */
void playSample(unsigned char idx, unsigned char channel, unsigned char incr, unsigned char incrFrac)
{
	setStreamRunning(channel, 0);
	AS[channel] = SampleInfo[idx].sample;
	ASReload[channel] = SampleInfo[idx].sample + SampleInfo[idx].loopEntry;
	ASEnd[channel] = SampleInfo[idx].sample + SampleInfo[idx].length + 1;
	ASIncr[channel] = incr;
	ASIncrFrac[channel] = incrFrac;
	ASIncrFracCnt[channel] = 0;
	setNibbleSelect(channel, SampleInfo[idx].nibble);
	setStreamRunning(channel, 1);
}

/*
 * plays a song.
 *
 * @param idx Song number
 */
void playSong(unsigned char idx)
{
	Song.pattern = SongInfo[idx].pattern;

}
