/** @file uartMode.c
 *
 * UART Mode Configuration.
 *
 * This module can be used to set the UART ports' mode.
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "pa_uartMode.h"


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
 * Command argument.
 */
//--------------------------------------------------------------------------------------------------
static const char* CmdPtr;


//--------------------------------------------------------------------------------------------------
/**
 * Uart number argument.
 */
//--------------------------------------------------------------------------------------------------
static int UartNum;


//--------------------------------------------------------------------------------------------------
/**
 * Prints help to stdout and exits.
 */
//--------------------------------------------------------------------------------------------------
static void PrintHelp
(
    void
)
{
    puts(
        "NAME:\n"
        "    uartMode - Used to set the mode for either UART1 or UART2.  Requires a reboot for new\n"
        "               modes to take affect.\n"
        "\n"
        "SYNOPSIS:\n"
        "    uartMode --help | -h\n"
        "    uartMode get <uartNum>\n"
        "    uartMode set <uartNum> <mode>\n"
        "\n"
        "DESCRIPTION:\n"
        "    uartMode --help | -h\n"
        "       Display this help and exit.\n"
        "\n"
        "    uartMode get <uartNum>\n"
        "       Gets the mode for the specified UART.  uartNum can be either 1 or 2.\n"
        "\n"
        "    uartMode set <uartNum> <mode>\n"
        "       Sets the mode for the specified UART.  uartNum can be either 1 or 2.\n"
        "       mode can be:\n"
        "           'disable' = Disable UART.\n"
        "           'atCmds' = AT Command services (not valid for UART2).\n"
        "           'diag' = Diagnostic Message service.\n"
        "           'nmea' = NMEA service.\n"
        "           'console' = Linux /dev/console.\n"
        "           'app' = Linux application usage.\n"
        "\n"
        );

    exit(EXIT_SUCCESS);
}


//--------------------------------------------------------------------------------------------------
/**
 * Converts a mode string to a mode value.
 **/
//--------------------------------------------------------------------------------------------------
static pa_uartMode_Mode_t ModeStrToVal
(
    const char* modeStr
)
{
    if (strcmp(modeStr, "disable") == 0)
    {
        return UART_DISABLED;
    }
    if (strcmp(modeStr, "atCmds") == 0)
    {
        return UART_AT_CMD;
    }
    if (strcmp(modeStr, "diag") == 0)
    {
        return UART_DIAG_MSG;
    }
    if (strcmp(modeStr, "nmea") == 0)
    {
        return UART_NMEA;
    }
    if (strcmp(modeStr, "console") == 0)
    {
        return UART_LINUX_CONSOLE;
    }
    if (strcmp(modeStr, "app") == 0)
    {
        return UART_LINUX_APP;
    }

    fprintf(stderr, "Unrecognized mode '%s'.\n", modeStr);
    exit(EXIT_FAILURE);
}


//--------------------------------------------------------------------------------------------------
/**
 * Set UART mode.
 **/
//--------------------------------------------------------------------------------------------------
static void SetUartMode
(
    int uartNum,                    ///< [IN] 1 for UART1, 2 for UART2.
    const char* modeStr             ///< [IN] Mode value to set.
)
{
    pa_uartMode_Mode_t mode = ModeStrToVal(modeStr);

    INTERNAL_ERR_IF(pa_uartMode_Set(uartNum, mode) != LE_OK, "Could not set uart mode.");

    printf("UART%d will be set to '%s' after the next reboot.\n", uartNum, modeStr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Prints UART mode.
 **/
//--------------------------------------------------------------------------------------------------
static void PrintUartMode
(
    int uartNum             ///<[IN] 1 for UART1, 2 for UART2.
)
{
    // Get the mode.
    pa_uartMode_Mode_t uartMode;

    INTERNAL_ERR_IF(pa_uartMode_Get(uartNum, &uartMode) != LE_OK, "Could not get uart mode.");

    // Print the mode
    switch (uartMode)
    {
        case UART_DISABLED:
            printf("UART%d is disabled.\n", uartNum);
            break;

        case UART_AT_CMD:
            printf("UART%d is being used for AT Commands.\n", uartNum);
            break;

        case UART_DIAG_MSG:
            printf("UART%d is being used for Diagnostic Message Service.\n", uartNum);
            break;

        case UART_NMEA:
            printf("UART%d is being used for NMEA Service.\n", uartNum);
            break;

        case UART_LINUX_CONSOLE:
            printf("UART%d is being used for the /dev/console.\n", uartNum);
            break;

        case UART_LINUX_APP:
            printf("UART%d is available for use by Linux applications.\n", uartNum);
            break;

        default:
            INTERNAL_ERR("Unrecognized mode value %d for UART%d.", uartMode, uartNum);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Handle specified mode strings.
 **/
//--------------------------------------------------------------------------------------------------
static void ModeArgHandler
(
    const char* modeStr
)
{
    SetUartMode(UartNum, modeStr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Handle specified UART numbers.
 **/
//--------------------------------------------------------------------------------------------------
static void UartNumArgHandler
(
    const char* uartNumStr
)
{
    if (strcmp(uartNumStr, "1") == 0)
    {
        UartNum = 1;
    }
    else if (strcmp(uartNumStr, "2") == 0)
    {
        UartNum = 2;
    }
    else
    {
        fprintf(stderr, "Unsupported UART number.\n");
        exit(EXIT_FAILURE);
    }

    if (strcmp(CmdPtr, "get") == 0)
    {
        PrintUartMode(UartNum);
    }
    else
    {
        // Get the mode string.
        le_arg_AddPositionalCallback(ModeArgHandler);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Handle specified commands.
 **/
//--------------------------------------------------------------------------------------------------
static void CommandArgHandler
(
    const char* command
)
{
    CmdPtr = command;

    if ( (strcmp(CmdPtr, "get") != 0) &&
         (strcmp(CmdPtr, "set") != 0) )
    {
        fprintf(stderr, "Invalid command '%s'.\n", command);
        exit(EXIT_FAILURE);
    }

    // Get the uart number.
    le_arg_AddPositionalCallback(UartNumArgHandler);
}


//--------------------------------------------------------------------------------------------------
/**
 * Initialization function.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // -h, --help option causes everything else to be ignored, prints help, and exits.
    le_arg_SetFlagCallback(PrintHelp, "h", "help");

    // The command-line has a command string followed by a uart number.
    le_arg_AddPositionalCallback(CommandArgHandler);

    le_arg_Scan();

    exit(EXIT_SUCCESS);
}
