#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

static char *param1 = "PARAM1";
module_param(param1, charp, S_IRUGO);
MODULE_PARM_DESC(param1, "First module parameter");

static char *param2 = "PARAM2";
module_param(param2, charp, S_IRUGO);
MODULE_PARM_DESC(param2, "Second module parameter");

static int __init module1_init(void)
{
   pr_info("Executing %s(), param1='%s' param2='%s'.\n", __func__, param1, param2);
   return 0;
}

static void __exit module1_exit(void)
{
   pr_info("Executing %s().\n", __func__);
}

module_init(module1_init);
module_exit(module1_exit);

MODULE_LICENSE("MPL");
MODULE_AUTHOR("Sierra Wireless, Inc.");
MODULE_DESCRIPTION("Legato kernel module for testing");
MODULE_VERSION("1.0");
