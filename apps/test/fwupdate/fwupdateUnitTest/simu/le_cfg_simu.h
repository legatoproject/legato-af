/** @file le_cfg_simu.h
 *
 * Legato @ref le_cfg_simu include file.
 *
 * Copyright (C) Sierra Wireless Inc.
 */


#ifndef LE_CFGSIMU_INTERFACE_H_INCLUDE_GUARD
#define LE_CFGSIMU_INTERFACE_H_INCLUDE_GUARD


#include "legato.h"

//--------------------------------------------------------------------------------------------------
/**
 * Set a system version in the simulated configtree
 *
 */
//--------------------------------------------------------------------------------------------------
void le_cfgSimu_SetSystemVersion
(
    const char* systemVersion,
        ///< [IN]
        ///< System version name string

    const char* version
        ///< [IN]
        ///< System version string
);


#endif // LE_CFGSIMU_INTERFACE_H_INCLUDE_GUARD

