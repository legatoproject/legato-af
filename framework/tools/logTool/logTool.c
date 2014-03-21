/** @file logTool.c
 *
 * This is the Legato log command line tool used to set logging configurations for legato
 * components.  When the user executes the log command the proper arguments must be specified
 * depending on the command.  The command is then translated and sent to the Legato log daemon which
 * forwards it to the correct destination.
 *
 * The general format of log commands is:
 *
 * @verbatim
$ log command commandParameter in destination
@endverbatim
 *
 * The following are examples of supported commands:
 *
 * To set the log level to INFO for a component in a process:
 * @verbatim
$ log level INFO in "processName/componentName"
@endverbatim
 *
 * To enable a trace:
 * @verbatim
$ log trace "keyword" in "processName/componentName"
@endverbatim
 *
 * To disable a trace:
 * @verbatim
$ log stoptrace "keyword" in "processName/componentName"
@endverbatim
 *
 *
 * With all of the above examples "*" can be used in place of processName and componentName to mean
 * all processes and/or all components.  In fact if the "processName/componentName" is omitted the
 * default destination is set to all processes and all components.  Also in the examples above the
 * 'in' is optional.
 *
 * The translated command to send to the log daemon has this format:
 *
 *    ----------------------------------------
 *    | cmd | destination | commandParameter |
 *    ----------------------------------------
 *
 * where,
 *    cmd is a command code that is one byte in length.
 *    destination is the "processName/componentName" followed by a '/' character.
 *    commandParameter is the string specific to the command.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#include "legato.h"
#include "log.h"
#include "logDaemon.h"
#include "limit.h"
#include <ctype.h>


//--------------------------------------------------------------------------------------------------
/**
 * Log command strings.
 */
//--------------------------------------------------------------------------------------------------
#define CMD_SET_LEVEL_STR               "level"
#define CMD_ENABLE_TRACE_STR            "trace"
#define CMD_DISABLE_TRACE_STR           "stoptrace"
#define CMD_LIST_COMPONENTS_STR         "list"
#define CMD_FORGET_PROCESS_STR          "forget"
#define CMD_HELP_STR                    "help"


//--------------------------------------------------------------------------------------------------
/**
 * Information string that is printed when there is an error.
 */
//--------------------------------------------------------------------------------------------------
#define ERROR_INFO_STR      "Try 'log help' for more information.\n"


//--------------------------------------------------------------------------------------------------
/**
 * The default destination for commands.  The default is to address the command to all processes and
 * components.
 */
//--------------------------------------------------------------------------------------------------
#define DEFAULT_DEST_STR    "*/*"


//--------------------------------------------------------------------------------------------------
/**
 * Maximum parameter length.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_CMD_PARAM_BYTES 512


//--------------------------------------------------------------------------------------------------
/**
 * True if an error response was received from the Log Control Daemon.
 **/
//--------------------------------------------------------------------------------------------------
static bool ErrorOccurred = false;


//--------------------------------------------------------------------------------------------------
/**
 * The command being executed.
 **/
//--------------------------------------------------------------------------------------------------
static char Command;


//--------------------------------------------------------------------------------------------------
/**
 * Translates a log command string to the log command character.
 *
 * @return
 *      The log command if successful.
 *      NULL character if unsuccessful.
 */
