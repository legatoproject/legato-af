/** @file le_port_local.h
 *
 * Implementation of PortService server API.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_LE_PORT_LOCAL_INCLUDE_GUARD
#define LEGATO_LE_PORT_LOCAL_INCLUDE_GUARD

#if LE_CONFIG_PORT_CONFIG_IS_STATIC
LE_SHARED void le_portLocal_InitStaticCfg
(
    const char *cfgPtr   ///< static configuration to parse
);
#endif

#endif //LEGATO_LE_PORT_LOCAL_INCLUDE_GUARD
