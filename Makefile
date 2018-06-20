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
#    docs = build the documentation.
#
#    tools = just build the host tools.
#
#    release = build and package a release.
#
#    sdk = build and package a "software development kit" containing the build tools.
#
# To enable coverage testing, run make with "TEST_COVERAGE=1" on the command-line.
#
# To get more details from the build as it progresses, run make with "VERBOSE=1".
#
# Targets to be built for release can be selected with RELEASE_TARGETS.
#
# Copyright (C) Sierra Wireless Inc.
# --------------------------------------------------------------------------------------------------

# List of target devices needed by the release process:
ifndef RELEASE_TARGETS
  RELEASE_TARGETS := ar7 ar758x ar759x ar86 wp85 wp750x wp76xx wp77xx
endif

# List of target devices supported:
TARGETS := localhost $(RELEASE_TARGETS) raspi virt

# By default, build for the localhost and build the documentation.
.PHONY: default
default:
	$(MAKE) localhost
	$(MAKE) docs

# Define the LEGATO_ROOT environment variable.
export LEGATO_ROOT := $(CURDIR)

# Add the framework's bin directory to the PATH environment variable.
export PATH := $(PATH):$(LEGATO_ROOT)/bin

# Some framework on-target runtime parameters.
export LEGATO_FRAMEWORK_NICE_LEVEL := -19
export LE_RUNTIME_DIR := /tmp/legato/
export LE_SVCDIR_SERVER_SOCKET_NAME := $(LE_RUNTIME_DIR)serviceDirectoryServer
export LE_SVCDIR_CLIENT_SOCKET_NAME := $(LE_RUNTIME_DIR)serviceDirectoryClient

# Do not use clang by default.
export USE_CLANG ?= 0

# Default eCall build to be ON
export INCLUDE_ECALL ?= 1

# Do not be verbose by default.
export VERBOSE ?= 0

# secStoreAdmin APIs disabled by default.
export SECSTOREADMIN ?= 0

# Do not enable IMA signing by default.
export ENABLE_IMA ?= 0

# In case of release, override parameters
ifeq ($(MAKECMDGOALS),release)
  # We never build for coverage testing when building a release.
  override TEST_COVERAGE := 0
endif
export TEST_COVERAGE

# PlantUML file path
PLANTUML_PATH ?= $(LEGATO_ROOT)/3rdParty/plantuml

# PlantUML file definition
export PLANTUML_JAR_FILE := $(PLANTUML_PATH)/plantuml.jar

# Use ccache by default if available
ifeq ($(USE_CCACHE),)
  ifneq ($(shell which ccache 2>/dev/null),)
    export USE_CCACHE = 1
  endif
endif
ifeq ($(USE_CCACHE),1)
  # Unset CCACHE_PATH as to not interfere with host builds
  unexport CCACHE_PATH

  ifeq ($(CCACHE),)
    CCACHE := $(shell which ccache 2>/dev/null)
  endif
  ifeq ($(CCACHE),)
    $(error "Unable to find ccache while it is enabled.")
  endif
  export CCACHE
  ifeq ($(LEGATO_JOBS),)
    # If ccache is enabled, we can raise the number of concurrent
    # jobs as it likely to get objects from cache, therefore not
    # consumming much CPU.
    LEGATO_JOBS := $(shell echo $$((4 * $$(nproc))))
  endif
else
  CCACHE :=
  unexport CCACHE
endif

# ========== TARGET-SPECIFIC VARIABLES ============