//--------------------------------------------------------------------------------------------------
static char GetCmdChar
(
    char* cmdStringPtr          // The command string.
)
{
    if (strcmp(cmdStringPtr, CMD_SET_LEVEL_STR) == 0)
    {
        return LOG_CMD_SET_LEVEL;
    }
    else if (strcmp(cmdStringPtr, CMD_ENABLE_TRACE_STR) == 0)
    {
        return LOG_CMD_ENABLE_TRACE;
    }
    else if (strcmp(cmdStringPtr, CMD_DISABLE_TRACE_STR) == 0)
    {
        return LOG_CMD_DISABLE_TRACE;
    }
    else if (strcmp(cmdStringPtr, CMD_LIST_COMPONENTS_STR) == 0)
    {
        return LOG_CMD_LIST_COMPONENTS;
    }
    else if (strcmp(cmdStringPtr, CMD_FORGET_PROCESS_STR) == 0)
    {
        return LOG_CMD_FORGET_PROCESS;
    }
    else
    {
        return '\0';
    }
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
    puts(
        "NAME:\n"
        "    log - Sets log configurations for Legato components.\n"
        "\n"
        "SYNOPSIS:\n"
        "    log list\n"
        "    log level FILTER_STR [in] [DESTINATION]\n"
        "    log trace KEYWORD_STR [in] [DESTINATION]\n"
        "    log stoptrace KEYWORD_STR [in] [DESTINATION]\n"
        "    log forget PROCESS_NAME\n"
        "\n"
        "DESCRIPTION:\n"
        "    log list            Lists all processes/components registered with the\n"
        "                        log daemon.\n"
        "\n"
        "    log level           Sets the log filter level.  Log messages that are\n"
        "                        less severe than the filter will be ignored.\n"
        "                        The FILTER_STR must be one of the following:\n"
        "                            EMERGENCY\n"
        "                            CRITICAL\n"
        "                            ERROR\n"
        "                            WARNING\n"
        "                            INFO\n"
        "                            DEBUG\n"
        "\n"
        "    log trace           Enables a trace by keyword.  Any traces with a\n"
        "                        matching keyword is logged.  The KEYWORD_STR is a\n"
        "                        trace keyword.\n"
        "\n"
        "    log stoptrace       Disables a trace keyword.  Any traces with this\n"
        "                        keyword is not logged.  The KEYWORD_STR is a trace\n"
        "                        keyword.\n"
        "\n"
        "    log forget          Forgets all settings for processes with a given name.\n"
        "                        Future processes with that name will have default\n"
        "                        settings.\n"
        "\n"
        "The [DESTINATION] is optional and specifies the process and component to\n"
        "send the command to.  The [DESTINATION] must be in this format:\n"
        "\n"
        "    \"process/componentName\"\n"
        "\n"
        "The 'process' may be either a processName or a PID.  If the 'process' is a\n"
        "processName then the command will apply to all processes with the same name.\n"
        "If the 'process' is a PID then the command will only apply to the process\n"
        "with the matching PID.\n"
        "\n"
        "Both the 'process' and the 'componentName' may be replaced with '*' to mean\n"
        "all processes and/or all components.  If the [DESTINATION] is omitted the\n"
        "default destination \"*/*\" is used meaning all processes and all components.\n"
        "\n"
        "The [in] preceding the [DESTINATION] is optional and may be omitted.\n"
        "\n"
        "A command may be sent to a process/component that may not exist yet.  The\n"
        "command will be saved and applied to the process/component when the process\n"
        "and component are available.  This makes it possible to pre-configure\n"
        "processes/components before they are spawned.  However, this is only valid\n"
        "if the 'process' in the [DESTINATION] is a process name.  If the 'process'\n"
        "in the [DESTINATION] is a PID but the PID does not exist yet the command\n"
        "will be dropped."
        );
}


//--------------------------------------------------------------------------------------------------
/**
 * Handles a message received from the Log Control Daemon.
 **/
