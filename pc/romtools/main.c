#include <stdio.h>
#include <malloc.h>

void printUsage(void)
{
  printf("Usage: program <data layout file> <output binary file>\n");
}

typedef struct {
  char *filename;
  int loopEntry;
} FILEINFO;

typedef struct {
	unsigned short sample;		/* point to begin of sample data */
	unsigned short length;	/* length in samples */
	unsigned char nibble;	/* 0: sound data in low nibble, 1: sound data in high nibble */
	unsigned short loopEntry;	/* offset from beginning of sample. when loopEntry >= length
	 	 	 	 	 	 	 	   there will be no loop, but DC offset generated at output*/
} SAMPLE;

#define SIZE_SAMPLE_INFO 7
#define SIZE_SONG_INFO 2

int main(int argc, char **argv)
{
  FILE *layout, *output;
  unsigned char *rom;
  int songStart;
  int songCount;
  int sampleStart;
  int sampleCount;
  int romStart;
  int romSize;
  int song = 0;
  int sample = 0;
  unsigned char type;
  int i;
  int lowNibbleUsed = 0;
  int highNibbleUsed = 0;
  int songDataAddr;
  FILEINFO *songs, *samples;

  if(argc != 3)
  {
    printUsage();
    return 1;
  }
  layout = fopen(argv[1], "r");
  output = fopen(argv[2], "w");

  while(fscanf(layout, "%c ", &type) != EOF)
  {
    switch(type)
    {
      case '#':
        while(fgetc(layout) != '\n')
	  ;
	break;
      case 'A':
        fscanf(layout, "%d %d\n", &sampleStart, &sampleCount);
	samples = malloc(sampleCount * sizeof(*samples));
	break;
      case 'B':
        fscanf(layout, "%d %d\n", &songStart, &songCount);
	songs = malloc(songCount * sizeof(*songs));
	break;
      case 'R':
        fscanf(layout, "%d %d\n", &romStart, &romSize);
	rom = calloc(1, romSize * sizeof(*rom));
	break;
      case 'S':
        samples[sample].filename = malloc(100);
	fscanf(layout, "%s %d\n", samples[sample].filename, &samples[sample].loopEntry);
	sample++;
	break;
      case 'M':
        songs[song].filename = malloc(100);
	fscanf(layout, "%s\n", songs[song].filename);
	song++;
	break;
    }
  }
  printf("Rom: 0x%4X to 0x%4X (output file will start at first rom address, all printed addresses refer to system rom address)\n", romStart, romSize + romStart - 1);
  printf("Placing %d sample info structs at 0x%4X\n", sampleCount, sampleStart);
  printf("Placing %d song info structs at 0x%4X\n", songCount, songStart);
  for(i = 0; i < sample; i++)
  {
    FILE *smp;
    SAMPLE info;
    int bytes = 0;
    unsigned char buffer;
    int targetNibble = 0;
    int writeIdx = lowNibbleUsed;
    if(lowNibbleUsed > highNibbleUsed)
    {
      targetNibble = 1;
      writeIdx = highNibbleUsed;
    }

    info.sample = romStart + writeIdx;
    info.nibble = targetNibble;
    

    smp = fopen(samples[i].filename, "r");
    printf("opened file %s\n", samples[i].filename);
    while(fread(&buffer, 1, 1, smp) == 1)
    {
      if(targetNibble)
        buffer <<= 4;
      else
        buffer &= 0x0F;
      rom[writeIdx++] |= buffer;
      bytes++;
    }
    fclose(smp);
    if(targetNibble)
      highNibbleUsed += bytes;
    else
      lowNibbleUsed += bytes;

    info.length = bytes;
    info.loopEntry = samples[i].loopEntry;
    printf("wrote file at %4X (nibble: %d, size: %d)\n", info.sample, info.nibble, info.length);

    rom[SIZE_SAMPLE_INFO * i + sampleStart - romStart + 1] = info.sample & 0x00FF;
    rom[SIZE_SAMPLE_INFO * i + sampleStart - romStart + 0] = (info.sample >> 8);
    rom[SIZE_SAMPLE_INFO * i + sampleStart - romStart + 3] = info.length & 0x00FF;
    rom[SIZE_SAMPLE_INFO * i + sampleStart - romStart + 2] = (info.length >> 8);
    rom[SIZE_SAMPLE_INFO * i + sampleStart - romStart + 4] = info.nibble;
    rom[SIZE_SAMPLE_INFO * i + sampleStart - romStart + 6] = info.loopEntry & 0x00FF;
    rom[SIZE_SAMPLE_INFO * i + sampleStart - romStart + 5] = (info.loopEntry >> 8);
    printf("wrote SAMPLE INFO at 0x%4X\n", SIZE_SAMPLE_INFO * i + sampleStart);

  }

  if(highNibbleUsed > lowNibbleUsed)
    songDataAddr = highNibbleUsed;
  else
    songDataAddr = lowNibbleUsed;

  for(i = 0; i < song; ++i)
  {
    
    FILE *smp;
    unsigned short pattern;
    int bytes = 0;
    unsigned char buffer;

    pattern = romStart + songDataAddr;

    smp = fopen(songs[i].filename, "r");
    printf("opened file %s\n", songs[i].filename);
    while(fread(&buffer, 1, 1, smp) == 1)
    {
      rom[songDataAddr++] = buffer;
      bytes++;
    }
    fclose(smp);

    printf("wrote file at %4X (size: %d)\n", pattern, bytes);

    rom[SIZE_SONG_INFO * i + songStart - romStart + 1] = pattern & 0x00FF;
    rom[SIZE_SONG_INFO * i + songStart - romStart + 0] = (pattern >> 8);
    printf("wrote SONG INFO at 0x%4X\n", SIZE_SONG_INFO * i + songStart);
  }
    

  fwrite(rom, 1, romSize, output);
  return 0;
}
        

