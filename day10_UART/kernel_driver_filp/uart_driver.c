#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/tty.h>
#include <linux/tty_driver.h>
#include <linux/tty_flip.h>
#include <linux/termios.h>
#include <uapi/linux/termios.h>
#include <uapi/asm-generic/termios.h>
#include <uapi/asm-generic/ioctls.h>


#define DRIVER_NAME   "uart_krithik"
#define DEVICE_NAME   "uart_krithik"
#define UART_DEVICE   "/dev/ttyAMA10"
#define BUF_SIZE      256

static int    major;
static struct class  *uart_class;
static struct device *uart_device;

/* ── file_operations forward declaration ── */
static struct file_operations uart_fops;

/* ──────────────────────────────────────────
 * configure_tty - set ttyAMA10 to 115200 8N1 raw
 * ────────────────────────────────────────── */
static int configure_tty(struct file *filp)
{
    struct tty_struct *tty;
    struct ktermios   kt;

    tty = ((struct tty_file_private *)filp->private_data)->tty;
    if (!tty) {
        pr_err("%s: could not get tty_struct\n", DRIVER_NAME);
        return -ENODEV;
    }

    kt = tty->termios;

    /* raw mode */
    kt.c_iflag &= ~(IXON | IXOFF | IXANY | ICRNL | INLCR);
    kt.c_oflag &= ~OPOST;
    kt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG | IEXTEN);
    kt.c_cflag &= ~(PARENB | CSTOPB | CSIZE);
    kt.c_cflag |=  (CS8 | CREAD | CLOCAL);

    /* VMIN=0, VTIME=10 */
    kt.c_cc[VMIN]  = 0;
    kt.c_cc[VTIME] = 10;

    /* baud rate 115200 */
    tty_termios_encode_baud_rate(&kt, 115200, 115200);

    tty_set_termios(tty, &kt);

    pr_info("%s: ttyAMA10 configured 115200 8N1 raw\n", DRIVER_NAME);
    return 0;
}

/* ──────────────────────────────────────────
 * open
 * ────────────────────────────────────────── */
static int uart_open(struct inode *inode, struct file *filp)
{
    struct file *tty_filp;
    int ret;

    tty_filp = filp_open(UART_DEVICE, O_RDWR | O_NOCTTY, 0);
    if (IS_ERR(tty_filp)) {
        pr_err("%s: failed to open %s, err=%ld\n",
               DRIVER_NAME, UART_DEVICE, PTR_ERR(tty_filp));
        return PTR_ERR(tty_filp);
    }

    ret = configure_tty(tty_filp);
    if (ret) {
        filp_close(tty_filp, NULL);
        return ret;
    }

    filp->private_data = tty_filp;
    pr_info("%s: opened\n", DRIVER_NAME);
    return 0;
}

/* ──────────────────────────────────────────
 * write — user → ttyAMA10
 * ────────────────────────────────────────── */
static ssize_t uart_write(struct file *filp, const char __user *buf,
                          size_t count, loff_t *ppos)
{
    struct file *tty_filp = filp->private_data;
    char kbuf[BUF_SIZE];
    ssize_t ret;

    if (count > BUF_SIZE)
        count = BUF_SIZE;

    if (copy_from_user(kbuf, buf, count))
        return -EFAULT;

    ret = kernel_write(tty_filp, kbuf, count, ppos);
    pr_info("%s: wrote %zd bytes\n", DRIVER_NAME, ret);
    return ret;
}

/* ──────────────────────────────────────────
 * read — ttyAMA10 → user
 * ────────────────────────────────────────── */
static ssize_t uart_read(struct file *filp, char __user *buf,
                         size_t count, loff_t *ppos)
{
    struct file *tty_filp = filp->private_data;
    char kbuf[BUF_SIZE];
    ssize_t ret;

    if (count > BUF_SIZE)
        count = BUF_SIZE;

    ret = kernel_read(tty_filp, kbuf, count, ppos);
    if (ret <= 0)
        return ret;

    if (copy_to_user(buf, kbuf, ret))
        return -EFAULT;

    pr_info("%s: read %zd bytes\n", DRIVER_NAME, ret);
    return ret;
}

/* ──────────────────────────────────────────
 * release
 * ────────────────────────────────────────── */
static int uart_release(struct inode *inode, struct file *filp)
{
    struct file *tty_filp = filp->private_data;

    if (tty_filp)
        filp_close(tty_filp, NULL);

    pr_info("%s: closed\n", DRIVER_NAME);
    return 0;
}

/* ──────────────────────────────────────────
 * file_operations
 * ────────────────────────────────────────── */
static struct file_operations uart_fops = {
    .owner   = THIS_MODULE,
    .open    = uart_open,
    .write   = uart_write,
    .read    = uart_read,
    .release = uart_release,
};

/* ──────────────────────────────────────────
 * init
 * ────────────────────────────────────────── */
static int __init uart_init(void)
{
    major = register_chrdev(0, DEVICE_NAME, &uart_fops);
    if (major < 0) {
        pr_err("%s: register_chrdev failed: %d\n", DRIVER_NAME, major);
        return major;
    }

    uart_class = class_create(DEVICE_NAME);
    if (IS_ERR(uart_class)) {
        unregister_chrdev(major, DEVICE_NAME);
        return PTR_ERR(uart_class);
    }

    uart_device = device_create(uart_class, NULL,
                                MKDEV(major, 0), NULL, DEVICE_NAME);
    if (IS_ERR(uart_device)) {
        class_destroy(uart_class);
        unregister_chrdev(major, DEVICE_NAME);
        return PTR_ERR(uart_device);
    }

    pr_info("%s: loaded, major=%d, /dev/%s created\n",
            DRIVER_NAME, major, DEVICE_NAME);
    return 0;
}

/* ──────────────────────────────────────────
 * exit
 * ────────────────────────────────────────── */
static void __exit uart_exit(void)
{
    device_destroy(uart_class, MKDEV(major, 0));
    class_destroy(uart_class);
    unregister_chrdev(major, DEVICE_NAME);
    pr_info("%s: unloaded\n", DRIVER_NAME);
}

module_init(uart_init);
module_exit(uart_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Krithikkannan");
MODULE_DESCRIPTION("Kernel UART driver using ttyAMA10");
