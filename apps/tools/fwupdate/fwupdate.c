//--------------------------------------------------------------------------------------------------
/**
 * FW Update command line tool
 *
 * Copyright (C) Sierra Wireless Inc.
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
    fwupdate downloadOnly FILE\n\
    fwupdate query\n\
    fwupdate install\n\
    fwupdate checkStatus\n\
    fwupdate markGood\n\
    fwupdate download FILE\n\
\n\
DESCRIPTION:\n\
    fwupdate help\n\
      - Print this help message and exit\n\
\n\
    fwupdate query\n\
      - Query the current firmware version. This includes the modem firmware version, the\n\
        bootloader version, and the linux kernel version.\n\
        This can be used after a download and modem reset, to confirm the firmware version.\n\
\n\
    fwupdate downloadOnly FILE\n\
      - Download the given CWE file; if '-' is given as the FILE, then use stdin.\n\
        Waits for another command after a successful download.\n\
\n\
    fwupdate checkStatus\n\
      - Check the status of the downloaded package (DualSys platform only)\n\
\n\
    fwupdate install\n\
      - Install the downloaded firmware.\n\
        Single System: Trigger reset to initiate install.\n\
        Dual System: Swap and reset to run the downloaded firmware or go back to the old system\n\
        if the running system is not marked good.\n\
\n\
\n\
    fwupdate markGood\n\
      - Mark good the current system (DualSys platform only)\n\
\n\
    fwupdate download FILE\n\
      - do download, install and markGood in one time\n\
        After a successful download, the modem will reset\n\
";

//--------------------------------------------------------------------------------------------------
/**
 * State for the connection to the fwupdate service.
 */
