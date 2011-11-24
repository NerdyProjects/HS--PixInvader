/*
 * sound.c
 *
 *  Created on: 24.11.2011
 *      Author: matthias
 */


#include "main.h"
#include "sound.h"

#if defined(__C51__)
/* Keil declaration */
static xdata volatile unsigned char SoundReg _at_ 0x6000;
//#define M1_0 (T0_M1_)
#define M1_0 (0x02)

#elif defined(SDCC)
/* sdcc declaration */
static xdata volatile __at (0x6000) unsigned char SoundReg ;
#else
/* befriend other compilers */
static unsigned char SoundReg;
#endif


/* IRQ rate: F_OSC / 12 / (256 - RELOAD) */
#define TIMER0_RELOAD 0		/* 7812,5 Hz */

#ifdef __C51__
data unsigned char xdata *AudioStream1;
data unsigned char xdata *AudioStream2;
data unsigned char xdata *AudioStream3;
data unsigned char xdata *AudioStream4;
data unsigned char xdata *AudioStreamEnd1;
data unsigned char xdata *AudioStreamEnd2;
data unsigned char xdata *AudioStreamEnd3;
data unsigned char xdata *AudioStreamEnd4;
#else
xdata unsigned char * data AudioStream1;
xdata unsigned char * data AudioStream2;
xdata unsigned char * data AudioStream3;
xdata unsigned char * data AudioStream4;
xdata unsigned char * data AudioStreamEnd1;
xdata unsigned char * data AudioStreamEnd2;
xdata unsigned char * data AudioStreamEnd3;
xdata unsigned char * data AudioStreamEnd4;
#endif


/* timer 0 ISR.
 * this ISR serves the sound timer.
 * Call frequency will be F_OSC / 12 / (256 - TH0).
 * F_OSC of 24 MHz leads to 7812 Hz .
 * total execution time must not exceed timer rate to prevent audio jitter!
 * */
#ifdef SDCC
void timer0_isr(void) __interrupt (1) __using (1)
#elif defined(__C51__)
void timer0_isr(void) interrupt 1 using 1
#else
void timer0_isr(void)
#endif
{
	static bit highNibble = 0;

	unsigned char audioOut = 0;
	unsigned char audioTemp;

    if(AudioStream1 != AudioStreamEnd1)
    {
        audioTemp = *AudioStream1;
        if(highNibble)
        {
            AudioStream1++;
            audioTemp >>= 4;
        }
        audioOut += audioTemp & 0x0F;
    }

    if(AudioStream2 != AudioStreamEnd2)
    {
        audioTemp = *AudioStream2;
        if(highNibble)
        {
            AudioStream2++;
            audioTemp >>= 4;
        }
        audioOut += audioTemp & 0x0F;
    }

    if(AudioStream3 != AudioStreamEnd3)
    {
        audioTemp = *AudioStream3;
        if(highNibble)
        {
            AudioStream3++;
            audioTemp >>= 4;
        }
        audioOut += audioTemp & 0x0F;
    }

    if(AudioStream4 != AudioStreamEnd4)
    {
        audioTemp = *AudioStream4;
        if(highNibble)
        {
            AudioStream4++;
            audioTemp >>= 4;
        }
        audioOut += audioTemp & 0x0F;
    }

	highNibble = ~highNibble;
	SoundReg = audioOut;
}

void soundInit(void)
{
	TMOD = M1_0;	/* 8 bit timer */
	TH0 = TIMER0_RELOAD;
	TR0 = 1;
	ET0 = 1;
	PT0 = 1;		/* high priority IRQ */
}
