/**
 * @file atProxyAdaptor.c
 *
 * AT Proxy adaptor file, includes platform-specific functions
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"
#include "atProxyCmdHandler.h"
#include "atProxySerialUart.h"
#include "atProxyRemote.h"

//--------------------------------------------------------------------------------------------------
/**
 * Perform platform initialization
 *
 * @return none
 */
//--------------------------------------------------------------------------------------------------
void le_atProxy_init(void)
{
#if NO_EXTERNAL_STDOUT_PORT
    // Initialize the AT Port External Serial UART
    atProxySerialUart_init();

    // Initialize the AT Command Handler
    atProxyCmdHandler_init();

    // Initialize the Emux channel
    atProxyRemote_init();
#else
    LE_WARN("Unable to initialize AT Proxy");
#endif
}
