obj-m := simple_switch.o

export BUILDROOT_DIR=/home/nk/Workspaces/buildroot
export ARCH=arm64
export CROSS_COMPILE=$(BUILDROOT_DIR)/output/host/bin/aarch64-buildroot-linux-gnu-

KERNEL_DIR ?= $(BUILDROOT_DIR)/output/build/linux-custom

all default: modules
install: modules_install

modules modules_install:
	$(MAKE) -C $(KERNEL_DIR) M=$(shell pwd) $@

.PHONY: clean nfs

clean:
	rm -rf *.o *.ko *.mod *.mod.* .*.cmd .tmp_versions Module.symvers modules.order

nfs:
	cp simple_switch.ko ~/Shared