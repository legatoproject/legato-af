/**
 * @page pa_clkSync Clock Sync Service Adapter API
 *
 * @ref pa_clkSync.h "API Reference"
 *
 * <HR>
 *
 * @section pa_clkSync_toc Table of Contents
 *
 *  - @ref pa_clkSync_intro
 *  - @ref pa_clkSync_rational
 *
 *
 * @section pa_clkSync_intro Introduction
 *
 * As Sierra Wireless is moving into supporting multiple OS platforms,
 * we need to abstract the Clock Sync Service layer
 *
 * @section pa_clkSync_rational Rational
 * Up to now, only Linux OS was supported. Now, as support for RTOS
 * and other OSs is being made available, there is a need for
 * this kind of platform adapter
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc.
 */


#ifndef LEGATO_PA_CLKSYNC_INCLUDE_GUARD
#define LEGATO_PA_CLKSYNC_INCLUDE_GUARD

#include "legato.h"
#include "interfaces.h"


//--------------------------------------------------------------------------------------------------
/**
 * Retrieve time from a server using the Time Protocol.
 *
 * @return
 *      - LE_OK             Function succeeded to get (if getOnly) or update clock time
 *      - LE_BAD_PARAMETER  Incorrect parameter
 *      - LE_NOT_FOUND      Given server as name or address not found or resolvable into an IP addr
 *      - LE_UNAVAILABLE    No current clock time retrieved from the given server
 *      - LE_UNSUPPORTED    Function not supported by the target
 *      - LE_FAULT          Function failed to get clock time
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_clkSync_GetTimeWithTimeProtocol
(
    const char* serverStrPtr,       ///< [IN]  Time server
    bool getOnly,                   ///< [IN]  Get the time acquired without updating system clock
    le_clkSync_ClockTime_t* timePtr ///< [OUT] Time structure
);


//--------------------------------------------------------------------------------------------------
/**
 * Retrieve time from a server using the Network Time Protocol.
 *
 * @return
 *      - LE_OK             Function succeeded to get (if getOnly) or update clock time
 *      - LE_BAD_PARAMETER  Incorrect parameter
 *      - LE_NOT_FOUND      Given server as name or address not found or resolvable into an IP addr
 *      - LE_UNAVAILABLE    No current clock time retrieved from the given server
 *      - LE_UNSUPPORTED    Function not supported by the target
 *      - LE_FAULT          Function failed to get clock time
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_clkSync_GetTimeWithNetworkTimeProtocol
(
    const char* serverStrPtr,       ///< [IN]  Time server
    bool getOnly,                   ///< [IN]  Get the time acquired without updating system clock
    le_clkSync_ClockTime_t* timePtr ///< [OUT] Time structure
);

#endif
