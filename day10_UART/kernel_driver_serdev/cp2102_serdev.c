#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/serdev.h>
#include <linux/of.h>
#include <linux/string.h>

#define DRIVER_NAME  "cp2102_serdev"
#define BAUD_RATE    115200

static size_t cp2102_receive(struct serdev_device *serdev,
                             const u8 *buf, size_t count)
{
    int i;
    pr_info("%s: received %zu bytes: ", DRIVER_NAME, count);
    for (i = 0; i < count; i++)
        pr_cont("%c", buf[i]);
    pr_cont("\n");
    return count;
}

static void cp2102_wakeup(struct serdev_device *serdev)
{
    pr_info("%s: write wakeup\n", DRIVER_NAME);
}

static const struct serdev_device_ops cp2102_ops = {
    .receive_buf  = cp2102_receive,
    .write_wakeup = cp2102_wakeup,
};

static int cp2102_probe(struct serdev_device *serdev)
{
    int ret;
    const char *msg = "Hello from serdev driver!\n";

    pr_info("%s: probe called\n", DRIVER_NAME);

    serdev_device_set_client_ops(serdev, &cp2102_ops);

    ret = serdev_device_open(serdev);
    if (ret) {
        pr_err("%s: failed to open: %d\n", DRIVER_NAME, ret);
        return ret;
    }

    serdev_device_set_baudrate(serdev, BAUD_RATE);
    serdev_device_set_flow_control(serdev, false);
    serdev_device_set_parity(serdev, SERDEV_PARITY_NONE);

    ret = serdev_device_write(serdev, msg, strlen(msg),
                              msecs_to_jiffies(1000));
    if (ret < 0)
        pr_err("%s: write failed: %d\n", DRIVER_NAME, ret);
    else
        pr_info("%s: sent %d bytes\n", DRIVER_NAME, ret);

    return 0;
}

static void cp2102_remove(struct serdev_device *serdev)
{
    pr_info("%s: remove called\n", DRIVER_NAME);
    serdev_device_close(serdev);
}

static const struct of_device_id cp2102_of_match[] = {
    { .compatible = "cp2102-serdev" },
    { }
};
MODULE_DEVICE_TABLE(of, cp2102_of_match);

static struct serdev_device_driver cp2102_driver = {
    .probe  = cp2102_probe,
    .remove = cp2102_remove,
    .driver = {
        .name           = DRIVER_NAME,
        .of_match_table = cp2102_of_match,
    },
};

static int __init cp2102_init(void)
{
    pr_info("%s: loading\n", DRIVER_NAME);
    return serdev_device_driver_register(&cp2102_driver);
}

static void __exit cp2102_exit(void)
{
    serdev_device_driver_unregister(&cp2102_driver);
    pr_info("%s: unloaded\n", DRIVER_NAME);
}

module_init(cp2102_init);
module_exit(cp2102_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Krithikkannan");
MODULE_DESCRIPTION("serdev UART driver for CP2102");
