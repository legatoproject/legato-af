//--------------------------------------------------------------------------------------------------
/**
 * @file system.h
 *
 * Small set of functions for checking if a system is good.
 *
 * Copyright (C) Sierra Wireless Inc.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_SYSTEM_H_INCLUDE_GUARD
#define LEGATO_SYSTEM_H_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Get the status of the current system.
 *
 * @return true if the system is marked "good", false otherwise (e.g., if "tried 2").
 */
//--------------------------------------------------------------------------------------------------
bool system_IsGood
(
    void
);


#endif // LEGATO_SYSTEM_H_INCLUDE_GUARD