# If the user specified a goal other than "clean", ensure that all required target-specific vars
# are defined.
ifneq ($(MAKECMDGOALS),clean)

  export HOST_ARCH := $(shell uname -m)
  TOOLS_ARCH ?= $(HOST_ARCH)
  FINDTOOLCHAIN := framework/tools/scripts/findtoolchain

  ifeq ($(TARGET),localhost)
    export TARGET_ARCH := $(shell uname -m)
  endif

  include targetDefs

  ifeq ($(USE_CLANG),1)
    export TARGET_CC = $(TOOLCHAIN_DIR)/$(TOOLCHAIN_PREFIX)clang
    export TARGET_CXX = $(TOOLCHAIN_DIR)/$(TOOLCHAIN_PREFIX)clang++
  else
    export TARGET_CC = $(TOOLCHAIN_DIR)/$(TOOLCHAIN_PREFIX)gcc
    export TARGET_CXX = $(TOOLCHAIN_DIR)/$(TOOLCHAIN_PREFIX)g++
  endif

  export $(TARGET)_CC = $(TARGET_CC)
  export $(TARGET)_CXX = $(TARGET_CXX)

endif

include $(wildcard modules/*/moduleDefs)

# Legato ReadOnly system tree
READ_ONLY ?= 0

# Disable SMACK
ifneq (,$(filter $(TARGET),virt))
  export DISABLE_SMACK ?= 1
endif
export DISABLE_SMACK ?= 0

STAGE_SYSTOIMG = stage_systoimg
ifeq ($(READ_ONLY),1)
  override STAGE_SYSTOIMG := stage_systoimgro
endif

# ========== GENERIC BUILD RULES ============

# Tell make that the targets are not actual files.
.PHONY: $(TARGETS)

# The rule to make each target is: build the system and stage for cwe image generation
# in build/$TARGET/staging.
# This will also cause the host tools, framework, and target tools to be built, since those things
# depend on them.
$(TARGETS): %: system_% stage_%

# Cleaning rule.
.PHONY: clean
clean:
	rm -rf build Documentation* bin doxygen.*.log doxygen.*.err
	rm -f framework/doc/toolsHost.dox framework/doc/toolsHost_*.dox
	rm -f sources.md5

# Version related rules.
ifndef LEGATO_VERSION
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

ifeq ($(ENABLE_IMA),1)
  # Export the variable so that sub command inherits the value. No need to check the variable value
  # mksys and mkapp will take care of it.
  export IMA_PRIVATE_KEY
  export IMA_PUBLIC_CERT

  # Check whether something specified in IMA_SMACK environment variable. If yes, export it.
  ifneq ($(strip $(IMA_SMACK)),)
    export IMA_SMACK := $(strip $(IMA_SMACK))
  endif
endif

# Source code directories and files to include in the MD5 sum in the version and package.properties.
FRAMEWORK_SOURCES = framework \
					components \
					interfaces \
					platformAdaptor \
					modules \
					apps/platformServices \
					$(wildcard apps/proprietary) \
					apps/tools \
					targetFiles \
					$(wildcard Makefile*) \
					$(wildcard targetDefs*) \
					CMakeLists.txt

# Error for missing platform adaptors
platformAdaptor modules:
	@echo -e "\033[1;31m'$@' directory is missing, which means these Legato sources have not been downloaded properly."
	@echo -e "Please refer to https://github.com/legatoproject/legato-af#clone-from-github \033[0m"
	@exit 1

.PHONY: sources.md5
sources.md5: $(FRAMEWORK_SOURCES)
	# Generate an MD5 hash of everything in the source directories.
	find $(FRAMEWORK_SOURCES) -type f | grep -v ".git" | sort | while read filePath ; \
	do \
	  echo "$$filePath" && \
	  cat "$$filePath" ; \
	done | md5sum | awk '{ print $$1 }' > sources.md5

.PHONY: version
version:
	@if [ -n "$(LEGATO_VERSION)" ] ; then \
		echo "$(LEGATO_VERSION)" > $@  ; \
	elif ! [ -e version ] ; then \
		echo "unknown" > $@ ; \
	fi

package.properties: version sources.md5
	@echo "version=`cat version`" > $@
	@echo "md5=`cat sources.md5`" >> $@
	@cat $@

# Goal for building all documentation.
.PHONY: docs user_docs implementation_docs
docs: $(PLANTUML_JAR_FILE) user_docs implementation_docs

# Docs for people who don't want to be distracted by the internal implementation details.
user_docs: localhost $(PLANTUML_JAR_FILE) build/localhost/Makefile
	$(MAKE) -C build/localhost user_docs
	rm -f Documentation
	@if [ -e "docManagement/Makefile" ] ; then \
		$(MAKE) -C docManagement ; \
	fi
	@if [ -d "build/doc/user/html_converted/" ] ; then \
		ln -sf build/doc/user/html_converted Documentation ; \
	else \
		ln -sf build/doc/user/html Documentation ; \
	fi

user_pdf: localhost build/localhost/Makefile
	$(MAKE) -C build/localhost user_pdf
	ln -sf build/localhost/bin/doc/user/legato-user.pdf Documentation.pdf

plantuml_docs: $(PLANTUML_JAR_FILE)
	for dir in components/doc platformAdaptor/qmi/src/components/doc; do \
		files=`ls $(LEGATO_ROOT)/$$dir/*` ; \
		java -Djava.awt.headless=true -jar $(PLANTUML_JAR_FILE) \
		                              -o $(LEGATO_ROOT)/build/doc/implementation/html $$files ; \
	done

# Docs for people who want or need to know the internal implementation details.
implementation_docs: localhost plantuml_docs build/localhost/Makefile
	$(MAKE) -C build/localhost implementation_docs

implementation_pdf: localhost build/localhost/Makefile
	$(MAKE) -C build/localhost implementation_pdf

# Rule building the unit test covergae report
coverage_report:
	$(MAKE) -C build/localhost coverage_report

# Rule for how to build the build tools.
.PHONY: tools
tools: version
	$(MAKE) -f Makefile.hostTools

# Rule for how to extract the build tool messages
.PHONY: tool-messages
tool-messages: version
	$(MAKE) -f Makefile.hostTools tool-messages

# Rule to create a "software development kit" from the tools.
.PHONY: sdk
sdk: tools
	createsdk

# Rule building the framework for a given target.
FRAMEWORK_TARGETS = $(foreach target,$(TARGETS),framework_$(target))
.PHONY: $(FRAMEWORK_TARGETS)
$(FRAMEWORK_TARGETS): tools package.properties
	$(MAKE) -f Makefile.framework CC=$(TARGET_CC)

## Tests

# Rule building the C tests for a given target
TESTS_C_TARGETS = $(foreach target,$(TARGETS),tests_c_$(target))
.PHONY: $(TESTS_C_TARGETS)
$(TESTS_C_TARGETS):tests_c_%: % framework_% build/%/Makefile
	$(MAKE) -C build/$(TARGET) tests_c

# Rule building the Java tests for a given target
TESTS_JAVA_TARGETS = $(foreach target,$(TARGETS),tests_java_$(target))
.PHONY: $(TESTS_JAVA_TARGETS)
$(TESTS_JAVA_TARGETS):tests_java_%: % framework_% build/%/Makefile
	$(MAKE) -C build/$(TARGET) tests_java

# Rule building the tests for a given target -- build both C and Java tests
TESTS_TARGETS = $(foreach target,$(TARGETS),tests_$(target))
.PHONY: $(TESTS_TARGETS)
$(TESTS_TARGETS):tests_%: % framework_% build/%/Makefile
	$(MAKE) -C build/$(TARGET) tests
	$(MAKE) -C apps/test/framework/mk CC=$(TARGET_CC)

## Samples

# Rule building the C samples for a given target
SAMPLES_C_TARGETS = $(foreach target,$(TARGETS),samples_c_$(target))
.PHONY: $(SAMPLES_C_TARGETS)
$(SAMPLES_C_TARGETS):samples_c_%: % framework_% build/%/Makefile
	$(MAKE) -C build/$(TARGET) samples_c

# Rule building the Java samples for a given target
SAMPLES_JAVA_TARGETS = $(foreach target,$(TARGETS),samples_java_$(target))
.PHONY: $(SAMPLES_JAVA_TARGETS)
$(SAMPLES_JAVA_TARGETS):samples_java_%: % framework_% build/%/Makefile
	$(MAKE) -C build/$(TARGET) samples_java

# Rule building the samples for a given target -- build both C and Java samples
SAMPLES_TARGETS = $(foreach target,$(TARGETS),samples_$(target))
.PHONY: $(SAMPLES_TARGETS)
$(SAMPLES_TARGETS):samples_%: % framework_% build/%/Makefile
	$(MAKE) -C build/$(TARGET) samples

## All

# Rule building all C content for a given target
ALL_C_TARGETS = $(foreach target,$(TARGETS),all_c_$(target))
.PHONY: $(ALL_C_TARGETS)
$(ALL_C_TARGETS):all_c_%: % framework_% tests_c_% samples_c_%

# Rule building all Java content for a given target
ALL_JAVA_TARGETS = $(foreach target,$(TARGETS),all_java_$(target))
.PHONY: $(ALL_JAVA_TARGETS)
$(ALL_JAVA_TARGETS):all_java_%: % framework_% tests_java_% samples_java_%

# Rule building all content for a given target
ALL_TARGETS = $(foreach target,$(TARGETS),all_$(target))
.PHONY: $(ALL_TARGETS)
$(ALL_TARGETS):all_%: % framework_% tests_% samples_%

# Rule for invoking CMake to generate the Makefiles inside the build directory.
# Depends on the build directory being there.
# NOTE: CMake is only used to build tests and samples.
$(foreach target,$(TARGETS),build/$(target)/Makefile):
	export PATH=$(TOOLCHAIN_DIR):$(PATH) && \
		cd `dirname $@` && \
		cmake ../.. \
			-DLEGATO_ROOT=$(LEGATO_ROOT) \
			-DLEGATO_TARGET=$(TARGET) \
			-DLEGATO_JOBS=$(LEGATO_JOBS) \
			-DPLANTUML_JAR_FILE=$(PLANTUML_JAR_FILE) \
			-DPA_DIR=$(LEGATO_ROOT)/platformAdaptor \
			-DTEST_COVERAGE=$(TEST_COVERAGE) \
			-DINCLUDE_ECALL=$(INCLUDE_ECALL) \
			-DUSE_CLANG=$(USE_CLANG) \
			-DSECSTOREADMIN=$(SECSTOREADMIN) \
			-DDISABLE_SMACK=$(DISABLE_SMACK) \
			-DPLATFORM_SIMULATION=$(PLATFORM_SIMULATION) \
			-DTOOLCHAIN_PREFIX=$(TOOLCHAIN_PREFIX) \
			-DTOOLCHAIN_DIR=$(TOOLCHAIN_DIR) \
			-DCMAKE_TOOLCHAIN_FILE=$(LEGATO_ROOT)/cmake/toolchain.yocto.cmake

# If set, generate an image with stripped binaries
ifneq ($(STRIP_STAGING_TREE),0)
  SYSTOIMG_FLAGS += -s
  MKSYS_FLAGS += -d build/$(TARGET)/debug
endif

ifeq ($(DISABLE_SMACK),1)
  SYSTOIMG_FLAGS += --disable-smack
endif

.PHONY: stage_systoimg
stage_systoimg:
	systoimg $(SYSTOIMG_FLAGS) $(TARGET) build/$(TARGET)/system.$(TARGET).update build/$(TARGET)
	# Check PA libraries.
	checkpa $(TARGET) || true
	# Link legato R/W images to default legato images
	(cd build/$(TARGET); \
	    for f in legato.*; do \
	        ln -sf $$f `echo $$f | sed 's/legato/legato_rw/'`; \
	    done)

.PHONY: stage_systoimgro
stage_systoimgro: stage_systoimg
	systoimg $(SYSTOIMG_FLAGS) -S _ro --read-only -a \
	    $(TARGET) build/$(TARGET)/system.$(TARGET).update build/$(TARGET)

.PHONY: stage_mkavmodel
stage_mkavmodel:
	@if [ -f "apps/platformServices/airVantageConnector/tools/scripts/mkavmodel" ] ; then \
		cp "apps/platformServices/airVantageConnector/tools/scripts/mkavmodel" bin/ ; \
		mkavmodel -t $(TARGET) -o build/$(TARGET) -v $(LEGATO_VERSION) ; \
	fi

# ==== localhost ====

.PHONY: stage_localhost
stage_localhost: $(STAGE_SYSTOIMG)

# ==== 9x15-based Sierra Wireless modules ====

.PHONY: stage_9x15
stage_9x15:

.PHONY: stage_ar7 stage_ar86 stage_wp85 stage_wp750x
stage_ar7 stage_ar86 stage_wp85 stage_wp750x: stage_9x15 $(STAGE_SYSTOIMG) stage_mkavmodel

# ==== AR758X (9x28-based Sierra Wireless modules) ====

.PHONY: stage_9x28
stage_9x28:

.PHONY: stage_ar758x stage_wp76xx stage_wp77xx
stage_ar758x stage_wp76xx stage_wp77xx: stage_9x28 $(STAGE_SYSTOIMG) stage_mkavmodel

# ==== AR759X (9x40-based Sierra Wireless modules) ====

.PHONY: stage_9x40
stage_9x40:

.PHONY: stage_ar759x
stage_ar759x: stage_9x40 $(STAGE_SYSTOIMG) stage_mkavmodel

# ==== Virtual ====

.PHONY: stage_virt
stage_virt: $(STAGE_SYSTOIMG)

# ==== Raspberry Pi ====

.PHONY: stage_raspi
stage_raspi: $(STAGE_SYSTOIMG)

# ========== RELEASE ============

# Clean first, then build for localhost and selected embedded targets and generate the documentation
# before packaging it all up into a compressed tarball.
# Partition images for relevant devices are provided as well in the releases/ folder.
.PHONY: release
release: clean
	for target in localhost $(RELEASE_TARGETS) docs; do set -e; make $$target; done
	releaselegato -t "$(shell echo ${RELEASE_TARGETS} | tr ' ' ',')"

# ========== PROTOTYPICAL SYSTEM ============

ifeq ($(VERBOSE),1)
  MKSYS_FLAGS += -v
endif

ifeq ($(ENABLE_IMA),1)
  MKSYS_FLAGS += -S
endif

ifneq ($(DEBUG),yes)
  # Optimize release builds
  MKSYS_FLAGS += --cflags="-O2 -fno-omit-frame-pointer"
endif

ifeq ($(TEST_COVERAGE),1)
  MKSYS_FLAGS += --cflags=--coverage --ldflags=--coverage

  # Except on localhost, storage coverage data (gcda) in /data/coverage by default
  ifneq ($(TARGET),localhost)
    TEST_COVERAGE_DIR ?= "/data/coverage"
  endif

  ifneq ($(TEST_COVERAGE_DIR),)
    MKSYS_FLAGS += --cflags=-fprofile-dir=${TEST_COVERAGE_DIR}
  endif
endif

ifneq ($(LEGATO_JOBS),)
  $(info Job Count: $(LEGATO_JOBS))
  MKSYS_FLAGS += -j $(LEGATO_JOBS)
  export LEGATO_JOBS
endif

# Define the default sdef file to use
SDEF_TO_USE ?= default.sdef

SYSTEM_TARGETS = $(foreach target,$(TARGETS),system_$(target))
.PHONY: $(SYSTEM_TARGETS)
$(SYSTEM_TARGETS):system_%: framework_%
	mksys -t $(TARGET) -w build/$(TARGET)/system -o build/$(TARGET) $(SDEF_TO_USE) \
			$(MKSYS_FLAGS)
	mv build/$(TARGET)/$(notdir $(SDEF_TO_USE:%.sdef=%)).$(TARGET).update \
	    build/$(TARGET)/system.$(TARGET).update
	if [ $(ENABLE_IMA) -eq 1 ] ; then \
		mv build/$(TARGET)/$(notdir $(SDEF_TO_USE:%.sdef=%)).$(TARGET).signed.update \
			build/$(TARGET)/system.$(TARGET).signed.update ; \
	fi
