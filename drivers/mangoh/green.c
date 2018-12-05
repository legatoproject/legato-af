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
/*
 * mangOH green platform has a pca9548 I2C switch with three sx150 GPIO
 * expanders. GPIOs 11, 13 and 12 on the second GPIO expander are card
 * detect signals for slots 1, 2 and 3, respectively. We need to probe
 * those GPIO signals to determine which cards are present. GPIOs
 * 10-15 on the first expander, as well as GPIO 8 on the third expander
 * control the SDIO, SPI and UART switches on the board.
 *
 * Once an IoT card is detected, we read the at24 EEPROM on the card
 * to determine type of card and add the appropriate device type. This
 * should also insmod/launch an appropriate card driver. Some bus types
 * (e.g. USB or SDIO) have the auto-detect capability so the device
 * gets added once the board is taken out of reset and its switch
 * configured.
 *
 * Assuming that the first GPIO line on the card is always used for
 * interrupts, we can hard-code each card's IRQ and leave it to the
 * driver to request it.
 */

#include <linux/version.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/i2c/pca954x.h>
#include <linux/i2c/sx150x.h>
#include <linux/gpio.h>
#include <linux/gpio/driver.h>
#include <linux/spi/spi.h>

#include "mangoh.h"
#include "eeprom.h"

static struct sx150x_platform_data sx150x_data[] = {
	[0] = {
		.gpio_base         = -1,
		.oscio_is_gpo      = false,
		.io_pullup_ena     = 0,
		.io_pulldn_ena     = 0,
		.io_open_drain_ena = 0,
		.io_polarity       = 0,
		.irq_summary       = -1,
		.irq_base          = -1,
	},
	[1] = {
		.gpio_base         = -1,
		.oscio_is_gpo      = false,
		.io_pullup_ena     = 0x3800, /* pulled low when card present */
		.io_pulldn_ena     = 0,
		.io_open_drain_ena = 0,
		.io_polarity       = 0,
		.irq_summary       = -1,
		.irq_base          = -1,
	},
	[2] = {
		.gpio_base         = -1,
		.oscio_is_gpo      = false,
		.io_pullup_ena     = 0,
		.io_pulldn_ena     = 0,
		.io_open_drain_ena = 0,
		.io_polarity       = 0,
		.irq_summary       = -1,
		.irq_base          = -1,
	},
};

static struct i2c_board_info sx150x_devinfo[] = {
	[0] = {
		I2C_BOARD_INFO("sx1509q", 0x3e),
		.platform_data = &(sx150x_data[0]),
		.irq = 0,
	},
	[1] = {
		I2C_BOARD_INFO("sx1509q", 0x3f),
		.platform_data = &(sx150x_data[1]),
		.irq = 0,
	},
	[2] = {
		I2C_BOARD_INFO("sx1509q", 0x70),
		.platform_data = &(sx150x_data[2]),
		.irq = 0,
	},
};


enum wp_model {
	wp_model_wp85,
	wp_model_wp76,
	wp_model_last,
};
static struct pca954x_platform_mode pca954x_adap_modes[][8] = {
	[wp_model_wp85] = {
		{1, 1, 0}, {2, 1, 0}, {3, 1, 0}, {4, 1, 0},
		{5, 1, 0}, {6, 1, 0}, {7, 1, 0}, {8, 1, 0}
	},
	[wp_model_wp76] = {
		{5, 1, 0}, {6, 1, 0}, {7, 1, 0}, {8, 1, 0},
		{9, 1, 0}, {10, 1, 0}, {11, 1, 0}, {12, 1, 0}
	},
};

static struct pca954x_platform_data pca954x_pdata[wp_model_last] = {
	[wp_model_wp85] = {
		pca954x_adap_modes[wp_model_wp85],
		ARRAY_SIZE(pca954x_adap_modes[wp_model_wp85]),
	},
	[wp_model_wp76] = {
		pca954x_adap_modes[wp_model_wp76],
		ARRAY_SIZE(pca954x_adap_modes[wp_model_wp76]),
	},
};

