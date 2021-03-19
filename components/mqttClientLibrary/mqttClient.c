/**
 * @file mqttClient.c
 *
 * Common portions of Legato MQTT client library.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "le_mqttClientLib.h"

COMPONENT_INIT
{
    // Initialize the lib
    le_mqttClient_Init();
}
