//--------------------------------------------------------------------------------------------------
/**
 * Header file for statically allocated thread tests.  These functions are called by the
 * main module (main.c).
 *
 * Copyright (C) Sierra Wireless Inc.
 **/
//--------------------------------------------------------------------------------------------------

#ifndef LE_STATIC_THREAD_TEST_H_INCLUSION_GUARD
#define LE_STATIC_THREAD_TEST_H_INCLUSION_GUARD

// -------------------------------------------------------------------------------------------------
/**
 * Starts the test.
 */
// -------------------------------------------------------------------------------------------------
void static_Start
(
    void
);

// -------------------------------------------------------------------------------------------------
/**
 * Checks the completion status of the test.
 */
// -------------------------------------------------------------------------------------------------
void static_CheckResults
(
    void
);

#endif // LE_STATIC_THREAD_TEST_H_INCLUSION_GUARD
