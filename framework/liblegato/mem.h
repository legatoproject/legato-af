//--------------------------------------------------------------------------------------------------
/** @file mem.h
 *
 * Legato memory management module's inter-module include file.
 *
 * This file exposes interfaces that are for use by other modules inside the framework
 * implementation, but must not be used outside of the framework implementation.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */
#ifndef MEM_INCLUDE_GUARD
#define MEM_INCLUDE_GUARD

#include "limit.h"


//--------------------------------------------------------------------------------------------------
/**
 * Initializes the memory pool system.  This function must be called before any other memory pool
 * functions are called.
 *
 * @note
 *      On failure, the process exits.
 */
//--------------------------------------------------------------------------------------------------
void mem_Init
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Exposing the memory pool list; mainly for the Inspect tool.
 */
//--------------------------------------------------------------------------------------------------
le_dls_List_t* mem_GetPoolList
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Exposing the memory pool list change counter; mainly for the Inspect tool.
 */
//--------------------------------------------------------------------------------------------------
size_t** mem_GetPoolListChgCntRef
(
    void
);

#if LE_CONFIG_RTOS
//--------------------------------------------------------------------------------------------------
/**
 * Exposing the memory pool mutex lock; mainly for the Inspect tool on RTOS.
 */
//--------------------------------------------------------------------------------------------------
void mem_Lock
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Exposing the memory pool mutex unlock; mainly for the Inspect tool on RTOS.
 */
//--------------------------------------------------------------------------------------------------
void mem_Unlock
(
    void
);
#endif /* end LE_CONFIG_RTOS */

#endif  // MEM_INCLUDE_GUARD
