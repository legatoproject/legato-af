//--------------------------------------------------------------------------------------------------
/** @file example.c
 *
 * @section Description
 *
 * This file contains an example of a kernel module to be installed with Legato on a target system.
 * The module itself does nothing other than printing a kernel log message whenever the module
 * is loaded and unloaded. It also prints values of optional two module parameters (param1 and param2).
 * Values of these two parameters can be overriden in the params: section of the module's .mdef file.
 * See example.mdef file for details.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
//--------------------------------------------------------------------------------------------------


#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

static char *param1 = "PARAM1";
module_param(param1, charp, S_IRUGO);
MODULE_PARM_DESC(param1, "First module parameter");

static char *param2 = "PARAM2";
module_param(param2, charp, S_IRUGO);
MODULE_PARM_DESC(param2, "Second module parameter");

static int __init example_init(void)
{
   pr_info("Executing %s(), param1='%s' param2='%s'.\n", __func__, param1, param2);
   return 0;
}

static void __exit example_exit(void)
{
   pr_info("Executing %s().\n", __func__);
}

module_init(example_init);
module_exit(example_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sierra Wireless, Inc.");
MODULE_DESCRIPTION("Example of Legato kernel module");
MODULE_VERSION("1.0");
