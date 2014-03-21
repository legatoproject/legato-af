/**
 * @file le_ms.c
 *
 * This file contains the source code of Modem Services Initialization.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */


#include "legato.h"
#include "pa.h"
#include "le_mrc_local.h"
#include "le_mcc_local.h"
#include "le_sim_local.h"
#include "le_sms_local.h"
#include "le_mdc_local.h"
#include "le_cfg_interface.h"

// This prototype is needed to register Modem Services as a component.
// todo: This is temporary until the build tools do this automatically
le_log_SessionRef_t log_RegComponent
(
    const char* componentNamePtr,       ///< [IN] A pointer to the component's name.
    le_log_Level_t** levelFilterPtrPtr  ///< [OUT] Set to point to the component's level filter.
);


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the Modem Services.
 *
 * @note If the initialization failed, it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_ms_Init
(
    void
)
{
    // Register Modem Services as a component.
    // todo: This is temporary until the build tools do this automatically
    LE_LOG_SESSION = log_RegComponent(STRINGIZE(LE_COMPONENT_NAME), &LE_LOG_LEVEL_FILTER_PTR);

    le_cfg_Initialize();

    pa_Init();

    le_mrc_Init();
    le_sim_Init();
    le_sms_msg_Init();
    le_mdc_Init();
    le_mcc_Init();
}

