//--------------------------------------------------------------------------------------------------
/**
 * FW Update command line tool
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "le_print.h"

#include <sys/utsname.h>

//--------------------------------------------------------------------------------------------------
/**
 * Maximum length of the version string.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_VERS_BYTES  257


//--------------------------------------------------------------------------------------------------
/**
 * Definition for connect service function pointer
 */
//--------------------------------------------------------------------------------------------------
typedef void (*ConnectServiceFunc_t)
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Help Message
 */
//--------------------------------------------------------------------------------------------------
static char* HelpMessage = "\
NAME:\n\
    fwupdate - Download or Query modem firmware\n\
\n\
SYNOPSIS:\n\
    fwupdate help\n\
    fwupdate download FILE\n\
    fwupdate query\n\
\n\
DESCRIPTION:\n\
    fwupdate help\n\
      - Print this help message and exit\n\
\n\
    fwupdate download FILE\n\
      - Download the given CWE file; if '-' is given as the FILE, then use stdin.\n\
        After a successful download, the modem will reset.\n\
\n\
    fwupdate query\n\
      - Query the current firmware version. This includes the modem firmware version, the\n\
        bootloader version, and the linux kernel version.\n\
        This can be used after a download and modem reset, to confirm the firmware version.\n\
";


//--------------------------------------------------------------------------------------------------
/**
 * Print the help message to stdout
 */
//--------------------------------------------------------------------------------------------------
static void PrintHelp
(
    void
)
{
    puts(HelpMessage);
}


//--------------------------------------------------------------------------------------------------
/**
 * Thread used to recover from problems connecting to a service, probably because the service is
 * down.  It will timeout after 20 seconds, print an error message, and then exit.
 *
 * Once successfully connected to the service, this thread should be stopped.
 */
//--------------------------------------------------------------------------------------------------
static void* TimeoutThread
(
    void* contextPtr        ///< This should be string containing name of service
)
{
    char* serviceNamePtr = contextPtr;

    // This thread doesn't have to do anything else, at least for now, so just sleep.
    sleep(20);

    printf("Error: can't connect to service; is %s running?\n", serviceNamePtr);
    exit(EXIT_FAILURE);

    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Try calling the given function to connect to a service.  If can't connect to the service within
 * 20 seconds, then the program will exit.
 */
//--------------------------------------------------------------------------------------------------
static void TryConnect
(
    ConnectServiceFunc_t connectFuncPtr,    ///< Function to call to connect to service
    char* serviceNamePtr                    ///< String containing name of the service
)
{
    // Print out message before trying to connect to service to give user some kind of feedback
    printf("Connecting to service ...\n");
    fflush(stdout);

    // Use a separate thread for recovery.  It will be stopped once connected to the service.
    // Make the thread joinable, so we can be sure the thread is stopped before continuing.
    le_thread_Ref_t threadRef = le_thread_Create("timout thread", TimeoutThread, serviceNamePtr);
    le_thread_SetJoinable(threadRef);
    le_thread_Start(threadRef);

    // Try connecting to the service
    connectFuncPtr();

    // Connected to the service, so stop the timeout thread
    le_thread_Cancel(threadRef);
    le_thread_Join(threadRef, NULL);
}


//--------------------------------------------------------------------------------------------------
/**
 * Process the download firmware command
 *
 * @return
 *      - LE_OK if the download was successful
 *      - LE_FAULT if there was an issue during the download process
 */
//--------------------------------------------------------------------------------------------------
static le_result_t DownloadFirmware
(
    const char* fileName    ///< Name of file containing firmware image
)
{
    int fd;

    if ( strcmp(fileName, "-") == 0 )
    {
        // Use stdin
        fd = STDIN_FILENO;
    }
    else
    {
        // Open the file
        fd = open( fileName, O_RDONLY);
        LE_PRINT_VALUE("%d", fd);

        if ( fd == -1 )
        {
            // Inform the user of the error; it's also useful to log this info
            printf("Can't open file '%s' : %m\n", fileName);
            return LE_FAULT;
        }
    }

    TryConnect(le_fwupdate_ConnectService, "fwupdateService");

    // Connected to service so continue
    printf("Download started ...\n");
    fflush(stdout);

    LE_PRINT_VALUE("%d", fd);
    if ( le_fwupdate_Download(fd) == LE_OK )
    {
        printf("Download successful; please wait for modem to reset\n");
    }
    else
    {
        printf("Error in download\n");
        return LE_FAULT;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Process the query command, and print out the firmware, bootloader and linux versions.
 *
 * @return
 *      - LE_OK if it was possible to show all versions
 *      - LE_FAULT if that was not the case
 */
//--------------------------------------------------------------------------------------------------
static le_result_t QueryVersion
(
    void
)
{
    le_result_t result = LE_OK;

    TryConnect(le_fwupdate_ConnectService, "fwupdateService");

    // Connected to service so continue
    char version[MAX_VERS_BYTES];
    struct utsname linuxInfo;

    if ( le_fwupdate_GetFirmwareVersion(version, sizeof(version)) == LE_OK )
    {
        printf("Firmware Version: %s\n", version);
    }
    else
    {
        result = LE_FAULT;
    }

    if ( le_fwupdate_GetBootloaderVersion(version, sizeof(version)) == LE_OK )
    {
        printf("Bootloader Version: %s\n", version);
    }
    else
    {
        result = LE_FAULT;
    }

    if ( uname(&linuxInfo) == 0 )
    {
        printf("Linux Version: %s %s\n", linuxInfo.release, linuxInfo.version);
    }
    else
    {
        result = LE_FAULT;
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Program init
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // Process the command
    if (le_arg_NumArgs() >= 1)
    {
        const char* command = le_arg_GetArg(0);

        if ( strcmp(command, "help") == 0 )
        {
            PrintHelp();
            exit(EXIT_SUCCESS);
        }

        else if ( strcmp(command, "download") == 0 )
        {
            // Get the filename of the firmware image; could be '-' if stdin
            if (le_arg_NumArgs() > 1)
            {
                if (DownloadFirmware(le_arg_GetArg(1)) == LE_OK)
                {
                    exit(EXIT_SUCCESS);
                }

                exit(EXIT_FAILURE);
            }
            else
            {
                printf("Missing FILE\n\n");
            }
        }

        else if ( strcmp(command, "query") == 0 )
        {
            if (QueryVersion() == LE_OK)
            {
                exit(EXIT_SUCCESS);
            }

            exit(EXIT_FAILURE);
        }

        else
        {
            printf("Invalid command '%s'\n\n", command);
        }
    }

    // Only get here if an error occurred.
    PrintHelp();
    exit(EXIT_FAILURE);
}

