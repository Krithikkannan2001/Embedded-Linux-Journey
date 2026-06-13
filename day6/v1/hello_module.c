#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Krithik");
MODULE_DESCRIPTION("My first kernel module");
MODULE_VERSION("1.0");


static int __init hello_init(void)
{
	printk(KERN_INFO "Hello from kernel space!\n");
	return 0;
}

static void __exit hello_exit(void)
{
	printk(KERN_INFO "Goodbye from kernel space!\n");
}

module_init(hello_init);
module_exit(hello_exit);
