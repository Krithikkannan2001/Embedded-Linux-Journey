#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Krithik");
MODULE_DESCRIPTION("My first kernel module with parameter");
MODULE_VERSION("1.1");

static char *name = "World";
module_param(name, charp,0644);
MODULE_PARM_DESC(name, "Name to greet");

static int __init hello_init(void)
{
	printk(KERN_INFO "Hello, %s! from kernel space!\n",name);
	return 0;
}

static void __exit hello_exit(void)
{
	printk(KERN_INFO "Goodbye, %s! From kernel space!",name);
}

module_init(hello_init);
module_exit(hello_exit);

