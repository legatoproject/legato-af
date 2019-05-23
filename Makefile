# --------------------------------------------------------------------------------------------------
# Makefile used to build the Legato framework.
#
# See the TARGETS variable definition for a list of supported build targets.  Each of these
# is a valid goal for this Makefile.  For example, to build for the Sierra Wireless AR7xxx family
# of modules, run "make ar7".
#
# Building a target will build the following:
#  - host tools (needed to build the framework for the target)
#  - liblegato
#  - framework daemons and libs they require
#  - on-target tools
#  - a basic working system (see system.sdef)
#
# To build tests for a target X, build tests_X (e.g., 'make tests_wp85').
#
# To build samples for a target X, build samples_X (e.g., 'make samples_wp85').
#
# To build everything for a target X, build all_X (e.g., 'make all_wp85').
#
# Other goals supported by this Makefile are:
#
#    clean = delete all build output.
#
#    distclean = clean + remove build configuration files (.config.<target>).
#
#    docs = build the documentation.
#
#    tools = just build the host tools.
#
#    sdk = build and package a "software development kit" containing the build tools.
#
# To get more details from the build as it progresses, run make with "V=1".
#
# Copyright (C) Sierra Wireless Inc.
# --------------------------------------------------------------------------------------------------

# List of target devices supported:
TARGETS := localhost ar7 ar758x ar759x ar86 wp85 wp750x wp76xx wp77xx raspi virt virt-x86 virt-arm

# Define the LEGATO_ROOT environment variable.
export LEGATO_ROOT := $(CURDIR)

# Add the framework's bin directory to the PATH environment variable.
export PATH := $(PATH):$(LEGATO_ROOT)/bin

# ========== TARGET DETERMINATION ============

THIS_FILE := $(lastword $(MAKEFILE_LIST))
.DEFAULT_GOAL := default
.PHONY: list

# Command to list recipies
LISTCMD := $(MAKE) -qp -f $(THIS_FILE) list 2> /dev/null | \
  awk -F: '/^[a-zA-Z0-9/][a-zA-Z0-9._/]*:([^=]|$$)/ {split($$1,A,/ /); for(i in A) print A[i]}'

# List of "utility" recipies
UTILITIES := list clean distclean

# Determine the target platform
TARGET ?=
ifeq ($(MAKECMDGOALS),)
  TARGET := localhost
  KNOWN_TARGET := 1
endif
ifeq ($(TARGET),)
  ifeq ($(filter-out $(UTILITIES),$(MAKECMDGOALS)),)
        TARGET := nothing
  endif
endif

ifeq ($(TARGET),)
  # Look for "<name>_<target>" style goals
  COMPOUND_TARGET := $(filter $(TARGETS:%=\%_%),$(MAKECMDGOALS))
  ifneq ($(COMPOUND_TARGET),)
    PARTS := $(subst _, ,$(COMPOUND_TARGET))
    TARGET := $(lastword $(PARTS))
  endif # end have compound target

  TARGET := $(filter $(TARGETS),$(MAKECMDGOALS) $(TARGET))
  ifeq ($(TARGET),)
    ALL_RULES := $(shell $(LISTCMD)) menuconfig_%
    TARGET := $(filter-out $(ALL_RULES),$(MAKECMDGOALS))
    ifeq ($(TARGET)$(filter-out menuconfig_%,$(MAKECMDGOALS)),)
      TARGET := $(subst menuconfig_,,$(MAKECMDGOALS))
    endif # end no target
    ifeq ($(TARGET),)
      TARGET := localhost
      KNOWN_TARGET := 1
    endif # end no target
  else # have target
    KNOWN_TARGET := 1
  endif # end have target

  VIRT_TARGET_ARCH ?= x86
  ifeq ($(TARGET),virt)
    TARGET := virt-$(VIRT_TARGET_ARCH)
    export VIRT_TARGET_ARCH
    $(warning DEPRECATED: The 'virt' target will be removed in a future release. Please use \
      virt-<arch> instead.)
  endif # end target is "virt"
endif # end no target
export TARGET
TARGET_CAPS := $(shell echo $(TARGET) | tr a-z- A-Z_)
ifneq ($(TARGET),nothing)
  $(info Building Legato for target '$(TARGET)')
endif

# KConfig settings location.
export LEGATO_KCONFIG ?= $(LEGATO_ROOT)/.config.$(TARGET)

