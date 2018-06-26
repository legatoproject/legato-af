/**
 * Simple test of Legato clock API
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"

COMPONENT_INIT
{
    char buffer[100];
    size_t numChars;
    le_result_t result;

    LE_TEST_PLAN(40);

    /*
     * Clock related tests
     */

    // This will abort on error
    le_clk_GetRelativeTime();
    LE_TEST_OK(true, "Relative clock exists");

    // This will abort on error
    le_clk_GetAbsoluteTime();
    LE_TEST_OK(true, "Absolute clock exists");


    /*
     * UTC date/time related tests
     */

    // General tests

    result = le_clk_GetUTCDateTimeString(LE_CLK_STRING_FORMAT_DATE_TIME, buffer, sizeof(buffer),
                                         &numChars);
    LE_TEST_OK(LE_OK == result, "date time GetUTCDateTimeString() returns OK");
    LE_TEST_OK(numChars > 0, "date time GetUTCDateTimeString() produces output");

    result = le_clk_GetUTCDateTimeString(LE_CLK_STRING_FORMAT_DATE_TIME, buffer, sizeof(buffer),
                                         NULL);
    LE_TEST_OK(LE_OK == result, "GetUTCDateTimeString returns OK without number of characters");

    result = le_clk_GetUTCDateTimeString(LE_CLK_STRING_FORMAT_DATE_TIME, buffer, 5, &numChars);
    LE_TEST_OK(result == LE_OVERFLOW, "GetUTCDateTimeString overflow returns LE_OVERFLOW");
    LE_TEST_OK(numChars == 0, "GetUTCDateTimeString overflow produces no output");

    result = le_clk_GetUTCDateTimeString(LE_CLK_STRING_FORMAT_DATE, buffer, sizeof(buffer),
                                         &numChars);
    LE_TEST_OK(LE_OK == result, "date GetUTCDateTimeString() returns OK");
    LE_TEST_OK(numChars == 8, "date GetUTCDateTimeString() output is 8 characters");

    result = le_clk_GetUTCDateTimeString(LE_CLK_STRING_FORMAT_TIME, buffer, sizeof(buffer),
                                         &numChars);
    LE_TEST_OK(LE_OK == result, "time GetUTCDateTimeString() returns OK");
    LE_TEST_OK(numChars == 8, "time GetUTCDateTimeString() output is 8 characters");

    // Testing %J

    result = le_clk_GetUTCDateTimeString("%J", buffer, sizeof(buffer), &numChars);
    LE_TEST_OK(LE_OK == result, "%%J GetUTCDateTimeString() returns OK");
    LE_TEST_OK(numChars == 3, "%%J GetUTCDateTimeString() output is 8 characters");

    result = le_clk_GetUTCDateTimeString("%J", buffer, 2, &numChars);
    LE_TEST_OK(result == LE_OVERFLOW, "%%J GetUTCDateTimeString() 2 byte buffer return");
    LE_TEST_OK(numChars == 0, "%%J GetUTCDateTimeString() 2 byte buffer output size");

    result = le_clk_GetUTCDateTimeString("%J", buffer, 3, &numChars);
    LE_TEST_OK(result == LE_OVERFLOW, "%%J GetUTCDateTimeString() 3 byte buffer return");
    LE_TEST_OK(numChars == 0, "%%J GetUTCDateTimeString() 3 byte buffer output size");

    result = le_clk_GetUTCDateTimeString("%J", buffer, 4, &numChars);
    LE_TEST_OK(LE_OK == result, "%%J GetUTCDateTimeString() 4 byte buffer return");
    LE_TEST_OK(numChars == 3, "%%J GetUTCDateTimeString() 4 byte buffer output size");

    // Testing %K

    result = le_clk_GetUTCDateTimeString("%K", buffer, sizeof(buffer), &numChars);
    LE_TEST_OK(LE_OK == result, "%%K GetUTCDateTimeString() returns OK");
    LE_TEST_OK(numChars == 6, "%%K GetUTCDateTimeString() output is 6 characters");

    result = le_clk_GetUTCDateTimeString("%K", buffer, 5, &numChars);
    LE_TEST_OK(result == LE_OVERFLOW, "%%K GetUTCDateTimeString() 5 byte buffer return");
    LE_TEST_OK(numChars == 0, "%%K GetUTCDateTimeString() 5 byte buffer output size");

    result = le_clk_GetUTCDateTimeString("%K", buffer, 6, &numChars);
    LE_TEST_OK(result == LE_OVERFLOW, "%%K GetUTCDateTimeString() 6 byte buffer return");
    LE_TEST_OK(numChars == 0, "%%K GetUTCDateTimeString() 6 byte buffer output size");

    result = le_clk_GetUTCDateTimeString("%K", buffer, 7, &numChars);
    LE_TEST_OK(LE_OK == result, "%%K GetUTCDateTimeString() 7 byte buffer return");
    LE_TEST_OK(numChars == 6, "%%K GetUTCDateTimeString() 7 byte buffer output size");

    // Testing %J and %K

    result = le_clk_GetUTCDateTimeString("%J%K", buffer, sizeof(buffer), &numChars);
    LE_TEST_OK(LE_OK == result, "%%J%%K GetUTCDateTimeString() returns OK");
    LE_TEST_OK(numChars == 9, "%%J%%K GetUTCDateTimeString() output is 9 characters");

    result = le_clk_GetUTCDateTimeString("%J%K", buffer, 2, &numChars);
    LE_TEST_OK(result == LE_OVERFLOW, "%%J%%K GetUTCDateTimeString() 2 byte buffer return");
    LE_TEST_OK(numChars == 0, "%%J%%K GetUTCDateTimeString() 2 byte buffer output size");

    result = le_clk_GetUTCDateTimeString("%J%K", buffer, 8, &numChars);
    LE_TEST_OK(result == LE_OVERFLOW, "%%J%%K GetUTCDateTimeString() 8 byte buffer return");
    LE_TEST_OK(numChars == 0, "%%J%%K GetUTCDateTimeString() 8 byte buffer output size");

    result = le_clk_GetUTCDateTimeString("%%J%%K", buffer, sizeof(buffer), &numChars);
    LE_TEST_OK(LE_OK == result, "%%%%J%%%%K GetUTCDateTimeString() returns OK");
    LE_TEST_OK(strcmp(buffer, "%J%K") == 0, "%%%%J%%%%K GetUTCDateTimeString() gives correct output");


    /*
     * Local date/time related tests
     */

    result = le_clk_GetLocalDateTimeString(LE_CLK_STRING_FORMAT_DATE_TIME, buffer, sizeof(buffer),
                                           &numChars);
    LE_TEST_OK(LE_OK == result, "simple GetLocalDateTimeString() returns OK");
    LE_TEST_OK(numChars > 0, "simple GetLocalDateTimeString() returns some output");

    result = le_clk_GetLocalDateTimeString(LE_CLK_STRING_FORMAT_DATE_TIME, buffer, sizeof(buffer),
                                           NULL);
    LE_TEST_OK(LE_OK == result, "GetLocalDateTimeString() with no numChars returns OK");

    result = le_clk_GetLocalDateTimeString(LE_CLK_STRING_FORMAT_DATE_TIME, buffer, 5, &numChars);
    LE_TEST_OK(result == LE_OVERFLOW, "GetLocalDateTimeString() overflow returns LE_OVERFLOW");
    LE_TEST_OK(numChars == 0, "GetLocalDateTimeString() overflow produces no output");

    LE_TEST_EXIT;
}
