// SPDX-License-Identifier: GPL-2.0
/*
 * Step 1 of an embedded Linux driver project.
 * A minimal loadable kernel module, built against the running kernel.
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

static char *who = "world";
module_param(who, charp, 0444);
MODULE_PARM_DESC(who, "name to greet in the kernel log");

static int __init hello_init(void)
{
	pr_info("hello: module loaded, hello %s\n", who);
	return 0;
}

static void __exit hello_exit(void)
{
	pr_info("hello: module unloaded\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yusuf Mehmed");
MODULE_DESCRIPTION("Minimal loadable kernel module");
MODULE_VERSION("0.1");
