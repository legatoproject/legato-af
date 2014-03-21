//--------------------------------------------------------------------------------------------------
/** @file sandbox.h
 *
 * API for creating Legato Sandboxes.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */
#ifndef LEGATO_SRC_SANDBOX_INCLUDE_GUARD
#define LEGATO_SRC_SANDBOX_INCLUDE_GUARD


#include "app.h"


//--------------------------------------------------------------------------------------------------
/**
 * The debugging program to use when in debug mode.
 */
//--------------------------------------------------------------------------------------------------
#define DEBUG_PROGRAM               "/usr/bin/gdbserver"


//--------------------------------------------------------------------------------------------------
/**
 * Gets the sandbox location path string.  The sandbox does not have to exist before this function
 * is called.  This function gives the expected location of the sandbox by simply appending the
 * appName to the sandbox root path.
 *
 * @return
 *      LE_OK if successful.
 *      LE_OVERFLOW the provided buffer is too small.
 */
//--------------------------------------------------------------------------------------------------
le_result_t sandbox_GetPath
(
    const char* appName,    ///< [IN] The application name.
    char* pathBuf,          ///< [OUT] The buffer that will contain the path string.
    size_t pathBufSize      ///< [IN] The buffer size.
);


//--------------------------------------------------------------------------------------------------
/**
 * Sets up an application's sandbox.  This function looks at the settings in the config tree and
 * sets up the application's sandbox area.
 *
 *  - Creates the sandbox directory.
 *  - Imports all needed files (libraries, executables, config files, socket files, device files).
 *  - Import syslog socket.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t sandbox_Setup
(
    app_Ref_t appRef                ///< [IN] The application to setup the sandbox for.
);


//--------------------------------------------------------------------------------------------------
/**
 * Removes an applications sandbox.  Deletes everything in the sandbox area and the sandbox itself.
 * All processes in the sandbox must be killed prior to calling this function.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t sandbox_Remove
(
    app_Ref_t appRef                ///< [IN] The application to remove the sandbox for.
);


//--------------------------------------------------------------------------------------------------
/**
 * Confines the calling process into the sandbox.
 *
 * @note This function kills the calling process if there is an error.
 */
//--------------------------------------------------------------------------------------------------
void sandbox_ConfineProc
(
    const char* sandboxRootPtr, ///< [IN] Path to the sandbox root.
    uid_t uid,                  ///< [IN] The user ID the process should be set to.
    gid_t gid,                  ///< [IN] The group ID the process should be set to.
    const char* workingDirPtr   ///< [IN] The directory the process's working directory will be set
                                ///       to relative to the sandbox root.
);



#endif  // LEGATO_SRC_SANDBOX_INCLUDE_GUARD
