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

#endif // LEGATO_SRC_SAFEREF_H_INCLUDE_GUARD
