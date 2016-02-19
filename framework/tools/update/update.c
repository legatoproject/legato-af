/** @file update.c
 *
 * Client of update service provider(i.e. UpdateDaemon) for installing/removing app, and installing
 * firmware. This client receives update package via STDIN and sequentially calls update APIs for
 * successful update. This client follows the steps mentioned in le_update.api documentation
 * while performing update. A callback is implemented here to receive progress of ongoing update
 * process.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "limit.h"
#include "interfaces.h"


//--------------------------------------------------------------------------------------------------
/**
 * true = -r or --remove was specified on the command-line.
 */
//--------------------------------------------------------------------------------------------------
static bool DoRemove = false;


//--------------------------------------------------------------------------------------------------
/**
 * Positional command-line argument.
 */
//--------------------------------------------------------------------------------------------------
static const char* ArgPtr = "-";


//--------------------------------------------------------------------------------------------------
/**
 * Prints help to stdout.
 */
//--------------------------------------------------------------------------------------------------
static void PrintHelp
(
    void
)
{
    puts(
        "NAME:\n"
        "    update - install/remove utility for legato.\n"
        "\n"
        "SYNOPSIS:\n"
        "    update --help\n"
        "    update [FILE_NAME]\n"
        "    update --remove APP_NAME\n"
        "\n"
        "DESCRIPTION:\n"
        "    update --help\n"
        "        Display this help and exit.\n"
        "\n"
        "    update [FILE_NAME]\n"
        "        Command takes an update file, decodes the manifest, and takes appropriate action.\n"
        "        If no file name or the file name '-' is given, input is taken from the standard\n"
        "        input stream (stdin).\n"
        "\n"
        "     update --remove APP_NAME\n"
        "     update -r APP_NAME\n"
        "        Removes an app from the device.\n"
    );

    exit(EXIT_SUCCESS);
}


//--------------------------------------------------------------------------------------------------
/**
 * Function that gets called when --remove or -r appear on the command-line.
 */
