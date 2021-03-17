/**
 * @file pa_atProxy.h
 *
 * AT Proxy platform adaptor for platform specific implementation.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef _AT_PROXY_PA_ATPROXY_H_INCLUDE_GUARD_
#define _AT_PROXY_PA_ATPROXY_H_INCLUDE_GUARD_

#include "legato.h"
#include "atProxy.h"


//--------------------------------------------------------------------------------------------------
/**
 * Register an AT command to firmware for allowing forwarding
 *
 * @return
 *      - LE_FAULT          registration failed
 *      - LE_OK             registration successful
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_atProxy_Register
(
    const char* commandStr  ///< [IN] AT command to be registered to firmware
);

//--------------------------------------------------------------------------------------------------
/**
 * This function enables extended error codes on the selected device
 *
 * @return none
 */
//--------------------------------------------------------------------------------------------------
void pa_atProxy_EnableExtendedErrorCodes
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * This function disables the current error codes mode on the selected device
 *
 * @return none
 */
//--------------------------------------------------------------------------------------------------
void pa_atProxy_DisableExtendedErrorCodes
(
    void
);

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
ErrorCodesMode_t pa_atProxy_GetExtendedErrorCodes
(
    void
);


#endif // _AT_PROXY_PA_ATPROXY_H_INCLUDE_GUARD_
