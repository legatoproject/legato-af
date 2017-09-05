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
 * The signal event initialization function.  This must be called before any other functions in this
 * module is called.
 */
//--------------------------------------------------------------------------------------------------
void sig_Init
(
    void
);


#endif  // LEGATO_SIG_INCLUDE_GUARD
