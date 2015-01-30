/**
 * @file pa_remoteMgmt.c
 *
 * AT implementation of @ref c_pa_remoteMgmt API.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "pa_remoteMgmt.h"

//--------------------------------------------------------------------------------------------------
// Symbols and enums.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Data structures.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
//                                       Static declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// APIs.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register a handler for Message Wake Up handling.
 *
 * If the handler is NULL, then the previous handler will be removed.
 *
 * @note
 *      The process exits on failure
 */
//--------------------------------------------------------------------------------------------------
void pa_remoteMgmt_SetMessageWakeUpHandler
(
    pa_remoteMgmt_MessageWakeupHdlrFunc_t handlerRef ///< [IN] The handler function to handle
                                                     ///       the Message Wake Up
)
{
    // @TODO implementation
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to enable/disable the firmware update.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
void pa_remoteMgmt_FirmwareUpdateActivate
(
    bool toActivate
)
{
    // @TODO implementation
}
