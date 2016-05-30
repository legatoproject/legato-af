include ../common.mk

$(TARGET): %: "$(BUILD_DIR)/libexternal/libexternal.%.so"
	mkapp mkappTest.adef -v -t $@ --cflags="'-g' -O0" --ldflags=-lrt -w $(BUILD_DIR) -o $(BUILD_DIR)

"$(BUILD_DIR)/libexternal/libexternal.%.so":
	$(MAKE) -C libexternal

clean:
	rm -rf $(BUILD_DIR)
	$(MAKE) -C libexternal clean