# Makefile include generated from KConfig values
MAKE_CONFIG := build/$(TARGET)/.config.mk

# Configuration header
HEADER_CONFIG := build/$(TARGET)/framework/include/le_config.h

# Configuration environment script
SHELL_CONFIG := build/$(TARGET)/config.sh

# Doxygen definition from KConfig
DOXYGEN_DEFS_FILE := doxygen.Kconfig.cfg
DOXYGEN_DEFS := $(LEGATO_ROOT)/build/doc/$(DOXYGEN_DEFS_FILE)

# Include target-specific configuration values
ifneq ($(TARGET),nothing)
  include $(MAKE_CONFIG)
endif

include utils.mk

# Host architecture
export HOST_ARCH := $(shell uname -m)
export TOOLS_ARCH ?= $(HOST_ARCH)

# Toolchain finding script
FINDTOOLCHAIN := framework/tools/scripts/findtoolchain

# Load module definitions
include $(wildcard modules/*/moduleDefs)

# Read-only setting
STAGE_SYSTOIMG = stage_systoimg
ifeq ($(LE_CONFIG_READ_ONLY),y)
  override STAGE_SYSTOIMG := stage_systoimgro
endif

export MKTOOLS_FLAGS
export MKSYS_FLAGS=$(MKTOOLS_FLAGS)
export MKAPP_FLAGS=$(MKTOOLS_FLAGS)
export MKEXE_FLAGS=$(MKTOOLS_FLAGS)

# If set, generate an image with stripped binaries
ifeq ($(LE_CONFIG_STRIP_STAGING_TREE),y)
  SYSTOIMG_FLAGS += -s
  MKTOOLS_FLAGS += -d $(LEGATO_ROOT)/build/$(TARGET)/debug
endif

# Disable SMACK in image creation, if appropriate
ifneq ($(LE_CONFIG_ENABLE_SMACK),y)
  SYSTOIMG_FLAGS += --disable-smack
endif

# Build AV model for appropriate targets
ifeq ($(LE_CONFIG_AVC_ENABLE_AV_MODEL),y)
  STAGE_MKAVMODEL := stage_mkavmodel
endif

# Target architecture for tests
ifeq ($(TARGET),localhost)
  export LEGATO_TARGET_ARCH := $(shell uname -m)
endif

# ========== BUILD PARAMETER/TOOL SELECTION ============

# Select Clang or GCC
ifeq ($(filter 1,$(or $(call k2b,$(LE_CONFIG_USE_CLANG)),$(USE_CLANG))),)
  CC_NAME = gcc
  CXX_NAME = g++
  CPP_NAME = cpp
else
  CC_NAME = clang
  CXX_NAME = clang++
  CPP_NAME = cpp
endif
export USE_CLANG := $(call k2b,$(LE_CONFIG_USE_CLANG))

# Host compilers
export HOST_CC ?= $(CC_NAME)
export HOST_CXX ?= $(CXX_NAME)

# Use ccache by default if available
ifeq ($(LE_CONFIG_USE_CCACHE),y)
  ifeq ($(shell which ccache 2>/dev/null),)
    LE_CONFIG_USE_CCACHE :=
    unexport LE_CONFIG_USE_CCACHE
  endif
endif
ifeq ($(LE_CONFIG_USE_CCACHE),y)
  # Unset CCACHE_PATH as to not interfere with host builds
  unexport CCACHE_PATH

  ifeq ($(LE_CONFIG_CCACHE),)
    CCACHE := $(shell which ccache 2>/dev/null)
  else
    CCACHE := $(LE_CONFIG_CCACHE)
  endif
  ifeq ($(CCACHE),)
    $(error "Unable to find ccache while it is enabled.")
  endif
  export CCACHE
else
  CCACHE :=
  unexport CCACHE
endif

ifneq ($(TARGET),nothing)
  ifeq ($(TARGET),localhost)
    export LEGATO_KERNELROOT    :=
    export LEGATO_SYSROOT       :=
    export TOOLCHAIN_DIR        := $(dir $(shell which $(CC_NAME)))
    export TOOLCHAIN_PREFIX     :=
  else # not localhost
    ifeq ($(TOOLCHAIN_DIR),)
      export LEGATO_KERNELROOT    := $(shell $(FINDTOOLCHAIN) $(TARGET) kernelroot)
      export LEGATO_SYSROOT       := $(shell $(FINDTOOLCHAIN) $(TARGET) sysroot)
      export TOOLCHAIN_DIR        := $(shell $(FINDTOOLCHAIN) $(TARGET) dir)
      export TOOLCHAIN_PREFIX     := $(shell $(FINDTOOLCHAIN) $(TARGET) prefix)
    endif
  endif # end not localhost

  # Target compiler variables
  export TARGET_CC                       ?= $(TOOLCHAIN_DIR)/$(TOOLCHAIN_PREFIX)$(CC_NAME)
  export TARGET_CXX                      ?= $(TOOLCHAIN_DIR)/$(TOOLCHAIN_PREFIX)$(CXX_NAME)
  export TARGET_CPP                      ?= $(TOOLCHAIN_DIR)/$(TOOLCHAIN_PREFIX)$(CPP_NAME)
  export $(TARGET_CAPS)_CC               := $(TARGET_CC)
  export $(TARGET_CAPS)_CXX              := $(TARGET_CXX)
  export $(TARGET_CAPS)_CPP              := $(TARGET_CPP)
  export $(TARGET_CAPS)_TOOLCHAIN_DIR    := $(TOOLCHAIN_DIR)
  export $(TARGET_CAPS)_TOOLCHAIN_PREFIX := $(TOOLCHAIN_PREFIX)
  export $(TARGET_CAPS)_SYSROOT          := $(LEGATO_SYSROOT)
  export $(TARGET_CAPS)_KERNELROOT       := $(LEGATO_KERNELROOT)

  ifeq ($(TARGET_CC),/$(CC_NAME))
    $(error Unable to find toolchain for target '$(TARGET)')
  endif

  # Set the LD, AR, AS, STRIP, OBJCOPY, and READELF variables for use by the Legato framework
  # build
  TOOLCHAIN_PATH_PREFIX := $(TOOLCHAIN_PREFIX)
  ifneq ($(TOOLCHAIN_DIR),)
    ifneq ($(TOOLCHAIN_PREFIX),)
      TOOLCHAIN_PATH_PREFIX := $(TOOLCHAIN_DIR)/$(TOOLCHAIN_PREFIX)
    else
      TOOLCHAIN_PATH_PREFIX := $(TOOLCHAIN_DIR)/
    endif
  endif
  prefixtool = $(if $(filter-out $(origin TARGET_$(1)),undefined),$(TARGET_$(1)),\
                 $(if $(wildcard $(2)$(3)),$(2)$(3),$(3)))

  ifeq ($(LE_CONFIG_CONFIGURED),y)
    export ADDR2LINE := $(call prefixtool,ADDR2LINE,$(TOOLCHAIN_PATH_PREFIX),addr2line)
    export AR        := $(call prefixtool,AR,$(TOOLCHAIN_PATH_PREFIX),ar)
    export AS        := $(call prefixtool,AS,$(TOOLCHAIN_PATH_PREFIX),as)
    export DB        := $(call prefixtool,DB,$(TOOLCHAIN_PATH_PREFIX),gdb)
    export ELFEDIT   := $(call prefixtool,ELFEDIT,$(TOOLCHAIN_PATH_PREFIX),elfedit)
    export GCOV      := $(call prefixtool,GCOV,$(TOOLCHAIN_PATH_PREFIX),gcov)
    export GCOV_DUMP := $(call prefixtool,GCOV_DUMP,$(TOOLCHAIN_PATH_PREFIX),gcov-dump)
    export GCOV_TOOL := $(call prefixtool,GCOV_TOOL,$(TOOLCHAIN_PATH_PREFIX),gcov-tool)
    export GPROF     := $(call prefixtool,GPROF,$(TOOLCHAIN_PATH_PREFIX),gprof)
    export LD        := $(call prefixtool,LD,$(TOOLCHAIN_PATH_PREFIX),ld)
    export NM        := $(call prefixtool,NM,$(TOOLCHAIN_PATH_PREFIX),nm)
    export OBJCOPY   := $(call prefixtool,OBJCOPY,$(TOOLCHAIN_PATH_PREFIX),objcopy)
    export OBJDUMP   := $(call prefixtool,OBJDUMP,$(TOOLCHAIN_PATH_PREFIX),objdump)
    export RANLIB    := $(call prefixtool,RANLIB,$(TOOLCHAIN_PATH_PREFIX),ranlib)
    export READELF   := $(call prefixtool,READELF,$(TOOLCHAIN_PATH_PREFIX),readelf)
    export SIZE      := $(call prefixtool,SIZE,$(TOOLCHAIN_PATH_PREFIX),size)
    export STRINGS   := $(call prefixtool,STRINGS,$(TOOLCHAIN_PATH_PREFIX),strings)
    export STRIP     := $(call prefixtool,STRIP,$(TOOLCHAIN_PATH_PREFIX),strip)
  endif # end LE_CONFIG_CONFIGURED
endif # end not "nothing" target

# Python executable
PYTHON_EXECUTABLE ?= python2.7

# KConfig executables
MENUCONFIG_TOOL   ?= python3 $(LEGATO_ROOT)/3rdParty/Kconfiglib/menuconfig.py
SETCONFIG_TOOL    ?= python3 $(LEGATO_ROOT)/3rdParty/Kconfiglib/setconfig.py
OLDDEFCONFIG_TOOL ?= python3 $(LEGATO_ROOT)/3rdParty/Kconfiglib/olddefconfig.py

# Set KConfig prefix
export CONFIG_    := LE_CONFIG_

# Set path to find KConfig functions.
export PYTHONPATH := .

# Set non-debug flags
ifeq ($(LE_CONFIG_DEBUG),y)
  # If not stripping the staging tree, add debug for the debug build.
  # If stripping the staging tree this isn't needed as debug is always generated
  # as a separate file on the host.
  ifneq ($(LE_CONFIG_STRIP_STAGING_TREE),y)
      MKTOOLS_FLAGS += --cflags="-g"
  endif
else
  # Optimize release builds
  MKTOOLS_FLAGS += --cflags="-O2 -fno-omit-frame-pointer"
endif

# Set flags for test coverage
ifeq ($(LE_CONFIG_TEST_COVERAGE),y)
  MKSYS_FLAGS += --cflags=--coverage --ldflags=--coverage

  ifneq ($(LE_CONFIG_TEST_COVERAGE_DIR),)
    MKSYS_FLAGS += --cflags=-fprofile-dir=$(LE_CONFIG_TEST_COVERAGE_DIR)
  endif

  export TEST_COVERAGE := 1
  export TEST_COVERAGE_DIR := $(LE_CONFIG_TEST_COVERAGE_DIR)
endif

# PlantUML file path
PLANTUML_PATH ?= $(LEGATO_ROOT)/3rdParty/plantuml

# PlantUML file definition
export PLANTUML_JAR_FILE := $(PLANTUML_PATH)/plantuml.jar

# Directory containing the .sdef
export LEGATO_SDEF_ROOT := $(dir $(abspath $(LE_CONFIG_SDEF)))

# Version related rules
ifeq ($(LEGATO_VERSION),)
  export LEGATO_VERSION := $(shell git describe --tags 2> /dev/null)

  ifeq ($(LEGATO_VERSION),)
    export LEGATO_VERSION := $(shell cat version 2> /dev/null)

    # If we still cannot determine the legato version, set it to unknown
    ifeq ($(LEGATO_VERSION),)
      $(warning Unable to determine Legato version)
      export LEGATO_VERSION := "unknown"
    endif
  endif
endif

ifeq ($(LE_CONFIG_ENABLE_IMA),y)
  MKSYS_FLAGS += -S
endif

# Source code directories and files to include in the MD5 sum in the version and package.properties.
FRAMEWORK_SOURCES = framework/                    \
                    components/                   \
                    interfaces/                   \
                    platformAdaptor/              \
                    modules/                      \
                    apps/platformServices/        \
                    $(wildcard apps/proprietary/) \
                    apps/tools/                   \
                    targetFiles/                  \
                    $(wildcard Makefile*)         \
                    CMakeLists.txt

# Generator for making Legato systems
define sysmk
	$(L) MKSYS $(2)
	$(Q)mksys -t $(TARGET) -w $(1) -o build/$(TARGET) $(2) $(3) $(MKSYS_FLAGS)
endef

# Test and sample selection
ALL_TESTS_y   = mktools_tests subsys_tests tests_c
ALL_SAMPLES_y = samples_c

ALL_TESTS_$(LE_CONFIG_JAVA)   += tests_java
ALL_SAMPLES_$(LE_CONFIG_JAVA) += samples_java

# ========== CONFIGURATION RECIPES ============

# Generate an initial KConfig from the environment.  This rule translates the old configuration
# method using environment variables into an initial KConfig set.
$(LEGATO_ROOT)/.config.$(TARGET): $(shell find . -name 'KConfig' -o -name '*.kconfig')
ifeq ($(KNOWN_TARGET),1)
	$(L) KSET "$@ - TARGET_$(TARGET_CAPS)"
	$(Q)KCONFIG_CONFIG=$@ $(SETCONFIG_TOOL) --kconfig=KConfig "TARGET_$(TARGET_CAPS)=y" $(VOUTPUT)
else
	$(L) KSET "$@ - TARGET_CUSTOM"
	$(Q)KCONFIG_CONFIG=$@ $(SETCONFIG_TOOL) --kconfig=KConfig "TARGET_CUSTOM=y" $(VOUTPUT)
endif

	$(L) KCONFIG $@
	$(Q)KCONFIG_CONFIG=$@ $(OLDDEFCONFIG_TOOL) KConfig $(VOUTPUT)
	$(Q)rm -f $@.old

# Generate the Makefile include containing the KConfig values
$(MAKE_CONFIG): $(LEGATO_KCONFIG)
	$(Q)cat $< $(VOUTPUT)
	$(L) GEN $@
	$(Q)mkdir -p $(dir $@)
	$(Q)sed -e 's/^LE_CONFIG_/export &/g' \
		-e 's/="/=/g' \
		-e 's/"$$//g' \
		-e 's/=/ := /g' $< > $@
ifneq ($(KNOWN_TARGET),1)
	$(Q)printf '\n# Additional Definitions\nexport LE_CONFIG_TARGET_%s := y\n' "$(TARGET_CAPS)" \
		>> $@
endif

# Generate a shell include file containing the KConfig values
$(SHELL_CONFIG): $(LEGATO_KCONFIG)
	$(L) GEN $@
	$(Q)mkdir -p $(dir $@)
	$(Q)sed -e 's/^LE_CONFIG_/export &/g' $< > $@
ifneq ($(KNOWN_TARGET),1)
	$(Q)printf '\n# Additional Definitions\nexport LE_CONFIG_TARGET_%s=y\n' "$(TARGET_CAPS)" >> $@
endif

# Generate doxygen configuration containing the KConfig definitions
$(DOXYGEN_DEFS): $(LEGATO_KCONFIG)
	$(L) GEN $@
	$(Q)mkdir -p $(dir $@)
	$(Q)sed -e 's/^LE_CONFIG_/PREDEFINED += &/g' -e 's/=y/=1/g' $< > $@

# Interactively select build options
.PHONY: menuconfig
menuconfig: $(LEGATO_KCONFIG)
	$(L) KCONFIG $(LEGATO_KCONFIG)
	$(Q)KCONFIG_CONFIG=$(LEGATO_KCONFIG) $(MENUCONFIG_TOOL) KConfig

# Interactively select build options for a specific target
.PHONY: menuconfig_$(TARGET)
menuconfig_$(TARGET): menuconfig

# ========== TOOLS RECIPES ============

# Rule for how to build the build tools.
.PHONY: tools
# Use HOST_CC and HOST_CXX when building the tools.
tools: export CC := $(HOST_CC)
tools: export CXX := $(HOST_CXX)
tools: version $(HEADER_CONFIG)
	$(L) MAKE $@
	$(Q)$(MAKE) -f Makefile.hostTools

# Rule for how to extract the build tool messages
.PHONY: tool-messages
tool-messages: version
	$(MAKE) -f Makefile.hostTools tool-messages

# Rule to create a "software development kit" from the tools.
.PHONY: sdk
sdk: tools
	createsdk

# ========== MAIN RECIPES ============

# By default, build for the localhost and build the documentation.
.PHONY: default
default:
	$(Q)$(MAKE) -f $(THIS_FILE) docs

# Cleaning rule
.PHONY: clean
clean:
	@echo "Cleaning..."
	$(L) CLEAN ""
	$(Q)rm -Rf build Documentation* bin doxygen.*.log doxygen.*.err
	$(Q)rm -f framework/doc/toolsHost.dox framework/doc/toolsHost_*.dox
	$(Q)rm -f sources.md5

# Clean configuration too
.PHONY: distclean
distclean: clean
	$(Q)rm -f .config*

# Error for missing platform adaptors
platformAdaptor modules:
	@echo -e "\033[1;31m'$@' directory is missing, which means these Legato sources have not been downloaded properly."
	@echo -e "Please refer to https://github.com/legatoproject/legato-af#clone-from-github \033[0m"
	@exit 1

# Generate an MD5 hash of everything in the source directories.
.PHONY: sources.md5
sources.md5: $(FRAMEWORK_SOURCES)
	$(L) GEN $@
	$(Q)find $(FRAMEWORK_SOURCES) -type f | grep -v ".git" | sort | while read filePath ; \
	do \
	  echo "$$filePath" && \
	  cat "$$filePath" ; \
	done | md5sum | awk '{ print $$1 }' > sources.md5

# Produce a file containing the Legato version
.PHONY: version
version:
	$(L) GEN $@
	$(Q)if [ -n "$(LEGATO_VERSION)" ] ; then \
		echo "$(LEGATO_VERSION)" > $@  ; \
	elif ! [ -e version ] ; then \
		echo "unknown" > $@ ; \
	fi

# Generate package properties
package.properties: version sources.md5
	$(L) GEN $@
	$(Q)echo "version=`cat version`" > $@
	$(Q)echo "md5=`cat sources.md5`" >> $@

# Header containing all of the KConfig parameters
$(HEADER_CONFIG): $(LEGATO_KCONFIG) Makefile
	$(L) GEN $@
	$(Q)mkdir -p $(dir $@)
	$(Q)printf '#ifndef LEGATO_CONFIG_INCLUDE_GUARD\n' > $@
	$(Q)printf '#define LEGATO_CONFIG_INCLUDE_GUARD\n' >> $@
	$(Q)sed -e 's:^#://:' \
		-e 's/^LE_CONFIG_/#define &/' \
		-e 's/=y$$/ 1/' \
		-e 's/=n$$/ 0/' \
		-e 's/=m$$/ LE_MODULE/' \
		-e 's/=/ /' $< >> $@
	$(Q)printf '\n//\n// Additional Definitions\n//\n' >> $@
	$(Q)printf '#define LE_VERSION "%s"\n' "$(LEGATO_VERSION)" >> $@
	$(Q)printf '#define LE_TARGET "%s"\n' "$(TARGET)" >> $@
ifneq ($(KNOWN_TARGET),1)
	$(Q)printf '#define LE_CONFIG_TARGET_%s 1\n' "$(TARGET_CAPS)" >> $@
endif
	$(Q)printf '#define LE_SVCDIR_SERVER_SOCKET_NAME \\\n' >> $@
	$(Q)printf '    LE_CONFIG_RUNTIME_DIR "/" LE_CONFIG_SVCDIR_SERVER_SOCKET_NAME\n' >> $@
	$(Q)printf '#define LE_SVCDIR_CLIENT_SOCKET_NAME \\\n' >> $@
	$(Q)printf '    LE_CONFIG_RUNTIME_DIR "/" LE_CONFIG_SVCDIR_CLIENT_SOCKET_NAME\n' >> $@
	$(Q)printf '\n#endif /* end LEGATO_CONFIG_INCLUDE_GUARD */\n' >> $@

# Rule building the framework for a given target
.PHONY: framework framework_$(TARGET)
framework_$(TARGET): framework
framework: tools package.properties $(HEADER_CONFIG) $(SHELL_CONFIG)
	$(L) MAKE $@
	$(Q)$(MAKE) -f Makefile.framework CC=$(TARGET_CC)

.PHONY: system system_$(TARGET)
system_$(TARGET): system
system: framework
	$(call sysmk,build/$(TARGET)/system,$(LE_CONFIG_SDEF))
	$(Q)mv build/$(TARGET)/$(notdir $(LE_CONFIG_SDEF:%.sdef=%)).$(TARGET).update \
		build/$(TARGET)/system.$(TARGET).update
	$(Q)if [ "$(LE_CONFIG_ENABLE_IMA)" = y ]; then \
		mv build/$(TARGET)/$(notdir $(LE_CONFIG_SDEF:%.sdef=%)).$(TARGET).signed.update \
			build/$(TARGET)/system.$(TARGET).signed.update ; \
	fi

# Trigger the build for a specific target
ifneq ($(TARGET),nothing)
.PHONY: $(TARGET)
$(TARGET): system stage

# NOTE: In the "virt" cases, the target will always be one of virt-arm or virt-x86.
.PHONY: virt all_$(TARGET) all_virt all_c_$(TARGET)
virt: all_virt
all_$(TARGET): tests samples
all_virt: all_$(TARGET)
all_c_$(TARGET): tests_c samples_c

.PHONY: tests_$(TARGET) tests_virt tests_c_$(TARGET)
tests_$(TARGET): tests
tests_virt: tests_$(TARGET)
tests_c_$(TARGET): tests_c

.PHONY: samples_$(TARGET) samples_virt samples_c_$(TARGET)
samples_$(TARGET): samples
samples_virt: samples_$(TARGET)
samples_c_$(TARGET): samples_c

ifeq ($(LE_CONFIG_JAVA),y)
.PHONY: all_java_$(TARGET) tests_java_$(TARGET) samples_java_$(TARGET)
all_java_$(TARGET): tests_java samples_java
tests_java_$(TARGET): tests_java
samples_java_$(TARGET): samples_java
endif # end LE_CONFIG_JAVA
endif # end target not "nothing"

# ========== STAGING/IMAGING RECIPES ============

# Build a read/write system image
.PHONY: stage_systoimg
stage_systoimg:
	$(L) IMAGE build/$(TARGET)/system.$(TARGET).update
	$(Q)systoimg $(SYSTOIMG_FLAGS) $(TARGET) build/$(TARGET)/system.$(TARGET).update build/$(TARGET)
	@# Check PA libraries.
	$(Q)checkpa $(TARGET) || true
	@# Link legato R/W images to default legato images
	$(Q)(cd build/$(TARGET); \
	    for f in legato.*; do \
	        ln -sf $$f `echo $$f | sed 's/legato/legato_rw/'`; \
	    done)

# Build a read-only system image
.PHONY: stage_systoimgro
stage_systoimgro: stage_systoimg
	$(L) ROIMAGE build/$(TARGET)/system.$(TARGET).update
	$(Q)systoimg $(SYSTOIMG_FLAGS) -S _ro --read-only -a \
	    $(TARGET) build/$(TARGET)/system.$(TARGET).update build/$(TARGET)

# Generate AirVantage application model
.PHONY: stage_mkavmodel
stage_mkavmodel:
	$(L) GEN build/$(TARGET)/model.app
	$(Q)if [ -f "apps/platformServices/airVantageConnector/tools/scripts/mkavmodel" ] ; then \
		cp "apps/platformServices/airVantageConnector/tools/scripts/mkavmodel" bin/ ; \
		mkavmodel -t $(TARGET) -o build/$(TARGET) -v $(LEGATO_VERSION) ; \
	fi

.PHONY: stage
stage: $(STAGE_SYSTOIMG) $(STAGE_MKAVMODEL)

# ========== DOCUMENTATION RECIPES ============

# Goal for building all documentation
.PHONY: docs user_docs implementation_docs
docs: $(PLANTUML_JAR_FILE) $(DOXYGEN_DEFS) user_docs implementation_docs

# Docs for people who don't want to be distracted by the internal implementation details.
user_docs: localhost $(PLANTUML_JAR_FILE) $(DOXYGEN_DEFS) build/localhost/Makefile
	$(L) MAKE Documentation
	$(Q)$(MAKE) -C build/localhost user_docs
	$(Q)rm -f Documentation
	$(Q)if [ -e "docManagement/Makefile" ] ; then \
		$(MAKE) -C docManagement ; \
	fi
	$(Q)if [ -d "build/doc/user/html_converted/" ] ; then \
		ln -sf build/doc/user/html_converted Documentation ; \
	else \
		ln -sf build/doc/user/html Documentation ; \
	fi

# Generate PDF of documentation
user_pdf: localhost $(DOXYGEN_DEFS) build/localhost/Makefile
	$(L) MAKE Documentation.pdf
	$(Q)$(MAKE) -C build/localhost user_pdf
	$(Q)ln -sf build/localhost/bin/doc/user/legato-user.pdf Documentation.pdf

# Generate UML diagrams for documentaton
plantuml_docs: $(PLANTUML_JAR_FILE)
	$(L) MAKE $@
	$(Q)for dir in components/doc platformAdaptor/qmi/src/components/doc; do \
		files=`ls $(LEGATO_ROOT)/$$dir/*` ; \
		java -Djava.awt.headless=true -jar $(PLANTUML_JAR_FILE) \
		                              -o $(LEGATO_ROOT)/build/doc/implementation/html $$files ; \
	done

# Docs for people who want or need to know the internal implementation details.
implementation_docs: localhost plantuml_docs build/localhost/Makefile
	$(L) MAKE $@
	$(Q)$(MAKE) -C build/localhost $@

# Generate PDF of implmentation docs
implementation_pdf: localhost build/localhost/Makefile
	$(L) MAKE $@
	$(Q)$(MAKE) -C build/localhost $@

# Rule building the unit test covergage report
coverage_report:
	$(L) MAKE $@
	$(Q)$(MAKE) -C build/localhost $@

# ========== TEST RECIPES ============

# Rule for invoking CMake to generate the Makefiles inside the build directory.
# Depends on the build directory being there.
# NOTE: CMake is only used to build tests and samples.
build/$(TARGET)/Makefile:
	export PATH=$(TOOLCHAIN_DIR):$(PATH) && cd `dirname $@` && \
		cmake ../.. \
			-DLEGATO_ROOT=$(LEGATO_ROOT) \
			-DLEGATO_TARGET=$(TARGET) \
			-DLEGATO_JOBS=$(LEGATO_JOBS) \
			-DPLANTUML_JAR_FILE=$(PLANTUML_JAR_FILE) \
			-DDOXYGEN_DEFS_FILE=$(DOXYGEN_DEFS_FILE) \
			-DPA_DIR=$(LEGATO_ROOT)/platformAdaptor \
			-DTEST_COVERAGE=$(call k2b,$(LE_CONFIG_TEST_COVERAGE)) \
			-DINCLUDE_ECALL=$(call k2b,$(LE_CONFIG_ENABLE_ECALL)) \
			-DUSE_CLANG=$(call k2b,$(LE_CONFIG_USE_CLANG)) \
			-DPLATFORM_SIMULATION=$(PLATFORM_SIMULATION) \
			-DTOOLCHAIN_PREFIX=$(TOOLCHAIN_PREFIX) \
			-DTOOLCHAIN_DIR=$(TOOLCHAIN_DIR) \
			-DCMAKE_TOOLCHAIN_FILE=$(LEGATO_ROOT)/cmake/toolchain.yocto.cmake

# Rule building the C tests for a given target
.PHONY: tests_c
tests_c: $(TARGET) build/$(TARGET)/Makefile subsys_tests
	$(L) MAKE $@
	$(Q)$(MAKE) -C build/$(TARGET) $@

# Subsystem and pytest-based unit tests
.PHONY: subsys_tests
subsys_tests: $(TARGET)
	$(call sysmk,build/$(TARGET)/testFramework,framework/test/testFramework.sdef,\
		-s $(LEGATO_ROOT)/components)
	$(call sysmk,build/$(TARGET)/testComponents,components/test/testComponents.sdef,\
		-s $(LEGATO_ROOT)/components)
	$(call sysmk,build/$(TARGET)/testApps,apps/test/testApps.sdef,-s $(LEGATO_ROOT)/components)

ifeq ($(LE_CONFIG_JAVA),y)
# Rule building the Java tests for a given target
.PHONY: tests_java
tests_java: $(TARGET) framework build/$(TARGET)/Makefile
	$(L) MAKE $@
	$(Q)$(MAKE) -C build/$(TARGET) $@
endif # end LE_CONFIG_JAVA

# Tests for mktools
.PHONY: mktools_tests
mktools_tests: $(TARGET)
	$(L) MAKE $@
	$(Q)$(MAKE) -C apps/test/framework/mk CC=$(TARGET_CC)

# Rule building the tests for a given target -- build both C and Java tests
.PHONY: tests
tests: $(ALL_TESTS_y)

# ========== SAMPLE RECIPES ============

# Rule building the C samples for a given target
.PHONY: samples_c
samples_c: $(TARGET) build/$(TARGET)/Makefile
	$(L) MAKE $@
	$(Q)$(MAKE) -C build/$(TARGET) $@

ifeq ($(LE_CONFIG_JAVA),y)
# Rule building the Java samples for a given target
.PHONY: samples_java
samples_java: $(TARGET) build/$(TARGET)/Makefile
	$(L) MAKE $@
	$(Q)$(MAKE) -C build/$(TARGET) $@
endif # end LE_CONFIG_JAVA

# Rule building the samples for a given target -- build both C and Java samples
.PHONY: samples
samples: $(TARGET) $(ALL_SAMPLES_y)
