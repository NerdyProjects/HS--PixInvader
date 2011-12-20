/*
 * sound.h
 *
 *  Created on: 24.11.2011
 *      Author: matthias
 */

#ifndef SOUND_H_
#define SOUND_H_

/* due to optimisations this is useless. do not change */
#define AUDIO_MAX_PARALLEL 4
#define AUDIO_SAMPLE_16	4
#if 0
#define AUDIO_SAMPLE_8 3
#endif

typedef struct {
	void xdata * sample;		/* point to begin of sample data */
	unsigned short length;	/* length in samples */
	unsigned char nibble;	/* 0: sound data in low nibble, 1: sound data in high nibble */
	unsigned short loopEntry;	/* offset from beginning */
} SAMPLE;

void soundInit(void);

#endif /* SOUND_H_ */
