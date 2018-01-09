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
	struct dev_context *devc;
	struct sr_usb_dev_inst *usb;
	int err;

	devc = sdi->priv;
	usb = sdi->conn;

	sr_dbg("attempting to load fpga");

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
