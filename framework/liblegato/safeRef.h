/** @file safeRef.h
 *
 * Safe References module's inter-module interface include file.
 *
 * This file defines interfaces that are for use by other modules in the framework implementation.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_SRC_SAFEREF_H_INCLUDE_GUARD
#define LEGATO_SRC_SAFEREF_H_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the Safe Reference Module.
 *
 * This function must be called exactly once at process start-up, before any Safe Reference API
 * functions are called.
 */
//--------------------------------------------------------------------------------------------------
#define safeRef_Init()

//--------------------------------------------------------------------------------------------------
/**
 * Exposing the ref map list; mainly for the Inspect tool.
 */
//--------------------------------------------------------------------------------------------------
le_dls_List_t *ref_GetRefMapList
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Exposing the ref map list change counter; mainly for the Inspect tool.
 */
//--------------------------------------------------------------------------------------------------
size_t **ref_GetRefMapListChgCntRef
(
    void
);

#endif // LEGATO_SRC_SAFEREF_H_INCLUDE_GUARD
