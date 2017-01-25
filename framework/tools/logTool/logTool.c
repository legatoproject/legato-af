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
$ log command commandParameter destination
@endverbatim
 *
 * The following are examples of supported commands:
 *
 * To set the log level to INFO for a component in a process:
 * @verbatim
$ log level INFO processName/componentName
@endverbatim
 *
 * To enable a trace:
 * @verbatim
$ log trace keyword processName/componentName
@endverbatim
 *
 * To disable a trace:
 * @verbatim
$ log stoptrace keyword processName/componentName
@endverbatim
 *
 *
 * With all of the above examples "*" can be used in place of processName and componentName to mean
 * all processes and/or all components.  In fact if the "processName/componentName" is omitted the
 * default destination is set to all processes and all components.
 *
 * The translated command to send to the log daemon has this format:
 *
 * @verbatim
 *    ----------------------------------------
 *    | cmd | destination | commandParameter |
 *    ----------------------------------------
 * @endverbatim
 *
 * where,
 *    cmd is a command code that is one byte in length.
 *    destination is the "processName/componentName" followed by a '/' character.
 *    commandParameter is the string specific to the command.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "log.h"
#include "logDaemon.h"
#include "limit.h"
#include <ctype.h>


//--------------------------------------------------------------------------------------------------
/**
 * The default log session for commands, if not specified.
 *
 * The default is to address the command to all processes and components.
 */
//--------------------------------------------------------------------------------------------------
#define DEFAULT_SESSION_ID    "*/*"


//--------------------------------------------------------------------------------------------------
/**
 * Command character byte.
 **/
//--------------------------------------------------------------------------------------------------
static char Command;


//--------------------------------------------------------------------------------------------------
/**
 * Pointer to the "command parameter" string.  If used, this is a log level, trace keyword,
 * or process identifier.
 **/
//--------------------------------------------------------------------------------------------------
static const char* CommandParamPtr = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Pointer to the log session identifier.
 **/
//--------------------------------------------------------------------------------------------------
static const char* SessionIdPtr = DEFAULT_SESSION_ID;


//--------------------------------------------------------------------------------------------------
/**
 * True if an error response was received from the Log Control Daemon.
 **/
//--------------------------------------------------------------------------------------------------
static bool ErrorOccurred = false;


//--------------------------------------------------------------------------------------------------
/**
 * Prints help to stdout.
 */
//--------------------------------------------------------------------------------------------------
static void PrintHelpAndExit
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
        "    log level FILTER_STR [DESTINATION]\n"
        "    log trace KEYWORD_STR [DESTINATION]\n"
        "    log stoptrace KEYWORD_STR [DESTINATION]\n"
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
        "A command may be sent to a process/component that may not exist yet.  The\n"
        "command will be saved and applied to the process/component when the process\n"
        "and component are available.  This makes it possible to pre-configure\n"
        "processes/components before they are spawned.  However, this is only valid\n"
        "if the 'process' in the [DESTINATION] is a process name.  If the 'process'\n"
        "in the [DESTINATION] is a PID but the PID does not exist yet the command\n"
        "will be dropped."
        );
        exit(EXIT_SUCCESS);
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
 *
 * @return  A reference to the IPC message session.
 */
