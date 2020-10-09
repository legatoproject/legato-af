/**
 * @file atProxySerialUart.c
 *
 * AT Proxy Serial UART implementation.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"
#include "atProxy.h"
#include "atProxyCmdHandler.h"


// File handle to AT Port Serial UART
static int SerialFd;

// Reference to the fd monitor
static le_fdMonitor_Ref_t FdMonitorRef;


//--------------------------------------------------------------------------------------------------
/**
 *  Write to te AT Port External Serial UART
 */
//--------------------------------------------------------------------------------------------------
uint32_t atProxySerialUart_write
(
    char *buf,
    uint32_t len
)
{
    // Verify AT Port handle is initialized
    if (SerialFd == -1)
    {
        return 0;
    }

    return (le_fd_Write(SerialFd, buf, len));
}


//--------------------------------------------------------------------------------------------------
/**
 * Read from the AT Port External Serial UART
 */
//--------------------------------------------------------------------------------------------------
uint32_t atProxySerialUart_read
(
    char *buf,
    uint32_t len
)
{
    // Verify AT Port handle is initialized
    if (SerialFd == -1)
    {
        return 0;
    }

    return (le_fd_Read(SerialFd, buf, len));
}


//--------------------------------------------------------------------------------------------------
/**
 * Initialize the AT Port External Serial UART
 */
//--------------------------------------------------------------------------------------------------
void atProxySerialUart_init(void)
{
    SerialFd = le_fd_Open("ExternalUART", O_RDWR);
    if (SerialFd == -1)
    {
        LE_ERROR("Unable to open AT Port External Serial UART");
        return;
    }

    LE_INFO("Opened Serial device Fd:[%d]", SerialFd);

    int opts = le_fd_Fcntl(SerialFd, F_GETFL);
    if (opts < 0)
    {
        LE_ERROR("le_fd_Fcntl(F_GETFL)");
        le_fd_Close(SerialFd);
        return;
    }
    // Set Non-Blocking socket option
    opts |= O_NONBLOCK;
    if (le_fd_Fcntl(SerialFd, F_SETFL, opts) < 0)
    {
        LE_ERROR("le_fd_Fcntl(F_SETFL)");
        le_fd_Close(SerialFd);
        return;
    }

    // Create thread to monitor FD handle for activity, as defined by the events
    FdMonitorRef =
        le_fdMonitor_Create("externalSerialComm_FD",
                            SerialFd,
                            (le_fdMonitor_HandlerFunc_t) &atProxyCmdHandler_AsyncRecvHandler,
                            POLLIN);
}
