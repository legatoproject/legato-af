legacy: main.o
	$(CC) -o legacy main.o

main.o: main.c
	$(CC) -c main.c

.PHONY: clean
clean:
	rm -f main.o legacy

ar7 wp7: CC := /opt/swi/y16-ext/sysroots/x86_64-pokysdk-linux/usr/bin/arm-poky-linux-gnueabi/arm-poky-linux-gnueabi-gcc

ar7 wp7: legacy

