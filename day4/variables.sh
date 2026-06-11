#!/bin/bash

NAME="Krithik"
BOARD="RaspberryPi5"
VERSION=5

echo "Hello $NAME"
echo "Board: $BOARD version $VERSION"

KERNEL=$(uname -r)
CPU=$(uname -m)

echo "Kernel: $KERNEL"
echo "Architecture: $CPU"

readonly MAX_RETRIES=3
echo "Max retries: $MAX_RETRIES"
