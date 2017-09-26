/**
 * @page c_pa_mdmCfg modem configuration Platform Adapter API
 *
 * @ref pa_mdmCfg.h "API Reference"
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc.
 */

//--------------------------------------------------------------------------------------------------
/** @file pa_mdmCfg.h
 *
 * Legato @ref c_pa_mdmCfg include file.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_PA_MDMCFG_INCLUDE_GUARD
#define LEGATO_PA_MDMCFG_INCLUDE_GUARD

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the PA modem configuration module.
 *
 * @return
 *   - LE_FAULT         The function failed to initialize the PA modem configuration module.
 *   - LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_mdmCfg_Init
(
    void
);

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
LE_SHARED le_result_t pa_mdmCfg_StoreCurrentConfiguration
(
    void
);

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
LE_SHARED le_result_t pa_mdmCfg_RestoreSavedConfiguration
(
    void
);

#endif // LEGATO_PA_MDMCFG_INCLUDE_GUARD
