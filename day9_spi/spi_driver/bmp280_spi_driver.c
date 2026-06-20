#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/delay.h>

struct bmp280_data {
    struct spi_device *spi;
    struct miscdevice  misc;
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;
};

static int bmp280_read_reg(struct spi_device *spi, uint8_t reg, uint8_t *val)
{
    uint8_t tx[2] = { reg | 0x80, 0x00 };
    uint8_t rx[2] = { 0 };
    struct spi_transfer xfer = {
        .tx_buf = tx,
        .rx_buf = rx,
        .len    = 2,
    };
    struct spi_message msg;
    spi_message_init(&msg);
    spi_message_add_tail(&xfer, &msg);
    if (spi_sync(spi, &msg) < 0)
        return -EIO;
    *val = rx[1];
    return 0;
}

static int bmp280_write_reg(struct spi_device *spi, uint8_t reg, uint8_t val)
{
    uint8_t tx[2] = { reg & 0x7F, val };
    struct spi_transfer xfer = {
        .tx_buf = tx,
        .len    = 2,
    };
    struct spi_message msg;
    spi_message_init(&msg);
    spi_message_add_tail(&xfer, &msg);
    return spi_sync(spi, &msg);
}

static int bmp280_read_burst(struct spi_device *spi,
                             uint8_t reg, uint8_t *buf, int len)
{
    uint8_t tx[32] = { 0 };
    uint8_t rx[32] = { 0 };
    struct spi_transfer xfer;
    struct spi_message msg;

    tx[0] = reg | 0x80;
    memset(&xfer, 0, sizeof(xfer));
    xfer.tx_buf = tx;
    xfer.rx_buf = rx;
    xfer.len    = len + 1;

    spi_message_init(&msg);
    spi_message_add_tail(&xfer, &msg);
    if (spi_sync(spi, &msg) < 0)
        return -EIO;
    memcpy(buf, &rx[1], len);
    return 0;
}

static ssize_t bmp280_dev_read(struct file *filp,
                               char __user *ubuf,
                               size_t count, loff_t *ppos)
{
    struct bmp280_data *data =
        container_of(filp->private_data, struct bmp280_data, misc);
    struct spi_device *spi = data->spi;
    uint8_t raw[3];
    int32_t adc_T, var1, var2, t_fine, T;
    char    outbuf[64];
    int     len;

    if (bmp280_read_burst(spi, 0xFA, raw, 3) < 0)
        return -EIO;

    adc_T = ((int32_t)raw[0] << 12) |
            ((int32_t)raw[1] <<  4) |
            ((int32_t)raw[2] >>  4);

    var1 = ((((adc_T >> 3) - ((int32_t)data->dig_T1 << 1))) *
             ((int32_t)data->dig_T2)) >> 11;

    var2 = (((((adc_T >> 4) - ((int32_t)data->dig_T1)) *
              ((adc_T >> 4) - ((int32_t)data->dig_T1))) >> 12) *
             ((int32_t)data->dig_T3)) >> 14;

    t_fine = var1 + var2;
    T      = (t_fine * 5 + 128) >> 8;

    len = snprintf(outbuf, sizeof(outbuf),
                   "Temperature: %d.%02d C\n", T / 100, T % 100);

    if (*ppos > 0 || count < len)
        return 0;

    if (copy_to_user(ubuf, outbuf, len))
        return -EFAULT;

    *ppos = len;
    return len;
}

static const struct file_operations bmp280_fops = {
    .owner = THIS_MODULE,
    .read  = bmp280_dev_read,
};

static int bmp280_probe(struct spi_device *spi)
{
    struct bmp280_data *data;
    uint8_t chip_id;
    uint8_t calib[6];
    int ret;

    spi->mode          = SPI_MODE_0;
    spi->bits_per_word = 8;
    spi->max_speed_hz  = 1000000;
    spi_setup(spi);

    ret = bmp280_read_reg(spi, 0xD0, &chip_id);
    if (ret < 0) {
        dev_err(&spi->dev, "Failed to read chip ID\n");
        return ret;
    }
    if (chip_id != 0x58) {
        dev_err(&spi->dev, "Wrong chip ID: 0x%02X\n", chip_id);
        return -ENODEV;
    }
    dev_info(&spi->dev, "BMP280 found, chip ID = 0x%02X\n", chip_id);

    ret = bmp280_write_reg(spi, 0xF4, 0x27);
    if (ret < 0) {
        dev_err(&spi->dev, "Failed to set normal mode\n");
        return ret;
    }
    msleep(10);

    ret = bmp280_read_burst(spi, 0x88, calib, 6);
    if (ret < 0) {
        dev_err(&spi->dev, "Failed to read calibration\n");
        return ret;
    }

    data = devm_kzalloc(&spi->dev, sizeof(*data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    data->spi    = spi;
    data->dig_T1 = ((uint16_t)calib[1] << 8) | calib[0];
    data->dig_T2 = ((int16_t) calib[3] << 8) | calib[2];
    data->dig_T3 = ((int16_t) calib[5] << 8) | calib[4];

    dev_info(&spi->dev, "dig_T1=%u dig_T2=%d dig_T3=%d\n",
             data->dig_T1, data->dig_T2, data->dig_T3);

    data->misc.minor = MISC_DYNAMIC_MINOR;
    data->misc.name  = "bmp280";
    data->misc.fops  = &bmp280_fops;

    ret = misc_register(&data->misc);
    if (ret) {
        dev_err(&spi->dev, "Failed to register misc device\n");
        return ret;
    }

    spi_set_drvdata(spi, data);
    dev_info(&spi->dev, "BMP280 driver probed. /dev/bmp280 ready.\n");
    return 0;
}

static void bmp280_remove(struct spi_device *spi)
{
    struct bmp280_data *data = spi_get_drvdata(spi);
    misc_deregister(&data->misc);
    dev_info(&spi->dev, "BMP280 driver removed\n");
}

static const struct spi_device_id bmp280_id[] = {
    { "bmp280", 0 },
    { }
};
MODULE_DEVICE_TABLE(spi, bmp280_id);

static struct spi_driver bmp280_driver = {
    .driver = {
        .name = "bmp280",
    },
    .id_table = bmp280_id,
    .probe    = bmp280_probe,
    .remove   = bmp280_remove,
};

module_spi_driver(bmp280_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Krithik");
MODULE_DESCRIPTION("BMP280 SPI Kernel Driver");
