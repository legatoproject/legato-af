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
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
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
 * Inspection function to use.
 **/
//--------------------------------------------------------------------------------------------------
static InspectFunc_t InspectFunc;


//--------------------------------------------------------------------------------------------------
/**
 * true = follow (periodically update the output until the program is killed with SIGINT or
 *        something).
 **/
//--------------------------------------------------------------------------------------------------
static bool IsFollowing = false;


//--------------------------------------------------------------------------------------------------
/**
 * Time between periodic updates when in "following" mode.
 **/
//--------------------------------------------------------------------------------------------------
static le_clk_Time_t RefreshInterval = {.sec = DEFAULT_REFRESH_INTERVAL, .usec = 0};


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

    exit(EXIT_SUCCESS);
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
 * Refresh timer handler.
 */
//--------------------------------------------------------------------------------------------------
static void RefreshTimerHandler
(
    le_timer_Ref_t timerRef
)
{
    // Perform the inspection.
    InspectFunc(PidToInspect);
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


//--------------------------------------------------------------------------------------------------
/**
 * Function called by command line argument scanner when the command argument is found.
 **/
//--------------------------------------------------------------------------------------------------
static void CommandArgHandler
(
    const char* command
)
{
    if (strcmp(command, "pools") == 0)
    {
        InspectFunc = InspectMemoryPools;
    }
    else
    {
        fprintf(stderr, "Invalid command '%s'.\n", command);
        exit(EXIT_FAILURE);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Function called by command line argument scanner when the pid argument is found.
 **/
//--------------------------------------------------------------------------------------------------
static void PidArgHandler
(
    const char* pidStr
)
{
    int pid;
    le_result_t result = le_utf8_ParseInt(&pid, pidStr);

    if ((result == LE_OK) && (pid > 0))
    {
        PidToInspect = pid;
    }
    else
    {
        fprintf(stderr, "Invalid PID (%s).\n", pidStr);
        exit(EXIT_FAILURE);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Function called by command line argument scanner when the -f or --interval= option is given.
 **/
//--------------------------------------------------------------------------------------------------
static void FollowOptionCallback
(
    int value
)
{
    if (value <= 0)
    {
        fprintf(stderr,
                "Interval value must be a positive integer. "
                    " Using the default interval %d seconds.\n",
                DEFAULT_REFRESH_INTERVAL);

        value = DEFAULT_REFRESH_INTERVAL;
    }

    RefreshInterval.sec = value;

    IsFollowing = true;
}


//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // The command-line has a command string followed by a PID.
    le_arg_AddPositionalCallback(CommandArgHandler);
    le_arg_AddPositionalCallback(PidArgHandler);

    // --help option causes everything else to be ignored, prints help, and exits.
    le_arg_SetFlagCallback(PrintHelp, NULL, "help");

    // -f option starts "following" (periodic updates until the program is terminated).
    le_arg_SetFlagVar(&IsFollowing, "f", NULL);

    // --interval=N option specifies the update period (implies -f).
    le_arg_SetIntCallback(FollowOptionCallback, NULL, "interval");

    le_arg_Scan();

    // Perform the inspection at least once.
    InspectFunc(PidToInspect);

    if ( IsFollowing )
    {
        // The "follow" option was selected.

        // Set up the refresh timer.
        le_timer_Ref_t refreshTimer = le_timer_Create("RefreshTimer");

        INTERNAL_ERR_IF(le_timer_SetHandler(refreshTimer, RefreshTimerHandler) != LE_OK,
                        "Could not set timer handler.\n");

        INTERNAL_ERR_IF(le_timer_SetRepeat(refreshTimer, 0) != LE_OK,
                        "Could not set repeating timer.\n");

        INTERNAL_ERR_IF(le_timer_SetInterval(refreshTimer, RefreshInterval) != LE_OK,
                        "Could not set refresh time.\n");

        // Start the refresh timer.
        INTERNAL_ERR_IF(le_timer_Start(refreshTimer) != LE_OK,
                        "Could not start refresh timer.\n");
    }
    else
    {
        exit(EXIT_SUCCESS);
    }
}
