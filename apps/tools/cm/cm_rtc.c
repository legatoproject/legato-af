//-------------------------------------------------------------------------------------------------
/**
 * @file cm_rtc.c
 *
 * Handle RTC related functionality.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//-------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "cm_rtc.h"
#include "cm_common.h"
#include <time.h>

//-------------------------------------------------------------------------------------------------
/**
 * Print the IPS help text to stdout.
 */
//-------------------------------------------------------------------------------------------------
void cm_rtc_PrintRtcHelp
(
    void
)
{
    printf("RTC usage\n"
           "==========\n\n"
           "To read the RTC time:\n"
           "\tcm rtc read\n\n"
           "To set the RTC time:\n"
           "\tcm rtc set \"25 Dec 2015 12:30:45\"\n"
           "time format:\n"
           "- day of the month (leading zeros are permitted)\"\n"
           "- month (either the abbreviated or the full name)\"\n"
           "- year with century\"\n"
           "- hour (leading zeros are permitted)\"\n"
           "- minute (leading zeros are permitted)\"\n"
           "- seconds (leading zeros are permitted)\"\n"
           );
}

//-------------------------------------------------------------------------------------------------
/**
 * Read the RTC time.
 */
//-------------------------------------------------------------------------------------------------
static le_result_t ReadAndPrintRtc
(
    void
)
{
    le_result_t result = LE_FAULT;
    uint64_t epochTime;

    result = le_rtc_GetUserTime(&epochTime);
    if (result == LE_OK)
    {
        char buff[255];
        memset(buff,0,255);
        struct tm tm;

        time_t time = (epochTime/1000) + CM_DELTA_POSIX_TIME_EPOCH_GPS_TIME_EPOCH_IN_SEC;
        LE_DEBUG(" read posixEpochtime %ld seconds\n", (time_t) time);

        snprintf(buff, sizeof(buff), "%ld", (long) time);
        strptime(buff, "%s", &tm);
        memset(buff,0,255);
        strftime(buff, sizeof(buff), "%d %b %Y %H:%M:%S", &tm);


        printf("%s\n", buff);
    }

    return result;
}

//-------------------------------------------------------------------------------------------------
/**
 * Set RTC.
 */
//-------------------------------------------------------------------------------------------------
static le_result_t SetRtc
(
    void
)
{
    struct tm tm;
    time_t t;
    const char* datePtr = le_arg_GetArg(2);

    char* lastCarPtr;
    lastCarPtr = strptime(datePtr, "%d %b %Y %H:%M:%S", &tm);

    if ((NULL == lastCarPtr) || (*lastCarPtr != '\0'))
    {
        printf("Failed to get the time.\n");
        return LE_FAULT;
    }

    // Not set by strptime(); tells mktime() to determine whether
    // daylight saving time is in effect
    tm.tm_isdst = -1;

    t = mktime(&tm);

    if (t == -1)
    {
        printf("mktime error\n");
        return LE_FAULT;
    }

    uint64_t gpsEpochTimeMs =
       ((uint64_t) t - CM_DELTA_POSIX_TIME_EPOCH_GPS_TIME_EPOCH_IN_SEC) * 1000;

    LE_DEBUG(" posixEpochtime: %ld seconds, gpsEpochTime: %llu seconds",
                                          (time_t) t,
                                          (unsigned long long int)(gpsEpochTimeMs/1000));

    return le_rtc_SetUserTime(gpsEpochTimeMs);
}


//--------------------------------------------------------------------------------------------------
/**
 * Process commands for IPS service.
 */
//--------------------------------------------------------------------------------------------------
void cm_rtc_ProcessRtcCommand
(
    const char * command,   ///< [IN] Command
    size_t numArgs          ///< [IN] Number of arguments
)
{
    // true if the command contains extra arguments.
    bool extraArguments = true;

    if (0 == strcmp(command, "help"))
    {
        if (numArgs < CM_MAX_ARGUMENTS_FOR_RTC_HELP)
        {
            extraArguments = false;
            cm_rtc_PrintRtcHelp();
        }
    }
    else if (0 == strcmp(command, "read"))
    {
        if (numArgs < CM_MAX_ARGUMENTS_FOR_RTC_READ)
        {
            extraArguments = false;
            if (LE_OK != ReadAndPrintRtc())
            {
                printf("Read failed.\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    else if (0 == strcmp(command, "set"))
    {
        if (numArgs < CM_MAX_ARGUMENTS_FOR_RTC_SET)
        {
            extraArguments = false;
            if (LE_OK != SetRtc())
            {
                printf("Set RTC failure.\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    else
    {
        printf("Unexpected string has been given to cm.\n");
        exit(EXIT_FAILURE);
    }

    if (true == extraArguments)
    {
        printf("Invalid command for RTC service.\n");
        exit(EXIT_FAILURE);
    }
    else
    {
        exit(EXIT_SUCCESS);
    }
}
