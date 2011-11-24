/*
 * display.h
 *
 *  Created on: 23.11.2011
 *      Author: matthias
 */

#ifndef DISPLAY_H_
#define DISPLAY_H_

#if !defined(SDCC) && !defined(__C51__)
#define xdata
#define idata
#define data
#endif

#if defined(__C51__)
/* Keil declaration */
xdata volatile unsigned char DisplaySelectReg _at_ 0x4000;
xdata volatile unsigned char DisplayDataReg _at_ 0x2000;
xdata volatile unsigned char SoundReg _at_ 0x6000;
//#define M1_0 (T0_M1_)
#define M1_0 (0x02)

#elif defined(SDCC)
/* sdcc declaration */
xdata volatile __at (0x4000) unsigned char DisplaySelectReg ;
xdata volatile __at (0x2000) unsigned char DisplayDataReg ;
xdata volatile __at (0x6000) unsigned char SoundReg ;
#else
/* befriend other compilers */
unsigned char DisplaySelectReg;
unsigned char DisplayDataReg;
unsigned char SoundReg;
#endif

/* size of display. Electrical specification of display may forbid changing these definitions. */
#define DISPLAY_ROWS 14
#define DISPLAY_COLS 20
#define DISPLAY_COLS_PER_MATRIX 5

/* sets the number of ON colors that can be generated.
 * implemented with one buffer per color, buffers are switched on output.
 */
#define DISPLAY_COLORS 2

/* due to optimisations this is useless. do not change */
#define AUDIO_MAX_PARALLEL 4


#endif /* DISPLAY_H_ */
