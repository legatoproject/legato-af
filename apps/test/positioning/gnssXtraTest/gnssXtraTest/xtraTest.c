/** @file xtraTest.c
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"
#include <arpa/inet.h>

#define WEEK_NUMBER_OFFSET_IN_FILE   21
#define SECONDS_IN_A_DAY             86400 // 24*60*60

#define TEMP_DIR        "/tmp"
// Note: XTRA1 is not supported by LE55. XTRA2 must be used.
#define DOWNLOAD_CMD_XTRA1 "wget -O /tmp/xtra.bin http://xtra1.gpsonextra.net/xtra.bin"
#define DOWNLOAD_CMD_XTRA2 "wget -O /tmp/xtra2.bin http://xtra1.gpsonextra.net/xtra2.bin"

#define MAX_DOWNLOAD_RETRY   5

#define XTRA1_FILE_PATH      TEMP_DIR"/xtra.bin"
#define XTRA2_FILE_PATH      TEMP_DIR"/xtra2.bin"
#define XTRA_NO_FILE_PATH    TEMP_DIR"/dummy.bin"


//--------------------------------------------------------------------------------------------------
//                                       Test Function
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Test: Read the week number from file.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ReadWeekNumber
(
    const char *filePtr,
    uint16_t   *weekNumPtr
)
{
    uint16_t weekNumRead=0;

    int fd = open(filePtr,O_RDONLY);
    LE_ASSERT(fd!=-1)

    LE_ASSERT(lseek(fd,WEEK_NUMBER_OFFSET_IN_FILE,SEEK_SET)==WEEK_NUMBER_OFFSET_IN_FILE);

    LE_ASSERT(read(fd,&weekNumRead,sizeof(weekNumRead))==sizeof(weekNumRead));

    LE_ASSERT(close(fd)==0);

    *weekNumPtr = htons(weekNumRead);
}


//--------------------------------------------------------------------------------------------------
/**
 * Test: download XTRA1.bin and XTRA2.bin files from the network.
 *
 */
//--------------------------------------------------------------------------------------------------
static void DownloadXtraFile
(
    void
)
{
    int retryDwlCounter=0;
    bool dwlComplete = true;

    /* Download XTRA1 file */
    do
    {
        if ( system(DOWNLOAD_CMD_XTRA1) != 0 )
        {
            LE_INFO("system '%s' failed",DOWNLOAD_CMD_XTRA1);
            retryDwlCounter++;
        }
        else
        {
            break;
        }
    } while (retryDwlCounter < MAX_DOWNLOAD_RETRY);

    if(retryDwlCounter == MAX_DOWNLOAD_RETRY)
    {
        LE_ERROR("Download XTRA1 Failed %d times",retryDwlCounter);
        dwlComplete = false;
    }
    else
    {
        LE_INFO("Download XTRA1 done");
    }

    /* Download XTRA2 file */
    retryDwlCounter = 0;
    do
    {
        if ( system(DOWNLOAD_CMD_XTRA2) != 0 )
        {
            LE_INFO("system '%s' failed",DOWNLOAD_CMD_XTRA2);
            retryDwlCounter++;
        }
        else
        {
            break;
        }
    } while (retryDwlCounter < MAX_DOWNLOAD_RETRY);

    if(retryDwlCounter == MAX_DOWNLOAD_RETRY)
    {
        LE_ERROR("Download XTRA2 Failed %d times",retryDwlCounter);
        dwlComplete = false;
    }
    else
    {
        LE_INFO("Download XTRA2 done");
    }

    // Check XTRA files download status
    LE_ASSERT(dwlComplete == true);

    return;
}


//--------------------------------------------------------------------------------------------------
/**
 * Test: removing Xtra file
 *
 */
