#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/jiffies.h>

#define DRIVER_NAME       "gpio_krithik"
#define DEVICE_NAME       "gpio_krithik"
#define GPIO_OUT_NUM      17
#define GPIO_IN_NUM       27
#define TIMER_INTERVAL_MS 500

static int major;
static struct class  *gpio_class;
static struct device *gpio_device;
static int irq_number;
static int gpio_out_state = 0;
static unsigned int irq_count = 0;
static struct timer_list toggle_timer;
static struct gpio_desc *gpio_out_desc;
static struct gpio_desc *gpio_in_desc;

/* ──────────────────────────────────────────
 * IRQ Handler
 * ────────────────────────────────────────── */
static irqreturn_t gpio_irq_handler(int irq, void *dev_id)
{
    irq_count++;
    pr_info("%s: interrupt #%u fired on GPIO%d\n",
            DRIVER_NAME, irq_count, GPIO_IN_NUM);
    return IRQ_HANDLED;
}

/* ──────────────────────────────────────────
 * Timer callback — toggles GPIO17 every 500ms
 * ────────────────────────────────────────── */
static void toggle_timer_callback(struct timer_list *t)
{
    gpio_out_state = !gpio_out_state;
    gpiod_set_value(gpio_out_desc, gpio_out_state);

    pr_info("%s: GPIO%d set %s\n", DRIVER_NAME, GPIO_OUT_NUM,
            gpio_out_state ? "HIGH" : "LOW");

    mod_timer(&toggle_timer,
              jiffies + msecs_to_jiffies(TIMER_INTERVAL_MS));
}

/* ──────────────────────────────────────────
 * file_operations
 * ────────────────────────────────────────── */
static int gpio_open(struct inode *inode, struct file *filp)
{
    pr_info("%s: device opened\n", DRIVER_NAME);
    return 0;
}

static int gpio_release(struct inode *inode, struct file *filp)
{
    pr_info("%s: device closed\n", DRIVER_NAME);
    return 0;
}

static struct file_operations gpio_fops = {
    .owner   = THIS_MODULE,
    .open    = gpio_open,
    .release = gpio_release,
};

/* ──────────────────────────────────────────
 * Module init
 * ────────────────────────────────────────── */
static int __init gpio_init(void)
{
    int ret;

    pr_info("%s: loading\n", DRIVER_NAME);

    /* 1. Request GPIO17 and get descriptor */
    ret = gpio_request(GPIO_OUT_NUM, DRIVER_NAME);
    if (ret) {
        pr_err("%s: failed to request GPIO%d: %d\n",
               DRIVER_NAME, GPIO_OUT_NUM, ret);
        return ret;
    }
    gpio_out_desc = gpio_to_desc(GPIO_OUT_NUM);
    gpiod_direction_output(gpio_out_desc, 0);
    pr_info("%s: GPIO%d configured as output\n", DRIVER_NAME, GPIO_OUT_NUM);

    /* 2. Request GPIO27 and get descriptor */
    ret = gpio_request(GPIO_IN_NUM, DRIVER_NAME);
    if (ret) {
        pr_err("%s: failed to request GPIO%d: %d\n",
               DRIVER_NAME, GPIO_IN_NUM, ret);
        gpio_free(GPIO_OUT_NUM);
        return ret;
    }
    gpio_in_desc = gpio_to_desc(GPIO_IN_NUM);
    gpiod_direction_input(gpio_in_desc);
    pr_info("%s: GPIO%d configured as input\n", DRIVER_NAME, GPIO_IN_NUM);

    /* 3. Get IRQ number for GPIO27 */
    irq_number = gpiod_to_irq(gpio_in_desc);
    if (irq_number < 0) {
        pr_err("%s: failed to get IRQ for GPIO%d\n",
               DRIVER_NAME, GPIO_IN_NUM);
        gpio_free(GPIO_IN_NUM);
        gpio_free(GPIO_OUT_NUM);
        return irq_number;
    }
    pr_info("%s: GPIO%d mapped to IRQ %d\n",
            DRIVER_NAME, GPIO_IN_NUM, irq_number);

    /* 4. Register IRQ handler */
    ret = request_irq(irq_number,
                      gpio_irq_handler,
                      IRQF_TRIGGER_RISING,
                      DRIVER_NAME,
                      NULL);
    if (ret) {
        pr_err("%s: failed to request IRQ %d: %d\n",
               DRIVER_NAME, irq_number, ret);
        gpio_free(GPIO_IN_NUM);
        gpio_free(GPIO_OUT_NUM);
        return ret;
    }
    pr_info("%s: IRQ %d registered, trigger=RISING\n",
            DRIVER_NAME, irq_number);

    /* 5. Setup kernel timer */
    timer_setup(&toggle_timer, toggle_timer_callback, 0);
    mod_timer(&toggle_timer,
              jiffies + msecs_to_jiffies(TIMER_INTERVAL_MS));
    pr_info("%s: timer started, interval=%dms\n",
            DRIVER_NAME, TIMER_INTERVAL_MS);

    /* 6. Register char device */
    major = register_chrdev(0, DEVICE_NAME, &gpio_fops);
    if (major < 0) {
        timer_delete_sync(&toggle_timer);
        free_irq(irq_number, NULL);
        gpio_free(GPIO_IN_NUM);
        gpio_free(GPIO_OUT_NUM);
        return major;
    }

    gpio_class = class_create(DEVICE_NAME);
    if (IS_ERR(gpio_class)) {
        unregister_chrdev(major, DEVICE_NAME);
        timer_delete_sync(&toggle_timer);
        free_irq(irq_number, NULL);
        gpio_free(GPIO_IN_NUM);
        gpio_free(GPIO_OUT_NUM);
        return PTR_ERR(gpio_class);
    }

    gpio_device = device_create(gpio_class, NULL,
                                MKDEV(major, 0), NULL, DEVICE_NAME);
    if (IS_ERR(gpio_device)) {
        class_destroy(gpio_class);
        unregister_chrdev(major, DEVICE_NAME);
        timer_delete_sync(&toggle_timer);
        free_irq(irq_number, NULL);
        gpio_free(GPIO_IN_NUM);
        gpio_free(GPIO_OUT_NUM);
        return PTR_ERR(gpio_device);
    }

    pr_info("%s: loaded, major=%d, /dev/%s created\n",
            DRIVER_NAME, major, DEVICE_NAME);
    return 0;
}

/* ──────────────────────────────────────────
 * Module exit
 * ────────────────────────────────────────── */
static void __exit gpio_exit(void)
{
    timer_delete_sync(&toggle_timer);

    device_destroy(gpio_class, MKDEV(major, 0));
    class_destroy(gpio_class);
    unregister_chrdev(major, DEVICE_NAME);

    free_irq(irq_number, NULL);

    gpiod_set_value(gpio_out_desc, 0);
    gpio_free(GPIO_OUT_NUM);
    gpio_free(GPIO_IN_NUM);

    pr_info("%s: unloaded, total IRQ count=%u\n",
            DRIVER_NAME, irq_count);
}

module_init(gpio_init);
module_exit(gpio_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Krithikkannan");
MODULE_DESCRIPTION("Kernel GPIO driver with interrupt on RPi5");
