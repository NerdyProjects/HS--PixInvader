/*
 * sound.h
 *
 *  Created on: 24.11.2011
 *      Author: matthias
 */

#ifndef SOUND_H_
#define SOUND_H_

#include "main.h"

void playSong(unsigned char idx);
extern void playSample(unsigned char idx, unsigned char channel, unsigned char period);
void soundInit(void);
void stopSong(void);


/* number of total saved audio samples */
#define AUDIO_SAMPLES CNT_SAMPLE_INFO
/* number of total saved songs */
#define AUDIO_SONGS CNT_SONG_INFO



typedef struct {
	void xdata * sample;		/* point to begin of sample data */
	void xdata * end;			/* points one after the last sample point */
	unsigned char nibble;		/* 0: sound data in low nibble, 1: sound data in high nibble */
	void xdata * loopEntry;		/* where to start the loop. Value < 0x0100 disable looping */
} SAMPLE;

typedef struct {
	void xdata * pattern;	/* first 64 bytes of pointer target will be patternOrderTable
							  points to begin of 4-ch pattern data */
							/* each pattern must have an infinite loop or
							 * song-end command
							 */
} SONG;


extern xdata SAMPLE SampleInfo[AUDIO_SAMPLES];
extern xdata SONG   SongInfo[AUDIO_SONGS];
extern xdata volatile unsigned char SoundReg;

#endif /* SOUND_H_ */
