.PHONY: default localhost ar7 wp7 ar86 wp85

default: localhost

legacy: main.o api_client_libs
	$(CC) -o legacy main.o libIF_le_sms_client.so -L$(LEGATO_ROOT)/build/$(TARGET)/bin/lib -llegato

main.o: main.c api_client_libs
	$(CC) -c main.c -I_build -I$(LEGATO_ROOT)/framework/c/inc

.PHONY: clean
clean:
	rm -f main.o legacy
	rm -rf _build*
	rm -f *.localhost *.wp7 *.ar7 *.ar86 *.wp85
	rm -f *.so

ar7 wp7 ar86 wp85: CC := /opt/swi/y16-ext/sysroots/x86_64-pokysdk-linux/usr/bin/arm-poky-linux-gnueabi/arm-poky-linux-gnueabi-gcc

localhost ar7 wp7 ar86 wp85: legacy.$(TARGET)

localhost: TARGET := localhost
ar7: TARGET := ar7
ar86: TARGET := ar86
wp7: TARGET := wp7
wp85: TARGET := wp85

legacy.$(TARGET): legacy legacy.adef
	mkapp -t $(TARGET) legacy.adef

.PHONY: api_client_libs
api_client_libs:
	mkcomp --stand-alone -t $(TARGET) -i $(LEGATO_ROOT)/interfaces/modemServices api_client

