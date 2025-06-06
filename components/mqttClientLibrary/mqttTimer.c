/*******************************************************************************
 * Copyright (c) 2014, 2015 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Allan Stockdill-Mander - initial API and implementation and/or initial documentation
 *    Ian Craggs - convert to FreeRTOS
 *******************************************************************************/

#include "mqttTimer.h"

//--------------------------------------------------------------------------------------------------
/**
 * Initialize a timer
 *
 * @return none.
 */
//--------------------------------------------------------------------------------------------------
void TimerInit
(
    Timer* timer                /// [OUT] Timer structure
)
{
    timer->end_time = (struct timeval){0, 0};
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if a timer is expired.
 *
 * @return true if expired, false otherwise.
 */
//--------------------------------------------------------------------------------------------------
char TimerIsExpired
(
    Timer* timer                /// [IN] Timer structure
)
{
    struct timeval now, res;
    gettimeofday(&now, NULL);
    timersub(&timer->end_time, &now, &res);

    return res.tv_sec < 0 || (res.tv_sec == 0 && res.tv_usec <= 0);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set count down timeout value in millisecond.
 *
 * @return none.
 */
//--------------------------------------------------------------------------------------------------
void TimerCountdownMS
(
    Timer* timer,               /// [OUT] Timer structure
    unsigned int timeout        /// [IN] Timeout value in millisecond
)
{
    struct timeval now;
    gettimeofday(&now, NULL);
    struct timeval interval = {timeout / 1000, (timeout % 1000) * 1000};
    timeradd(&now, &interval, &timer->end_time);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set countdown timeout value in second.
 *
 * @return none.
 */
//--------------------------------------------------------------------------------------------------
void TimerCountdown
(
    Timer* timer,               /// [OUT] Timer structure
    unsigned int timeout        /// [IN] Timeout value in second
)
{
    struct timeval now;
    gettimeofday(&now, NULL);
    struct timeval interval = {timeout, 0};
    timeradd(&now, &interval, &timer->end_time);
}

//--------------------------------------------------------------------------------------------------
/**
 * @brief Get the remaining milliseconds before timer expires.
 *
 * This function handles two potential overflow scenarios:
 * 1. INT64_MAX overflow: Theoretically possible if tv_sec approaches
 *    INT64_MAX, but practically impossible since tv_sec represents
 *    seconds since Unix epoch (1970). Even the Year 2038 problem
 *    occurs at tv_sec = 2,147,483,647, which is much smaller than
 *    INT64_MAX (9,223,372,036,854,775,807).
 * 2. INT_MAX overflow: More realistic concern when converting large
 *    time differences from seconds to milliseconds, handled by
 *    checking final result.
 *
 * @param timer [IN] The timer structure containing expiration time.
 * @return Milliseconds until expiry (0 if expired, INT_MAX if overflow)
 */
//--------------------------------------------------------------------------------------------------
int TimerLeftMS
(
    Timer* timer                /// [IN] Timer structure
)
{
    struct timeval now, remaining;
    gettimeofday(&now, NULL);
    timersub(&timer->end_time, &now, &remaining);

    // Return 0 if timer has expired
    if (remaining.tv_sec < 0)
    {
        return 0;
    }

    // Use 64-bit arithmetic to prevent INT64_MAX overflow during calculation
    // Since tv_sec values are realistic (Unix epoch-based), INT64_MAX overflow
    // is extremely unlikely, so no explicit pre-check is needed
    int64_t milliseconds = (int64_t)remaining.tv_sec * 1000 +
                          remaining.tv_usec / 1000;

    // Check for INT_MAX overflow when converting back to int
    // This is the realistic overflow scenario for large time differences
    if (milliseconds > INT_MAX) {
        LE_ERROR("Timer overflow: result exceeds INT_MAX");
        return INT_MAX;
    }

    return milliseconds > 0 ? (int)milliseconds : 0;
}
