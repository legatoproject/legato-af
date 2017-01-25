/**
 * @file le_mcc_local.h
 *
 * Local Modem Call Control (MCC) Definitions
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_MCC_LOCAL_INCLUDE_GUARD
#define LEGATO_MCC_LOCAL_INCLUDE_GUARD

#include "legato.h"


//--------------------------------------------------------------------------------------------------
/**
 * Initialize the MCC component.
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mcc_Init
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * This function returns the call identifier number.
 *
 *
 * @return
 *    - LE_OK        The function succeed.
 *    - LE_NOT_FOUND The call reference was not found.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mcc_GetCallIdentifier
(
    le_mcc_CallRef_t callRef,       ///< [IN] The call reference.
    int32_t* callIdPtr             ///< [OUT] Call identifier
);

#endif // LEGATO_MCC_LOCAL_INCLUDE_GUARD

