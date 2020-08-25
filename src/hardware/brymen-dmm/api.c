/*
 * This file is part of the libsigrok project.
 *
 * Copyright (C) 2012 Alexandru Gagniuc <mr.nuke.me@gmail.com>
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

static const uint32_t scanopts[] = {
	SR_CONF_CONN,
	SR_CONF_SERIALCOMM,
};

static const uint32_t drvopts[] = {
	SR_CONF_MULTIMETER,
};

static const uint32_t devopts[] = {
	SR_CONF_CONTINUOUS,
	SR_CONF_LIMIT_SAMPLES | SR_CONF_SET,
	SR_CONF_LIMIT_MSEC | SR_CONF_SET,
};

static const int rts_toggle_delay_us = 100000;

static GSList *brymen_scan(struct sr_dev_driver *di, const char *conn,
	const char *serialcomm)
{
	struct sr_dev_inst *sdi;
	struct dev_context *devc;
	struct sr_serial_dev_inst *serial;
	GSList *devices;
	int ret;
	uint8_t buf[128];
	size_t len;

	serial = sr_serial_dev_inst_new(conn, serialcomm);

	if (serial_open(serial, SERIAL_RDWR) != SR_OK)
		return NULL;

	sr_info("Probing port %s.", conn);

	/**
	 * per "Protocol for 500000-count multimeter series" document from wiki
	 * we must toggle RTS on port open to wakeup the meter.
	 * As this was not in the original code, it is presumably only needed
	 * for the newer meters supported by the BC-85Xa interface cable
	 * not the original BC-85X cable used traditionally.
	 */
	/* FIXME - we're not meant to reach directly into libserialport :( */
	g_usleep(rts_toggle_delay_us);
	sp_set_rts(serial->sp_data, SP_RTS_ON);
	g_usleep(rts_toggle_delay_us);
	sp_set_rts(serial->sp_data, SP_RTS_OFF);
	g_usleep(rts_toggle_delay_us);
	sp_set_rts(serial->sp_data, SP_RTS_ON);
	g_usleep(rts_toggle_delay_us);

	devices = NULL;

	/* Request reading */
	if ((ret = brymen_packet_request(serial)) < 0) {
		sr_err("Unable to send command: %d.", ret);
		goto scan_cleanup;
	}

	len = sizeof(buf);
	ret = brymen_stream_detect(serial, buf, &len, brymen_packet_length,
			     brymen_packet_is_valid, 1000, 9600);
	if (ret != SR_OK)
		goto scan_cleanup;

	sr_info("Found device on port %s.", conn);

	sdi = g_malloc0(sizeof(struct sr_dev_inst));
	sdi->status = SR_ST_INACTIVE;
	sdi->vendor = g_strdup("Brymen");
	sdi->model = g_strdup("BM85x");
	devc = g_malloc0(sizeof(struct dev_context));
	sr_sw_limits_init(&devc->sw_limits);
	sdi->inst_type = SR_INST_SERIAL;
	sdi->conn = serial;
	sdi->priv = devc;
	sr_channel_new(sdi, 0, SR_CHANNEL_ANALOG, TRUE, "P1");
	devices = g_slist_append(devices, sdi);

scan_cleanup:
	serial_close(serial);

	return std_scan_complete(di, devices);
}

static GSList *scan(struct sr_dev_driver *di, GSList *options)
{
	struct sr_config *src;
	GSList *devices, *l;
	const char *conn, *serialcomm;

	devices = NULL;

	conn = serialcomm = NULL;
	for (l = options; l; l = l->next) {
		src = l->data;
		switch (src->key) {
		case SR_CONF_CONN:
			conn = g_variant_get_string(src->data, NULL);
			break;
		case SR_CONF_SERIALCOMM:
			serialcomm = g_variant_get_string(src->data, NULL);
			break;
		}
	}
	if (!conn)
		return NULL;

	if (serialcomm)
		devices = brymen_scan(di, conn, serialcomm);
	else
		devices = brymen_scan(di, conn, "9600/8n1/dtr=1/rts=1");

	return devices;
}

static int config_set(uint32_t key, GVariant *data,
	const struct sr_dev_inst *sdi, const struct sr_channel_group *cg)
{
	struct dev_context *devc;

	(void)cg;

	devc = sdi->priv;

	return sr_sw_limits_config_set(&devc->sw_limits, key, data);
}

static int config_list(uint32_t key, GVariant **data,
	const struct sr_dev_inst *sdi, const struct sr_channel_group *cg)
{
	return STD_CONFIG_LIST(key, data, sdi, cg, scanopts, drvopts, devopts);
}

static int dev_acquisition_start(const struct sr_dev_inst *sdi)
{
	struct dev_context *devc;
	struct sr_serial_dev_inst *serial;

	devc = sdi->priv;

	sr_sw_limits_acquisition_start(&devc->sw_limits);
	std_session_send_df_header(sdi);

	serial = sdi->conn;
	serial_source_add(sdi->session, serial, G_IO_IN, 50,
			brymen_dmm_receive_data, (void *)sdi);

	return SR_OK;
}

static struct sr_dev_driver brymen_bm857_driver_info = {
	.name = "brymen-bm857",
	.longname = "Brymen BM857",
	.api_version = 1,
	.init = std_init,
	.cleanup = std_cleanup,
	.scan = scan,
	.dev_list = std_dev_list,
	.dev_clear = std_dev_clear,
	.config_get = NULL,
	.config_set = config_set,
	.config_list = config_list,
	.dev_open = std_serial_dev_open,
	.dev_close = std_serial_dev_close,
	.dev_acquisition_start = dev_acquisition_start,
	.dev_acquisition_stop = std_serial_dev_acquisition_stop,
	.context = NULL,
};
SR_REGISTER_DEV_DRIVER(brymen_bm857_driver_info);
