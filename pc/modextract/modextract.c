/*
 * main.c
 *
 *  Created on: 01.12.2011
 *      Author: matthias
 */
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <math.h>
#include "periods.h"

#define FX_ARPEGGIO	0
#define FX_PORTAMENTO_UP 1
#define FX_PORTAMENTO_DOWN 2
#define FX_PORTAMENTO_TARGET 3
#define FX_VIBRATO	4
#define FX_PORTAMENTO_TARGET_VOL 5
#define FX_VIBRATO_VOL 6
#define FX_TREMOLO 7
#define FX_SAMPLE_OFFSET 9
#define FX_VOLUME_SLIDE 0xA
#define FX_POSITION_JUMP 0xB
#define FX_VOLUME 0xC
#define FX_PATTERN_JUMP 0xD
#define FX_EXTENDED 0xE
#define FX_SPEED 0xF
#define FX_E_FILTER 0
#define FX_E_FINESLIDE_UP 1
#define FX_E_FINESLIDE_DOWN 2
#define FX_E_GLISSANDO 3
#define FX_E_VIBRATO_WAVE 4
#define FX_E_FINETUNE 5
#define FX_E_LOOP 6
#define FX_E_TREMOLO_WAVE 7
#define FX_E_RETRIG_EVERY 9
#define FX_E_FINE_VOL_UP 0xA
#define FX_E_FINE_VOL_DOWN 0xB
#define FX_E_CUT_NOTE 0xC
#define FX_E_DELAY_NOTE 0xD
#define FX_E_DELAY_PATTERN 0xE
#define FX_E_INVERT 0xF

/* 1 period is a delay by 1/Frequency between output sample poi1nts */
#define MOD_PERIOD_FREQUENCY (7093789.2/2)
/* looked up period is in sample increments -> a value of 256 will increment the sample by 1 every 1/Frequency
 * 512 will increment the sample by 2 every 1/Frequency -> effectively we have 1/(2*frequency) between sample points
 * so looked up period: a delay by 256/(period*frequency) between sample points
 * -> period = 256/(delay*frequency)
 *  */
#define FMOD_PERIOD_BASE_FREQUENCY (20000000.0/12/256)


const char* FxNames[] = {"Arpeggio", "Portamento Up", "Portamento Down", "Portamento to Note", "Vibrato", "Portamento to Target and Volume Slide", "Vibrato and Volume Slide", "Tremolo",
		"0x08", "Sample Offset", "Volume slide", "Jump to position", "Volume", "Jump to pattern", "-Extended-", "Set speed", "enable filter", "fineslide up", "fineslide down",
		"glissando control", "vibrato waveform select", "finetune", "set loop", "tremolo waveform select", "0xE8", "retrigger note", "fine volumeslide up", "fine volumeslide down",
		"cut note", "delay note", "delay pattern", "invert pattern playback"};

#define FX_UNIMPLEMENTED(fx, param) (fprintf(stderr, "encountered unimplemented fx: %X %s (%2X)\n", (fx), FxNames[(((fx) > 0x0F) ? (((fx) > 0xE0 && (fx) < 0xEF) ? ((fx) - 0xE0 + 0x10) : 8) : (fx))], (param)))


#define FOUR_BIT_SAMPLES 1
#define FX_ENABLE_ARPEGGIO 1
typedef struct {
  int period;
  int sample;
  int fx;
  int fx_param;
} CHANNEL;

typedef struct {
  CHANNEL channel[4];
} DIVISION;

typedef struct {
  DIVISION division[64];
} PATTERN;

typedef struct {
  char *title;
  int finetune;
  int volume;
  int length;
  int repeatFrom;
  int repeatTo;
  signed char *data;
} SAMPLE;

typedef struct {
  char *title;
  char type[4];
  int length;
  SAMPLE sample[31];
  int patternorder[128];
  int numPattern;
  PATTERN *pattern;
} MOD;

void errmsg(char *msg)
{
  fprintf(stderr, msg);
  fprintf(stderr, "\n");
}

