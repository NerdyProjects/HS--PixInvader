/**
 * @file keys.c
 * @date 26.11.2011
 * @author matthias
 * @author nils
 * @brief The key input module.
 */

#include "keys.h"
#include "main.h"

 /** Hardware Keyport register. */
#define KEY_PORT	P3

/** Initialize key hardware */
void keyInit(void)
{
	/* Enable inputs */
	KEY_PORT |= KEY_ALL;
}

/**
 * Test if given keys are pressed.
 * @param keyMask Mask with key to test. @see KEY_ALL
 */
bit KeyIsPressed(unsigned char keyMask)
{
	return ((KEY_PORT & keyMask));
}

