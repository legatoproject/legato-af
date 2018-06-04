include ../common.mk

.PHONY: all clean test1

all: test1

test1:  $(BUILD_DIR)/lib/libexternal.so
	mkexe -v -o '$(BUILD_DIR)/foo' foo.c munchkin '$(BUILD_DIR)/lib/libexternal.so'  -w $(BUILD_DIR) -l $(BUILD_DIR)/lib/ -t $(TARGET)
	mkexe -v -o '$(BUILD_DIR)/foo2' foo.c munchkin '$(BUILD_DIR)/lib/libexternal.so' -w $(BUILD_DIR) --cflags="-g -O0" -l $(BUILD_DIR)/lib/ -t $(TARGET)
	mkexe -v -o '$(BUILD_DIR)/foo' foo.c munchkin '$(BUILD_DIR)/lib/libexternal.so'  -w $(BUILD_DIR) -l $(BUILD_DIR)/lib/ -t $(TARGET)

$(BUILD_DIR)/lib/libexternal.so:
	mkdir -p $(@D)
	$(TARGET_CC) $(TARGET_CFLAGS) -fPIC -shared -o $@ libexternal.c
