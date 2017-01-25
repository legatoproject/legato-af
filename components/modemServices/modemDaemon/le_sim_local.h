/** @file le_sim_local.h
 *
 * This file contains the declaration of the SIM Initialization.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_SIM_LOCAL_INCLUDE_GUARD
#define LEGATO_SIM_LOCAL_INCLUDE_GUARD

#include "legato.h"



//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the SIM operations component
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_Init
(
    void
);


#endif // LEGATO_SIM_LOCAL_INCLUDE_GUARD