static struct i2c_board_info pca954x_device_info[wp_model_last] = {
	[wp_model_wp85] = {
		I2C_BOARD_INFO("pca9548", 0x71),
		.platform_data = &(pca954x_pdata[wp_model_wp85]),
		.irq = 0,
	},
	[wp_model_wp76] = {
		I2C_BOARD_INFO("pca9548", 0x71),
		.platform_data = &(pca954x_pdata[wp_model_wp76]),
		.irq = 0,
	},
};

static struct green_platform_data {
	char *wp_name;
	int i2c_mux_bus;
	int spi_bus;
	struct i2c_client *mux;
	struct i2c_client *expander[ARRAY_SIZE(sx150x_data)];
	struct slot_status {
		/*
		 * Assigned GPIOs on IoT connector. Index 0-3 cover the
		 * dedicated GPIO pins of the IoT card interface. Index 5 and 6
		 * are for the card detect and card reset functions.
		 */
		int	gpio[6];
		bool	present;			/* card present */
		void	*busdev[mangoh_bus_last];	/* IoT bus devices */
	} slot[3];
	int sdio_sel_gpio;
	int spi_sw_en_gpio;
	int spi_sw_gpio;
	int uart0_sw_en_gpio;
	int uart0_sw_gpio;
	int uart1_sw_en_gpio;
	int uart1_sw_gpio;
} green_pdata [] = {
	[wp_model_wp85] = {
		"WP85",
		0,		/* I2C mux bus */
		0,		/* SPI bus */
		NULL,		/* Mux */
		{		/* Expanders */
			NULL
		},
		{	/*
			 * Slots: GPIO #s are relative to GPIO base on its chip
			 *		GPIOs		present	busdev
			 */
			{{80, 78, 84, 29, 11, 4},	false,	{NULL}},
			{{73, 79, 30, 50, 13, 3},	false,	{NULL}},
			{{49, 54, 61, 92, 12, 2},	false,	{NULL}},
		},
		13, 14, 15, 10, 11, 8, 12,
	},
	[wp_model_wp76] = {
		"WP76",
		4,		/* I2C mux bus */
		1,		/* SPI bus */
		NULL,		/* Mux */
		{		/* Expanders */
			NULL
		},
		{	/*
			 * Slots: GPIO #s are relative to GPIO base on its chip
			 *		GPIOs		present	busdev
			 */
			{{79, 78, 76, 58, 11, 4},	false,	{NULL}},
			{{17, 16, 77,  8, 13, 3},	false,	{NULL}},
			{{ 9, 10, 11, 54, 12, 2},	false,	{NULL}},
		},
		13, 14, 15, 10, 11, 8, 12,
	},
};

#define det_gpio	gpio[4] /* slot detect */
#define rst_gpio	gpio[5] /* slot reset */

#define i2cdev		busdev[mangoh_bus_i2c]
#define spidev		busdev[mangoh_bus_spi]
#define uartdev		busdev[mangoh_bus_uart]
#define sdiodev		busdev[mangoh_bus_sdio]
#define usbdev		busdev[mangoh_bus_usb]
#define gpiodev		busdev[mangoh_bus_gpio]
#define pcmdev		busdev[mangoh_bus_pcm]
#define adcdev		busdev[mangoh_bus_adc]


/* Helper functions for accessing mangOH green slots */
static inline int green_num_slots(struct platform_device *pdev)
{
	struct green_platform_data *pdata = dev_get_platdata(&pdev->dev);
	return ARRAY_SIZE(pdata->slot);
}

static inline struct slot_status *green_get_slot(struct platform_device *pdev,
						 int slot)
{
	struct green_platform_data *pdata = dev_get_platdata(&pdev->dev);
	return &(pdata->slot[slot]);
}

static inline int green_get_i2c_bus(struct platform_device *pdev, int slot)
{
	struct green_platform_data *pdata = dev_get_platdata(&pdev->dev);
	return pdata->i2c_mux_bus + 1 + slot;
}

