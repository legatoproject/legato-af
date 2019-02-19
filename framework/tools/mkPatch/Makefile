# --------------------------------------------------------------------------------------------------
# Makefile used to build the tool to make patches
#
# Copyright (C) Sierra Wireless Inc.
# --------------------------------------------------------------------------------------------------

include $(LEGATO_ROOT)/utils.mk

TARGET ?= localhost

HOST_CFLAGS = -Wall -Werror
ifeq ($(shell test $(GCC_VERSION) -ge 80000 && echo 1), 1)
  HOST_CFLAGS += -Wno-format-truncation
endif

MKPATCH_SRC = mkPatch.c $(LEGATO_ROOT)/framework/liblegato/crc.c
$(LEGATO_ROOT)/bin/mkPatch: $(MKPATCH_SRC)
	$(L) CCLD $@
	$(Q)$(CCACHE) $(CC) \
		$(HOST_CFLAGS) \
		-o $@ $(MKPATCH_SRC) \
		-I$(LEGATO_ROOT)/framework/include \
		-I$(LEGATO_ROOT)/3rdParty/include \
		-I$(LEGATO_ROOT)/build/$(TARGET)/framework/include
