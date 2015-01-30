/** @file le_sim_local.h
 *
 * This file contains the declaration of the SIM Initialization.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#ifndef LEGATO_SIM_LOCAL_INCLUDE_GUARD
#define LEGATO_SIM_LOCAL_INCLUDE_GUARD

#include "legato.h"



//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the SIM operations component
 *
 * @note If the initialization failed, it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_sim_Init
(
    void
);


#endif // LEGATO_SIM_LOCAL_INCLUDE_GUARD
