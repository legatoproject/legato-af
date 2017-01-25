/**
 * @file le_riPin_local.h
 *
 * Local the Ring Indicator signal API Definitions
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_RIPIN_LOCAL_INCLUDE_GUARD
#define LEGATO_RIPIN_LOCAL_INCLUDE_GUARD

#include "legato.h"


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the riPin service
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_riPin_Init
(
    void
);


#endif // LEGATO_RIPIN_LOCAL_INCLUDE_GUARD

