/**
 * @file pa_atProxy.c
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "pa_atProxy.h"
#include "pa_port.h"

//--------------------------------------------------------------------------------------------------
/**
 * Register an AT command to firmware for allowing forwarding
 *
 * @return
 *      - LE_FAULT          registration failed
 *      - LE_OK             registration successful
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_atProxy_Register
(
    const char* commandStr  ///< [IN] AT command to be registered to firmware
)
{
    LE_UNUSED(commandStr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Component initializer automatically called by the application framework when the process starts.
 *
 **/
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("Start the atProxy PA initialization.");
}
