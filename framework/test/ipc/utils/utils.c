/*
 * Helper functions.
 *
 * Trivial functions are performed inline (e.g. add one, not).  These helpers are used for
 * other operations (incrementing enums, rot13, etc.)
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include <legato.h>
#include <interfaces.h>

#include "utils.h"

/**
 * Get the next ipcTest_SmallEnum value.
 */
ipcTest_SmallEnum_t util_IncSmallEnum
(
    ipcTest_SmallEnum_t val
)
{
    if (val == IPCTEST_SE_VALUE5)
    {
        return IPCTEST_SE_VALUE1;
    }
    else
    {
        return (ipcTest_SmallEnum_t)((int)val + 1);
    }
}

/**
 * Get the next ipcTest_LargeEnum value
 */
ipcTest_LargeEnum_t util_IncLargeEnum
(
    ipcTest_LargeEnum_t val
)
{
    /*
     * Large enums are not dense, so use a switch to return the next value as
     * you can't simply add one and a table would be too large.
     */
    switch (val)
    {
        case IPCTEST_LE_VALUE1:
            return IPCTEST_LE_VALUE2;
        case IPCTEST_LE_VALUE2:
            return IPCTEST_LE_LARGE_VALUE1;
        case IPCTEST_LE_LARGE_VALUE1:
            return IPCTEST_LE_LARGE_VALUE2;
        case IPCTEST_LE_LARGE_VALUE2:
            return IPCTEST_LE_VALUE1;
        default:
            LE_TEST_FATAL("Invalid large enum %"PRIx64, (uint64_t)val);
    }
}

/**
 * Get the next le_result_t value
 */
le_result_t util_IncResult
(
    le_result_t val
)
{
    /*
     * The "next" result has a value one less than the previous one.
     */
    if (val > LE_SUSPENDED)
    {
        return (le_result_t)((int)val - 1);
    }
    else if (val == LE_SUSPENDED)
    {
        return LE_OK;
    }
    else
    {
        LE_TEST_FATAL("Invalid result %d", (int) val);
    }
}

/**
 * Negate the input
 */
le_onoff_t util_NotOnOff
(
    le_onoff_t val
)
{
    if (val == LE_OFF)
    {
        return LE_ON;
    }
    else
    {
        return LE_OFF;
    }
}

/**
 * Test two floating point numbers are equal within precision.
 *
 * This is mainly useful for testing over RPC if one side does not fully support IEEE floating
 * points.  Otherwise the simple operations performed in these tests should always yield
 * exactly identical values.
 */
bool util_IsDoubleEqual
(
    double a,
    double b
)
{
    // How close two numbers have to be to be considered "equal".
    static const double eps = 1e-6;

    // Test passes if a & b are both NaN.  Conversely in C NaN != NaN, so we can't test this
    // via direct comparison
    if (isnan(a) || isnan(b))
    {
        return isnan(a) && isnan(b);
    }

    // Add/subtract epsilon instead of comparing the difference against epsilon to handle large
    // numbers (including infinity) correctly.
    return (a*(1 - eps) <= b) && (b <= a*(1 + eps));
}

/**
 * Rotate letters in a string by 13 places.
 */
void util_ROT13String
(
    const char *in,
    char *out,
    size_t outSz
)
{
    size_t i;

    for (i = 0; i < outSz; ++i)
    {
        if (('a' <= in[i]) && (in[i] <= 'z'))
        {
            out[i] = ((in[i] - 'a' + 13) % 26) + 'a';
        }
        else if (('A' <= in[i]) && (in[i] <= 'Z'))
        {
            out[i] = ((in[i] - 'A' + 13) % 26) + 'A';
        }
        else
        {
            out[i] = in[i];
        }

        // Found terminating NULL? exit
        if (!out[i])
        {
            return;
        }
    }
}
