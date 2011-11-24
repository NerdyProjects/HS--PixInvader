#include "main.h"
#include "display.h"
#include "sound.h"

void main(void)
{
	displayInit();
	soundInit();
	EA = 1;
	while(1)
		;
}