static inline int green_get_spi_bus(struct platform_device *pdev, int slot)
{
	struct green_platform_data *pdata = dev_get_platdata(&pdev->dev);
	return pdata->spi_bus;
}

/*
 * Unfortunately gpio_pull_up()/_down() have different prototypes in
 * kernels 3.14 and 3.18.
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 18, 0)
#define GPIO_REFERENCE(i) (i)
#else
#define GPIO_REFERENCE(i) gpio_to_desc((i))
#endif

static struct device *green_add_gpio(struct platform_device *pdev,
				     int slot,
				     struct list_head *item)
{
	int i;
	struct slot_status *s = green_get_slot(pdev, slot);
	for (i = 0; i < 4; i++) {
		uint8_t cfg = eeprom_if_gpio_cfg(item, i);
		switch (cfg)
		{
		case EEPROM_GPIO_CFG_OUTPUT_LOW:
			gpio_direction_output(s->gpio[i], 0);
			break;
		case EEPROM_GPIO_CFG_OUTPUT_HIGH:
			gpio_direction_output(s->gpio[i], 1);
			break;
		case EEPROM_GPIO_CFG_INPUT_PULL_UP:
			gpio_direction_input(s->gpio[i]);
			gpio_pull_up(GPIO_REFERENCE(s->gpio[i]));
			break;
		case EEPROM_GPIO_CFG_INPUT_PULL_DOWN:
			gpio_direction_input(s->gpio[i]);
			gpio_pull_down(GPIO_REFERENCE(s->gpio[i]));
			break;
		case EEPROM_GPIO_CFG_INPUT_FLOATING:
			gpio_direction_input(s->gpio[i]);
			break;
		default:
			/* reserved, ignore */
			break;
		}
	}
	return &pdev->dev;	/* return anything non-null */
}

/* Add I2C device detected in slot */
static struct i2c_client *green_add_i2c(struct platform_device *pdev,
					int slot,
					struct list_head *item)
{
	struct i2c_adapter *adapter;
	struct i2c_client *i2cdev;
	struct i2c_board_info board = {};
	struct slot_status *s = green_get_slot(pdev, slot);
	uint8_t irq_gpio;

	strncpy(board.type, eeprom_if_i2c_modalias(item), sizeof(board.type));
	irq_gpio = eeprom_if_i2c_irq_gpio(item);
	if (irq_gpio != IRQ_GPIO_UNUSED)
	{
		board.irq = gpio_to_irq(s->gpio[irq_gpio]);
	}
	board.addr = eeprom_if_i2c_address(item);

	/* TODO: Find intelligent way to assign platform data */

	/* Lookup I2C adapter */
	adapter = i2c_get_adapter(green_get_i2c_bus(pdev, slot));
	if (!adapter) {
		dev_err(&pdev->dev, "No I2C adapter for slot %d.\n", slot);
		return NULL;
	}

	i2cdev = i2c_new_device(adapter, &board);
	i2c_put_adapter(adapter);
	return i2cdev;
}

/* Remove I2C device from slot */
static void green_remove_i2c(struct platform_device *pdev, int slot)
{
	struct slot_status *s = green_get_slot(pdev, slot);
	i2c_unregister_device(s->i2cdev);
	s->i2cdev = NULL;
}

/* Add SPI device detected in slot */
static struct spi_device *green_add_spi(struct platform_device *pdev,
					int slot,
					struct list_head *item)
{
	struct slot_status *s;
	struct green_platform_data *pdata = dev_get_platdata(&pdev->dev);
	struct spi_board_info board = {
		.max_speed_hz = 2*1000*1000,
		.mode = SPI_MODE_0,
		.platform_data = NULL,
		.bus_num = 0,
		.chip_select = 0,
	};
	struct spi_master *master;
	uint8_t irq_gpio;

	s = green_get_slot(pdev, slot);

