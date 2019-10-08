/**
 * @file clock.h
 *
 * Clock module's intra-framework header file.  This file exposes type definitions and function
 * interfaces to other modules inside the framework implementation.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_SRC_CLOCK_H_INCLUDE_GUARD
#define LEGATO_SRC_CLOCK_H_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Get relative time since some fixed but unspecified starting point. However, retrieve the time
 * based on the desired wakeup behaviour.
 *
 * @return
 *      Relative time in seconds/microseconds
 *
 */
//--------------------------------------------------------------------------------------------------
le_clk_Time_t clk_GetRelativeTime
(
    bool isWakeup   ///< true if a waking clock be used, false otherwise
);

#endif /* LEGATO_SRC_CLOCK_H_INCLUDE_GUARD */
