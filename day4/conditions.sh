#!/bin/bash

# Check if a device file exists
DEVICE="/dev/i2c-1"

if [ -e "$DEVICE" ]; then
	echo "I2C device found: $DEVICE"
else
	echo "I2C Device NOT found!"

fi 

# NUMBER comparison
TEMP=45

if [ $TEMP -gt 80 ]; then
	echo "WARNING: CPU too hot!"
elif [ $TEMP -gt 60 ]; then
	echo  "CAUTION: CPU warm"
else
	echo "Temperature OK: $TEMP C"
fi

STATUS="running"


if [ "$STATUS" =  "running" ]; then
        echo "Service is running 2 "
fi

