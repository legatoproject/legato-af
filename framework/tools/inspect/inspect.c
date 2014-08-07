/** @file inspect.c
 *
 * Legato inspection tool used to inspect Legato structures such as memory pools, timers, threads,
 * mutexes, etc. in running processes.
 *
 * Must be run as root.
 *
 * @todo Only supports memory pools right now.  Add support for other timers, threads, etc.
 *
 * @todo Add inspect by process name.
 *
 * Copyright (C) Sierra Wireless, Inc. 2014. All rights reserved. Use of this work is subject to license.
 */

#include "legato.h"
#include "mem.h"
#include "limit.h"


//--------------------------------------------------------------------------------------------------
/**
 * Default refresh interval in seconds.
 */
//--------------------------------------------------------------------------------------------------
#define DEFAULT_REFRESH_INTERVAL            3


//--------------------------------------------------------------------------------------------------
/**
 * PID of the process to inspect.
 */
//--------------------------------------------------------------------------------------------------
static pid_t PidToInspect = -1;


//--------------------------------------------------------------------------------------------------
/**
 * Prototype for inspection functions.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*InspectFunc_t)
(
    pid_t pidToInspect          // PID of the process to inspect.
);


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
        "    inspect - Inspects the internal structures such as memory pools, timers, etc. of a \n"
        "              Legato process.\n"
        "\n"
        "SYNOPSIS:\n"
        "    inspect pools [OPTIONS] PID\n"
        "\n"
        "DESCRIPTION:\n"
        "    inspect pools              Prints the memory pools usage for the specified process. \n"
        "\n"
        "OPTIONS:\n"
        "    -f\n"
        "        Periodically prints updated information for the process.\n"
        "\n"
        "    --interval=SECONDS\n"
        "        Prints updated information every SECONDS.\n"
        "\n"
        "    --help\n"
        "        Display this help and exit.\n"
        );
}


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
 * Gets the PID of the process to inspect from the command line.  The PID is assumed to be the last
 * argument on the command line.
 *
 * @note On failure an error message will be printed and the calling process will exit.
 *
 * @return
 *      The pid of the process to inspect.
 */
