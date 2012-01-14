/*
 * display.h
 *
 *  Created on: 23.11.2011
 *      Author: matthias
 */

#ifndef DISPLAY_H_
#define DISPLAY_H_

#include "display_def.h"

void displayInit(void);
void displayPixel(unsigned char x, unsigned char y, unsigned char color);
void displayChangeBuffer(void);


#endif /* DISPLAY_H_ */
