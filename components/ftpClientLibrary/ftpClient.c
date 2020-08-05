/**
 * @file ftpClient.c
 *
 * Common portions of Legato FTP client library.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"
#include "src/le_ftpClientLib.h"

COMPONENT_INIT_ONCE
{
    InitFtpClientComponent();
}

COMPONENT_INIT
{
    // do nothing
}
