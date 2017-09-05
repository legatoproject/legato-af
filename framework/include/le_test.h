/**
 * @page c_test Unit Testing API
 *
 * @ref le_test.h "API Reference"
 *
 * <HR>
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
 * #include "legato.h"
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
 *     LE_TEST_EXIT;
 * }
 * @endcode
 *
 * @section c_test_exit Exiting a Test Program
 *
 * When a test program is finished executing tests and needs to exit, it should always exit
 * using the LE_TEST_EXIT macro.
 *
 * It's also okay to exit using LE_FATAL(), LE_FATAL_IF() or LE_ASSERT(), if the test must be
 * halted immeditately due to some failure that cannot be recovered from.
 *
 * @section c_test_result Test Results
 *
 * The LE_TEST_EXIT macro will cause the process to exit with the number of failed tests as the exit
 * code.
 *
 * Also, LE_TEST will log an error message if the test fails and will log an info message if the
 * test passes.
 *
 * If the unit test in the example above was run in "pass-through mode" (continue even when a
 * test fails) and Test1 failed and Test2 passed, the logs will contain the messages:
 *
@verbatim
 =ERR= | Unit Test Failed: 'Test1()'
  INFO | Unit Test Passed: 'Test2()'
@endverbatim
 *
 * And the return code would be 1.
 *
 * @note The log message format depends on the current log settings.
 *
 * @section c_test_multiThread Multi-Threaded Tests
 *
 * For unit tests that contain multiple threads run the various tests, the normal testing procedure
 * will work because all the macros in this test framework are thread safe.
 *
 * @section c_test_multiProcess Multi-Process Tests
 *
 * For unit tests that require the use of multiple concurrent processes, a single process can
 * fork the other processes using LE_TEST_FORK() and then wait for them to terminate using
 * LE_TEST_JOIN().
 *
 * When a child that is being waited for terminates, LE_TEST_JOIN() will look at the child's
 * termination status and add the results to the running test summary.
 *
 * If the child process exits normally with a non-negative exit code, that exit code will be
 * considered a count of the number of test failures that occurred in that child process.
 *
 * If the child exits normally with a negative exit code or if the child is terminated due to
 * a signal (which can be caused by a segmentation fault, etc.), LE_TEST_JOIN() will count one test
 * failure for that child process.
 *
 * @code
 * COMPONENT_INIT
 * {
 *     // Setup the Legato Test Framework.
 *     LE_TEST_INIT;
 *
 *     // Run the test programs.
 *     le_test_ChildRef_t test1 = LE_TEST_FORK("test1");
 *     le_test_ChildRef_t test2 = LE_TEST_FORK("test2");
 *
 *     // Wait for the test programs to finish and tally the results.
 *     LE_TEST_JOIN(test1);
 *     LE_TEST_JOIN(test2);
 *
 *     // Exit with the number of failed tests as the exit code.
 *     LE_TEST_EXIT;
 * }
 * @endcode
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc.
 */

//--------------------------------------------------------------------------------------------------
/** @file le_test.h
 *
 * Legato @ref c_test include file.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_TEST_INCLUDE_GUARD
#define LEGATO_TEST_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Reference to a forked child process.  See LE_TEST_FORK() and LE_TEST_JOIN().
 **/
//--------------------------------------------------------------------------------------------------
typedef struct le_test_Child* le_test_ChildRef_t;


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
le_test_ChildRef_t _le_test_Fork(const char* exePath, ...);
void _le_test_Join(le_test_ChildRef_t child);
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
#define LE_TEST_EXIT                exit(_le_test_GetNumFailures());

/// <b> *DEPRECATED* </b> old name for LE_TEST_EXIT.
#define LE_TEST_SUMMARY LE_TEST_EXIT


//--------------------------------------------------------------------------------------------------
/**
 * Fork a child process and have it execute a given program.
 *
 * @param exePath [in] String containing the path to the program to be executed.  If this is not
 *                     an absolute path (doesn't start with a slash '/'), then the PATH environment
 *                     variable will be used to search for the executable.
 *
 * A variable list of subsequent parameters is allowed, each of which will be passed as a
 * command-line argument to the child program when it is executed.
 *
 * @return A reference to the child process (see LE_TEST_JOIN()).
 **/
//--------------------------------------------------------------------------------------------------
#define LE_TEST_FORK(exePath, ...) _le_test_Fork(exePath, ##__VA_ARGS__, NULL)


//--------------------------------------------------------------------------------------------------
/**
 * Wait for a given child process to terminate and add its results to the running test summary.
 *
 * @param child [IN] Child process reference returned by LE_TEST_FORK().
 **/
//--------------------------------------------------------------------------------------------------
#define LE_TEST_JOIN(child)     _le_test_Join(child)


#endif // LEGATO_TEST_INCLUDE_GUARD
