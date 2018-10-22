 /**
  * This module is for unit testing the le_clock module in the legato
  * runtime library.
  *
  * Copyright (C) Sierra Wireless Inc.
  *
  * TODO:
  *  - modify to fit into the new "app" structure
  *  - return non-zero status when test cases or asserts fail.
  */

#include "legato.h"
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>

#include <CUnit/Console.h>
#include <CUnit/Basic.h>

// These two macros are similar to CU_PASS and CU_FAIL, but allow an arbitrary value for the msg
// parameter, rather than assuming a string literal.
#define CU_PASS_MSG(msg) \
  { CU_assertImplementation(CU_TRUE, __LINE__, msg, STRINGIZE(LE_FILENAME), "", CU_FALSE); }

#define CU_FAIL_MSG(msg) \
  { CU_assertImplementation(CU_FALSE, __LINE__, msg, STRINGIZE(LE_FILENAME), "", CU_FALSE); }

// Date time formatted string
#define DATE_TIME_FORMATTED_STRING  "Tue Oct 10 10:00:58 2017"


// Is the test program running interactively.
bool IsInteractive=false;


void PrintLine(char* delimiter)
{
    int i;

    for (i=0; i<60; i++)
        putchar((int)*delimiter);
    printf("\n");
}


char GetUserResponse(char* msgStr)
{
    int answer;

    // Don't allow <return> to select a default value.  This will hopefully reduce errors,
    // if someone just presses <return> too quickly.
    do
    {
        printf("%s (y/n) ", msgStr);
        answer = getchar();

        if ( answer == EOF )
        {
            fprintf(stderr, "\nERROR: got EOF\n");
            exit(EXIT_FAILURE);
        }

        // Throw away the rest of the line.
        while ( getchar() != '\n' );

        answer = tolower(answer);

    } while ( answer != 'y' && answer != 'n' );

    return answer;
}


void StartTest(void)
{
    PrintLine("=");
}


void VerifyTest(char* msgStr)
{
    PrintLine("-");
    printf("%s\n", msgStr);

    // Don't wait for user input in non-interactive mode.
    if ( !IsInteractive )
    {
        printf("Verify result later\n\n");
    }
    else
    {
        char answer;

        answer = GetUserResponse("Pass?");

        if ( answer == 'y' )
            CU_PASS_MSG(msgStr)
        else
            CU_FAIL_MSG(msgStr);
    }
}


