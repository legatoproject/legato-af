# --------------------------------------------------------------------------------------------------
# Makefile used to build the Legato framework.
#
# See the TARGETS variable definition for a list of supported build targets.  Each of these
# is a valid goal for this Makefile.  For example, to build for the Sierra Wireless AR7xxx family
# of modules, run "make ar7".
#
# Other goals supported by this Makefile are:
#
#    clean = delete all build output.
#
#    docs = build the documentation.
#
#    tools = build the Legato-specific build tools.
#
#    release = build and package a release.
#
# To enable coverage testing, run make with "TEST_COVERAGE=1" on the command-line.
#
# Copyright (C) 2013-2014, Sierra Wireless, Inc.  Use of this work is subject to license.
# --------------------------------------------------------------------------------------------------

# List of target devices supported:
TARGETS := localhost ar6 ar7 wp7 raspi

# By default, build for the localhost and build the documentation.
.PHONY: default
default: localhost docs

# Define the LEGATO_ROOT environment variable.
export LEGATO_ROOT := $(CURDIR)

# Add the framework's bin directory to the PATH environment variable.
export PATH := $(PATH):$(LEGATO_ROOT)/bin

# Define paths to various platform adaptors' directories.
AUDIO_PA_DIR := $(LEGATO_ROOT)/components/audio/platformAdaptor
MODEM_PA_DIR := $(LEGATO_ROOT)/components/modemServices/platformAdaptor
GNSS_PA_DIR := $(LEGATO_ROOT)/components/positioning/platformAdaptor

