TARGETS := wp85 wp750x wp76xx wp77xx

# List of "utility" recipies
UTILITIES := clean

# Determine the target platform
TARGET := $(filter $(TARGETS),$(MAKECMDGOALS) $(TARGET))

ifeq ($(TARGET),)
  TARGET := $(filter $(UTILITIES),$(MAKECMDGOALS))
  ifeq ($TARGET),)
     TARGET := nothing
     endif
endif

all: $(TARGETS)

# Makefile include generated from KConfig values
MAKE_CONFIG := $(LEGATO_ROOT)/build/$(TARGET)/.config.mk

# Include target-specific configuration values
ifneq ($(TARGET), clean)
  include $(MAKE_CONFIG)
endif

$(TARGETS):
	mksys -t $(TARGET) helloRpc-Alice.sdef
	mksys -t $(TARGET) helloRpc-Bob.sdef

clean:
	rm -rf _build_* *.*.update