void TestClockBatch(void)
{
    char buffer[100];
    size_t numChars;
    le_result_t result;

    printf("\n");  // for better formatted test output

    /*
     * Clock related tests
     */

    // This will abort on error
    le_clk_GetRelativeTime();
    CU_PASS("Relative clock exists\n");

    // This will abort on error
    le_clk_GetAbsoluteTime();
    CU_PASS("Absolute clock exists\n");


    /*
     * UTC date/time related tests
     */

    // General tests
    result = le_clk_GetUTCDateTimeString(LE_CLK_STRING_FORMAT_DATE_TIME, buffer, sizeof(buffer), &numChars);
    CU_ASSERT_EQUAL(result, LE_OK);
    CU_ASSERT(numChars > 0);

    result = le_clk_GetUTCDateTimeString(LE_CLK_STRING_FORMAT_DATE_TIME, buffer, sizeof(buffer), NULL);
    CU_ASSERT_EQUAL(result, LE_OK);

    result = le_clk_GetUTCDateTimeString(LE_CLK_STRING_FORMAT_DATE_TIME, buffer, 5, &numChars);
    CU_ASSERT_EQUAL(result, LE_OVERFLOW);
    CU_ASSERT_EQUAL(numChars, 0);

    result = le_clk_GetUTCDateTimeString(LE_CLK_STRING_FORMAT_DATE, buffer, sizeof(buffer), &numChars);
    CU_ASSERT_EQUAL(result, LE_OK);
    CU_ASSERT_EQUAL(numChars, 8);

    result = le_clk_GetUTCDateTimeString(LE_CLK_STRING_FORMAT_TIME, buffer, sizeof(buffer), &numChars);
    CU_ASSERT_EQUAL(result, LE_OK);
    CU_ASSERT_EQUAL(numChars, 8);

    result = le_clk_SetUTCDateTimeString(LE_CLK_STRING_FORMAT_DATE_TIME, DATE_TIME_FORMATTED_STRING);
    CU_ASSERT_EQUAL(result, LE_NOT_PERMITTED);

    result = le_clk_SetUTCDateTimeString(NULL, DATE_TIME_FORMATTED_STRING);
    CU_ASSERT_EQUAL(result, LE_BAD_PARAMETER);

    result = le_clk_SetUTCDateTimeString("", DATE_TIME_FORMATTED_STRING);
    CU_ASSERT_EQUAL(result, LE_BAD_PARAMETER);

    result = le_clk_SetUTCDateTimeString(LE_CLK_STRING_FORMAT_DATE_TIME, NULL);
    CU_ASSERT_EQUAL(result, LE_BAD_PARAMETER);

    result = le_clk_SetUTCDateTimeString(LE_CLK_STRING_FORMAT_DATE_TIME, "");
    CU_ASSERT_EQUAL(result, LE_BAD_PARAMETER);

    // Testing %J
    result = le_clk_GetUTCDateTimeString("%J", buffer, sizeof(buffer), &numChars);
    CU_ASSERT_EQUAL(result, LE_OK);
    CU_ASSERT_EQUAL(numChars, 3);

    result = le_clk_GetUTCDateTimeString("%J", buffer, 2, &numChars);
    CU_ASSERT_EQUAL(result, LE_OVERFLOW);
    CU_ASSERT_EQUAL(numChars, 0);

    result = le_clk_GetUTCDateTimeString("%J", buffer, 3, &numChars);
    CU_ASSERT_EQUAL(result, LE_OVERFLOW);
    CU_ASSERT_EQUAL(numChars, 0);

    result = le_clk_GetUTCDateTimeString("%J", buffer, 4, &numChars);
    CU_ASSERT_EQUAL(result, LE_OK);
    CU_ASSERT_EQUAL(numChars, 3);

    result = le_clk_SetUTCDateTimeString("%J", "015");
    CU_ASSERT_EQUAL(result, LE_FAULT);

    // Testing %K
    result = le_clk_GetUTCDateTimeString("%K", buffer, sizeof(buffer), &numChars);
    CU_ASSERT_EQUAL(result, LE_OK);
    CU_ASSERT_EQUAL(numChars, 6);

    result = le_clk_GetUTCDateTimeString("%K", buffer, 5, &numChars);
    CU_ASSERT_EQUAL(result, LE_OVERFLOW);
    CU_ASSERT_EQUAL(numChars, 0);

    result = le_clk_GetUTCDateTimeString("%K", buffer, 6, &numChars);
    CU_ASSERT_EQUAL(result, LE_OVERFLOW);
    CU_ASSERT_EQUAL(numChars, 0);

    result = le_clk_GetUTCDateTimeString("%K", buffer, 7, &numChars);
    CU_ASSERT_EQUAL(result, LE_OK);
    CU_ASSERT_EQUAL(numChars, 6);

    result = le_clk_SetUTCDateTimeString("%K", "001015");
    CU_ASSERT_EQUAL(result, LE_FAULT);

    // Testing %J and %K
    result = le_clk_GetUTCDateTimeString("%J%K", buffer, sizeof(buffer), &numChars);
    CU_ASSERT_EQUAL(result, LE_OK);
    CU_ASSERT_EQUAL(numChars, 9);

    result = le_clk_GetUTCDateTimeString("%J%K", buffer, 2, &numChars);
    CU_ASSERT_EQUAL(result, LE_OVERFLOW);
    CU_ASSERT_EQUAL(numChars, 0);

    result = le_clk_GetUTCDateTimeString("%J%K", buffer, 8, &numChars);
    CU_ASSERT_EQUAL(result, LE_OVERFLOW);
    CU_ASSERT_EQUAL(numChars, 0);

    result = le_clk_GetUTCDateTimeString("%%J%%K", buffer, sizeof(buffer), &numChars);
    CU_ASSERT_EQUAL(result, LE_OK);
    CU_ASSERT_STRING_EQUAL(buffer, "%J%K");


    /*
     * Local date/time related tests
     */
    result = le_clk_GetLocalDateTimeString(LE_CLK_STRING_FORMAT_DATE_TIME, buffer, sizeof(buffer), &numChars);
    CU_ASSERT_EQUAL(result, LE_OK);
    CU_ASSERT(numChars > 0);

    result = le_clk_GetLocalDateTimeString(LE_CLK_STRING_FORMAT_DATE_TIME, buffer, sizeof(buffer), NULL);
    CU_ASSERT_EQUAL(result, LE_OK);

    result = le_clk_GetLocalDateTimeString(LE_CLK_STRING_FORMAT_DATE_TIME, buffer, 5, &numChars);
    CU_ASSERT_EQUAL(result, LE_OVERFLOW);
    CU_ASSERT_EQUAL(numChars, 0);
}


