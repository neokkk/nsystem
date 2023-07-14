obj-m := simple_io.o

export BUILDROOT_DIR=/home/nk/Workspaces/buildroot
export ARCH=arm64
export CROSS_COMPILE=$(BUILDROOT_DIR)/output/host/bin/aarch64-buildroot-linux-gnu-

KERNEL_DIR ?= $(BUILDROOT_DIR)/output/build/linux-custom

all default: modules
install: modules_install

modules modules_install help clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(shell pwd) $@

.PHONY: nfs
nfs:
	cp simple_io.ko ~/Shared
