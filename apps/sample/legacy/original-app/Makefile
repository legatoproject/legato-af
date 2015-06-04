legacy: main.o
	$(CC) -o legacy main.o

main.o: main.c
	$(CC) -c main.c

.PHONY: clean
clean:
	rm -f main.o legacy
	rm -rf _build*
	rm -f *.wp7 *.ar7 *.ar86 *.wp85

ar7 wp7 ar86 wp85: CC := /opt/swi/y16-ext/sysroots/x86_64-pokysdk-linux/usr/bin/arm-poky-linux-gnueabi/arm-poky-linux-gnueabi-gcc

ar7 wp7 ar86 wp85: legacy.$(TARGET)

ar7: TARGET := ar7
ar86: TARGET := ar86
wp7: TARGET := wp7
wp85: TARGET := wp85

legacy.$(TARGET): legacy legacy.adef
	mkapp -t $(TARGET) legacy.adef

