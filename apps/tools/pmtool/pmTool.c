//--------------------------------------------------------------------------------------------------
/**
 * Power manager command line tool
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"


//--------------------------------------------------------------------------------------------------
/**
 * Prototype for command handler functions.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*cmdHandlerFunc_t)
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * The command handler function.
 */
//--------------------------------------------------------------------------------------------------
static cmdHandlerFunc_t CommandHandler = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * The trigger option for boot source.
 */
//--------------------------------------------------------------------------------------------------
static const char* TriggerOptionPtr = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Flag to indicate whether the size of the entry should be listed.
 */
//--------------------------------------------------------------------------------------------------
static uint32_t BootSrcVal = 0;


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
    puts("\
            NAME:\n\
                pmtool - Used to set different option of power manager\n\
            \n\
            SYNOPSIS:\n\
                pmtool --help\n\
                pmtool bootOn gpio  <gpioNum> <triggerOption>\n\
                pmtool bootOn timer <timeOutVal>\n\
                pmtool shutdown\n\
                pmtool bootReason timer\n\
                pmtool bootReason gpio <gpioNum>\n\
                pmtool query\n\
            \n\
            DESCRIPTION:\n\
                pmtool help\n\
                  - Print this help message and exit\n\
            \n\
                pmtool bootOn gpio <gpioNum> <triggerOption>\n\
                  - Configure specified gpio as boot source, triggerOption(options: high, low, rising,\n\
                    falling, both, off)\n\
                    specifies the gpio state which should trigger device boot\n\
            \n\
                pmtool bootOn timer <timeOutVal>\n\
                  - Configure specified timer as boot source, timeOutVal specifies the time interval\n\
                    seconds to trigger device boot\n\
            \n\
                pmtool shutdown\n\
                  - Initiate shutdown of the device\n\
            \n\
                pmtool bootReason gpio  <gpioNum>\n\
                  - Checks whether specified gpio triggers device boot\n\
            \n\
                pmtool bootReason timer\n\
                  - Checks whether timer expiry triggers device boot\n\
            \n\
                pmtool query\n\
                  - Query the current ultra-low power manager firmware version.\n\
            ");

    exit(EXIT_FAILURE);
}


//--------------------------------------------------------------------------------------------------
/**
 * Function to set gpio as boot source.
 */
