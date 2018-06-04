include ../common.mk

$(TARGET):%: munchkin_% wwow_% dorothy_%

BIN_DIR := "$(BUILD_DIR)/bin"
LIB_DIR := "$(BUILD_DIR)/lib"
OBJ_DIR := "$(BUILD_DIR)/obj"

localhost: externalLibTests


munchkin_%:
	mkdir -p $(BIN_DIR) $(LIB_DIR) $(OBJ_DIR)
	mkexe -v -t $(TARGET) -j $(LEGATO_JOBS) -o $(BIN_DIR)/$@ munchkin -w $(OBJ_DIR) -l $(LIB_DIR)

wwow_%:
	mkdir -p $(BIN_DIR) $(LIB_DIR) $(OBJ_DIR)
	mkexe -v -t $(TARGET) -j $(LEGATO_JOBS) -o $(BIN_DIR)/$@ munchkin wwow -w $(OBJ_DIR) -l $(LIB_DIR)

dorothy_%:
	mkdir -p $(BIN_DIR) $(LIB_DIR) $(OBJ_DIR)
	mkexe -v -t $(TARGET) -j $(LEGATO_JOBS) -o $(BIN_DIR)/$@ munchkin dorothy -w $(OBJ_DIR) -l $(LIB_DIR)

.PHONY: externalLibTests
externalLibTests: foo fandm

.PHONY: foo
foo: $(BUILD_DIR)/libexternal/lib/libexternal.so
	mkdir -p $(BIN_DIR) $(LIB_DIR) $(OBJ_DIR)
	mkexe -v -t $(TARGET) -j $(LEGATO_JOBS) -o $(BIN_DIR)/foo foo.c $(BUILD_DIR)/libexternal/lib/libexternal.so -w $(OBJ_DIR) -l $(LIB_DIR)

.PHONY: fandm
fandm: $(BUILD_DIR)/libexternal/lib/libexternal.so
	mkdir -p $(BIN_DIR) $(LIB_DIR) $(OBJ_DIR)
	mkexe -v -t $(TARGET) -j $(LEGATO_JOBS) -o $(BIN_DIR)/fandm foo.c munchkin $(BUILD_DIR)/libexternal/lib/libexternal.so -w $(OBJ_DIR) -l $(LIB_DIR)

$(BUILD_DIR)/libexternal/lib/libexternal.so:
	mkdir -p $(dir $@)
	$(TARGET_CC) $(TARGET_CFLAGS) -fPIC -shared -o $@ libexternal.c
