/**
 * @date 24.11.2011
 * @author matthias
 * @author nils
 */

#ifndef MAIN_H_
#define MAIN_H_

#ifndef F_OSC
/** Oscillator Frequency. */
#define F_OSC 20000000UL
#endif

#ifdef __C51__
#define M1_0 (0x02)
#if __C51__ == 750
#include <atmel/regx52.h>
#else
#include <atmel/at89x52.h>
#endif
#elif !defined(_NO_8051_INCLUDE)
#include <at89x52.h>
#endif

#if defined(SDCC)
#define bit __bit
#define data __data
#define xdata __xdata
#define pdata __pdata
#define code __code
#endif

#if !defined(SDCC) && !defined(__C51__)
#define xdata
#define idata
#define data
#define pdata
#define bit unsigned char
#define code const
#define using(x)
#define reentrant
#endif

#ifdef _DEBUG
#include <assert.h>
#include <stdio.h>
#define ASSERT(x) assert(x);
#else
#define ASSERT(x)
#endif

#define CHAR_BIT 8

#define CNT_SAMPLE_INFO		15
#define SIZE_SAMPLE_INFO	7
#define ADDR_SAMPLE_INFO	(0xFFFF - CNT_SAMPLE_INFO * SIZE_SAMPLE_INFO)
#define CNT_SONG_INFO		4
#define SIZE_SONG_INFO		2
#define ADDR_SONG_INFO		(ADDR_SAMPLE_INFO - CNT_SONG_INFO * SIZE_SONG_INFO)

#define PAGE_SELECT		P2
#define PDATA_PAGE		0


// demo board
/*#define LED		P3_2
#define ADDR_DISPLAY_SELECT 0xC000
#define ADDR_DISPLAY_DATA 0x8000
*/
#define ADDR_SOUND_REG 0x6000

	// Our own board
#define ADDR_DISPLAY_SELECT 0x4000
#define ADDR_DISPLAY_DATA 0x2000
#define LED		P1_0


#define LED_ON()		(LED = 0)
#define LED_OFF()		(LED = 1)
#define LED_TOGGLE()	(LED = ~LED)


#endif /* MAIN_H_ */
