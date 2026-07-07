#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/platform_device.h>
#include <linux/of.h>


#define DRIVER_NAME       "gpio_krithik"
#define DEVICE_NAME       "gpio_krithik"
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


static irqreturn_t gpio_irq_handler(int irq, void *dev_id)
{
    irq_count++;
    pr_info("%s: interrupt #%u fired\n", DRIVER_NAME, irq_count);
    return IRQ_HANDLED;
}

static void toggle_timer_callback(struct timer_list *t)
{
    gpio_out_state = !gpio_out_state;
    gpiod_set_value(gpio_out_desc, gpio_out_state);
    pr_info("%s: GPIO output set %s\n", DRIVER_NAME,
            gpio_out_state ? "HIGH" : "LOW");
    mod_timer(&toggle_timer,
              jiffies + msecs_to_jiffies(TIMER_INTERVAL_MS));
}


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


static int gpio_probe(struct platform_device *pdev)
{
    int ret;

    pr_info("%s: probe() called\n", DRIVER_NAME);

    /* 1. Get GPIO17 descriptor from DT property "output-gpios" */
    gpio_out_desc = gpiod_get(&pdev->dev, "output", GPIOD_OUT_LOW);
    if (IS_ERR(gpio_out_desc)) {
        pr_err("%s: failed to get output GPIO: %ld\n",
               DRIVER_NAME, PTR_ERR(gpio_out_desc));
        return PTR_ERR(gpio_out_desc);
    }
    pr_info("%s: output GPIO acquired\n", DRIVER_NAME);

    /* 2. Get GPIO27 descriptor from DT property "interrupt-gpios" */
    gpio_in_desc = gpiod_get(&pdev->dev, "interrupt", GPIOD_IN);
    if (IS_ERR(gpio_in_desc)) {
        pr_err("%s: failed to get interrupt GPIO: %ld\n",
               DRIVER_NAME, PTR_ERR(gpio_in_desc));
        gpiod_put(gpio_out_desc);
        return PTR_ERR(gpio_in_desc);
    }
    pr_info("%s: interrupt GPIO acquired\n", DRIVER_NAME);

    /* 3. Get IRQ number from DT "interrupts" property */
    irq_number = platform_get_irq(pdev, 0);
    if (irq_number < 0) {
        pr_err("%s: failed to get IRQ: %d\n", DRIVER_NAME, irq_number);
        gpiod_put(gpio_in_desc);
        gpiod_put(gpio_out_desc);
        return irq_number;
    }
    pr_info("%s: IRQ number = %d\n", DRIVER_NAME, irq_number);

    /* 4. Register IRQ handler */
    ret = request_irq(irq_number, gpio_irq_handler,
                      IRQF_TRIGGER_FALLING, DRIVER_NAME, NULL);
    if (ret) {
        pr_err("%s: failed to request IRQ: %d\n", DRIVER_NAME, ret);
        gpiod_put(gpio_in_desc);
        gpiod_put(gpio_out_desc);
        return ret;
    }
    pr_info("%s: IRQ %d registered\n", DRIVER_NAME, irq_number);

    /* 5. Setup kernel timer */
    timer_setup(&toggle_timer, toggle_timer_callback, 0);
    mod_timer(&toggle_timer,
              jiffies + msecs_to_jiffies(TIMER_INTERVAL_MS));
    pr_info("%s: timer started\n", DRIVER_NAME);

    /* 6. Register char device */
    major = register_chrdev(0, DEVICE_NAME, &gpio_fops);
    if (major < 0) {
        timer_delete_sync(&toggle_timer);
        free_irq(irq_number, NULL);
        gpiod_put(gpio_in_desc);
        gpiod_put(gpio_out_desc);
        return major;
    }

    gpio_class = class_create(DEVICE_NAME);
    if (IS_ERR(gpio_class)) {
        unregister_chrdev(major, DEVICE_NAME);
        timer_delete_sync(&toggle_timer);
        free_irq(irq_number, NULL);
        gpiod_put(gpio_in_desc);
        gpiod_put(gpio_out_desc);
        return PTR_ERR(gpio_class);
    }

    gpio_device = device_create(gpio_class, NULL,
                                MKDEV(major, 0), NULL, DEVICE_NAME);
    if (IS_ERR(gpio_device)) {
        class_destroy(gpio_class);
        unregister_chrdev(major, DEVICE_NAME);
        timer_delete_sync(&toggle_timer);
        free_irq(irq_number, NULL);
        gpiod_put(gpio_in_desc);
        gpiod_put(gpio_out_desc);
        return PTR_ERR(gpio_device);
    }

    pr_info("%s: loaded successfully, major=%d\n", DRIVER_NAME, major);
    return 0;
}

static void gpio_remove(struct platform_device *pdev)
{
    timer_delete_sync(&toggle_timer);
    device_destroy(gpio_class, MKDEV(major, 0));
    class_destroy(gpio_class);
    unregister_chrdev(major, DEVICE_NAME);
    free_irq(irq_number, NULL);
    gpiod_set_value(gpio_out_desc, 0);
    gpiod_put(gpio_out_desc);
    gpiod_put(gpio_in_desc);
    pr_info("%s: removed, total IRQ count=%u\n", DRIVER_NAME, irq_count);
    
}

static const struct of_device_id gpio_of_match[] = {
    { .compatible = "krithik-gpio-interrupt" },
    { }
};
MODULE_DEVICE_TABLE(of, gpio_of_match);

static struct platform_driver gpio_platform_driver = {
    .probe  = gpio_probe,
    .remove = gpio_remove,
    .driver = {
        .name           = DRIVER_NAME,
        .of_match_table = gpio_of_match,
    },
};

module_platform_driver(gpio_platform_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Krithikkannan");
MODULE_DESCRIPTION("Platform GPIO driver with interrupt on RPi5");








