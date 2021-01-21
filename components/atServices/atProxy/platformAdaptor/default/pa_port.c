/**
 * @file pa_port.c
 *
 * AT Proxy port communication implementation.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "pa_port.h"


//--------------------------------------------------------------------------------------------------
/**
 * Perform platform-specific initialization for AT port
 *
 * @return
 *      - LE_FAULT          initialization failed
 *      - LE_OK             initialization successful
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_port_Init
(
    void
)
{
    return LE_OK;
}

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
)
{
    LE_UNUSED(portName);
    LE_WARN("Not implemented!");

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Write to the given AT Port
 *
 * @return the number of bytes written if successful, -1 otherwise
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED ssize_t pa_port_Write
(
    le_atProxy_PortRef_t port,  ///< [IN] Port to write to
    char *buf,                  ///< [IN] Buffer of data needs to write
    uint32_t len                ///< [IN] Length of buffer
)
{
    LE_UNUSED(port);
    LE_UNUSED(buf);
    LE_UNUSED(len);

    return -1;
}

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
)
{
    LE_UNUSED(port);
    LE_UNUSED(buf);
    LE_UNUSED(len);

    return -1;
}

//--------------------------------------------------------------------------------------------------
/**
 * Disable receiving on AT Port
 *
 * @return none
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void pa_port_Disable
(
    le_atProxy_PortRef_t port   ///< [IN] Port to disable events
)
{
    LE_UNUSED(port);

    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Enable receiving on AT Port
 *
 * @return none
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void pa_port_Enable
(
    le_atProxy_PortRef_t port   ///< [IN] Port to enable events
)
{
    LE_UNUSED(port);

    return;
}
