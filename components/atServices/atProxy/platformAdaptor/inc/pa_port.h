/**
 * @file pa_port.h
 *
 * AT Proxy platform adaptor for port communication.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef _AT_PROXY_PA_PORT_H_INCLUDE_GUARD_
#define _AT_PROXY_PA_PORT_H_INCLUDE_GUARD_

#include "legato.h"

typedef struct le_atProxy_Port* le_atProxy_PortRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * Perform platform-specific initialization for AT port
 *
 * @return
 *      - LE_FAULT          initialization failed
 *      - LE_OK             initialization successful
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_port_Init
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Open an AT port
 *
 * @return the created port reference if successful, NULL otherwise
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_atProxy_PortRef_t pa_port_Open
(
    const char* portName        ///< [IN] Port name string
);

//--------------------------------------------------------------------------------------------------
/**
 * Write to the given AT Port
 *
 * @return the number of bytes written if successful, -1 otherwise
 */
//--------------------------------------------------------------------------------------------------
ssize_t pa_port_Write
(
    le_atProxy_PortRef_t port,  ///< [IN] Port to write to
    char *buf,                  ///< [IN] Buffer of data needs to write
    uint32_t len                ///< [IN] Length of buffer
);

//--------------------------------------------------------------------------------------------------
/**
 * Read from the given AT Port
 *
 * @return the number of bytes read if successful, otherwise -1 and errno is set
 */
//--------------------------------------------------------------------------------------------------
ssize_t pa_port_Read
(
    le_atProxy_PortRef_t port,  ///< [IN] Port to read from
    char *buf,                  ///< [IN] Buffer for reading data
    uint32_t len                ///< [IN] Length of buffer
);

//--------------------------------------------------------------------------------------------------
/**
 * Disable receiving on AT Port
 *
 * @return none
 */
//--------------------------------------------------------------------------------------------------
void pa_port_Disable
(
    le_atProxy_PortRef_t port   ///< [IN] Port to disable events
);

//--------------------------------------------------------------------------------------------------
/**
 * Enable receiving on AT Port
 *
 * @return none
 */
//--------------------------------------------------------------------------------------------------
void pa_port_Enable
(
    le_atProxy_PortRef_t port   ///< [IN] Port to enable events
);


#endif // _AT_PROXY_PA_PORT_H_INCLUDE_GUARD_
