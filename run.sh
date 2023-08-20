#!/bin/bash

kernel_version=$(uname -r)

# modprobe i2c-dev
insmod /lib/modules/${kernel_version}/kernel/drivers/i2c/i2c-dev.ko
# modprobe i2c-bcm2835
insmod /lib/modules/${kernel_version}/kernel/drivers/i2c/busses/i2c-bcm2835.ko

insmod bmp280.ko engine.ko

./nsystem