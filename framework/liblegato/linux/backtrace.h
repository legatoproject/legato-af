//--------------------------------------------------------------------------------------------------
/** @file backtrace.h
 *
 * This module provides an internal function for printing a backtrace that is usable within or
 * outside of a signal handler.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */
#ifndef LEGATO_FA_BACKTRACE_INCLUDE_GUARD
#define LEGATO_FA_BACKTRACE_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 *  Dump call stack and register information to stderr in a signal-handler safe manner.
 */
//--------------------------------------------------------------------------------------------------
void backtrace_DumpContextStack
(
    const void  *infoPtr,   ///< [IN]       Context pointer.
    int          skip,      ///< [IN]       Number of initial stack frames to skip, if applicable.
    char        *buf,       ///< [IN,OUT]   Scratch buffer to use for composing output.
    size_t       bufLen     ///< [IN]       Size of scratch buffer.
);

#endif  // LEGATO_FA_BACKTRACE_INCLUDE_GUARD
