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
#ifndef MANGOH_IOT_EEPROM_1V0_H
#define MANGOH_IOT_EEPROM_1V0_H

typedef struct eeprom_if_reserved_ {
	char reserved[63];
} __attribute__((packed)) eeprom_if_reserved;

typedef struct eeprom_if_gpio_1v0_ {
	uint8_t cfg[4];
	char reserved[59];
} __attribute__((packed)) eeprom_if_gpio_1v0;

typedef struct eeprom_if_i2c_1v0_ {
	uint8_t address;
	uint8_t irq_gpio;
	char modalias[32];
	char reserved[29];
} __attribute__((packed)) eeprom_if_i2c_1v0;

typedef struct eeprom_if_spi_1v0_ {
	uint8_t irq_gpio;
	char modalias[32];
	char reserved[30];
} __attribute__((packed)) eeprom_if_spi_1v0;

typedef eeprom_if_reserved eeprom_if_usb_1v0;
typedef eeprom_if_reserved eeprom_if_sdio_1v0;
typedef eeprom_if_reserved eeprom_if_adc_1v0;
typedef eeprom_if_reserved eeprom_if_pcm_1v0;
typedef eeprom_if_reserved eeprom_if_clk_1v0;
typedef eeprom_if_reserved eeprom_if_uart_1v0;

typedef struct eeprom_if_plat_1v0_ {
	uint8_t irq_gpio;
	char modalias[32];
	char reserved[30];
} __attribute__((packed)) eeprom_if_plat_1v0;

typedef struct eeprom_if_1v0_ {
	uint8_t type;
	union {
		eeprom_if_gpio_1v0 gpio;
		eeprom_if_i2c_1v0 i2c;
		eeprom_if_spi_1v0 spi;
		eeprom_if_sdio_1v0 sdio;
		eeprom_if_usb_1v0 usb;
		eeprom_if_adc_1v0 adc;
		eeprom_if_pcm_1v0 pcm;
		eeprom_if_clk_1v0 clk;
		eeprom_if_uart_1v0 uart;
		eeprom_if_plat_1v0 plat;
	} ifc;
} __attribute__((packed)) eeprom_if_1v0;

#define EEPROM_1V0_INTERFACE_OFFSET	192

#endif /* MANGOH_IOT_EEPROM_1V0_H */
