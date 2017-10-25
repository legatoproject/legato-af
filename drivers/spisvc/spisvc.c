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

#define SPI_INVALID_BUS (-1)
int busnum = SPI_INVALID_BUS;
module_param(busnum, int, 0644);
MODULE_PARM_DESC(busnum, "SPI bus number");

unsigned int cs = 0;
module_param(cs, uint, 0644);
MODULE_PARM_DESC(cs, "SPI chip select");

#define SPI_MAX_BUS	16	/* take a reasonable number */

static struct spi_device *spidev;

static __init int spisvc_init(void)
{
	struct spi_master *m = NULL;
	struct spi_board_info board = {
		.modalias = "spidev",
		.max_speed_hz = 15058800,
		.mode = SPI_MODE_3,
		.platform_data = NULL,
		.bus_num = 0,
		.chip_select = 0,
		.irq = 0,
	};

	if (SPI_INVALID_BUS == busnum)
		/* Bus not assigned: find SPI master with lowest bus number */
		for (busnum = 0; SPI_MAX_BUS > busnum && NULL == m; busnum++)
			m = spi_busnum_to_master(busnum);
	else
		m = spi_busnum_to_master(busnum);

	if (!m) {
		pr_err("SPI bus not available.\n");
		return -ENODEV;
	}

	board.bus_num = busnum = m->bus_num;
	board.chip_select = cs;
	spidev = spi_new_device(m, &board);
	if (!spidev) {
		dev_err(&m->dev, "Cannot add '%s' on bus %u, cs %u\n",
			board.modalias, board.bus_num, board.chip_select);
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