//--------------------------------------------------------------------------------------------------
static le_msg_SessionRef_t ConnectToLogControlDaemon
(
    void
)
{
    le_msg_ProtocolRef_t protocolRef = le_msg_GetProtocolRef(LOG_CONTROL_PROTOCOL_ID,
                                                             LOG_MAX_CMD_PACKET_BYTES);
    le_msg_SessionRef_t sessionRef = le_msg_CreateSession(protocolRef, LOG_CONTROL_SERVICE_NAME);

    le_msg_SetSessionRecvHandler(sessionRef, MsgReceiveHandler, NULL);
    le_msg_SetSessionCloseHandler(sessionRef, SessionCloseHandler, NULL);

    le_result_t result = le_msg_TryOpenSessionSync(sessionRef);
    if (result != LE_OK)
    {
        printf("***ERROR: Can't communicate with the Log Control Daemon.\n");

        switch (result)
        {
            case LE_UNAVAILABLE:
                printf("Service not offered by Log Control Daemon.\n"
                       "Perhaps the Log Control Daemon is not running?\n");
                break;

            case LE_NOT_PERMITTED:
                printf("Missing binding to log control service.\n"
                       "System misconfiguration detected.\n");
                break;

            case LE_COMM_ERROR:
                printf("Service Directory is unreachable.\n"
                       "Perhaps the Service Directory is not running?\n");
                break;

            default:
                printf("Unexpected result code %d (%s)\n", result, LE_RESULT_TXT(result));
                break;
        }
        exit(EXIT_FAILURE);
    }

    return sessionRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Print error messages and exits.
 */
//--------------------------------------------------------------------------------------------------
__attribute__ ((__noreturn__))
static void ExitWithErrorMsg
(
    const char* errorMsg
)
{
    printf("log: %s\n", errorMsg);
    printf("Try 'log --help' for more information.\n");
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
 * Appends some text to the command message.
 **/
//--------------------------------------------------------------------------------------------------
static void AppendToCommand
(
    le_msg_MessageRef_t msgRef, ///< Command message to which the text should be appended.
    const char* textPtr         ///< Text to append to the message.
)
{
    if (LE_OVERFLOW == le_utf8_Append(le_msg_GetPayloadPtr(msgRef),
                                      textPtr,
                                      le_msg_GetMaxPayloadSize(msgRef),
                                      NULL) )
    {
        ExitWithErrorMsg("Command string is too long.");
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Function that gets called by le_arg_Scan() when a log session identifier is seen on the
 * command line.
 **/
//--------------------------------------------------------------------------------------------------
static void SessionIdArgHandler
(
    const char* sessionId
)
{
    // Check that the session identifier is formatted correctly.
    if (strchr(sessionId, '/') == NULL)
    {
        // Permit an optional "in" here, once.
        if (strcmp(sessionId, "in") == 0)
        {
            static bool optionalInSeen = false;
            if (!optionalInSeen)
            {
                optionalInSeen = true;
                le_arg_AddPositionalCallback(SessionIdArgHandler);
            }
        }

        ExitWithErrorMsg("Invalid destination.");
    }

    SessionIdPtr = sessionId;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function that gets called by le_arg_Scan() when a log level argument is seen on the command
 * line.
 **/
//--------------------------------------------------------------------------------------------------
static void LogLevelArgHandler
(
    const char* logLevel
)
{
    // Check that string is one of the level strings.
    le_log_Level_t level = ParseSeverityLevel(logLevel);
    if (level == (le_log_Level_t)(-1))
    {
        ExitWithErrorMsg("Invalid log level.");
    }

    const char* levelStr = log_SeverityLevelToStr(level);
    LE_ASSERT(levelStr != NULL);
    CommandParamPtr = levelStr;

    // Wait for an optional log session identifier next.
    le_arg_AddPositionalCallback(SessionIdArgHandler);
    le_arg_AllowLessPositionalArgsThanCallbacks();
}


//--------------------------------------------------------------------------------------------------
/**
 * Function the gets called by le_arg_Scan() when a trace keyword argument is seen on the command
 * line.
 **/
//--------------------------------------------------------------------------------------------------
static void TraceKeywordArgHandler
(
    const char* keyword
)
{
    CommandParamPtr = keyword;

    // Wait for an optional log session identifier next.
    le_arg_AddPositionalCallback(SessionIdArgHandler);
    le_arg_AllowLessPositionalArgsThanCallbacks();
}


//--------------------------------------------------------------------------------------------------
/**
 * Function that gets called by le_arg_Scan() when the process identifier argument (either a process
 * name or a PID) for a "forget" command is found on the command line.
 **/
//--------------------------------------------------------------------------------------------------
static void ProcessIdArgHandler
(
    const char* processId
)
{
    CommandParamPtr = processId;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function that gets called by le_arg_Scan() when it sees the first positional argument while
 * scanning the command line.  This should be the command name.
 **/
//--------------------------------------------------------------------------------------------------
static void CommandArgHandler
(
    const char* command
)
{
    if (strcmp(command, "help") == 0)
    {
        PrintHelpAndExit();
    }

    else if (strcmp(command, "level") == 0)
    {
        Command = LOG_CMD_SET_LEVEL;

        // Expect a log level next.
        le_arg_AddPositionalCallback(LogLevelArgHandler);
    }
    else if (strcmp(command, "trace") == 0)
    {
        Command = LOG_CMD_ENABLE_TRACE;

        // Expect a trace keyword next.
        le_arg_AddPositionalCallback(TraceKeywordArgHandler);
    }
    else if (strcmp(command, "stoptrace") == 0)
    {
        Command = LOG_CMD_DISABLE_TRACE;

        // Expect a trace keyword next.
        le_arg_AddPositionalCallback(TraceKeywordArgHandler);
    }
    else if (strcmp(command, "list") == 0)
    {
        Command = LOG_CMD_LIST_COMPONENTS;

        // This command has no parameters and no destination.
    }
    else if (strcmp(command, "forget") == 0)
    {
        Command = LOG_CMD_FORGET_PROCESS;

        // This command has only a process name (or pid) as a parameter.
        le_arg_AddPositionalCallback(ProcessIdArgHandler);
    }
    else
    {
        char errorMsg[100];
        snprintf(errorMsg, sizeof(errorMsg), "Invalid log command (%s)", command);
        ExitWithErrorMsg(errorMsg);
    }
}


//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // The first positional argument must always be a command.
    le_arg_AddPositionalCallback(CommandArgHandler);

    // Remaining arguments will depend on the command.  CommandArgHandler() will add more
    // positional callbacks if necessary.

    // Print help and exit if the "-h" or "--help" options are given.
    le_arg_SetFlagCallback(PrintHelpAndExit, "h", "help");

    le_arg_Scan();

    // Connect to the Log Control Daemon and allocate a message buffer to hold the command.
    le_msg_SessionRef_t sessionRef = ConnectToLogControlDaemon();
    le_msg_MessageRef_t msgRef = le_msg_CreateMsg(sessionRef);
    char* payloadPtr = le_msg_GetPayloadPtr(msgRef);

    // Construct the message.
    payloadPtr[0] = Command;
    payloadPtr[1] = '\0';
    switch (Command)
    {
        case LOG_CMD_SET_LEVEL:
        case LOG_CMD_ENABLE_TRACE:
        case LOG_CMD_DISABLE_TRACE:

            AppendToCommand(msgRef, SessionIdPtr);
            AppendToCommand(msgRef, "/");
            AppendToCommand(msgRef, CommandParamPtr);

            break;

        case LOG_CMD_LIST_COMPONENTS:

            // This one has no arguments.

            break;

        case LOG_CMD_FORGET_PROCESS:

            AppendToCommand(msgRef, CommandParamPtr);

            break;
    }

    // Send the command and wait for messages from the Log Control Daemon.  When the Log Control
    // Daemon has finished executing the command, it will close the IPC session, resulting in a
    // call to SessionCloseHandler().
    le_msg_Send(msgRef);
}
