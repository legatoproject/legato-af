/**
 * @page c_pa_remoteMgmt Wakup Up API
 *
 * This file contains the prototypes definitions of the wake up message from the Modem Platform.
 *
 *
 * @section pa_remoteMgmt_toc Table of Contents
 *
 *  - @ref pa_remoteMgmt_intro
 *  - @ref pa_remoteMgmt_rational
 *
 *
 * @section pa_remoteMgmt_intro Introduction
 * These APIs are on the top of the platform-dependent adapter layer. They are independent of the
 * implementation. They guarantee the portability on different kind of platform without any changes
 * for the components developped upon these APIs.
 *
 *
 * @section pa_remoteMgmt_rational Rational
 * These functions are all blocking functions, so that they return when the modem has answered or
 * when a timeout has occured due to an interrupted communication with the modem.
 *
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */


/** @file pa_remoteMgmt.h
 *
 * Legato @ref c_pa_remoteMgmt include file.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#ifndef LEGATO_PA_REMOTE_MGMT_INCLUDE_GUARD
#define LEGATO_PA_REMOTE_MGMT_INCLUDE_GUARD

#include "legato.h"

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
/**
 * Prototype for handler functions used to report message wake up Indication.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef void (*pa_remoteMgmt_MessageWakeupHdlrFunc_t)
(
    void
);

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
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to enable/disable the firmware update.
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_remoteMgmt_FirmwareUpdateActivate
(
    bool toActivate
);

#endif // LEGATO_PA_REMOTE_MGMT_INCLUDE_GUARD
