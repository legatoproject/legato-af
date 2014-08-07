//--------------------------------------------------------------------------------------------------
/**
 * Copyright (C) 2014 Sierra Wireless Inc.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"

#include "../inc/atMgr.h"
#include "../inc/atCmdSync.h"
#include "../inc/atPorts.h"


COMPONENT_INIT
{
    if ( atmgr_IsStarted() == false )
    {
        atmgr_Start();

        atcmdsync_Init();

        atports_Init();
    }
}
