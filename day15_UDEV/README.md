

# Day 15 — Udev

- Explored kernel uevent -> udevd -> /dev/ pipeline using udevadm monitor
- Reused Day 8 MPU6050 I2C driver (mpu6050_driver.ko) to trigger real probe/device_create events
- Hit a driver binding race: kernel's built-in inv_mpu6050_i2c claimed the device before custom driver on first attempt
- Resolved by testing after a fresh reboot (no competing module auto-loaded yet) -> custom driver's probe() fired successfully
- Wrote udev rule (99-mpu6050.rules) to auto-set MODE=0664 GROUP=i2c on /dev/mpu6050, removing need for sudo
- Note: driver load (insmod) and device registration (new_device) are NOT persistent across reboots -> future Device Tree overlay needed for permanent setup
