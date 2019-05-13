//--------------------------------------------------------------------------------------------------
/** @file inspect.c
 *
 * Legato inspection tool used to inspect Legato structures such as memory pools, timers, threads,
 * mutexes, etc. on RTOS.
 *
 * @todo Consolidate common code between RTOS and Linux inspect tools.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "mem.h"


//--------------------------------------------------------------------------------------------------
/**
 * Objects of these types are used to refer to lists of memory pools, thread objects, timers,
 * mutexes, semaphores, and service objects. They can be used to iterate over those lists in a
 * remote process.
 */
//--------------------------------------------------------------------------------------------------
typedef struct MemPoolIter*         MemPoolIter_Ref_t;


//--------------------------------------------------------------------------------------------------
/**
 * Inspection types - what's being inspected for the remote process.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    INSPECT_INSP_TYPE_MEM_POOL,
    INSPECT_INSP_TYPE_LAST
}
InspType_t;


//--------------------------------------------------------------------------------------------------
/**
 * Object containing items necessary for accessing a list in the remote process.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_dls_List_t List;         ///< The list in the remote process.
    size_t* ListChgCntRef;      ///< Change counter for the remote list.
    le_dls_Link_t* headLinkPtr; ///< Pointer to the first link.
}
RemoteListAccess_t;


//--------------------------------------------------------------------------------------------------
/**
 * Iterator objects for stepping through the list of memory pools, thread objects, timers, mutexes,
 * and semaphores in a remote process.
 */
//--------------------------------------------------------------------------------------------------
typedef struct MemPoolIter
{
    RemoteListAccess_t memPoolList; ///< Memory pool list in the remote process.
    le_mem_Pool_t currMemPool;          ///< Current memory pool from the list.
}
MemPoolIter_t;

//--------------------------------------------------------------------------------------------------
/**
 * Local memory pool that is used for allocating an inspection object iterator.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t IteratorPool = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * ASCII code for the escape character.
 */
//--------------------------------------------------------------------------------------------------
#define ESCAPE_CHAR         27


//--------------------------------------------------------------------------------------------------
/**
 * Inspection type.
 **/
//--------------------------------------------------------------------------------------------------
InspType_t InspectType;


//--------------------------------------------------------------------------------------------------
/**
 * true = verbose mode (everything is printed).
 **/
//--------------------------------------------------------------------------------------------------
static bool IsVerbose;

//--------------------------------------------------------------------------------------------------
/**
 * true = exit command ASAP.
 **/
//--------------------------------------------------------------------------------------------------
static bool IsExiting;

//--------------------------------------------------------------------------------------------------
/**
 * Flags indicating how an inspection ended.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    INSPECT_SUCCESS,    ///< inspection completed without interruption or error.
    INSPECT_INTERRUPTED ///< inspection was interrupted due to list changes.
}
InspectEndStatus_t;


//--------------------------------------------------------------------------------------------------
/**
 * Prints a generic message on stderr so that the user is aware there is a problem, logs the
 * internal error message and exit.
 */
