//--------------------------------------------------------------------------------------------------
/**
 * Power manager command line tool
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"

// This program will exit with this exit code when one of the bootReason subcommands is called and
// the given boot reason was not the reason for boot.
#define EXIT_DIFFERENT_BOOT_SOURCE (2)


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
 * Parsed version of the command line parameters which are applicable to the subcommands.
 */
//--------------------------------------------------------------------------------------------------
static union
{
    struct
    {
        uint32_t num;
        le_ulpm_GpioState_t trigger;
    } bootOnGpio;
    struct
    {
        uint32_t timeout;
    } bootOnTimer;
    struct
    {
        uint32_t num;
        uint32_t pollingInterval;
        double bootAboveValue;
        double bootBelowValue;
    } bootOnAdc;
    struct
    {
        uint32_t num;
    } bootReasonGpio;
    struct
    {
        uint32_t num;
    } bootReasonAdc;
} Params;


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
                pmtool bootOn gpio <gpioNum> <trigger>\n\
                pmtool bootOn timer <timeOutVal>\n\
                pmtool bootOn adc <adcNum> <pollingInterval> <bootAboveValue> <bootBelowValue>\n\
                pmtool shutdown\n\
                pmtool bootReason gpio <gpioNum>\n\
                pmtool bootReason timer\n\
                pmtool bootReason adc <adcNum>\n\
                pmtool query\n\
            \n\
            DESCRIPTION:\n\
                pmtool help\n\
                  - Print this help message and exit\n\
            \n\
                pmtool bootOn gpio <gpioNum> <trigger>\n\
                  - Configure specified gpio as boot source, trigger(options: high, low, rising,\n\
                    falling, both, off) specifies the gpio state which should trigger device boot\n\
            \n\
                pmtool bootOn timer <timeOutVal>\n\
                  - Configure the timer boot source, timeOutVal specifies the time interval in\n\
                    seconds to trigger device boot.\n\
            \n\
                pmtool bootOn adc <adcNum> <pollingInterval> <bootAboveValue> <bootBelowValue>\n\
                  - Configure the specified adc as a boot source. The bootBelowValue and\n\
                    bootAboveValue parameters are integer value in milliVolts. If bootAboveValue\n\
                    is less than bootBelowValue, the device will boot if an ADC reading falls\n\
                    between bootAboveValue and bootBelowValue. If bootAboveValue is greater than\n\
                    bootBelowValue, the system will boot if an ADC reading is either above\n\
                    bootAboveValue or below bootBelowValue. The pollingInterval parameter\n\
                    specifies the time in milliseconds between ADC samples.\n\
            \n\
                pmtool shutdown\n\
                  - Initiate shutdown of the device.\n\
            \n\
                pmtool bootReason gpio <gpioNum>\n\
                  - Checks whether specified gpio triggered device boot.\n\
            \n\
                pmtool bootReason timer\n\
                  - Checks whether timer expiry triggered device boot.\n\
            \n\
                pmtool bootReason adc <adcNum>\n\
                  - Checks whether the specified adc triggered device boot.\n\
            \n\
                pmtool query\n\
                  - Query the current ultra-low power manager firmware version.\n\
            \n\
                For all bootReason subcommands, the exit code of the program is 0 if the given\n\
                boot source was the reason the system booted or 2 otherwise.\n\
            ");

    exit(EXIT_SUCCESS);
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the selected GPIO as a boot source.
 */
