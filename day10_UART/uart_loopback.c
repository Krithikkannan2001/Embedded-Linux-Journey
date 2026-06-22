#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#define DEVICE "/dev/ttyUSB0"
#define BAUD 	B115200


int main()
{
int fd;
struct termios tty;
char tx_buf[] = "Hello from RPi5!\n";
char rx_buf[64];
int n;

/* Open device  */
fd = open(DEVICE,O_RDWR | O_NOCTTY);
if(fd<0)
{
perror("open");
return 1;
}
printf("Opened %s\n", DEVICE);

/* Read Current Settings  */
if(tcgetattr(fd,&tty)!=0)
{
perror("tcgetattr");
close(fd);
return 1;
}

cfsetispeed(&tty, BAUD);
cfsetospeed(&tty, BAUD);

/* 8N1 — 8 data bits, no parity, 1 stop bit  */
tty.c_cflag &= ~PARENB;        /* no parity */
tty.c_cflag &= ~CSTOPB;        /* 1 stop bit */
tty.c_cflag &= ~CSIZE;         /* clear size bits */
tty.c_cflag |=  CS8;           /* 8 data bits */
tty.c_cflag |=  CREAD | CLOCAL; /* enable RX, ignore modem lines */

/* 5. Raw mode — no processing */
tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG | IEXTEN);
tty.c_iflag &= ~(IXON | IXOFF | IXANY | ICRNL);
tty.c_oflag &= ~OPOST;

/* 6. Read timeout: wait up to 1s, minimum 0 bytes */
tty.c_cc[VMIN]  = 0;
tty.c_cc[VTIME] = 10;   /* 10 * 0.1s = 1 second timeout */


    /* 7. Apply settings */
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        perror("tcsetattr");
        close(fd);
        return 1;
    }

    /* 8. Flush buffers */
    tcflush(fd, TCIOFLUSH);

    /* 9. Write */
    n = write(fd, tx_buf, strlen(tx_buf));
    printf("Wrote %d bytes: %s", n, tx_buf);

    /* 10. Small delay for loopback */
    usleep(100000);  /* 100ms */

    /* 11. Read back */
    memset(rx_buf, 0, sizeof(rx_buf));
    n = read(fd, rx_buf, sizeof(rx_buf) - 1);
    if (n > 0)
        printf("Received %d bytes: %s", n, rx_buf);
    else
        printf("No data received (check TXD->RXD wire)\n");

    close(fd);
    return 0;
}



























