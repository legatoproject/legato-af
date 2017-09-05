
//--------------------------------------------------------------------------------------------------
/** @file pathIter.h
 *
 * Legato Path module's inter-module include file.
 *
 * This file exposes interfaces that are for use by other modules inside the framework
 * implementation, but must not be used outside of the framework implementation.
 *
 * Copyright (C) Sierra Wireless Inc.
 * license.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_SRC_PATH_ITER_INCLUDE_GUARD
#define LEGATO_SRC_PATH_ITER_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Initialize the path subsystem's internal memory pools.  This function is ment to be called
 * from Legato's internal init.
 */
//--------------------------------------------------------------------------------------------------
void pathIter_Init
(
    void
);


#endif  // LEGATO_SRC_PATH_ITER_INCLUDE_GUARD
