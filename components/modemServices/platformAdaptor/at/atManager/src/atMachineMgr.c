/** @file atmgr.c
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#include "legato.h"
#include "atMachineCommand.h"
#include "atMachineMgrItf.h"
#include "atMachineParser.h"
#include "atMachineFsm.h"
#include "atMachineString.h"
#include "atMachineUnsolicited.h"

static bool IsStarted = false;

// This prototype is needed to register Modem Services as a component.
// todo: This is temporary until the build tools do this automatically
le_log_SessionRef_t log_RegComponent
(
    const char* componentNamePtr,       ///< [IN] A pointer to the component's name.
    le_log_Level_t** levelFilterPtr     ///< [OUT] Set to point to the component's level filter.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function init the ATManager.
 */
//--------------------------------------------------------------------------------------------------
void atmgr_Start
(
    void
)
{
    // Register ATManager as a component.
    // todo: This is temporary until the build tools do this automatically
    LE_LOG_SESSION = log_RegComponent(STRINGIZE(LE_COMPONENT_NAME), &LE_LOG_LEVEL_FILTER_PTR);

    atmachinemgritf_Init();
    atmachinecommand_Init();
    atmachinestring_Init();
    atmachineunsolicited_Init();

    IsStarted = true;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to check it the ATManager is started
 *
 * @return true if the ATManager is started, false otherwise.
 */
//--------------------------------------------------------------------------------------------------
bool atmgr_IsStarted
(
    void
)
{
    return IsStarted;
}
