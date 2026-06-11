#!/bin/bash

# For loop — fixed list
echo "--- Counting ---"
for i in 1 2 3 4 5; do
    echo "Count: $i"
done

# For loop with range
echo "--- Even numbers ---"
for i in $(seq 2 2 10); do
    echo $i
done

# While loop
echo "--- While loop ---"
COUNT=0
while [ $COUNT -lt 3 ]; do
        echo "Count is: $COUNT"
        COUNT=$((COUNT+1))
done

echo "--- I2C devices on system ---"
for dev in /dev/i2c-*; do
        if [ -e "$dev" ]; then
		echo "Found: $dev"
	fi
done

