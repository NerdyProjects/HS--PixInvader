#include "main.h"
#include "display.h"
#include "sound.h"
#include "keys.h"
#include "pixinvaders.h"
#include "font.h"
#include "spi_command.h"


data volatile unsigned long waitVar;
void waitFewMs(void)
{
	waitVar = 0;
	while(waitVar < 2000UL)
		waitVar++;
}

void waitALittleSecond(unsigned char wait)
{
  unsigned char i;
  for(i = 0; i < wait; ++i)
	  waitFewMs();
}

void main(void)
{
	unsigned char i;
	PAGE_SELECT = PDATA_PAGE;		/* not really neccessary, but so everything is clear... */

	//keyInit();
	displayInit();
	soundInit();
	EA = 1;
	//displayNumber(0, 0, SampleInfo[0].length >> 8);		// 100
	//displayNumber(0, 0, SampleInfo[0].loopEntry);			// 204
	/*for(i = 0; i < 12; ++i)
	{
		displayNumber(0, 0, i);
		displayChangeBuffer();
		playSample(i, 0, 20);
		waitALittleSecond(15);
	} */

	displayString(0, 0, "Song", LINEBREAK_NONE);
	displayNumber(6, 5, 2);
	displayChangeBuffer();
	stopSong();

	/*playSong(0);

	waitALittleSecond(200);
	waitALittleSecond(200);
	waitALittleSecond(200);
	stopSong();
	displayString(0, 4, "Stop", LINEBREAK_NONE);
	displayChangeBuffer(); */

	displayString(0, 0, "Dies ist ein Text!", LINEBREAK_X0);
	displayChangeBuffer();
	while(1)
	{
		handleSPI();
		LED_OFF();
		EA = 1;

		waitALittleSecond(20);
		/*for(x = 0; x < 20; ++x)
		{
			for(y = 0; y < 14; ++y)
			{
				displayPixel(x, y, COLOR_FULL);
				displayChangeBuffer();
				waitFewMs();
			}
		} */
		game();
	}
}
