/**
 * @file ftpClient.c
 *
 * Common portions of Legato FTP client library.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"

#ifdef LE_CONFIG_LWIP
#   include "lwftp/impl.h"
#endif

COMPONENT_INIT_ONCE
{
    InitFtpClientComponent();
}

COMPONENT_INIT
{
    // do nothing
}
