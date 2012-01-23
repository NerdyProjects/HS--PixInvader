/*
 * USBasp-spi: Use spi commands of an USBasp to communicate with device
 * over the ISP.
 *
 * USBasp code is taken from avrdude-5.11
 *
 * 2012 by Matthias Larisch
 * This code is crap, do not use it!
 */

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <assert.h>
#include <unistd.h>
#include "usbasp.h"

#include <libusb-1.0/libusb.h>

libusb_context *ctx;
libusb_device_handle *usbhandle;

const char *progname = "usbasp-spi";

/*											0      1    2    3 */
/* SPI protocol: every chunk has 4 bytes. COMMAND DATA DATA DATA */
#define OP_WRITE_DATA 0x01		/* (ADDR HIGH), ADDR LOW, DATA */
#define OP_WRITE_XDATA 0x02		/* ADDR HIGH, ADDR LOW, DATA */
#define OP_WRITE_MULTI 0x03		/* ADDR HIGH, ADDR LOW, SIZE (SIZE up to 64 byte)*/
#define OP_MULTI_DATA 0x80		/* 3x DATA; unused data will be discarded */
#define OP_FINISHED 0xF0		/* leave SPI mode */
#define OP_READ_XDATA 0x04		/* read byte of xdata memory: ADDR HIGH, ADDR LOW, DATA(in)*/
#define OP_READ_DATA 0x05		/* read byte of data memory: ADDR HIGH (0), ADDR LOW, DATA(in)*/

#define DEFAULT_RESULT 0x55		/* every undefined byte we receive should be this one */

#define MAX_IMAGE_SIZE	0xFFFF
#define BLOCK_ALIGN		64		/* EEPROM page size */
#define PAGE_WRITE_WAIT	15		/* wait time in ms to wait after each page write */

static int usbasp_transmit(unsigned char receive, unsigned char functionid,
		unsigned char send[4], unsigned char * buffer, int buffersize);
static int usbOpenDevice(libusb_device_handle **device, int vendor,
		char *vendorName, int product, char *productName);
// interface - prog.
static int usbasp_open(void);
static void usbasp_close(void);
// SPI specific functions
static int usbasp_spi_cmd(unsigned char cmd[4], unsigned char res[4]);
static int usbasp_spi_program_enable(void);

/* Internal functions */
/*
 * wrapper for usb_control_msg call
 */
static int usbasp_transmit(unsigned char receive, unsigned char functionid,
		unsigned char send[4], unsigned char * buffer, int buffersize) {
	int nbytes;
	nbytes = libusb_control_transfer(
			usbhandle,
			(LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE
					| (receive << 7)) & 0xff, functionid & 0xff,
			((send[1] << 8) | send[0]) & 0xffff,
			((send[3] << 8) | send[2]) & 0xffff, (char *) buffer,
			buffersize & 0xffff, 5000);
	if (nbytes < 0) {
		fprintf(stderr, "error: usbasp_transmit: %d\n", progname, nbytes);
		return -1;
	}
	return nbytes;
}

/*
 * Try to open USB device with given VID, PID, vendor and product name
 * Parts of this function were taken from an example code by OBJECTIVE
 * DEVELOPMENT Software GmbH (www.obdev.at) to meet conditions for
 * shared VID/PID
 */
static int usbOpenDevice(libusb_device_handle **device, int vendor,
		char *vendorName, int product, char *productName) {
	libusb_device_handle *handle = NULL;
	int errorCode = USB_ERROR_NOTFOUND;
	static int didUsbInit = 0;
	int j;
	int r;

	if (!didUsbInit) {
		didUsbInit = 1;
		libusb_init(&ctx);
	}

	libusb_device **dev_list;
	int dev_list_len = libusb_get_device_list(ctx, &dev_list);

	for (j = 0; j < dev_list_len; ++j) {
		libusb_device *dev = dev_list[j];
		struct libusb_device_descriptor descriptor;
		libusb_get_device_descriptor(dev, &descriptor);
		if (descriptor.idVendor == vendor && descriptor.idProduct == product) {
			char string[256];
			/* we need to open the device in order to query strings */
			r = libusb_open(dev, &handle);
			if (!handle) {
				errorCode = USB_ERROR_ACCESS;
				fprintf(stderr, "%s: Warning: cannot open USB device: %d\n",
						progname, r);
				continue;
			}
			if (vendorName == NULL && productName == NULL) {
				/* name does not matter */
				break;
			}
			/* now check whether the names match: */
			r = libusb_get_string_descriptor_ascii(handle,
					descriptor.iManufacturer & 0xff, string, sizeof(string));
			if (r < 0) {
				errorCode = USB_ERROR_IO;
				fprintf(
						stderr,
						"%s: Warning: cannot query manufacturer for device: %d\n",
						progname, r);
			} else {
				errorCode = USB_ERROR_NOTFOUND;
				if (strcmp(string, vendorName) == 0) {
					r = libusb_get_string_descriptor_ascii(handle,
							descriptor.iProduct & 0xff, string, sizeof(string));
					if (r < 0) {
						errorCode = USB_ERROR_IO;
						fprintf(
								stderr,
								"%s: Warning: cannot query product for device: %d\n",
								progname, r);
					} else {
						errorCode = USB_ERROR_NOTFOUND;
						if (strcmp(string, productName) == 0)
							break;
					}
				}
			}
			libusb_close(handle);
			handle = NULL;
		}
	}
	if (handle != NULL) {
		errorCode = 0;
		*device = handle;
	}
	return errorCode;
}

