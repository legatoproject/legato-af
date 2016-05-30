TARGET ?= localhost
export TARGET

.PHONY: $(TARGET) all clean
all: $(TARGET)

# the subst command gets the path to $CURDIR relative to $LEGATO_ROOT
BUILD_DIR :=  ${LEGATO_ROOT}/build/$(TARGET)/$(subst ${LEGATO_ROOT}/,,$(CURDIR))
export BUILD_DIR

localhost: CC ?= gcc
ar7: CC ?= $(AR7_TOOLCHAIN_DIR)/arm-poky-linux-gnueabi-gcc
wp85: CC ?= $(WP85_TOOLCHAIN_DIR)/arm-poky-linux-gnueabi-gcc

VERBOSE := -v

clean:
	rm -rf $(BUILD_DIR)