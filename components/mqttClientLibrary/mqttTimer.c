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
 * Get remaining time of a timer.
 *
 * @return number of milliseconds before expiry.
 */
//--------------------------------------------------------------------------------------------------
int TimerLeftMS
(
    Timer* timer                /// [IN] Timer structure
)
{
    struct timeval now, res;
    gettimeofday(&now, NULL);
    timersub(&timer->end_time, &now, &res);

    return (res.tv_sec < 0) ? 0 : res.tv_sec * 1000 + res.tv_usec / 1000;
}
