/**
 * @file rand.h
 *
 * Random number generator interface that must be implemented by a Legato framework adaptor.
 *
 * This Random Number API is a wrapper around a cryptographic pseudo-random number generator (CPRNG)
 * that is properly seeded with entropy.
 *
 * @warning The availability of entropy and seeding of the CPRNG is system dependent.  When porting
 *          this module care must be taken to ensure that the underlying CPRNG and entropy pools are
 *          configured properly.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef FA_RAND_H_INCLUDE_GUARD
#define FA_RAND_H_INCLUDE_GUARD

#include "legato.h"

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the platform-specific random number generator.
 *
 * This function must be called exactly once at process start-up before any other rand module
 * functions are called.
 */
//--------------------------------------------------------------------------------------------------
void fa_rand_Init
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Read a buffer of random data from the platform-specific random number generator.
 *
 * @return LE_OK on success, otherwise an appropriate error code if random data could not be
 *         provided.
 */
//--------------------------------------------------------------------------------------------------
le_result_t fa_rand_Read
(
    void    *bufferPtr, ///< [out] Buffer to write the random data to.
    size_t   count      ///< [in]  Number of bytes of random data requested.
);

#endif /* end FA_RAND_H_INCLUDE_GUARD */
