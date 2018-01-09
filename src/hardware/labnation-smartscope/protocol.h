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

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <glib.h>
#include <libusb.h>
#include <libsigrok/libsigrok.h>
#include "libsigrok-internal.h"

#define LOG_PREFIX "labnation-smartscope"

#define USB_INTERFACE			0

enum PICCMD {
	PICCMD_PIC_VERSION = 1,
	PICCMD_PIC_WRITE = 2,
	PICCMD_PIC_READ = 3,
	PICCMD_PIC_RESET = 4,
	PICCMD_PIC_BOOTLOADER = 5,
	PICCMD_EEPROM_READ = 6,
	PICCMD_EEPROM_WRITE = 7,
	PICCMD_FLASH_ROM_READ = 8,
	PICCMD_FLASH_ROM_WRITE = 9,
	PICCMD_I2C_WRITE = 10,
	PICCMD_I2C_READ = 11,
	PICCMD_PROGRAM_FPGA_START = 12,
	PICCMD_PROGRAM_FPGA_END = 13,
	PICCMD_I2C_WRITE_START = 14,
	PICCMD_I2C_WRITE_BULK = 15,
	PICCMD_I2C_WRITE_STOP = 16,
};


/** Private, per-device-instance driver context. */
struct dev_context {
	/* Model-specific information */
	char hw_rev[4];

	/* Acquisition settings */

	/* Operational state */

	/* Temporary state across callbacks */

};

SR_PRIV bool lnss_version_fpga(const struct sr_dev_inst *sdi, char *dest);
SR_PRIV bool lnss_load_fpga(const struct sr_dev_inst *sdi);

SR_PRIV int labnation_smartscope_receive_data(int fd, int revents, void *cb_data);