/* Interface - prog. */
static int usbasp_open(void) {
	libusb_init(&ctx);
	if (usbOpenDevice(&usbhandle, USBASP_SHARED_VID, "www.fischl.de",
			USBASP_SHARED_PID, "USBasp") != 0) {

		/* check if device with old VID/PID is available */
		if (usbOpenDevice(&usbhandle, USBASP_OLD_VID, "www.fischl.de",
				USBASP_OLD_PID, "USBasp") != 0) {

			/* no USBasp found */
			fprintf(stderr, "%s: error: could not find USB device "
					"\"USBasp\" with vid=0x%x pid=0x%x\n", progname,
					USBASP_SHARED_VID, USBASP_SHARED_PID);
			return -1;

		} else {

			/* found USBasp with old IDs */
			fprintf(stderr, "%s: Warning: Found USB device \"USBasp\" with "
					"old VID/PID! Please update firmware of USBasp!\n",
					progname);
		}
	}

	return 0;
}

static void usbasp_func_disconnect(void)
{
	unsigned char temp[4];
	usbasp_transmit(1, USBASP_FUNC_DISCONNECT, temp, temp, sizeof(temp));
}

static void usbasp_close(void) {
	if (usbhandle != NULL) {
		usbasp_func_disconnect();

		libusb_close(usbhandle);
	}
}

static int usbasp_spi_cmd(unsigned char cmd[4], unsigned char res[4]) {
	int nbytes = usbasp_transmit(1, USBASP_FUNC_TRANSMIT, cmd, res,
			sizeof(res));

	if (nbytes != 4) {
		fprintf(stderr, "%s: error: wrong responds size\n", progname);
		return -1;
	}

	return 0;
}

static int usbasp_func_connect() {
	unsigned char res[4];
	unsigned char cmd[4];

	int nbytes = usbasp_transmit(1, USBASP_FUNC_CONNECT, cmd, res,
			sizeof(res));

	return 0;
}


static int usbasp_spi_program_enable() {
	unsigned char res[4];
	unsigned char cmd[4];
	memset(cmd, 0, sizeof(cmd));
	memset(res, 0, sizeof(res));

	cmd[0] = 0;

	int nbytes = usbasp_transmit(1, USBASP_FUNC_ENABLEPROG, cmd, res,
			sizeof(res));

	if ((nbytes != 1) | (res[0] != 0)) {
		fprintf(stderr,
				"%s: error: programm enable: target doesn't answer. %x \n",
				progname, res[0]);
		return -1;
	}

	return 0;
}

int spi_write(char data[4])
{
	char res[4];
	int i;
	usbasp_spi_cmd(data, res);

	for(i = 0; i < 4; ++i)
	{
		if(res[i] != DEFAULT_RESULT)
		{
			fprintf(stderr, "spi receive: did not read expected changing "
					"bit values. 0x%2X%2X%2X%2X (at %d)\n", res[0],
					res[1], res[2], res[3], i);
			return -1;
		}
	}
	return 0;
}


/**
 * leave SPI mode on target.
 */
void cmd_exit()
{
	char cmd[4];
	char res[4];
	cmd[0] = OP_FINISHED;
	cmd[1] = OP_FINISHED;
	cmd[2] = OP_FINISHED;
	cmd[3] = OP_FINISHED;
	spi_write(cmd);
}

