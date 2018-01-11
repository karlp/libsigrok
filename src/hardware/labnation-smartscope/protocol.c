/*
 * This file is part of the libsigrok project.
 *
 * Copyright (C) 2017 Karl Palsson <karlp@tweak.net.au>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <string.h>
#include <unistd.h>
#include "protocol.h"

#define FPGA_I2C_ADDRESS_ROM  0x0D

#define EP_CMD_IN 0x83
#define EP_CMD_OUT 0x2
#define EP_DATA 0x81

#define HEADER_CMD_BYTE 0xC0 //C0 as in Command
#define HEADER_RESPONSE_BYTE 0xAD // AD as in Answer Dude


static char get_i2c_reg(const struct sr_dev_inst *sdi, char i2caddr, int idx) {
	int len, ret;
	struct sr_usb_dev_inst *usb = sdi->conn;

	uint8_t buf1[] = { HEADER_CMD_BYTE, PICCMD_I2C_WRITE, 2, i2caddr << 1, idx };
	uint8_t buf2[] = { HEADER_CMD_BYTE, PICCMD_I2C_READ, i2caddr, 1 };
	uint8_t inp[16];

	// TODO - len vs expected is probably ideal
	if ((ret = libusb_bulk_transfer(usb->devhdl, EP_CMD_OUT, buf1, sizeof(buf1), &len, 500))) {
		sr_err("Failed to write buf1: %s", libusb_error_name(ret));
		return 0;  // FIXME - really? 
	}
	if ((ret = libusb_bulk_transfer(usb->devhdl, EP_CMD_OUT, buf2, sizeof(buf2), &len, 500))) {
		sr_err("Failed to write buf1: %s", libusb_error_name(ret));
		return 0;  // FIXME - really?
	}
	if ((ret = libusb_bulk_transfer(usb->devhdl, EP_CMD_IN, inp, sizeof(inp), &len, 500))) {
		sr_err("Failed to read i2c response: %s", libusb_error_name(ret));
		return 0;  // FIXME - really?
	}
	return inp[4];
}

//def get_i2c_reg(i2caddr, idx):
//    dev.write(EP_CMD_OUT, [HEADER_CMD_BYTE, PIC_CMDS.I2C_WRITE, 2, i2caddr<<1, idx])
//    dev.write(EP_CMD_OUT, [HEADER_CMD_BYTE, PIC_CMDS.I2C_READ, i2caddr, 1])
//    y = dev.read(EP_CMD_IN, 16)
//    return y[4]


SR_PRIV bool lnss_version_fpga(const struct sr_dev_inst *sdi, char *dest) {
	bool rv = false;
	if (!dest) {
		sr_warn("no destination?!");
		return false;
	}
	for (int i = 0; i < 4; i++) {
		// to match vendor sw, it's in reverse order
		uint8_t x = get_i2c_reg(sdi, FPGA_I2C_ADDRESS_ROM, 3-i);
		sr_dbg("fpga byte %d = %02x", i, x);
		/* if the git rev is _plausible_ it's true */
		if (x != 255) {
			rv = true;
		}
		sprintf(dest + i*2, "%02x", x);
	}
	dest[8] = 0;
	return rv;
}

SR_PRIV bool lnss_load_fpga(const struct sr_dev_inst *sdi) {
	struct sr_usb_dev_inst *usb = sdi->conn;
	struct drv_context *drvc = sdi->driver->context;
	struct dev_context *devc = sdi->priv;
	unsigned i, err;
	char name[200];
	uint8_t *firmware;
	size_t length;

	/* Straight from labnation code. don't ask me what the real plan is */
	int PACKSIZE = 32;
	unsigned PADDING = 2048/8;

	snprintf(name, sizeof(name) - 1, "SmartScope_%s.bin", devc->hw_rev);

	/* All existing blogs are < 300k, don't really expect much change */
	firmware = sr_resource_load(drvc->sr_ctx, SR_RESOURCE_FIRMWARE,
		name, &length, 400 * 1024);
	if (!firmware) {
		sr_err("Failed to load firmware :(");
		return false;
	}

	int cmd = length / PACKSIZE + PADDING;
	uint8_t cmd_start[] = {HEADER_CMD_BYTE, PICCMD_PROGRAM_FPGA_START, cmd >> 8, cmd & 0xff};
	uint8_t cmd_end[] = {HEADER_CMD_BYTE, PICCMD_PROGRAM_FPGA_END};

	sr_info("Uploading firmware '%s'.", name);
	err = libusb_bulk_transfer(usb->devhdl, EP_CMD_OUT, cmd_start, sizeof(cmd_start), NULL, 200);
	if (err != 0) {
		sr_err("Failed to start fpga programming: %s", libusb_error_name(err));
		return false;
	}
	err = libusb_clear_halt(usb->devhdl, EP_DATA);
	if (err != 0) {
		sr_err("Failed to clear halt stage 1: %s", libusb_error_name(err));
		return false;
	}
	sleep(1); // workaround from labnation, not _entirely_ sure it's necessary

	int chunkcnt = 0;
	int actual;
	int actual_sum = 0;
	for (i = 0; i < length; i += 2048) {
		int desired = MIN(2048, length - i);
		err = libusb_bulk_transfer(usb->devhdl, EP_CMD_OUT, firmware+i, desired, &actual, 200);
		if (err != 0) {
			sr_err("Failed to write chunk %d (%d bytes): %s", chunkcnt, desired, libusb_error_name(err));
			return false;
		}
		chunkcnt++;
		sr_dbg("wrote chunk %d for %d bytes, libusb: %d", chunkcnt, actual, err);
		if (actual != desired) {
			sr_warn("Failed to write a chunk %d : %d < 2048", chunkcnt, actual);
		}
		actual_sum += actual;
	}
	sr_dbg("After %d chunks, have written %u, length=%lu", chunkcnt, actual_sum, length);

	/* this seems rather insane, but, hey, it's what the vendor code does... */
	uint8_t data[32];
	memset(data, 0xff, sizeof(data));
	for (i = 0; i < PADDING; i++) {
		err = libusb_bulk_transfer(usb->devhdl, EP_CMD_OUT, data, 32, NULL, 200);
		if (err != 0) {
			sr_err("Failed to write 0xff trailer iteration: %d : %s", i, libusb_error_name(err));
			return false;
		}
	}
	err = libusb_bulk_transfer(usb->devhdl, EP_CMD_OUT, cmd_end, sizeof(cmd_end), NULL, 200);
	if (err != 0) {
		sr_err("Failed to exit fpga programming : %s", libusb_error_name(err));
		return false;
	}
	err = libusb_clear_halt(usb->devhdl, EP_DATA);
	if (err != 0) {
		sr_err("Failed to clear halt stage 2: %s", libusb_error_name(err));
		return false;
	}
	return true;
}

SR_PRIV int labnation_smartscope_receive_data(int fd, int revents, void *cb_data)
{
	const struct sr_dev_inst *sdi;
	struct dev_context *devc;

	(void)fd;

	if (!(sdi = cb_data))
		return TRUE;

	if (!(devc = sdi->priv))
		return TRUE;

	if (revents == G_IO_IN) {
		/* TODO */
	}

	return TRUE;
}
