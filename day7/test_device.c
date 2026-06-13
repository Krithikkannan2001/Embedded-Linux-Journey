#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main()
{
    int fd;
    char write_buf[] = "Hello from my C program!";
    char read_buf[100];
    int bytes_read;

    // Open the device
    printf("Opening device...\n");
    fd = open("/dev/mydevice", O_RDWR);
    if (fd < 0) {
        printf("Failed to open device. Run as sudo?\n");
        return -1;
    }
    printf("Device opened. fd = %d\n", fd);

    // Write to the device
    printf("Writing to device...\n");
    write(fd, write_buf, strlen(write_buf));
    printf("Written: %s\n", write_buf);

    // Reset file position to beginning
    lseek(fd, 0, SEEK_SET);

    // Read from the device
    printf("Reading from device...\n");
    bytes_read = read(fd, read_buf, 100);
    read_buf[bytes_read] = '\0';
    printf("Read back: %s\n", read_buf);

    // Close the device
    printf("Closing device...\n");
    close(fd);
    printf("Done.\n");

    return 0;
}
