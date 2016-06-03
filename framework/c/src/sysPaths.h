//--------------------------------------------------------------------------------------------------
/** @file sysPaths.h
 *
 * This file defines several common system paths. Currently they are simply #defined as strings
 * in this header.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 *
 */
#ifndef LEGATO_SYSPATHS_INCLUDE_GUARD
#define LEGATO_SYSPATHS_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * The path to the directory containing the current running "system".
 */
//--------------------------------------------------------------------------------------------------
#define CURRENT_SYSTEM_PATH         "/legato/systems/current"

//--------------------------------------------------------------------------------------------------
/**
 * The location where all applications are installed.
 */
//--------------------------------------------------------------------------------------------------
#define APPS_INSTALL_DIR            CURRENT_SYSTEM_PATH"/apps"

//--------------------------------------------------------------------------------------------------
/**
 * The writeable location for all installed applications.
 */
//--------------------------------------------------------------------------------------------------
#define APPS_WRITEABLE_DIR          CURRENT_SYSTEM_PATH"/appsWriteable"

//--------------------------------------------------------------------------------------------------
/**
 * The location where all system binaries are installed.
 */
//--------------------------------------------------------------------------------------------------
#define SYSTEM_BIN_PATH             CURRENT_SYSTEM_PATH"/bin"

//--------------------------------------------------------------------------------------------------
/**
 * The location where all kernel modules are installed.
 */
//--------------------------------------------------------------------------------------------------
#define SYSTEM_MODULE_PATH          CURRENT_SYSTEM_PATH"/modules"

//--------------------------------------------------------------------------------------------------
/**
 * The location of config tree directory.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_TREE_PATH               CURRENT_SYSTEM_PATH"/config"


#endif  // LEGATO_SYSPATHS_INCLUDE_GUARD
