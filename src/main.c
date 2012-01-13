#include "main.h"
#include "display.h"
#include "sound.h"
#include "keys.h"
#include "pixinvaders.h"

void main(void)
{
	PAGE_SELECT = PDATA_PAGE;		/* not really neccessary, but so everything is clear... */
	keyInit();
	displayInit();
	soundInit();
	EA = 1;
	while(1)
	{
		game();
	}

}
