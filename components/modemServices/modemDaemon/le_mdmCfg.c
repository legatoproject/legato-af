/**
 * @file le_mdmCfg.c
 *
 * Implementation of Modem Configuration API
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */


#include "legato.h"
#include "interfaces.h"
#include "pa_mdmCfg.h"

//--------------------------------------------------------------------------------------------------
/**
 * Store the modem current configuration.
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_FAULT          The function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mdmCfg_StoreCurrentConfiguration
(
    void
)
{
    return pa_mdmCfg_StoreCurrentConfiguration();
}

//--------------------------------------------------------------------------------------------------
/**
 * Restore previously saved the modem configuration.
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_FAULT          The function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mdmCfg_RestoreSavedConfiguration
(
    void
)
{
    return pa_mdmCfg_RestoreSavedConfiguration();
}
