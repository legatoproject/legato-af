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
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
# --------------------------------------------------------------------------------------------------

# List of target devices needed by the release process:
RELEASE_TARGETS := localhost ar7 wp7 ar86 wp85

# List of target devices supported:
TARGETS := $(RELEASE_TARGETS) ar6 raspi virt

# By default, build for the localhost and build the documentation.
.PHONY: default
default: localhost docs

# Define the LEGATO_ROOT environment variable.
export LEGATO_ROOT := $(CURDIR)

# Add the framework's bin directory to the PATH environment variable.
export PATH := $(PATH):$(LEGATO_ROOT)/bin

# Some framework on-target runtime parameters.
export LEGATO_FRAMEWORK_NICE_LEVEL := -19
export LE_RUNTIME_DIR := /tmp/legato/
export LE_SVCDIR_SERVER_SOCKET_NAME := $(LE_RUNTIME_DIR)serviceDirectoryServer
export LE_SVCDIR_CLIENT_SOCKET_NAME := $(LE_RUNTIME_DIR)serviceDirectoryClient

# Define paths to various platform adaptors' directories.
AUDIO_PA_DIR := $(LEGATO_ROOT)/components/audio/platformAdaptor
MODEM_PA_DIR := $(LEGATO_ROOT)/components/modemServices/platformAdaptor
GNSS_PA_DIR := $(LEGATO_ROOT)/components/positioning/platformAdaptor
AVC_PA_DIR := $(LEGATO_ROOT)/components/airVantage/platformAdaptor
SECSTORE_PA_DIR := $(LEGATO_ROOT)/components/secStore/platformAdaptor
FWUPDATE_PA_DIR := $(LEGATO_ROOT)/components/fwupdate/platformAdaptor

# Configure appropriate QMI platform adaptor build variables.
#   Sources:
export LEGATO_QMI_AUDIO_PA_SRC := $(AUDIO_PA_DIR)/mdm9x15/le_pa_audio
export LEGATO_QMI_MODEM_PA_SRC := $(MODEM_PA_DIR)/qmi/le_pa
export LEGATO_QMI_MODEM_PA_ECALL_SRC := $(MODEM_PA_DIR)/qmi/le_pa_ecall
export LEGATO_QMI_GNSS_PA_SRC := $(GNSS_PA_DIR)/qmi/le_pa_gnss
export LEGATO_QMI_AVC_PA_SRC := $(AVC_PA_DIR)/qmi/le_pa_avc
export LEGATO_QMI_SECSTORE_PA_SRC := $(SECSTORE_PA_DIR)/qmi/le_pa_secStore
export LEGATO_QMI_FWUPDATE_PA_SRC := $(FWUPDATE_PA_DIR)/qmi/le_pa_fwupdate

