//--------------------------------------------------------------------------------------------------
/**
 * @file userAppMain.c
 *
 * This component is used for testing the AirVantage Controller API.  It simulates a
 * user app that would block or unblock updates.
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"

COMPONENT_INIT
{
    le_avc_BlockRequestRef_t blockRef;

    blockRef = le_avc_BlockInstall();
    if ( blockRef == NULL )
    {
        LE_FATAL("Blocking attempt failed");
    }

    // Block install attempt was successful
    LE_INFO("Got ref = %p", blockRef);

    // wait a bit
    sleep(300);

    le_avc_UnblockInstall(blockRef);

    // Unblock install attempt was successful
    LE_INFO("Finished unblock");

    // Block again, but this time exit after a short while
    blockRef = le_avc_BlockInstall();
    if ( blockRef == NULL )
    {
        LE_FATAL("Blocking attempt failed");
    }

    // Block install attempt was successful
    LE_INFO("Again got ref = %p", blockRef);

    // wait a bit
    sleep(60);

    exit(EXIT_SUCCESS);
}
