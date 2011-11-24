/*
 * display.h
 *
 *  Created on: 23.11.2011
 *      Author: matthias
 */

#ifndef DISPLAY_H_
#define DISPLAY_H_

/* size of display. Electrical specification of display may forbid changing these definitions. */
#define DISPLAY_ROWS 14
#define DISPLAY_COLS 20
#define DISPLAY_COLS_PER_MATRIX 5

/* sets the number of ON colors that can be generated.
 * implemented with one buffer per color, buffers are switched on output.
 */
#define DISPLAY_COLORS 2

void displayInit(void);


#endif /* DISPLAY_H_ */
