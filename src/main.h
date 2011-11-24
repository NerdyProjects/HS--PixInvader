/*
 * main.h
 *
 *  Created on: 24.11.2011
 *      Author: matthias
 */

#ifndef MAIN_H_
#define MAIN_H_

#ifndef F_OSC
#define F_OSC 24000000UL
#endif

#ifdef __C51__
#if __C51__ = 750
#include <atmel/regx52.h>
#else
#include <atmel/at89x52.h>
#endif
#else
#include <at89x52.h>
#endif

#if !defined(SDCC) && !defined(__C51__)
#define xdata
#define idata
#define data
#define bit unsigned char
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

#endif /* MAIN_H_ */
