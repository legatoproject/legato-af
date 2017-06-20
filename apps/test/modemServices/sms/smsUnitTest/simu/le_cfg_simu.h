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
 * Set a simulated integer value for a specific node.
 */
//--------------------------------------------------------------------------------------------------
void le_cfgSimu_SetIntNodeValue
(
    le_cfg_IteratorRef_t iteratorRef,   ///< [IN] Iterator to use as a basis for the transaction.
    const char* path,                   ///< [IN] Path to the target node. Can be an absolute path,
                                        ///<      or a path relative from the iterator's current
                                        ///<      position.
    int32_t value                       ///< [IN] Value to write.
);

//--------------------------------------------------------------------------------------------------
/**
 * Set a simulated boolean value for a specific node.
 */
//--------------------------------------------------------------------------------------------------
void le_cfgSimu_SetBoolNodeValue
(
    le_cfg_IteratorRef_t iteratorRef,   ///< [IN] Iterator to use as a basis for the transaction.
    const char* path,                   ///< [IN] Path to the target node. Can be an absolute path,
                                        ///<      or a path relative from the iterator's current
                                        ///<      position.
    bool value                          ///< [IN] Value to write.
);

#endif // LE_CFGSIMU_INTERFACE_H_INCLUDE_GUARD
