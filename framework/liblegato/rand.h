/**
 * @file rand.h
 *
 * Random number generator interface.
 *
 * This Random Number API is a wrapper around a cryptographic pseudo-random number generator (CPRNG)
 * that is properly seeded with entropy.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef RAND_INCLUDE_GUARD
#define RAND_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Initializes the Random Number API service.
 */
//--------------------------------------------------------------------------------------------------
void rand_Init
(
    void
);

#endif /* end RAND_INCLUDE_GUARD */