/**
 * write byte to an address on target.
 * @param address 2 byte target address (1 byte for data mem)
 * @param byte data byte
 * @param xdata 0 for data, 1 for xdata as target memory
 */
void cmd_write_byte(unsigned short address, unsigned char byte, int xdata)
{
	char cmd[4];

	cmd[0] = xdata ? OP_WRITE_XDATA : OP_WRITE_DATA;
	cmd[1] = address >> 8;
	cmd[2] = address;
	cmd[3] = byte;

	spi_write(cmd);
}


/**
 * reads byte from an address on target.
 * @param address 2 byte target address (1 byte for data mem)
 * @param xdata 0 for data, 1 for xdata as target memory
 * @return data byte
 */
unsigned char cmd_read_byte(unsigned short address, int xdata)
{
	char cmd[4];
	char res[4];
	int i;

	cmd[0] = xdata ? OP_READ_XDATA : OP_READ_DATA;
	cmd[1] = address >> 8;
	cmd[2] = address;
	cmd[3] = 0;

	usbasp_spi_cmd(cmd, res);
	
	for(i = 0; i < 3; ++i)
	{
		if(res[i] != DEFAULT_RESULT)
		{
			fprintf(stderr, "spi readByte: did not read expected changing "
					"bit values. 0x%2X%2X%2X%2X (at %d)\n", res[0],
					res[1], res[2], res[3], i);
			return -1;
		}
	}

	return res[3];
}

/**
 * write more bytes to an address range on target.
 * we may read up to two bytes beyond given data array -_-
 */
void spi_write_block(unsigned short address, unsigned char *data, int len)
{
	char cmd[4];
	int i;

	assert(len <= 64);
	cmd[0] = OP_WRITE_MULTI;
	cmd[1] = address >> 8;
	cmd[2] = address;
	cmd[3] = len;
	if(spi_write(cmd) != 0)
	{
		return;
	}
	cmd[0] = OP_MULTI_DATA;
	for(i = 0; i < len; i+=3)
	{
		cmd[1] = data[i];
		cmd[2] = data[i+1];
		cmd[3] = data[i+2];
		if(spi_write(cmd) != 0)
		{
			cmd_exit();
			return;
		}
	}
}

/**
 * writes data to target. begins target addressing at offset and sends
 * len bytes.
 */
void write_image(unsigned char *data, int offset, int len)
{
	int i;
	int blocksize = 64;
	while(i < len)
	{
		blocksize = 64;
		if(len - i < blocksize)		/* send no more than we want */
			blocksize = len - i;

		/* do not cross given page boundary with this write */
		if((BLOCK_ALIGN - ((offset + i) % BLOCK_ALIGN)) < blocksize)
			blocksize = BLOCK_ALIGN - ((offset + i) % BLOCK_ALIGN);

		spi_write_block(offset + i, &data[i], blocksize);
		printf("sent %5d/%5d bytes (bs %d)\n", i, len, blocksize);
		usleep(PAGE_WRITE_WAIT * 1000);
		i += blocksize;
	}

}

void writeRomfile(char *filename, int offset) {
	FILE *img;
	unsigned char *imgData;
	int rc;
	img = fopen(filename, "rb");
	if (img == 0) {
		fprintf(stderr, "could not open input file!\n");
		return;
	}

	imgData = malloc(MAX_IMAGE_SIZE * sizeof(char));
	rc = fread(imgData, 1, MAX_IMAGE_SIZE, img);
	printf("read %d bytes of image file!\n", rc);

	write_image(imgData, offset, rc);
}

static int readAddressData(char *buf, int *address, int *data, int dataNeeded)
{
	int rc;
	rc = sscanf(buf, " 0x%x 0x%x", address, data);
	if((dataNeeded && rc != 2) || (!dataNeeded && rc != 1))
	{
		printf("got wrong number of arguments for that command!(n %d, %d, %x, %x)\n: %s", dataNeeded, rc, address, data, buf);
		return -1;
	}
	return 0;
}

