/*
 * spi.h
 *
 *  Created on: 17.01.2012
 *      Author: matthias
 */

#ifndef SPI_H_
#define SPI_H_


#define MOSI (P1_5)
#define SCK  (P1_7)
/*											0      1    2    3 */
/* SPI protocol: every chunk has 4 bytes. COMMAND DATA DATA DATA */
#define OP_WRITE_BYTE 0x01		/* ADDR HIGH, ADDR LOW, DATA */
#define OP_WRITE_MULTI 0x02		/* ADDR HIGH, ADDR LOW, SIZE (SIZE up to 64 byte)*/
#define OP_MULTI_DATA 0x80		/* 3x DATA; unused data will be discarded */
#define OP_FINISHED 0xF0		/* leave SPI mode */


#endif /* SPI_H_ */
