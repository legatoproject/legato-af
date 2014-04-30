/** @file inspect.c
 *
 * Legato inspection tool used to inspect Legato structures such as memory pools, timers, threads,
 * mutexes, etc. in running processes.
 *
 * Must be run as root.
 *
 * @todo Only supports memory pools right now.  Add support for other timers, threads, etc.
 *
 * @todo Only supports inspection by PID right now.  Add support for inspecting applications,
 *       processes, components by name.
 *
 * Copyright (C) Sierra Wireless, Inc. 2014. All rights reserved. Use of this work is subject to license.
 */

#include "legato.h"
#include "mem.h"
#include "fileDescriptor.h"
#include "limit.h"


//--------------------------------------------------------------------------------------------------
/**
 * The item to inspect.
 *
 * @todo Add appName/procName/compName.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    pid_t pid;                                          // PID of the process to inpsect.
}
ItemToInspect_t;


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
        "    inspect - Inspects the memory pools of a Legato process.\n"
        "\n"
        "SYNOPSIS:\n"
        "    inspect [OPTIONS] PID\n"
        "\n"
        "DESCRIPTION:\n"
        "    Prints the memory pools usage for the specified process to stdout.\n"
        "\n"
        "OPTIONS:\n"
        "    -f         Periodically prints an updated memory usage for the process.\n"
        "\n"
        "    --help     Display this help and exit.\n"
        );
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the pid of the process to inspect from the command line.
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
    size_t numArgs = le_arg_NumArgs();
    size_t i = 0;

    char argBuf[LIMIT_MAX_ARGS_STR_BYTES];

    // Search the command line arguments for a pid.
    for (; i < numArgs; i++)
    {
        // Get a command line argument.
        LE_ASSERT(le_arg_GetArg(i, argBuf, sizeof(argBuf)) == LE_OK);

        // Attempt to convert it to a PID.
        char* endPtr;
        errno = 0;

        pid_t pid = strtol(argBuf, &endPtr, 10);

        if ( (errno == 0) && (*endPtr == '\0') && (pid > 0) )
        {
            return pid;
        }
    }

    fprintf(stderr, "Invalid PID.\n\n");
    exit(EXIT_FAILURE);
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks the command line arguments to see if the specified option was selected.
 *
 * @return
 *      true if the option was selected on the command-line.
 *      false if the option was not selected.
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
        LE_ASSERT(le_arg_GetArg(i, argBuf, sizeof(argBuf)) == LE_OK);

        if (strcmp(option, argBuf) == 0)
        {
            return true;
        }
    }

    return false;
}



//--------------------------------------------------------------------------------------------------
/**
 * Reads a line of text from the opened file descriptor specified up to the first newline or eof
 * character.  The output buffer will always be NULL-terminated and will not include the newline or
 * eof character.
 *
 * @return
 *      LE_OK if successful.
 *      LE_OVERFLOW if the buffer is too small.  As much of the line as possible will be copied to
 *                  buf.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ReadLine
(
    int fd,             // File to read from.
    char* buf,          // Buffer to store the line.
    size_t bufSize      // Buffer size.
)
{
    size_t index = 0;

    for (; index < bufSize - 1; index++)
    {
        // read one byte at a time.
        char c;
        int result;

        do
        {
            result = read(fd, &c, 1);
        }
        while ( (result == -1) && (errno == EINTR) );

        if (result == 1)
        {
            if (c == '\n')
            {
                // This is the end of the line.  Terminate the string and return.
                buf[index] = '\0';
                return LE_OK;
            }

            // Store char.
            buf[index] = c;
        }
        else if (result == 0)
        {
            // End of file nothing else to read.  Terminate the string and return.
            buf[index] = '\0';
            return LE_OK;
        }
        else
        {
            LE_ERROR("Could not read file.  %m.");
            return LE_FAULT;
        }
    }

    // No more buffer space.  Terminate the string and return.
    LE_ERROR("Buffer too small.");
    buf[index] = '\0';
    return LE_OVERFLOW;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the address of the .data section of the Legato framework library for the specified process.
 * The address is in the address space of the specified process.
 *
 * @note On failure an error message will be printed and the calling process will exit.
 *
 * @return
 *      Address of the text section of the Legato framework library.
 */
