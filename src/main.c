#include "main.h"
#include "display.h"
#include "sound.h"
#include "keys.h"

void main(void)
{
	keyInit();
	displayInit();
	soundInit();
	EA = 1;
	while(1)
		if(keyPress(KEY_ENTER))
		{
			displayInit();
		}
		;
}
