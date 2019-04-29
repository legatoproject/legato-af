/**
 * @file rand.c
 *
 * This Random Number API is a wrapper around a cryptographic pseudo-random number generator (CPRNG)
 * that is properly seeded with entropy.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"

#include "fa/rand.h"
#include "rand.h"

//--------------------------------------------------------------------------------------------------
/**
 * Initializes the Random Number API service.
 */
//--------------------------------------------------------------------------------------------------
void rand_Init
(
    void
)
{
    fa_rand_Init();
}

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
    uint32_t min,   ///< [IN] Minimum value in range (inclusive).
    uint32_t max    ///< [IN] Maximum value in range (inclusive).
)
{
    le_result_t result;

    LE_ASSERT(max > min);

    // Determine range of numbers to reject.
    uint32_t interval = max - min + 1;

    uint64_t numPossibleVals = (uint64_t) UINT32_MAX + 1;
    uint64_t rejectThreshold = numPossibleVals - (numPossibleVals % interval);

    // Get random number.
    uint32_t randNum;

    for (;;)
    {
        result = fa_rand_Read(&randNum, sizeof(randNum));
        LE_FATAL_IF(result != LE_OK, "Could not read random numbers (%s).", LE_RESULT_TXT(result));

        // Check if this number is valid.  Reject numbers that are about greater than or equal to
        // our threshold to avoid bias.
        if (randNum < rejectThreshold)
        {
            // The number is valid.
            break;
        }
    }

    return (randNum % interval) + min;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get a buffer of random numbers.
 */
//--------------------------------------------------------------------------------------------------
void le_rand_GetBuffer
(
    uint8_t *bufPtr,    ///< [OUT] Buffer to store the random numbers in.
    size_t   bufSize    ///< [IN]  Number of random numbers to get.
)
{
    le_result_t result;

    LE_ASSERT(bufPtr != NULL);

    result = fa_rand_Read(bufPtr, bufSize);
    LE_FATAL_IF(result != LE_OK, "Could not read random numbers (%s).", LE_RESULT_TXT(result));
}
