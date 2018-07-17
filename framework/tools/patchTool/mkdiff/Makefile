# --------------------------------------------------------------------------------------------------
# Makefile used to build the tool to make patches
#
# Copyright (C) Sierra Wireless Inc.
# --------------------------------------------------------------------------------------------------

include $(LEGATO_ROOT)/utils.mk

TARGET ?= localhost

MKDIFF_SRC = mkdiff.c $(LEGATO_ROOT)/framework/tools/patchTool/patch_utils.c \
             $(LEGATO_ROOT)/framework/liblegato/crc.c
$(LEGATO_ROOT)/bin/mkdiff: $(MKDIFF_SRC)
	$(L) CCLD $@
	$(Q)$(CCACHE) $(CC) -Wall -Werror -o $@ $(MKDIFF_SRC) \
		-I$(LEGATO_ROOT)/framework/include \
		-I$(LEGATO_ROOT)/3rdParty/include \
		-I$(LEGATO_ROOT)/framework/tools/patchTool \
		-I$(LEGATO_ROOT)/build/$(TARGET)/framework/include
