//--------------------------------------------------------------------------------------------------
/**
 * @file pa_mdmCfg_default.c
 *
 * Default implementation of @ref pa_mdmCfg interface
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

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
 *  - LE_UNSUPPORTED    The function does not supported on this platform.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdmCfg_StoreCurrentConfiguration
(
    void
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Restore previously saved the modem configuration.
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_FAULT          The function failed.
 *  - LE_UNSUPPORTED    The function does not supported on this platform.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdmCfg_RestoreSavedConfiguration
(
    void
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the PA modem configuration module.
 *
 * @return
 *   - LE_FAULT         The function failed to initialize the PA modem configuration module.
 *   - LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdmCfg_Init
(
    void
)
{
    return LE_OK;
}