void parsePattern(PATTERN *pat, FILE *in, int channels) {
  unsigned char b[4];
  int ch;
  int div;
  int i;
  int fxOccurence[32];	/* count fx usage */
  for(i = 0; i < 32; ++i)
	  fxOccurence[i] = 0;

  for(div = 0; div < 64; ++div) {
    for(ch = 0; ch < channels; ++ch) {
      fread(b, 1, 4, in);
      pat->division[div].channel[ch].sample = (b[0] & 0xF0) | ((b[2] & 0xF0) >> 4);
      pat->division[div].channel[ch].period = ((b[0] & 0x0F) << 8) | b[1];
      pat->division[div].channel[ch].fx = b[2] & 0x0F;
      pat->division[div].channel[ch].fx_param = b[3];
      if(pat->division[div].channel[ch].fx == 0x0E) {
        //pat->division[div].channel[ch].fx += (b[3] >> 4);
        //pat->division[div].channel[ch].fx_param = (b[3] & 0x0F);
        fxOccurence[0x10 + (b[3] >> 4)]++;
      } else {
        //pat->division[div].channel[ch].fx_param = b[3];
        fxOccurence[pat->division[div].channel[ch].fx]++;
      }
    }

  }
  fprintf(stderr, "--- FX USAGE STATISTICS---\n");
  for(i = 0; i < 32; ++i)
  {
	  if(fxOccurence[i])
		  fprintf(stderr, "%s: %d\n", FxNames[i], fxOccurence[i]);
  }
  fprintf(stderr, "------\n");

}

void parseSample(SAMPLE *smp, FILE *in)
{
  unsigned char buffer[23];
  buffer[22] = 0;
  fread(buffer, 1, 22, in);
  smp->title = malloc(strlen((char*)buffer));
  strcpy (smp->title, (char*)buffer);
  fread(buffer, 1, 8, in);
  smp->length = buffer[0] * 512 + buffer[1] * 2;
  smp->finetune = (buffer[2] & 0x08)    /* twoth complement -> machine format */
      ? (-(~(buffer[2] & 0x07) + 1))  /* negative */
      : (buffer[2] & 0x07);           /* positive */
  smp->volume = buffer[3];
  if(smp->volume > 64) {
    errmsg("Volume of Sample out of range");
    smp->volume = 64;
  }

  smp->repeatFrom = buffer[4] * 512 + buffer[5] * 2;
  if(smp->repeatFrom > smp->length) {
    errmsg("repeat start > sample length!");
    smp->repeatFrom = 0;
  }
  smp->repeatTo = smp->repeatFrom + buffer[6] * 512 + buffer[7] * 2;
  if(smp->repeatTo == 2) {  /* no repeat */
    smp->repeatTo = 0;
  }
  if(smp->repeatTo > smp->length) {
    errmsg("repeat length of sample greater than sample length");
    smp->repeatTo = smp->length;
  }

}

void parseSampleData(SAMPLE *smp, FILE *in, int fourBit)
{
  int i;
  signed char sample;
  if(smp->length)
  {
	  smp->data = malloc(smp->length);
	  for(i = 0; i < smp->length; ++i) {
		fread(&sample, 1, 1, in);
		if(fourBit)
			smp->data[i] = (sample - 8 + 8*(i%2)) / 16;
		else
			smp->data[i] = sample;
	  }
  } else
  {
	  smp->data = 0;
  }
}

void parseMod(MOD *mod, FILE *in, int fourBit) {
  unsigned char buffer[128];
  int i;
  int channels = 4;
  buffer[20] = 0;
  fread(buffer, 1, 20, in);
  mod->title = malloc(strlen((char*)buffer));
  strcpy(mod->title, (char*)buffer);

  for(i = 0; i < 31; ++i) {
    parseSample(&(mod->sample[i]), in);
  }

  fread(buffer, 1, 2, in);
  mod->length = buffer[0];
  mod->numPattern = 0;

  fread(buffer, 1, 128, in);
  for(i = 0; i < 128; ++i) {
    int pattern = buffer[i];
    if(pattern > 127)
      pattern = 0;

    mod->patternorder[i] = pattern;
    if(pattern > mod->numPattern)
        mod->numPattern = pattern;
  }
  ++mod->numPattern;
  fread(mod->type, 1, 4, in);
  if(mod->type[0] == '2')
    channels = 2;

  mod->pattern = malloc(mod->numPattern * sizeof(PATTERN));

  for(i = 0; i < mod->numPattern; ++i){
    parsePattern(&(mod->pattern[i]), in, channels);
  }

  for(i = 0; i < 31; ++i) {
    parseSampleData(&mod->sample[i], in, fourBit);
  }
}

