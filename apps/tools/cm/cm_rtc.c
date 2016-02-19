//-------------------------------------------------------------------------------------------------
/**
 * @file cm_rtc.c
 *
 * Handle RTC related functionality.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
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

        time_t time = epochTime/1000;

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


    if (strptime(datePtr, "%d %b %Y %H:%M:%S", &tm) == NULL)
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

    uint64_t timeMs = (uint64_t) t * 1000;

    return le_rtc_SetUserTime(timeMs);
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
    if (strcmp(command, "help") == 0)
    {
        cm_rtc_PrintRtcHelp();
    }
    else if (strcmp(command, "read") == 0)
    {
        if (LE_OK != ReadAndPrintRtc())
        {
            printf("Read failed.\n");
            exit(EXIT_FAILURE);
        }
    }
    else if (strcmp(command, "set") == 0)
    {
        if (LE_OK != SetRtc())
        {
            printf("Set RTC failure.\n");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        printf("Invalid command for RTC service.\n");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}
