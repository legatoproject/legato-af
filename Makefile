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
# Copyright (C) 2013, Sierra Wireless, Inc. All rights reserved. Use of this work is subject to license.
# --------------------------------------------------------------------------------------------------

# List of target devices supported:
TARGETS = localhost ar6 ar7 wp7 raspi

VERSION := $(shell if git describe > /dev/null ; \
                   then \
                       git describe --tags | tee version ; \
                   elif [ -e version ] ; \
                   then \
                       cat version ; \
                   else \
                       echo "unknown" ; \
                       exit 1; \
                   fi)

# By default, build for the localhost and build the documentation.
.PHONY: default
default: localhost docs

export LEGATO_ROOT = $(CURDIR)

# ========== TARGET-SPECIFIC VARIABLES ============

# If the user specified a goal other than "clean", ensure that
#  - COMPILER_DIR is set to the file system path of the directory containing the appropriate
#    build tool chain for the target device.
#  - TARGET is set to the target device family (e.g., ar7).
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


# Goal for building all documentation.
.PHONY: docs user_docs implementation_docs
docs: user_docs implementation_docs

# When building the docs, use the "localhost" target device type.
user_docs: TARGET := localhost
implementation_docs: TARGET := localhost

# Docs for people who don't want to be distracted by the internal implementation details.
user_docs: build/localhost/Makefile
	$(MAKE) -C build/localhost doc
	ln -sf build/localhost/bin/doc/user/html Documentation

# Docs for people who want or need to know the internal implementation details.
implementation_docs: build/localhost/Makefile
	$(MAKE) -C build/localhost implementation_doc


