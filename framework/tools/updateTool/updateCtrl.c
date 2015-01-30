/** @file updateCtrl.c
 *
 * Basic working principle of update tool as follows
 * 1) Update tool will receive install/uninstall file with manifest string prepended at the
 *    beginning. It might receive file either via STDIN or via parameter.
 * 2) Extract manifest and find out the command (with its parameters) from manifest.
 * 3) Call appropiate API (for firmware update) /fork-exec(for app) right process to execute the
 *    command.
 *
 * @LIMITATION: Currently only firmware update and app install/remove are supported in this tool.
 *    Should be extended for framework & system.
 * @TODO: Replace update tool with update daemon.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "limit.h"
#include "interfaces.h"
#include "manifest.h"
#include "fileDescriptor.h"


/// Targets to be supported by update tool
// TODO: No support for framework and system yet
#define PARAM_FRAMEWORK              "framework"
#define PARAM_FIRMWARE               "firmware"
#define PARAM_APPLICATION            "app"
#define PARAM_SYSTEM                 "system"

/// Supported commands by update tool
#define CMD_UPDATE                   "update"
#define CMD_REMOVE                   "remove"

/// Command for target tool(e.g. app control tool)
#define CMD_INSTALL                  "install"

/// Location of app control tool
// TODO: This path should come from .config file.
#define APP_TOOL_PATH                "/usr/local/bin/app"

/// Timeout value for STDIN stream. Used when update tool expect data via STDIN.
// TODO: Current timeout = 1s. Tune this if needed.
#define SELECT_TIMEOUT_SEC           1
#define SELECT_TIMEOUT_uSEC          0

/// Minimum chunk size for file/stream read & write
// TODO: Tune this if needed. It might be specified in .config file.
#define CHUNK_SIZE                   4096


/// File system path of the input file, or "-" for stdin.
static const char* FilePath = "-";


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
 * If the condition is true, print a generic message on stderr and call cleanup function, log the
 * internal error message and exit.
 */
