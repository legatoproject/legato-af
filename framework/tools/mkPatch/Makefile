# --------------------------------------------------------------------------------------------------
# Makefile used to build the tool to make patches
#
# Copyright (C) Sierra Wireless Inc.
# --------------------------------------------------------------------------------------------------

# ========== GENERIC BUILD RULES ============

ifeq ($(USE_CLANG),1)
  export CC := $(shell which clang)
else
  export CC := $(shell which gcc)
endif

ifeq (${USE_CCACHE},1)
  export CC := ${CCACHE} ${CC}
endif

# Tell make that the targets are not actual files.
.PHONY: mkPatch

MKPATCH_SRC = mkPatch.c $(LEGATO_ROOT)/framework/liblegato/crc.c

mkPatch: $(MKPATCH_SRC)
	$(CC) -Wall -Werror -o $(LEGATO_ROOT)/bin/$@ \
	    $(MKPATCH_SRC) \
	    -I$(LEGATO_ROOT)/framework/include \
	    -I$(LEGATO_ROOT)/3rdParty/include
