include ../common.mk

$(TARGET):
	mkapp sandboxed.adef $(VERBOSE) -t $@ -w $(BUILD_DIR) -o $(BUILD_DIR)
	mkapp unsandboxed.adef $(VERBOSE) -t $@ -w $(BUILD_DIR) -o $(BUILD_DIR)
