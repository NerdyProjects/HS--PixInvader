#include "main.h"
#include "display.h"
#include "sound.h"
#include "keys.h"
#include "pixinvaders.h"
#include "writeEeprom.h"


data volatile unsigned long waitVar;
void waitFewMs(void)
{
	waitVar = 0;
	while(waitVar < 2000UL)
		waitVar++;
}


void main(void)
{
	unsigned char x, y;
	unsigned char correctRead = 1;
	PAGE_SELECT = PDATA_PAGE;		/* not really neccessary, but so everything is clear... */

	//keyInit();
	displayInit();
	/*soundInit(); */
	EA = 1;

	while(1)
	{
		handleSPI();
		for(x = 0; x < 20; ++x)
		{
			for(y = 0; y < 14; ++y)
			{
				displayPixel(x, y, COLOR_FULL);
				displayChangeBuffer();
				waitFewMs();
			}
		}
	}
}
