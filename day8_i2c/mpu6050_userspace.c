#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <stdint.h>

#define I2C_BUS        "/dev/i2c-1"
#define MPU6050_ADDR   0x68

#define PWR_MGMT_1     0x6B
#define WHO_AM_I       0x75
#define ACCEL_XOUT_H   0x3B

int fd;

/* Write one byte to a register */
void mpu_write(uint8_t reg, uint8_t data)
{
    uint8_t buf[2] = {reg, data};
    write(fd, buf, 2);
}

/* Read one byte from a register */
uint8_t mpu_read_byte(uint8_t reg)
{
    write(fd, &reg, 1);
    uint8_t val;
    read(fd, &val, 1);
    return val;
}

/* Read len bytes starting from reg */
void mpu_read_burst(uint8_t reg, uint8_t *buf, int len)
{
    write(fd, &reg, 1);
    read(fd, buf, len);
}

/* Combine high and low bytes into signed 16-bit */
int16_t combine(uint8_t high, uint8_t low)
{
    return (int16_t)((high << 8) | low);
}

int main(void)
{
    /* Open I2C bus */
    fd = open(I2C_BUS, O_RDWR);
    if (fd < 0) {
        perror("Failed to open I2C bus");
        return 1;
    }

    /* Set slave address */
    if (ioctl(fd, I2C_SLAVE, MPU6050_ADDR) < 0) {
        perror("Failed to set I2C slave address");
        return 1;
    }

    /* Verify WHO_AM_I */
    uint8_t who = mpu_read_byte(WHO_AM_I);
    printf("WHO_AM_I = 0x%02X (expected 0x68)\n", who);
    if (who != 0x68 && who != 0x70) {
        printf("ERROR: Wrong device!\n");
        return 1;
    }

    /* Wake up sensor — clear sleep bit */
    mpu_write(PWR_MGMT_1, 0x00);
    usleep(100000); /* 100ms settle time */

    printf("MPU6050 awake. Reading sensor data...\n\n");

    /* Read 14 bytes: ACCEL(6) + TEMP(2) + GYRO(6) */
    uint8_t buf[14];
    mpu_read_burst(ACCEL_XOUT_H, buf, 14);

    /* Combine bytes into 16-bit signed values */
    int16_t ax = combine(buf[0],  buf[1]);
    int16_t ay = combine(buf[2],  buf[3]);
    int16_t az = combine(buf[4],  buf[5]);
    int16_t temp_raw = combine(buf[6], buf[7]);
    int16_t gx = combine(buf[8],  buf[9]);
    int16_t gy = combine(buf[10], buf[11]);
    int16_t gz = combine(buf[12], buf[13]);

    /* Convert to real units */
    float ax_g = ax / 16384.0f;
    float ay_g = ay / 16384.0f;
    float az_g = az / 16384.0f;
    float temp_c = temp_raw / 340.0f + 36.53f;
    float gx_ds = gx / 131.0f;
    float gy_ds = gy / 131.0f;
    float gz_ds = gz / 131.0f;

    printf("=== Accelerometer ===\n");
    printf("  X: %6d raw  =>  %.3f g\n", ax, ax_g);
    printf("  Y: %6d raw  =>  %.3f g\n", ay, ay_g);
    printf("  Z: %6d raw  =>  %.3f g\n", az, az_g);

    printf("\n=== Temperature ===\n");
    printf("  Raw: %d  =>  %.2f C\n", temp_raw, temp_c);

    printf("\n=== Gyroscope ===\n");
    printf("  X: %6d raw  =>  %.3f deg/s\n", gx, gx_ds);
    printf("  Y: %6d raw  =>  %.3f deg/s\n", gy, gy_ds);
    printf("  Z: %6d raw  =>  %.3f deg/s\n", gz, gz_ds);

    close(fd);
    return 0;
}
