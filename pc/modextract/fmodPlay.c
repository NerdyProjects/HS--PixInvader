#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
/*
 * sound.c
 *
 *  Created on: 24.11.2011
 *      Author: matthias
 */


/* direct references in code - be careful when changing this!
 * maximum number of parallel played samples */
#define AUDIO_MAX_PARALLEL 4
/* X of them are 16 bit indexed samples: */
#define AUDIO_SAMPLE_16	4
/* and X 8 bit indexed: */
#define AUDIO_SAMPLE_8 0

/* number of total saved audio samples */
#define AUDIO_SAMPLES 16
/* number of total saved songs */
#define AUDIO_SONGS 4

#define bit unsigned char
#define data
#define xdata


typedef struct {
	void * sample;		/* point to begin of sample data */
	void * end;	/* length in samples */
	unsigned char nibble;	/* 0: sound data in low nibble, 1: sound data in high nibble */
	void * reload;	/* offset from beginning of sample. when loopEntry >= length
	 	 	 	 	 	 	 	   there will be no loop, but DC offset generated at output*/
} SAMPLE;

typedef struct {
	void  * pattern;	/* points to begin of 4-ch pattern data */
							/* each pattern must have an infinite loop or
							 * song-end command
							 */
} SONG;

/* stores increment, fractional increment values for each note C0-B4
 * + 1 value for playing without resample (256)*/
unsigned short PeriodTable[] = { 81, 86, 92, 97, 103, 109, 115, 122, 130,
		137, 145, 154, 163, 173, 183, 194, 206, 218, 231, 245, 256, 259, 275,
		291, 308, 326, 345, 366, 387, 411, 436, 462, 489, 518, 549, 581, 617,
		652, 690, 734, 775, 820, 872, 924, 975, 1033, 1098, 1162, 1234, 1303,
		1381, 1468, 1550, 1641, 1743, 1860, 1964, 2082, 2214, 2324, 2491 };

/*unsigned short PeriodTable[] = { 14, 14, 15, 16, 17, 18, 19, 20, 22, 23, 24, 26,
		27, 29, 31, 32, 34, 36, 38, 41, 256, 43, 46, 48, 51, 54, 58, 61, 65, 69, 73,
		77, 82, 86, 92, 97, 103, 109, 115, 122, 129, 137, 145, 154, 163, 172,
		183, 194, 206, 217, 230, 245, 258, 273, 291, 310, 327, 347, 369, 387,
		415 };
		//39800 Hz
		*/

/* imperfect first table :) change by try & error. Will just be added, so needs to be relative! */
signed char VibratoTable[] = { -3, 0, +3, +3, 0, -3, -3, 0, +3, +3, 0, -3, -3, 0, +3, +3, 0, -3, -3, 0, +3, +3, 0, -3};

unsigned char *ROM;

unsigned char data Pattern;
unsigned char data Line;
unsigned char xdata * xdata PatternOrderTable;

	/* befriend other compilers */
	SAMPLE SampleInfo[32];
	SONG SongInfo[4];

/* auxiliary for all resample-able streams: */			/* for each output sample: */
data unsigned short ASIncr[AUDIO_MAX_PARALLEL];			/* input sample offset += ASIncr */
data unsigned char ASIncrFracCnt[AUDIO_MAX_PARALLEL];	/* on overflow: input sample offset += 1 */
data unsigned char ASVolume[AUDIO_MAX_PARALLEL];		/* Volume of channel: 15 is max volume, 0 is zero volume (not audible) */


	unsigned char *AS[AUDIO_MAX_PARALLEL];
	unsigned char *ASEnd[AUDIO_SAMPLE_16];		/* end of sample (this will not be played)*/
	unsigned char *ASReload[AUDIO_SAMPLE_16];		/* loop start */



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

static bit getStreamRunning(unsigned char ch) {
	switch (ch) {
	case 0:
		return AS0R;
		break;
	case 1:
		return AS1R;
		break;
	case 2:
		return AS2R;
		break;
	case 3:
		return AS3R;
		break;
	}
	return 0;
}

