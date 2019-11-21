/**
 * Simple test of Legato clock API
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"

#define SECONDS_IN_HOUR 3600

/**
 * Clock related tests
 */
static void TestClock(void)
{
    // This will abort on error
    le_clk_GetRelativeTime();
    LE_TEST_OK(true, "Relative clock exists");

    // This will abort on error
    le_clk_GetAbsoluteTime();
    LE_TEST_OK(true, "Absolute clock exists");
}

/**
 * UTC date/time related tests
 */
static void TestDateTimeString(void)
{
    size_t numChars;
    char buffer[100];
    le_result_t result;
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
}

/**
 * Helper Function for verifying gmtime_r and localtime_r results
 */
static bool VerifyTimeMatch(struct tm* pExpected,  struct tm* pActual)
{
    if (!pExpected || !pActual)
        return false;

    if ((pExpected->tm_year != pActual->tm_year) ||
        (pExpected->tm_mon != pActual->tm_mon)   ||
        (pExpected->tm_mday != pActual->tm_mday) ||
        (pExpected->tm_hour != pActual->tm_hour) ||
        (pExpected->tm_min != pActual->tm_min) ||
        (pExpected->tm_sec != pActual->tm_sec) ||
        (pExpected->tm_wday != pActual->tm_wday) ||
        (pExpected->tm_yday != pActual->tm_yday) ||
        (pExpected->tm_isdst != pActual->tm_isdst))
    {
        LE_TEST_INFO("Expected year %d, month %d, mday %d, hour %d, "
                "min %d, sec %d, wday %d, yday %d, isdst %d",
                pExpected->tm_year, pExpected->tm_mon, pExpected->tm_mday,
                pExpected->tm_hour, pExpected->tm_min, pExpected->tm_sec,
                pExpected->tm_wday, pExpected->tm_yday, pExpected->tm_isdst);

        LE_TEST_INFO("Got year %d, month %d, mday %d, hour %d, "
                "min %d, sec %d, wday %d, yday %d, isdst %d",
                pActual->tm_year, pActual->tm_mon, pActual->tm_mday,
                pActual->tm_hour, pActual->tm_min, pActual->tm_sec,
                pActual->tm_wday, pActual->tm_yday, pActual->tm_isdst);

        return false;
    }

    return true;
}

/**
 * Helper function to change the timezone for test cases
 */
static void UpdateTzInfo
(
    int32_t timezoneHr,     ///< [IN] set timezone offset +/- in hours
    uint8_t dst,            ///< [IN] set the dst
    const time_t* num,            ///< [IN] number of seconds past epoch
    struct tm* actualTm     ///< [OUT] pointer to store result
)
{
    le_clk_SetTimezoneFile(timezoneHr * SECONDS_IN_HOUR, dst);
    tzset();
    localtime_r(num, actualTm);
}