//--------------------------------------------------------------------------------------------------
static void RemoveXtraFile
(
    const char *filePtr
)
{
    struct stat   buffer;
    if  (stat (filePtr, &buffer) == 0)
    {
        LE_INFO("remove XTRA file");
        LE_ASSERT(unlink(filePtr)==0);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Test: unexisting XTRA file and invalid XTRA file injection
 *
 */
//--------------------------------------------------------------------------------------------------
static void TestGetWrongExtendedEphemeris
(
    void
)
{
    int32_t fd;

    if ((fd=open(XTRA_NO_FILE_PATH, O_RDONLY)) == -1)
    {
        LE_INFO("Test using an unexisting XTRA file");
        // Test with an unexisting XTRA file
        LE_ASSERT(le_gnss_LoadExtendedEphemerisFile(fd) == LE_FAULT);
    }
    else
    {
        // Close the fd before opening another resource.
        close(fd);
    }

    // Note: XTRA1 is not supported by LE55. XTRA2 must be used.
    // Test "invalid" XTRA file injection
    fd = open(XTRA1_FILE_PATH, O_RDONLY);
    LE_ASSERT(fd != -1);

    LE_INFO("Open file %s with fd.%d",  XTRA1_FILE_PATH, fd);

    LE_INFO("Test using an inconsistent XTRA file");
    if (le_gnss_LoadExtendedEphemerisFile(fd) != LE_FORMAT_ERROR)
    {
        LE_FATAL("Assert Failed: le_gnss_LoadExtendedEphemerisFile(fd) == LE_FORMAT_ERROR");
    }
    else
    {
        LE_DEBUG("Received LE_FORMAT_ERROR");
    }

    // Closing fd is unnecessary since the messaging infrastructure underneath
    // le_gnss_LoadExtendedEphemerisFile API that use it would close it.

}


//--------------------------------------------------------------------------------------------------
/**
 * Test: XTRA2 file injection
 *
 */
//--------------------------------------------------------------------------------------------------
static void TestGetExtendedEphemerisValidity
(
    void
)
{
    uint16_t weekNum=0;
    uint64_t start, stop, validity;
    int32_t fd;

    fd = open(XTRA2_FILE_PATH, O_RDONLY);
    LE_ASSERT(fd != -1);

    LE_INFO("Open file %s with fd.%d",  XTRA2_FILE_PATH, fd);

    LE_ASSERT(le_gnss_LoadExtendedEphemerisFile(fd)==LE_OK);

    LE_ASSERT(le_gnss_GetExtendedEphemerisValidity(&start,&stop) == LE_OK);

    ReadWeekNumber(XTRA2_FILE_PATH,&weekNum);
    LE_INFO("XTRA2 file weekNum %d",weekNum);

    /* Check validity duration of injected XTRA file (7 days) */
    validity = stop-start;
    LE_ASSERT(validity/SECONDS_IN_A_DAY == 7);
    close(fd);
}


//--------------------------------------------------------------------------------------------------
/**
 * App init.
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("======== Begin Positioning Xtra QMI implementation Test  ========");

    // Note that Reboot/Reset must be issued after XTRA Enable/Disable
    LE_ASSERT(le_gnss_DisableExtendedEphemerisFile()==LE_OK); // Will be never affected
                                                              // as XTRA Enable is done after:
    LE_ASSERT(le_gnss_EnableExtendedEphemerisFile()==LE_OK); // Will be affected after a Reset

    LE_ASSERT(le_gnss_Start() == LE_OK);

    RemoveXtraFile(XTRA1_FILE_PATH);
    RemoveXtraFile(XTRA2_FILE_PATH);

    DownloadXtraFile();

    // Test an unexisting XTRA file
    // and an invalid" XTRA file injection
    TestGetWrongExtendedEphemeris();

    TestGetExtendedEphemerisValidity();

    LE_ASSERT(le_gnss_Stop() == LE_OK);

    LE_INFO("======== Completed Positioning Xtra QMI implementation Test ========");

    exit(EXIT_SUCCESS);
}

