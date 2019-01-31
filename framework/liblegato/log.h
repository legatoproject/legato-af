/** @file log.h
 *
 * Log module's intra-framework header file.  This file exposes type definitions and function
 * interfaces to other modules inside the framework implementation.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LOG_INCLUDE_GUARD
#define LOG_INCLUDE_GUARD

#if LE_CONFIG_LINUX
#  include "linux/linux_log.h"
#endif

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the logging system.  This must be called VERY early in the process initialization.
 * Anything that is logged prior to this call will be logged with the wrong component name.
 */
//--------------------------------------------------------------------------------------------------
void log_Init
(
    void
);

#endif // LOG_INCLUDE_GUARD
