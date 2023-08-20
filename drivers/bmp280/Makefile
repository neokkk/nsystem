obj-m += bmp280.o

kernel_version := $(shell uname -r)

# export BUILDROOT_DIR=/home/nk/Workspaces/buildroot
# export ARCH=arm64
# export CROSS_COMPILE=$(BUILDROOT_DIR)/output/host/bin/aarch64-buildroot-linux-gnu-

# KERNEL_DIR ?= $(BUILDROOT_DIR)/output/build/linux-custom
KERNEL_DIR ?= /lib/modules/$(kernel_version)/build

all default: modules
install: modules_install

modules modules_install help clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(shell pwd) $@

.PHONY: nfs
nfs:
	cp bmp280.ko ~/Shared