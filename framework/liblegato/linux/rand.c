/**
 * @file rand.c
 *
 * This Random Number API is a wrapper around a cryptographic pseudo-random number generator (CPRNG)
 * that is properly seeded with entropy.
 *
 * @warning
 *          The availability of entropy and seeding of the CPRNG is system dependent.  When porting
 *          this module care must be taken to ensure that the underlying CPRNG and entropy pools are
 *          configured properly.
 *
 * @note If build-time glibc is >= 2.25, this module tries to use getrandom, which takes care of the checks
 *       regarding the configuration and initialization.
 *       However, we check for the presence of the symbol at runtime if case we are running on an older
 *       version of glibc.
 *
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "fileDescriptor.h"
#include "rand.h"

#if ( (__GLIBC__ == 2 ) && (__GLIBC_MINOR__ >= 25) ) \
    || (__GLIBC__ > 2 )

#define USE_SYS_RANDOM

#include <sys/random.h>
#include <dlfcn.h>
#include <gnu/lib-names.h>

#endif

//--------------------------------------------------------------------------------------------------
/**
 * Prototype for the getrandom() function, only available on glibc >= 2.25.
 */
//--------------------------------------------------------------------------------------------------
typedef ssize_t (*GetRandom_t)(void *buffer, size_t length, int flags);

//--------------------------------------------------------------------------------------------------
/**
 * Pointer to getrandom function, if available.
 */
//--------------------------------------------------------------------------------------------------
static GetRandom_t GetRandom = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Initializes the Random Number API service.
 *
 * If built with glibc >= 2.25, this function will look for the getrandom
 * symbol in glibc dynamically.
 */
//--------------------------------------------------------------------------------------------------
void rand_Init
(
    void
)
{
#if defined(USE_SYS_RANDOM)
    void* handle = dlopen(LIBC_SO, RTLD_LAZY);
    if (handle)
    {
        GetRandom = (GetRandom_t)dlsym(handle, "getrandom");
        dlclose(handle);
    }

    LE_DEBUG("getrandom function: %p", GetRandom);
#endif
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
    uint32_t min,               ///< [IN] Minimum value in range (inclusive).
    uint32_t max                ///< [IN] Maximum value in range (inclusive).
)
{
    LE_ASSERT(max > min);

    // Versions of glibc before 2.25 do not have support for the getrandom() functions in which case
    // we need to read directly from /dev/urandom.

    int fd = -1;

    if (!GetRandom)
    {
        // Open /dev/urandom for reading.
        do
        {
            fd = open("/dev/urandom", O_RDONLY);
        }
        while ((fd == -1) && (errno == EINTR));

        LE_FATAL_IF(fd == -1, "Failed to open /dev/urandom. %m.");
    }

    // Determine range of numbers to reject.
    uint32_t interval = max - min + 1;

    uint64_t numPossibleVals = (uint64_t)UINT32_MAX + 1;
    uint64_t rejectThreshold = numPossibleVals - (numPossibleVals % interval);

    // Get random number.
    uint32_t randNum;

    while (1)
    {
        ssize_t c;

        do
        {
            if (!GetRandom)
            {
                c = read(fd, &randNum, sizeof(randNum));
            }
            else
            {
                c = GetRandom(&randNum, sizeof(randNum), 0);
            }
        }
        while ( ((c == -1) && (errno == EINTR)) || (c < sizeof(randNum)) );

        LE_FATAL_IF(c == -1, "Could not read random numbers. %m.");

        // Check if this number is valid.  Reject numbers that are about greater than or equal to
        // our threshold to avoid bias.
        if (randNum < rejectThreshold)
        {
            // The number is valid.
            break;
        }
    }

    if (!GetRandom)
    {
        fd_Close(fd);
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
    uint8_t* bufPtr,            ///< [OUT] Buffer to store the random numbers in.
    size_t bufSize              ///< [IN] Number of random numbers to get.
)
{
    LE_ASSERT(bufPtr != NULL);

    // Versions of glibc before 2.25 do not have support for the getrandom() functions in which case
    // we need to read directly from /dev/urandom.

    int fd = -1;

    if (!GetRandom)
    {
        // Open /dev/urandom for reading.
        do
        {
            fd = open("/dev/urandom", O_RDONLY);
        }
        while ((fd == -1) && (errno == EINTR));

        LE_FATAL_IF(fd == -1, "Failed to open /dev/urandom. %m.");
    }

    // Get random numbers.
    size_t readCount = 0;

    while (readCount < bufSize)
    {
        ssize_t c;

        do
        {
            if (!GetRandom)
            {
                c = read(fd, &(bufPtr[readCount]), (bufSize-readCount));
            }
            else
            {
                c = GetRandom(&(bufPtr[readCount]), (bufSize-readCount), 0);
            }
        }
        while ( (c == -1) && (errno == EINTR) );

        LE_FATAL_IF(c == -1, "Could not read random numbers. %m.");

        readCount += c;
    }

    if (!GetRandom)
    {
        fd_Close(fd);
    }
}