void printSampleInfo(SAMPLE *smp, int tabbed)
{
  printf("Title: %s\n", smp->title);
  printf("Length: %d\n", smp->length);
  printf("Repeat: %d - %i\n", smp->repeatFrom, smp->repeatTo);
  printf("Volume: %d\n", smp->volume);
  printf("Finetune: %d\n", smp->finetune);
}

void printPatternInfo(PATTERN *pat)
{
  int i, j;
  printf("smp\tper\tfx\tpar\tsmp\tper\tfx\tpar\tsmp\tper\tfx\tpar\tsmp\tper\tfx\tpar\n");
  for(i = 0; i < 64; ++i) {
    for(j = 0; j < 4; ++j) {
      printf("%d\t%d\t%X\t%d\t", pat->division[i].channel[j].sample, pat->division[i].channel[j].period, pat->division[i].channel[j].fx, pat->division[i].channel[j].fx_param);
    }
    printf("\n");
  }
}

void printModInfo(MOD *mod)
{
  int i;
  printf("Title: %s\n", mod->title);
  printf("Type: %c%c%c%c\n", mod->type[0], mod->type[1], mod->type[2], mod->type[3]);
  printf("Pattern: %d\n", mod->numPattern);
  for(i = 0; i < mod->length; ++i) {
    printf("pattern pos %d is %d\n", i, mod->patternorder[i]);
  }
  for(i = 0; i < 31; ++i) {
    printf("--- SAMPLES --- %d", i+1);
    printSampleInfo(&mod->sample[i], 0);
  }
}

void writeSamples(MOD *mod, char* prefix)
{
	char *outputFilename = malloc(strlen(prefix) + 10);
	int sample;
	for(sample = 0; sample < (sizeof(mod->sample) / sizeof(mod->sample[0])); ++sample)
	{
		if(mod->sample[sample].data)
		{
			FILE *output;
			int repeat = mod->sample[sample].repeatFrom;

			sprintf(outputFilename, "%s%d.smp", prefix, sample+1);
			output = fopen(outputFilename, "w");
			fwrite(mod->sample[sample].data, mod->sample[sample].length, 1, output);
			fclose(output);
			printf("# sample: %s (%d-%d; %d) 0x%X\n", mod->sample[sample].title, mod->sample[sample].repeatFrom, mod->sample[sample].repeatTo, mod->sample[sample].length, mod->sample[sample].data[0]);
			if(repeat == 0)
				repeat = mod->sample[sample].length;

			printf("S %s %d\n", outputFilename, repeat);
		}
	}
}

/**
 * translates period from mod to period of fmod.
 * fmod uses a table; matching table index is returned.
 * @param modPeriod period stored in mod file
 * @return index to period table for fmod
 */
static int translatePeriod(int modPeriod)
{
	int tblIdx = 0;
	int fmodPeriod = (256.0/(FMOD_PERIOD_BASE_FREQUENCY*modPeriod/MOD_PERIOD_FREQUENCY) + 0.5);
    while(Periods[tblIdx] < fmodPeriod)
    	tblIdx++;
    if(Periods[tblIdx] != fmodPeriod)
    	fprintf(stderr, "Missing exact period table match: wanted %d\n", fmodPeriod);

	return tblIdx;
}

static void writeOutputChannel(CHANNEL *ch, FILE* out)
{
	unsigned char byte[3];
	byte[0] = (ch->sample << 4) | ch->fx;
	byte[1] = ch->period;
	byte[2] = ch->fx_param;
	fwrite(byte, 1, 3, out);
}

