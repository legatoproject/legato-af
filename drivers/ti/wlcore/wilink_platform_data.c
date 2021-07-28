/*
 * This file is part of wl12xx
 *
 * Copyright (C) Sierra Wireless Inc.
 * Copyright (C) 2010-2011 Texas Instruments, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include <linux/module.h>
#include <linux/err.h>
#include <linux/wl12xx.h>

#include <linux/gpio.h>
#include <linux/sierra_gpio.h>

#define MSM_WIFI_IRQ_ALIAS_GPIO		"WIFI_IRQ"	/* IOT0_GPIO1 */
#define MSM_WLAN_EN_ALIAS_GPIO		"WLAN_EN"	/* IOT0_GPIO3 */


static struct wl1251_platform_data *wl1251_platform_data;
static struct wl12xx_static_platform_data *wl12xx_static_platform_data;

int __init wl1251_set_platform_data(const struct wl1251_platform_data *data)
{
	if (wl1251_platform_data)
		return -EBUSY;
	if (!data)
		return -EINVAL;

	wl1251_platform_data = kmemdup(data, sizeof(*data), GFP_KERNEL);
	if (!wl1251_platform_data)
		return -ENOMEM;

	return 0;
}

struct wl1251_platform_data *wl1251_get_platform_data(void)
{
	if (!wl1251_platform_data)
		return ERR_PTR(-ENODEV);

	return wl1251_platform_data;
}
//EXPORT_SYMBOL(wl1251_get_platform_data); Commented to avoid duplication with WLAN_VENDOR_TI enabled kernel.

int wl12xx_set_platform_data(const struct wl12xx_static_platform_data *data)
{
	if (wl12xx_static_platform_data)
		return -EBUSY;
	if (!data)
		return -EINVAL;

	wl12xx_static_platform_data = kmemdup(data, sizeof(*data), GFP_KERNEL);
	if (!wl12xx_static_platform_data)
		return -ENOMEM;

	return 0;
}

struct wl12xx_static_platform_data *wl12xx_get_platform_data(void)
{
	struct wl12xx_static_platform_data msm_wl12xx_pdata;
	int ret;
	struct gpio_desc *desc;

	memset(&msm_wl12xx_pdata, 0, sizeof(msm_wl12xx_pdata));

	if (gpio_alias_lookup(MSM_WLAN_EN_ALIAS_GPIO, &desc)) {
		pr_err("wl18xx: NO WLAN_EN gpio");
		return ERR_PTR(-ENODEV);
	}
	msm_wl12xx_pdata.wlan_en = desc_to_gpio(desc);
	pr_info("wl12xx WLAN_EN GPIO: %d\n", msm_wl12xx_pdata.wlan_en);
	if (gpio_alias_lookup(MSM_WIFI_IRQ_ALIAS_GPIO, &desc)) {
		pr_err("wl18xx: NO WIFI_IRQ gpio");
		return ERR_PTR(-ENODEV);
	}
	msm_wl12xx_pdata.irq = gpio_to_irq(desc_to_gpio(desc));
	pr_info("wl12xx IRQ: %d\n", msm_wl12xx_pdata.irq);
	if (msm_wl12xx_pdata.irq < 0)
		return ERR_PTR(-ENODEV);

	msm_wl12xx_pdata.ref_clock_freq = 38400000;
	msm_wl12xx_pdata.tcxo_clock_freq = 19200000;

	ret = wl12xx_set_platform_data(&msm_wl12xx_pdata);

	if (!wl12xx_static_platform_data)
		return ERR_PTR(-ENOMEM);

	return wl12xx_static_platform_data;
}
//EXPORT_SYMBOL(wl12xx_get_platform_data); Commented to avoid duplication with WLAN_VENDOR_TI enabled kernel.
