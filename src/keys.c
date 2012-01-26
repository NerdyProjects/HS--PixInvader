/*
 * keys.c
 *
 *  Created on: 26.11.2011
 *      Author: matthias
 */


#include "keys.h"
#include "main.h"


#define KEY_PORT	P2

#define KEY_ALL		(KEY_LEFT | KEY_RIGHT | KEY_ENTER | KEY_AUX)


static data volatile unsigned char KeyState;
static data volatile unsigned char KeyPressed;

void keyInit(void)
{
	/* Enable inputs */
	KEY_PORT |= KEY_ALL;
}

/**
 * has to be called periodically to read in keys.
 * all key presses that are shorter than calls of this will be missed.
 */
void keyRead(void) using 2
{
	unsigned char oldState = KeyState;
	KeyState = KEY_PORT;	/* key state, key port: 1 = key is pressed. */
	KeyPressed |=	KeyState & ~oldState;   /* press should be one a) oldState = 0, key State = 1; b) press was one before */
}

/* returns true, when a given key have been pressed.
 * @param key number of key
 * @return false/true
 * sideeffect: clears internal key press state on positive result.
*/

bit keyPress(unsigned char keyMask)
{
	return 0;
	if((KeyPressed & keyMask) == keyMask)
	{
		KeyPressed = 0;
		return 1;
	}
	return 0;

}

bit KeyIsPressed(unsigned char keyMask)
{
	return ((KeyState & keyMask) == keyMask);
}

