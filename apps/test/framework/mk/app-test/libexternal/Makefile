
TARGETS := localhost ar7 wp85

.PHONY: all clean

all: $(TARGETS)

clean:
	rm -f *.so

localhost: CC = gcc
ar7: CC = $(AR7_TOOLCHAIN_DIR)/arm-poky-linux-gnueabi-gcc
wp85: CC = $(WP85_TOOLCHAIN_DIR)/arm-poky-linux-gnueabi-gcc

.PHONY: $(TARGETS)
$(TARGETS):
	$(CC) -o libexternal.$@.so -shared -fPIC -Wall -Werror libexternal.c

