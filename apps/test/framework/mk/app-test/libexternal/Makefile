
TARGETS := localhost wp7 ar7

.PHONY: all clean

all: $(TARGETS)

clean:
	rm -f *.so

localhost: CC = gcc
ar7: CC = $(AR7_TOOLCHAIN_DIR)/arm-poky-linux-gnueabi-gcc
wp7: CC = $(WP7_TOOLCHAIN_DIR)/arm-poky-linux-gnueabi-gcc

.PHONY: $(TARGETS)
$(TARGETS):
	$(CC) -o libexternal.$@.so -shared -fPIC -Wall -Werror libexternal.c

