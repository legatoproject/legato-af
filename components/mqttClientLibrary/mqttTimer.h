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
 *******************************************************************************/

#ifndef __MQTT_FREERTOS_TIMER_HEADER_GUARD__
#define __MQTT_FREERTOS_TIMER_HEADER_GUARD__

#include "legato.h"

//--------------------------------------------------------------------------------------------------
/**
 * Timer structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct Timer
{
    struct timeval end_time;
} Timer;


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
);

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
);

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
    unsigned int timeout_ms     /// [IN] Timeout value in millisecond
);

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
);

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
);


#endif // __MQTT_FREERTOS_TIMER_HEADER_GUARD__