static bit getNibbleSelect(unsigned char ch) {
	switch (ch) {
	case 0:
		return AS0N;
		break;
	case 1:
		return AS1N;
		break;
	case 2:
		return AS2N;
		break;
	case 3:
		return AS3N;
		break;
	}
	return 0;
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
	static unsigned char sample;
	int ch;
	/* output sample here */
	//fprintf(stderr, "call sample output\n");
	printf("%c", (sample << 2) + 128);
	sample = 0;
	for(ch = 0; ch < 3; ++ch)
	{
		if(getStreamRunning(ch))
		{
			int newFracCnt = ASIncrFracCnt[ch] + (ASIncr[ch] & 0x00FF);
			unsigned char samplePart;

			/* add to output */
			//fprintf(stderr, "ch %d play addr %p\n", ch, AS[ch]);
			samplePart = *AS[ch];
			if(getNibbleSelect(ch))
			{	/* high nibble */
				samplePart >>= 4;
			} else
			{
				samplePart &= 0x0F;
			}
			if(samplePart & 0x08)
			{	/* negative */
				samplePart |= 0xF0;
			}
			sample += samplePart;

			if(newFracCnt > 255)
			{
				AS[ch]++;
			}
			ASIncrFracCnt[ch] += ASIncr[ch] & 0x00FF;

			AS[ch] += (ASIncr[ch] >> 8);
			if(AS[ch] >= ASEnd[ch])
			{
				if(ASReload[ch] <= &ROM[0x00FF])
								setStreamRunning(ch, 0);
				//fprintf(stderr, "read over end\n");
				AS[ch] -= (ASEnd[ch] - ASReload[ch]);
			}
		}
	}
}
#endif

void soundInit(void)
{
}

static void setSampleTone(unsigned char channel, unsigned char period)
{
   ASIncr[channel] = PeriodTable[period];
   //fprintf(stderr, "set ch %d period ind %d to h: %d l: %d\n", channel, period, ASIncr[channel] >> 8, ASIncr[channel] & 0x00FF);
 }

/*
 * plays  a sound sample.
 * @param idx Sample number
 * @param channel output channel number
 * @param period which note should be played? looked up with periodTable
 * needs over 250 cycles! *todo* optimize
 */
void playSample(unsigned char idx, unsigned char channel, unsigned char period)
{
	setStreamRunning(channel, 0);
	AS[channel] = SampleInfo[idx].sample;
	ASReload[channel] = SampleInfo[idx].reload;
	ASEnd[channel] = SampleInfo[idx].end;
	setSampleTone(channel, period);
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
	Pattern = 0;
	Line = 0;
	PatternOrderTable = SongInfo[idx].pattern;
}

void stopSong(void)
{
	PatternOrderTable = 0;
}

/**
 * periodically called for module playback.
 * Callrate should be 2ms.
 */
