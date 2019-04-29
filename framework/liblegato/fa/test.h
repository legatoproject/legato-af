/**
 * @file test.h
 *
 * Test interface that must be implemented by a Legato framework adaptor.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
#ifndef FA_TEST_H_INCLUDE_GUARD
#define FA_TEST_H_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Performs test system initialization.
 */
//--------------------------------------------------------------------------------------------------
void fa_test_Init
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Start a unitary test suite.
 */
//--------------------------------------------------------------------------------------------------
void fa_test_Start
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Exit a unitary test suite.  This terminates the process.
 */
//--------------------------------------------------------------------------------------------------
void fa_test_Exit
(
    int failedTests ///< Number of failed test cases.
) __attribute__((noreturn));

#endif /* end FA_TEST_H_INCLUDE_GUARD */
