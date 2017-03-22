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
#define READ_BUFFER_BYTES  255

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
        char buff[READ_BUFFER_BYTES] = {0};
        struct tm tm;

        time_t time = (epochTime/1000) + CM_DELTA_POSIX_TIME_EPOCH_GPS_TIME_EPOCH_IN_SEC;
        LE_DEBUG(" read posixEpochtime %ld seconds\n", (time_t) time);

        snprintf(buff, sizeof(buff), "%ld", (long) time);
        strptime(buff, "%s", &tm);
        memset(buff, 0, sizeof(buff));
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
    const char* datePtr     ///< Date the RTC should be set to.
)
{
    struct tm tm;
    time_t t;
    char* lastCarPtr;

    if (NULL == datePtr)
    {
        fprintf(stderr, "Date not provided.\n");
        return LE_BAD_PARAMETER;
    }

    lastCarPtr = strptime(datePtr, "%d %b %Y %H:%M:%S", &tm);

    if ((NULL == lastCarPtr) || (*lastCarPtr != '\0'))
    {
        fprintf(stderr, "Unable to parse provided date.\n");
        return LE_FAULT;
    }

    // Not set by strptime(); tells mktime() to determine whether
    // daylight saving time is in effect
    tm.tm_isdst = -1;

    t = mktime(&tm);

    if (t == -1)
    {
        fprintf(stderr, "mktime error\n");
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
    if (0 == strcmp(command, "help"))
    {
        cm_cmn_CheckNumberParams(CM_NUM_PARAMETERS_FOR_RTC_HELP,
                                 CM_NUM_PARAMETERS_FOR_RTC_HELP,
                                 numArgs,
                                 NULL);

        cm_rtc_PrintRtcHelp();
    }
    else if (0 == strcmp(command, "read"))
    {
        cm_cmn_CheckNumberParams(CM_NUM_PARAMETERS_FOR_RTC_READ,
                                 CM_NUM_PARAMETERS_FOR_RTC_READ,
                                 numArgs,
                                 NULL);

        if (LE_OK != ReadAndPrintRtc())
        {
            printf("Read failed.\n");
            exit(EXIT_FAILURE);
        }
    }
    else if (0 == strcmp(command, "set"))
    {
        cm_cmn_CheckNumberParams(CM_NUM_PARAMETERS_FOR_RTC_SET,
                                 CM_NUM_PARAMETERS_FOR_RTC_SET,
                                 numArgs,
                                 "Date is missing. e.g. cm rtc set <date>");

        const char* datePtr = le_arg_GetArg(2);

        if (LE_OK != SetRtc(datePtr))
        {
            printf("Set RTC failure.\n");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        printf("Invalid command '%s' for RTC service.\n", command);
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}