# Rule for how to build the build tools.
.PHONY: tools
tools:
	mkdir -p build/tools && cd build/tools && cmake ../../buildTools && make
	mkdir -p bin
	ln -sf $(CURDIR)/build/tools/bin/mk* bin/
	ln -sf mk bin/mkexe
	ln -sf mk bin/mkapp
	ln -sf $(CURDIR)/framework/tools/scripts/* bin/
	ln -sf $(CURDIR)/framework/tools/ifgen/ifgen bin/


# Rule for creating a build directory
build/%:
	mkdir -p $@
ifeq ($(INCLUDE_AIRVANTAGE), 1)
	mkdir -p build/$(TARGET)/airvantage
endif

# Rule for running the build for a given target using the Makefiles generated by CMake.
# This depends on the build tools and the CMake-generated Makefile.
build_%: tools build/%/Makefile
	make -C build/$(TARGET)
ifeq ($(INCLUDE_AIRVANTAGE), 1)
	make -j`getconf _NPROCESSORS_ONLN` legato_airvantage -C build/$(TARGET)/airvantage
endif
	make -C build/$(TARGET) samples

# Rule for invoking CMake to generate the Makefiles inside the build directory.
# Depends on the build directory being there.
$(foreach target,$(TARGETS),build/$(target)/Makefile): build/%/Makefile: build/%
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
BIN_INSTALL_LIST =  supervisor       \
					serviceDirectory \
					logCtrlDaemon    \
					log              \
					configTree       \
					config           \
					audioDaemon      \
					modemDaemon      \
					posDaemon        \
					dcsDaemon        \
					cnetDaemon       \
					appCfgInstall	 \
					appCfgRemove     \
					data             \
					appCtrl          \
					sim              \
					inspect          \
                                        apn



# List of libraries to install from the build's bin/lib directory to the /usr/local/lib directory
# on the target.
LIB_INSTALL_LIST =  liblegato.so            \
					lible_pa.so             \
					lible_pa_gnss.so        \
					lible_mdm_services.so   \
					lible_mdm_client.so     \
					lible_pos_services.so   \
					lible_pos_client.so     \
					lible_pa_audio.so       \
					lible_audio_services.so \
					lible_audio_client.so   \
					lible_data_client.so    \
					lible_cellnet_client.so    \
					lible_cfg_api.so        \
					lible_cfgAdmin_api.so   \
					libappConfig.so         \
					lible_sup_api.so


# List of libraries to install from the AirVantage Agent lib directory to the /usr/local/lib directory
# on the target.
ifeq ($(INCLUDE_AIRVANTAGE), 1)
AV_AGENT_LIB_INSTALL_LIST = libSwi_AirVantage.so \
							libEmp.so            \
							libyajl.so           \
							libSwi_log.so        \
							libSwi_DSet.so       \
							libSwi_DeviceTree.so \
							libreturncodes.so
endif

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
ifeq ($(INCLUDE_AIRVANTAGE), 1)
	install build/$(TARGET)/bin/lib/lible_tree_hdlr.so -D build/$(TARGET)/staging/usr/local/lib
	install -d build/$(TARGET)/staging/usr/local/agent-runtime
	cp -frPp build/$(TARGET)/airvantage/runtime/* build/$(TARGET)/staging/usr/local/agent-runtime
	mv build/$(TARGET)/staging/usr/local/agent-runtime/bin/agent build/$(TARGET)/staging/usr/local/bin
	install -d build/$(TARGET)/staging/opt/legato/agent
	mv build/$(TARGET)/airvantage/runtime/envVars build/$(TARGET)/staging/opt/legato/agent
	# Install Legato Tree Handler
	mkdir -p build/$(TARGET)/staging/usr/local/agent-runtime/lua/agent/treemgr/handlers/extvars
	cp build/$(TARGET)/bin/lib/lible_tree_hdlr.so build/$(TARGET)/staging/usr/local/agent-runtime/lua/agent/treemgr/handlers/extvars/lible_tree_hdlr.so
	cp interfaces/legatoTreeHandler/c/legatoTreeHdlr.map build/$(TARGET)/staging/usr/local/agent-runtime/resources/legatoTreeHdlr.map
	# move AirVantage Agent public libs to correct folder on the target
	for library in $(AV_AGENT_LIB_INSTALL_LIST) ; \
	do \
	    rm -f build/$(TARGET)/staging/usr/local/agent-runtime/lib/$$library ; \
	    cp --update build/$(TARGET)/airvantage/runtime/lib/$$library build/$(TARGET)/staging/usr/local/lib ; \
	done
endif
	tar -C build/$(TARGET)/staging -cf build/$(TARGET)/legato-runtime.tar .

# Stage AT-related files
.PHONY: stage_at
stage_at:
	install build/$(TARGET)/bin/lib/lible_mdm_atmgr.so -D build/$(TARGET)/staging/usr/local/lib
	install build/$(TARGET)/bin/lib/lible_uart.so -D build/$(TARGET)/staging/usr/local/lib

# ==== AR7 and WP7 (9x15-based Sierra Wireless modules) ====

.PHONY: stage_ar7 stage_wp7
stage_ar7 stage_wp7: stage_common

# ==== AR6 (Ykem-based Sierra Wireless modules) ====

.PHONY: stage_ar6
stage_ar6: stage_common stage_at

# ==== Raspberry Pi ====

.PHONY: stage_raspi
stage_raspi: stage_common stage_at

# ========== RELEASE ============

# Following are the rules for building and packaging a release.

.PHONY: release

# Define a regex specifying a set of directories that must be excluded from releases.
release: EXCLUDE_PATTERN := 'proprietary|qmi|jenkins-build'

# Define a bunch of variables that are only used for release builds.
release: RELEASE_DIR := releases
release: STAGE_NAME := legato-$(VERSION)
release: STAGE_DIR := $(RELEASE_DIR)/$(STAGE_NAME)
release: PKG_NAME := $(STAGE_NAME).tar.bz2

# We never build for coverage testing when building a release.
release: override TEST_COVERAGE=0


# Clean first, then build for localhost and selected embedded targets and generate the documentation
# before packaging it all up into a compressed tarball.
release: clean localhost ar7 wp7 docs
	@echo "Version = $(VERSION)"
	@echo "Preparing package in $(STAGE_DIR)"
	rm -rf $(STAGE_DIR)
	mkdir -p $(STAGE_DIR)
	@echo "version=$(VERSION)" | tee $(STAGE_DIR)/package.properties
	git ls-tree -r --name-only HEAD | egrep -v $(EXCLUDE_PATTERN) | while read LINE ; \
	do \
		install -D $$LINE $(STAGE_DIR)/$$LINE ; \
	done
	install -D build/ar7/bin/lib/lible_pa.so $(STAGE_DIR)/components/modemServices/platformAdaptor/qmi/lib/lible_pa.ar7.so
	install -D build/wp7/bin/lib/lible_pa.so $(STAGE_DIR)/components/modemServices/platformAdaptor/qmi/lib/lible_pa.wp7.so
	install -D build/ar7/bin/lib/lible_pa_gnss.so $(STAGE_DIR)/components/positioning/platformAdaptor/qmi/lib/lible_pa_gnss.ar7.so
	install -D build/wp7/bin/lib/lible_pa_gnss.so $(STAGE_DIR)/components/positioning/platformAdaptor/qmi/lib/lible_pa_gnss.wp7.so
	install -D build/ar7/bin/lib/lible_pa_audio.so $(STAGE_DIR)/components/audio/platformAdaptor/qmi/lib/lible_pa_audio.ar7.so
	install -D build/wp7/bin/lib/lible_pa_audio.so $(STAGE_DIR)/components/audio/platformAdaptor/qmi/lib/lible_pa_audio.wp7.so
	cp -R airvantage $(STAGE_DIR)
	cp version $(STAGE_DIR)
	@echo "Creating release package $(RELEASE_DIR)/$(PKG_NAME)"
	cd $(RELEASE_DIR) && tar jcf $(PKG_NAME) $(STAGE_NAME)
