#include "main.h"
#include "display.h"
#include "sound.h"
#include "keys.h"
#include "pixinvaders.h"

void main(void)
{
	keyInit();
	displayInit();
	soundInit();
	EA = 1;
	while(1)
	{
		game();
	}

}
