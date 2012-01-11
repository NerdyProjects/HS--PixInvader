/*
 * keys.h
 *
 *  Created on: 26.11.2011
 *      Author: matthias
 */

#ifndef KEYS_H_
#define KEYS_H_
#include "main.h"

/* key bit definitions must correspond to port pin numbers */
#define KEY_LEFT	(1 << 0)
#define KEY_RIGHT	(1 << 1)
#define KEY_ENTER	(1 << 2)
#define KEY_AUX		(1 << 3)

void keyInit(void);
void keyRead(void);
bit keyPress(unsigned char keyMask);
bit KeyIsPressed(unsigned char keyMask);


#endif /* KEYS_H_ */
