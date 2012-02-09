/**
 * @file keys.c
 * @author matthias
 * @author nils
 * @brief The SPI2EEPROM module.
 * this file will handle EEPROM upgrades through ISP programmer AVRusb
 * ISP Speed will be 375 kHz for HW Clock mode (normal),
 * max. 93,75 kHz for HW Clock mode (AT89S5x compatible AVRusb after sending EnterProgram command)
 * -> typ. ~70 kHz; !!!aware: 8051 mode will convert 0x30->0x28 on first of 4 usb transmit bytes!!!
 * max. 16 kHz for SW Clock mode
 * -> typ. ~12 kHz;
 *
 * As the high speed mode needs special handling that would disallow us to understand slower SPI masters,
 * we concentrate on the slower ones.
 * For 93,75 kHz SPI Clock rate we have 17.78 instructions per bit at 20 MHz
 * USBASP supports max. 4 bytes in one USB frame, so we have a guaranted pause length of 1 ms after 4 bytes.
 *
 */
#include <absacc.h>
#include "main.h"
#include "spi.h"
#include "display.h"


extern unsigned long get4Bytes(void);
extern unsigned long get4Bytes_command(void);


/* get us into SPI receive mode.
 * interrupts will be disabled because we need lowest response times during transfers.
 *
 */
unsigned char handleSPI(void)
{
	unsigned long recv;
	unsigned char b3, b2, b1, b0;

	/* only handle SPI when master is active. USBasp sets all IOs to high impedance on exit */
	while(SCK != SCK_INACTIVE)
	{
		EA = 0;
		displayOff();
		LED_ON();
		recv = get4Bytes_command();	/* directly implements handling of READ commands */
		b3 = recv >> 24;
		b2 = recv >> 16;
		b1 = recv >> 8;
		b0 = recv;
		switch (b0) {
		case OP_WRITE_XDATA: {
			unsigned char xdata *dsta = (unsigned char *) (((unsigned short) b1) << 8 | b2);
			*dsta = b3;
			break;
		}
		case OP_WRITE_DATA: {
			unsigned char data *dstb = (unsigned char *) (b2);
			*dstb = b3;
			break;
		}
		case OP_WRITE_MULTI:
		{
			unsigned char cnt = b3;
			unsigned char i = 0;
			xdata unsigned char pageBuf[66];	/* (64+2)/3*8=66 */
			unsigned char xdata *dst = (unsigned char *) (((unsigned short) b1) << 8 | b2);
			if(cnt > 64)
			{
				return 1;
			}
			for (i = 0; i < cnt; i += 3) {
				recv = get4Bytes();
				b3 = recv >> 24;
				b2 = recv >> 16;
				b1 = recv >> 8;
				b0 = recv;
				if (b0 != OP_MULTI_DATA)
				{
					/* some error occured... better stop writing! */
					return 1;
				}
				pageBuf[i] = b1;
				pageBuf[i + 1] = b2;
				pageBuf[i + 2] = b3;
			}
			for (i = 0; i < cnt; ++i) {
				*(dst++) = pageBuf[i];
			}
			break;
		}
		case OP_READ_DATA:
		case OP_READ_XDATA:
		/* Commands READ_DATA and READ_XDATA are implemented in ASM get4Bytes_command function */
			break;
		case OP_DISABLE_SDP:
			XBYTE[0xD555] = 0xAA;
			XBYTE[0xAAAA] = 0x55;
			XBYTE[0xD555] = 0x80;
			XBYTE[0xD555] = 0xAA;
			XBYTE[0xAAAA] = 0x55;
			XBYTE[0xD555] = 0x20;
			break;
		case OP_FINISHED:
			return 0;
			break;
		case OP_ERROR:
			return 1;
			break;
		}
	}
	return 0;
}
