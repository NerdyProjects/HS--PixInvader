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
#elif !defined(_NO_8051_INCLUDE)
#include <at89x52.h>
#endif

#if !defined(SDCC) && !defined(__C51__)
#define xdata
#define idata
#define data
#define bit unsigned char
#endif

#ifdef _DEBUG
#include <assert.h>
#include <stdio.h>
#define ASSERT(x) assert(x);
#else
#define ASSERT(x)
#endif

#define CHAR_BIT 8

#endif /* MAIN_H_ */
