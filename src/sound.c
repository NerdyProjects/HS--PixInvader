/**
 * @file sound.c
 * @date 24.11.2011
 * @author matthias
 * @author nils
 * @brief The sound module.
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



/* stores increment, fractional increment values for each note C0-B4
 * + 1 value for playing without resample (256)*/
unsigned short code PeriodTable[] = { 81, 86, 92, 97, 103, 109, 115, 122, 130,
		137, 145, 154, 163, 173, 183, 194, 206, 218, 231, 245, 256, 259, 275,
		291, 308, 326, 345, 366, 387, 411, 436, 462, 489, 518, 549, 581, 617,
		652, 690, 734, 775, 820, 872, 924, 975, 1033, 1098, 1162, 1234, 1303,
		1381, 1468, 1550, 1641, 1743, 1860, 1964, 2082, 2214, 2324, 2491 };

/* imperfect first table :) change by try & error. Will just be added, so needs to be relative! */
signed char code VibratoTable[] = { -1, 0, -1, -1, -1, -2, -1, -1, 0, 1, 1, 1, 2, 1, 1, 1, 0, 1, 0, 1, 1, 2, 1, 1, 1, 0, -1, -1, -1, -2, -1, -1, 0, -1 };

unsigned char data Pattern;
unsigned char data Line;
unsigned char xdata * xdata PatternOrderTable;

#if defined(__C51__)
	/* Keil declaration */
	volatile unsigned char xdata SoundReg _at_ ADDR_SOUND_REG;
	SAMPLE xdata SampleInfo[AUDIO_SAMPLES] _at_ ADDR_SAMPLE_INFO;	/* ALIGNMENT REQUIREMENT: must be within one 256 byte page! */
	SONG xdata SongInfo[AUDIO_SONGS]   _at_ ADDR_SONG_INFO;
#else
	/* befriend other compilers */
	static unsigned char SoundReg;
	SAMPLE SampleInfo[AUDIO_SAMPLES];
	SONG SongInfo[AUDIO_SONGS];
#endif


/* IRQ rate: F_OSC / 12 / (256 - RELOAD) */
#define TIMER0_RELOAD 0		/* 7812,5 Hz */

/* auxiliary for all resample-able streams: */			/* for each output sample: */
unsigned short data ASIncr[AUDIO_MAX_PARALLEL];			/* input sample offset += ASIncr */
unsigned char data ASIncrFracCnt[AUDIO_MAX_PARALLEL];	/* on overflow: input sample offset += 1 */
unsigned char data ASVolume[AUDIO_MAX_PARALLEL];		/* Volume of channel: 15 is max volume, 0 is zero volume (not audible) */


#if defined (__C51__)
	unsigned char xdata * data AS[AUDIO_MAX_PARALLEL];
	unsigned char xdata * data ASEnd[AUDIO_SAMPLE_16];		/* end of sample (this will not be played)*/
	unsigned char xdata * data ASReload[AUDIO_SAMPLE_16];		/* loop start */
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
#if 0
#pragma NOAREGS
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
#pragma AREGS
#endif
/**
 * <= 19 cycles including RET
 */
#pragma NOAREGS
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
#pragma AREGS

//const SAMPLE xdata *sample, unsigned char channel, unsigned char period)
/* ensure braces around parameters at the caller!! */
#define PLAY_SAMPLE(smp, channel, period) \
{ \
	setStreamRunning(channel, 0); \
	AS[channel] = smp->sample;	\
	ASReload[channel] = smp->loopEntry; \
	ASEnd[channel] = smp->end; \
	setSampleTone(channel, period);	\
	ASIncrFracCnt[channel] = 0;	\
	setNibbleSelect(channel, smp->nibble); \
	setStreamRunning(channel, 1); \
}


#pragma rb(2)
static void setSampleTone(unsigned char channel, unsigned char period)
{
   ASIncr[channel] = PeriodTable[period];
}
#pragma rb(0)

/*
 * plays  a sound sample.
 * @param idx Sample number
 * @param channel output channel number
 * @param period which note should be played? looked up with periodTable
 * 101 cycles + two times callee (2*21 -> 42 -> 143 cycles)
 * -> ASM version ~ 80 cycles
 */
#if 0
#pragma NOAREGS
void playSample(unsigned char idx, unsigned char channel, unsigned char period)
{
	SAMPLE xdata *sample = &SampleInfo[idx];
	if(channel == 0)
		PLAY_SAMPLE(sample, 0, period)
	else if(channel == 1)
		PLAY_SAMPLE(sample, 1, period)
	else if(channel == 1)
		PLAY_SAMPLE(sample, 2, period)
	else
		PLAY_SAMPLE(sample, 3, period)

}
#pragma AREGS
#endif

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
	AS0R = 0;		/* do not use setStreamRunning to avoid linker warning */
	AS1R = 0;		/* as setStreamRunning is called from interrupt */
	AS2R = 0;
	AS3R = 0;
}

/**
 * periodically called for module playback.
 * Callrate should be 2ms.
 */
#define DEFAULT_BPM 125
#pragma rb(2)
void songTick(void) {
	static unsigned char durationTick; 	/* duration of a tick in 2ms steps */
	static unsigned char durationLine; 	/* duration of a line in ticks */
	static unsigned char tick; 			/* actual tick number */
	static unsigned char subTick;		/* counts 2ms timer steps until durationTick is reached */
	static unsigned char vibratoIdx[4];	/* stores index to vibrato table per channel*/
	static unsigned char lineDelayCnt;	/* counts how many lines we did already wait for FX 0xEE */

	if (PatternOrderTable == 0) 	/* nothing to be played, set default options */
	{
		durationLine = 6;
		durationTick = (2500.0 / DEFAULT_BPM / 2) + 0.5;	/* see modfile TEMPO specification */
		tick = 0;
		subTick = durationTick;		/* let subtick overflow at first real call*/
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
					playSample(sample, channel, period);		/* additionally 55 cycles + playSample + 21 for
						loop/no fx -> at least 304 cycles + (n * playSample) -> worst case 624 cycles _without_ fx*/
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
				nextLine = 64 + fxParam;
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
		}
	}
}
#pragma rb(0)