static void TestTime()
{
    struct tm expectedTm;
    struct tm actualTm;
    time_t num = 0;

    num = 619215418;
    expectedTm = (struct tm) {
        .tm_sec = 58,
        .tm_min = 16,
        .tm_hour = 20,
        .tm_mday = 15,
        .tm_mon = 7,
        .tm_year = 89,
        .tm_wday = 2,
        .tm_yday = 226,
        .tm_isdst = 0,
    };

    LE_TEST_OK(num == timegm(&expectedTm), "Test timegm at time %lu", num);

    gmtime_r(&num, &actualTm);
    LE_TEST_OK(VerifyTimeMatch(&expectedTm, &actualTm),
               "Test gmtime_r at time %lu", num);

    LE_TEST_BEGIN_SKIP(!LE_CONFIG_IS_ENABLED(DUT_TARGET_gill), 2);
    UpdateTzInfo(4, 1, &num, &actualTm);
    expectedTm.tm_hour = 1;
    expectedTm.tm_mday = 16;
    expectedTm.tm_wday = 3;
    expectedTm.tm_yday = 227;
    expectedTm.tm_isdst = 1;
    LE_TEST_OK(VerifyTimeMatch(&expectedTm, &actualTm),
               "Test localtime_r GMT +4 and DST +1 at time %lu", num);

    UpdateTzInfo(-3, 2, &num, &actualTm);
    expectedTm.tm_hour = 19;
    expectedTm.tm_mday = 15;
    expectedTm.tm_wday = 2;
    expectedTm.tm_yday = 226;
    expectedTm.tm_isdst = 1;
    LE_TEST_OK(VerifyTimeMatch(&expectedTm, &actualTm),
               "Test localtime_r GMT -3 and DST +2 at time %lu", num);
    LE_TEST_END_SKIP();

    num = 1939003259;
    expectedTm = (struct tm) {
        .tm_sec = 59,
        .tm_min = 0,
        .tm_hour = 4,
        .tm_mday = 12,
        .tm_mon = 5,
        .tm_year = 131,
        .tm_wday = 4,
        .tm_yday = 162,
        .tm_isdst = 0,
    };

    LE_TEST_OK(num == timegm(&expectedTm), "Test timegm at time %lu", num);

    gmtime_r(&num, &actualTm);
    LE_TEST_OK(VerifyTimeMatch(&expectedTm, &actualTm),
               "Test gmtime_r at time %lu", num);

    LE_TEST_BEGIN_SKIP(!LE_CONFIG_IS_ENABLED(DUT_TARGET_gill), 2);
    UpdateTzInfo(13, 1, &num, &actualTm);
    expectedTm.tm_hour = 18;
    expectedTm.tm_isdst = 1;
    LE_TEST_OK(VerifyTimeMatch(&expectedTm, &actualTm),
               "Test localtime_r GMT +13 and DST +1 at time %lu", num);

    UpdateTzInfo(-5, 0, &num, &actualTm);
    expectedTm.tm_hour = 23;
    expectedTm.tm_mday = 11;
    expectedTm.tm_wday = 3;
    expectedTm.tm_yday = 161;
    expectedTm.tm_isdst = 0;
    LE_TEST_OK(VerifyTimeMatch(&expectedTm, &actualTm),
               "Test localtime_r GMT -5 and no DST at time %lu", num);
    LE_TEST_END_SKIP();

    // Test leap year 2000
    num = 951782400;
    expectedTm = (struct tm) {
        .tm_sec = 0,
        .tm_min = 0,
        .tm_hour = 0,
        .tm_mday = 29,
        .tm_mon = 1,
        .tm_year = 100,
        .tm_wday = 2,
        .tm_yday = 59,
        .tm_isdst = 0,
    };

    LE_TEST_OK(num == timegm(&expectedTm), "Test timegm at time %lu", num);

    gmtime_r(&num, &actualTm);
    LE_TEST_OK(VerifyTimeMatch(&expectedTm, &actualTm),
               "Test gmtime_r at time %lu", num);

    LE_TEST_BEGIN_SKIP(!LE_CONFIG_IS_ENABLED(DUT_TARGET_gill), 1);
    UpdateTzInfo(-2, 0, &num, &actualTm);
    expectedTm.tm_hour = 22;
    expectedTm.tm_mday = 28;
    expectedTm.tm_wday = 1;
    expectedTm.tm_yday = 58;
    LE_TEST_OK(VerifyTimeMatch(&expectedTm, &actualTm),
               "Test localtime_r GMT -2 and no DST at time %lu", num);
    LE_TEST_END_SKIP();

    // Test leap year 2080
    num = 3476520032ul;
    expectedTm = (struct tm) {
        .tm_sec = 32,
        .tm_min = 0,
        .tm_hour = 12,
        .tm_mday = 1,
        .tm_mon = 2,
        .tm_year = 180,
        .tm_wday = 5,
        .tm_yday = 60,
        .tm_isdst = 0,
    };

    LE_TEST_OK(num == timegm(&expectedTm), "Test timegm at time %lu", num);

    gmtime_r(&num, &actualTm);
    LE_TEST_OK(VerifyTimeMatch(&expectedTm, &actualTm),
               "Test gmtime_r at time %lu", num);
}

static void TestTzset()
{
    struct tm expectedTm;
    struct tm actualTm;
    static const time_t seconds = 619215418;
    expectedTm = (struct tm) {
        .tm_sec = 58,
        .tm_min = 16,
        .tm_hour = 20,
        .tm_mday = 15,
        .tm_mon = 7,
        .tm_year = 89,
        .tm_wday = 2,
        .tm_yday = 226,
        .tm_isdst = 0,
    };

    // Initialize to no offsets before starting test
    le_clk_SetTimezoneFile(0, 0);
    tzset();

    LE_TEST_BEGIN_SKIP(!LE_CONFIG_IS_ENABLED(DUT_TARGET_gill), 3);
    //invalid tz file case
    UpdateTzInfo(15, 0, &seconds, &actualTm);
    LE_TEST_OK(VerifyTimeMatch(&expectedTm, &actualTm),
            "Timezone offset > GMT +14 won't be applied");

    UpdateTzInfo(-15, 0, &seconds, &actualTm);
    LE_TEST_OK(VerifyTimeMatch(&expectedTm, &actualTm),
            "Timezone offset < GMT -12 won't be applied");
    //typical value case 1
    UpdateTzInfo(0, 4, &seconds, &actualTm);
    LE_TEST_OK(VerifyTimeMatch(&expectedTm, &actualTm),
            "DST > 2 won't be applied");
    LE_TEST_END_SKIP();
}

static void TestFailCases()
{
    struct tm testTm;
    struct tm *testNullTm = NULL;
    time_t testNum = 123;
    time_t *testNullNum = NULL;

    LE_TEST_OK(localtime_r(&testNum, testNullTm) == NULL,
               "localtime_r null result pointer");
    LE_TEST_OK(gmtime_r(testNullNum, &testTm) == NULL, "gmtime_r null result pointer");

    LE_TEST_OK(localtime_r(&testNum, testNullTm) == NULL,
               "localtime_r null time input");
    LE_TEST_OK(gmtime_r(testNullNum, &testTm) == NULL, "gmtime_r null time input");
}

COMPONENT_INIT
{
    LE_TEST_PLAN(60);
    TestTzset();
    TestClock();
    TestDateTimeString();
    TestTime();
    TestFailCases();
    LE_TEST_EXIT;
}
