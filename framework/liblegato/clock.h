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
   IsWakeup: False - Use non-waking clock. Otherwise not.
 *
 * @return
 *      Relative time in seconds/microseconds
 *
 */
//--------------------------------------------------------------------------------------------------
le_clk_Time_t clk_GetRelativeTime
(
    bool IsWakeup   ///< Use a waking clock.
);

#endif /* LEGATO_SRC_CLOCK_H_INCLUDE_GUARD */