	/* Assign IRQ number */
	irq_gpio = eeprom_if_spi_irq_gpio(item);
	if (irq_gpio != IRQ_GPIO_UNUSED)
	{
		board.irq = gpio_to_irq(s->gpio[irq_gpio]);
	}
	strncpy(board.modalias, eeprom_if_spi_modalias(item),
		sizeof(board.modalias));

	master = spi_busnum_to_master(green_get_spi_bus(pdev, slot));
	if (!master) {
		dev_err(&pdev->dev, "No master for SPI bus 0.\n");
		return NULL;
	}

	/* Switch SPI bus to selected slot */
	gpio_set_value_cansleep(pdata->spi_sw_gpio, 1 - slot);

	return spi_new_device(master, &board);
}

/* Remove SPI device from slot */
static void green_remove_spi(struct platform_device *pdev, int slot)
{
	struct green_platform_data *pdata = dev_get_platdata(&pdev->dev);
	struct slot_status *s = green_get_slot(pdev, slot);

	spi_unregister_device(s->spidev);
	s->spidev = NULL;

	/* Restore SPI switch default value */
	gpio_set_value_cansleep(pdata->spi_sw_gpio, 0);
}

/* Assign UART device to slot */
static struct device *green_add_uart(struct platform_device *pdev,
				     int slot,
				     struct list_head *item)
{
	struct device *ttydev;
	char *ttyname;
	struct green_platform_data *pdata = dev_get_platdata(&pdev->dev);

	if (slot < 2) {
		ttyname = "msm_serial_hs.0";
		gpio_set_value_cansleep(pdata->uart0_sw_gpio, 1 - slot);
	} else {
		ttyname = "msm_serial_hsl.1";
		gpio_set_value_cansleep(pdata->uart1_sw_gpio, 3 - slot);
	}

	ttydev = bus_find_device_by_name(&platform_bus_type, NULL, ttyname);
	if (!ttydev) {
		dev_err(&pdev->dev, "Slot%d: No UART %s\n", slot, ttyname);
		return NULL;
	}
	dev_dbg(&pdev->dev, "Slot%d: Using UART %s.\n", slot, ttyname);
	return ttydev;
}

/* De-assign UART device from slot */
static void green_remove_uart(struct platform_device *pdev, int slot)
{
	int sw_gpio;
	struct green_platform_data *pdata = dev_get_platdata(&pdev->dev);
	struct slot_status *s = green_get_slot(pdev, slot);

	/* Restore switch GPIO to original value */
	sw_gpio = (slot < 2 ? pdata->uart0_sw_gpio : pdata->uart1_sw_gpio);
	gpio_set_value_cansleep(sw_gpio, 0);
	s->uartdev = NULL;
}

/* Enable SDIO bus in slot 1. Note: only slot 1 supports SDIO */
static struct device *green_add_sdio(struct platform_device *pdev,
				     int slot,
				     struct list_head *item)
{
	struct green_platform_data *pdata = dev_get_platdata(&pdev->dev);

	if (0 != slot) {
		dev_err(&pdev->dev, "Slot%d: SDIO unsupported.\n", slot);
		return NULL;
	}

	gpio_set_value_cansleep(pdata->sdio_sel_gpio, 0);

	return &pdev->dev; /* just return anything non-NULL */
}

/* Disable SDIO bus in slot 1. */
static void green_remove_sdio(struct platform_device *pdev, int slot)
{
	struct green_platform_data *pdata = dev_get_platdata(&pdev->dev);
	struct slot_status *s = green_get_slot(pdev, slot);

	if (0 != slot) {
		dev_err(&pdev->dev, "Slot%d: SDIO unsupported.\n", slot);
		return;
	}

	gpio_set_value_cansleep(pdata->sdio_sel_gpio, 1);
	s->sdiodev = NULL;
}

