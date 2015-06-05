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
 * Prints a generic message on stderr so that the user is aware there is a problem, logs the
 * internal error message and exit.
 */
//--------------------------------------------------------------------------------------------------
#define INTERNAL_ERR(formatString, ...)                                                 \
            { fprintf(stderr, "Internal error check logs for details.\n");              \
              LE_FATAL(formatString, ##__VA_ARGS__); }


//--------------------------------------------------------------------------------------------------
/**
 * If the condition is true, print a generic message on stderr so that the user is aware there is a
 * problem, log the internal error message and exit.
 */
//--------------------------------------------------------------------------------------------------
#define INTERNAL_ERR_IF(condition, formatString, ...)                                   \
        if (condition) { INTERNAL_ERR(formatString, ##__VA_ARGS__); }


//--------------------------------------------------------------------------------------------------
/**
 * Handle to an update task. This handle is used to identify an update task.
 */
//--------------------------------------------------------------------------------------------------
static le_update_HandleRef_t Handle;


//--------------------------------------------------------------------------------------------------
/**
 * File system path of the input file, or "-" for stdin.
 */
//--------------------------------------------------------------------------------------------------
static const char* FilePath = "-";


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
    // TODO: Update this as per universal update.
    puts(
        "NAME:\n"
        "    update - install/remove utility for legato.\n"
        "\n"
        "SYNOPSIS:\n"
        "    update --help\n"
        "    update [FILE_NAME]\n"
        "\n"
        "DESCRIPTION:\n"
        "   update --help\n"
        "       Display this help and exit.\n"
        "\n"
        "   update [FILE_NAME]\n"
        "       Command takes an update file, decodes the manifest, and takes appropriate action.\n"
        "       If no file name or the file name '-' is given, input is taken from the standard\n"
        "       input stream (stdin).\n"
    );
    exit(EXIT_SUCCESS);
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
    void
)
{
    int fileDescriptor;

    if (strcmp(FilePath, "-") == 0)
    {
        fileDescriptor = 0;
    }
    else
    {
        fileDescriptor = open(FilePath, O_RDONLY);

        if (fileDescriptor == -1)
        {
            fprintf(stderr,
                    "Can't open file '%s': errno %d (%m)\n",
                    FilePath,
                    errno);
            exit(EXIT_FAILURE);
        }
    }

    return fileDescriptor;
}


//--------------------------------------------------------------------------------------------------
/**
 * Processes a file path argument from the command line.
 **/
//--------------------------------------------------------------------------------------------------
static void HandleFilePath
(
    const char* filePath
)
{
    FilePath = filePath;
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

    if (percentDone == 100)
    {
        fprintf(stdout, "\n");
    }

    fflush(stdout);
}


//--------------------------------------------------------------------------------------------------
/**
 * Print message according to the error code.
 */
//--------------------------------------------------------------------------------------------------
static void PrintErrorMsg
(
    le_update_ErrorCode_t errorCode       ///<[IN] Error code received from update daemon.
)
{
    switch (errorCode)
    {
        case LE_UPDATE_ERR_NONE:
            break;

        case LE_UPDATE_ERR_BAD_MANIFEST:
            fprintf(stderr, "***Error: Found bad manifest in update package\n");
            break;

        case LE_UPDATE_ERR_IO_ERROR:
            fprintf(stderr, "***Error: Faced I/O error during update\n");
            break;

        case LE_UPDATE_ERR_INTERNAL_ERROR:
            fprintf(stderr, "***Error: Faced internal error during update. Please see log for details.\n");
            break;

        case LE_UPDATE_ERR_OUT_OF_MEMORY:
            fprintf(stderr, "***Error: Too low memory for update\n");
            break;

        case LE_UPDATE_ERR_VERSION_MISMATCH:
            fprintf(stderr, "***Error: Wrong update version. Please see log for details.\n");
            break;

        case LE_UPDATE_ERR_WRONG_TARGET:
            fprintf(stderr, "***Error: Wrong target. Please see log for details.\n");
            break;

    }

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
    static const int StatusMsgBytes = 100;
    char statusMsg[StatusMsgBytes];

    switch(updateState)
    {
        // TODO: Currently app/firmware(app script, modemService) tools are not able to give
        // status in the middle of installation. Revise the way of printing when these tools
        // will be able to provide installation status.
        case LE_UPDATE_STATE_NEW:
            // Update started. Print this information.
            fprintf(stdout, "New update started\n");
            break;

        case LE_UPDATE_STATE_UNPACKING:
            // Reset all characters to zero.
            memset(statusMsg, 0, StatusMsgBytes);
            snprintf(statusMsg, sizeof(statusMsg), "Unpacking package");
            // Print progress bar if there is any noticeable progress.
            PrintProgressBar(percentDone, statusMsg);
            break;

        case LE_UPDATE_STATE_APPLYING:
            // Reset all characters to zero.
            memset(statusMsg, 0, StatusMsgBytes);
            snprintf(statusMsg, sizeof(statusMsg), "Applying update");
            // Print progress bar if there is any noticeable progress.
            PrintProgressBar(percentDone, statusMsg);
            break;

        case LE_UPDATE_STATE_SUCCESS:
            //Successful update(install/remove) task.
            fprintf(stdout, "SUCCESS\n");
            le_update_Delete(Handle);
            exit(EXIT_SUCCESS);

        case LE_UPDATE_STATE_FAILED:
            // Failure in update, exit with failure code.
            PrintErrorMsg(le_update_GetErrorCode(Handle));
            fprintf(stderr, "FAILED\n");
            le_update_Delete(Handle);
            exit(EXIT_FAILURE);
    }

}


COMPONENT_INIT
{
    // update --help
    le_arg_SetFlagCallback(PrintHelp, NULL, "help");

    // update [FILE_NAME]
    le_arg_AddPositionalCallback(HandleFilePath);
    le_arg_AllowLessPositionalArgsThanCallbacks();

    le_arg_Scan();

    int fd = GetUpdateFile();

    le_update_ConnectService();

    // Create and update handle.
    INTERNAL_ERR_IF(((Handle = le_update_Create(fd)) == NULL), "Update failure, exiting."
                    "FileDesc: %d.", fd);
    // Register callback function.
    INTERNAL_ERR_IF(le_update_AddProgressHandler(Handle, UpdateProgressHandler, NULL) == NULL,
                    "Can't register status handler");
    // Start update process(asynchronous). Completion will be notified via callback function.
    INTERNAL_ERR_IF( le_update_Start(Handle) != LE_OK, "Can't start update task !");

}
