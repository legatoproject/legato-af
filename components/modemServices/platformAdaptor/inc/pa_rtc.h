/**
 * @page c_pa_rtc Read and Write time to the Real Time Clock
 *
 * @ref pa_rtc.h "API Reference"
 *
 * <HR>
 *
 * For modules that support real time clock (battery backed up VCOIN clock).
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless, Inc. 2015.
 */


/** @file pa_rtc.h
 *
 * Legato @ref c_pa_rtc include file.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_PARTC_INCLUDE_GUARD
#define LEGATO_PARTC_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Reads the time from the RTC.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_UNAVAILABLE   QMI succeeded but the user time was not retrieved.
 *      - LE_FAULT         The function failed.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_rtc_GetUserTime
(
    uint64_t*  millisecondsPastGpsEpochPtr ///< [OUT]
);

//--------------------------------------------------------------------------------------------------
/**
 * Writes the time to the RTC.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_rtc_SetUserTime
(
    uint64_t  millisecondsPastGpsEpoch ///< [IN]
);

#endif // LEGATO_PARTC_INCLUDE_GUARD
