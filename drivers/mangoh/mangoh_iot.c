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
 * IoT expansion platform driver for Sierra Wireless mangOH board(s).
 * Currently supporting only mangOH green platform.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/platform_device.h>

#include "mangoh.h"

static char *model = "green";
module_param(model, charp, S_IRUGO);
MODULE_PARM_DESC(model, "mangOH board model.");

static const struct platform_device_id mangoh_iot_ids[] = {
	{"mangoh-green", (kernel_ulong_t)&mangoh_green_desc},
	{},
};
MODULE_DEVICE_TABLE(platform, mangoh_iot_ids);

static int mangoh_iot_probe(struct platform_device *pdev)
{
	struct mangoh_desc *desc;

	desc = (struct mangoh_desc *)platform_get_device_id(pdev)->driver_data;
	if (!desc || !desc->map)
		return -ENODEV;

	platform_set_drvdata(pdev, desc);

	return desc->map(pdev);
}

static int mangoh_iot_remove(struct platform_device *pdev)
{
	struct mangoh_desc *desc = platform_get_drvdata(pdev);

	if (desc && desc->unmap)
		desc->unmap(pdev);

	/* desc will be freed with device removal, so we're done */
	dev_info(&pdev->dev, "Removed.\n");

	return 0;
}

static struct platform_driver mangoh_iot_driver = {
	.probe		= mangoh_iot_probe,
	.remove		= mangoh_iot_remove,
	.driver		= {
		.name	= "mangoh-iot",
		.owner	= THIS_MODULE,
		.bus	= &platform_bus_type,
	},
	.id_table	= mangoh_iot_ids,
};

static int __init mangoh_iot_init(void)
{
	struct platform_device *pdev;

	if (!strcasecmp(model, "green"))
		pdev = &mangoh_green;
	else
		pdev = NULL;

	if (!pdev) {
		pr_err("%s: unknown model 'mangoh-%s'.\n", __func__, model);
		return -ENODEV;
	}

	platform_driver_register(&mangoh_iot_driver);
	if (platform_device_register(pdev)) {
		platform_driver_unregister(&mangoh_iot_driver);
		return -ENODEV;
	}

	return 0;
}

static void __exit mangoh_iot_exit(void)
{
	platform_device_unregister(&mangoh_green);
	platform_driver_unregister(&mangoh_iot_driver);
}

module_init(mangoh_iot_init);
module_exit(mangoh_iot_exit);

MODULE_ALIAS("platform:mangoh-iot");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sierra Wireless");
MODULE_DESCRIPTION("Linux driver for mangOH IoT expander");
MODULE_VERSION("0.2");
