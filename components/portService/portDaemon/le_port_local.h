/** @file le_port_local.h
 *
 * Implementation of PortService server API.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_LE_PORT_LOCAL_INCLUDE_GUARD
#define LEGATO_LE_PORT_LOCAL_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Pipe device path for AT command mode.
 */
//--------------------------------------------------------------------------------------------------
#define DEVICE_PIPE_ATCMD_MODE "/tmp/sock0"

//--------------------------------------------------------------------------------------------------
/**
 * Pipe device path for AT data mode.
 */
//--------------------------------------------------------------------------------------------------
#define DEVICE_PIPE_DATA_MODE "/tmp/sock1"

#if LE_CONFIG_PORT_CONFIG_IS_STATIC
LE_SHARED void le_portLocal_InitStaticCfg
(
    const char *cfgPtr   ///< static configuration to parse
);
#endif

#endif //LEGATO_LE_PORT_LOCAL_INCLUDE_GUARD
