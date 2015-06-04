#
# Makefile for the AirVantage Agent application.
#
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.

.PHONY: no_goal
no_goal:
	$(error "NO MAKE GOAL SPECIFIED.  Expect either 'pre_build' or 'package'.")
ifeq ($(LEGATO_ROOT),)
    $(error LEGATO_ROOT not defined)
endif

ifeq ($(TARGET),)
    $(error TARGET not defined)
endif

APP_OUTPUT_DIR := $(LEGATO_ROOT)/build/$(TARGET)/bin/apps
LIB_OUTPUT_DIR := $(LEGATO_ROOT)/build/$(TARGET)/bin/lib
APP_WORKING_DIR := $(LEGATO_ROOT)/build/$(TARGET)/apps/airvantage
APP := $(APP_OUTPUT_DIR)/airvantage.$(TARGET)

LE_DATA_CLIENT := $(LIB_OUTPUT_DIR)/lible_data_client.so

LE_TREE_HDLR := $(LIB_OUTPUT_DIR)/lible_tree_hdlr.so


.PHONY: pre_build
pre_build: $(LE_DATA_CLIENT) $(LE_TREE_HDLR)

.PHONY: package
package: $(APP)


# Use mkcomp to build a component library for the le_data_client.
# Give it the --stand-alone option to generate and link with interface instance libs so that
# the library can be loaded dynamically by the agent (using dlopen()) without having pre-linked
# with it.
# TODO: Remove dependency list when mkcomp does dependency checking.
$(LE_DATA_CLIENT):	le_data_client/clientMain.c \
				le_data_client/Component.cdef \
				$(LEGATO_ROOT)/interfaces/dataConnectionService/le_data.api

	mkcomp le_data_client --stand-alone -v -t $(TARGET) \
				-o $(LIB_OUTPUT_DIR)/lible_data_client.so \
				-w $(APP_WORKING_DIR)/le_data_client \
				-l $(LIB_OUTPUT_DIR) \
				-i $(LEGATO_ROOT)/interfaces/dataConnectionService


# Use mkcomp to build a component library for the le_tree_hdlr.
# Give it the --stand-alone option to generate and link with interface instance libs so that
# the library can be loaded dynamically by the agent (using dlopen()) without having pre-linked
# with it.
# TODO: Remove dependency list when mkcomp does dependency checking.
$(LE_TREE_HDLR): le_tree_hdlr/Component.cdef \
				le_tree_hdlr/extvars.c \
				$(LEGATO_ROOT)/interfaces/modemServices/le_mdmDefs.api \
				$(LEGATO_ROOT)/interfaces/modemServices/le_info.api \
				$(LEGATO_ROOT)/interfaces/modemServices/le_mdc.api \
				$(LEGATO_ROOT)/interfaces/modemServices/le_mrc.api \
				$(LEGATO_ROOT)/interfaces/modemServices/le_sim.api \
				$(LEGATO_ROOT)/interfaces/positioning/le_pos.api \
				$(LEGATO_ROOT)/interfaces/positioning/le_posCtrl.api \
				$(LEGATO_ROOT)/interfaces/dataConnectionService/le_data.api

	mkcomp le_tree_hdlr --stand-alone -v -t $(TARGET) \
				-o $(LIB_OUTPUT_DIR)/lible_tree_hdlr.so \
				-w $(APP_WORKING_DIR)/le_tree_hdlr \
				-l $(LIB_OUTPUT_DIR) \
				-i $(LEGATO_ROOT)/airvantage/libs/common \
				-i $(LEGATO_ROOT)/airvantage/agent/agent/treemgr/handlers/extvars \
				-i $(LEGATO_ROOT)/interfaces/modemServices \
				-i $(LEGATO_ROOT)/interfaces/positioning \
				-i $(LEGATO_ROOT)/interfaces/dataConnectionService


# Use mkapp to package everything up into a deployable application file.
# TODO: Remove dependencies from the Makefile when the mk tools do dependency checking.
$(APP):	$(APP_OUTPUT_DIR) \
		airvantage.adef \
		$(LE_DATA_CLIENT) \
		$(LE_TREE_HDLR)

	mkapp airvantage.adef	-t $(TARGET) -v -o $(APP_OUTPUT_DIR) \
							-w $(APP_WORKING_DIR) \
							-i $(LEGATO_ROOT)/airvantage/libs/common \
							-i $(LEGATO_ROOT)/airvantage/agent/agent/treemgr/handlers/extvars

$(APP_WORKING_DIR) $(APP_OUTPUT_DIR):
	mkdir -p $@
