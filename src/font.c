/*
 * font.c
 *
 *  Created on: 18.01.2012
 *      Author: matthias
 */


#include "main.h"
#include "font_impl.h"
#include "font.h"
#include "display.h"

#define FONT_HEIGHT	6
#define FONT_HEIGHT_DISPLAY 5

/* looks up index of character in font index table */
static unsigned char getIndex(char c)
{
	unsigned char idx = 0;
	while(__font_index__[idx] != c && idx < (sizeof(__font_index__) / sizeof(__font_index__[0])))
		idx++;

	if(__font_index__[idx] != c)
		idx = 0;

	return idx;
}

static unsigned char getWidth(char c)
{
	unsigned char idx;

	idx = getIndex(c);

	return __font_widths__[idx];
}

/**
 * displays a character at given position.
 * @param x, y: position of character TOP LEFT
 * @param c: character to draw
 * @return width of character
 */
unsigned char displayChar(unsigned char x, unsigned char y, char c)
{
	unsigned char idx;
	unsigned char i;
	unsigned char j;

	idx = getIndex(c);

	for(i = 0; i < FONT_HEIGHT_DISPLAY; ++i)
	{
		unsigned char fontLine;
		fontLine = __font_bitmap__[FONT_HEIGHT * idx + i];
		for(j = 0; j < 8; ++j)
		{
			if(fontLine & (1 << 7 - j))
				displayPixel(x + j, y + i, COLOR_FULL);
		}
	}

	return __font_widths__[idx];
}

/**
 * displays a string at given position.
 * @param x, y: position
 * @param str: pointer to null terminated string
 * @param breakmode: how to handle display end reached
 * @return width of text. This is crap for multiline text *todo*
 */
unsigned char displayString(unsigned char x, unsigned char y, const char *str, LINEBREAK_MODE breakmode)
{
	unsigned char xOld = x;

	while(*str)
	{
		char c = *(str++);
		if(x + getWidth(c) - 3 >= DISPLAY_COLS)
		{	/* allow one column be off the display */
			if(c == ' ')	/* omit whitespace at line start */
				if(!(c = *(str++)))
						break;

			if(breakmode == LINEBREAK_NONE)
				break;
			else if(breakmode == LINEBREAK_X0)
				x = 0;
			else
				x = xOld;

			y += FONT_HEIGHT_DISPLAY;

			if(y >= (DISPLAY_ROWS - FONT_HEIGHT_DISPLAY/2))
				break;	/* only draw when there is a chance you can read it */

		}
		x += displayChar(x, y, c);
	}

	return x - xOld;
}

