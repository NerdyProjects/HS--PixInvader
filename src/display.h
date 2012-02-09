/**
 * @date 23.11.2011
 * @author matthias
 * @author nils
 */

#ifndef DISPLAY_H_
#define DISPLAY_H_

#include "display_def.h"

void displayInit(void);
void displayPixel(unsigned char x, unsigned char y, unsigned char color);
void displayChangeBuffer(void);
void displayOff(void);

#endif /* DISPLAY_H_ */
