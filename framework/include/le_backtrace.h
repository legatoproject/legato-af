/**
 *  @page c_backtrace Call Stack Backtrace Functionality
 *
 *  Provides a function to dump the current call stack.
 *
 *  <HR>
 *
 *  Copyright (C) Sierra Wireless Inc.
 */

/** @file le_backtrace.h
 *
 *  Legato @ref c_backtrace include file.
 *
 *  Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_BACKTRACE_INCLUDE_GUARD
#define LEGATO_BACKTRACE_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 *  Dump the current call stack to stderr.
 *
 *  @param  msg String to print as the title of the backtrace.
 */
//--------------------------------------------------------------------------------------------------
#if LE_CONFIG_ENABLE_BACKTRACE
#   define LE_BACKTRACE(msg)    _le_backtrace(msg)
void _le_backtrace(const char *msg);
#else
#   define LE_BACKTRACE(msg)    ((void) (msg))
#endif

#endif // LEGATO_BACKTRACE_INCLUDE_GUARD
