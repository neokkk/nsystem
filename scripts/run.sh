#!/bin/bash

kernel_version=$(uname -r)

modprobe i2c-dev
modprobe i2c-bcm2835

sudo insmod ./bmp280.ko
sudo insmod ./engine.ko
sudo ./nsystem