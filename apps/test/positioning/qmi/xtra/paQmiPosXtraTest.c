/** @file paQmiPosXtraTest.c
 *
 * Copyright (C) Sierra Wireless, Inc. 2014. Use of this work is subject to license.
 */

#include <arpa/inet.h>

#include "legato.h"
#include "pa_gnss.h"


#define OFFSET          21

#define CONVERT_SECONDS_TO_WEEK     604800 // 7*24*60*60

#define TEMP_DIR        "/tmp"
#define DOWNLOAD_COMMAND    "wget --directory-prefix="TEMP_DIR" http://xtra1.gpsonextra.net/xtra.bin"
#define DOWNLOAD_RETRY      5

#define XTRA1_PATH      TEMP_DIR"/xtra.bin"
#define XTRA2_PATH      "./xtra.bin"

#define GSP_TIME_ZERO_SEC   315964800
#define GSP_TIME_ZERO_USEC  0

static void ReadWeekNumber
(
    const char *filePtr,
    uint16_t   *weekNumPtr
)
{
    uint16_t weekNumRead=0;

    int fd = open(filePtr,O_RDONLY);
    LE_ASSERT(fd!=-1)

    LE_ASSERT(lseek(fd,OFFSET,SEEK_SET)==OFFSET);

    LE_ASSERT(read(fd,&weekNumRead,sizeof(weekNumRead))==sizeof(weekNumRead));

    LE_ASSERT(close(fd)==0);

    *weekNumPtr = htons(weekNumRead);
}


static void DownloadXtraFile
(
    void
)
{
    int cpt = DOWNLOAD_RETRY;
    do
    {
        if ( system(DOWNLOAD_COMMAND) != 0 )
        {
            LE_INFO("system '%s' failed",DOWNLOAD_COMMAND);
            cpt--;
        }
        else
        {
            return;
        }
    } while (cpt);
    LE_INFO("Download Failed %d times",DOWNLOAD_RETRY);
    exit(EXIT_FAILURE);
}

static void RemoveXtraFile
(
    const char *filePtr
)
{
    struct stat   buffer;
    if  (stat (filePtr, &buffer) == 0)
    {
        LE_ASSERT(unlink(filePtr)==0);
    }
}


static void TestGetXtraValidity_1
(
    void
)
{
    uint16_t weekNum=0;
    le_clk_Time_t start,stop;
    char startTime[100] = {0};
    char StopTime[100] = {0};

    RemoveXtraFile(XTRA1_PATH);
    DownloadXtraFile();

    LE_ASSERT(pa_gnss_LoadXtra(XTRA1_PATH)==LE_OK);

    LE_ASSERT(pa_gnss_GetXtraValidityTimes(&start,&stop) == LE_OK);

    ReadWeekNumber(XTRA1_PATH,&weekNum);

    if ( le_clk_ConvertToUTCString(start,
                                  LE_CLK_STRING_FORMAT_DATE_TIME,
                                  startTime,
                                  sizeof(startTime),
                                  NULL) != LE_OK )
    {
        LE_INFO("Could not convert date time");
    }
    if ( le_clk_ConvertToUTCString(stop,
                                  LE_CLK_STRING_FORMAT_DATE_TIME,
                                  StopTime,
                                  sizeof(StopTime),
                                  NULL) != LE_OK )
    {
        LE_INFO("Could not convert date time");
    }

    LE_INFO("Start time  %s",startTime);
    LE_INFO("Stop time %s",StopTime);

    LE_ASSERT(start.sec/CONVERT_SECONDS_TO_WEEK==weekNum+GSP_TIME_ZERO_SEC/CONVERT_SECONDS_TO_WEEK);

    RemoveXtraFile(XTRA1_PATH);
}

static void TestGetXtraValidity_2
(
    void
)
{
    LE_ASSERT(pa_gnss_LoadXtra(XTRA2_PATH)==LE_FAULT);
}

COMPONENT_INIT
{
    LE_INFO("======== Begin Positioning Platform Adapter's QMI implementation Test  ========");



    LE_ASSERT(pa_gnss_Init() == LE_OK);

    // The modem seems to need time to initialize.
    sleep(1);

    LE_ASSERT(pa_gnss_SetAcquisitionRate(3) == LE_OK);


    LE_ASSERT(pa_gnss_Start() == LE_OK);

    LE_ASSERT(pa_gnss_EnableXtraSession() == LE_OK);

    TestGetXtraValidity_1();

    TestGetXtraValidity_1();

    TestGetXtraValidity_2();

    LE_ASSERT(pa_gnss_DisableXtraSession() == LE_OK);

    LE_ASSERT(pa_gnss_Stop() == LE_OK);

    exit(EXIT_SUCCESS);
}
