# --------------------------------------------------------------------------------------------------
# Shared Makefile features for building the Legato framework and tools.
#
# Copyright (C) Sierra Wireless Inc.
# --------------------------------------------------------------------------------------------------

# Verbosity
L := @printf "  %-9s %s\n"
ifeq ($(shell whoami),jenkins)
  # Make it always verbose if building under Jenkins
  V := 1
endif
ifeq ($(or $(V),$(VERBOSE)),1)
  Q :=
  V := 1
  VERBOSE := 1
  MKSYS_FLAGS += -v
  MKEXE_FLAGS += -v
  MKAPP_FLAGS += -v
  NINJA_FLAGS += -v
else
  Q := @
  V := 0
  VERBOSE := 0
endif
export V
export VERBOSE

# Helper to suppress output in non-verbose mode
VOUTPUT := $(if $(filter 1,$(V)),,> /dev/null)

ifeq ($(LE_CONFIG_USE_CCACHE),y)
  ifeq ($(LEGATO_JOBS),)
    # If ccache is enabled, we can raise the number of concurrent
    # jobs as it likely to get objects from cache, therefore not
    # consuming much CPU.
    LEGATO_JOBS := $(shell echo $$((4 * $$(nproc))))
  endif # end LEGATO_JOBS
endif # end LE_CONFIG_USE_CCACHE

# Set flags for the number of concurrent jobs
ifneq ($(LEGATO_JOBS),)
  ifeq ($(V),1)
    $(info Job Count: $(LEGATO_JOBS))
  endif
  MKSYS_FLAGS += -j $(LEGATO_JOBS)
  MKEXE_FLAGS += -j $(LEGATO_JOBS)
  MKAPP_FLAGS += -j $(LEGATO_JOBS)
  NINJA_FLAGS += -j $(LEGATO_JOBS)
  export LEGATO_JOBS
endif

# Convert KConfig y/m/n to boolean 1/0
define k2b
$(if $(filter y,$(1)),1,$(if $(filter m,$(1)),1,0))
endef
