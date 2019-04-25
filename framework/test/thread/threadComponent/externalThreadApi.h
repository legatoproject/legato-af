//--------------------------------------------------------------------------------------------------
/**
 * Header file for external thread API compatibility test.
 *
 * Copyright (C) Sierra Wireless Inc.
 **/
//--------------------------------------------------------------------------------------------------

#ifndef EXTERNAL_THREAD_API_TEST_H_INCLUDE_GUARD
#define EXTERNAL_THREAD_API_TEST_H_INCLUDE_GUARD

// -------------------------------------------------------------------------------------------------
/**
 * Starts the test.
 *
 * Increments the reference count on a given memory pool object, then releases it when the test is
 * complete.
 */
// -------------------------------------------------------------------------------------------------
void eta_Start
(
    void
);

// -------------------------------------------------------------------------------------------------
/**
 * Checks the completion status of the test.
 */
// -------------------------------------------------------------------------------------------------
void eta_CheckResults
(
    void
);

#endif // EXTERNAL_THREAD_API_TEST_H_INCLUDE_GUARD
