/**
 * @file rand.c
 *
 * This Random Number API is a wrapper around a cryptographic pseudo-random number generator (CPRNG)
 * that is properly seeded with entropy.
 *
 * @note If build-time glibc is >= 2.25, this module tries to use getrandom, which takes care of the
 *       checks regarding the configuration and initialization. However, we check for the presence
 *       of the symbol at runtime if case we are running on an older version of glibc.
 *
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "fa/rand.h"
#include "fileDescriptor.h"

#if ((__GLIBC__ == 2) && (__GLIBC_MINOR__ >= 25)) || (__GLIBC__ > 2)

#define USE_SYS_RANDOM

#include <dlfcn.h>
#include <gnu/lib-names.h>
#include <sys/random.h>

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
 * File descriptor for /dev/urandom, if applicable.
 */
//--------------------------------------------------------------------------------------------------
static int RandFd = -1;


//--------------------------------------------------------------------------------------------------
/**
 * Read data from /dev/urandom.
 *
 * @return Number of bytes read, or -1 on error.
 */
//--------------------------------------------------------------------------------------------------
static ssize_t ReadDev
(
    void    *bufPtr,    ///< [OUT] Buffer to fill.
    size_t   count,     ///< [IN]  Number of bytes to read.
    int      flags      ///< [IN]  Flags (unused).
)
{
    LE_UNUSED(flags);
    return read(RandFd, bufPtr, count);
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the reading of data from /dev/urandom.
 */
//--------------------------------------------------------------------------------------------------
static void InitDev
(
    void
)
{
    LE_FATAL_IF(RandFd != -1, "/dev/urandom already initialized");

    // Open /dev/urandom for reading.
    do
    {
        RandFd = open("/dev/urandom", O_RDONLY);
    }
    while ((RandFd == -1) && (errno == EINTR));

    LE_FATAL_IF(RandFd == -1, "Failed to open /dev/urandom (error %d)", errno);
    GetRandom = &ReadDev;
}

//--------------------------------------------------------------------------------------------------
/**
 * Initializes the Random Number API service.
 *
 * If built with glibc >= 2.25, this function will look for the getrandom
 * symbol in glibc dynamically.
 */
//--------------------------------------------------------------------------------------------------
void fa_rand_Init
(
    void
)
{
#if defined(USE_SYS_RANDOM)
    void *handle = dlopen(LIBC_SO, RTLD_LAZY);
    if (handle)
    {
        GetRandom = (GetRandom_t)dlsym(handle, "getrandom");
        dlclose(handle);
    }

    LE_DEBUG("getrandom function: %p", GetRandom);
#endif

    // Versions of glibc before 2.25 do not have support for the getrandom() functions in which case
    // we need to read directly from /dev/urandom.
    if (GetRandom == NULL)
    {
        InitDev();
    }
}

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
    void    *bufferPtr, ///< [OUT] Buffer to write the random data to.
    size_t   count      ///< [IN]  Number of bytes of random data requested.
)
{
    size_t   readCount = 0;
    ssize_t  c;
    uint8_t *bufPtr = bufferPtr;

    // Get random numbers.
    while (readCount < count)
    {
        do
        {
            c = GetRandom(bufPtr + readCount, count - readCount, 0);
        }
        while ((c == -1) && (errno == EINTR));

        if (c == -1)
        {
            if ( (errno == ENOSYS) && (GetRandom != &ReadDev) )
            {
                // Fallback to /dev/urandom
                InitDev();
                continue;
            }

            LE_CRIT("Could not read random numbers (error %d)", errno);
            return LE_IO_ERROR;
        }
        readCount += c;
    }

    return LE_OK;
}
