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
#include "usbasp.h"

#include <libusb-1.0/libusb.h>

libusb_context *ctx;
libusb_device_handle *usbhandle;

const char *progname = "usbasp-spi";

/*											0      1    2    3 */
/* SPI protocol: every chunk has 4 bytes. COMMAND DATA DATA DATA */
#define OP_WRITE_BYTE 0x01		/* ADDR HIGH, ADDR LOW, DATA */
#define OP_WRITE_MULTI 0x02		/* ADDR HIGH, ADDR LOW, SIZE (SIZE up to 64 byte)*/
#define OP_MULTI_DATA 0x80		/* 3x DATA; unused data will be discarded */
#define OP_FINISHED 0xF0		/* leave SPI mode */

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

static void usbasp_close(void) {
	if (usbhandle != NULL) {
		unsigned char temp[4];
		memset(temp, 0, sizeof(temp));

		usbasp_transmit(1, USBASP_FUNC_DISCONNECT, temp, temp, sizeof(temp));

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
	int j;
	unsigned char bit = 0;
	usbasp_spi_cmd(data, res);

	for(i = 0; i < 4; ++i)
	{
		for(j = 0; j < 8; ++j)
		{
			if(((res[i] & (1 << j)) == 0) != (bit == 0))
			{
				fprintf(stderr, "spi receive: did not read expected changing "
						"bit values. 0x%2X%2X%2X%2X (at %d %d)\n", res[0],
						res[1], res[2], res[3], i, j);
				bit = !bit;
				return -1;
			}
		}
	}
	return 0;
}

/**
 * leave SPI mode on target.
 */
void spi_exit()
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
 */
void spi_write_byte(unsigned short address, unsigned char byte)
{
	char cmd[4];

	cmd[0] = OP_WRITE_BYTE;
	cmd[1] = address >> 8;
	cmd[2] = address;
	cmd[3] = byte;

	spi_write(cmd);
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
			spi_exit();
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

int main(int argc, char **argv) {
	FILE *img;
	int offset;
	int rc;
	int i;
	unsigned char *imgData;

	if(argc != 3)
	{
		printf("Usage: program <binary file> <offset: byte 0 in file is byte X on target>\n");
		return 1;
	}

	if(usbasp_open() != 0)
	{
		return 2;
	}

	sscanf(argv[2], "%d", &offset);

	if(strlen(argv[1]) == 0 || offset > 0xFFFF || offset < 0)
	{
		fprintf(stderr, "invalid parameters given: %s %d\n", argv[1], offset);
		return 3;
	}

	img = fopen(argv[1], "rb");
	if(img == 0)
	{
		fprintf(stderr, "could not open input file!\n");
		return 4;
	}
	printf("Going to send CONNECT command. Your device will reset if ISP reset is connected!\n");
	usbasp_func_connect();

	printf("USB connected!\n"
		   "Sending ENTER PROGRAM MODE command to fall through to S5x mode (SPI CLK ~90 kHz for fast mode).\n");

	rc = usbasp_spi_program_enable();
	if(rc == 0)
	{
		fprintf(stderr, "Error: Seems that entering programming mode was successful. That is not good!\n"
						"Disconnect reset line or use an S5x mode unaware USBasp in slow mode!\n");
		return 5;
	}

	imgData = malloc(MAX_IMAGE_SIZE * sizeof(char));
	rc = fread(imgData, 1, MAX_IMAGE_SIZE, img);
	printf("read %d bytes of image file!\n", rc);

	write_image(imgData, offset, rc);

	return 0;
}
