#!/usr/bin/make
#
# Main Makefile for the Legato framework.
#
# TODO: Change to a non-recursive Makefile structure or switch to CMake.
#
# Copyright (C) 2012 - 2013, Sierra Wireless, Inc., all rights reserved.

# Make sure that a target device is specified.
target ?= localhost
TARGET ?= $(target)
export TARGET

# Include the appropriate (cross) build configuration settings for the target device.
include make/Makefile.$(TARGET)

# Define variables to export to other Makefiles so they can find things that they
# need that are located outside of their directories.
export buildRoot := $(CURDIR)
export libDir := $(buildRoot)/lib/$(TARGET)
export objDir := $(buildRoot)/obj/$(TARGET)
export docOutputDir := $(buildRoot)/Documentation
export CUnitDir := $(buildRoot)/../3rdParty/CUnit
export CUnitLibDir := $(CUnitDir)/src/extract/CUnit-2.1-2/CUnit/Sources/.libs
export CUnitIncDir := $(CUnitDir)/src/extract/CUnit-2.1-2/CUnit/Headers

export IMAGE_PATH=$(buildRoot)/images


# Set up our globally standard CFLAGS (standard on all platforms that we build for).
export CFLAGS = -Wall -Werror

# When building with DEBUG defined,
ifdef DEBUG
  # Make sure DEBUG is defined for sub-makes too.
  export DEBUG
  # Include debug symbols in compiler output when building with DEBUG defined.
  CFLAGS += -g
  # When building with DEBUG defined, make sure DEBUG is also defined inside C source files.
  CFLAGS += -DDEBUG
endif


# ======= BUILD RULES =======

.PHONY: all
all: docs build tool

.PHONY: docs
docs:
	export buildRoot=$(buildRoot); export docOutputDir=$(docOutputDir); doxygen doc/doxygen.cfg

.PHONY: build
build:
	make -C c

.PHONY: tools
tools: build
	make -C tools/logTool


# ======= DEPRECATED, EMBEDDED AUTOMATED TEST APP RULES =======
# Note: Test apps are kept separate from the framework itself now.
#		They can be found under /apps/test/framework.

# The CUnit unit testing framework is used by at least one framework module's unit tests.
.PHONY: CUnit
CUnit:
	make -C $(CUnitDir)

.PHONY: embeddedTestApps
embeddedTestApps:
	make -C c test

.PHONY: test
test: CUnit tools embeddedTestApps


# ======= CLEANING RULES =======

apps-clean := $(foreach dir,$(testApps),$(dir).clean)

.PHONY: $(apps-clean)
$(apps-clean):
	make -C $(basename $@) clean

.PHONY: clean
clean: $(apps-clean)
	rm -rf $(docOutputDir)
	make -C c clean
	rm -rf lib
	rm -rf obj
	make -C $(CUnitDir) clean
	make -C tools/logTool clean