void songTick(void) {
	static unsigned char durationTick; /* duration of a tick in 2ms steps */
	static unsigned char durationLine; /* duration of a line in ticks */
	static unsigned char tick; /* actual tick number */
	static unsigned char subTick;	/* counts 2ms timer steps until durationTick is reached */
	static unsigned char vibratoIdx[4];	/* stores index to vibrato table per channel*/
	static unsigned char lineDelayCnt;	/* counts how many lines we did already wait for FX 0xEE */

	if (PatternOrderTable == 0) /* nothing to be played, set default options */
	{
		durationLine = 3;
		durationTick = 15;
		tick = 0;
		subTick = durationTick;	/* let subtick overflow at first real call*/
		return;
	}

	if (subTick++ >= durationTick) {
		unsigned char channel;		/* ~19 cycles until we are here */
		unsigned char xdata * tempSongPosition = PatternOrderTable + 64 + 4*3*64*PatternOrderTable[Pattern] + 4*3*Line;
		unsigned char nextLine = Line + 1;
		subTick = 0;

		for (channel = 0; channel <= 3; ++channel) {
			unsigned char fx;
			unsigned char fxParam;
			unsigned char period;

			fx = tempSongPosition[0] & 0x0F;
			period = tempSongPosition[1];
			fxParam = tempSongPosition[2];

			if (tick == 0) {
				unsigned char sample;
				sample = tempSongPosition[0] >> 4;
				if (sample) {
					fprintf(stderr, "playing smp %d ch %d per %d\n", sample, channel, period);
					playSample(sample, channel, period);		/* additionally 55 cycles + playSample + 21 for loop/no fx -> at least 304 cycles + (n * playSample) -> worst case 624 cycles _without_ fx*/
				}
			}

			tempSongPosition += 3;

			switch (fx) {		/* worst case FX may be vibrato with ~60 cycles -> max. 240 extra cycles -> 864 cycles worst case in total. this is a bit too much,
								   but mostly we will not have worst case so it should be okay. */
			/* *todo*
			 * effects may introduce unwanted artifacts when ASIncr is modified!
			 * disabling interrupts may cause jitter (typical 30 cycles about max. 18µs)
			 * disabling the stream causes jitter in the length of one sample point
			 * -> disabling interrupts may be the best way to go, try without at first.
			 */
			case 0x00: /* Arpeggio */
				if(fxParam != 0)	/* shortcut for no effect */
				{
					if((tick % 3) == 0)
						setSampleTone(channel, period);
					else if((tick % 3) == 1)
						setSampleTone(channel, period + (fxParam >> 4));
					else
						setSampleTone(channel, period + (fxParam & 0x0F));
				}
				break;
			case 0x01: /* Portamento up */	/* portamento increase/decrease are very small pitch steps, applied every tick
			 	 	 	  3*4 equals ~1 semitone (depending on absolute note value)
			 	 	 	  Translate param to something intended in MOD file that works with this simple way */
				ASIncr[channel] += fxParam;
				break;
			case 0x02: /* Portamento down */
				ASIncr[channel] -= fxParam;
				break;
			case 0x03: /* Portamento to current note */ /* increment steps just like portamento (but automatically in the right direction); note may not be fully reached */
				/* do not implement. instead, translate to portamento up/down and precalculate the parameters (do not forget to change target note when it will not be reached) */
				break;
			case 0x04: /* Vibrato */ /* slide down and up again, continue vibrato in next line with parameters 00; param: rate, depth */
				if(tick == 0 && fxParam != 0)	/* new vibrato command */
					vibratoIdx[channel] = 0;	/* -> else we continue last vibrato applied to this channel */
												/* this _may_ cause unintended tones when the tone is changed within an ongoing vibrato */
				/* we DO NOT use rate and depth parameter... */
				ASIncr[channel] += VibratoTable[vibratoIdx[channel]++];

				/* we may want to skip the range check */
				if(vibratoIdx[channel] >= sizeof(VibratoTable))
					vibratoIdx[channel] = 0;

				break;
			case 0x05: /* 0x03 + volume slide */
				break;
			case 0x06: /* 0x04 + volume slide */
				break;
			case 0x07: /* tremolo: volume vibrate */
				break;
			case 0x09: /* sample offset: play given sample from position XX * 256 */
				/* umm... we already started the sample when we reach this code here :) */
				break;
			case 0xA0: /* volume slide: change volume each tick. params: upspeed, downspeed */
			{
				unsigned char newVol = ASVolume[channel];
				newVol += fxParam >> 4;
				newVol -= fxParam & 0x0F;
				if(newVol > 100)	/* subtraction overflow */
					newVol = 0;
				if(newVol > 15)
					newVol = 15;

				ASVolume[channel] = newVol;
				/* as long as we do not implement volume... */
				setStreamRunning(channel, newVol > 0);
				break;
			}
			case 0x0B: /* jump to order -> jump to line X * 16 */
				Pattern = fxParam;
				break;
			case 0x0C: /* Volume: scaled to internal volume format 0..15 */
				ASVolume[channel] = fxParam;
				/* as long as we do not implement volume... */
				setStreamRunning(channel, fxParam > 0);
				break;
			case 0x0D: /* pattern break -> increment line by X (0 = next line) */
				nextLine = fxParam + 64;
				fprintf(stderr, "pattern break to %d\n", nextLine);
				break;

			case 0x0E: /* extended commands */
				switch (fxParam & 0xF0) {
				case 0x90: /* restart at tick */
					break;
				case 0xC0: /* note cut at tick */
					if(tick == (fxParam & 0x0F) - 1)
						setStreamRunning(channel, 0);
					break;
				case 0xD0: /* note delay in ticks */
					if(tick < (fxParam & 0x0F))
						setStreamRunning(channel, 0);
					else
						setStreamRunning(channel, 1);
					break;
				case 0xE0: /* song delay in lines */
					if(tick == durationLine - 1)
					{	/* this will be the last tick for this line */
						fprintf(stderr, "song delay\n");
						if(lineDelayCnt++ >= fxParam & 0x0F)
							lineDelayCnt = 0;	/* delay reached -> continue */
						else
							nextLine = Line;	/* else stay at this line */
					}
					break;
				}
				break;
			case 0x0F: /* set song speed: MSB selects line(=1) or tick(=0) duration */
				if (fxParam & 0x80)
					durationLine = fxParam & 0x7F;
				else
					durationTick = fxParam;
				break;
			}

		}
		if (++tick == durationLine) {
			tick = 0;
			Line = nextLine;
			if(Line > 63) {
				Line -= 64;
				Pattern++;
			}
			fprintf(stderr, "next: order %d line %d\n", Pattern, Line);
		}
	}
}

