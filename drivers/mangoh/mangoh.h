/* Copyright (c) 2009-2012, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef MANGOH_IOT_MANGOH_H
#define MANGOH_IOT_MANGOH_H

#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/spi/spi.h>

typedef enum {
	mangoh_bus_i2c,
	mangoh_bus_spi,
	mangoh_bus_uart,
	mangoh_bus_sdio,
	mangoh_bus_usb,
	mangoh_bus_gpio,
	mangoh_bus_pcm,
	mangoh_bus_adc,
	mangoh_bus_last,
} mangoh_bus_t;

struct mangoh_desc {
	char *type;
	int (*map)(struct platform_device *pdev);
	int (*unmap)(struct platform_device *pdev);
};

extern struct mangoh_desc mangoh_green_desc;
extern struct platform_device mangoh_green;

#endif /* MANGOH_IOT_MANGOH_H */

