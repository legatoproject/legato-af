//--------------------------------------------------------------------------------------------------
/** @file signals.h
 *
 * This module contains initialization functions for the Legato signal events system that should
 * be called by the build system.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */
#ifndef LEGATO_SIG_INCLUDE_GUARD
#define LEGATO_SIG_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * WRITE macro to discard the return code inside the ShowStackSignalHandler
 */
//--------------------------------------------------------------------------------------------------
#define SIG_WRITE(buffer, sz)                                       \
    do {                                                            \
        int _rc;                                                    \
        _rc = write(STDERR_FILENO, (buffer), (sz));                 \
        if ((_rc >= 0) && (_rc < (sz)))                             \
        {                                                           \
            _rc = write(STDERR_FILENO, (buffer) + _rc, (sz) - _rc); \
        }                                                           \
    } while(0)


//--------------------------------------------------------------------------------------------------
/**
 * The signal event initialization function.  This must be called before any other functions in this
 * module is called.
 */
//--------------------------------------------------------------------------------------------------
void sig_Init
(
    void
);


#endif  // LEGATO_SIG_INCLUDE_GUARD