void terminalMode(void) {
	int done = 0;
	char terminalBuf[128];
	int address, data;
	printf("Terminal mode started. Type h<enter> for help.\n");
	while (!done) {
		printf("> ");
		fgets(terminalBuf, 128, stdin);
		switch (terminalBuf[0]) {
		case 'h':
			printf("Terminal Mode:\n");
			printf(
					"enter a one letter command followed by its parameters and press enter.\n");
			printf("commands:\n");
			printf(
					"x address: read from xdata address. Address is specified in hexadecimal: 0x50\n");
			printf(
					"X address data: write data to xdata address. Address and data are hexadecimal: 0x50 0x00\n");
			printf("d address: read from data address.\n");
			printf("D address data: write data to data address.\n");
			printf("o: enable SPI code by setting SCK idle. (resets target)\n");
			printf("O: disable SPI code by setting SCK high impedance.\n");
			printf("c: continue program operation by exiting SPI code once.\n");
			printf("h: this help\n");
			printf("q: quit (continue, disable, quit)\n\n");
			break;
		case 'x':	/* read xdata address */
			if (readAddressData(terminalBuf + 1, &address, &data, 0) == 0) {
				data = cmd_read_byte(address, 1);
				printf("rXDATA: 0x%4X: %X (%d)\n", address, data, data);
			}
			break;
		case 'X':	/* write xdata address data */
			if (readAddressData(terminalBuf + 1, &address, &data, 1) == 0) {
				cmd_write_byte(address, data, 1);
				printf("wXDATA: 0x%4X: %X (%d)\n", address, data, data);
			}
			break;
		case 'd':	/* read data address */
			if (readAddressData(terminalBuf + 1, &address, &data, 0) == 0) {
				data = cmd_read_byte(address, 0);
				printf("r DATA: 0x%4X: %X (%d)\n", address, data, data);
			}
			break;
		case 'D':	/* write data address data */
			if (readAddressData(terminalBuf + 1, &address, &data, 1) == 0) {
				cmd_write_byte(address, data, 0);
				printf("w DATA: 0x%4X: %X (%d)\n", address, data, data);
			}
			break;
		case 'o':
			usbasp_func_connect();
			break;
		case 'O':
			usbasp_func_disconnect();
			break;
		case 'c':
			cmd_exit();
			break;
		case 'q':
			cmd_exit();
			usbasp_func_disconnect();
			return;
			break;
		default:
			printf("unknown command!\n");
			break;
		}

	}
}



int main(int argc, char **argv) {
	FILE *img;
	int offset = 0;
	int rc;
	unsigned char *imgData;
	char *romfile = NULL;
	int terminal = 0;
	int mode5X = 1;

	while (optind < argc) {
		int rc;

		rc = getopt(argc, argv, "htb:o:X");

		switch (rc) {
		case -1:
		case '?':
		case 'h':
			printf("Usage: program [-h] [-b <binary file>] [-o <offset: byte 0"
					" in file is byte X on target>] [-t]\n");
			printf("\t -b: copy binary file to target (offset is 0 or "
					"specified with -o)\n");
			printf("\t -o: set target byte of byte 0 of binary file\n");
			printf("\t -t: start terminal mode\n");
			printf("\t -X: do not AT89S5X mode of USBasp. This requires "
					"manual interaction: You have to powercycle the USBasp "
					"when it already entered 89S5X mode (by programming or "
					"previous calls to this program without -X)\n\t"
					"ensure to have the LOW SPEED jumper closed!\n");
			return 1;
			break;
		case 't':
			terminal = 1;
			break;
		case 'b':
			romfile = optarg;
			break;
		case 'o':
			sscanf(optarg, "%d", &offset);
			break;
		case 'X':
			mode5X = 0;
		}

	}

	if (romfile) {
		if (strlen(romfile) == 0 || offset > 0xFFFF || offset < 0) {
			fprintf(stderr, "invalid parameters given for romfile or offset\n");
			return 3;
		}
	}

	if (usbasp_open() != 0) {
		return 2;
	}

	printf("Going to send CONNECT command. Your device will reset if ISP"
			"reset is connected!\n");
	usbasp_func_connect();

	printf("USB connected!\n");

	if(mode5X) {
		printf("Sending ENTER PROGRAM MODE command to fall through to S5x mode"
			   "(SPI CLK ~90 kHz for fast mode).\n");

		rc = usbasp_spi_program_enable();
		if (rc == 0) {
			fprintf(stderr, "Error: Seems that entering programming mode was"
					" successful. That is not good!\n"
					"Disconnect reset line or use an S5x mode unaware USBasp in"
					" slow mode!\n");
			usbasp_close();
			return 5;
		}
	}

	if (romfile) {
		writeRomfile(romfile, offset);
	}

	if(terminal) {
		terminalMode();
	}

	usbasp_close();
	return 0;
}