//--------------------------------------------------------------------------------------------------
static void RemoveSelected
(
    void
)
{
    if (DoRemove)
    {
        fprintf(stderr, "--remove or -r specified more than once.\n");
        exit(EXIT_FAILURE);
    }

    DoRemove = true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the file descriptor for the input stream.  Input file either might be given via STDIN or
 * parameter.  If no parameter is given, this function waits for data through STDIN.
 *
 * @return
 *      - File descriptor pointing to update file.
 */
//--------------------------------------------------------------------------------------------------
static int GetUpdateFile
(
    const char* filePathPtr ///< The file path, or "-" for stdin.
)
{
    int fileDescriptor;

    if (strcmp(filePathPtr, "-") == 0)
    {
        fileDescriptor = 0;
    }
    else
    {
        fileDescriptor = open(filePathPtr, O_RDONLY);

        if (fileDescriptor == -1)
        {
            fprintf(stderr,
                    "Can't open file '%s': errno %d (%m)\n",
                    filePathPtr,
                    errno);
            exit(EXIT_FAILURE);
        }
    }

    return fileDescriptor;
}


//--------------------------------------------------------------------------------------------------
/**
 * Processes a positional argument from the command line.
 **/
//--------------------------------------------------------------------------------------------------
static void HandlePositionalArg
(
    const char* argPtr
)
{
    ArgPtr = argPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function is used for printing progress bar.
 */
//--------------------------------------------------------------------------------------------------
static void PrintProgressBar
(
    uint percentDone,           ///<[IN] Percent done (for current state) of update task underway.
    const char* progMsg         ///<[IN] Message to print with progress bar
)
{
    static const int ProgressStringBytes = 256;
    static const int ProgressBarLen = 50;
    static uint lastPercentDone = 0;
    static const char* lastProgMsg = NULL;

    if ((percentDone < lastPercentDone) || ((lastProgMsg != NULL) && (lastProgMsg != progMsg)))
    {
        fprintf(stdout, "\n");
    }
    lastPercentDone = percentDone;
    lastProgMsg = progMsg;

    int progressCharCnt = percentDone/2;
    char progressStr[ProgressStringBytes];
    char tempStr[ProgressBarLen+1];

    if (percentDone > 100)
    {
        LE_ERROR("Unexpected percentDone value: %d!!", percentDone);
        return;
    }

    // Change last character to zero.
    tempStr[ProgressBarLen] = 0;
    // Reset all characters to zero.
    memset(progressStr, 0, ProgressStringBytes);
    // Forming progress string.
    memset(tempStr, ' ', ProgressBarLen);
    memset(tempStr, '+', progressCharCnt);

    snprintf(progressStr, sizeof(progressStr), "%s: %3d%% %s",
             progMsg,
             percentDone,
             tempStr);

    // Print the progress string. CR(i.e. \r) is used as same line will be overwritten multiple times.
    fprintf(stdout, "%s\r", progressStr);

    fflush(stdout);
}


//--------------------------------------------------------------------------------------------------
/**
 * Print message according to the error code.
 */
//--------------------------------------------------------------------------------------------------
static void PrintErrorMsg
(
    void
)
{
    switch (le_update_GetErrorCode())
    {
        case LE_UPDATE_ERR_NONE:
            fprintf(stderr, "\n***Error: Unexpected error code: NONE\n");
            return;

        case LE_UPDATE_ERR_BAD_PACKAGE:
            fprintf(stderr, "\n***Error: Received bad update package. See log for details.\n");
            return;

        case LE_UPDATE_ERR_SECURITY_FAILURE:
            fprintf(stderr, "\n***Error: Security check failure. See log for details.\n");
            return;

        case LE_UPDATE_ERR_INTERNAL_ERROR:
            fprintf(stderr, "\n***Error: Internal error during update. See log for details.\n");
            return;
    }

    fprintf(stderr, "\n***Error: Unexpected error code: %d.\n", le_update_GetErrorCode());
}


//--------------------------------------------------------------------------------------------------
/**
 * Callback function which is registered in update service provider(UpdateDaemon) to get status
 * information for ongoing update task.
 */
//--------------------------------------------------------------------------------------------------
static void UpdateProgressHandler
(
    le_update_State_t updateState,   ///< Current State of ongoing update task in Update State
                                     ///< machine.

    uint percentDone,                ///< Percent done for current state. As example: at state
                                     ///< LE_UPDATE_STATE_UNPACKING, percentDone=80 means,
                                     ///< 80% of the update file data is already transferred to
                                     ///< unpack process.

    void* contextPtr                 ///< Context pointer.
)
{
    switch(updateState)
    {
        case LE_UPDATE_STATE_UNPACKING:
            // Print progress bar if there is any noticeable progress.
            PrintProgressBar(percentDone, "Unpacking package");
            break;

        case LE_UPDATE_STATE_APPLYING:
            // Print progress bar if there is any noticeable progress.
            PrintProgressBar(percentDone, "Applying update");
            break;

        case LE_UPDATE_STATE_SUCCESS:
            //Successful completion.
            printf("\nSUCCESS\n");
            exit(EXIT_SUCCESS);

        case LE_UPDATE_STATE_FAILED:
            // Failure in update, exit with failure code.
            PrintErrorMsg();
            printf("\nFAILED\n");
            exit(EXIT_FAILURE);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Process an update pack.
 */
//--------------------------------------------------------------------------------------------------
static void Update
(
    const char* filePathPtr ///< The file path, or "-" for stdin.
)
{
    int fd = GetUpdateFile(filePathPtr);

    le_update_ConnectService();

    // Register for progress notifications.
    le_update_AddProgressHandler(UpdateProgressHandler, NULL);

    // Start update process(asynchronous). Completion will be notified via callback function.
    le_result_t result = le_update_Start(fd);
    switch (result)
    {
        case LE_BUSY:
            fprintf(stderr, "**ERROR: Another update is currently in progress.\n");
            exit(EXIT_FAILURE);

        case LE_UNAVAILABLE:
            fprintf(stderr, "**ERROR: The system is still in its probation period"
                            " (not marked \"good\" yet).\n");
            exit(EXIT_FAILURE);

        case LE_OK:
            break;

        default:
            fprintf(stderr, "**ERROR: Unexpected result code from update server.\n");
            exit(EXIT_FAILURE);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Remove an application.
 */
//--------------------------------------------------------------------------------------------------
static void RemoveApp
(
    const char* appNamePtr ///< The app name.
)
{
    le_appRemove_ConnectService();

    le_result_t result = le_appRemove_Remove(appNamePtr);

    if (result == LE_OK)
    {
        exit(EXIT_SUCCESS);
    }
    else
    {
        fprintf(stderr, "Failed to remove app '%s' (%s)\n", appNamePtr, LE_RESULT_TXT(result));

        exit(EXIT_FAILURE);
    }
}



COMPONENT_INIT
{
    // update --help
    le_arg_SetFlagCallback(PrintHelp, NULL, "help");

    // update --remove APP_NAME
    le_arg_SetFlagCallback(RemoveSelected, "r", "remove");

    // update [FILE_NAME]
    le_arg_AddPositionalCallback(HandlePositionalArg);
    le_arg_AllowLessPositionalArgsThanCallbacks();

    le_arg_Scan();

    // If --remove (or -r) was specified, then remove the app.
    if (DoRemove)
    {
        if (ArgPtr == NULL)
        {
            fprintf(stderr, "No app name specified.\n");
            exit(EXIT_FAILURE);
        }

        RemoveApp(ArgPtr);
    }
    // If --remove (or -r) was NOT specified, then process an update pack.
    else
    {
        // If no file path was provided on the command line, default to "-" for standard in.
        if (ArgPtr == NULL)
        {
            ArgPtr = "-";
        }

        Update(ArgPtr);
    }
}
