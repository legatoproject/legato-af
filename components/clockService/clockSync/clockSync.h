//--------------------------------------------------------------------------------------------------
/**
 *  Clock Service's header file for the C code implementation of its le_clkSync APIs
 *
 *  Copyright (C) Sierra Wireless Inc.
 *
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_CLOCKSYNC_H_INCLUDE_GUARD
#define LEGATO_CLOCKSYNC_H_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * CLOCKSYNC_INTERNAL_CLIENT_SESSION_REF
 * Clock Sync Service's internal client session reference which is received in the le_clkSync code
 * when called from le_data or likewise.
 */
//--------------------------------------------------------------------------------------------------
#define CLOCKSYNC_INTERNAL_CLIENT_SESSION_REF 0

////////////////////////////////////////////////////////////////////////////////////////////////////
// Intra Clock & DCS services' interface
//
// This is the interface exported by le_clkSync to other DCS modules (e.g. dcsDaemon).
//

LE_SHARED le_result_t clkSync_GetCurrentTime
(
    le_msg_SessionRef_t sessionRef,     ///< [IN] messaging session making request
    le_clkSync_ClockTime_t* timePtr,    ///< [OUT] Clock time retrieved from configured source
    le_clkSync_ClockSource_t* sourcePtr ///< [OUT] Clock source from which current time is acquired
);

#endif
