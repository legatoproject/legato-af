//--------------------------------------------------------------------------------------------------
/** @file rand.h
 *
 * Legato Random Number API service module's inter-module include file.
 *
 * This file exposes interfaces that are for use by other modules inside the framework
 * implementation, but must not be used outside of the framework implementation.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */
#ifndef RAND_INCLUDE_GUARD
#define RAND_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Initializes the Random Number API service.
 *
 * @note
 *      On failure, the process exits.
 */
//--------------------------------------------------------------------------------------------------
void rand_Init
(
    void
);


#endif  // RAND_INCLUDE_GUARD
