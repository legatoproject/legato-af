
//--------------------------------------------------------------------------------------------------
/**
 * Startup code for the pre-built configuration admin client API library (le_cfg_admin_api.so).
 *
 * Copyright 2013, Sierra Wireless Inc., all rights reserved.
 **/
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "le_cfgAdmin_interface.h"




void le_cfgAdmin_Initialize
(
    void
)
{
    LE_INFO("---- Initializing the configuration administration API ----");
    le_cfgAdmin_StartClient("configTreeAdmin");
    LE_INFO("---- Done ----");
}
