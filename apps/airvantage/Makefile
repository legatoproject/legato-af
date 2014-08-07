#
# Makefile for the AirVantage Agent application.
#
# Copyright 2014, Sierra Wireless Inc., Use of this work is subject to license.

ifeq ($(LEGATO_ROOT),)
    $(error LEGATO_ROOT not defined)
endif

ifeq ($(TARGET),)
    $(error TARGET not defined)
endif

APP_OUTPUT_DIR := $(LEGATO_ROOT)/build/$(TARGET)/bin/apps
APP_WORKING_DIR := $(LEGATO_ROOT)/build/$(TARGET)/apps/airvantage
APP := $(APP_OUTPUT_DIR)/airvantage.$(TARGET)


# TODO: Remove dependencies from the Makefile when the mk tools do dependency checking.
$(APP):	$(APP_OUTPUT_DIR) \
		airvantage.adef \
		$(LEGATO_ROOT)/interfaces/modemServices/le_mrc.api \
		$(LEGATO_ROOT)/interfaces/modemServices/le_info.api \
		$(LEGATO_ROOT)/interfaces/modemServices/le_sms.api \
		$(LEGATO_ROOT)/interfaces/modemServices/le_mrc.api \
		$(LEGATO_ROOT)/interfaces/modemServices/le_sim.api \
		$(LEGATO_ROOT)/interfaces/modemServices/le_mdc.api \
		$(LEGATO_ROOT)/interfaces/modemServices/le_mcc_profile.api \
		$(LEGATO_ROOT)/interfaces/modemServices/le_mcc_call.api \
		$(LEGATO_ROOT)/interfaces/audio/le_audio.api \
		$(LEGATO_ROOT)/interfaces/dataConnectionService/le_data.api \
		$(LEGATO_ROOT)/interfaces/positioning/le_pos.api \
		$(LEGATO_ROOT)/components/airvantage/le_data_client/clientMain.c \
		$(LEGATO_ROOT)/components/airvantage/le_data_client/Component.cdef \
		$(LEGATO_ROOT)/components/airvantage/le_tree_hdlr/Component.cdef \
		$(LEGATO_ROOT)/components/airvantage/le_tree_hdlr/extvars.c

	mkapp airvantage.adef	-t $(TARGET) -v -o $(APP_OUTPUT_DIR) \
							-w $(APP_WORKING_DIR) \
							-c components \
							-i $(LEGATO_ROOT)/airvantage/libs/common \
							-i $(LEGATO_ROOT)/airvantage/agent/agent/treemgr/handlers/extvars \
							-i $(LEGATO_ROOT)/interfaces/modemServices \
							-i $(LEGATO_ROOT)/interfaces/positioning \
							-i $(LEGATO_ROOT)/interfaces/audio \
							-i $(LEGATO_ROOT)/interfaces/dataConnectionService

$(APP_WORKING_DIR) $(APP_OUTPUT_DIR):
	mkdir -p $@