/* Read slot EEPROM and create appropriate device(s). */
static int scan_slot_eeprom(struct platform_device *pdev, int slot)
{
	int rv;
	struct list_head *item;
	struct slot_status *s;
	struct i2c_client *eeprom;

	/* Pull slot-detect GPIO high and load EEPROM */
	s = green_get_slot(pdev, slot);
	gpio_direction_output(s->det_gpio, 1);

	eeprom = eeprom_load(green_get_i2c_bus(pdev, slot));
	if (NULL == eeprom) {
		dev_err(&pdev->dev, "Slot%d: Bad or missing EEPROM.\n", slot);
		rv = -ENODEV;
		goto eeprom_detect_failed;
	}
	dev_dbg(&pdev->dev, "Slot%d: Found EEPROM\n", slot);

	/* Take IoT board out of reset */
	gpio_set_value_cansleep(s->rst_gpio, 1);

	list_for_each(item, eeprom_if_list(eeprom)) {
		if (eeprom_is_if_gpio(item)) {
			dev_info(&eeprom->dev, "\t\tGPIO device\n");
			s->gpiodev = green_add_gpio(pdev, slot, item);
		}
		if (eeprom_is_if_plat(item)) {
			dev_info(&eeprom->dev, "\t\tPlatform device\n");
		}
		if (eeprom_is_if_i2c(item)) {
			dev_info(&eeprom->dev, "\t\tI2C device\n");
			s->i2cdev = green_add_i2c(pdev, slot, item);
		}
		if (eeprom_is_if_spi(item)) {
			dev_info(&eeprom->dev, "\t\tSPI device\n");
			s->spidev = green_add_spi(pdev, slot, item);
		}
		if (eeprom_is_if_usb(item))
			dev_info(&eeprom->dev, "\t\tUSB device\n");
		if (eeprom_is_if_sdio(item)) {
			dev_info(&eeprom->dev, "\t\tSDIO device\n");
			s->sdiodev = green_add_sdio(pdev, slot, item);
		}
		if (eeprom_is_if_adc(item))
			dev_info(&eeprom->dev, "\t\tADC device\n");
		if (eeprom_is_if_pcm(item))
			dev_info(&eeprom->dev, "\t\tPCM device\n");
		if (eeprom_is_if_uart(item)) {
			dev_info(&eeprom->dev, "\t\tUART device\n");
			s->uartdev = green_add_uart(pdev, slot, item);
		}
	}

	rv = eeprom_num_slots(eeprom);

eeprom_detect_failed:
	/* Restore GPIO direction */
	gpio_direction_input(s->det_gpio);
	return rv;
}

#define SETUP_GPIO(dev, gpio, name) do { \
	int r = devm_gpio_request((dev), (gpio), (name)); \
	if (r) { \
		dev_err((dev), "%s (GPIO%d): error %d\n", \
			(name), (gpio), -r); \
		return r; \
	} \
} while (0)

#define SETUP_INPUT_GPIO(dev, gpio, name) do { \
	SETUP_GPIO((dev), (gpio), (name)); \
	gpio_direction_input((gpio)); \
} while (0)

#define SETUP_OUTPUT_GPIO(dev, gpio, name, value) do { \
	SETUP_GPIO((dev), (gpio), (name)); \
	gpio_direction_output((gpio), (value)); \
} while (0)

