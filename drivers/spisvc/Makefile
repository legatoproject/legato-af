#
# Makefile for spisvc kernel module
#
# Copyright (C) Sierra Wireless Inc.  Use of this work is subject to license.
#
obj-m+=spisvc.o

KBUILD_DIR=/opt/swi/y17-ext/sysroots/armv7a-vfp-neon-poky-linux-gnueabi/usr/src/kernel

all:
	make -C $(KBUILD_DIR) M=$(PWD) modules

clean:
	make -C $(KBUILD_DIR) M=$(PWD) clean
