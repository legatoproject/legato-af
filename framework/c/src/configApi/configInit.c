//--------------------------------------------------------------------------------------------------
/**
 * Startup code for the pre-built configuration client API library (le_cfg_api.so).
 *
 * Copyright 2013, Sierra Wireless Inc., all rights reserved.
 **/
//--------------------------------------------------------------------------------------------------


#include "legato.h"
#include "le_cfg_interface.h"


void le_cfg_Initialize()
{
    LE_DEBUG("---- Initializing the configuration API ----");
    le_cfg_StartClient("configTree");
}
