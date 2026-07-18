#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/delay.h>

#define DRIVER_NAME     "mpu6050"
#define CLASS_NAME      "mpu6050_class"
#define PWR_MGMT_1      0x6B
#define ACCEL_XOUT_H    0x3B

static struct i2c_client *mpu_client;
static dev_t dev_num;
static struct class *mpu_class;
static struct cdev mpu_cdev;

/* Read len bytes starting from reg into buf */
static int mpu_read_burst(u8 reg, u8 *buf, int len)
{
    return i2c_smbus_read_i2c_block_data(mpu_client, reg, len, buf);
}

/* Called when user does read() on /dev/mpu6050 */
static ssize_t mpu_read(struct file *file, char __user *ubuf,
                         size_t count, loff_t *ppos)
{
    u8 raw[14];
    char output[256];
    int len;
    s16 ax, ay, az, gx, gy, gz, temp_raw;

    if (*ppos > 0)
        return 0;

    /* Read 14 bytes: accel + temp + gyro */
    mpu_read_burst(ACCEL_XOUT_H, raw, 14);

    dev_dbg(&mpu_client->dev, "Raw bytes read: %02x %02x %02x %02x\n", raw[0],raw[1],raw[2],raw[3]);


    /* Combine high and low bytes */
    ax       = (s16)((raw[0]  << 8) | raw[1]);
    ay       = (s16)((raw[2]  << 8) | raw[3]);
    az       = (s16)((raw[4]  << 8) | raw[5]);
    temp_raw = (s16)((raw[6]  << 8) | raw[7]);
    gx       = (s16)((raw[8]  << 8) | raw[9]);
    gy       = (s16)((raw[10] << 8) | raw[11]);
    gz       = (s16)((raw[12] << 8) | raw[13]);

    /* Format output string */
    len = snprintf(output, sizeof(output),
        "Accel  X: %d  Y: %d  Z: %d\n"
        "Gyro   X: %d  Y: %d  Z: %d\n"
        "Temp raw: %d\n",
        ax, ay, az, gx, gy, gz, temp_raw);

    /* Copy from kernel space to user space */
    if (copy_to_user(ubuf, output, len))
        return -EFAULT;

    *ppos = len;
    return len;
}

static struct file_operations mpu_fops = {
    .owner = THIS_MODULE,
    .read  = mpu_read,
};

/* Called when kernel finds MPU6050 on I2C bus */
static int mpu_probe(struct i2c_client *client)
{
    int ret;

    mpu_client = client;
    dev_info(&client->dev, "MPU6050 probed at 0x%02X\n", client->addr);

    /* Wake up sensor */
    i2c_smbus_write_byte_data(client, PWR_MGMT_1, 0x00);
    msleep(100);

    /* Allocate device number */
    ret = alloc_chrdev_region(&dev_num, 0, 1, DRIVER_NAME);
    if (ret < 0) {
        dev_err(&client->dev, "Failed to allocate device number\n");
        return ret;
    }

    /* Create device class */
    mpu_class = class_create(CLASS_NAME);
    if (IS_ERR(mpu_class)) {
        unregister_chrdev_region(dev_num, 1);
        return PTR_ERR(mpu_class);
    }

    /* Create /dev/mpu6050 */
    device_create(mpu_class, NULL, dev_num, NULL, DRIVER_NAME);

    /* Register character device */
    cdev_init(&mpu_cdev, &mpu_fops);
    cdev_add(&mpu_cdev, dev_num, 1);

    dev_info(&client->dev, "/dev/mpu6050 created\n");
    return 0;
}

/* Called when driver is removed */
static void mpu_remove(struct i2c_client *client)
{
    cdev_del(&mpu_cdev);
    device_destroy(mpu_class, dev_num);
    class_destroy(mpu_class);
    unregister_chrdev_region(dev_num, 1);
    dev_info(&client->dev, "MPU6050 driver removed\n");
}

/* Device match table */
static const struct i2c_device_id mpu_id[] = {
    { "mpu6050", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, mpu_id);

/* I2C driver structure */
static struct i2c_driver mpu_driver = {
    .driver = {
        .name  = DRIVER_NAME,
        .owner = THIS_MODULE,
    },
    .probe    = mpu_probe,
    .remove   = mpu_remove,
    .id_table = mpu_id,
};

module_i2c_driver(mpu_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Krithik");
MODULE_DESCRIPTION("MPU6050 I2C Kernel Driver");



