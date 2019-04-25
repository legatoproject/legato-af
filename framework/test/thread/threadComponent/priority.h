//--------------------------------------------------------------------------------------------------
/**
 * Header file for thread priority tests.  These functions are called by the
 * main module (main.c).
 *
 * Copyright (C) Sierra Wireless Inc.
 **/
//--------------------------------------------------------------------------------------------------

#ifndef LE_PRIORITY_TEST_H_INCLUSION_GUARD
#define LE_PRIORITY_TEST_H_INCLUSION_GUARD

// -------------------------------------------------------------------------------------------------
/**
 * Starts the test.
 *
 * Increments the reference count on a given memory pool object, then releases it when the test is
 * complete.
 */
// -------------------------------------------------------------------------------------------------
void prio_Start
(
    void
);

// -------------------------------------------------------------------------------------------------
/**
 * Checks the completion status of the test.
 */
// -------------------------------------------------------------------------------------------------
void prio_CheckResults
(
    void
);

#endif // LE_PRIORITY_TEST_H_INCLUSION_GUARD
