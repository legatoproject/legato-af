//--------------------------------------------------------------------------------------------------
/** @file execInApp/sandbox.h
 *
 * Temporary solution until command apps are available.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
#ifndef LEGATO_SRC_SANDBOX_INCLUDE_GUARD
#define LEGATO_SRC_SANDBOX_INCLUDE_GUARD



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
    const gid_t* groupsPtr,     ///< [IN] List of supplementary groups for this process.
    size_t numGroups,           ///< [IN] The number of groups in the supplementary groups list.
    const char* workingDirPtr   ///< [IN] The directory the process's working directory will be set
                                ///       to relative to the sandbox root.
);



#endif  // LEGATO_SRC_SANDBOX_INCLUDE_GUARD