//--------------------------------------------------------------------------------------------------
static pid_t GetPidToInspect
(
    void
)
{
    char argBuf[LIMIT_MAX_ARGS_STR_BYTES];

    // Get the last argument from the command line.
    INTERNAL_ERR_IF(le_arg_GetArg(le_arg_NumArgs() - 1, argBuf, sizeof(argBuf)) != LE_OK,
                    "Wrong number of parameters.");

    // Attempt to convert it to a PID.
    char* endPtr;
    errno = 0;

    pid_t pid = strtol(argBuf, &endPtr, 10);

    if ( (errno == 0) && (argBuf[0] != '\0') && (*endPtr == '\0') && (pid > 0) )
    {
        return pid;
    }

    fprintf(stderr, "Invalid PID.\n");
    exit(EXIT_FAILURE);
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks the command line arguments to see if the specified option was selected.
 *
 * @return
 *      true if the option was selected on the command-line.
 *      false if the option was not selected.
 *
 * @todo
 *      This function should be replaced by a general command-line options utility.
 */
//--------------------------------------------------------------------------------------------------
static bool IsOptionSelected
(
    const char* option
)
{
    size_t numArgs = le_arg_NumArgs();
    size_t i = 0;

    char argBuf[LIMIT_MAX_ARGS_STR_BYTES];

    // Search the list of command line arguments for the specified option.
    for (; i < numArgs; i++)
    {
        INTERNAL_ERR_IF(le_arg_GetArg(i, argBuf, sizeof(argBuf)) != LE_OK,
                        "Wrong number of arguments.");

        char* subStr = strstr(argBuf, option);

        if ( (subStr != NULL) && (subStr == argBuf) )
        {
            return true;
        }
    }

    return false;
}


//--------------------------------------------------------------------------------------------------
/**
 * Returns the integer value of a command-line option.  The command-line option must take the form:
 *
 * option=value
 *
 * The portion before the '=' is considered the option.  The portion after the '=' is considered the
 * value.
 *
 * For example with,
 *
 * --interval=200
 *
 * the option would be the string "--interval" and the value would be the integer 200.
 *
 * @note
 *      This function only supports values that are integers.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NOT_FOUND if the option is not available on the command-line.
 *      LE_FORMAT_ERROR if the option was found but the value was not an integer.
 *
 * @todo
 *      This function should be replaced by a general command-line options utility.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetOptionIntValue
(
    const char* option,     ///< [IN] The option.
    int* valuePtr           ///< [OUT] The storage location for the value.
)
{
    // Make the string to search for.
    char searchStr[LIMIT_MAX_ARGS_STR_BYTES + 1];

    int n = snprintf(searchStr, sizeof(searchStr), "%s=", option);

    INTERNAL_ERR_IF( (n < 0) || (n >= sizeof(searchStr)), "Option string is too long.");

    // Search the list of command line arguments for the specified option.
    size_t numArgs = le_arg_NumArgs();
    size_t i = 0;

    char argBuf[LIMIT_MAX_ARGS_STR_BYTES];

    for (; i < numArgs; i++)
    {
        INTERNAL_ERR_IF(le_arg_GetArg(i, argBuf, sizeof(argBuf)) != LE_OK,
                 "Wrong number of arguments.");

        char* subStr = strstr(argBuf, option);

        if ( (subStr != NULL) && (subStr == argBuf) )
        {
            // The argBuf begins with the searchStr.  The remainder of the argBuf is the value string.
            char* valueStr = argBuf + strlen(searchStr);

            // Attempt to convert the value string to an integer value.
            char* endPtr;
            errno = 0;

            int value = strtol(valueStr, &endPtr, 10);

            if ( (errno == 0) && (valueStr[0] != '\0') && (*endPtr == '\0') )
            {
                *valuePtr = value;
                return LE_OK;
            }
            else
            {
                return LE_FORMAT_ERROR;
            }
        }
    }

    return LE_NOT_FOUND;
}


//--------------------------------------------------------------------------------------------------
/**
 * Refresh timer handler.
 */
//--------------------------------------------------------------------------------------------------
static void RefreshTimerHandler
(
    le_timer_Ref_t timerRef
)
{
    // Get the inspection function to use.
    InspectFunc_t inspectFunc = le_timer_GetContextPtr(timerRef);

    INTERNAL_ERR_IF(inspectFunc == NULL,
                    "Inspection function not set.");

    // Perform the inspection.
    inspectFunc(PidToInspect);
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the refresh timer interval.
 */
//--------------------------------------------------------------------------------------------------
static le_clk_Time_t GetRefreshInterval
(
    void
)
{
    int seconds = DEFAULT_REFRESH_INTERVAL;

    le_result_t result = GetOptionIntValue("--interval", &seconds);

    switch (result)
    {
        case LE_OK:
            if (seconds <= 0)
            {
                fprintf(stderr, "Interval value must be a positive integer.  Using the default interval %d \
seconds.\n", DEFAULT_REFRESH_INTERVAL);

                seconds = DEFAULT_REFRESH_INTERVAL;
            }
            break;

        case LE_NOT_FOUND:
            LE_DEBUG("No interval specified on the command line.  Using the default interval \
%d seconds.", DEFAULT_REFRESH_INTERVAL);
            break;

        case LE_FORMAT_ERROR:
            fprintf(stderr, "Interval value must be an integer.  Using the default interval %d \
seconds.\n", DEFAULT_REFRESH_INTERVAL);
            break;

        default:
            INTERNAL_ERR("Unexpected return code '%d'.", result);
    }

    le_clk_Time_t intervalTime = {.sec = seconds, .usec = 0};
    return intervalTime;
}


//--------------------------------------------------------------------------------------------------
/**
 * Print memory pool information header.  Clears the screen and prints information about the process
 * under inspection.
 *
 * @return
 *      The number of lines printed.
 */
//--------------------------------------------------------------------------------------------------
static int PrintMemPoolHeaderInfo
(
    pid_t pidToInspect          // PID of the process under inspection.
)
{
    int lineCount = 0;

    printf("\n");
    lineCount++;

    // Print title.
    printf("Legato Memory Pools Inspector\n");
    lineCount++;

    // Print title.
    printf("Inspecting process %d\n", pidToInspect);
    lineCount++;

    // Print column headers.
    printf("%10s %10s %10s %10s %10s %10s %10s  %s\n",
           "TOTAL BLKS", "USED BLKS", "MAX USED", "OVERFLOWS", "ALLOCS", "BLK BYTES", "USED BYTES",
           "MEMORY POOL");
    lineCount++;

    return lineCount;
}


//--------------------------------------------------------------------------------------------------
/**
 * Print memory pool information to stdout.
 */
//--------------------------------------------------------------------------------------------------
static void PrintMemPoolInfo
(
    le_mem_PoolRef_t memPool
)
{
    // Get pool stats.
    le_mem_PoolStats_t poolStats;
    le_mem_GetStats(memPool, &poolStats);

    size_t blockSize = le_mem_GetObjectFullSize(memPool);

    // See if it is a sub pool.
    char subPoolStr[] = "(Sub-pool)";
    if (!le_mem_IsSubPool(memPool))
    {
        subPoolStr[0] = '\0';
    }

    // Get the pool name.
    char name[LIMIT_MAX_MEM_POOL_NAME_BYTES];
    INTERNAL_ERR_IF(le_mem_GetName(memPool, name, sizeof(name)) != LE_OK, "Name buffer is too small.");

    printf("%10zu %10zu %10zu %10zu %10"PRIu64" %10zu %10zu  %s%s\n",
           le_mem_GetTotalNumObjs(memPool), poolStats.numBlocksInUse,
           poolStats.maxNumBlocksUsed, poolStats.numOverflows, poolStats.numAllocs,
           blockSize, blockSize*(poolStats.numBlocksInUse),
           name, subPoolStr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Inspects the memory pool usage for the specified process.  Prints the results to stdout.
 */
//--------------------------------------------------------------------------------------------------
static void InspectMemoryPools
(
    pid_t pid           // The process to inspect.
)
{
#define ESCAPE_CHAR         27

    // Create the memory pool iterator.
    le_result_t result;
    mem_Iter_Ref_t memIter = mem_iter_Create(pid, &result);

    if (memIter == NULL)
    {
        if (result == LE_NOT_POSSIBLE)
        {
            fprintf(stderr, "The specified process is not a Legato process.\n");
        }
        else
        {
             fprintf(stderr, "Could not access specified process.\n");
        }
        exit(EXIT_FAILURE);
    }

    // Print header information.
    static int lineCount = 0;

    printf("%c[1G", ESCAPE_CHAR);   // Move cursor to the column 1.
    printf("%c[%dA", ESCAPE_CHAR, lineCount); // Move cursor up to the top of the table.
    printf("%c[0J", ESCAPE_CHAR);    // Clear Screen.

    lineCount = PrintMemPoolHeaderInfo(pid);

    // Iterate through the list of memory pools.
    le_mem_PoolRef_t memPool = mem_iter_GetNextPool(memIter);

    while (memPool != NULL)
    {
        PrintMemPoolInfo(memPool);
        lineCount++;

        memPool = mem_iter_GetNextPool(memIter);
    }

    mem_iter_Delete(memIter);
}


COMPONENT_INIT
{
    if (IsOptionSelected("--help"))
    {
        // User is asking for help.
        PrintHelp();
        exit(EXIT_SUCCESS);
    }

    // See what to inspect.
    InspectFunc_t inspectFunc;

    if (IsOptionSelected("pools"))
    {
        inspectFunc = InspectMemoryPools;
    }
    else
    {
        fprintf(stderr, "Missing required command parameter.\n");
        exit(EXIT_FAILURE);
    }

    PidToInspect = GetPidToInspect();

    // Perform the inspection at least once.
    inspectFunc(PidToInspect);

    if ( IsOptionSelected("-f") || IsOptionSelected("--interval") )
    {
        // The "follow" option was selected.

        // Set up the refresh timer.
        le_timer_Ref_t refreshTimer = le_timer_Create("RefreshTimer");

        INTERNAL_ERR_IF(le_timer_SetHandler(refreshTimer, RefreshTimerHandler) != LE_OK,
                        "Could not set timer handler.\n");

        INTERNAL_ERR_IF(le_timer_SetRepeat(refreshTimer, 0) != LE_OK,
                        "Could not set repeating timer.\n");

        INTERNAL_ERR_IF(le_timer_SetInterval(refreshTimer, GetRefreshInterval()) != LE_OK,
                        "Could not set refresh time.\n");

        // Set the inspection function in the context pointer for the timer.
        INTERNAL_ERR_IF(le_timer_SetContextPtr(refreshTimer, inspectFunc) != LE_OK,
                        "Could not set timer context pointer.\n");

        // Start the refresh timer.
        INTERNAL_ERR_IF(le_timer_Start(refreshTimer) != LE_OK,
                        "Could not start refresh timer.\n");
    }
    else
    {
        exit(EXIT_SUCCESS);
    }
}
