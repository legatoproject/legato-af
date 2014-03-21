//--------------------------------------------------------------------------------------------------
/** @file test.c
 *
 * Implements the Legato Test Framework.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#include "legato.h"


//--------------------------------------------------------------------------------------------------
/**
 * The number of test failures.  This may be accessed by multiple threads but since an int is atomic
 * we do not need to protect it with a mutex.
 */
//--------------------------------------------------------------------------------------------------
static int NumFailures = 0;


//--------------------------------------------------------------------------------------------------
/**
 * Controls whether the process exits when there is a failure or if the number of failures is
 * incremented and the testing continues (pass through).
 */
//--------------------------------------------------------------------------------------------------
static bool PassThrough = false;


//--------------------------------------------------------------------------------------------------
/**
 * The pass through argument strings.
 */
//--------------------------------------------------------------------------------------------------
static const char PassThroughArg[] = "-p";
static const char PassThroughArgLongForm[] = "--pass-through";


//--------------------------------------------------------------------------------------------------
/**
 * Mutex for protect multithreaded access to our data.
 */
//--------------------------------------------------------------------------------------------------
static le_mutex_Ref_t Mutex;


//--------------------------------------------------------------------------------------------------
/**
 * Initializes the Legato test framework.  This should be called once at the beginning of test
 * process.
 */
//--------------------------------------------------------------------------------------------------
void _le_test_Init
(
    void
)
{
    Mutex = le_mutex_CreateNonRecursive("UnitTestMutex");

    NumFailures = 0;
    PassThrough = false;

    int i = le_arg_NumArgs() - 1;

    for (; i >= 0; i--)
    {
        char buf[sizeof(PassThroughArgLongForm)];

        // Check the command line arguments for the PassThroughArgs strings.
        if ( (le_arg_GetArg(i, buf, sizeof(PassThroughArgLongForm)) == LE_OK) &&
             ((strcmp(buf, PassThroughArg) == 0) || (strcmp(buf, PassThroughArgLongForm) == 0)) )
        {
            PassThrough = true;
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Called when a test fails.  Logs an error message and either exits right away or increments the
 * number of failures.
 */
//--------------------------------------------------------------------------------------------------
void _le_test_Fail
(
    void
)
{
    if (PassThrough)
    {
        le_mutex_Lock(Mutex);

        NumFailures++;

        le_mutex_Unlock(Mutex);
    }
    else
    {
        exit(EXIT_FAILURE);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Returns the number of test failures.
 */
//--------------------------------------------------------------------------------------------------
int _le_test_GetNumFailures
(
    void
)
{
    return NumFailures;
}
