#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

#define SPI_DEVICE "/dev/spidev0.0"
// #define dig_T1		0x88
#define temp		0xFA


int main(void)
{
    int fd;
    uint8_t mode = SPI_MODE_0;
    uint8_t bits = 8;
    uint32_t speed = 1000000; // 1 MHz

    fd = open(SPI_DEVICE, O_RDWR);
    if (fd < 0)
    {
        perror("Failed to open SPI device");
        return -1;
    }

    ioctl(fd, SPI_IOC_WR_MODE, &mode);
    ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
    ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);

    uint8_t tx[] = {0xD0 | 0x80, 0x00}; // Read Chip ID register
    uint8_t rx[2] = {0};

    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = 2,
        .speed_hz = speed,
        .bits_per_word = bits,
    };

    if (ioctl(fd, SPI_IOC_MESSAGE(1), &tr) < 1)
    {
        perror("SPI transfer failed");
        close(fd);
        return -1;
    }

    printf("BMP280 Chip ID = 0x%02X\n", rx[1]);

    if (rx[1] == 0x58)
        printf("BMP280 detected successfully!\n");
    else
        printf("Unexpected Chip ID. Check wiring/configuration.\n");


  //  uint8_t tx_dig_T[] = {0x88 | 0x80, 0x00, 0x00,0x00}; // Read DIG_T register for calculation
  // uint8_t rx_dig_T[4] = {0};

uint8_t tx_dig_T[] = {
    0x88 | 0x80,
    0x00,0x00,0x00,
    0x00,0x00,0x00
};

uint8_t rx_dig_T[7] = {0};


    struct spi_ioc_transfer tr_dig_T = {
        .tx_buf = (unsigned long)tx_dig_T,
        .rx_buf = (unsigned long)rx_dig_T,
        .len = 7,
        .speed_hz = speed,
        .bits_per_word = bits,
    };
    if (ioctl(fd, SPI_IOC_MESSAGE(1), &tr_dig_T) < 1)
    {
        perror("SPI transfer failed");
        close(fd);
        return -1;
    }

// uint16_t dig_T1 = rx_dig_T[1];
// int16_t  dig_T2 = rx_dig_T[2];
// int16_t  dig_T3 = rx_dig_T[3];
   
uint16_t dig_T1 =
        ((uint16_t)rx_dig_T[2] << 8) |
         (uint16_t)rx_dig_T[1];

int16_t dig_T2 =
        ((int16_t)rx_dig_T[4] << 8) |
         (int16_t)rx_dig_T[3];

int16_t dig_T3 =
        ((int16_t)rx_dig_T[6] << 8) |
         (int16_t)rx_dig_T[5];


 printf(" BMP280 dig_T1 = 0x%x\n dig_T2 = 0x%x\n dig_T3 = 0x%x\n", rx_dig_T[1], rx_dig_T[2], rx_dig_T[3]);




    uint8_t tx_val[] = {0xFA | 0x80, 0x00, 0x00,0x00}; // Read DIG_T register for calculat>
    uint8_t rx_val[4] = {0};

    struct spi_ioc_transfer tr_val = {
        .tx_buf = (unsigned long)tx_val,
        .rx_buf = (unsigned long)rx_val,
        .len = 4,
        .speed_hz = speed,
        .bits_per_word = bits,
    };
    if (ioctl(fd, SPI_IOC_MESSAGE(1), &tr_val) < 1)
    {
        perror("SPI transfer failed");
        close(fd);
        return -1;
    }


int32_t adc_T;

adc_T = (rx_val[1]<<12 | rx_val[2]<<4 | rx_val[3]>>4);

printf("adc_T = %d\n",adc_T);


int32_t var1, var2;
int32_t t_fine;
int32_t T;

var1 = ((((adc_T >> 3) -
         ((int32_t)dig_T1 << 1))) *
         ((int32_t)dig_T2)) >> 11;

var2 = (((((adc_T >> 4) -
          ((int32_t)dig_T1)) *
         ((adc_T >> 4) -
          ((int32_t)dig_T1))) >> 12) *
         ((int32_t)dig_T3)) >> 14;

t_fine = var1 + var2;

T = (t_fine * 5 + 128) >> 8;

float temperature = T / 100.0f;


printf("Temperature = %.2f C\n", temperature);




    close(fd);
    return 0;
}