/* Probe slots for device presence. Configure devices if present */
static int green_probe_slots(struct platform_device *pdev)
{
	int i, j, rv;
	struct slot_status *s;
	struct green_platform_data *pdata = dev_get_platdata(&pdev->dev);
	struct gpio_chip *en_chip = i2c_get_clientdata(pdata->expander[0]);
	struct gpio_chip *det_chip = i2c_get_clientdata(pdata->expander[1]);
	struct gpio_chip *rst_chip = i2c_get_clientdata(pdata->expander[2]);

	/* Adjust GPIO numbers for SPI and UART switches and acquire GPIOs */
	pdata->sdio_sel_gpio += en_chip->base;
	SETUP_OUTPUT_GPIO(&pdev->dev, pdata->sdio_sel_gpio, "SDIO_sel", 1);

	pdata->spi_sw_en_gpio += en_chip->base;
	SETUP_OUTPUT_GPIO(&pdev->dev, pdata->spi_sw_en_gpio, "SPI_sw_en", 0);

	pdata->spi_sw_gpio += en_chip->base;
	SETUP_OUTPUT_GPIO(&pdev->dev, pdata->spi_sw_gpio, "SPI_en", 0);

	pdata->uart0_sw_en_gpio += en_chip->base;
	SETUP_OUTPUT_GPIO(&pdev->dev, pdata->uart0_sw_en_gpio, "UART0_sw_en", 0);

	pdata->uart0_sw_gpio += en_chip->base;
	SETUP_OUTPUT_GPIO(&pdev->dev, pdata->uart0_sw_gpio, "UART0_sw", 0);

	pdata->uart1_sw_en_gpio += rst_chip->base;
	SETUP_OUTPUT_GPIO(&pdev->dev, pdata->uart1_sw_en_gpio, "UART1_sw_en", 0);

	pdata->uart1_sw_gpio += en_chip->base;
	SETUP_OUTPUT_GPIO(&pdev->dev, pdata->uart1_sw_gpio, "UART1_sw", 0);

	for (i = 0; i < green_num_slots(pdev); i++) {
		char name[32];
		s = green_get_slot(pdev, i);

		/* Let's first adjust the gpio number to be global */
		s->det_gpio += det_chip->base;
		s->rst_gpio += rst_chip->base;
		/* IRQ GPIOs not routed through expanders */

		/* Now request the "slot-present" GPIO, input direction */
		snprintf(name, sizeof(name), "Slot%d_detect", i);
		SETUP_INPUT_GPIO(&pdev->dev, s->det_gpio, name);

		/* "present" is active low */
		s->present = !gpio_get_value_cansleep(s->det_gpio);
		dev_info(&pdev->dev, "%s (GPIO%d): %s\n", name,
			s->det_gpio, (s->present ? "present" : "absent"));

		/* Take the slot reset GPIO */
		snprintf(name, sizeof(name), "Slot%d_reset", i);
		SETUP_OUTPUT_GPIO(&pdev->dev, s->rst_gpio, name, 0);

		/* Take the slot GPIOs */
		for (j = 0; j < 4; j++) {
			snprintf(name, sizeof(name), "Slot%d_GPIO%d", i, j);
			SETUP_INPUT_GPIO(&pdev->dev, s->gpio[j], name);
		}
	}

	/* Map devices in slots */
	i = 0;
	while(i < green_num_slots(pdev)) {
		rv = 1;
		s = green_get_slot(pdev, i);
		if (s->present)
			rv = scan_slot_eeprom(pdev, i);
		/* Ignore slots where eeprom could not be parsed */
		if (rv < 1) {
			s->present = false;
			i++;    /* skip the slot with the unreadable eeprom */
		}
		else {
			i += rv;    /* rv contains number of occupied slots */
		}
	}

	return 0;
}

/* Release all devices from slots and free GPIOs. */
static void green_release_slots(struct platform_device *pdev)
{
	int i, j;
	struct slot_status *s;
	struct green_platform_data *pdata = dev_get_platdata(&pdev->dev);

	/* Release GPIOs */
	for (i = 0; i < green_num_slots(pdev); i++) {
		s = green_get_slot(pdev, i);
		/* TODO: check all busdev types */
		if (s->i2cdev)
			green_remove_i2c(pdev, i);
		if (s->spidev)
			green_remove_spi(pdev, i);
		if (s->uartdev)
			green_remove_uart(pdev, i);
		if (s->sdiodev)
			green_remove_sdio(pdev, i);
		gpio_direction_input(s->det_gpio);
		gpio_direction_input(s->rst_gpio);
		devm_gpio_free(&pdev->dev, s->rst_gpio);
		devm_gpio_free(&pdev->dev, s->det_gpio);
		for (j = 0; j < 4; j++)
			devm_gpio_free(&pdev->dev, s->gpio[j]);
	}

	/* Release switch control GPIOs */
	devm_gpio_free(&pdev->dev, pdata->sdio_sel_gpio);
	devm_gpio_free(&pdev->dev, pdata->spi_sw_en_gpio);
	devm_gpio_free(&pdev->dev, pdata->spi_sw_gpio);
	devm_gpio_free(&pdev->dev, pdata->uart0_sw_en_gpio);
	devm_gpio_free(&pdev->dev, pdata->uart0_sw_gpio);
	devm_gpio_free(&pdev->dev, pdata->uart1_sw_en_gpio);
	devm_gpio_free(&pdev->dev, pdata->uart1_sw_gpio);
}