//--------------------------------------------------------------------------------------------------
static void SetGpioBootSrc
(
    void
)
{
    le_ulpm_GpioState_t gpioState;

    if (strcmp(TriggerOptionPtr, "high") == 0)
    {
        gpioState = LE_ULPM_GPIO_HIGH;
    }
    else if (strcmp(TriggerOptionPtr, "low") == 0)
    {
        gpioState = LE_ULPM_GPIO_LOW;
    }
    else if (strcmp(TriggerOptionPtr, "rising") == 0)
    {
        gpioState = LE_ULPM_GPIO_RISING;
    }
    else if (strcmp(TriggerOptionPtr, "falling") == 0)
    {
        gpioState = LE_ULPM_GPIO_FALLING;
    }
    else if (strcmp(TriggerOptionPtr, "both") == 0)
    {
        gpioState = LE_ULPM_GPIO_BOTH;
    }
    else if (strcmp(TriggerOptionPtr, "off") == 0)
    {
        gpioState = LE_ULPM_GPIO_OFF;
    }
    else
    {
        fprintf(stderr, "Bad trigger option: %s\n", TriggerOptionPtr);
        exit(EXIT_FAILURE);
    }

    // Set gpio as boot source. BootSrcVal contains target gpio number.
    if (le_ulpm_BootOnGpio(BootSrcVal, gpioState) == LE_OK)
    {
        fprintf(stdout, "SUCCESS!\n");
        exit(EXIT_SUCCESS);
    }
    else
    {
        fprintf(stderr, "FAILED.\n");
        exit(EXIT_FAILURE);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Function to set timer as boot source.
 */
//--------------------------------------------------------------------------------------------------
static void SetTimerBootSrc
(
    void
)
{
    // Set timer as a boot source. BootSrcVal contains timeout value.
    if (le_ulpm_BootOnTimer(BootSrcVal) == LE_OK)
    {
        fprintf(stdout, "SUCCESS!\n");
        exit(EXIT_SUCCESS);
    }
    else
    {
        fprintf(stderr, "FAILED.\n");
        exit(EXIT_FAILURE);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks whether device booted due to gpio state change.
 */
//--------------------------------------------------------------------------------------------------
static void CheckGpioBootSource
(
    void
)
{
    if (le_bootReason_WasGpio(BootSrcVal))
    {
        fprintf(stdout, "Boot source gpio%u? Yes.\n", BootSrcVal);
        exit(EXIT_SUCCESS);
    }
    else
    {
        fprintf(stderr, "Boot source gpio%u? No.\n", BootSrcVal);
        exit(EXIT_FAILURE);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks whether device booted due to timer expiry.
 */
//--------------------------------------------------------------------------------------------------
static void CheckTimerBootSource
(
    void
)
{
    if (le_bootReason_WasTimer())
    {
        fprintf(stdout, "Boot source timer? Yes.\n");
        exit(EXIT_SUCCESS);
    }
    else
    {
        fprintf(stderr, "Boot source timer? No.\n");
        exit(EXIT_FAILURE);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Function to get ultra low power manager firmware version.
 */
//--------------------------------------------------------------------------------------------------
static void QueryVersion
(
    void
)
{
    char version[LE_ULPM_MAX_VERS_LEN+1];

    if ( le_ulpm_GetFirmwareVersion(version, sizeof(version)) == LE_OK )
    {
        fprintf(stdout, "\nUltra Low Power Manager Firmware Version: %s\n", version);
        exit(EXIT_SUCCESS);
    }
    else
    {
        fprintf(stderr, "Failed to get Firmware version\n");
        exit(EXIT_FAILURE);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Initiate shutdown of MDM.
 */
//--------------------------------------------------------------------------------------------------
static void ShutDown
(
    void
)
{
    if (le_ulpm_ShutDown() == LE_OK)
    {
        fprintf(stdout, "Initiated shutdown of MDM\n");
        exit(EXIT_FAILURE);
    }
    else
    {
        fprintf(stderr, "Can't initiate shutdown of MDM\n");
        exit(EXIT_FAILURE);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Callback function to get any numerical value associated to boot source.
 */
//--------------------------------------------------------------------------------------------------
static void BootSourceValue
(
    const char* argPtr                  ///< [IN] Command-line argument.
)
{
    char *endPtr;
    BootSrcVal = strtol(argPtr, &endPtr, 10);

    if (*endPtr != '\0')
    {
        fprintf(stderr, "Bad parameter: %s. This should be a decimal number.\n", argPtr);
        exit(EXIT_FAILURE);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Callback function to get boot source trigger option.
 */
//--------------------------------------------------------------------------------------------------
static void BootSourceTrigger
(
    const char* argPtr                  ///< [IN] Command-line argument.
)
{
    TriggerOptionPtr = argPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Callback function to set boot source depending on command line arguments.
 */
//--------------------------------------------------------------------------------------------------
static void SetBootSource
(
    const char* argPtr                  ///< [IN] Command-line argument.
)
{
    if (strcmp(argPtr, "gpio") == 0)
    {
        CommandHandler = SetGpioBootSrc;
        le_arg_AddPositionalCallback(BootSourceValue);
        le_arg_AddPositionalCallback(BootSourceTrigger);
    }
    else if (strcmp(argPtr, "timer") == 0)
    {
        CommandHandler = SetTimerBootSrc;
        le_arg_AddPositionalCallback(BootSourceValue);
    }
    else
    {
        fprintf(stderr, "Bad boot source: %s\n", argPtr);
        exit(EXIT_FAILURE);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Callback function to check boot source depending on command line arguments.
 */
//--------------------------------------------------------------------------------------------------
static void CheckBootSource
(
    const char* argPtr                  ///< [IN] Command-line argument.
)
{
    if (strcmp(argPtr, "gpio") == 0)
    {
        CommandHandler = CheckGpioBootSource;
        le_arg_AddPositionalCallback(BootSourceValue);
    }
    else if (strcmp(argPtr, "timer") == 0)
    {
        CommandHandler = CheckTimerBootSource;
    }
    else
    {
        fprintf(stderr, "Bad boot source: %s\n", argPtr);
        exit(EXIT_FAILURE);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the command handler to call depending on which command was specified on the command-line.
 */
//--------------------------------------------------------------------------------------------------
static void SetCommandHandler
(
    const char* argPtr                  ///< [IN] Command-line argument.
)
{
    if (strcmp(argPtr, "bootOn") == 0)
    {
        le_arg_AddPositionalCallback(SetBootSource);
    }
    else if (strcmp(argPtr, "bootReason") == 0)
    {
        le_arg_AddPositionalCallback(CheckBootSource);
    }
    else if (strcmp(argPtr, "shutdown") == 0)
    {
        CommandHandler = ShutDown;
    }
    else if (strcmp(argPtr, "query") == 0)
    {
        CommandHandler = QueryVersion;
    }
    else
    {
        fprintf(stderr, "Unknown command: %s.\n", argPtr);
        exit(EXIT_FAILURE);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Program init
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // Setup command-line argument handling.
    le_arg_SetFlagCallback(PrintHelp, "h", "help");

    le_arg_AddPositionalCallback(SetCommandHandler);

    le_arg_Scan();

    // Call the actual command handler.
    CommandHandler();

    // Should not come here.
    exit(EXIT_FAILURE);
}

