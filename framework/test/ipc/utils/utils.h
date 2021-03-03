/*
 * Helper functions.
 *
 * Trivial functions are performed inline (e.g. add one, not).  These helpers are used for
 * other operations (incrementing enums, rot13, etc.)
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef UTILS_UTILS_H
#define UTILS_UTILS_H 1

/**
 * Get the next ipcTest_SmallEnum value.
 */
ipcTest_SmallEnum_t util_IncSmallEnum
(
    ipcTest_SmallEnum_t val
);

/**
 * Get the next ipcTest_LargeEnum value
 */
ipcTest_LargeEnum_t util_IncLargeEnum
(
    ipcTest_LargeEnum_t val
);

/**
 * Get the next le_result_t value
 */
le_result_t util_IncResult
(
    le_result_t val
);

/**
 * Negate the input
 */
le_onoff_t util_NotOnOff
(
    le_onoff_t val
);

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
);

/**
 * Rotate letters in a string by 13 places.
 */
void util_ROT13String
(
    const char *in,
    char *out,
    size_t outSz
);

#endif
