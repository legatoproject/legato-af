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
# To build sample apps and tests for a target X, build tests_X (e.g., 'make tests_wp85').
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
  RELEASE_TARGETS := ar7 ar758x ar759x ar86 wp85 wp750x wp76xx
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
USE_CLANG ?= 0

# Default eCall build to be ON
export INCLUDE_ECALL ?= 1

# Do not be verbose by default.
export VERBOSE ?= 0

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

# ========== TARGET-SPECIFIC VARIABLES ============

# If the user specified a goal other than "clean", ensure that all required target-specific vars
# are defined.
ifneq ($(MAKECMDGOALS),clean)

  export HOST_ARCH := $(shell uname -m)
  TOOLS_ARCH ?= $(HOST_ARCH)
  FINDTOOLCHAIN := framework/tools/scripts/findtoolchain

  include targetDefs

  ifeq ($(USE_CLANG),1)
    export TARGET_CC = $(TOOLCHAIN_DIR)/$(TOOLCHAIN_PREFIX)clang
  else
    export TARGET_CC = $(TOOLCHAIN_DIR)/$(TOOLCHAIN_PREFIX)gcc
  endif

endif

include $(wildcard modules/*/moduleDefs)

# Legato ReadOnly system tree
READ_ONLY ?= 0

# Disable SMACK
export DISABLE_SMACK ?= 0

STAGE_MKLEGATOIMG = stage_mklegatoimg
ifeq ($(READ_ONLY),1)
  override STAGE_MKLEGATOIMG := stage_mklegatoimgro
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
endif

# Source code directories and files to include in the MD5 sum in the version and package.properties.
FRAMEWORK_SOURCES = framework \
					components \
					interfaces \
					platformAdaptor \
					modules \
					apps/platformServices \
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
	$(MAKE) -f Makefile.framework

# Rule building the C tests for a given target
TESTS_C_TARGETS = $(foreach target,$(TARGETS),tests_c_$(target))
.PHONY: TESTS_C_TARGETS
$(TESTS_C_TARGETS):tests_c_%: % framework_% build/%/Makefile
	$(MAKE) -C build/$(TARGET) tests_c

# Rule building the Java tests for a given target
TESTS_JAVA_TARGETS = $(foreach target,$(TARGETS),tests_java_$(target))
.PHONY: TESTS_JAVA_TARGETS
$(TESTS_JAVA_TARGETS):tests_java_%: % framework_% build/%/Makefile
	$(MAKE) -C build/$(TARGET) tests_java

# Rule building the tests for a given target -- build both C and Java tests
TESTS_TARGETS = $(foreach target,$(TARGETS),tests_$(target))
.PHONY: $(TESTS_TARGETS)
$(TESTS_TARGETS):tests_%: % framework_% build/%/Makefile
	$(MAKE) -C build/$(TARGET)
	$(MAKE) -C apps/test/framework/mk

# Rule for invoking CMake to generate the Makefiles inside the build directory.
# Depends on the build directory being there.
# NOTE: CMake is only used to build tests and samples.
$(foreach target,$(TARGETS),build/$(target)/Makefile):
	export PATH=$(TOOLCHAIN_DIR):$(PATH) && \
		cd `dirname $@` && \
		cmake ../.. \
			-DLEGATO_ROOT=$(LEGATO_ROOT) \
			-DLEGATO_TARGET=$(TARGET) \
			-DPLANTUML_JAR_FILE=$(PLANTUML_JAR_FILE) \
			-DPA_DIR=$(LEGATO_ROOT)/platformAdaptor \
			-DTEST_COVERAGE=$(TEST_COVERAGE) \
			-DINCLUDE_ECALL=$(INCLUDE_ECALL) \
			-DUSE_CLANG=$(USE_CLANG) \
			-DDISABLE_SMACK=$(DISABLE_SMACK) \
			-DPLATFORM_SIMULATION=$(PLATFORM_SIMULATION) \
			-DTOOLCHAIN_PREFIX=$(TOOLCHAIN_PREFIX) \
			-DTOOLCHAIN_DIR=$(TOOLCHAIN_DIR) \
			-DCMAKE_TOOLCHAIN_FILE=$(LEGATO_ROOT)/cmake/toolchain.yocto.cmake

# Construct the staging directory with common files for embedded targets.
# Staging directory will become /mnt/legato/ in cwe
.PHONY: stage_embedded
stage_embedded:
	# Make sure the TARGET environment variable is set.
	if [ -z "$(TARGET)" ]; then exit -1; fi
	# Prep the staging area to make sure there are no leftover files from last build.
	rm -rf build/$(TARGET)/staging
	# Create the directory.
	mkdir -p build/$(TARGET)/staging/apps
	outputDir=build/$(TARGET)/staging && \
	stagingDir="build/$(TARGET)/system/staging" && \
	echo "Copying system staging directory '$$stagingDir' to '$$outputDir/system'." && \
	cp -r -P $$stagingDir "$$outputDir/system" && \
	echo "Adding apps to '$$outputDir'." && \
	for appName in `ls "$$outputDir/system/apps/"` ; \
	do \
		md5=`readlink "$$outputDir/system/apps/$$appName" | sed 's#^/legato/apps/##'` && \
		if grep -e `printf '"md5":"%s"' $$md5` build/$(TARGET)/system.$(TARGET).update > /dev/null ; \
		then \
			echo "  $$appName ($$md5)" && \
			cp -r -P build/$(TARGET)/system/app/$$appName/staging "$$outputDir"/apps/$$md5 ; \
		else \
			echo "  $$appName ($$md5) <-- EXCLUDED FROM .CWE (must be preloaded on target)" ; \
		fi \
	done
	cp -r targetFiles/shared/bin build/$(TARGET)/staging/system/
	# Print some diagnostic messages.
	@echo "== built system's info.properties: =="
	cat build/$(TARGET)/staging/system/info.properties
	# Check PA libraries.
	checkpa $(TARGET) || true

# If set, generate an image with stripped binaries
ifeq ($(STRIP_STAGING_TREE),1)
  MKLEGATOIMG_FLAGS += -s
endif

.PHONY: stage_mklegatoimgro
stage_mklegatoimgro:
	mklegatoimg -t $(TARGET) -d build/$(TARGET)/staging -o build/$(TARGET) -S _rw $(MKLEGATOIMG_FLAGS)
	mklegatotreero $(TARGET) $(DISABLE_SMACK)
	mklegatoimg -t $(TARGET) -d build/$(TARGET)/readOnlyStaging/legato -o build/$(TARGET) -a $(MKLEGATOIMG_FLAGS)

.PHONY: stage_mklegatoimg
stage_mklegatoimg:
	mklegatoimg -t $(TARGET) -d build/$(TARGET)/staging -o build/$(TARGET) $(MKLEGATOIMG_FLAGS)

# ==== localhost needs no staging. Just a blank rule ====

.PHONY: stage_localhost
stage_localhost:

.PHONY: stage_shared
stage_shared:
	install targetFiles/shared/bin/start build/$(TARGET)/staging

# ==== 9x15-based Sierra Wireless modules ====

.PHONY: stage_9x15
stage_9x15: stage_shared

.PHONY: stage_ar7 stage_ar86 stage_wp85 stage_wp750x
stage_ar7 stage_ar86 stage_wp85 stage_wp750x: stage_embedded stage_9x15 $(STAGE_MKLEGATOIMG)

# ==== AR758X (9x28-based Sierra Wireless modules) ====

.PHONY: stage_9x28
stage_9x28: stage_shared

.PHONY: stage_ar758x stage_wp76xx
stage_ar758x stage_wp76xx: stage_embedded stage_9x28 $(STAGE_MKLEGATOIMG)

# ==== AR759X (9x40-based Sierra Wireless modules) ====

.PHONY: stage_9x40
stage_9x40: stage_shared

.PHONY: stage_ar759x
stage_ar759x: stage_embedded stage_9x40 $(STAGE_MKLEGATOIMG)

# ==== Virtual ====

.PHONY: stage_virt
stage_virt: stage_embedded stage_shared stage_mklegatoimg

# ==== Raspberry Pi ====

.PHONY: stage_raspi
stage_raspi: stage_embedded stage_shared stage_mklegatoimg

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

ifneq ($(DEBUG),yes)
  # Optimize release builds
  MKSYS_FLAGS += --cflags=-O2
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