#define ROM_SIZE 65536
#define CNT_SAMPLE_INFO		15
#define SIZE_SAMPLE_INFO	7
#define ADDR_SAMPLE_INFO	(0xFFFF - CNT_SAMPLE_INFO * SIZE_SAMPLE_INFO)
#define CNT_SONG_INFO		4
#define SIZE_SONG_INFO		2
#define ADDR_SONG_INFO		(ADDR_SAMPLE_INFO - CNT_SONG_INFO * SIZE_SONG_INFO)
#define SOUND_ISR_FREQ		(20000000UL/12/256)
#define SONG_SUBTICK_FREQ	(500)
int main(int argc, char **argv)
{
	FILE *romFile;
	int i;
	if(argc != 3)
	{
		printf("usage: program <rom image> <Pn or Sn for play Pattern n or play Sample n>\n");
	}

	romFile = fopen(argv[1], "rb");
	ROM = malloc(ROM_SIZE * sizeof(char));
	i = fread(&ROM[0x8000], 1, ROM_SIZE, romFile);
	fclose(romFile);
	fprintf(stderr, "load of %d bytes of rom data complete\n", i);
	/*SampleInfo = &ROM[ADDR_SAMPLE_INFO];
	SongInfo = &ROM[ADDR_SONG_INFO]; */
	/* we have incompatible data types, so read in the info structure.
	 * plain sound data can be read directly from rom image */
	for(i = 0; i < CNT_SAMPLE_INFO; ++i)
	{
		int idx = ADDR_SAMPLE_INFO + i*SIZE_SAMPLE_INFO;
		SampleInfo[i].sample = &ROM[ROM[idx] * 256 + ROM[idx + 1]];
		SampleInfo[i].end = &ROM[ROM[idx+2] * 256 + ROM[idx + 3]];
		SampleInfo[i].nibble = ROM[idx+4];
		SampleInfo[i].reload = &ROM[ROM[idx+5] * 256 + ROM[idx + 6]];
		fprintf(stderr, "read raw    %d: %d nibble %d length %d loopstart %d\n", i, ROM[idx] * 256 + ROM[idx + 1], SampleInfo[i].nibble, ROM[idx+2] * 256 + ROM[idx + 3], ROM[idx+5] * 256 + ROM[idx + 6]);
		fprintf(stderr, "read sample %d: %d nibble %d length %d loopstart %d\n", i, SampleInfo[i].sample, SampleInfo[i].nibble, SampleInfo[i].end, SampleInfo[i].reload);
	}

	for(i = 0; i < CNT_SONG_INFO; ++i)
	{
		int idx = ADDR_SONG_INFO + i*SIZE_SONG_INFO;
		SongInfo[i].pattern = &ROM[ROM[idx] * 256 + ROM[idx + 1]];
		fprintf(stderr, "read song %d: starting at offset 0x%4X\n", i, (unsigned char*)SongInfo[i].pattern - ROM);
	}
	fprintf(stderr, "playing...\n");
	/* initialize */
	songTick();

	//playSample(0, 1, 27);

	playSong(0);
	while(1) {
		for(i = 0; i < (SOUND_ISR_FREQ/SONG_SUBTICK_FREQ); ++i)
		{
			timer0_isr();
		}
		songTick();
	}

	//songTick();
	//timer0_isr();
	return 0;
	
}
