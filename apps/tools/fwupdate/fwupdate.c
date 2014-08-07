//--------------------------------------------------------------------------------------------------
/**
 * FW Update command line tool
 *
 * Copyright (C) Sierra Wireless, Inc. 2014. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "le_print.h"

#include <sys/utsname.h>


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
 * Process the download firmware command
 */
//--------------------------------------------------------------------------------------------------
static void DownloadFirmware
(
    char* fileName    ///< Name of file containing firmware image
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

        LE_FATAL_IF(fd == -1, "Can't open file %s : %m", fileName);
    }

    LE_PRINT_VALUE("%d", fd);
    if ( le_fwupdate_Download(fd) == LE_OK )
    {
        printf("Download successful; please wait for modem to reset\n");
    }
    else
    {
        printf("Error in download\n");
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Process the query command, and print out the firmware, bootloader and linux versions.
 */
//--------------------------------------------------------------------------------------------------
static void QueryVersion
(
    void
)
{
    le_result_t result;
    char version[LE_FWUPDATE_MAX_VERS_LEN];
    struct utsname linuxInfo;

    result = le_fwupdate_GetFirmwareVersion(version, sizeof(version));
    if ( result == LE_OK )
    {
        printf("Firmware Version: %s\n", version);
    }

    result = le_fwupdate_GetBootloaderVersion(version, sizeof(version));
    if ( result == LE_OK )
    {
        printf("Bootloader Version: %s\n", version);
    }

    if ( uname(&linuxInfo) == 0 )
    {
        printf("Linux Version: %s %s\n", linuxInfo.release, linuxInfo.version);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Program init
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    char buffer[100] = "";

    // Process the command
    if (le_arg_NumArgs() >= 1)
    {
        le_arg_GetArg(0, buffer, sizeof(buffer));

        if ( strcmp(buffer, "help") == 0 )
        {
            PrintHelp();
            exit(EXIT_SUCCESS);
        }

        else if ( strcmp(buffer, "download") == 0 )
        {
            // Get the filename of the firmware image; could be '-' if stdin
            if (le_arg_NumArgs() > 1)
            {
                le_arg_GetArg(1, buffer, sizeof(buffer));

                DownloadFirmware(buffer);
                exit(EXIT_SUCCESS);
            }
            else
            {
                printf("Missing FILE\n\n");
            }
        }

        else if ( strcmp(buffer, "query") == 0 )
        {
            QueryVersion();
            exit(EXIT_SUCCESS);
        }

        else
        {
            printf("Invalid command '%s'\n\n", buffer);
        }
    }

    // Only get here if an error occurred.
    PrintHelp();
    exit(EXIT_FAILURE);
}

