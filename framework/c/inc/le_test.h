/**
 * @page c_test Unit Testing API
 *
 * @ref le_test.h "Click here for the API reference documentation."
 *
 * <HR>
 *
 * @ref c_test_modes <br>
 * @ref c_test_setup <br>
 * @ref c_test_testing <br>
 * @ref c_test_result <br>
 * @ref c_test_multiProcess <br>
 *
 * Unit testing is an important aspect of a quantifiable quality assurance methodology.  Although
 * unit testing requires some extra overhead (the writing of the unit tests) during the development
 * process it can provide enormous benefits during the project life cycle.
 *
 * One benefit of writing unit tests is that it gets the developer using the interface to the
 * unit they designed. This forces the developer to think about, and hopefully design for,
 * usability of the interface early in the development cycle.
 *
 * Another major benefit to unit testing is that it provides a documented and verifiable level of
 * correctness for the designed unit. This allows the developer to refactor the code more
 * aggressively and to quickly verify its correctness.  Unit tests can also be used to perform
 * regression testing when adding new features.
 *
 * Despite the benefits of unit testing, unit tests are often omitted because of the initial
 * overhead of writing the tests and the complexity of testing frameworks.  Legato's Unit Test
 * Framework is simple to use and very flexible and lightweight consisting of some
 * handy macros.
 *
 * @section c_test_modes Modes of Operation
 *
 * The Legato Test Framework can run in one of two modes: <b>pass through</b> or <b>exit on
 * failure</b>.  In <b>pass through</b> mode all tests are run even if some of the tests fail.
 * Failed tests are counted and reported on completion.  <b>Exit on failure</b> mode also runs all
 * tests but exits right away if any tests fail.
 *
 * Mode selection is done through a command line argument.  If the test program is run with the
 * command line argument '-p' or '--pass-through' then <b>pass through</b> mode is selected.  If the
 * neither the '-p' or '--pass-through' arguments are present then <b>exit on failure</b> mode is
 * selected.
 *
 * @section c_test_setup Setting Up the Test Framework
 *
 * To setup the Legato Test Framework, call the @c LE_TEST_INIT macro, 
 * once before any tests are started.
 *
 * @section c_test_testing Performing Tests
 *
 * To perform tests, call the @c LE_TEST macro and pass in your test function to LE_TEST as a parameter.
 * The test function must have a bool return type which indicates that the test passed (true) or
 * failed (false).
 *
 * For example:
 *
 * @code
 *
 * // Returns true if the test passes, otherwise returns false.
 * bool Test1(void)
 * {
 *     int expectedValue;
 *
 *     // Do some initializations and/or calculations.
 *     ...
 *
 *     // Call one of the unit-under-test's interface function and check it's return value against
 *     // an expected value that was calculated earlier.
 *     return (unitUnderTest_foo() == expectedValue);
 * }
 *
 *
 * // Returns true if the test passes, otherwise returns false.
 * bool Test2(void)
 * {
 *     int expectedValue;
 *
 *     // Do some initializations and/or calculations.
 *     ...
 *
 *     // Call one of the unit-under-test's interface function and check it's return value against
 *     // an expected value that was calculated earlier.
 *     return (unitUnderTest_foo2() == expectedValue);
 * }
 *
 *
 * int main (void)
 * {
 *     // Setup the Legato Test Framework.
 *     LE_TEST_INIT;
 *
 *     // Run the tests.
 *     LE_TEST(Test1());
 *     LE_TEST(Test2());
 *
 *     // Exit with the number of failed tests as the exit code.
 *     LE_TEST_SUMMARY;
 * }
 * @endcode
 *
 * @section c_test_result Test Summary
 *
 * This code sample shows @c LE_TEST_SUMMARY was called after all tests have been performed.  The
 * LE_TEST_SUMMARY macro will cause the process to exit with the number of failed tests as the exit
 * code.
 *
 * Also, LE_TEST will log an error message if the test fails and will log an info message if the
 * test passes.
 *
 * If the unit test in the example above was ran in pass through mode and Test1 failed
 * and Test2 passed, the logs will contain the messages:
 *
@verbatim
 =ERR= | Unit Test Failed: 'Test1()'
  INFO | Unit Test Passed: 'Test2()'

  And the return code would be 1.
@endverbatim
 *
 * @note The log message format depends on the current log settings.
 *
 * @section c_test_multiProcess Flexible Test Scenarios
 *
 * The Legato Test Framework is designed to be flexible because developers may find that structuring
 * unit tests as a single thread, multiple threads or multiple processes is more natural for
 * certain unit tests.
 *
 * For unit tests that have multiple threads run the various tests, the normal testing procedure
 * will work because all the macros in this test framework are thread safe.
 *
 * For unit tests using multiple processes to run the various tests, the return codes for all
 * processes must be summed to get the total number of test failures.  This is easy if the processes
 * all have a common parent.
 *
 * In the case where multiple processes run the various tests but do not share a common parent, the
 * macro @c LE_TEST_NUM_FAILURES can be used to get the number of failures in the process and pass this
 * to another process using some other method (ie. sockets, shared memory, etc.).
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

//--------------------------------------------------------------------------------------------------
/** @file le_test.h
 *
 * Legato @ref c_test include file.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#ifndef LEGATO_TEST_INCLUDE_GUARD
#define LEGATO_TEST_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * @name Local definitions that should not be used directly.
 *
 * @{
 */
//--------------------------------------------------------------------------------------------------
void _le_test_Init(void);
void _le_test_Fail(void);
int _le_test_GetNumFailures(void);
// @}


//--------------------------------------------------------------------------------------------------
/**
 * Initializes the testing framework.  Must be called once before any tests are performed.
 */
//--------------------------------------------------------------------------------------------------
#define LE_TEST_INIT                _le_test_Init()


//--------------------------------------------------------------------------------------------------
/**
 * Performs the test.  If the test fails (testResult == false) then an error message is logged and
 * the process either exits or the number of test failures is incremented depending on the operating
 * mode.  If the test passes (testResult == true) then an info message is logged and this macro just
 * returns.
 */
//--------------------------------------------------------------------------------------------------
#define LE_TEST(testResult)         if (testResult) \
                                    { \
                                        LE_INFO("Unit Test Passed: '%s'", #testResult); \
                                    } \
                                    else \
                                    { \
                                        LE_ERROR("Unit Test Failed: '%s'", #testResult); \
                                        _le_test_Fail(); \
                                    }


//--------------------------------------------------------------------------------------------------
/**
 * Exits the process and returns the number of failed tests.
 */
//--------------------------------------------------------------------------------------------------
#define LE_TEST_SUMMARY             exit(_le_test_GetNumFailures());


//--------------------------------------------------------------------------------------------------
/**
 * Number of failed tests.
 */
//--------------------------------------------------------------------------------------------------
#define LE_TEST_NUM_FAILURES        _le_test_GetNumFailures()



#endif // LEGATO_TEST_INCLUDE_GUARD
