/**
 * @page c_atports AT Ports
 *
 * This module provide an API that will create all ports needed by Modem Services or others.
 *
 * @section atports_toc Table of Contents
 *
 * - @ref atports_intro
 * - @ref atports_rational
 *      - @ref atports_creation
 *      - @ref atports_utilisation
 *
 * @section atports_intro Introduction
 *
 * This is a layer on the @ref c_atdevice and @ref c_atmgr module.
 *
 * @section atports_rational Rational
 *
 * @subsection atports_creation Initialization
 *
 * when this module is initialized, then you have (@ref ATPORT_MAX - 1) @ref atmgr_Ref_t (thread)
 * that you can manage by @ref c_atmgr:
 *  - @ref atmgr_StartInterface
 *  - @ref atmgr_StopInterface
 *  - @ref atmgr_SubscribeUnsolReq
 *  - @ref atmgr_UnSubscribeUnsolReq
 *  - @ref atmgr_SendCommandRequest
 *  - @ref atmgr_CancelCommandRequest
 *
 * @section atports_utilisation Utilization
 *
 * When needed you can ask an @ref atmgr_Ref_t by calling @ref atports_GetInterface with
 * @ref atports_t
 *
 *
 */

/** @file atPorts.h
 *
 * Legato @ref c_atports include file.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#ifndef LEGATO_ATPORTS_INCLUDE_GUARD
#define LEGATO_ATPORTS_INCLUDE_GUARD

#include "legato.h"
#include "atMgr.h"

#define AT_COMMAND      "/dev/ttyATCMD"
#define AT_PPP          "/dev/ttyPPP"
#define AT_GNSS         "/dev/ttyGNSS"

typedef enum {
    ATPORT_COMMAND, ///< port where AT Command must be send
    ATPORT_PPP,     ///< port that will be used for PPP/Data connection
    ATPORT_GNSS,    ///< port that will be used for gnss data
    ATPORT_MAX      ///< Do not use this one
} atports_t;

//--------------------------------------------------------------------------------------------------
/**
 * This function Initialize the all ports that will be available.
 * Must be call only once
 *
 * @return LE_OK           The function succeeded.
 * @return LE_DUPLICATE    Module is already initialized
 */
//--------------------------------------------------------------------------------------------------
le_result_t atports_Init
(
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get an interface for the given name.
 *
 * @return a reference on the ATManager of this device
 */
//--------------------------------------------------------------------------------------------------
atmgr_Ref_t atports_GetInterface
(
    atports_t name
);

#endif // LEGATO_ATPORTS_INCLUDE_GUARD
