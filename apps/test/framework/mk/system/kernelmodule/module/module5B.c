//--------------------------------------------------------------------------------------------------
/**
 * @file module5B.c
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

extern int GLOBAL_VARIABLE;

static int __init module5B_init(void)
{
   pr_info("Executing %s()\n", __func__);
   printk(KERN_INFO "Value of GLOBAL_VARIABLE %d", GLOBAL_VARIABLE);
   return 0;
}

static void __exit module5B_exit(void)
{
   pr_info("Executing %s().\n", __func__);
}

module_init(module5B_init);
module_exit(module5B_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sierra Wireless, Inc.");
MODULE_DESCRIPTION("Example of Legato kernel module");
MODULE_VERSION("1.0");
