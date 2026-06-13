#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Krithik");
MODULE_DESCRIPTION("Simple character device driver");
MODULE_VERSION("1.0");

#define DEVICE_NAME "mydevice"
#define BUFFER_SIZE 1024

static int major_number;
static char device_buffer[BUFFER_SIZE];
static int buffer_length = 0;


static int dev_open(struct inode *inode, struct file *file)
{
printk(KERN_INFO "mydevice: device opened\n");
return 0;
}


static int dev_release(struct inode *inode, struct file *file)
{
printk(KERN_INFO "mydevice: device closed\n");
return 0;
}

static ssize_t dev_read(struct file *file, char __user *buf, size_t len , loff_t *offset)
{
int bytes_read = 0;

if (*offset >= buffer_length)
	return 0;

while (len && (*offset < buffer_length))
{
put_user(device_buffer[*offset],buf++);
len--;
(*offset)++;
bytes_read++;
}

printk(KERN_INFO "mydevice: sent %d bytes to user\n", bytes_read);
return bytes_read;

}


static ssize_t dev_write(struct file *file, const char __user *buf,size_t len, loff_t *offset)
{
int i;

buffer_length = 0;

for(i=0;i< len && i< (BUFFER_SIZE -1);i++)
{
get_user(device_buffer[i],buf + i);
buffer_length++;
}

device_buffer[buffer_length] = '\0';

printk(KERN_INFO ":mydevice: recieved %d bytes from user\n",buffer_length);
return len;

}



static struct file_operations fops = {

	.owner 	= THIS_MODULE,
	.open	= dev_open,
	.read	= dev_read,
	.write	= dev_write,
	.release= dev_release,

};


static int __init chardev_init(void)
{

major_number = register_chrdev(0,DEVICE_NAME, &fops);

if (major_number < 0)
{
        printk(KERN_ALERT "mydevice: failed to register. Error %d\n", major_number);
        return major_number;
}

printk(KERN_INFO "mydevice: registered with major number %d\n", major_number);
printk(KERN_INFO "mydevice: create device with: sudo mknod /dev/%s c %d 0\n", DEVICE_NAME, major_number);
return 0;
}

static void __exit chardev_exit(void)
{
    unregister_chrdev(major_number, DEVICE_NAME);
    printk(KERN_INFO "mydevice: unregistered\n");
}

module_init(chardev_init);
module_exit(chardev_exit);




































