obj-m += mangoh.o
mangoh-objs += mangoh_iot.o green.o eeprom.o

KBUILD_DIR=/opt/swi/y17-ext/sysroots/armv7a-vfp-neon-poky-linux-gnueabi/usr/src/kernel

all:
	make -C $(KBUILD_DIR) M=$(PWD) modules

clean:
	make -C $(KBUILD_DIR) M=$(PWD) clean
