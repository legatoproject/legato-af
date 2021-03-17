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
 * This function enables extended error codes on the selected device
 *
 * @return none
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void pa_atProxy_EnableExtendedErrorCodes
(
    void
)
{
}

//--------------------------------------------------------------------------------------------------
/**
 * This function disables the current error codes mode on the selected device
 *
 * @return none
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void pa_atProxy_DisableExtendedErrorCodes
(
    void
)
{
}

//--------------------------------------------------------------------------------------------------
/**
 * This function retrieves the current error codes mode on the selected device
 *
 * @return
 *      - MODE_EXTENDED  If extended error code is enabled
 *      - MODE_VERBOSE   If extended verbose error code is enabled (NOTE: Not Supported)
 *      - MODE_DISABLED  If extended error code is disabled
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED ErrorCodesMode_t pa_atProxy_GetExtendedErrorCodes
(
    void
)
{
    return MODE_EXTENDED;
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
