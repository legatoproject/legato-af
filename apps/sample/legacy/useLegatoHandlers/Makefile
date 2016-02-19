# Define variable containing list of supported targets.
SUPPORTED_TARGETS := localhost ar7 ar86 wp85
.PHONY: $(SUPPORTED_TARGETS)

# Declare the default build goal.
.PHONY: default
default: localhost

# Select appropriate cross tool chain based on build goal.
ar7: CC := $(AR7_TOOLCHAIN_DIR)/arm-poky-linux-gnueabi-gcc
ar86: CC := $(AR86_TOOLCHAIN_DIR)/arm-poky-linux-gnueabi-gcc
wp85: CC := $(WP85_TOOLCHAIN_DIR)/arm-poky-linux-gnueabi-gcc

export CC

# If the TARGET variable is not set, create the appropriate _build/X directory and run
# make again with the TARGET variable set.
ifndef TARGET

localhost ar7 ar86 wp85:
	mkdir -p _build/$@
	$(MAKE) legacy.$@ TARGET=$@

endif

# Add the liblegato includes directory to the list of directories to search for header files.
CFLAGS := -I$(LEGATO_ROOT)/framework/c/inc

# Rule to link the "legacy" executable.
_build/$(TARGET)/legacy: _build/$(TARGET)/main.o _build/$(TARGET)/le_sms_client.o
	$(CC) -o _build/$(TARGET)/legacy $^ \
		  -L$(LEGATO_ROOT)/build/$(TARGET)/framework/lib -llegato -lpthread -lrt

# Rule to build main.o.
_build/$(TARGET)/main.o: main.c le_sms_interface.h le_mdmDefs_interface.h
	$(CC) $(CFLAGS) -o $@ -c $<

# Rule to build the application bundle.
legacy.$(TARGET): _build/$(TARGET)/legacy legacy.adef
	mkapp -t $(TARGET) legacy.adef

# Rule to build the generated SMS client code.
_build/$(TARGET)/le_sms_client.o: le_sms_client.c le_sms_interface.h le_mdmDefs_interface.h le_sms_local.h
	$(CC) $(CFLAGS) -o $@ -c $<

# Rule to generate the client-side IPC code for the SMS API.
le_sms_interface.h le_sms_client.c le_sms_local.h:
	ifgen --gen-interface --gen-client --gen-local $(LEGATO_ROOT)/interfaces/modemServices/le_sms.api

# Rule to generate the header file for the definitions shared by the SMS API and other modem APIs.
le_mdmDefs_interface.h:
	ifgen --gen-interface $(LEGATO_ROOT)/interfaces/modemServices/le_mdmDefs.api

.PHONY: clean
clean:
	rm -rf _build*
	rm -f *.update
	rm -f le_sms_* le_mdmDefs_*