#define FMOD_SONG_ORDER_TABLE_LENGTH 64
void playMod(MOD *mod, FILE* out, int *sampleTranslation)
{
  int pattern;
  int division;
  int currentSample[4];
  int currentPeriod[4];
  int ticksPerRow = 6;
  int ticksPerSecond = 125 * 2 / 5;
  int i;
  int numOutputLines = 0;
  int lastNote[4];
  int lastPortamentoNote[4];
  int usedSongOrder = 0;
  CHANNEL ch;		/* temporary channel from original mod */
  CHANNEL outCh = {0, 0, 0, 0};	/* temporary channel for destination fmod */

  int channels = 4;
  if(mod->type[0]=='2')
    channels = 2;
  for(i = 0; i < channels; ++i) {
    currentPeriod[i] = 0;
    currentSample[i] = 0;
  }

  printf("# Will need %d bytes for pattern data\n", 4*3* 64 * mod->numPattern);
  printf("# plus 64 bytes for song order table\n");

  for(i = 0; i < FMOD_SONG_ORDER_TABLE_LENGTH; ++i)
  {
	  unsigned char pat = 0;
	  if(i < mod->length)
	  {
		  pat = mod->patternorder[i];
		  usedSongOrder++;
	  }

	  fwrite(&pat, 1, 1, out);
  }
  fprintf(stderr, "wrote %d bytes of song order table, %d used\n", FMOD_SONG_ORDER_TABLE_LENGTH, usedSongOrder);

  for(pattern = 0; pattern < mod->numPattern; ++pattern)
  {
    for(division = 0; division < 64; ++division) {
      for(i = 0; i < channels; ++i) {
        ch = mod->pattern[pattern].division[division].channel[i];
        outCh.sample = sampleTranslation[ch.sample];
        outCh.period = 0;
        outCh.fx = 0;
        outCh.fx_param = 0;
        if(ch.sample)
        {
        	outCh.period = translatePeriod(ch.period);
        }
        switch (ch.fx) {
				case FX_ARPEGGIO: /* copy */
					outCh.fx = ch.fx;
					outCh.fx_param = ch.fx_param;
					break;
				case FX_PORTAMENTO_UP:
					outCh.fx = ch.fx;
					outCh.fx_param = ch.fx_param / 4;
					break;
				case FX_PORTAMENTO_DOWN:
					outCh.fx = ch.fx;
				    outCh.fx_param = ch.fx_param / 4;
					break;
				case FX_PORTAMENTO_TARGET:
					if(ch.sample)
					{
						if(outCh.period < lastNote[i])
						{
							outCh.fx = FX_PORTAMENTO_DOWN;
						}
						else
						{
							outCh.fx = FX_PORTAMENTO_UP;
						}
						lastPortamentoNote[i] = outCh.fx / 5;
					}
					else
					{
						outCh.fx = lastPortamentoNote[i];
					}
					outCh.fx_param = ch.fx_param / 5;
					break;
				case FX_VIBRATO:	/* just copy. target implementation does not use parameters */
					outCh.fx = ch.fx;
					outCh.fx_param = ch.fx_param;
					break;
				case FX_VIBRATO_VOL:
					FX_UNIMPLEMENTED(ch.fx, ch.fx_param);
					break;
				case FX_TREMOLO:
					FX_UNIMPLEMENTED(ch.fx, ch.fx_param);
					break;
				case FX_SAMPLE_OFFSET:
					FX_UNIMPLEMENTED(ch.fx, ch.fx_param);
					break;
				case FX_VOLUME_SLIDE:
					FX_UNIMPLEMENTED(ch.fx, ch.fx_param);
					break;
				case FX_POSITION_JUMP:
					outCh.fx = ch.fx;	/* copy */
					outCh.fx_param = ch.fx_param;

					break;
				case FX_VOLUME:		/* copy & scale to new max. volume*/
					outCh.fx = ch.fx;
					outCh.fx_param = (ch.fx_param * 15) / 64;
					break;
				case FX_PATTERN_JUMP:
					outCh.fx = ch.fx;	/* copy */
					outCh.fx_param = ch.fx_param;
					break;
				case FX_SPEED:
					outCh.fx = ch.fx;
					if(ch.fx_param <= 0x1F)
					{	/* duration of a line in ticks */
						outCh.fx_param = ch.fx_param | 0x80;
					}
					else
					{	/* duration of a tick in BPM. 125 BPM are 20 ms per tick.
						   we have 2 ms steps for tick duration.
						   duration_ms = 2500/bpm.
						   original bpm 20-250; target range 10-125
					 	 */
						outCh.fx_param = (2500.0 / ch.fx_param / 2) + 0.5;
					}
					break;
				case FX_EXTENDED:
					switch (ch.fx_param >> 4) {
					case FX_E_CUT_NOTE:
						outCh.fx = ch.fx;
						outCh.fx_param = ch.fx_param;
						break;
					case FX_E_DELAY_NOTE:
						outCh.fx = ch.fx;
						outCh.fx_param = ch.fx_param;
						break;
					case FX_E_DELAY_PATTERN:
						outCh.fx = ch.fx;
						outCh.fx_param = ch.fx_param;
						break;
					case FX_E_FILTER:
						FX_UNIMPLEMENTED(0xE0 | (ch.fx_param >> 4),
								ch.fx_param & 0x0F);
						break;
					case FX_E_FINESLIDE_DOWN:
						FX_UNIMPLEMENTED(0xE0 | (ch.fx_param >> 4),
								ch.fx_param & 0x0F);
						break;
					case FX_E_FINESLIDE_UP:
						FX_UNIMPLEMENTED(0xE0 | (ch.fx_param >> 4),
								ch.fx_param & 0x0F);
						break;
					case FX_E_FINETUNE:
						FX_UNIMPLEMENTED(0xE0 | (ch.fx_param >> 4),
								ch.fx_param & 0x0F);
						break;
					case FX_E_FINE_VOL_DOWN:
						FX_UNIMPLEMENTED(0xE0 | (ch.fx_param >> 4),
								ch.fx_param & 0x0F);
						break;
					case FX_E_FINE_VOL_UP:
						FX_UNIMPLEMENTED(0xE0 | (ch.fx_param >> 4),
								ch.fx_param & 0x0F);
						break;
					case FX_E_GLISSANDO:
						FX_UNIMPLEMENTED(0xE0 | (ch.fx_param >> 4),
								ch.fx_param & 0x0F);
						break;
					case FX_E_INVERT:
						FX_UNIMPLEMENTED(0xE0 | (ch.fx_param >> 4),
								ch.fx_param & 0x0F);
						break;
					case FX_E_LOOP:
						FX_UNIMPLEMENTED(0xE0 | (ch.fx_param >> 4),
								ch.fx_param & 0x0F);
						break;
					case FX_E_RETRIG_EVERY:
						FX_UNIMPLEMENTED(0xE0 | (ch.fx_param >> 4),
								ch.fx_param & 0x0F);
						break;
					case FX_E_TREMOLO_WAVE:
						FX_UNIMPLEMENTED(0xE0 | (ch.fx_param >> 4),
								ch.fx_param & 0x0F);
						break;
					case FX_E_VIBRATO_WAVE:
						FX_UNIMPLEMENTED(0xE0 | (ch.fx_param >> 4),
								ch.fx_param & 0x0F);
						break;
					}
					break;
				default:
					fprintf(stderr, "default\n");
					FX_UNIMPLEMENTED(ch.fx, ch.fx_param);
					break;
				}
        writeOutputChannel(&outCh, out);
        lastNote[i] = outCh.period;
      }
    }
  }
  fprintf(stderr, "generated output file with %d bytes!\n", ftell(out));
}

