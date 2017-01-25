/**
 * @file avData.h
 *
 * Interface for avData sub-component
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#ifndef LEGATO_AVDATA_INCLUDE_GUARD
#define LEGATO_AVDATA_INCLUDE_GUARD

#include "legato.h"

//--------------------------------------------------------------------------------------------------
// Definitions.
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
// Interface functions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Init this sub-component
 */
//--------------------------------------------------------------------------------------------------
void avData_Init
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Called by avcServer when the session state is SESSION_STARTED or SESSION_STOPPED.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void avData_ReportSessionState
(
    le_avdata_SessionState_t sessionState
);

#endif // LEGATO_AVDATA_INCLUDE_GUARD
