//--------------------------------------------------------------------------------------------------
/** @file pa_start.c
 *
 * This file provides the default implementation for querying information needed by the Linux start
 * program.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------
#include "pa_start.h"

/// Ensure public function visibility.
#define PA_START_SHARED __attribute__((visibility("default")))

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the start platform adaptor.
 */
//--------------------------------------------------------------------------------------------------
PA_START_SHARED void pa_start_Init
(
    void
)
{
    // Do nothing.
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if system reboot is due to hardware reason (under voltage, over temperature).
 *
 * @return True if the last reset was as a result of a hardware fault, false if it was not, or the
 *         reason could not be determined.
 */
//--------------------------------------------------------------------------------------------------
PA_START_SHARED bool pa_start_IsHardwareFaultReset
(
    bool isRepeated     ///< Whether the reboot happened shortly after the previous reboot.
)
{
    return false;
}
