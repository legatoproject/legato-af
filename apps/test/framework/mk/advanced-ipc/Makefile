include ../common.mk

VERBOSE := -v

$(TARGET): %: %_client %_server

%_server:
	mkapp server.adef $(VERBOSE) -i interfaces -t $* -w $(BUILD_DIR) -o $(BUILD_DIR)

%_client:
	mkapp client.adef $(VERBOSE) -i interfaces -t $* -w $(BUILD_DIR) -o $(BUILD_DIR)
