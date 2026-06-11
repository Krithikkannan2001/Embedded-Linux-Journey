#!/bin/bash

#===============================
# RPi Embedded System Checker
#===============================

PASS=0
FAIL=0


check_device() {
	local DEV=$1
	local NAME=$2
	if [ -e "$DEV" ]; then
		echo "[PASS] $NAME ($DEV)"
		PASS=$((PASS+1))
	else
		echo "[FAIL] $NAME ($DEV)"
		FAIL=$((FAIL+1))
	fi
}


print_header() {
	echo ""
	echo "==============================="
	echo " $1"
	echo "==============================="
}


# System Info
print_header "SYSTEM INFO"
echo "Hostname	: $(hostname)"
echo "Kernel	: $(uname -r)"
echo "Uptime	: $(uptime -p)"
echo "Date	: $(date)"


# CPU Temp
print_header "CPU TEMPERATURE"
TEMP=$(vcgencmd measure_temp 2>/dev/null | cut -d= -f2)
echo "CPU Temp  : $TEMP"


# MEMORY
print_header "MEMORY"
free -h | grep Mem


# Device Check
print_header "PERIPHERAL DEVICES"
check_device "/dev/i2c-1"	"I2C Bus 1"
check_device "/dev/spidev0.0"	"SPI Bus 0"
check_device "/dev/ttyUSB0"	"UART CP2102"
check_device "/dev/gpiochip0"	"GPIO Chip"

# Summary
print_header "SUMMARY"
echo "Passed: $PASS"
echo "Failed: $FAIL"
echo ""














