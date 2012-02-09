/**
 * @date 17.01.2011
 * @author matthias
 * @author nils
 */

#ifndef SPI_H_
#define SPI_H_


#define MOSI (P1_5)
#define MISO (P1_6)
#define SCK  (P1_7)

/* status of SCK when master is inactive */
#define SCK_INACTIVE	1
/*											0      1    2    3 */
/* SPI protocol: every chunk has 4 bytes. COMMAND DATA DATA DATA */
#define OP_WRITE_DATA 0x01		/* (ADDR HIGH), ADDR LOW, DATA */
#define OP_WRITE_XDATA 0x02		/* ADDR HIGH, ADDR LOW, DATA */
#define OP_WRITE_MULTI 0x03		/* ADDR HIGH, ADDR LOW, SIZE (SIZE up to 64 byte)*/
#define OP_MULTI_DATA 0x80		/* 3x DATA; unused data will be discarded */
#define OP_FINISHED 0xF0		/* leave SPI mode */
#define OP_ERROR	0xFF		/* signal error in SPI handling. used as internal command */
#define OP_DISABLE_SDP 0xF1		/* send DISABLE SDP command to EEPROM at 0x8000 */
#define OP_READ_XDATA 0x04		/* read byte of xdata memory: ADDR HIGH, ADDR LOW, DATA(in)*/
#define OP_READ_DATA 0x05		/* read byte of data memory: ADDR HIGH (0), ADDR LOW, DATA(in)*/
#define OP_READ_MULTI 0x08		/* read multiple bytes from xdata memory. ADDR HIGH, LOW, COUNT.
 	 	 	 	 	 	 	 	   gets followed by OP_MULTI_DATA (but read access here)*/
/* OP-CODES that have bit 2 (0x04) set are special cases for read access!
 * do not define another one without changing handling ASM code!
 * OP-CODES that have bit 3 (0x08) set are also special cases!
 * do not define those! */


#endif /* SPI_H_ */
