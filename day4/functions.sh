#!/bin/bash

# Function definition
check_device() {
	local DEVICE=$1

	if [ -e "$DEVICE" ]; then
		echo "[OK]  $DEVICE exists"
		return 0
	else	
		echo "[FAIL] $DEVICE not found"
		return 1
	fi
}

print_separator() {
	echo "===================="
}

# Call Function

print_separator
check_device "/dev/i2c-1"
check_device "/dev/spidev0.0"
check_device "/dev/ttyUSB0"
check_device "/dev/gpiochip0"
print_separator