# Configure appropriate QMI platform adaptor build variables.
#   Sources:
export LEGATO_QMI_AUDIO_PA_SRC := $(AUDIO_PA_DIR)/mdm9x15/le_pa_audio
export LEGATO_QMI_MODEM_PA_SRC := $(MODEM_PA_DIR)/qmi/le_pa
export LEGATO_QMI_GNSS_PA_SRC := $(GNSS_PA_DIR)/qmi/le_pa_gnss
#   Pre-built binaries:
export LEGATO_QMI_AUDIO_PA_BIN = $(AUDIO_PA_DIR)/pre-built/$(TARGET)/le_pa_audio
export LEGATO_QMI_MODEM_PA_BIN = $(MODEM_PA_DIR)/pre-built/$(TARGET)/le_pa
export LEGATO_QMI_GNSS_PA_BIN = $(GNSS_PA_DIR)/pre-built/$(TARGET)/le_pa_gnss
#   If the QMI PA sources are not available, use the pre-built binaries.
ifneq (,$(wildcard $(LEGATO_QMI_AUDIO_PA_SRC)/*))
  export LEGATO_QMI_AUDIO_PA = $(LEGATO_QMI_AUDIO_PA_SRC)
else
  export LEGATO_QMI_AUDIO_PA = $(LEGATO_QMI_AUDIO_PA_BIN)
endif
ifneq (,$(wildcard $(LEGATO_QMI_MODEM_PA_SRC)/*))
  export LEGATO_QMI_MODEM_PA = $(LEGATO_QMI_MODEM_PA_SRC)
else
  export LEGATO_QMI_MODEM_PA = $(LEGATO_QMI_MODEM_PA_BIN)
endif
ifneq (,$(wildcard $(LEGATO_QMI_GNSS_PA_SRC)/*))
  export LEGATO_QMI_GNSS_PA = $(LEGATO_QMI_GNSS_PA_SRC)
else
  export LEGATO_QMI_GNSS_PA = $(LEGATO_QMI_GNSS_PA_BIN)
endif

# Define the default platform adaptors to use if not otherwise specified for a given target.
export LEGATO_AUDIO_PA = $(AUDIO_PA_DIR)/stub/le_pa_audio
export LEGATO_MODEM_PA = $(MODEM_PA_DIR)/at/le_pa
export LEGATO_GNSS_PA = $(GNSS_PA_DIR)/at/le_pa_gnss

# Default AirVantage build to be ON
INCLUDE_AIRVANTAGE ?= 1

# ========== TARGET-SPECIFIC VARIABLES ============

# If the user specified a goal other than "clean", ensure that all required target-specific vars
# are defined.
ifneq ($(MAKECMDGOALS),clean)

  HOST_ARCH := $(shell arch)

  # LOCALHOST
  localhost: export TARGET := localhost
  localhost: export COMPILER_DIR = /usr/bin

  # AR6
  ifndef AR6_TOOLCHAIN_DIR
    ifeq ($(MAKECMDGOALS),ar6)
      ar6: $(warning AR6_TOOLCHAIN_DIR not defined.  Using default.)
    endif
    ar6: export AR6_TOOLCHAIN_DIR := /opt/poky/1.4.1/sdk/sysroots/$(HOST_ARCH)-pokysdk-linux/usr/bin/armv5te-poky-linux-gnueabi/
  endif
  ar6: export COMPILER_DIR = $(AR6_TOOLCHAIN_DIR)
  ar6: export TARGET := ar6
  ar6: export AV_CONFIG := -DCMAKE_TOOLCHAIN_FILE=../../../cmake/toolchain.yocto.cmake

  # AR7
  ifndef AR7_TOOLCHAIN_DIR
    ifeq ($(MAKECMDGOALS),ar7)
      ar7: $(warning AR7_TOOLCHAIN_DIR not defined.  Using default.)
    endif
    ar7: export AR7_TOOLCHAIN_DIR := /opt/swi/y14-ext/tmp/sysroots/$(HOST_ARCH)-linux/usr/bin/armv7a-vfp-neon-poky-linux-gnueabi
  endif
  ar7: export COMPILER_DIR = $(AR7_TOOLCHAIN_DIR)
  ar7: export TARGET := ar7
  ar7: export LEGATO_AUDIO_PA = $(LEGATO_QMI_AUDIO_PA)
  ar7: export LEGATO_MODEM_PA = $(LEGATO_QMI_MODEM_PA)
  ar7: export LEGATO_GNSS_PA = $(LEGATO_QMI_GNSS_PA)
  ar7: export AV_CONFIG := -DCMAKE_TOOLCHAIN_FILE=../../../cmake/toolchain.yocto.cmake

  # WP7
  ifndef WP7_TOOLCHAIN_DIR
    ifeq ($(MAKECMDGOALS),wp7)
      wp7: $(warning WP7_TOOLCHAIN_DIR not defined.  Using default. )
    endif
    wp7: export WP7_TOOLCHAIN_DIR := /opt/swi/y14-ext/tmp/sysroots/$(HOST_ARCH)-linux/usr/bin/armv7a-vfp-neon-poky-linux-gnueabi
  endif
  wp7: export COMPILER_DIR = $(WP7_TOOLCHAIN_DIR)
  wp7: export TARGET := wp7
  wp7: export LEGATO_AUDIO_PA = $(LEGATO_QMI_AUDIO_PA)
  wp7: export LEGATO_MODEM_PA = $(LEGATO_QMI_MODEM_PA)
  wp7: export LEGATO_GNSS_PA = $(LEGATO_QMI_GNSS_PA)
  wp7: export AV_CONFIG := -DCMAKE_TOOLCHAIN_FILE=../../../cmake/toolchain.yocto.cmake

  # Raspberry Pi
  ifndef RASPI_TOOLCHAIN_DIR
    ifeq ($(MAKECMDGOALS),raspi)
      raspi: $(warning RASPI_TOOLCHAIN_DIR not defined.)
    endif
  endif
  raspi: export COMPILER_DIR := $(RASPI_TOOLCHAIN_DIR)
  raspi: export TARGET := raspi
  raspi: export AV_CONFIG := -DCMAKE_TOOLCHAIN_FILE=../../../cmake/toolchain.yocto.cmake
endif


# ========== GENERIC BUILD RULES ============

# Tell make that the targets are not actual files.
.PHONY: $(TARGETS)


# The rule to make each target is: build it, then stage it.
$(TARGETS): %: build_% stage_%


# Cleaning rule.
.PHONY: clean
clean:
	rm -rf build Documentation bin doxygen.*.log doxygen.*.err
	rm -rf interfaces/config/c/configTypes.h \
		   interfaces/configAdmin/c/configAdminTypes.h

# Version related rules.
.PHONY: build_version
build_version:
	$(eval GIT_VERSION := $(shell git describe --tags 2> /dev/null))
	$(eval CURRENT_VERSION := $(shell cat version 2> /dev/null))

	@if [ -n "$(GIT_VERSION)" ] ; then \
		if test "$(GIT_VERSION)" != "$(CURRENT_VERSION)" ; then \
			echo "Version: $(GIT_VERSION)" ; \
			echo "$(GIT_VERSION)" > version  ; \
		fi \
	elif ! [ -e version ] ; then \
		echo "unknown" > version ; \
	fi
	@echo "version=`cat version`" > package.properties

version: build_version
package.properties: build_version

# Goal for building all documentation.
.PHONY: docs user_docs implementation_docs
docs: user_docs implementation_docs

# When building the docs, use the "localhost" target device type.
user_docs: TARGET := localhost
implementation_docs: TARGET := localhost

# Docs for people who don't want to be distracted by the internal implementation details.
user_docs: localhost
	$(MAKE) -C build/localhost doc
	ln -sf build/localhost/bin/doc/user/html Documentation

# Docs for people who want or need to know the internal implementation details.
implementation_docs: localhost
	$(MAKE) -C build/localhost implementation_doc


# Rule for how to build the build tools.
.PHONY: tools
tools: version
	mkdir -p build/tools && cd build/tools && cmake ../../buildTools && make
	mkdir -p bin
	ln -sf $(CURDIR)/build/tools/bin/mk* bin/
	ln -sf mk bin/mkif
	ln -sf mk bin/mkcomp
	ln -sf mk bin/mkexe
	ln -sf mk bin/mkapp
	ln -sf mk bin/mksys
	ln -sf $(CURDIR)/framework/tools/scripts/* bin/
	ln -sf $(CURDIR)/framework/tools/ifgen/ifgen bin/

# Rule for creating a build directory
build/%/bin/apps build/%/bin/lib build/%/airvantage:
	mkdir -p $@

# Rule for running the build for a given target using the Makefiles generated by CMake.
# This depends on the build tools and the CMake-generated Makefile.
build_%: tools version build/%/Makefile
	make -C build/$(TARGET)
ifeq ($(INCLUDE_AIRVANTAGE), 1)
	make -j`getconf _NPROCESSORS_ONLN` legato_airvantage -C build/$(TARGET)/airvantage
	make -C apps/airvantage
endif
	make -C build/$(TARGET) samples

# Rule for invoking CMake to generate the Makefiles inside the build directory.
# Depends on the build directory being there.
$(foreach target,$(TARGETS),build/$(target)/Makefile): build/%/Makefile: build/%/bin/apps \
																		 build/%/bin/lib \
																		 build/%/airvantage
	# Configure Legato
	export PATH=$(COMPILER_DIR):$(PATH) && \
		cd `dirname $@` && \
		cmake ../.. \
			-DLEGATO_TARGET=$(TARGET) \
			-DTEST_COVERAGE=$(TEST_COVERAGE) \
			-DINCLUDE_AIRVANTAGE=$(INCLUDE_AIRVANTAGE)

ifeq ($(INCLUDE_AIRVANTAGE), 1)
	# Configure AirVantage
	export PATH=$(COMPILER_DIR):$(PATH) && \
		mkdir -p `dirname $@`/airvantage && \
		cd `dirname $@`/airvantage && \
		cmake ../../../airvantage \
			-DPLATFORM=legato-wp7 \
			$(AV_CONFIG)
endif

# ========== STAGING RULES ============

# "Staging" = constructing on the local host a copy of (a part of) the target device file system.
# Each staging rule is named "stage_X", where the X is replaced with the name of the target.
# For example, the staging rule for the ar7 target is "stage_ar7".

# ==== LOCALHOST ====

# No staging is required for localhost builds at this time.
.PHONY: stage_localhost
stage_localhost:


# ==== EMBEDDED ====

# List of executables to install from the build's bin directory to the /usr/local/bin directory
# on the target.
BIN_INSTALL_LIST =  $(shell ls build/$(TARGET)/bin)

# List of libraries to install from the build's bin/lib directory to the /usr/local/lib directory
# on the target.
LIB_INSTALL_LIST =  $(shell ls build/$(TARGET)/bin/lib)

# List of applications to install from the build's bin/apps directory to the /usr/local/bin/apps
# directory on the target.
APP_INSTALL_LIST =  $(shell ls build/$(TARGET)/bin/apps)


# Construct the staging directory with common files
.PHONY: stage_common
stage_common:
	if [ -z "$(TARGET)" ]; then exit -1; fi
	install -d 	build/$(TARGET)/staging/opt/legato
	install version build/$(TARGET)/staging/opt/legato
	install -d 	build/$(TARGET)/staging/usr/local/bin
	install targetFiles/shared/bin/* -t build/$(TARGET)/staging/usr/local/bin
	for executable in $(BIN_INSTALL_LIST) ; \
	do \
	    install build/$(TARGET)/bin/$$executable -D build/$(TARGET)/staging/usr/local/bin ; \
	done
	mkdir -p build/$(TARGET)/staging/usr/local/lib
	for library in $(LIB_INSTALL_LIST) ; \
	do \
	    install build/$(TARGET)/bin/lib/$$library -D build/$(TARGET)/staging/usr/local/lib ; \
	done
	mkdir -p build/$(TARGET)/staging/usr/local/bin/apps
	for app in $(APP_INSTALL_LIST) ; \
	do \
	    install build/$(TARGET)/bin/apps/$$app -D build/$(TARGET)/staging/usr/local/bin/apps ; \
	done
	tar -C build/$(TARGET)/staging -cf build/$(TARGET)/legato-runtime.tar .

# ==== AR7 and WP7 (9x15-based Sierra Wireless modules) ====

.PHONY: stage_ar7 stage_wp7
stage_ar7 stage_wp7: stage_common

# ==== AR6 (Ykem-based Sierra Wireless modules) ====

.PHONY: stage_ar6
stage_ar6: stage_common

# ==== Raspberry Pi ====

.PHONY: stage_raspi
stage_raspi: stage_common

# ========== RELEASE ============

# Following are the rules for building and packaging a release.

.PHONY: release

# Define a regex specifying a set of directories that must be excluded from releases.
release: EXCLUDE_PATTERN := 'proprietary|qmi/le_pa|mdm9x15/le_pa|jenkins-build|^airvantage$$'

# Define output directory
release: RELEASE_DIR := releases

# We never build for coverage testing when building a release.
release: override TEST_COVERAGE=0

# Clean first, then build for localhost and selected embedded targets and generate the documentation
# before packaging it all up into a compressed tarball.
release: version package.properties clean localhost ar7 wp7 docs
	$(eval VERSION := $(shell cat version))
	$(eval STAGE_NAME := legato-$(VERSION))
	$(eval STAGE_DIR := $(RELEASE_DIR)/$(STAGE_NAME))
	$(eval PKG_NAME := $(STAGE_NAME).tar.bz2)
	@echo "Version = $(VERSION)"
	@echo "Preparing package in $(STAGE_DIR)"
	rm -rf $(STAGE_DIR)
	mkdir -p $(STAGE_DIR)
	git ls-tree -r --name-only HEAD | egrep -v $(EXCLUDE_PATTERN) | while read LINE ; \
	do \
		install -D "$$LINE" "$(STAGE_DIR)/$$LINE" ; \
	done
	install -D build/ar7/platformServices/apps/audio/staging/lib/lible_pa_audio.so $(STAGE_DIR)/components/audio/platformAdaptor/pre-built/ar7/le_pa_audio/
	install -D build/wp7/platformServices/apps/audio/staging/lib/lible_pa_audio.so $(STAGE_DIR)/components/audio/platformAdaptor/pre-built/wp7/le_pa_audio/
	install -D build/ar7/platformServices/apps/modem/staging/lib/lible_pa.so $(STAGE_DIR)/components/modemServices/platformAdaptor/pre-built/ar7/le_pa/
	install -D build/wp7/platformServices/apps/modem/staging/lib/lible_pa.so $(STAGE_DIR)/components/modemServices/platformAdaptor/pre-built/wp7/le_pa/
	install -D build/ar7/platformServices/apps/positioning/staging/lib/lible_pa_gnss.so $(STAGE_DIR)/components/positioning/platformAdaptor/pre-built/ar7/le_pa_gnss/
	install -D build/wp7/platformServices/apps/positioning/staging/lib/lible_pa_gnss.so $(STAGE_DIR)/components/positioning/platformAdaptor/pre-built/wp7/le_pa_gnss/
	cp -R airvantage $(STAGE_DIR)
	cp version $(STAGE_DIR)
	cp package.properties $(STAGE_DIR)
	find $(STAGE_DIR) -name ".git" | xargs -i rm -rf {}
	@echo "Creating release package $(RELEASE_DIR)/$(PKG_NAME)"
	cd $(RELEASE_DIR) && tar jcf $(PKG_NAME) $(STAGE_NAME)
