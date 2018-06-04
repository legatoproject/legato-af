include ../../common.mk

$(TARGET):
	mkdir -p $(BUILD_DIR)
	$(TARGET_CC) $(TARGET_CFLAGS) -o $(BUILD_DIR)/libexternal.$@.so -shared -fPIC -Wall -Werror libexternal.c

