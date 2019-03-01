#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
* Default time in seconds to monitor a given sensor's temperature
*
*/
//--------------------------------------------------------------------------------------------------
#define DEFAULT_MONITOR_TIME 15

//--------------------------------------------------------------------------------------------------
/**
* Mutex used to synchronize the temperature printing
*
*/
//--------------------------------------------------------------------------------------------------
#define mutexName "myMutex"
static le_mutex_Ref_t mut;

//--------------------------------------------------------------------------------------------------
/**
* Name of the client program
*
*/
//--------------------------------------------------------------------------------------------------
static const char* ProgramName;

//--------------------------------------------------------------------------------------------------
/**
* Reference to handler function for updating temperature value
*
*/
//--------------------------------------------------------------------------------------------------
static temperature_UpdatedValueHandlerRef_t UpdatedValueHandler;

//--------------------------------------------------------------------------------------------------
/**
* Input options
*
*/
//--------------------------------------------------------------------------------------------------
static int MonitorTime;

//--------------------------------------------------------------------------------------------------
/**
* Prints help to stdout
*
*/
//--------------------------------------------------------------------------------------------------
static void PrintHelp
(
    void
)
{
    puts(
        "NAME\n"
        "        psClient - Demonstrating Layered Publish Subscribe by performing sensor temperature monitoring.\n"
        "\n"
        "SYNOPSIS\n"
        "        psClient [OPTION]... COMMAND [Sensor Name]\n"
        "        psClient -h\n"
        "        psClient --help\n"
        "\n"
        "COMMANDS\n"
        "       amplifier\n"
        "               Monitor's Power Amplifier temperature and displays updated results in Fahrenheit every\n"
        "                second for a default of 15 seconds (can be changed via -t option).\n"
        "\n"
        "       controller\n"
        "                  Monitor's Power Controller temperature and displays updated results in Fahrenheit every\n"
        "                second for a default of 15 seconds (can be changed via -t option).\n"
        "\n"
        "OPTIONS\n"
        "       -t N\n"
        "       --time=N\n"
        "               The monitor time in seconds is specified using this option. If not specified, it will\n"
        "                be set to 15 seconds by default.\n"
        );

    exit(EXIT_SUCCESS);
}

//-------------------------------------------------------------------------------------------------
/**
* Handler function for displaying the updated temperature in Fahrenheit on the client side.
*/
//-------------------------------------------------------------------------------------------------
void UpdatedValueHandlerFunc
(
    int32_t value,        ///< temperature in F
    void* contextPtr      ///< Context pointer
)
{
    mut = le_mutex_CreateNonRecursive("myMutex");
    static int resCounter;
    le_mutex_Lock(mut);
    fprintf(stdout, "Temperature: %d F\n", value);
    if (resCounter == MonitorTime-1)
    {
        fprintf(stdout, "\nMonitor Complete!\n");
        resCounter = 0;
        temperature_RemoveUpdatedValueHandler(UpdatedValueHandler);
        exit(EXIT_SUCCESS);
    }
    resCounter++;
    le_mutex_Unlock(mut);
    le_mutex_Delete(mut);
}

//--------------------------------------------------------------------------------------------------
/**
* Process commands for psClient
*
*/
//--------------------------------------------------------------------------------------------------
static void CommandHandler
(
    const char * command   ///< [IN] Command
)
{
    if (MonitorTime <= 0)
    {
        MonitorTime = DEFAULT_MONITOR_TIME;
    }

    if (strcmp(command, "amplifier") == 0)
    {
        temperature_MonitorAmpTemp(MonitorTime);
    }
    else if (strcmp(command, "controller") == 0)
    {
        temperature_MonitorCtrlTemp(MonitorTime);
    }
    else
    {
        fprintf(stderr, "Unknown command.\n");
        fprintf(stderr, "Try '%s --help'.\n", ProgramName);
        exit(EXIT_FAILURE);
    }
}

//--------------------------------------------------------------------------------------------------
/**
* Temperature monitoring client side.
* Registers a handler function to monitor sensor temperatures.
* User can specify which sensor and for how long they wish to monitor it.
*
*/
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("psClient Started");
    UpdatedValueHandler = temperature_AddUpdatedValueHandler (UpdatedValueHandlerFunc, NULL);

    ProgramName = le_arg_GetProgramName();
    if (ProgramName == NULL)
    {
        ProgramName = "psClient";
    }

    le_arg_SetFlagCallback(PrintHelp, "h", "help");

    le_arg_SetIntVar(&MonitorTime, "t", "time");

    le_arg_AllowLessPositionalArgsThanCallbacks();
    le_arg_AddPositionalCallback(CommandHandler);
    le_arg_Scan();
}