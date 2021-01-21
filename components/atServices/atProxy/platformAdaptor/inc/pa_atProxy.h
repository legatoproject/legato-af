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

#endif // _AT_PROXY_PA_ATPROXY_H_INCLUDE_GUARD_
