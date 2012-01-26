/*
 * keys.c
 *
 *  Created on: 26.11.2011
 *      Author: matthias
 */


#include "keys.h"
#include "main.h"


#define KEY_PORT	P3



static data volatile unsigned char KeyState;
static data volatile unsigned char KeyPressed;

void keyInit(void)
{
	/* Enable inputs */
	KEY_PORT |= KEY_ALL;
}


bit KeyIsPressed(unsigned char keyMask)
{
	return ((KEY_PORT & keyMask) == keyMask);
}

