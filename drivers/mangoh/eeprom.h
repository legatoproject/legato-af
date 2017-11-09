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
#ifndef MANGOH_IOT_EEPROM_H
#define MANGOH_IOT_EEPROM_H

#include <linux/i2c.h>


#define IRQ_GPIO_UNUSED (0xFF)

#define EEPROM_GPIO_CFG_INPUT_PULL_UP	(0x1)
#define EEPROM_GPIO_CFG_INPUT_PULL_DOWN	(0x2)
#define EEPROM_GPIO_CFG_INPUT_FLOATING	(0x3)
#define EEPROM_GPIO_CFG_OUTPUT_LOW	(0x4)
#define EEPROM_GPIO_CFG_OUTPUT_HIGH	(0x5)

struct i2c_client *eeprom_load(int adap_id);
void eeprom_unload(struct i2c_client *eeprom);
struct list_head *eeprom_if_list(struct i2c_client *eeprom);
int eeprom_num_slots(struct i2c_client *eeprom);

#define __DECLARE_IS_IF_PROTOTYPE(bus, ifc)	\
	bool eeprom_is_if_##bus(struct list_head *ifc)
__DECLARE_IS_IF_PROTOTYPE(gpio, ifc);
__DECLARE_IS_IF_PROTOTYPE(i2c, ifc);
__DECLARE_IS_IF_PROTOTYPE(spi, ifc);
__DECLARE_IS_IF_PROTOTYPE(usb, ifc);
__DECLARE_IS_IF_PROTOTYPE(sdio, ifc);
__DECLARE_IS_IF_PROTOTYPE(adc, ifc);
__DECLARE_IS_IF_PROTOTYPE(pcm, ifc);
__DECLARE_IS_IF_PROTOTYPE(clk, ifc);
__DECLARE_IS_IF_PROTOTYPE(uart, ifc);
__DECLARE_IS_IF_PROTOTYPE(plat, ifc);

uint8_t eeprom_if_gpio_cfg(struct list_head *item, unsigned int pin);
char *eeprom_if_spi_modalias(struct list_head *item);
int eeprom_if_spi_irq_gpio(struct list_head *item);
char *eeprom_if_i2c_modalias(struct list_head *item);
int eeprom_if_i2c_irq_gpio(struct list_head *item);
uint8_t eeprom_if_i2c_address(struct list_head *item);
#endif /* MANGOH_IOT_EEPROM_H */

