/** @file le_apiFeatures.h
 *
 * API feature selection for Legato.  This file provides a mapping of KConfig selections to API
 * functions that are available.  As an example, if file system support is disabled, then a
 * compile-time error will be generated if an le_fs function is used.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_APIFEATURES_INCLUDE_GUARD
#define LEGATO_APIFEATURES_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Mark a function as disabled due to a KConfig selection.
 *
 * @param   setting Configuration setting name string.
 */
//--------------------------------------------------------------------------------------------------
#define LE_FUNC_DISABLED(setting) \
    __attribute__((error("Function unavailable due to " setting " configuration")))

//--------------------------------------------------------------------------------------------------
/**
 * Macro used to declare that a symbol is part of the "full" Legato API.
 *
 * There are two APIs for Legato: "full" and "limited".  Linux uses the "full" API and all
 * functions are available.  On other platforms only the "limited" API is supported, and
 * any attempt to use a "full" api function will lead to a compile error.
 */
//--------------------------------------------------------------------------------------------------
#if LE_CONFIG_LINUX
#  define LE_FULL_API
#else
#  define LE_FULL_API LE_FUNC_DISABLED("LINUX")
#endif

/// API requires filesystem support.
#if LE_CONFIG_FILESYSTEM
#  define LE_API_FILESYSTEM
#else
#  define LE_API_FILESYSTEM LE_FUNC_DISABLED("FILESYSTEM")
#endif

#endif /* end LEGATO_APIFEATURES_INCLUDE_GUARD */
