/**
 * @file test.h
 *
 * This module contains initialization functions for the Legato unitary test system that should
 * be called by the build system on RTOS.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
#ifndef TEST_INCLUDE_GUARD
#define TEST_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Performs RTOS-specific system initialization.
 */
//--------------------------------------------------------------------------------------------------
void test_SystemInit
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Start a unitary test suite
 */
//--------------------------------------------------------------------------------------------------
void fa_test_Start
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Exit a unitary test suite
 */
//--------------------------------------------------------------------------------------------------
void fa_test_Exit
(
    int failedTests
) __attribute__((noreturn));

#endif /* end TEST_INCLUDE_GUARD */
