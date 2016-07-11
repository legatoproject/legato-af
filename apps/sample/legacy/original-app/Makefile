# Define variable containing list of supported targets.
SUPPORTED_TARGETS := localhost ar7 ar86 wp85 wp750x
.PHONY: $(SUPPORTED_TARGETS)

# Declare the default build goal.
.PHONY: default
default: localhost

# Select appropriate cross tool chain based on build goal.
ar7: CC := $(AR7_TOOLCHAIN_DIR)/arm-poky-linux-gnueabi-gcc
ar86: CC := $(AR86_TOOLCHAIN_DIR)/arm-poky-linux-gnueabi-gcc
wp85: CC := $(WP85_TOOLCHAIN_DIR)/arm-poky-linux-gnueabi-gcc
wp750x: CC := $(WP750X_TOOLCHAIN_DIR)/arm-poky-linux-gnueabi-gcc

export CC

# If the TARGET variable is not set, create the appropriate _build/X directory and run
# make again with the TARGET variable set.
ifndef TARGET

localhost ar7 ar86 wp85 wp750x:
	mkdir -p _build/$@
	$(MAKE) legacy.$@ TARGET=$@

endif

# Rule to link the "legacy" executable.
_build/$(TARGET)/legacy: _build/$(TARGET)/main.o
	$(CC) $(LDFLAGS) -o $@ $^

# Rule to build main.o.
_build/$(TARGET)/main.o: main.c
	$(CC) $(CFLAGS) -o $@ -c $<

# Rule to build the application bundle.
legacy.$(TARGET): _build/$(TARGET)/legacy legacy.adef
	mkapp -t $(TARGET) legacy.adef

# Rule for cleaning up build artifacts.
.PHONY: clean
clean:
	rm -rf _build*
	rm -f *.update
