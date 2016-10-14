/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the version 2 of the GNU General Public License
 * as published by the Free Software Foundation
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2016 Sierra Wireless Inc.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spi/spi.h>

#define MOD_DESC "Spidev creation module"

MODULE_DESCRIPTION(MOD_DESC);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Sierra Wireless, Inc.");

static struct spi_device *spidev;

static __init int spisvc_init(void)
{
	struct spi_master *master;
	struct spi_board_info board = {
		.modalias = "spidev",
		.max_speed_hz = 15058800,
		.mode = SPI_MODE_3,
		.platform_data = NULL,
		.bus_num = 0,
		.chip_select = 0,
		.irq = 0,
	};

	master = spi_busnum_to_master(0);
	if (!master) {
		pr_err("No master for SPI bus 0.\n");
		return -ENODEV;
	}

	spidev = spi_new_device(master, &board);
	if (!spidev) {
		pr_err("Error creating device '%s'\n", board.modalias);
		return -ENODEV;
	}
	return 0;
}
module_init(spisvc_init);

static __exit void spisvc_exit(void)
{
	if (spidev)
		spi_unregister_device(spidev);
}
module_exit(spisvc_exit);

