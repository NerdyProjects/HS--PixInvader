/*
 * font.h
 *
 *  Created on: 18.01.2012
 *      Author: matthias
 */

#ifndef FONT_H_
#define FONT_H_

/* do nothing, begin at x = 0, begin at given x */
typedef enum {LINEBREAK_NONE, LINEBREAK_X0, LINEBREAK_X_POS} LINEBREAK_MODE;

unsigned char displayChar(unsigned char x,unsigned  char y, char c);
unsigned char displayString(unsigned char x,unsigned char y, const char *str, LINEBREAK_MODE breakmode);
unsigned char displayNumber(unsigned char x, unsigned char y, unsigned char d);


#endif /* FONT_H_ */