#   Pre-built binaries:
export LEGATO_QMI_AUDIO_PA_BIN = $(AUDIO_PA_DIR)/pre-built/$(TARGET)/le_pa_audio
export LEGATO_QMI_MODEM_PA_BIN = $(MODEM_PA_DIR)/pre-built/$(TARGET)/le_pa
export LEGATO_QMI_MODEM_PA_ECALL_BIN = $(MODEM_PA_DIR)/pre-built/$(TARGET)/le_pa_ecall
export LEGATO_QMI_GNSS_PA_BIN = $(GNSS_PA_DIR)/pre-built/$(TARGET)/le_pa_gnss
export LEGATO_QMI_AVC_PA_BIN = $(AVC_PA_DIR)/pre-built/$(TARGET)/le_pa_avc
export LEGATO_QMI_SECSTORE_PA_BIN = $(SECSTORE_PA_DIR)/pre-built/$(TARGET)/le_pa_secStore
export LEGATO_QMI_FWUPDATE_PA_BIN = $(FWUPDATE_PA_DIR)/pre-built/$(TARGET)/le_pa_fwupdate

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
ifneq (,$(wildcard $(LEGATO_QMI_MODEM_PA_ECALL_SRC)/*))
  export LEGATO_QMI_MODEM_PA_ECALL = $(LEGATO_QMI_MODEM_PA_ECALL_SRC)
else
  export LEGATO_QMI_MODEM_PA_ECALL = $(LEGATO_QMI_MODEM_PA_ECALL_BIN)
endif
ifneq (,$(wildcard $(LEGATO_QMI_GNSS_PA_SRC)/*))
  export LEGATO_QMI_GNSS_PA = $(LEGATO_QMI_GNSS_PA_SRC)
else
  export LEGATO_QMI_GNSS_PA = $(LEGATO_QMI_GNSS_PA_BIN)
endif
ifneq (,$(wildcard $(LEGATO_QMI_AVC_PA_SRC)/*))
  export LEGATO_QMI_AVC_PA = $(LEGATO_QMI_AVC_PA_SRC)
else
  export LEGATO_QMI_AVC_PA = $(LEGATO_QMI_AVC_PA_BIN)
endif
ifneq (,$(wildcard $(LEGATO_QMI_SECSTORE_PA_SRC)/*))
  export LEGATO_QMI_SECSTORE_PA = $(LEGATO_QMI_SECSTORE_PA_SRC)
else
  export LEGATO_QMI_SECSTORE_PA = $(LEGATO_QMI_SECSTORE_PA_BIN)
endif
ifneq (,$(wildcard $(LEGATO_QMI_FWUPDATE_PA_SRC)/*))
  export LEGATO_QMI_FWUPDATE_PA = $(LEGATO_QMI_FWUPDATE_PA_SRC)
else
  export LEGATO_QMI_FWUPDATE_PA = $(LEGATO_QMI_FWUPDATE_PA_BIN)
endif

# Define the default platform adaptors to use if not otherwise specified for a given target.
export LEGATO_AUDIO_PA = $(AUDIO_PA_DIR)/stub/le_pa_audio
export LEGATO_MODEM_PA = $(MODEM_PA_DIR)/at/le_pa
export LEGATO_MODEM_PA_ECALL = $(MODEM_PA_DIR)/stub/le_pa_ecall
export LEGATO_GNSS_PA = $(GNSS_PA_DIR)/at/le_pa_gnss
export LEGATO_AVC_PA = $(AVC_PA_DIR)/at/le_pa_avc
export LEGATO_SECSTORE_PA = $(SECSTORE_PA_DIR)/at/le_pa_secStore
export LEGATO_FWUPDATE_PA = $(FWUPDATE_PA_DIR)/at/le_pa_fwupdate

# Default AirVantage build to be ON
INCLUDE_AIRVANTAGE ?= 1

# Do not use clang by default.
USE_CLANG ?= 0

# Default eCall build  to be ON
INCLUDE_ECALL ?= 1

# In case of release, override parameters
ifeq ($(MAKECMDGOALS),release)
  # We never build for coverage testing when building a release.
  override TEST_COVERAGE := 0
endif

# ========== TARGET-SPECIFIC VARIABLES ============

FINDTOOLCHAIN := framework/tools/scripts/findtoolchain

# If the user specified a goal other than "clean", ensure that all required target-specific vars
# are defined.
ifneq ($(MAKECMDGOALS),clean)

  HOST_ARCH := $(shell uname -m)

  # LOCALHOST
  localhost: export TARGET := localhost
  localhost: export COMPILER_DIR = /usr/bin

  # AR6
  ifndef AR6_TOOLCHAIN_DIR
    ifeq ($(MAKECMDGOALS),ar6)
      ar6: $(warning AR6_TOOLCHAIN_DIR not defined.  Using default.)
    endif
    ar6: export AR6_TOOLCHAIN_DIR := $(shell $(FINDTOOLCHAIN) ar6)
  endif
  ar6: export COMPILER_DIR = $(AR6_TOOLCHAIN_DIR)
  ar6: export TARGET := ar6
  ar6: export CMAKE_CONFIG := -DCMAKE_TOOLCHAIN_FILE=$(LEGATO_ROOT)/cmake/toolchain.yocto.cmake

  # AR7
  ifndef AR7_TOOLCHAIN_DIR
    ifeq ($(MAKECMDGOALS),ar7)
      ar7: $(warning AR7_TOOLCHAIN_DIR not defined.  Using default.)
    endif
    export AR7_TOOLCHAIN_DIR := $(shell $(FINDTOOLCHAIN) ar7)
  endif
  export AR7_TOOLCHAIN_PREFIX = arm-poky-linux-gnueabi-
  ar7: export COMPILER_DIR = $(AR7_TOOLCHAIN_DIR)
  ar7: export TARGET := ar7
  ar7: export LEGATO_AUDIO_PA = $(LEGATO_QMI_AUDIO_PA)
  ar7: export LEGATO_MODEM_PA = $(LEGATO_QMI_MODEM_PA)
  ifeq ($(INCLUDE_ECALL),1)
    ar7: export LEGATO_MODEM_PA_ECALL = $(LEGATO_QMI_MODEM_PA_ECALL)
  endif
  ar7: export LEGATO_GNSS_PA = $(LEGATO_QMI_GNSS_PA)
  ar7: export LEGATO_AVC_PA = $(LEGATO_QMI_AVC_PA)
  ar7: export LEGATO_SECSTORE_PA = $(LEGATO_QMI_SECSTORE_PA)
  ar7: export LEGATO_FWUPDATE_PA = $(LEGATO_QMI_FWUPDATE_PA)
  ar7: export CMAKE_CONFIG := -DCMAKE_TOOLCHAIN_FILE=$(LEGATO_ROOT)/cmake/toolchain.yocto.cmake

  # AR86
  ifndef AR86_TOOLCHAIN_DIR
    ifeq ($(MAKECMDGOALS),ar86)
      ar86: $(warning AR86_TOOLCHAIN_DIR not defined.  Using default.)
    endif
    export AR86_TOOLCHAIN_DIR := $(shell $(FINDTOOLCHAIN) ar86)
  endif
  export AR86_TOOLCHAIN_PREFIX = arm-poky-linux-gnueabi-
  ar86: export COMPILER_DIR = $(AR86_TOOLCHAIN_DIR)
  ar86: export TARGET := ar86
  ar86: export LEGATO_AUDIO_PA = $(LEGATO_QMI_AUDIO_PA)
  ar86: export LEGATO_MODEM_PA = $(LEGATO_QMI_MODEM_PA)
  ifeq ($(INCLUDE_ECALL),1)
    ar86: export LEGATO_MODEM_PA_ECALL = $(LEGATO_QMI_MODEM_PA_ECALL)
  endif
  ar86: export LEGATO_GNSS_PA = $(LEGATO_QMI_GNSS_PA)
  ar86: export LEGATO_AVC_PA = $(LEGATO_QMI_AVC_PA)
  ar86: export LEGATO_SECSTORE_PA = $(LEGATO_QMI_SECSTORE_PA)
  ar86: export LEGATO_FWUPDATE_PA = $(LEGATO_QMI_FWUPDATE_PA)
  ar86: export CMAKE_CONFIG := -DCMAKE_TOOLCHAIN_FILE=$(LEGATO_ROOT)/cmake/toolchain.yocto.cmake

  # WP7
  ifndef WP7_TOOLCHAIN_DIR
    ifeq ($(MAKECMDGOALS),wp7)
      wp7: $(warning WP7_TOOLCHAIN_DIR not defined.  Using default.)
    endif
    export WP7_TOOLCHAIN_DIR := $(shell $(FINDTOOLCHAIN) wp7)
  endif
  export WP7_TOOLCHAIN_PREFIX = arm-poky-linux-gnueabi-
  wp7: export COMPILER_DIR = $(WP7_TOOLCHAIN_DIR)
  wp7: export TARGET := wp7
  wp7: export LEGATO_AUDIO_PA = $(LEGATO_QMI_AUDIO_PA)
  wp7: export LEGATO_MODEM_PA = $(LEGATO_QMI_MODEM_PA)
  wp7: export LEGATO_GNSS_PA = $(LEGATO_QMI_GNSS_PA)
  wp7: export LEGATO_AVC_PA = $(LEGATO_QMI_AVC_PA)
  wp7: export LEGATO_SECSTORE_PA = $(LEGATO_QMI_SECSTORE_PA)
  wp7: export LEGATO_FWUPDATE_PA = $(LEGATO_QMI_FWUPDATE_PA)
  wp7: export CMAKE_CONFIG := -DCMAKE_TOOLCHAIN_FILE=$(LEGATO_ROOT)/cmake/toolchain.yocto.cmake

  # WP85
  ifndef WP85_TOOLCHAIN_DIR
    ifeq ($(MAKECMDGOALS),wp85)
      wp85: $(warning WP85_TOOLCHAIN_DIR not defined.  Using default.)
    endif
    export WP85_TOOLCHAIN_DIR := $(shell $(FINDTOOLCHAIN) wp85)
  endif
  wp85: export COMPILER_DIR = $(WP85_TOOLCHAIN_DIR)
  wp85: export TARGET := wp85
  wp85: export LEGATO_MODEM_PA = $(LEGATO_QMI_MODEM_PA)
  wp85: export LEGATO_GNSS_PA = $(LEGATO_QMI_GNSS_PA)
  wp85: export LEGATO_AVC_PA = $(LEGATO_QMI_AVC_PA)
  wp85: export LEGATO_SECSTORE_PA = $(LEGATO_QMI_SECSTORE_PA)
  wp85: export LEGATO_FWUPDATE_PA = $(LEGATO_QMI_FWUPDATE_PA)
  wp85: export CMAKE_CONFIG := -DCMAKE_TOOLCHAIN_FILE=$(LEGATO_ROOT)/cmake/toolchain.yocto.cmake

  # Raspberry Pi
  ifndef RASPI_TOOLCHAIN_DIR
    ifeq ($(MAKECMDGOALS),raspi)
      raspi: $(error RASPI_TOOLCHAIN_DIR not defined.)
    endif
  endif
  raspi: export COMPILER_DIR := $(RASPI_TOOLCHAIN_DIR)
  raspi: export TARGET := raspi
  raspi: export CMAKE_CONFIG := -DCMAKE_TOOLCHAIN_FILE=$(LEGATO_ROOT)/cmake/toolchain.yocto.cmake

  # Virtual platform
  ifeq ($(MAKECMDGOALS),virt)
    ifndef VIRT_TARGET_ARCH
      export VIRT_TARGET_ARCH := arm
    endif

    ifndef VIRT_TOOLCHAIN_DIR
      virt: $(warning VIRT_TOOLCHAIN_DIR not defined.  Using default.)
      export VIRT_TOOLCHAIN_DIR := $(shell $(FINDTOOLCHAIN) virt_${VIRT_TARGET_ARCH})
    endif

    ifndef TOOLCHAIN_PREFIX
      ifeq ($(VIRT_TARGET_ARCH),x86)
        export TOOLCHAIN_PREFIX := i586-poky-linux-
      else
        export TOOLCHAIN_PREFIX := arm-poky-linux-gnueabi-
      endif
    endif

    ifndef VIRT_TOOLCHAIN_PREFIX
      export VIRT_TOOLCHAIN_PREFIX := ${TOOLCHAIN_PREFIX}
    endif
  endif
  virt: export CMAKE_CONFIG := -DCMAKE_TOOLCHAIN_FILE=$(LEGATO_ROOT)/cmake/toolchain.yocto.cmake -DTOOLCHAIN_PREFIX=$(TOOLCHAIN_PREFIX)
  virt: export COMPILER_DIR := $(VIRT_TOOLCHAIN_DIR)
  virt: export TARGET := virt
  virt: export LEGATO_AUDIO_PA := $(AUDIO_PA_DIR)/simu/le_pa_audio
  virt: export LEGATO_MODEM_PA := $(MODEM_PA_DIR)/simu/le_pa
  virt: export LEGATO_GNSS_PA := $(GNSS_PA_DIR)/simu/le_pa_gnss
  virt: export LEGATO_AVC_PA = $(AVC_PA_DIR)/simu/le_pa_avc
  virt: export LEGATO_SECSTORE_PA = $(SECSTORE_PA_DIR)/simu/le_pa_secStore
  virt: export LEGATO_FWUPDATE_PA = $(FWUPDATE_PA_DIR)/simu/le_pa_fwupdate
  virt: export PLATFORM_SIMULATION := 1
endif


# ========== GENERIC BUILD RULES ============

# Tell make that the targets are not actual files.
.PHONY: $(TARGETS)


# The rule to make each target is: build it, stage it, and finally package it.
$(TARGETS): %: build_% stage_% package_%


# Cleaning rule.
.PHONY: clean
clean:
	rm -rf build Documentation* bin doxygen.*.log doxygen.*.err
	rm -f framework/doc/toolsHost.dox framework/doc/toolsHost_*.dox
	rm -rf interfaces/config/c/configTypes.h \
		   interfaces/configAdmin/c/configAdminTypes.h

# Version related rules.
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
	$(MAKE) -C build/localhost user_docs
	ln -sf build/localhost/bin/doc/user/html Documentation

user_pdf: localhost
	$(MAKE) -C build/localhost user_pdf
	ln -sf build/localhost/bin/doc/user/legato-user.pdf Documentation.pdf

# Docs for people who want or need to know the internal implementation details.
implementation_docs: localhost
	$(MAKE) -C build/localhost implementation_docs

implementation_pdf: localhost
	$(MAKE) -C build/localhost implementation_pdf

# Rule for how to build the build tools.
.PHONY: tools
tools: version
	mkdir -p build/tools && cd build/tools && cmake ../../framework/tools/mkTools -DUSE_CLANG=$(USE_CLANG) && make
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
	make -C apps/airvantage pre_build
	cd build/$(TARGET)/apps/airvantage/le_data_client && ln -sf `find -name le_data_interface.h -type f` .
	make -j`getconf _NPROCESSORS_ONLN` legato_airvantage -C build/$(TARGET)/airvantage
	make -C apps/airvantage package
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
			-DINCLUDE_AIRVANTAGE=$(INCLUDE_AIRVANTAGE) \
			-DINCLUDE_ECALL=$(INCLUDE_ECALL) \
			-DUSE_CLANG=$(USE_CLANG) \
			-DPLATFORM_SIMULATION=$(PLATFORM_SIMULATION) \
			$(CMAKE_CONFIG)

ifeq ($(INCLUDE_AIRVANTAGE), 1)
	# Configure AirVantage
	export PATH=$(COMPILER_DIR):$(PATH) && \
		mkdir -p `dirname $@`/airvantage && \
		cd `dirname $@`/airvantage && \
		cmake ../../../airvantage \
			-DPLATFORM=legato-wp7 \
			-DLEGATO_TARGET=$(TARGET) \
			$(CMAKE_CONFIG)
endif

# ========== STAGING AND PACKAGING RULES ============

# "Staging" = constructing on the local host a copy of (a part of) the target device file system.
# Each staging rule is named "stage_X", where the X is replaced with the name of the target.
# For example, the staging rule for the ar7 target is "stage_ar7".

# "Packaging" = constructing on the local host an installation package that can be given to the
# on-target installer program to install the framework.
# Each packaging rule is named "package_X", where the X is replaced with the name of the target.

# ==== LOCALHOST ====

# No staging is required for localhost builds at this time.
.PHONY: stage_localhost
stage_localhost:

# No packaging is required for localhost builds at this time.
.PHONY: package_localhost
package_localhost:

# ==== EMBEDDED ====

# List of executables to install from the build's bin directory to the /usr/local/bin directory
# on the target.
BIN_INSTALL_LIST =  $(shell ls build/$(TARGET)/bin)

# List of libraries to install from the build's bin/lib directory to the /usr/local/lib directory
# on the target.
LIB_INSTALL_LIST =  $(shell ls build/$(TARGET)/bin/lib)

# List of applications to install from the build's bin/apps directory to the /usr/local/bin/apps
# directory on the target.
APP_INSTALL_LIST =  $(shell ls build/$(TARGET)/bin/apps --ignore='manifest-*.app')

# Construct the staging directory with common files for embedded targets.
.PHONY: stage_embedded
stage_embedded:
	# Make sure the TARGET environment variable is set.
	if [ -z "$(TARGET)" ]; then exit -1; fi
	# Prep the staging area to make sure there are no leftover files from last build.
	rm -rf build/$(TARGET)/staging
	# Install version file.
	install -d build/$(TARGET)/staging/opt/legato
	install version build/$(TARGET)/staging/opt/legato
	# Install binaries in /usr/local/bin.
	install -d build/$(TARGET)/staging/usr/local/bin
	install targetFiles/shared/bin/* -t build/$(TARGET)/staging/usr/local/bin
	for executable in $(BIN_INSTALL_LIST) ; \
	do \
	    install build/$(TARGET)/bin/$$executable -D build/$(TARGET)/staging/usr/local/bin ; \
	done
	# Install libraries in /usr/local/lib.
	mkdir -p build/$(TARGET)/staging/usr/local/lib
	for library in $(LIB_INSTALL_LIST) ; \
	do \
	    cp -P build/$(TARGET)/bin/lib/$$library build/$(TARGET)/staging/usr/local/lib ; \
	done
	# Install bundled framework apps in /usr/local/bin/apps.
	mkdir -p build/$(TARGET)/staging/usr/local/bin/apps
	for app in $(APP_INSTALL_LIST) ; \
	do \
	    install build/$(TARGET)/bin/apps/$$app -D build/$(TARGET)/staging/usr/local/bin/apps ; \
	done

# Construct the framework installation package.
.PHONY: package_embedded
package_embedded:
	# Create a tarball containing everything in the staging area.
	tar -C build/$(TARGET)/staging -cf build/$(TARGET)/legato-runtime.tar .

# ==== AR7, AR86 and WP7 (9x15-based Sierra Wireless modules) ====

.PHONY: stage_ar7 stage_wp7 stage_ar86 stage_wp85
stage_ar7 stage_wp7 stage_ar86 stage_wp85: stage_embedded stage_9x15

.PHONY: stage_9x15
stage_9x15:
	# Install default startup scripts.
	install -d build/$(TARGET)/staging/mnt/flash/startupDefaults
	install targetFiles/mdm-9x15/startup/* -t build/$(TARGET)/staging/mnt/flash/startupDefaults

.PHONY: package_ar7 package_wp7 package_ar86 package_wp85
package_ar7 package_wp7 package_ar86 package_wp85: package_embedded package_9x15

.PHONY: package_9x15
package_9x15:
	mklegatoimg -t $(TARGET) -d build/$(TARGET)/staging -o build/$(TARGET)

# ==== AR6 (Ykem-based Sierra Wireless modules) ====

.PHONY: stage_ar6
stage_ar6: stage_embedded

.PHONY: package_ar6
package_ar6: package_embedded

# ==== Raspberry Pi ====

.PHONY: stage_raspi
stage_raspi: stage_embedded

.PHONY: package_raspi
package_raspi: package_embedded

# ==== Virtual ====

.PHONY: stage_virt
stage_virt: stage_embedded
	# Install default startup scripts.
	install -d build/$(TARGET)/staging/mnt/flash/startupDefaults
	install targetFiles/virt/startup/* -t build/$(TARGET)/staging/mnt/flash/startupDefaults

.PHONY: package_virt
package_virt: package_embedded

# ========== RELEASE ============

# Clean first, then build for localhost and selected embedded targets and generate the documentation
# before packaging it all up into a compressed tarball.
# Partition images for relevant devices are provided as well in the releases/ folder.
.PHONY: release
release: version package.properties clean
	for target in $(RELEASE_TARGETS) docs; do set -e; make $$target; done
	releaselegato

