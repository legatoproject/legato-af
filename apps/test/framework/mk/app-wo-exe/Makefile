include ../common.mk

$(TARGET): %: "$(BUILD_DIR)/libexternal/libexternal.%.so"
	mkapp mkappTest.adef -v -t $@ -w $(BUILD_DIR) -o $(BUILD_DIR) -s $(BUILD_DIR)

"$(BUILD_DIR)/libexternal/libexternal.%.so":
	$(MAKE) -C libexternal

clean:
	rm -rf $(BUILD_DIR)
	$(MAKE) -C libexternal clean
