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
 * Default retry interval in microseconds.
 */
//--------------------------------------------------------------------------------------------------
#define DEFAULT_RETRY_INTERVAL              500000


//--------------------------------------------------------------------------------------------------
/**
 * Maximum length of a prompt message to the user.
 */
//--------------------------------------------------------------------------------------------------
#define USER_PROMPT_MSG_BYTES               250


//--------------------------------------------------------------------------------------------------
/**
 * Refresh timer for the interval and follow options
 */
//--------------------------------------------------------------------------------------------------
static le_timer_Ref_t refreshTimer = NULL;


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
 * Flags indicating how an inspection ended.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    INSPECT_SUCCESS     = 1,    ///< inspection completed without interruption or error.
    INSPECT_INTERRUPTED = 2,    ///< inspection was interrupted due to list changes.
    INSPECT_ERROR       = 3     ///< inspection was interrupted due to memory read error.
}
InspectEndStatus_t;


//--------------------------------------------------------------------------------------------------
/**
 * Definition of data relevant to what should happen when an inspection ends.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    InspectEndStatus_t endStatus;               ///< how an inspection ended.
    char userPromptMsg[USER_PROMPT_MSG_BYTES];  ///< based on end status, message to show the user.
    le_clk_Time_t refreshInterval;              ///< based on end status, refresh interval (seconds)
}
InspectEndHandlingData_t;


//--------------------------------------------------------------------------------------------------
/**
 * A table containing data relevant to what should happen when a memory pool inspection ends.
 */
//--------------------------------------------------------------------------------------------------
static InspectEndHandlingData_t InspectMemPoolEndHandlingTbl[] =
{
    {
        INSPECT_SUCCESS,
        ">>> End of List <<<\n",
        {.sec = DEFAULT_REFRESH_INTERVAL, .usec = 0}
    },

    {
        INSPECT_INTERRUPTED,
        ">>> Detected changes in List of Memory Pools. Stopping inspection. <<<\n",
        {.sec = 0, .usec = DEFAULT_RETRY_INTERVAL}
    },

    {
        INSPECT_ERROR,
        ">>> Error reading the process under inspection. Stopping inspection. <<<\n",
        {0}
    },

    // NOTE that the last element must be {0} to denote the end of the table
    {
        0
    }
};
// TODO: Create other similar tables for timer, thread, etc.


//--------------------------------------------------------------------------------------------------
/**
 * Inspection end handing table to use
 */
//--------------------------------------------------------------------------------------------------
static InspectEndHandlingData_t* InspectEndHandlingTbl;


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
    const char* subPoolStr = "(Sub-pool)";
    if (!le_mem_IsSubPool(memPool))
    {
        subPoolStr = "";
    }

    // Get the pool name.
    char name[LIMIT_MAX_COMPONENT_NAME_LEN + 1 + LIMIT_MAX_MEM_POOL_NAME_BYTES];
    INTERNAL_ERR_IF(le_mem_GetName(memPool, name, sizeof(name)) != LE_OK, "Name buffer is too small.");

    printf("%10zu %10zu %10zu %10zu %10"PRIu64" %10zu %10zu  %s%s\n",
           le_mem_GetTotalNumObjs(memPool), poolStats.numBlocksInUse,
           poolStats.maxNumBlocksUsed, poolStats.numOverflows, poolStats.numAllocs,
           blockSize, blockSize*(poolStats.numBlocksInUse),
           name, subPoolStr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Performs actions when an inspection ends depending on how it ends.
 */
//--------------------------------------------------------------------------------------------------
static void InspectEndHandling
(
    InspectEndStatus_t endStatus,
    int* lineCount
)
{
    int i = 0;
    while (InspectEndHandlingTbl[i].endStatus != 0)
    {
        if (InspectEndHandlingTbl[i].endStatus == endStatus)
        {

            // TODO: should I print error to stderr if end status is INSPECT_ERROR? see the todo
            // in InpsectMemoryPools
            printf("%s", InspectEndHandlingTbl[i].userPromptMsg);
            (*lineCount)++;

            // The last line of the current run of inspection has finished, so it's a good place to
            // flush the write buffer on stdout. This is important for redirecting the output to a
            // log file, so that the end of an inspection is written to the log as soon as it happens.
            fflush(stdout);

            // TODO: consider if LE_FATAL is appropriate here.  Would the clean-up routine be missed?
            // set up the timer only if the interval is not 0.
            if ((IsFollowing) &&
                ((InspectEndHandlingTbl[i].refreshInterval.sec != 0) ||
                 (InspectEndHandlingTbl[i].refreshInterval.usec != 0)))
            {
                // Set up the refresh timer.
                refreshTimer = le_timer_Create("RefreshTimer");

                INTERNAL_ERR_IF(le_timer_SetHandler(refreshTimer, RefreshTimerHandler) != LE_OK,
                                "Could not set timer handler.\n");

                INTERNAL_ERR_IF(le_timer_SetInterval(refreshTimer,
                                                     InspectEndHandlingTbl[i].refreshInterval) != LE_OK,
                                "Could not set refresh time.\n");

                // Start the refresh timer.
                INTERNAL_ERR_IF(le_timer_Start(refreshTimer) != LE_OK,
                                "Could not start refresh timer.\n");
            }

            break;
        }

        i++;
    }
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
        // TODO: consider calling InspectEndHandling
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
    uint32_t initialChangeCount;
    uint32_t currentChangeCount;
    le_mem_PoolRef_t memPool = NULL;

    if (mem_iter_GetPoolsListChgCnt(memIter, &initialChangeCount) != LE_OK)
    {
        goto ErrorReadProcCleanup;
    }

    do
    {
        if (mem_iter_GetNextPool(memIter, &memPool) != LE_OK)
        {
            goto ErrorReadProcCleanup;
        }

        if (memPool != NULL)
        {
            PrintMemPoolInfo(memPool);
            lineCount++;
        }

        if (mem_iter_GetPoolsListChgCnt(memIter, &currentChangeCount) != LE_OK)
        {
            goto ErrorReadProcCleanup;
        }
    }
    // Access the next mem pool only if the current mem pool is not NULL and there has been no
    // changes to mem pool list
    while ((memPool != NULL) && (currentChangeCount == initialChangeCount));

    // If the loop terminated because the next mem pool is NULL and there has been
    // no changes to the memory pool list, then we can delcare the end of list has been reached.
    if ((memPool == NULL) && (currentChangeCount == initialChangeCount))
    {
        InspectEndHandling(INSPECT_SUCCESS, &lineCount);
    }
    // Detected changes to mem pool list
    else
    {
        InspectEndHandling(INSPECT_INTERRUPTED, &lineCount);
    }

Cleanup:
    mem_iter_Delete(memIter);
    return;

ErrorReadProcCleanup:
    InspectEndHandling(INSPECT_ERROR, &lineCount);
    goto Cleanup;
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
        InspectEndHandlingTbl = InspectMemPoolEndHandlingTbl;
    }
    // TODO: Create other similar if blocks for timer, thread, etc.

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

    int i = 0;
    while (InspectEndHandlingTbl[i].endStatus != 0)
    {
        if (InspectEndHandlingTbl[i].endStatus == INSPECT_SUCCESS)
        {
            InspectEndHandlingTbl[i].refreshInterval.sec = value;
            break;
        }

        i++;
    }

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

    // Start the inspection.
    InspectFunc(PidToInspect);

    if (!IsFollowing)
    {
        exit(EXIT_SUCCESS);
    }
}