/* Map the mangOH green IoT slots. */
static int mangoh_green_map(struct platform_device *pdev)
{
	int i;
	struct i2c_adapter *adapter;
	struct i2c_client *newdev, *mux = NULL;
	struct green_platform_data *pdata;

	/*
	 * We first need to find out which WP model we're running on.
	 * Do this by probing I2C bus(ses) for presence of I2C mux
	 * device. Once we know the WP model, assign corresponding
	 * platform data to device.
	 */
	for (i = 0; i < wp_model_last && !mux; i++) {
		adapter = i2c_get_adapter(green_pdata[i].i2c_mux_bus);
		if (!adapter)
			continue;

		/* try mapping the I2C mux */
		mux = i2c_new_device(adapter, &(pca954x_device_info[i]));
		if (!mux && adapter)
			i2c_put_adapter(adapter);
		if (mux)
			/* exit loop for 'i' to index correct WP model */
			break;
	}
	if (!mux) {
		dev_err(&pdev->dev, "Failed to find I2C switch.\n");
		return -ENODEV;
	}

	/* Point 'pdata' to this WP model's platform data */
	pdata = &green_pdata[i];
	dev_info(&pdev->dev, "Detected %s or compatible.\n", pdata->wp_name);
	pdata->mux = mux;

	/* Map GPIO expanders, starting from bus 5 on I2C switch */
	for (i = 0; i < ARRAY_SIZE(sx150x_devinfo); i++) {
		int busno = pdata->i2c_mux_bus + 5 + i;
		adapter = i2c_get_adapter(busno);
		if (!adapter) {
			dev_err(&pdev->dev, "No I2C bus %d.\n", busno);
			return -ENODEV;
		}
		newdev = i2c_new_device(adapter, &(sx150x_devinfo[i]));
		if (!newdev) {
			dev_err(&pdev->dev, "Bus%d: Device %s missing\n",
			       busno, sx150x_devinfo[0].type);
			return -ENODEV;
		}
		pdata->expander[i] = newdev;
	}
	/* We know the WP model, assign platform data to device */
	platform_device_add_data(pdev, pdata, sizeof(*pdata));

	/* Now probe the slots */
	return green_probe_slots(pdev);
}

/* Unmap mangOH green IoT slots. */
static int mangoh_green_unmap(struct platform_device *pdev)
{
	int i;
	struct green_platform_data *pdata = dev_get_platdata(&pdev->dev);
	struct i2c_adapter *adapter = pdata->mux->adapter;

	/* Release gpios first */
	green_release_slots(pdev);

	for (i = 0; i < ARRAY_SIZE(sx150x_devinfo); i++) {
		i2c_unregister_device(pdata->expander[i]);
		i2c_put_adapter(pdata->expander[i]->adapter);
	}
	i2c_unregister_device(pdata->mux);
	i2c_put_adapter(adapter);

	return 0;
}

/* Release function is needed to avoid warning when device is deleted */
static void mangoh_green_release(struct device *dev) { /* do nothing */ }

struct platform_device mangoh_green = {
	.name = "mangoh-green",
	.id	= -1,
	.dev	= {
		.platform_data = NULL,	/* assign after determining WP model */
		.release = mangoh_green_release,
	},
};

struct mangoh_desc mangoh_green_desc = {
	.type = "mangoh-green",
	.map = mangoh_green_map,
	.unmap = mangoh_green_unmap,
};

