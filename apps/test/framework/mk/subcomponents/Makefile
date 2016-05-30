include ../common.mk

$(TARGET): %: %_test

%_test:
	mkapp test.adef $(VERBOSE) -t $* -w $(BUILD_DIR) -o $(BUILD_DIR)

