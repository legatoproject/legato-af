include ../common.mk

.PHONY: app-test exe-test

all: app-test exe-test

app-test:
	mkapp empty.adef -t $(TARGET) -w $(BUILD_DIR) -o $(BUILD_DIR)
	mkapp void.adef -t $(TARGET) -w $(BUILD_DIR) -o $(BUILD_DIR)   2>&1 | grep -q "Executable doesn't contain any components that have source code files." && echo "Correct error message caught."

exe-test:
	mkdir -p $(BUILD_DIR)/bin
	mkexe -t $(TARGET) -o $(BUILD_DIR)/bin/notMuch-exe void notMuch.c -w $(BUILD_DIR)
	mkexe -t $(TARGET) -o $(BUILD_DIR)/bin/void-exe void -w $(BUILD_DIR)  2>&1 | grep -q "Executable doesn't contain any source code files." && echo "Correct error message caught."
