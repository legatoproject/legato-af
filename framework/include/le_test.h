/**
 * @page c_test Unit Testing API
 *
 * @subpage le_test.h "API Reference"
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
 * The Legato test framework outputs test results to the log in TAP format (prefixed by "TAP |")
 * This allows test results to be processed with many test harnasses.
 *
 * @section c_test_setup Setting Up the Test Framework
 *
 * To setup the Legato Test Framework, call the @c LE_TEST_PLAN macro, once before any tests
 * are started. The macro takes the total number of planned tests as single argument.
 *
 * @section c_test_testing Performing Tests
 *
 * To perform tests, call the @c LE_TEST_OK macro.  The first argument is whether the
 * test passed (true or false).  The second argument is the name of the test.
 *
 * For example:
 *
 * @code
 * #include "legato.h"
 *
 * // Returns true if the test passes, otherwise returns false.
 * bool ComplexTest(void)
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
 *     LE_TEST_PLAN(2);
 *
 *     // Run the tests.
 *     // Do some initializations and/or calculations.
 *     ...
 *
 *     LE_TEST_OK(TestFunction(arguments) == EXPECTED_VALUE, "simple test");
 *     LE_TEST_OK(ComplexTest(), "complex test");
 *
 *     // Exit with the number of failed tests as the exit code.
 *     LE_TEST_EXIT;
 * }
 * @endcode
 *
 * @section c_test_exit Exiting a Test Program
 *
 * When a test program is finished executing tests and needs to exit, it should exit
 * using the LE_TEST_EXIT macro.
 *
 * If a test suite needs to exit early it should use the LE_TEST_FATAL() macro.  This will
 * log a message indicating the test suite has been aborted.  LE_ASSERT()
 * and LE_FATAL() macros should not be used as they will not print this message.
 *
 * As a convenience you can also use the LE_TEST_ASSERT() macro which will abort the test suite
 * if the test fails.  This is useful if running further test cases is pointless after this failure.
 *
 * @section c_test_result Test Results
 *
 * The LE_TEST_EXIT macro will cause the process to exit with the number of failed tests as the exit
 * code.
 *
 * @note The log message format depends on the current log settings.
 *
 * @section c_test_multiThread Multi-Threaded Tests
 *
 * For unit tests that contain multiple threads run the various tests, these macros can still
 * be used.  However your test program should ensure that only one thread uses the unit test
 * API at a time.
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
 * @name Local definitions that should not be used directly.
 *
 * @{
 */
//--------------------------------------------------------------------------------------------------
void _le_test_Init(int testCount);
void _le_test_Exit(void) __attribute__((noreturn));
bool _le_test_CheckNeedsPlan(void);
void _le_test_Fail(void);
int _le_test_NumberTest(void);
int _le_test_GetNumTests(void);
int _le_test_GetNumFailures(void);
bool _le_test_SetTodo(bool todo);
void _le_test_Skip(int count);
bool _le_test_IsSkipping(void);
const char* _le_test_GetTag(void);
#define LE_TEST_OUTPUT(format, ...) LE_INFO("TAP | " format, ##__VA_ARGS__)
// @}

//--------------------------------------------------------------------------------------------------
/**
 * Indicate the number of tests is not known in advance.
 */
//--------------------------------------------------------------------------------------------------
#define LE_TEST_NO_PLAN   -1

//--------------------------------------------------------------------------------------------------
/**
 * Initializes a test plan.  Must be called once before any tests are performed.
 *
 * @param testCount  Number of tests expected in this test case.
 */
//--------------------------------------------------------------------------------------------------
#define LE_TEST_PLAN(testCount) do {                                    \
        if (testCount >= 0) { LE_TEST_OUTPUT("1..%d", testCount); }     \
        _le_test_Init(testCount);                                       \
    } while(0)


//--------------------------------------------------------------------------------------------------
/**
 * Initializes the testing framework.  Must be called once before any tests are performed.
 *
 * Obselete synonym for LE_TEST_PLAN(LE_TEST_NO_PLAN)
 */
//--------------------------------------------------------------------------------------------------
#define LE_TEST_INIT                LE_TEST_PLAN(LE_TEST_NO_PLAN)


//--------------------------------------------------------------------------------------------------
/**
 * Perform the test
 */
