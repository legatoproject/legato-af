//--------------------------------------------------------------------------------------------------
/** @file appSmack.h
 *
 * Internal API for getting application information related to SMACK.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
#ifndef LEGATO_SRC_APP_SMACK_INCLUDE_GUARD
#define LEGATO_SRC_APP_SMACK_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Gets the application name of the process with the specified PID.
 *
 * @return
 *      LE_OK if the application name was successfully found.
 *      LE_OVERFLOW if the application name could not fit in the provided buffer.
 *      LE_NOT_FOUND if the process is not part of an application.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t appSmack_GetName
(
    int pid,                ///< [IN] PID of the process.
    char* bufPtr,           ///< [OUT] Buffer to hold the name of the app.
    size_t bufSize          ///< [IN] Size of the buffer.
);

#endif  // LEGATO_SRC_APP_SMACK_INCLUDE_GUARD