//--------------------------------------------------------------------------------------------------
static void SetGpioBootSrc
(
    void
)
{
    // Set gpio as boot source.
    if (le_ulpm_BootOnGpio(Params.bootOnGpio.num, Params.bootOnGpio.trigger) == LE_OK)
    {
        printf("SUCCESS!\n");
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
 * Set the timer as a boot source with the timeout in the Params struct.
 */
//--------------------------------------------------------------------------------------------------
static void SetTimerBootSrc
(
    void
)
{
    // Set timer as a boot source.
    if (le_ulpm_BootOnTimer(Params.bootOnTimer.timeout) == LE_OK)
    {
        printf("SUCCESS!\n");
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
 * Set an adc as a boot source.  The parameters of the adc are extracted from the Params struct.
 */
//--------------------------------------------------------------------------------------------------
static void SetAdcBootSrc
(
    void
)
{
    // Set adc as a boot source.
    if (le_ulpm_BootOnAdc(
            Params.bootOnAdc.num,
            Params.bootOnAdc.pollingInterval,
            Params.bootOnAdc.bootAboveValue,
            Params.bootOnAdc.bootBelowValue) == LE_OK)
    {
        printf("SUCCESS!\n");
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
    printf("Boot source gpio%u? ", Params.bootReasonGpio.num);
    if (le_bootReason_WasGpio(Params.bootReasonGpio.num))
    {
        printf("Yes\n");
        exit(EXIT_SUCCESS);
    }
    else
    {
        printf("No\n");
        exit(EXIT_DIFFERENT_BOOT_SOURCE);
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
    printf("Boot source timer? ");
    if (le_bootReason_WasTimer())
    {
        printf("Yes\n");
        exit(EXIT_SUCCESS);
    }
    else
    {
        printf("No\n");
        exit(EXIT_DIFFERENT_BOOT_SOURCE);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks whether device booted due to an adc reading.
 */
//--------------------------------------------------------------------------------------------------
static void CheckAdcBootSource
(
    void
)
{
    printf("Boot source adc%u? ", Params.bootReasonAdc.num);
    if (le_bootReason_WasAdc(Params.bootReasonAdc.num))
    {
        printf("Yes\n");
        exit(EXIT_SUCCESS);
    }
    else
    {
        printf("No\n");
        exit(EXIT_DIFFERENT_BOOT_SOURCE);
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
        printf("Ultra Low Power Manager Firmware Version: %s\n", version);
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
        printf("Initiated shutdown of MDM\n");
        exit(EXIT_SUCCESS);
    }
    else
    {
        fprintf(stderr, "Can't initiate shutdown of MDM\n");
        exit(EXIT_FAILURE);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse a 32-bit unsigned integer from a null terminated input string.
 *
 * This function will reject the input if it cannot be represented in a 32 bit unsigned integer or
 * if it cannot be parsed as an integer at all.
 *
 * @return
 *      true if a value was parsed or false otherwise.
 */
//--------------------------------------------------------------------------------------------------
static bool ParseU32
(
    const char* str, ///< [IN] Null terminated string to try to parse
    uint32_t* value  ///< [OUT] Location to write parsed value into
)
{
    if (isspace(*str))
    {
        // Explicitly reject input with leading whitespace that would otherwise be accepted by
        // strtol
        return false;
    }

    char* endOfParse = NULL;
    errno = 0;
    // Need to use a long long int to ensure that the variable is capable of representing the full
    // range of an unsigned 32-bit integer.
    long long int parsedVal = strtoll(str, &endOfParse, 10);
    if (errno)
    {
        // strtoll failed
        return false;
    }

    if ((endOfParse == str) || (*endOfParse != '\0'))
    {
        // Input string was empty or whole string wasn't consumed
        return false;
    }

    if ((parsedVal < 0) || (parsedVal > UINT32_MAX))
    {
        // Parsed value can't be stored in a uint32_t
        return false;
    }

    *value = parsedVal;
    return true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse a double from a null terminated input string.
 *
 * @return
 *      true if a value was parsed or false otherwise
 */
//--------------------------------------------------------------------------------------------------
static bool ParseDouble
(
    const char* str,
    double* value
)
{
    if (isspace(*str))
    {
        // Explicitly reject input with leading whitespace that would otherwise be accepted by
        // strtol
        return false;
    }

    char* endOfParse = NULL;
    errno = 0;
    double parsedVal = strtod(str, &endOfParse);
    if (errno)
    {
        // strtod failed
        return false;
    }

    if ((endOfParse == str) || (*endOfParse != '\0'))
    {
        // Input string was empty or whole string wasn't consumed
        return false;
    }

    *value = parsedVal;
    return true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse the gpio number argument of the "pmtool bootOn gpio <number> <trigger>" command.
 */
//--------------------------------------------------------------------------------------------------
static void PosArgCbSetGpioNum
(
    const char* arg  ///< [IN] Null terminated argument string
)
{
    if (!ParseU32(arg, &Params.bootOnGpio.num))
    {
        fprintf(stderr, "Couldn't parse a gpio number from \"%s\".\n", arg);
        exit(EXIT_FAILURE);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse the trigger argument of the "pmtool bootOn gpio <number> <trigger>" command.
 */
//--------------------------------------------------------------------------------------------------
static void PosArgCbSetGpioTrigger
(
    const char* arg  ///< [IN] Null terminated argument string
)
{
    if (strcmp(arg, "high") == 0)
    {
        Params.bootOnGpio.trigger = LE_ULPM_GPIO_HIGH;
    }
    else if (strcmp(arg, "low") == 0)
    {
        Params.bootOnGpio.trigger = LE_ULPM_GPIO_LOW;
    }
    else if (strcmp(arg, "rising") == 0)
    {
        Params.bootOnGpio.trigger = LE_ULPM_GPIO_RISING;
    }
    else if (strcmp(arg, "falling") == 0)
    {
        Params.bootOnGpio.trigger = LE_ULPM_GPIO_FALLING;
    }
    else if (strcmp(arg, "both") == 0)
    {
        Params.bootOnGpio.trigger = LE_ULPM_GPIO_BOTH;
    }
    else if (strcmp(arg, "off") == 0)
    {
        Params.bootOnGpio.trigger = LE_ULPM_GPIO_OFF;
    }
    else
    {
        fprintf(stderr, "Bad trigger option: %s\n", arg);
        exit(EXIT_FAILURE);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse the timeout argument of the "pmtool bootOn timer <timeout>" command.
 */
//--------------------------------------------------------------------------------------------------
static void PosArgCbSetTimerTimeout
(
    const char* arg  ///< [IN] Null terminated argument string
)
{
    if (!ParseU32(arg, &Params.bootOnTimer.timeout))
    {
        fprintf(stderr, "Couldn't parse a timeout value from \"%s\".\n", arg);
        exit(EXIT_FAILURE);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse the adc number argument of the "pmtool bootOn adc <number> <pollInterval> <bootAboveValue>
 * <bootBelowValue>" command.
 */
//--------------------------------------------------------------------------------------------------
static void PosArgCbSetAdcNum
(
    const char* arg  ///< [IN] Null terminated argument string
)
{
    if (!ParseU32(arg, &Params.bootOnAdc.num))
    {
        fprintf(stderr, "Couldn't parse an adc number from \"%s\".\n", arg);
        exit(EXIT_FAILURE);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse the pollInterval argument of the "pmtool bootOn adc <number> <pollInterval>
 * <bootAboveValue> <bootBelowValue>" command.
 */
//--------------------------------------------------------------------------------------------------
static void PosArgCbSetAdcPollingInterval
(
    const char* arg  ///< [IN] Null terminated argument string
)
{
    if (!ParseU32(arg, &Params.bootOnAdc.pollingInterval))
    {
        fprintf(stderr, "Couldn't parse pollingInterval from \"%s\".\n", arg);
        exit(EXIT_FAILURE);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse the bootAboveValue argument of the "pmtool bootOn adc <number> <pollInterval>
 * <bootAboveValue> <bootBelowValue>" command.
 */
//--------------------------------------------------------------------------------------------------
static void PosArgCbSetAdcBootAboveValue
(
    const char* arg  ///< [IN] Null terminated argument string
)
{
    if (!ParseDouble(arg, &Params.bootOnAdc.bootAboveValue))
    {
        fprintf(stderr, "Couldn't parse bootAboveValue from \"%s\".\n", arg);
        exit(EXIT_FAILURE);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse the bootBelowValue argument of the "pmtool bootOn adc <number> <pollInterval>
 * <bootAboveValue> <bootBelowValue>" command.
 */
//--------------------------------------------------------------------------------------------------
static void PosArgCbSetAdcBootBelowValue
(
    const char* arg
)
{
    if (!ParseDouble(arg, &Params.bootOnAdc.bootBelowValue))
    {
        fprintf(stderr, "Couldn't parse bootBelowValue from \"%s\".\n", arg);
        exit(EXIT_FAILURE);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse the gpioNum argument of the "pmtool bootReason gpio <gpioNum>" command.
 */
//--------------------------------------------------------------------------------------------------
static void PosArgCbCheckGpioNum
(
    const char* arg
)
{
    if (!ParseU32(arg, &Params.bootReasonGpio.num))
    {
        fprintf(stderr, "Couldn't parse a gpio number from \"%s\".\n", arg);
        exit(EXIT_FAILURE);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse the adcNumNum argument of the "pmtool bootReason adc <adcNum>" command.
 */
//--------------------------------------------------------------------------------------------------
static void PosArgCbCheckAdcNum
(
    const char* arg
)
{
    if (!ParseU32(arg, &Params.bootReasonAdc.num))
    {
        fprintf(stderr, "Couldn't parse an adc number from \"%s\".\n", arg);
        exit(EXIT_FAILURE);
    }
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
        le_arg_AddPositionalCallback(PosArgCbSetGpioNum);
        le_arg_AddPositionalCallback(PosArgCbSetGpioTrigger);
    }
    else if (strcmp(argPtr, "timer") == 0)
    {
        CommandHandler = SetTimerBootSrc;
        le_arg_AddPositionalCallback(PosArgCbSetTimerTimeout);
    }
    else if (strcmp(argPtr, "adc") == 0)
    {
        CommandHandler = SetAdcBootSrc;
        le_arg_AddPositionalCallback(PosArgCbSetAdcNum);
        le_arg_AddPositionalCallback(PosArgCbSetAdcPollingInterval);
        le_arg_AddPositionalCallback(PosArgCbSetAdcBootAboveValue);
        le_arg_AddPositionalCallback(PosArgCbSetAdcBootBelowValue);
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
        le_arg_AddPositionalCallback(PosArgCbCheckGpioNum);
    }
    else if (strcmp(argPtr, "timer") == 0)
    {
        CommandHandler = CheckTimerBootSource;
    }
    else if (strcmp(argPtr, "adc") == 0)
    {
        CommandHandler = CheckAdcBootSource;
        le_arg_AddPositionalCallback(PosArgCbCheckAdcNum);
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