//--------------------------------------------------------------------------------------------------
#define LE_TEST_OK(test, testName, ...) do {                            \
        if (test)                                                       \
        {                                                               \
            LE_TEST_OUTPUT("ok %d - " testName "%s",                    \
                           _le_test_NumberTest(),                       \
                           ##__VA_ARGS__,                               \
                           _le_test_GetTag());                          \
        }                                                               \
        else                                                            \
        {                                                               \
            LE_TEST_OUTPUT("not ok %d - " testName "%s",                \
                           _le_test_NumberTest(),                       \
                           ##__VA_ARGS__,                               \
                           _le_test_GetTag());                          \
            _le_test_Fail();                                            \
        }                                                               \
    } while(0)

//--------------------------------------------------------------------------------------------------
/**
 * Performs the test.
 *
 * For new tests, LE_TEST_OK is preferred which gives an option to set the test name.
 */
//--------------------------------------------------------------------------------------------------
#define LE_TEST(testResult) LE_TEST_OK((testResult), #testResult)


//--------------------------------------------------------------------------------------------------
/**
 * Bail out of a test suite early.
 *
 * Using this instead of LE_FATAL ensures the test harness is notified the test suite is
 * exiting abnormally.
 */
//--------------------------------------------------------------------------------------------------
#define LE_TEST_FATAL(...) do {                         \
        LE_TEST_OUTPUT("Bail out! " __VA_ARGS__);       \
        _le_test_Exit();                                \
    } while(0)

//--------------------------------------------------------------------------------------------------
/**
 * Perform a test, and bail out if the test fails.
 *
 * This should only be used if a test suite cannot continue if this test fails.  Generally
 * LE_TEST_OK should be used instead.
 */
//--------------------------------------------------------------------------------------------------
#define LE_TEST_ASSERT(test, testName, ...) do {                            \
        if (test)                                                       \
        {                                                               \
            LE_TEST_OUTPUT("ok %d - " testName "%s",                    \
                           _le_test_NumberTest(),                       \
                           ##__VA_ARGS__,                               \
                           _le_test_GetTag());                          \
        }                                                               \
        else                                                            \
        {                                                               \
            LE_TEST_OUTPUT("not ok %d - " testName "%s",                \
                           _le_test_NumberTest(),                       \
                           ##__VA_ARGS__,                               \
                           _le_test_GetTag());                          \
            _le_test_Fail();                                            \
            LE_TEST_FATAL();                                            \
        }                                                               \
    } while(0)


//--------------------------------------------------------------------------------------------------
/**
 * Ouptut some diagnostic information.  In tests this should be used instead of LE_INFO so
 * the output appears in the test results
 */
//--------------------------------------------------------------------------------------------------
#define LE_TEST_INFO(...) LE_TEST_OUTPUT("# " __VA_ARGS__)

//--------------------------------------------------------------------------------------------------
/**
 * Mark a test as not yet implemented.  Todo tests will still be run, but are expected to fail.
 *
 * Begins a block which must be terminated by LE_TEST_END_TODO()
 */
//--------------------------------------------------------------------------------------------------
#define LE_TEST_BEGIN_TODO(cond) do {                                   \
        bool _le_test_OldTodo = _le_test_SetTodo(cond);                 \

//--------------------------------------------------------------------------------------------------
/**
 * End a block of tests which may not be implemented yet.
 */
//--------------------------------------------------------------------------------------------------
#define LE_TEST_END_TODO()                          \
        _le_test_SetTodo(_le_test_OldTodo);         \
    } while(0)


//--------------------------------------------------------------------------------------------------
/**
 * Mark a test as skipped.  Skipped tests will not be run, and will always pass.
 *
 * Begins a block which must be terminated by LE_TEST_END_SKIP()
 */
//--------------------------------------------------------------------------------------------------
#define LE_TEST_BEGIN_SKIP(cond, count) do {                            \
          if (cond)                                                     \
          {                                                             \
              _le_test_Skip(count);                                     \
              break;                                                    \
          }

//--------------------------------------------------------------------------------------------------
/**
 * End a block of tests which may not be implemented yet.
 */
//--------------------------------------------------------------------------------------------------
#define LE_TEST_END_SKIP()                              \
    } while(0)


//--------------------------------------------------------------------------------------------------
/**
 * Exits the process and returns the number of failed tests.
 */
//--------------------------------------------------------------------------------------------------
#define LE_TEST_EXIT do {                                       \
        if (_le_test_CheckNeedsPlan())                          \
        {                                                       \
            LE_TEST_OUTPUT("1..%d", _le_test_GetNumTests());    \
        }                                                       \
        _le_test_Exit();                                        \
    } while(0)

/// <b> *DEPRECATED* </b> old name for LE_TEST_EXIT.
#define LE_TEST_SUMMARY LE_TEST_EXIT


#endif // LEGATO_TEST_INCLUDE_GUARD