//--------------------------------------------------------------------------------------------------
static bool FwupdateConnectionState = false;


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
    char* serviceNamePtr,                   ///< String containing name of the service
    bool* connectionStatePtr                ///< Connection state, as to avoid duplicate calls
)
{
    if(*connectionStatePtr)
    {
        // Connection was already established previously, no need to go any further.
        return;
    }

    // Print out message before trying to connect to service to give user some kind of feedback
    printf("Connecting to service ...\n");
    fflush(stdout);

    // Use a separate thread for recovery.  It will be stopped once connected to the service.
    // Make the thread joinable, so we can be sure the thread is stopped before continuing.
    le_thread_Ref_t threadRef = le_thread_Create("timeout thread", TimeoutThread, serviceNamePtr);
    le_thread_SetJoinable(threadRef);
    le_thread_Start(threadRef);

    // Try connecting to the service
    connectFuncPtr();

    // Connected to the service, so stop the timeout thread
    le_thread_Cancel(threadRef);
    le_thread_Join(threadRef, NULL);

    *connectionStatePtr = true;
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
    const char* fileNamePtr    ///< Name of file containing firmware image
)
{
    int fd;

    if ( strcmp(fileNamePtr, "-") == 0 )
    {
        // Use stdin
        fd = STDIN_FILENO;
    }
    else
    {
        // Open the file
        fd = open( fileNamePtr, O_RDONLY);
        LE_PRINT_VALUE("%d", fd);

        if ( fd == -1 )
        {
            // Inform the user of the error; it's also useful to log this info
            fprintf(stderr, "Can't open file '%s' : %m\n", fileNamePtr);
            return LE_FAULT;
        }
    }

    TryConnect(le_fwupdate_ConnectService, "fwupdateService", &FwupdateConnectionState);

    // Connected to service so continue
    printf("Download started ...\n");
    fflush(stdout);

    // force a fresh download on dualsys platform
    le_fwupdate_InitDownload();

    LE_PRINT_VALUE("%d", fd);
    if ( le_fwupdate_Download(fd) == LE_OK )
    {
        printf("Download successful\n");
    }
    else
    {
        fprintf(stderr, "Error in download\n");
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

    TryConnect(le_fwupdate_ConnectService, "fwupdateService", &FwupdateConnectionState);

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
 * Process the install firmware command
 *
 * @return
 *      - LE_OK if the install was successful
 *      - LE_FAULT if there was an issue during the install process
 */
//--------------------------------------------------------------------------------------------------
static le_result_t InstallFirmware
(
    void
)
{
    TryConnect(le_fwupdate_ConnectService, "fwupdateService", &FwupdateConnectionState);

    printf("Install the firmware, the system will reboot ...\n");
    return le_fwupdate_Install();
}

//--------------------------------------------------------------------------------------------------
/**
 * Check the status of the downloaded package
 *
 * @return
 *      - LE_OK if status check was successful
 *      - LE_FAULT if status check failed
 */
//--------------------------------------------------------------------------------------------------
static le_result_t CheckStatus
(
    void
)
{
    le_fwupdate_UpdateStatus_t status;
    char statusStr[LE_FWUPDATE_STATUS_LABEL_LENGTH_MAX];

    TryConnect(le_fwupdate_ConnectService, "fwupdateService", &FwupdateConnectionState);

    if (le_fwupdate_GetUpdateStatus(&status, statusStr, sizeof(statusStr)) != LE_OK)
    {
        fprintf(stderr, "Error reading update status\n");
        return LE_FAULT;
    }

    if (status != LE_FWUPDATE_UPDATE_STATUS_OK)
    {
        fprintf(stderr, "Bad status (%s), install not possible.\n", statusStr);
        return LE_FAULT;
    }

    printf("Update status: OK.\n");
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Mark good the current firmware
 *
 * @return
 *      - LE_OK            On success
 *      - LE_UNAVAILABLE   The flash access is not granted for SW update
 *      - LE_FAULT         On failure
 */
//--------------------------------------------------------------------------------------------------
static le_result_t MarkGoodFirmware
(
    void
)
{
    bool isSystemGood;
    le_result_t result;

    TryConnect(le_fwupdate_ConnectService, "fwupdateService", &FwupdateConnectionState);

    result = le_fwupdate_IsSystemMarkedGood(&isSystemGood);
    if ((result == LE_OK) && isSystemGood)
    {
        return LE_OK;
    }

    return le_fwupdate_MarkGood();
}

//--------------------------------------------------------------------------------------------------
/**
 * Download, install and mark good a firmware
 *
 * @return
 *      - LE_OK if the download was successful
 *      - LE_FAULT if there was an issue during the download process
 */
//--------------------------------------------------------------------------------------------------
static le_result_t FullInstallFirmware
(
    const char* fileNamePtr    ///< Name of file containing firmware image
)
{
    le_result_t result;

    TryConnect(le_fwupdate_ConnectService, "fwupdateService", &FwupdateConnectionState);

    result = DownloadFirmware(fileNamePtr);
    if (result != LE_OK)
    {
        return result;
    }

    printf("Installing & Reboot ...\n");
    result = le_fwupdate_InstallAndMarkGood();
    if (result != LE_OK)
    {
        fprintf(stderr, "Error during installation: %s\n", LE_RESULT_TXT(result));
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
        if (NULL == command)
        {
            LE_ERROR("command is NULL");
            exit(EXIT_FAILURE);
        }

        if ( strcmp(command, "help") == 0 )
        {
            PrintHelp();
            exit(EXIT_SUCCESS);
        }

        else if ( 0 == strcmp(command, "downloadOnly") )
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
                fprintf(stderr, "Missing FILE\n\n");
            }
        }

        else if ( 0 == strcmp(command, "query") )
        {
            if (QueryVersion() == LE_OK)
            {
                exit(EXIT_SUCCESS);
            }

            exit(EXIT_FAILURE);
        }

        else if ( 0 == strcmp(command, "checkStatus") )
        {
            if ( CheckStatus() == LE_OK )
            {
                exit(EXIT_SUCCESS);
            }

            exit(EXIT_FAILURE);
        }

        else if ( 0 == strcmp(command, "install") )
        {
            if ( InstallFirmware() == LE_OK )
            {
                exit(EXIT_SUCCESS);
            }

            exit(EXIT_FAILURE);
        }

        else if ( 0 == strcmp(command, "markGood") )
        {
            if ( MarkGoodFirmware() == LE_OK )
            {
                exit(EXIT_SUCCESS);
            }

            exit(EXIT_FAILURE);
        }

        else if ( 0 == strcmp(command, "download") )
        {
            // Get the filename of the firmware image; could be '-' if stdin
            if (le_arg_NumArgs() > 1)
            {
                if ( FullInstallFirmware(le_arg_GetArg(1)) == LE_OK )
                {
                    exit(EXIT_SUCCESS);
                }

                exit(EXIT_FAILURE);
            }
            else
            {
                fprintf(stderr, "Missing FILE\n\n");
            }
        }

        else
        {
            fprintf(stderr, "Invalid command '%s'\n\n", command);
        }
    }

    // Only get here if an error occurred.
    PrintHelp();
    exit(EXIT_FAILURE);
}

