TARGETS = wp85 ar7 localhost
.PHONY: $(TARGETS)

.PHONY: all
all: $(TARGETS)

ar7: TARGET := ar7

wp85: TARGET := wp85

localhost: TARGET := localhost

$(TARGETS):%: munchkin_% wwow_% dorothy_%

BIN_DIR = build/$@/$(TARGET)/bin
LIB_DIR = build/$@/$(TARGET)/lib
OBJ_DIR = build/$@/$(TARGET)/obj

localhost: externalLibTests

munchkin_%:
	mkdir -p $(BIN_DIR)
	mkdir -p $(LIB_DIR)
	mkdir -p $(OBJ_DIR)
	mkexe -v -t $(TARGET) -o $(BIN_DIR)/$@ munchkin -w $(OBJ_DIR) -l $(LIB_DIR)

wwow_%:
	mkdir -p build/$@/$(TARGET)/bin
	mkdir -p build/$@/$(TARGET)/lib
	mkdir -p build/$@/$(TARGET)/obj
	mkexe -v -t $(TARGET) -o $(BIN_DIR)/$@ munchkin wwow -w $(OBJ_DIR) -l $(LIB_DIR)

dorothy_%:
	mkdir -p build/$@/$(TARGET)/bin
	mkdir -p build/$@/$(TARGET)/lib
	mkdir -p build/$@/$(TARGET)/obj
	mkexe -v -t $(TARGET) -o $(BIN_DIR)/$@ munchkin dorothy -w $(OBJ_DIR) -l $(LIB_DIR)

.PHONY: externalLibTests
externalLibTests: foo fandm

.PHONY: foo
foo: build/libexternal/localhost/lib/libexternal.so
	mkdir -p build/$@/$(TARGET)/bin
	mkdir -p build/$@/$(TARGET)/lib
	mkdir -p build/$@/$(TARGET)/obj
	mkexe -v -t $(TARGET) -o $(BIN_DIR)/foo foo.c build/libexternal/$(TARGET)/lib/libexternal.so -w $(OBJ_DIR) -l $(LIB_DIR)

.PHONY: fandm
fandm: build/libexternal/localhost/lib/libexternal.so
	mkdir -p build/$@/$(TARGET)/bin
	mkdir -p build/$@/$(TARGET)/lib
	mkdir -p build/$@/$(TARGET)/obj
	mkexe -v -t $(TARGET) -o $(BIN_DIR)/fandm foo.c munchkin build/libexternal/$(TARGET)/lib/libexternal.so -w $(OBJ_DIR) -l $(LIB_DIR)

build/libexternal/%/lib/libexternal.so:
	mkdir -p $(dir $@)
	$(CC) -fPIC -shared -o $@ libexternal.c

.PHONY: clean
clean:
	rm -rf build

