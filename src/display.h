/*
 * display.h
 *
 *  Created on: 23.11.2011
 *      Author: matthias
 */

#ifndef DISPLAY_H_
#define DISPLAY_H_

#ifndef SDCC
#define xdata
#define idata
#define data
#endif

#if 0
/* Keil declaration */
xdata volatile unsigned char DisplaySelectReg _at_ 0x4000;
xdata volatile unsigned char DisplayDataReg _at_ 0x2000;
#endif

/* sdcc declaration */
#ifdef SDCC
xdata volatile __at (0x4000) unsigned char DisplaySelectReg ;
xdata volatile __at (0x2000) unsigned char DisplayDataReg ;
#else
/* befriend other compilers */
unsigned char DisplaySelectReg;
unsigned char DisplayDataReg;
#endif

/* size of display. Electrical specification of display may forbid changing these definitions. */
#define DISPLAY_ROWS 14
#define DISPLAY_COLS 20

/* sets the number of ON colors that can be generated.
 * implemented with one buffer per color, buffers are switched on output.
 */
#define DISPLAY_COLORS 2


#endif /* DISPLAY_H_ */