//--------------------------------------------------------------------------------------------------
#define INTERNAL_ERR(formatString, ...)                                 \
    {                                                                   \
        fprintf(stderr, "Internal error check logs for details.\n");    \
        LE_EMERG(formatString, ##__VA_ARGS__);                          \
        IsExiting = true;                                               \
    }


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
 * Initialize a RemoteListAccess_t data struct.
 */
//--------------------------------------------------------------------------------------------------
static void InitRemoteListAccessObj
(
    RemoteListAccess_t* remoteList
)
{
    remoteList->List.headLinkPtr = NULL;
    remoteList->ListChgCntRef = NULL;
    remoteList->headLinkPtr = NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates an iterator that can be used to iterate over the list of available memory pools for the
 * RTOS process.
 *
 * @note
 *      The calling process must be root or have appropriate capabilities for this function and all
 *      subsequent operations on the iterator to succeed.
 *
 * @return
 *      An iterator to the list of memory pools for the specified process.
 */
//--------------------------------------------------------------------------------------------------
static MemPoolIter_Ref_t CreateMemPoolIter
(
    void
)
{
    // Create the iterator.
    MemPoolIter_t* iteratorPtr = le_mem_ForceAlloc(IteratorPool);
    InitRemoteListAccessObj(&iteratorPtr->memPoolList);

    mem_Lock();

    memcpy(&iteratorPtr->memPoolList.List, mem_GetPoolList(),
        sizeof(iteratorPtr->memPoolList.List));
    iteratorPtr->memPoolList.ListChgCntRef = *mem_GetPoolListChgCntRef();

    mem_Unlock();

    return iteratorPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the memory pool list change counter from the specified iterator.
 *
 * @return
 *      List change counter.
 */
//--------------------------------------------------------------------------------------------------
static size_t GetMemPoolListChgCnt
(
    MemPoolIter_Ref_t iterator ///< [IN] The iterator to get the list change counter from.
)
{
    size_t count;

    mem_Lock();

    count = *iterator->memPoolList.ListChgCntRef;

    mem_Unlock();

    return count;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the next link of the provided link. This is for accessing a list in a remote process,
 * otherwise the doubly linked list API can simply be used. Note that "linkRef" is a ref to a
 * locally existing link obj, which is a link for a remote node. Therefore GetNextLink cannot be
 * called back-to-back.
 *
 * Also, if GetNextLink is called the first time for a given listInfoRef, linkRef is not used.
 *
 * After calling GetNextLink, the returned link ptr must be used to read the associated remote node
 * into the local memory space. One would then retrieve the link object from the node, and then
 * GetNextLink can be called on the ref of that link object.
 *
 * @return
 *      Pointer to a link of a node in the remote process
 */
//--------------------------------------------------------------------------------------------------
static le_dls_Link_t* GetNextLink
(
    RemoteListAccess_t* listInfoRef,    ///< [IN] Object for accessing a list in the remote process.
    le_dls_Link_t* linkRef              ///< [IN] Link of a node in the remote process. This is a
                                        ///<      ref to a locally existing link obj.
)
{
    INTERNAL_ERR_IF(listInfoRef == NULL,
                    "obj ref for accessing a list in the remote process is NULL.");

    // Create a fake list of nodes that has a single element.  Use this when iterating over the
    // links in the list because the links read from the mems file is in the address space of the
    // process under test.  Using a fake list guarantees that the linked list operation does not
    // accidentally reference memory in our own memory space.  This means that we have to check
    // for the end of the list manually.
    le_dls_List_t fakeList = LE_DLS_LIST_INIT;
    le_dls_Link_t fakeLink = LE_DLS_LINK_INIT;
    le_dls_Stack(&fakeList, &fakeLink);

    // Get the next link in the list.
    le_dls_Link_t* LinkPtr;

    if (listInfoRef->headLinkPtr == NULL)
    {
        // Get the address of the first node's link.
        LinkPtr = le_dls_Peek(&(listInfoRef->List));

        // The list is empty
        if (LinkPtr == NULL)
        {
            return NULL;
        }

        listInfoRef->headLinkPtr = LinkPtr;
    }
    else
    {
        // Get the address of the next node.
        LinkPtr = le_dls_PeekNext(&fakeList, linkRef);

        if (LinkPtr == listInfoRef->headLinkPtr)
        {
            // Looped back to the first node so there are no more nodes.
            return NULL;
        }
    }

    return LinkPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the next memory pool from the specified iterator.  The first time this function is called
 * after CreateMemPoolIter() is called, the first memory pool in the list is returned.  The second
 * time this function is called the second memory pool is returned and so on.
 *
 * @warning
 *      The memory pool returned by this function belongs to the remote process.  Do not attempt to
 *      expand the pool or allocate objects from the pool, doing so will lead to memory leaks in
 *      the calling process.
 *
 * @return
 *      A memory pool from the iterator's list of memory pools.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_Pool_t* GetNextMemPool
(
    MemPoolIter_Ref_t memPoolIterRef ///< [IN] The iterator to get the next mem pool from.
)
{
    le_dls_Link_t* linkPtr = GetNextLink(&(memPoolIterRef->memPoolList),
                                         &(memPoolIterRef->currMemPool.poolLink));

    if (linkPtr == NULL)
    {
        return NULL;
    }

    // Get the address of pool.
    le_mem_Pool_t* poolPtr = CONTAINER_OF(linkPtr, le_mem_Pool_t, poolLink);

    // Read the pool into our own memory.
    memcpy(&memPoolIterRef->currMemPool, poolPtr, sizeof(memPoolIterRef->currMemPool));

    return &(memPoolIterRef->currMemPool);
}

// TODO: migrate the above to a separate module.
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
        "    inspect - Inspects the internal structures such as memory pools, timers, etc. of a\n"
        "              Legato process.\n"
        "\n"
        "SYNOPSIS:\n"
        "    inspect <pools> [OPTIONS]\n"
        "\n"
        "DESCRIPTION:\n"
        "    inspect pools              Prints the current memory pools usage.\n"
        "\n"
        "OPTIONS:\n"
        "    -v\n"
        "        Prints in verbose mode.\n"
        "\n"
        "    --help\n"
        "        Display this help and exit.\n"
        );

    IsExiting = true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Object describing a column of a display table. Multiple columns make up a display table.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char* colTitle;      ///< Column title.
    char* titleFormat;   ///< Format template for the column title.
    char* colField;      ///< Column field.
    char* fieldFormat;   ///< Format template for a column field.
    uint8_t maxDataSize; ///< Max data size. For strings, string length; otherwise, data size in
                         ///< number of bytes.
    // TODO: can probably be figured out from format template
    bool isString;       ///< Is the field string or not.
    uint8_t colWidth;    ///< Column width in number of characters.
    bool isPrintSimple;  ///< Print this field in non-verbose mode or not.
}
ColumnInfo_t;


//--------------------------------------------------------------------------------------------------
/**
 * Characters representing dividers between columns.
 */
//--------------------------------------------------------------------------------------------------
static char ColumnSpacers[] = " | ";


//--------------------------------------------------------------------------------------------------
/**
 * Line buffer and its associated char length and byte size.
 */
//--------------------------------------------------------------------------------------------------
static size_t TableLineLen = 0;
#define TableLineBytes (TableLineLen + 1)
static char* TableLineBuffer;


//--------------------------------------------------------------------------------------------------
/**
 * Strings representing sub-pool and super-pool.
 */
//--------------------------------------------------------------------------------------------------
static char SubPoolStr[] = "(Sub-pool)";
static char SuperPoolStr[] = "";


//--------------------------------------------------------------------------------------------------
/**
 * These tables define the display tables of each inspection type. The column width is left at 0
 * here, but will be calculated in InitDisplayTable. The calculated column width is guaranteed to
 * accomodate all possible data, so long as maxDataSize and isString are correctly populated. The
 * 0 maxDataSize fields are populated in InitDisplay. A column title acts as an identifier so they
 * need to be unique. The order of the ColumnInfo_t structs directly affect the order they are
 * displayed at runtime (column with the smallest index is at the leftmost side).
 */
//--------------------------------------------------------------------------------------------------
static ColumnInfo_t MemPoolTableInfo[] =
{
    {"TOTAL BLKS",  "%*s",  NULL, "%*" PRIuS,   sizeof(size_t),              false, 0, true},
    {"USED BLKS",   "%*s",  NULL, "%*" PRIuS,   sizeof(size_t),              false, 0, true},
    {"MAX USED",    "%*s",  NULL, "%*" PRIuS,   sizeof(size_t),              false, 0, true},
    {"OVERFLOWS",   "%*s",  NULL, "%*" PRIuS,   sizeof(size_t),              false, 0, true},
    {"ALLOCS",      "%*s",  NULL, "%*" PRIu64,  sizeof(uint64_t),            false, 0, true},
    {"BLK BYTES",   "%*s",  NULL, "%*" PRIuS,   sizeof(size_t),              false, 0, true},
    {"USED BYTES",  "%*s",  NULL, "%*" PRIuS,   sizeof(size_t),              false, 0, true},
    {"MEMORY POOL", "%-*s", NULL, "%-*s",       LIMIT_MAX_MEM_POOL_NAME_LEN, true,  0, true},
    {"SUB-POOL",    "%*s",  NULL, "%*s",        0,                           true,  0, true}
};
static size_t MemPoolTableInfoSize = NUM_ARRAY_MEMBERS(MemPoolTableInfo);


//--------------------------------------------------------------------------------------------------
/**
 * Initialize the max data size of the specified column in the specified table.
 */
//--------------------------------------------------------------------------------------------------
static void InitDisplayTableMaxDataSize
(
    char* colTitle,      ///< [IN] As a key for lookup, title of column to print the string under.
    ColumnInfo_t* table, ///< [IN] Table ref.
    size_t tableSize,    ///< [IN] Table size.
    size_t maxDataSize   ///< [IN] Max data size to init the column with.
)
{
    int i;
    for (i = 0; i < tableSize; i++)
    {
        if (strcmp(table[i].colTitle, colTitle) == 0)
        {
            table[i].maxDataSize = maxDataSize;
            return;
        }
    }

    INTERNAL_ERR("Failed to init display table.");
}


//--------------------------------------------------------------------------------------------------
/**
 * Initialize a display table. This function calculates the appropriate column widths that will
 * accomodate all possible data for each column. With column widths calculated, column and line
 * buffers are also properly initialized.
 */
//--------------------------------------------------------------------------------------------------
static void InitDisplayTable
(
    ColumnInfo_t* table, ///< [IN] Table ref.
    size_t tableSize     ///< [IN] Table size.
)
{
    // Some columns in ThreadObjTableInfo needs its maxDataSize figured out.
    // Determine largest text size out of all possible text in a number-text table; update the
    // sizes to the thread object display table.
    if (table == MemPoolTableInfo)
    {
        size_t subPoolStrLen = strlen(SubPoolStr);
        size_t superPoolStrLen = strlen(SuperPoolStr);
        size_t subPoolColumnStrLen = subPoolStrLen > superPoolStrLen ? subPoolStrLen  :
                                                                       superPoolStrLen;
        InitDisplayTableMaxDataSize("SUB-POOL", table, tableSize, subPoolColumnStrLen);
    }

    int i;
    for (i = 0; i < tableSize; i++)
    {
        size_t headerTextWidth = strlen(table[i].colTitle);

        if (table[i].isString == false)
        {
            // uint8_t should be plenty enough to store the number of digits of even uint64_t
            // (which is 20). casting the result of log10 could overflow but highly unlikely.
            uint8_t maxDataWidth = (uint8_t)log10(exp2(table[i].maxDataSize*8))+1;
            table[i].colWidth = (maxDataWidth > headerTextWidth) ? maxDataWidth : headerTextWidth;
        }
        else
        {
            table[i].colWidth = (table[i].maxDataSize > headerTextWidth) ?
                                 table[i].maxDataSize : headerTextWidth;
        }

        // Now that column width is figured out, we can use that to allocate buffer for colField.
        #define colBytes table[i].colWidth + 1
        table[i].colField = (char*)malloc(colBytes);
        // Initialize the buffer
        memset(table[i].colField, 0, colBytes);
        #undef colBytes

        // Add the column width and column spacer length to the overall line length.
        TableLineLen += table[i].colWidth + strlen(ColumnSpacers);
    }

    // allocate and init the line buffer
    TableLineBuffer = (char*)malloc(TableLineBytes);
    if (!TableLineBuffer)
    {
       INTERNAL_ERR("TableLineBuffer is NULL.");
       return;
    }
    memset(TableLineBuffer, 0, TableLineBytes);
}


//--------------------------------------------------------------------------------------------------
/**
 * Initialize all display tables, and other misc. display related chores.
 */
//--------------------------------------------------------------------------------------------------
static void InitDisplay
(
    InspType_t inspectType ///< [IN] What to inspect.
)
{
    switch (inspectType)
    {
        case INSPECT_INSP_TYPE_MEM_POOL:
            // Initialize the display tables with the optimal column widths.
            InitDisplayTable(MemPoolTableInfo, MemPoolTableInfoSize);
            break;

        default:
            INTERNAL_ERR("Failed to initialize display table - unexpected inspect type %d.",
                         inspectType);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Prints the header row from the specified table.
 */
//--------------------------------------------------------------------------------------------------
static void PrintHeader
(
    ColumnInfo_t* table, ///< [IN] Table ref.
    size_t tableSize     ///< [IN] Table size.
)
{
    int index = 0;
    int i = 0;
    while (i < tableSize)
    {
        if ((table[i].isPrintSimple == true) || (IsVerbose == true))
        {
            index += snprintf((TableLineBuffer + index), (TableLineBytes - index),
                              table[i].titleFormat, table[i].colWidth, table[i].colTitle);
            index += snprintf((TableLineBuffer + index), (TableLineBytes - index), "%s",
                              ColumnSpacers);
        }

        i++;
    }
    puts(TableLineBuffer);
}


//--------------------------------------------------------------------------------------------------
/**
 * Prints a row for the currently inspected node from the specified table. The column buffers
 * (colField) need to be filled in prior to calling this function.
 */
//--------------------------------------------------------------------------------------------------
static void PrintInfo
(
    ColumnInfo_t* table, ///< [IN] Table ref.
    size_t tableSize     ///< [IN] Table size.
)
{
    int index = 0;
    int i = 0;
    while (i < tableSize)
    {
        if ((table[i].isPrintSimple == true) || (IsVerbose == true))
        {
            index += snprintf((TableLineBuffer + index), (TableLineBytes - index), "%s",
                              table[i].colField);
            index += snprintf((TableLineBuffer + index), (TableLineBytes - index), "%s",
                              ColumnSpacers);
        }

        i++;
    }
    puts(TableLineBuffer);
}


//--------------------------------------------------------------------------------------------------
/**
 * For the given table, return the next column.
 */
//--------------------------------------------------------------------------------------------------
static ColumnInfo_t* GetNextColumn
(
    ColumnInfo_t* table, ///< [IN] Table ref.
    size_t tableSize,    ///< [IN] Table size.
    int* indexRef        ///< [IN/OUT] Iterator to parse the table.
)
{
    int i = *indexRef;

    if (i == tableSize)
    {
        INTERNAL_ERR("Unable to get the next column.");
    }

    (*indexRef)++;

    return &(table[i]);
}


//--------------------------------------------------------------------------------------------------
/**
 * Print Inspect results header for human-readable format; and print global data for machine-
 * readable format.
 *
 * @return
 *      The number of lines printed, if outputting human-readable format.
 */
//--------------------------------------------------------------------------------------------------
static int PrintInspectHeader
(
    void
)
{
    int lineCount = 0;
    ColumnInfo_t* table;
    size_t tableSize;

    // The size should accomodate the longest inspectTypeString.
    #define inspectTypeStringSize 40
    char inspectTypeString[inspectTypeStringSize];
    switch (InspectType)
    {
        case INSPECT_INSP_TYPE_MEM_POOL:
#ifdef LE_MEM_VALGRIND
            strncpy(inspectTypeString, "Memory Pools [Valgrind Mode]", inspectTypeStringSize);
#else
            strncpy(inspectTypeString, "Memory Pools", inspectTypeStringSize);
#endif
            table = MemPoolTableInfo;
            tableSize = MemPoolTableInfoSize;
            break;

        default:
            INTERNAL_ERR("unexpected inspect type %d.", InspectType);
    }

    printf("\n");
    lineCount++;

    // Print title.
    printf("Legato %s Inspector\n", inspectTypeString);
    lineCount++;

    // Print column headers.
    PrintHeader(table, tableSize);
    lineCount++;

    return lineCount;
}


//--------------------------------------------------------------------------------------------------
/**
 * The ExportXXXToJson and FillXXXColField families of functions are used by the PrintXXXInfo
 * functions to output data in json format, or to print the "colField" string in the XXXTableInfo
 * tables (which are to be later printed to the terminal), respectively.
 *
 * These functions provide type checking for the data to be printed, and properly format the data
 * according to the formatting rules defined by the XXXTableInfo tables. They also determine of the
 * data should be output or not based on the "verbose mode" setting.
 *
 * For each data type used by the XXXTableInfo tables, there should be a corresponding pairs of
 * ExportXXXToJson and FillXXXColField functions.
 */
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
/**
 * The FillXXXColField family of functions.
 */
//--------------------------------------------------------------------------------------------------

// string
static void FillStrColField
(
    char*         field,     ///< [IN] the data to be printed to the ColField of the table.
    ColumnInfo_t* table,     ///< [IN] XXXTableInfo ref.
    size_t        tableSize, ///< [IN] XXXTableInfo size.
    int*          indexRef   ///< [IN/OUT] iterator to parse the table.
)
{
    ColumnInfo_t* col = GetNextColumn(table, tableSize, indexRef);
    if ((col->isPrintSimple == true) || (IsVerbose == true))
    {
        snprintf(col->colField, (col->colWidth + 1), col->fieldFormat, col->colWidth, field);
    }
}

// size_t
static void FillSizeTColField
(
    size_t        field, // param comments same as FillStrColField
    ColumnInfo_t* table,
    size_t        tableSize,
    int*          indexRef
)
{
    ColumnInfo_t* col = GetNextColumn(table, tableSize, indexRef);
    if ((col->isPrintSimple == true) || (IsVerbose == true))
    {
        snprintf(col->colField, (col->colWidth + 1), col->fieldFormat, col->colWidth, field);
    }
}


// uint64_t
static void FillUint64ColField
(
    uint64_t      field, // param comments same as FillStrColField
    ColumnInfo_t* table,
    size_t        tableSize,
    int*          indexRef
)
{
    ColumnInfo_t* col = GetNextColumn(table, tableSize, indexRef);
    if ((col->isPrintSimple == true) || (IsVerbose == true))
    {
        snprintf(col->colField, (col->colWidth + 1), col->fieldFormat, col->colWidth, field);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Print memory pool information to stdout.
 */
//--------------------------------------------------------------------------------------------------
static int PrintMemPoolInfo
(
    le_mem_PoolRef_t memPool    ///< [IN] ref to mem pool to be printed.
)
{
    int lineCount = 0;

    // Get pool stats.
    le_mem_PoolStats_t poolStats;
    le_mem_GetStats(memPool, &poolStats);

    size_t blockSize = le_mem_GetObjectFullSize(memPool);

    // Determine if this pool is a sub-pool, and set the appropriate string to display it.
    char* subPoolStr = le_mem_IsSubPool(memPool) ? SubPoolStr : SuperPoolStr;

    // Get the pool name.
    char name[LIMIT_MAX_COMPONENT_NAME_LEN + 1 + LIMIT_MAX_MEM_POOL_NAME_BYTES];
    INTERNAL_ERR_IF(le_mem_GetName(memPool, name, sizeof(name)) != LE_OK,
                    "Name buffer is too small.");

    // Output mem pool info
    int index = 0;

    // NOTE that the order has to correspond to the column orders in the corresponding table.
    // Since this order is "hardcoded" in a sense, one should avoid having multiple
    // copies of these. The same applies to other PrintXXXInfo functions.
    FillSizeTColField (le_mem_GetObjectCount(memPool),       MemPoolTableInfo,
                                                             MemPoolTableInfoSize, &index);
    FillSizeTColField (poolStats.numBlocksInUse,             MemPoolTableInfo,
                                                             MemPoolTableInfoSize, &index);
    FillSizeTColField (poolStats.maxNumBlocksUsed,           MemPoolTableInfo,
                                                             MemPoolTableInfoSize, &index);
    FillSizeTColField (poolStats.numOverflows,               MemPoolTableInfo,
                                                             MemPoolTableInfoSize, &index);
    FillUint64ColField(poolStats.numAllocs,                  MemPoolTableInfo,
                                                             MemPoolTableInfoSize, &index);
    FillSizeTColField (blockSize,                            MemPoolTableInfo,
                                                             MemPoolTableInfoSize, &index);
    FillSizeTColField (blockSize*(poolStats.numBlocksInUse), MemPoolTableInfo,
                                                             MemPoolTableInfoSize, &index);
    FillStrColField   (name,                                 MemPoolTableInfo,
                                                             MemPoolTableInfoSize, &index);
    FillStrColField   (subPoolStr,                           MemPoolTableInfo,
                                                             MemPoolTableInfoSize, &index);

    PrintInfo(MemPoolTableInfo, MemPoolTableInfoSize);
    lineCount++;

    return lineCount;
}


//--------------------------------------------------------------------------------------------------
/**
 * Performs actions when an inspection ends depending on how it ends.
 */
//--------------------------------------------------------------------------------------------------
static int InspectEndHandling
(
    InspectEndStatus_t endStatus ///< [IN] How an inspection ended.
)
{
    int lineCount = 0;

    if (endStatus == INSPECT_INTERRUPTED)
    {
        printf(">>> Detected list changes. Stopping inspection. <<<\n");
        lineCount++;
    }

    // The last line of the current run of inspection has finished, so it's a good place to
    // flush the write buffer on stdout. This is important for redirecting the output to a
    // log file, so that the end of an inspection is written to the log as soon as it
    // happens.
    fflush(stdout);

    return lineCount;
}


//--------------------------------------------------------------------------------------------------
/**
 * Performs the specified inspection for the specified process. Prints the results to stdout.
 */
//--------------------------------------------------------------------------------------------------
static void InspectFunc
(
    InspType_t inspectType ///< [IN] What to inspect.
)
{
    // function prototype for the CreateXXXIter family.
    typedef void* (*CreateIterFunc_t)(void);
    // function prototype for the GetXXXListChgCnt family.
    typedef size_t (*GetListChgCntFunc_t)(void* iterRef);
    // function prototype for the GetNextXXX family.
    typedef void* (*GetNextNodeFunc_t)(void* iterRef);
    // Function prototype for the PrintXXXInfo family.
    typedef int (*PrintNodeInfoFunc_t)(void* nodeRef);

    CreateIterFunc_t createIterFunc;
    GetListChgCntFunc_t getListChgCntFunc;
    GetNextNodeFunc_t getNextNodeFunc;
    PrintNodeInfoFunc_t printNodeInfoFunc;

    // assigns the appropriate set of functions according to the inspection type.
    switch (inspectType)
    {
        case INSPECT_INSP_TYPE_MEM_POOL:
            createIterFunc    = (CreateIterFunc_t)    CreateMemPoolIter;
            getListChgCntFunc = (GetListChgCntFunc_t) GetMemPoolListChgCnt;
            getNextNodeFunc   = (GetNextNodeFunc_t)   GetNextMemPool;
            printNodeInfoFunc = (PrintNodeInfoFunc_t) PrintMemPoolInfo;
            break;

        default:
            INTERNAL_ERR("unexpected inspect type %d.", inspectType);
    }

    // Create an iterator.
    void* iterRef = createIterFunc();

    static int lineCount = 0;

    // Print header information.
    lineCount += PrintInspectHeader();


    // Iterate through the list of nodes.
    size_t initialChangeCount = getListChgCntFunc(iterRef);
    size_t currentChangeCount;
    void* nodeRef = NULL;

    do
    {
        nodeRef = getNextNodeFunc(iterRef);

        if (nodeRef != NULL)
        {
            lineCount += printNodeInfoFunc(nodeRef);
        }

        currentChangeCount = getListChgCntFunc(iterRef);
    }
    // Access the next node only if the current node is not NULL and there has been no changes to
    // the node list.
    while ((nodeRef != NULL) && (currentChangeCount == initialChangeCount));

    // If the loop terminated because the next node is NULL and there has been no changes to the
    // node list, then we can delcare the end of list has been reached.
    if ((nodeRef == NULL) && (currentChangeCount == initialChangeCount))
    {
        lineCount += InspectEndHandling(INSPECT_SUCCESS);
    }
    // Detected changes to the node list.
    else
    {
        lineCount += InspectEndHandling(INSPECT_INTERRUPTED);
    }

    le_mem_Release(iterRef);

    return;
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
        InspectType = INSPECT_INSP_TYPE_MEM_POOL;
    }
    else
    {
        fprintf(stderr, "Invalid command '%s'.\n", command);
        IsExiting = true;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Create a memory pool for the iterators.
 **/
//--------------------------------------------------------------------------------------------------
static void InitIteratorPool
(
    void
)
{
    if (NULL == IteratorPool)
    {
        // TODO: If other types are supported in the future, the block size should be the maximum of
        //       all the iterator types.
        size_t size = sizeof(MemPoolIter_t);

        IteratorPool = le_mem_CreatePool("Iterators", size);
    }
}


//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    IsExiting = false;
    IsVerbose = false;
    InspectType = INSPECT_INSP_TYPE_LAST;

    // The command-line has a command string.
    le_arg_AddPositionalCallback(&CommandArgHandler);

    // --help option causes everything else to be ignored, prints help, and exits.
    le_arg_SetFlagCallback(&PrintHelp, NULL, "help");

    // -v option prints in verbose mode.
    le_arg_SetFlagVar(&IsVerbose, "v", NULL);

    le_arg_Scan();
    if (le_arg_GetScanResult() != LE_OK || IsExiting || InspectType == INSPECT_INSP_TYPE_LAST)
    {
        return;
    }

    // Create a memory pool for iterators.
    InitIteratorPool();
    if (IsExiting)
    {
        return;
    }

    InitDisplay(InspectType);
    if (IsExiting)
    {
        return;
    }

    // Start the inspection.
    InspectFunc(InspectType);
}
