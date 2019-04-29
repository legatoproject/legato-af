//--------------------------------------------------------------------------------------------------
/** @file test.c
 *
 * Implements the Legato Test Framework.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "fa/test.h"


//--------------------------------------------------------------------------------------------------
/**
 * Should the plan be printed out on exit?
 */
//--------------------------------------------------------------------------------------------------
static bool NeedsPlan = false;

//--------------------------------------------------------------------------------------------------
/**
 * The number of tests.
 */
//--------------------------------------------------------------------------------------------------
static int NumTests = 0;

//--------------------------------------------------------------------------------------------------
/**
 * The number of test failures.
 */
//--------------------------------------------------------------------------------------------------
static int NumFailures = 0;

//--------------------------------------------------------------------------------------------------
/**
 * Are tests being marked as TODO?
 */
//--------------------------------------------------------------------------------------------------
static bool IsTodo = false;

//--------------------------------------------------------------------------------------------------
/**
 * Is test execution being skipped.
 */
//--------------------------------------------------------------------------------------------------
static bool IsSkipped = false;

//--------------------------------------------------------------------------------------------------
/**
 * Performs test system initialization.
 */
//--------------------------------------------------------------------------------------------------
void test_Init
(
    void
)
{
    fa_test_Init();
}

//--------------------------------------------------------------------------------------------------
/**
 * Initializes the Legato test framework.  This should be called once at the beginning of test
 * process.
 */
//--------------------------------------------------------------------------------------------------
void _le_test_Init
(
    int maxTestCount    ///< [IN] Expected number of tests
)
{
    // Call this first, as it may block waiting for other test suites to finish.
    fa_test_Start();

    NumTests = 0;
    NumFailures = 0;
    IsSkipped = false;
    IsTodo = false;

    NeedsPlan = (maxTestCount < 0);
}

//--------------------------------------------------------------------------------------------------
/**
 * Clean up the Legato test framework.  This should be called once at the end of test
 * process.
 */
//--------------------------------------------------------------------------------------------------
void _le_test_Exit
(
    void
)
{
    fa_test_Exit(NumFailures);
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
    ++NumFailures;
}

//--------------------------------------------------------------------------------------------------
/**
 * Assign a number to a test, incrmenting the total number of tests.
 */
//--------------------------------------------------------------------------------------------------
int _le_test_NumberTest
(
    void
)
{
    int totalTests = ++NumTests;

    return totalTests;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the total number of tests seen so far.
 */
//--------------------------------------------------------------------------------------------------
int _le_test_GetNumTests
(
    void
)
{
    int totalTests = NumTests;

    return totalTests;
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
    int totalFailures = NumFailures;

    return totalFailures;
}

//--------------------------------------------------------------------------------------------------
/**
 * Mark next series of tests as todo
 */
//--------------------------------------------------------------------------------------------------
bool _le_test_SetTodo
(
    bool todo                   ///< [IN] Should next tests be marked as "todo"
)
{
    bool oldTodo = IsTodo;
    IsTodo = todo;

    return oldTodo;
}

//--------------------------------------------------------------------------------------------------
/**
 * Mark next series of tests as skipped.
 */
//--------------------------------------------------------------------------------------------------
void _le_test_Skip
(
    int count                  ///< [IN] Number of tests expected to skip
)
{
    IsSkipped = true;

    while (count--)
    {
        LE_TEST_OK(true, "");
    }

    IsSkipped = false;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if the tests are being skipped.
 */
//--------------------------------------------------------------------------------------------------
bool _le_test_IsSkipping
(
    void
)
{
    return IsSkipped;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get tag for skipped or TODO test.
 *
 * Skipping a test takes priority over marking it as todo.
 */
//--------------------------------------------------------------------------------------------------
const char*_le_test_GetTag
(
    void
)
{
    if (IsSkipped)
    {
        return " # skip";
    }
    else if (IsTodo)
    {
        return " # TODO";
    }
    else
    {
        return "";
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if a plan needs to be printed.
 */
//--------------------------------------------------------------------------------------------------
bool _le_test_CheckNeedsPlan
(
    void
)
{
    return NeedsPlan;
}