void TestClockInteractive(void)
{
    le_clk_Time_t timeVal;
    char buffer[100];
    size_t numChars;
    le_result_t result;

    printf("\n\n");  // for better formatted test output

    StartTest();
    timeVal = le_clk_GetRelativeTime();
    printf("Relative Seconds=%lu, Microseconds=%lu\n", timeVal.sec, timeVal.usec);
    VerifyTest("Verify Relative time looks okay");

    StartTest();
    timeVal = le_clk_GetAbsoluteTime();
    printf("Absolute Seconds=%lu, Microseconds=%lu\n", timeVal.sec, timeVal.usec);
    VerifyTest("Verify Absolute time looks okay");

    StartTest();
    le_clk_GetUTCDateTimeString(LE_CLK_STRING_FORMAT_DATE_TIME, buffer, sizeof(buffer), &numChars);
    printf("buffer:>>>%s<<<\n", buffer);
    VerifyTest("Verify %c in UTC time is correct");

    StartTest();
    result = le_clk_GetLocalDateTimeString(LE_CLK_STRING_FORMAT_DATE_TIME, buffer, sizeof(buffer), &numChars);
    CU_ASSERT_EQUAL(result, LE_OK);
    printf("buffer:>>>%s<<<\n", buffer);
    VerifyTest("Verify %c in local time is correct");

    StartTest();
    le_clk_GetUTCDateTimeString("milliseconds='%J', microseconds='%K'", buffer, sizeof(buffer), &numChars);
    printf("buffer:>>>%s<<<\n", buffer);
    VerifyTest("Verify ms and us is correct");

}


COMPONENT_INIT
{
    // Init the test case / test suite data structures

    CU_TestInfo testBatch[] =
    {
        { "Batch clock tests",               TestClockBatch },
        CU_TEST_INFO_NULL,
    };

    CU_TestInfo testInteractive[] =
    {
        { "Interactive clock tests",         TestClockInteractive },
        CU_TEST_INFO_NULL,
    };

    CU_SuiteInfo suites[] =
    {
        { "Batch tests",               NULL, NULL, testBatch },
        { "Interactive tests",         NULL, NULL, testInteractive },
        CU_SUITE_INFO_NULL,
    };

    // Parse command line options
    if (LE_OK == le_arg_GetFlagOption("i", "interactive"))
    {
        IsInteractive = true;
    }
    else if (0 != le_arg_NumArgs())
    {
        fprintf(stderr, "Usage: %s [-i]\n", le_arg_GetProgramName());
        exit(EXIT_FAILURE);
    }

    // Initialize the CUnit test registry and register the test suite
    if (CUE_SUCCESS != CU_initialize_registry())
        exit(CU_get_error());

    if ( CUE_SUCCESS != CU_register_suites(suites))
    {
        CU_cleanup_registry();
        exit(CU_get_error());
    }

    // Run either interactive or backgroud; default is background
    if ( IsInteractive )
    {
        CU_console_run_tests();
    }
    else
    {
        //CU_basic_set_mode(CU_BRM_NORMAL);
        CU_basic_set_mode(CU_BRM_VERBOSE);

        // It is possible to just run the batch tests here, using CU_basic_run_suite(), but
        // I think there is value in running all suites, even if the interactive tests are
        // not fully verified here.
        //CU_basic_run_suite(CU_get_suite("Batch tests"));
        CU_basic_run_tests();
    }

    // Output summary of failures, if there were any
    if ( CU_get_number_of_failures() > 0 )
    {
        fprintf(stdout,"\n [START]List of Failure :\n");
        CU_basic_show_failures(CU_get_failure_list());
        fprintf(stdout,"\n [STOP]List of Failure\n");
    }

    CU_cleanup_registry();
    exit(CU_get_error());
}