//--------------------------------------------------------------------------------------------------
static void MsgReceiveHandler
(
    le_msg_MessageRef_t msgRef,
    void* contextPtr // not used.
)
{
    const char* responseStr = le_msg_GetPayloadPtr(msgRef);
    // Print out whatever the Log Control Daemon sent us.
    printf("%s\n", responseStr);

    // If the first character of the response is a '*', then there has been an error.
    if (responseStr[0] == '*')
    {
        ErrorOccurred = true;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Handles the Log Control Daemon closing the IPC session.
 **/
//--------------------------------------------------------------------------------------------------
static void SessionCloseHandler
(
    le_msg_SessionRef_t sessionRef, // not used.
    void* contextPtr // not used.
)
{
    if (ErrorOccurred)
    {
        exit(EXIT_FAILURE);
    }
    else
    {
        exit(EXIT_SUCCESS);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Opens an IPC session with the Log Control Daemon.
 */
//--------------------------------------------------------------------------------------------------
static le_msg_MessageRef_t ConnectToLogControlDaemon
(
    void
)
{
    le_msg_ProtocolRef_t protocolRef = le_msg_GetProtocolRef(LOG_CONTROL_PROTOCOL_ID,
                                                             LOG_MAX_CMD_PACKET_BYTES);
    le_msg_SessionRef_t sessionRef = le_msg_CreateSession(protocolRef, LOG_CONTROL_SERVICE_NAME);

    le_msg_SetSessionRecvHandler(sessionRef, MsgReceiveHandler, NULL);
    le_msg_SetSessionCloseHandler(sessionRef, SessionCloseHandler, NULL);

    le_result_t result = le_msg_OpenSessionSync(sessionRef);
    if (result == LE_OK)
    {
        return le_msg_CreateMsg(sessionRef);
    }
    else
    {
        printf("***ERROR: Can't communicate with the Log Control Daemon.\n");

        if (result == LE_COMM_ERROR)
        {
            printf("Service Directory is unreachable.\n"
                   "Perhaps the Service Directory is not running?\n");
        }
        else if (result == LE_NOT_PERMITTED)
        {
            printf("Permission denied.\n");
        }
        else
        {
            printf("Unexpected result code from le_msg_OpenSessionSync(): %d (%s).",
                   result,
                   LE_RESULT_TXT(result));
        }

        exit(EXIT_FAILURE);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Print error messages and exits.
 */
//--------------------------------------------------------------------------------------------------
static void ExitWithErrorMsg
(
    const char* errorMsg
)
{
    printf("%s\n", errorMsg);
    printf(ERROR_INFO_STR);
    exit(EXIT_FAILURE);
}


//--------------------------------------------------------------------------------------------------
/**
 * Parses a command-line specification of the severity level.  Converts it into an le_log_Level_t.
 *
 * @return The level, or -1 on error.
 **/
//--------------------------------------------------------------------------------------------------
le_log_Level_t ParseSeverityLevel
(
    const char* levelStr
)
{
    char buff[12];

    if (*levelStr == '\0')
    {
        return -1;
    }

    // Copy to a writable buffer so we can manipulate the string.
    if (le_utf8_Copy(buff, levelStr, sizeof(buff), NULL) != LE_OK)
    {
        return -1;
    }

    // Convert everything to lower case to remove case sensitivity.
    int i;
    for (i = 0; (i < sizeof(buff)) && (buff[i] != '\0'); i++)
    {
        buff[i] = tolower(buff[i]);
    }

    // Now compare the strings.

    if (   (strcmp(buff, "d") == 0)
        || (strcmp(buff, "debug") == 0) )
    {
        return LE_LOG_DEBUG;
    }

    if (   (strcmp(buff, "i") == 0)
        || (strcmp(buff, "info") == 0) )
    {
        return LE_LOG_INFO;
    }

    if (   (strcmp(buff, "w") == 0)
        || (strcmp(buff, "warn") == 0)
        || (strcmp(buff, "warning") == 0) )
    {
        return LE_LOG_WARN;
    }

    if (   (strcmp(buff, "e") == 0)
        || (strcmp(buff, "err") == 0)
        || (strcmp(buff, "error") == 0) )
    {
        return LE_LOG_ERR;
    }

    if (   (strcmp(buff, "c") == 0)
        || (strcmp(buff, "crit") == 0)
        || (strcmp(buff, "critical") == 0) )
    {
        return LE_LOG_CRIT;
    }

    if (   (strcmp(buff, "em") == 0)
        || (strcmp(buff, "emerg") == 0)
        || (strcmp(buff, "emergency") == 0) )
    {
        return LE_LOG_EMERG;
    }

    return -1;
}


//--------------------------------------------------------------------------------------------------
/**
 * Verifies that the number of arguments is in a given range.
 *
 * Logs a message and exits if not.
 **/
//--------------------------------------------------------------------------------------------------
static void VerifyArgCount
(
    uint min,   ///< [IN] Minimum number of arguments (not including the command name)
    uint max    ///< [IN] Maximum number of arguments (not including the command name)
)
{
    if (le_arg_NumArgs() < (min + 1))
    {
        ExitWithErrorMsg("log: Too few arguments for command.");
    }
    else if (le_arg_NumArgs() > (max + 1))
    {
        ExitWithErrorMsg("log: Too many arguments for command.");
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * The main function for the log tool.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    char arg[LIMIT_MAX_PATH_LEN];
    size_t n;

    // Check if the user is asking for help.
    if (le_arg_GetArg(0, arg, LIMIT_MAX_PATH_LEN) != LE_OK)
    {
        ExitWithErrorMsg("log: Invalid log command.");
    }

    if (strcmp(arg, CMD_HELP_STR) == 0)
    {
        // print the help file to the screen.
        PrintHelp();

        exit(EXIT_SUCCESS);
    }

    le_msg_MessageRef_t msgRef = ConnectToLogControlDaemon();

    char* cmdBuffPtr = le_msg_GetPayloadPtr(msgRef);
    unsigned int buffLength = 0;

    // Get the command.
    Command = GetCmdChar(arg);
    cmdBuffPtr[buffLength++] = Command;

    switch (Command)
    {
        case LOG_CMD_LIST_COMPONENTS:
            // This command has no parameters and no destination.
            break;

        case LOG_CMD_FORGET_PROCESS:
        {
            // This command has only a process name (or pid) as a parameter.

            VerifyArgCount(1, 1);

            char cmdParam[MAX_CMD_PARAM_BYTES];
            if (le_arg_GetArg(1, cmdParam, MAX_CMD_PARAM_BYTES) != LE_OK)
            {
                ExitWithErrorMsg("log: Invalid command parameter.");
            }
            else if (LE_OVERFLOW == le_utf8_Copy(cmdBuffPtr + buffLength,
                                                 cmdParam,
                                                 LOG_MAX_CMD_PACKET_BYTES - buffLength - 1,
                                                 &n) )
            {
                ExitWithErrorMsg("log: Command string is too long.");
            }
            buffLength += n;

            break;
        }

        case LOG_CMD_SET_LEVEL:
        case LOG_CMD_ENABLE_TRACE:
        case LOG_CMD_DISABLE_TRACE:
        {
            // These commands must have a parameter.
            char cmdParam[MAX_CMD_PARAM_BYTES];
            if (le_arg_GetArg(1, cmdParam, MAX_CMD_PARAM_BYTES) != LE_OK)
            {
                ExitWithErrorMsg("log: Invalid command parameter.");
            }

            // Get the destination.
            char* destPtr;
            if (le_arg_GetArg(2, arg, LIMIT_MAX_PATH_LEN) == LE_NOT_FOUND)
            {
                // If there are no other parameters then use the default destination.
                destPtr = DEFAULT_DEST_STR;
            }
            else if (le_arg_NumArgs() == 3)
            {
                // The "in" before the destination is optional and so the next argument is the
                // destination.
                destPtr = arg;
            }
            else if ( (strcmp(arg, "in") == 0) &&
                      (le_arg_GetArg(3, arg, LIMIT_MAX_PATH_LEN) == LE_OK) )
            {
                // The parameter after the "in" is the destination.  Ignore all remaining parameters.
                destPtr = arg;
            }
            else
            {
                // The destination is incorrect.
                ExitWithErrorMsg("log: Invalid destination.");
            }

            // Check that the destination is formatted correctly.
            if (strchr(destPtr, '/') == NULL)
            {
                ExitWithErrorMsg("log: Invalid destination.");
            }

            // Copy the destination to the command buffer.
            if (le_utf8_Copy(&(cmdBuffPtr[buffLength]), destPtr,
                             LOG_MAX_CMD_PACKET_BYTES - buffLength - 2, &n) == LE_OVERFLOW)
            {
                ExitWithErrorMsg("log: Command string is too long.");
            }
            buffLength += n;

            // Add a slash after the destination.
            cmdBuffPtr[buffLength++] = '/';
            cmdBuffPtr[buffLength] = '\0';

            // Verify the command parameter string.
            if (Command == LOG_CMD_SET_LEVEL)
            {
                // Check that string is one of the level strings.
                le_log_Level_t level = ParseSeverityLevel(cmdParam);
                if (level == -1)
                {
                    ExitWithErrorMsg("log: Invalid log level.");
                }
                else
                {
                    const char* levelStr = log_SeverityLevelToStr(level);
                    LE_ASSERT(levelStr != NULL);
                    LE_ASSERT(LE_OK == le_utf8_Copy(&(cmdBuffPtr[buffLength]),
                                                    levelStr,
                                                    LOG_MAX_CMD_PACKET_BYTES - buffLength - 2,
                                                    &n) );
                }
            }
            else
            {
                // Copy the command parameter string to the command buffer.
                if (LE_OVERFLOW == le_utf8_Copy(cmdBuffPtr + buffLength,
                                                cmdParam,
                                                LOG_MAX_CMD_PACKET_BYTES - buffLength - 1,
                                                &n) )
                {
                    ExitWithErrorMsg("log: Command string is too long.");
                }
            }
            buffLength += n;

            break;
        }

        default:
            ExitWithErrorMsg("log: Invalid log command.");
    }

    // Send the command and wait for messages from the Log Control Daemon.  When the Log Control
    // Daemon has finished executing the command, it will close the IPC session.
    le_msg_Send(msgRef);
}