//--------------------------------------------------------------------------------------------------
static size_t GetFrameworkLibDataSectionAddr
(
    pid_t pid           // The pid to get the address for.  Zero means the calling process.
)
{
    // Build the path to the maps file.
    char fileName[LIMIT_MAX_PATH_BYTES];
    size_t snprintfSize;

    if (pid == 0)
    {
        snprintfSize = snprintf(fileName, sizeof(fileName), "/proc/self/maps");
    }
    else
    {
        snprintfSize = snprintf(fileName, sizeof(fileName), "/proc/%d/maps", pid);
    }

    if (snprintfSize >= sizeof(fileName))
    {
        fprintf(stderr, "Path is too long '%s'.\n", fileName);
        exit(EXIT_FAILURE);
    }

    // Open the maps file.
    int fd = open(fileName, O_RDONLY);

    if (fd == -1)
    {
        fprintf(stderr, "Could not open %s.  %m.\n", fileName);
        exit(EXIT_FAILURE);
    }

    // Search each line of the file to find the liblegato.so section.
    size_t address;
    char line[LIMIT_MAX_PATH_BYTES * 2];

    while (1)
    {
        if (ReadLine(fd, line, sizeof(line)) == LE_OK)
        {
            // The line is the section we are looking for if it specifies our library and has an
            // access mode of "r-xp" which we infer is the text section.
            if ( (strstr(line, "liblegato.so") != NULL) && (strstr(line, "rw-p") != NULL) )
            {
                // The line should begins with the starting address of the section.
                // Convert it to an address value.
                char* endPtr;
                errno = 0;
                address = strtoll(line, &endPtr, 16);

                if ( (errno != 0) || (endPtr == line) )
                {
                    fprintf(stderr, "Error reading file %s.\n", fileName);
                    exit(EXIT_FAILURE);
                }

                // Got the address.
                break;
            }
        }
        else
        {
            fprintf(stderr, "Error reading file %s.\n", fileName);
            exit(EXIT_FAILURE);
        }
    }

    fd_Close(fd);

    return address;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the address of the memory pools list in the address space of the specified process.
 *
 * @note On failure an error message will be printed and the calling process will exit.
 *
 * @return
 *      The address of the memory pools list in the address space of the specified process.
 */
//--------------------------------------------------------------------------------------------------
static size_t GetMemPoolsListAddress
(
    pid_t pid           // Process to to get the address for.
)
{
    // Calculate the offset address of the memory pools list by subtracting it by the start of our
    // own framwork library address.
    size_t offset = (size_t)mem_GetListOfPools() - GetFrameworkLibDataSectionAddr(0);

    // Calculate the process-under-inspection's mem pools address by adding the offset to the start
    // of their framework library address.
    return GetFrameworkLibDataSectionAddr(pid) + offset;
}


//--------------------------------------------------------------------------------------------------
/**
 * Reads bufSize bytes from the open file descriptor specified by fd, starting at offset, and stores
 * the bytes in the provided buffer.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ReadFromFile
(
    int fd,                 // File to read.
    size_t offset,          // Offset from the beginning of the file to start reading from.
    void* bufPtr,           // Buffer to store the read bytes in.
    size_t bufSize          // Size of the buffer.
)
{
    if (lseek(fd, offset, SEEK_SET) == -1)
    {
        LE_ERROR("Could not seek to address %zx.  %m.", offset);
        return LE_FAULT;
    }

    int result;

    do
    {
        result = read(fd, bufPtr, bufSize);
    }
    while ( (result == -1) && (errno == EINTR) );

    if (result == -1)
    {
        LE_ERROR("Could not read file.  %m.");
        return LE_FAULT;
    }
    else if (result != bufSize)
    {
        LE_ERROR("Unexpected end of file.");
        return LE_FAULT;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Print memory pool information header.  Clears the screen and prints information about the item
 * under inspection.
 */
//--------------------------------------------------------------------------------------------------
static void PrintMemPoolHeaderInfo
(
    const ItemToInspect_t* itemToInspectPtr     // The item under inspection.
)
{
    // Clear Screen.
    printf("%c[2J",27);

    // Print title.
    printf("Legato Memory Pools Inspector\n");

    // Print title.
    printf("Inspecting process %d\n", itemToInspectPtr->pid);

    // Print column headers.
    printf("%10s %10s %10s %10s %10s %10s %10s  %s\n",
           "TOTAL BLKS", "USED BLKS", "MAX USED", "OVERFLOWS", "ALLOCS", "BLK BYTES", "USED BYTES",
           "MEMORY POOL");
}


//--------------------------------------------------------------------------------------------------
/**
 * Print memory pool information to stdout.
 */
//--------------------------------------------------------------------------------------------------
static void PrintMemPoolInfo
(
    MemPool_t* poolPtr
)
{
    printf("%10zu %10zu %10zu %10zu %10"PRIu64" %10zu %10zu  %s\n",
           poolPtr->totalBlocks, poolPtr->numBlocksInUse,
           poolPtr->maxNumBlocksUsed, poolPtr->numOverflows, poolPtr->numAllocations,
           poolPtr->totalBlockSize, (poolPtr->totalBlockSize)*(poolPtr->numBlocksInUse),
           poolPtr->name);
}


//--------------------------------------------------------------------------------------------------
/**
 * Inspects the memory pool usage for the specified process.  Prints the results to stdout.
 *
 * @todo  This should probably be moved into the mem.c so that we don't need to know the internal
 *        structure of the memory pools or how the are stored to get the info.
 */
//--------------------------------------------------------------------------------------------------
static void InspectProcMemPools
(
    pid_t pid           // The process to inspect.
)
{
    // Build the path to the mem file for the process to inspect.
    char memFilePath[LIMIT_MAX_PATH_BYTES];
    if (snprintf(memFilePath, sizeof(memFilePath), "/proc/%d/mem", pid) >= sizeof(memFilePath))
    {
        fprintf(stderr, "Path is too long '%s'.", memFilePath);
        exit(EXIT_FAILURE);
    }

    // Open the mem file for the specified process.
    int fd = open(memFilePath, O_RDONLY);

    if (fd == -1)
    {
        fprintf(stderr, "Could not open %s.  %m.\n", memFilePath);
        exit(EXIT_FAILURE);
    }

    // Get the address offset of the list of memory pools for the process to inspect.
    size_t listOfPoolsAddrOffset = GetMemPoolsListAddress(pid);

    // Get the ListOfPools for the process-under-inspection.
    le_dls_List_t ListOfPools;

    if (ReadFromFile(fd, listOfPoolsAddrOffset, &ListOfPools, sizeof(ListOfPools)) != LE_OK)
    {
        fprintf(stderr, "Could not read memory file %s.\n", memFilePath);
        exit(EXIT_FAILURE);
    }

    // Get the address of the first pool's link.
    le_dls_Link_t* headPoolLinkPtr = le_dls_Peek(&ListOfPools);

    // Create a fake list of pools that has a single element.  Use this when iterating over the
    // link's in the list because the links read from the mems file is in the address space of the
    // process under test.  Using a fake pool guarantees that the linked list operation does not
    // accidentally reference memory in our own memory space.  This means that we have to check
    // for the end of the list manually.
    le_dls_List_t fakeListOfPools = LE_DLS_LIST_INIT;
    le_dls_Link_t fakeLink = LE_DLS_LINK_INIT;
    le_dls_Stack(&fakeListOfPools, &fakeLink);

    // Iterate over all memory pools.
    le_dls_Link_t* poolLinkPtr = headPoolLinkPtr;
    while (poolLinkPtr != NULL)
    {
        // Get the address of pool.
        MemPool_t* poolPtr = CONTAINER_OF(poolLinkPtr, MemPool_t, poolLink);

        // Read the pool into our own memory.
        MemPool_t pool;

        if (ReadFromFile(fd, (size_t)poolPtr, &pool, sizeof(pool)) != LE_OK)
        {
            fprintf(stderr, "Could not read memory file %s.\n", memFilePath);
            exit(EXIT_FAILURE);
        }

        // Print pool info.
        PrintMemPoolInfo(&pool);

        // Get the address of the next pool.
        poolLinkPtr = le_dls_PeekNext(&fakeListOfPools, &(pool.poolLink));

        // Check for the end of the list.
        if (poolLinkPtr == headPoolLinkPtr)
        {
            // Looped back to first pool so stop.
            break;
        }
    }

    fd_Close(fd);
}


//--------------------------------------------------------------------------------------------------
/**
 * Inspects the memory pool usage for the specified item.  Prints the results to stdout.
 */
//--------------------------------------------------------------------------------------------------
static void InspectMemoryPools
(
    const ItemToInspect_t* itemToInspectPtr  // The item to inspect.
)
{
    PrintMemPoolHeaderInfo(itemToInspectPtr);

    InspectProcMemPools(itemToInspectPtr->pid);
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
    // Get the entity to test from the context pointer.
    ItemToInspect_t* itemToInspectPtr = le_timer_GetContextPtr(timerRef);

    InspectMemoryPools(itemToInspectPtr);
}


COMPONENT_INIT
{
    if (IsOptionSelected("--help"))
    {
        // User is asking for help.
        PrintHelp();
        exit(EXIT_SUCCESS);
    }

    // Static local variable available for the life-time of this program.
    static ItemToInspect_t itemToInspect;

    itemToInspect.pid = GetPidToInspect();

    // Inspect memory pools.
    InspectMemoryPools(&itemToInspect);

    if (IsOptionSelected("-f"))
    {
        // The "follow" option was selected.

        // Set up the refresh timer.
        le_timer_Ref_t refreshTimer = le_timer_Create("RefreshTimer");
        LE_ASSERT(le_timer_SetHandler(refreshTimer, RefreshTimerHandler) == LE_OK);

        LE_ASSERT(le_timer_SetRepeat(refreshTimer, 0) == LE_OK);

        le_clk_Time_t interval = {.sec = 3, .usec = 0};
        LE_ASSERT(le_timer_SetInterval(refreshTimer, interval) == LE_OK);

        // Put the entity to test in the context pointer for the timer.
        LE_ASSERT(le_timer_SetContextPtr(refreshTimer, &itemToInspect) == LE_OK);

        // Start the refresh timer.
        LE_ASSERT(le_timer_Start(refreshTimer) == LE_OK);
    }
    else
    {
        exit(EXIT_SUCCESS);
    }
}