//--------------------------------------------------------------------------------------------------
#define CLEANUP_ON_ERR_IF(condition, cleanupFunction, formatString, ...)               \
        if (condition) { cleanupFunction;                                              \
            INTERNAL_ERR(formatString, ##__VA_ARGS__); }

//--------------------------------------------------------------------------------------------------
/**
 * Close multiple file descriptors.  Used as cleanup function while exit on error
 */
//--------------------------------------------------------------------------------------------------
static void CloseFiles
(
    int numFileDesc,                      ///<[IN] No. of file descriptors to close.
    ...                                   ///<[IN] List of file descriptors (variable params).
)
{
    va_list fileDescriptors;
    int index = 0, fileDescToClose;

    // Initializing fileDescriptors to store all values after numFileDesc
    va_start(fileDescriptors, numFileDesc);

    // Close all file descriptors one by one
    for (index = 0; index < numFileDesc; index++)
    {
        fileDescToClose = va_arg(fileDescriptors, int);
        fd_Close(fileDescToClose);
    }

    va_end(fileDescriptors);
}

//--------------------------------------------------------------------------------------------------
/**
 * Closes a file descriptor.
 *
 * @note This is a wrapper around close() that takes care of retrying if interrupted by a signal.
 *
 * @return
 *        Zero on success, -1 on error.
 */
//--------------------------------------------------------------------------------------------------
static int Closefd
(
    int fd                                ///<[IN] File descriptor to close.
)
{
    int result;

    // Keep trying to close the fd as long as it keeps getting interrupted by signals.
    do
    {
        result = close(fd);
    }
    while ((result == -1) && (errno == EINTR));

    return result;
}

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
 * This function tries to figure out the input file to update tool.  Input file either might be
 * given via STDIN or parameter.  If no parameter is given, this function waits for data through
 * STDIN.
 *
 * @return
 *      File descriptor pointing to update file.
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
 * Handles App installation/remove specific stuffs.
 *
 * @note
 *      This function does not return.
 */
//--------------------------------------------------------------------------------------------------
static void HandleAppCmds
(
    int fileDesc,                            ///<[IN] File Descriptor of update data.
    char (*cmdStr)[MAN_MAX_CMD_TOKEN_BYTES], ///<[IN] Array containing command with its params.
    int cmdParamsNum,                        ///<[IN] Number of elements in command string array.
    size_t payLoadSize                       ///<[IN] Size of update data.
)
{
    int status, pid, count = 1, fildes[2];
    char *execArg[cmdParamsNum+1];
    int pipeReadfd,pipeWritefd;
    size_t payLoadLeft = payLoadSize;

    execArg[0] = APP_TOOL_PATH;
    while (count < cmdParamsNum)
    {
        execArg[count] = cmdStr[count];
        count++;
    }

    execArg[count] = NULL;

    CLEANUP_ON_ERR_IF((pipe(fildes) == -1), CloseFiles(1, fileDesc),
                       "Can't create  pipe, errno: %d (%m)", errno);

    CLEANUP_ON_ERR_IF(((pid = fork()) == -1), CloseFiles(3, fileDesc, fildes[0], fildes[1]),
                      "Can't create child process, errno: %d (%m)", errno);

    pipeReadfd=fildes[0];
    pipeWritefd=fildes[1];

    if (pid == 0)
    {
        INTERNAL_ERR_IF(((Closefd(pipeWritefd) == -1) || (Closefd(fileDesc) == -1)),
                        "Child process can't close write end of pipe, errno: %d (%m)", errno);

        INTERNAL_ERR_IF((dup2(pipeReadfd, STDIN_FILENO) == -1),
                        "Cannot tie read end of pipe to stdin, errno: %d (%m)", errno);

        INTERNAL_ERR_IF((execv(APP_TOOL_PATH, execArg) == -1),
                        "Error while exec: %s , errno: %d (%m)", APP_TOOL_PATH, errno);
    }
    else
    {
        char buffer[CHUNK_SIZE+1] = "";
        CLEANUP_ON_ERR_IF((Closefd(pipeReadfd) == -1), CloseFiles(1, fileDesc),
                          "Parent process cannot close read end of pipe, errno: %d (%m)", errno);

        while (payLoadLeft > 0)
        {
            ssize_t readReqSize, result;

            readReqSize = (payLoadLeft >= CHUNK_SIZE) ? CHUNK_SIZE : payLoadLeft;
            buffer[readReqSize] = '\0';

            // Unix read() might return less amount of data than requested. So called our custom
            // read function.
            result = fd_ReadSize(fileDesc, buffer, readReqSize);

            CLEANUP_ON_ERR_IF((result == LE_FAULT), CloseFiles(2, fileDesc, pipeWritefd),
                              "Read error. errno: %d (%m).", errno);

            CLEANUP_ON_ERR_IF((result != readReqSize), CloseFiles(2, fileDesc, pipeWritefd),
                              "Wrong payload size: %zd B. Reached EOF after reading %zd B",
                              payLoadSize, payLoadSize - payLoadLeft);

            // Unix write() might write less amount of data than requested. So called our custom
            // write function.
            CLEANUP_ON_ERR_IF((fd_WriteSize(pipeWritefd, buffer, readReqSize) == LE_FAULT),
                              CloseFiles(2, fileDesc, pipeWritefd),
                              "Write error. errno: %d (%m), buffer: %s", errno, buffer);

            payLoadLeft -= readReqSize;
        }

        CLEANUP_ON_ERR_IF((Closefd(pipeWritefd) == -1), CloseFiles(1, fileDesc),
                          "Parent process can't close write end of pipe, errno: %d (%m)", errno);

        CLEANUP_ON_ERR_IF((waitpid(pid, &status, 0) == -1), CloseFiles(2, pipeWritefd, fileDesc),
                          "Error calling waitpid, errno: %d (%m)", errno);

    }

    CloseFiles(1, fileDesc);
    exit(EXIT_SUCCESS);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handles firmware installation specific stuffs.
 *
 * @note
 *      This function does not return.
 */
//--------------------------------------------------------------------------------------------------
static void HandleFwCmds
(
    int fileDesc,                            ///<[IN] File Descriptor of update data.
    char (*cmdStr)[MAN_MAX_CMD_TOKEN_BYTES], ///<[IN] Array containing command with its params.
    int cmdParamsNum                         ///<[IN] Number of elements in command string array.
)
{
    // Start client for fwupdate. This is started only before fwupdate
    // TODO: Use tryconnect when proper API is available
    le_fwupdate_ConnectService();

    fprintf(stdout, "Updating firmware\n");
    le_result_t resultCode = le_fwupdate_Download(fileDesc);

    if (resultCode == LE_OK)
    {
        fprintf(stdout, "Download successful; please wait for modem to reset\n");
        CloseFiles(1, fileDesc);
        exit(EXIT_SUCCESS);
    }
    else
    {
        fprintf(stderr, "Error in download, resultCode: %d\n", (int)resultCode);
        CloseFiles(1, fileDesc);
        exit(EXIT_FAILURE);
    }
}

//--------------------------------------------------------------------------------------------------
/**
* Handle app/system/firmware/framework update specific stuffs
*
* @return
*      This function does not return.
*/
//--------------------------------------------------------------------------------------------------
static void HandleUpdateCmd
(
    int fileDesc,                            ///<[IN] File Descriptor of update data.
    char (*cmdStr)[MAN_MAX_CMD_TOKEN_BYTES], ///<[IN] Array containing command with its params.
    int cmdParamsNum,                        ///<[IN] Number of elements in command string array.
    size_t payLoadSize                       ///<[IN] Size of update data.
)
{
    if (strcmp(cmdStr[1], PARAM_APPLICATION) == 0)
    {
        strcpy(cmdStr[1], CMD_INSTALL);
        HandleAppCmds(fileDesc, cmdStr, cmdParamsNum, payLoadSize);
    }
    else if (strcmp(cmdStr[1], PARAM_FIRMWARE) == 0)
    {
        // TODO: Invoke firmware install tool
        HandleFwCmds(fileDesc, cmdStr, cmdParamsNum);
    }
    else if (strcmp(cmdStr[1], PARAM_FRAMEWORK) == 0)
    {
        // TODO: Invoke framework install/remove tool
        fprintf(stderr, "Update not supported for %s yet.\n", cmdStr[1]);
    }
    else if (strcmp(cmdStr[1], PARAM_SYSTEM) == 0)
    {
        // TODO: Invoke system install/remove tool
        fprintf(stderr, "Update not supported for %s yet.\n", cmdStr[1]);
    }
    else
    {
        fprintf(stderr, "Unknown command '%s'.  Try --help.\n", cmdStr[1]);
    }

    exit(EXIT_FAILURE);
}

//--------------------------------------------------------------------------------------------------
/**
* Handle app/system/firmware/framework remove specific stuffs
*
* @return
*      This function doesn't return.
*/
//--------------------------------------------------------------------------------------------------
static void HandleRemoveCmd
(
    int fileDesc,                            ///<[IN] File Descriptor of remove data.
    char (*cmdStr)[MAN_MAX_CMD_TOKEN_BYTES], ///<[IN] Array containing command with its params.
    int cmdParamsNum                         ///<[IN] Number of elements in command string array.
)
{
    if (strcmp(cmdStr[1], PARAM_APPLICATION) == 0)
    {
        strcpy(cmdStr[1], CMD_REMOVE);
        HandleAppCmds(fileDesc, cmdStr, cmdParamsNum, 0);
    }
    else if (strcmp(cmdStr[1], PARAM_FIRMWARE) == 0)
    {
        // TODO: Invoke framework install/remove tool
        fprintf(stderr, "Remove not supported for %s yet.\n", cmdStr[1]);
    }
    else if (strcmp(cmdStr[1], PARAM_FRAMEWORK) == 0)
    {
        // TODO: Invoke framework install/remove tool
        fprintf(stderr, "Remove not supported for %s yet.\n", cmdStr[1]);
    }
    else if (strcmp(cmdStr[1], PARAM_SYSTEM) == 0)
    {
        // TODO: Invoke system install/remove tool
        fprintf(stderr, "Remove not supported for %s yet.\n", cmdStr[1]);
    }
    else
    {
        fprintf(stderr, "Unknown command '%s'.  Try --help.\n", cmdStr[1]);
    }

    exit(EXIT_FAILURE);
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
COMPONENT_INIT
{
    // update --help
    le_arg_SetFlagCallback(PrintHelp, NULL, "help");

    // update [FILE_NAME]
    le_arg_AddPositionalCallback(HandleFilePath);
    le_arg_AllowLessPositionalArgsThanCallbacks();

    le_arg_Scan();

    int fd = GetUpdateFile();

    char cmdList[MAN_MAX_TOKENS_IN_CMD_STR][MAN_MAX_CMD_TOKEN_BYTES];
    int cmdParamsNum = 0;
    size_t payLoadSize = 0;
    man_Ref_t manifestRef;

    CLEANUP_ON_ERR_IF(((manifestRef = man_Create(fd))== NULL),
                      CloseFiles(1, fd), "Error in getting manifest");

    payLoadSize = man_GetPayLoadSize(manifestRef);

    CLEANUP_ON_ERR_IF((man_GetCmd(manifestRef, cmdList, &cmdParamsNum) == LE_FAULT),
                      CloseFiles(1, fd), "Error in extracting command from manifest");

    // Command params should be minimum 2, otherwise exit.
    if (cmdParamsNum < 2)
    {
        fprintf(stderr, "Too few params for command: %s. Please look at update-pack documentation\n",
                cmdList[0]);
        CloseFiles(1, fd);
        exit(EXIT_FAILURE);
    }

    // Process the command.
    if (strcmp(cmdList[0], CMD_UPDATE) == 0)
    {
        //Matches with update command
        //Following update commands will be supported
        //update app <appName>
        //update firmware
        //update framework
        //update system <systemName>

        HandleUpdateCmd(fd, cmdList, cmdParamsNum, payLoadSize);
    }
    else if (strcmp(cmdList[0], CMD_REMOVE) == 0)
    {
        //Matches with update command
        //Following update commands will be supported
        //remove app <appName>
        //remove framework
        //remove system <systemName>

        HandleRemoveCmd(fd, cmdList, cmdParamsNum);
    }

    fprintf(stderr, "Unknown command: %s. Please look at update-pack documentation\n", cmdList[0]);
    CloseFiles(1, fd);
    exit(EXIT_FAILURE);
}
