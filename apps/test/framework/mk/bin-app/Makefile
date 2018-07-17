
include ../common.mk

.NOTPARALLEL:
.PHONY: $(TARGET) source.$(TARGET) binary.$(TARGET) app.$(TARGET)

SHELL := /bin/bash

# Use same system, same system name for both source and binary build as GCC embeds file names
# into generated binaries.
$(TARGET): source.$(TARGET) binary.$(TARGET)
	if ! diff -q $(BUILD_DIR)/source/hello-system.$(TARGET).update \
	             $(BUILD_DIR)/binary/hello-system.$(TARGET).update; then \
	    src_system_dir=`mktemp -d`; \
	    bin_system_dir=`mktemp -d`; \
	    tar -xjvf $(BUILD_DIR)/build-source/hello-system.$(TARGET) -C $$src_system_dir; \
	    tar -xjvf $(BUILD_DIR)/build-binary/hello-system.$(TARGET) -C $$bin_system_dir; \
	    diff -r -U2 "$$src_system_dir" "$$bin_system_dir"; \
	    src_app_dir=`mktemp -d`; \
	    bin_app_dir=`mktemp -d`; \
	    tar -xjvf $(BUILD_DIR)/build-source/app/hello/hello.$(TARGET) -C $$src_app_dir; \
	    tar -xjvf $(BUILD_DIR)/build-binary/app/hello/hello.$(TARGET) -C $$bin_app_dir; \
	    diff -r -U2 "$$src_app_dir" "$$bin_app_dir"; \
	    rm -rf $$src_system_dir $$bin_system_dir $$src_app_dir $$bin_app_dir; \
	    echo "error: source and binary systems differ" >&2; \
	    exit 1; \
	fi
	if [ $(LE_CONFIG_ENABLE_IMA) -eq y ] ; then \
		if ! diff -q $(BUILD_DIR)/source/hello-system.$(TARGET).signed.update \
		             $(BUILD_DIR)/binary/hello-system.$(TARGET).signed.update; then \
		    src_system_dir=`mktemp -d`; \
		    bin_system_dir=`mktemp -d`; \
		    tar -xjvf $(BUILD_DIR)/build-source/hello-system.signed.$(TARGET) -C $$src_system_dir; \
		    tar -xjvf $(BUILD_DIR)/build-binary/hello-system.signed.$(TARGET) -C $$bin_system_dir; \
		    diff -r -U2 "$$src_system_dir" "$$bin_system_dir"; \
		    src_app_dir=`mktemp -d`; \
		    bin_app_dir=`mktemp -d`; \
		    tar -xjvf $(BUILD_DIR)/build-source/app/hello/hello.$(TARGET).signed -C $$src_app_dir; \
		    tar -xjvf $(BUILD_DIR)/build-binary/app/hello/hello.$(TARGET).signed -C $$bin_app_dir; \
		    diff -r -U2 "$$src_app_dir" "$$bin_app_dir"; \
		    rm -rf $$src_system_dir $$bin_system_dir $$src_app_dir $$bin_app_dir; \
		    echo "error: source and binary signed systems differ" >&2; \
		    exit 1; \
	fi; \
	fi
	# Providing a 'build' symlink as to keep sources at the same place where they have been
	# built, otherwise klocwork complains about missing files.
	ln -sf build-source $(BUILD_DIR)/build

source.$(TARGET):
	rm -rf $(BUILD_DIR)/build
	HELLO_APP=hello.adef \
	    mksys hello-system.sdef -t $(TARGET) -i $(LEGATO_ROOT)/interfaces/modemServices \
	        -w $(BUILD_DIR)/build -o $(BUILD_DIR)/source
	rm -rf $(BUILD_DIR)/build-source && mv $(BUILD_DIR)/build $(BUILD_DIR)/build-source

binary.$(TARGET): app.$(TARGET)
	rm -rf $(BUILD_DIR)/build
	HELLO_APP=$(BUILD_DIR)/app/hello.$(TARGET).app \
	    mksys hello-system.sdef -t $(TARGET) -i $(LEGATO_ROOT)/interfaces/modemServices \
	        -w $(BUILD_DIR)/build -o $(BUILD_DIR)/binary
	rm -rf $(BUILD_DIR)/build-binary && mv $(BUILD_DIR)/build $(BUILD_DIR)/build-binary
	#Check if IMA build is enabled, if yes then try to build with signed binary app.
	if [ $(LE_CONFIG_ENABLE_IMA) -eq y ] ; then \
		rm -rf $(BUILD_DIR)/build; \
		HELLO_APP=$(BUILD_DIR)/app/hello.$(TARGET).signed.app \
			mksys hello-system.sdef -t $(TARGET) -i $(LEGATO_ROOT)/interfaces/modemServices \
				-w $(BUILD_DIR)/build -o $(BUILD_DIR)/binary && \
		rm -rf $(BUILD_DIR)/build-binary && mv $(BUILD_DIR)/build $(BUILD_DIR)/build-binary; \
	fi

app.$(TARGET):
	rm -rf $(BUILD_DIR)/build
	mkapp hello.adef -b -t $(TARGET) -i $(LEGATO_ROOT)/interfaces/modemServices \
	        -w $(BUILD_DIR)/build -o $(BUILD_DIR)/app
	rm -rf $(BUILD_DIR)/build-app && mv $(BUILD_DIR)/build $(BUILD_DIR)/build-app
