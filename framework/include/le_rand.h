/**
 * @page c_rand Random Number API
 *
 * @subpage le_rand.h "API Reference"
 *
 * <HR>
 *
 * This Random Number API is a wrapper around a cryptographic pseudo-random number generator (CPRNG)
 * that is properly seeded with entropy.
 *
 * The random numbers returned by this API may be used for cryptographic purposes such as encryption
 * keys, initialization vectors, etc.
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc.
 */

//--------------------------------------------------------------------------------------------------
/** @file le_rand.h
 *
 * Legato @ref c_rand include file.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_RAND_INCLUDE_GUARD
#define LEGATO_RAND_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Get a random number within the specified range, min to max inclusive.
 *
 * @warning The max value must be greater than the min value, if not this function will log the
 *          error and kill the calling process.
 *
 * @return  The random number.
 */
//--------------------------------------------------------------------------------------------------
uint32_t le_rand_GetNumBetween
(
    uint32_t min,               ///< [IN] Minimum value in range (inclusive).
    uint32_t max                ///< [IN] Maximum value in range (inclusive).
);


//--------------------------------------------------------------------------------------------------
/**
 * Get a buffer of random numbers.
 */
//--------------------------------------------------------------------------------------------------
void le_rand_GetBuffer
(
    uint8_t* bufPtr,            ///< [OUT] Buffer to store the random numbers in.
    size_t bufSize              ///< [IN] Number of random numbers to get.
);


#endif // LEGATO_RAND_INCLUDE_GUARD
