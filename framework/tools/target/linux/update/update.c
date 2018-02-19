/** @file update.c
 *
 * Client of update service provider(i.e. UpdateDaemon) for installing/removing app, and installing
 * firmware. This client receives update package via STDIN and sequentially calls update APIs for
 * successful update. This client follows the steps mentioned in le_update.api documentation
 * while performing update. A callback is implemented here to receive progress of ongoing update
 * process.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "limit.h"
#include "interfaces.h"


//--------------------------------------------------------------------------------------------------
/**
 * true = -f or --force was specified on the command-line.
 */
//--------------------------------------------------------------------------------------------------
static bool Force = false;

//--------------------------------------------------------------------------------------------------
/**
 * true = -r or --remove was specified on the command-line.
 */
//--------------------------------------------------------------------------------------------------
static bool DoRemove = false;


//--------------------------------------------------------------------------------------------------
/**
 * set to true in an option parsing callback if the option should cause the update or removal work
 * to be skipped.
 */
//--------------------------------------------------------------------------------------------------
static bool Done = false;


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
        "    update --mark-good\n"
        "    update --mark-bad\n"
        "    update --defer\n"
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
        "\n"
        "    update --mark-good\n"
        "    update -g\n"
        "        Ends the new system probation period and marks the current system good.\n"
        "        Ignored if the current system is already marked good."
        "\n"
        "    update --mark-bad\n"
        "    update -b\n"
        "        Marks the current system bad and reboots to rollback to the previous good system.\n"
        "        The command has no effect if the current system has already been marked good.\n"
        "        The restart waits for any deferral that is in effect.\n"
        "\n"
        "    update --defer\n"
        "    update -d\n"
        "        Command causes all updates to be deferred as long as the program is left running.\n"
        "        To release the deferral use Ctrl-C or kill to exit this command.\n"
        "        More than one deferral can be in effect at any time. All of them must be cleared\n"
        "        before an update can take place.\n"
    );

    exit(EXIT_SUCCESS);
}


//--------------------------------------------------------------------------------------------------
/**
 * Function that gets called when --force or -f appear on the command-line.
 */
//--------------------------------------------------------------------------------------------------
static void SetForce
(
    void
)
{
    Force = true;
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
 * Function that gets called when --mark-good or -g appear on the command-line.
 */
//--------------------------------------------------------------------------------------------------
static void MarkGood
(
    void
)
{
    le_updateCtrl_ConnectService();
    le_result_t result = le_updateCtrl_MarkGood(Force);
    switch (result)
    {
        case LE_OK:
            printf("System is now marked 'Good'.\n");
            exit(EXIT_SUCCESS);
            break;

        case LE_BUSY:
            fprintf(stderr, "**ERROR: One or more processes are holding probation locks - check logs.\n");
            fprintf(stderr, "Use -f (or --force) option to override.\n");
            exit(EXIT_FAILURE);
            break;

        case LE_DUPLICATE:
            fprintf(stderr, "**ERROR: The probation period has already ended. Nothing to do.\n");
            exit(EXIT_FAILURE);
            break;

        default:
            fprintf(stderr, "**ERROR: Unknown return code from le_updateCtrl_MarkGood().\n");
            exit(EXIT_FAILURE);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Function that gets called when --mark-bad or -b appear on the command-line.
 */
//--------------------------------------------------------------------------------------------------
static void MarkBad
(
    void
)
{
    le_updateCtrl_ConnectService();
    le_updateCtrl_FailProbation();
    exit(EXIT_SUCCESS);
}


//--------------------------------------------------------------------------------------------------
/**
 * Function that gets called when we get SIGINT (generally user hits Ctrl-C) so we can release
 * our Defer before we die.
 */
//--------------------------------------------------------------------------------------------------
static void EndDeferral
(
    int sigNum
)
{
    le_updateCtrl_Allow();
    exit(EXIT_SUCCESS);
}


//--------------------------------------------------------------------------------------------------
/**
 * Function that gets called when --defer or -d appear on the command-line.
 */
//--------------------------------------------------------------------------------------------------
static void StartDeferral
(
    void
)
{
    le_updateCtrl_ConnectService();
    // Setup the signal event handler before we do Defer. This way, even if we get signalled
    // before Defer gets done we won't deal with the signal until the next time round the
    // event loop - so our Defer and Allow count will match by the time we exit.
    le_sig_Block(SIGINT);
    le_sig_SetEventHandler(SIGINT, EndDeferral);
    le_sig_Block(SIGTERM);
    le_sig_SetEventHandler(SIGTERM, EndDeferral);

    le_updateCtrl_Defer();
    // Our work is done here. Go wait on the event loop until someone SIGINTs or kills us.
    Done = true;
}


//--------------------------------------------------------------------------------------------------
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

        case LE_UPDATE_STATE_DOWNLOAD_SUCCESS:
            le_update_Install();
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
            break;

        case LE_UNAVAILABLE:
            fprintf(stderr, "**ERROR: Updates are currently deferred.\n");
            break;

        case LE_OK:
            break;

        default:
            fprintf(stderr, "**ERROR: Unexpected result code from update server.\n");
            break;
    }

    // Closing fd is unnecessary since the messaging infrastructure underneath
    // le_update_Start API that use it would close it.

    if (result != LE_OK)
    {
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
    else if (result == LE_BUSY)
    {
        fprintf(stderr, "Failed to remove app '%s'. System busy, check logs.\n", appNamePtr);
    }
    else if (result == LE_NOT_FOUND)
    {
        fprintf(stderr, "App '%s' is not installed\n", appNamePtr);
    }
    else
    {
        fprintf(stderr, "Failed to remove app '%s' (%s)\n", appNamePtr, LE_RESULT_TXT(result));
    }

    exit(EXIT_FAILURE);
}


COMPONENT_INIT
{
    // update --help
    le_arg_SetFlagCallback(PrintHelp, NULL, "help");

    // force option --force for mark-good. Must be set first
    le_arg_SetFlagCallback(SetForce, "f", "force");

    // update --remove APP_NAME
    le_arg_SetFlagCallback(RemoveSelected, "r", "remove");

    // update --mark-good
    le_arg_SetFlagCallback(MarkGood,"g", "mark-good");

    // update --mark-bad
    le_arg_SetFlagCallback(MarkBad, "b", "mark-bad");

    // update --defer
    le_arg_SetFlagCallback(StartDeferral, "d", "defer");

    // update [FILE_NAME]
    le_arg_AddPositionalCallback(HandlePositionalArg);
    le_arg_AllowLessPositionalArgsThanCallbacks();

    le_arg_Scan();

    if (!Done)
    {
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
}
