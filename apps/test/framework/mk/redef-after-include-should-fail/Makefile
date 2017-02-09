include ../common.mk

$(TARGET):
	mksys system.sdef -t $@ -v -w $(BUILD_DIR) -o $(BUILD_DIR) 2>&1 \
	    |grep -q "it has already been used in a processing directive." \
            && echo "Correct error message caught."