int main(int argc, char **argv) {
  FILE *modfile;
  FILE *outputPattern;
  MOD mod;
  int fourBit = 0;
  int sampleTranslationTable[32];
  int i;

  if(argc != 6)
  {
	  fprintf(stderr, "Usage: program <modfile> <prefix for samplefiles> <output pattern file> <convert samples to 4bit> <sample translation mod X is fmod Y: XXYY>\n");
	  return 1;
  }

  for(i = 0; i < 32; ++i)
	  sampleTranslationTable[i] = i;

  modfile = fopen(argv[1], "rb");

  if(argv[4][0] == '1')
  {
 	  fourBit = 1;
  	  fprintf(stderr, "enabled 4 bit sample conversion\n");
  }
  i = 0;
  while(argv[5][i] != 0)
  {
	  int modSample;
	  int fmodSample;
	  modSample = (argv[5][i] & 0xF) * 10 + (argv[5][i+1] & 0x0F);
	  fmodSample = (argv[5][i+2] & 0x0F) * 10 + (argv[5][i+3] & 0x0F);
	  fprintf(stderr, "%d->%d\n", modSample, fmodSample);
	  sampleTranslationTable[modSample] = fmodSample;
	  i+=4;

  }

  parseMod(&mod, modfile, fourBit);
  printModInfo(&mod);
  //printf("--- Info for pattern %d ---\n", 1);
  //printPatternInfo(&mod.pattern[0]);
  fclose(modfile);

  writeSamples(&mod, argv[2]);

  outputPattern = fopen(argv[3], "w");
  playMod(&mod, outputPattern, sampleTranslationTable);
  fclose(outputPattern);

  return 0;
}
