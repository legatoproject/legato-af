//--------------------------------------------------------------------------------------------------
/**
 * @file module5A.c
 *
 * A test kernel module
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */
//--------------------------------------------------------------------------------------------------

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

int GLOBAL_VARIABLE = 1000;
EXPORT_SYMBOL(GLOBAL_VARIABLE);

static int __init module5A_init(void)
{
   pr_info("Executing %s()\n", __func__);
   return 0;
}

static void __exit module5A_exit(void)
{
   pr_info("Executing %s().\n", __func__);
}

module_init(module5A_init);
module_exit(module5A_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sierra Wireless, Inc.");
MODULE_DESCRIPTION("Example of Legato kernel module");
MODULE_VERSION("1.0");
