//--------------------------------------------------------------------------------------------------
/** @file pa_start.h
 *
 * This file provides an interface for querying information needed by the Linux start program.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------
#ifndef LEGATO_SRC_PA_START_INCLUDE_GUARD
#define LEGATO_SRC_PA_START_INCLUDE_GUARD

#include <stdbool.h>

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the start platform adaptor.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*pa_start_Init_t)
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Check if system reboot is due to hardware reason (under voltage, over temperature).
 *
 * @return True if the last reset was as a result of a hardware fault, false if it was not, or the
 *         reason could not be determined.
 */
//--------------------------------------------------------------------------------------------------
typedef bool (*pa_start_IsHardwareFaultReset_t)
(
    bool isRepeated     ///< Whether the reboot happened shortly after the previous reboot.
);

#endif /* end LEGATO_SRC_PA_START_INCLUDE_GUARD */
