/** @file le_mrc_local.h
 *
 * This file contains the declaration of the MRC Initialization.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_MRC_LOCAL_INCLUDE_GUARD
#define LEGATO_MRC_LOCAL_INCLUDE_GUARD

#include "legato.h"


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the MRC component.
 *
 * @note If the initialization failed, it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_mrc_Init
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Test mcc and mnc strings
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on error
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_TestMccMnc
(
    const char*   mccPtr,      ///< [IN] Mobile Country Code
    const char*   mncPtr       ///< [IN] Mobile Network Code
);

#endif // LEGATO_MRC_LOCAL_INCLUDE_GUARD
