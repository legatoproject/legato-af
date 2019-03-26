 /**
  * This module is for integration testing of the fwupdate status feature (dual system case)
  *
  * * You must issue the following commands:
  * @verbatim
  * $ app start fwupdateStatusTest
  * $ execInApp fwupdateStatusTest fwupdateStatusTest -- <command> [<parameter>]
  *     <command>
  *         list_parts
  *             list the partition present on the system
  *         get_status
  *             return the FW update status
  *         corrupt <part id>
  *             perform a corruption of the given partition
  *
  * Example:
  * $ execInApp fwupdateStatusTest fwupdateStatusTest help
  * @endverbatim
  *
  * Copyright (C) Sierra Wireless Inc.
  *
  */

#include "interfaces.h"
#include "pa_flash.h"

// Number of partition information descriptors in the pool
#define PART_INFO_DESC_MAX 20
// Partition information descriptors pool id
#define PART_INFO_POOL_NAME "Partitions pool"

// Erase the 3rd block (starting from 0) of each partition
// This way, the header and eventually its backup (UBI) are skipped.
#define ERASE_BLOCK_NDX 2

// Two arguments limit
#define SECOND_ARG 2

// Default string size
#define STRING_SIZE 255

// Partition information structure
typedef struct partInfo
{
    le_dls_Link_t   node;
    pa_flash_Info_t info;
    uint32_t        index;
}
partInfo_t;

// Dual system partitions id array
const char *dualSysId[] = { "tz", "rpm" };

// Partition information descriptors pool
le_mem_PoolRef_t  PartPool;

//--------------------------------------------------------------------------------------------------
/**
 * Print function.
 *
 */
//--------------------------------------------------------------------------------------------------
static void Print
(
    char* string
)
{
    bool sandboxed = (getuid() != 0);

    if(sandboxed)
    {
        LE_INFO("%s", string);
    }
    else
    {
        fprintf(stderr, "%s\n", string);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Help.
 *
 */
//--------------------------------------------------------------------------------------------------
static void PrintUsage
(
    void
)
{
    int idx;
    const char * usagePtr[] = {
            "Usage of the 'fwupdateStatusTest' application is:",
            "fwupdateStatusTest -- list_parts: list the partition present on the system",
            "fwupdateStatusTest -- get_status: return the FW update status",
            "fwupdateStatusTest -- corrupt <part id>: perform a corruption of the given partition",
            "",
    };

    for(idx = 0; idx < NUM_ARRAY_MEMBERS(usagePtr); idx++)
    {
        Print((char*) usagePtr[idx]);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Build a list of the partitions
 */
//--------------------------------------------------------------------------------------------------
static le_result_t BuildMtdList
(
    le_dls_List_t *mtdListPtr
)
{
    le_result_t      result;
    int              partIndex   = 0;
    partInfo_t      *partInfoPtr;
    pa_flash_Info_t  info;

    if (NULL == mtdListPtr)
    {
        LE_ERROR("Invalid MTD devices list!");
        return LE_BAD_PARAMETER;
    }

    result = pa_flash_GetInfo(partIndex, &info, false, false);
    while (LE_UNSUPPORTED != result)
    {
        uint32_t idCount;

        idCount = sizeof(dualSysId) / sizeof(dualSysId[0]);
        while (idCount--)
        {
            if (0 == strncmp(info.name, dualSysId[idCount], strlen(dualSysId[idCount])))
            {
                LE_INFO("WARNING: %s partition is dual...", &info.name[0]);
                result = pa_flash_GetInfo(partIndex, &info, false, true);
                if (LE_OK != result)
                {
                    LE_ERROR("Cannot get info for %s!", dualSysId[idCount]);
                }
            }
        }

        if (LE_OK == result)
        {
            partInfoPtr = le_mem_ForceAlloc(PartPool);
            partInfoPtr->info = info;
            partInfoPtr->index = partIndex;
            partInfoPtr->node = LE_DLS_LINK_INIT;
            le_dls_Queue(mtdListPtr, &partInfoPtr->node);
            LE_INFO("Added %s partition to the list...", &partInfoPtr->info.name[0]);
        }
        else
        {
            LE_INFO("Skipped mtd%d", partIndex);
        }
        partIndex++;
        result = pa_flash_GetInfo(partIndex, &info, false, false);
    }

    LE_INFO("Found %d mtd devices.", partIndex);
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Determine whether the given id matches a partition.
 */
//--------------------------------------------------------------------------------------------------
static bool IsInMtdList
(
    le_dls_List_t *mtdListPtr,
    const char *idPtr,
    partInfo_t **partInfoPtr
)
{
    le_dls_Link_t *nodePtr;

    // Set NULL as default value if possible
    if (NULL != partInfoPtr)
    {
        *partInfoPtr = NULL;
    }

    // Check parameters
    if (NULL == mtdListPtr)
    {
        LE_ERROR("Invalid MTD devices list!");
        return false;
    }

    if (false != le_dls_IsEmpty(mtdListPtr))
    {
        LE_INFO("MTD devices list is empty...");
        return false;
    }

    if ((NULL == idPtr) || ('\0' == *idPtr))
    {
        LE_ERROR("Invalid partition id!");
        return false;
    }

    // Peek the first descriptor
    nodePtr = le_dls_Peek(mtdListPtr);

    while (NULL != nodePtr)
    {
        partInfo_t *infoPtr = CONTAINER_OF(nodePtr, partInfo_t, node);

        if (NULL != infoPtr)
        {
            // Look for the matching descriptor
            if (0 == strncmp(&infoPtr->info.name[0], idPtr, PA_FLASH_MAX_INFO_NAME))
            {
                // Return the descriptor if possible
                if (NULL != partInfoPtr)
                {
                    *partInfoPtr = infoPtr;
                }
                else
                {
                    LE_INFO("Unable to return partition descriptor due to NULL parameter.");
                }
                LE_INFO("Found %s partition...", idPtr);
                return true;
            }
        }
        else
        {
            LE_ERROR("MTD list is corrupted!");
            return false;
        }

        // Peek the next descriptor
        nodePtr = le_dls_PeekNext(mtdListPtr, nodePtr);
    }

    LE_ERROR("%s partition not found...", idPtr);
    return false;
}


//--------------------------------------------------------------------------------------------------
/**
 * Delete a list of the partitions
 */
//--------------------------------------------------------------------------------------------------
static void DeleteMtdList
(
    le_dls_List_t *mtdListPtr
)
{
    le_dls_Link_t *nodePtr;

    if (NULL == mtdListPtr)
    {
        LE_ERROR("Invalid MTD devices list!");
        return;
    }

    if (false != le_dls_IsEmpty(mtdListPtr))
    {
        LE_INFO("MTD devices list is empty...");
        return;
    }

    nodePtr = le_dls_Pop(mtdListPtr);

    while (NULL != nodePtr)
    {
        partInfo_t *partInfoPtr = CONTAINER_OF(nodePtr, partInfo_t, node);

        if (NULL != partInfoPtr)
        {
            le_mem_Release(partInfoPtr);
        }
        else
        {
            LE_ERROR("MTD list is corrupted!");
            return;
        }

        nodePtr = le_dls_Pop(mtdListPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Perform the corruption on the required paritition
 */
//--------------------------------------------------------------------------------------------------
static le_result_t CorruptMtdPartition
(
    partInfo_t *partInfoPtr
)
{
    le_result_t     result = LE_OK;
    pa_flash_Desc_t desc;

    if (NULL == partInfoPtr)
    {
        LE_ERROR("Invalid MTD info decriptor!");
        return LE_BAD_PARAMETER;
    }

    if (LE_OK != pa_flash_Open(partInfoPtr->index, PA_FLASH_OPENMODE_READWRITE, &desc, NULL))
    {
        LE_ERROR("Unable to acces the %s partition in RW mode!", &partInfoPtr->info.name[0]);
        return LE_FAULT;
    }

    // Switch into logic blocks in order to skip potential bad block
    if (LE_OK != pa_flash_Scan(desc, NULL))
    {
        LE_ERROR("Unable to map LEB on PEB for the %s partition in RW mode!",
            &partInfoPtr->info.name[0]);
        result = LE_FAULT;
        goto error_close;
    }

    LE_INFO("Erase flash block of %s partition...", &partInfoPtr->info.name[0]);

    // Erase the 3rd block (starting from 0) of each partition
    // This way, the header and eventually its backup (UBI) are skipped.
    if (LE_OK != pa_flash_EraseBlock(desc, ERASE_BLOCK_NDX))
    {
        LE_ERROR("Unable to erase the %s partition in RW mode!", &partInfoPtr->info.name[0]);
        result = LE_FAULT;
    }
    else
    {
        LE_INFO("Flash block of %s partition has been erased...", &partInfoPtr->info.name[0]);
    }

    // Switch back into physical blocks
    if (LE_OK != pa_flash_Unscan(desc))
    {
        LE_ERROR("Unable to restore PEB mapping for the %s partition in RW mode!",
            &partInfoPtr->info.name[0]);
    }

error_close:
    if (LE_OK != pa_flash_Close(desc))
    {
        LE_ERROR("Unable to acces the %s partition in RW mode!", &partInfoPtr->info.name[0]);
        return LE_FAULT;
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Display a list of the partitions
 */
//--------------------------------------------------------------------------------------------------
static void DisplayMtd
(
    le_dls_List_t *mtdListPtr
)
{
    le_dls_Link_t *nodePtr;

    if (NULL == mtdListPtr)
    {
        LE_ERROR("Invalid MTD devices list!");
        return;
    }

    if (false != le_dls_IsEmpty(mtdListPtr))
    {
        LE_INFO("MTD devices list is empty...");
        return;
    }

    nodePtr = le_dls_Peek(mtdListPtr);

    while (NULL != nodePtr)
    {
        partInfo_t *partInfoPtr;
        char prompt[STRING_SIZE];

        partInfoPtr = CONTAINER_OF(nodePtr, partInfo_t, node);
        snprintf(&prompt[0], STRING_SIZE, "mtd%d -> %s",
               partInfoPtr->index, &partInfoPtr->info.name[0]);
        Print(&prompt[0]);
        nodePtr = le_dls_PeekNext(mtdListPtr, nodePtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Main thread.
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    const char    *testString          = "";
    const char    *secondString        = "";
    le_result_t    result;
    char           string[STRING_SIZE];
    le_dls_List_t  PartList            = LE_DLS_LIST_INIT;

    /* Get the test identifier */
    if (le_arg_NumArgs() >= 1)
    {
        testString = le_arg_GetArg(0);

        if (le_arg_NumArgs() >= SECOND_ARG)
        {
            secondString = le_arg_GetArg(1);
        }
    }
    else
    {
        PrintUsage();
        exit(0);
    }

    LE_INFO("Start fwupdateStatusTest app.");
    memset (string, 0, STRING_SIZE);
    PartPool = le_mem_ExpandPool(
        le_mem_CreatePool(PART_INFO_POOL_NAME, sizeof(partInfo_t)), PART_INFO_DESC_MAX);

    if (0 == strcmp(testString, "help"))
    {
        PrintUsage();
        exit(0);
    }
    else if (0 == strcmp(testString, "corrupt"))
    {
        if (strlen(secondString) > 0)
        {
            partInfo_t *partInfoPtr;

            BuildMtdList(&PartList);
            if (true == IsInMtdList(&PartList, secondString, &partInfoPtr))
            {
                result = CorruptMtdPartition(partInfoPtr);
                if (LE_OK == result)
                {
                    snprintf(string,
                        STRING_SIZE,
                        "%s partition is now corrupted.",
                        &partInfoPtr->info.name[0]);
                }
                else
                {
                    snprintf(string,
                        STRING_SIZE,
                        "Corruption of %s partition failed!",
                        &partInfoPtr->info.name[0]);
                }
                Print (string);
                if (LE_OK == result)
                {
                    Print("In order to detect the corruption, "
                            "the module needs to swap to the other system by executing on PC\n"
                            "fastboot oem swi-set-ssid <xxx>\n"
                            "with <xxx> = 111 for system 1 and <xxx> = 222 for system 2.\n"
                            "The module has to be switched in bootloader mode.");
                }
            }
            else
            {
                snprintf(string,
                    STRING_SIZE,
                    "ERROR: %s partition not found!",
                    &partInfoPtr->info.name[0]);
                Print (string);
            }
            DeleteMtdList(&PartList);
        }
        else
        {
            Print("No partition specified!");
        }
        exit(0);
    }
    else if (0 == strcmp(testString, "get_status"))
    {
        le_fwupdate_UpdateStatus_t updateStatus;
        char statusLabel[LE_FWUPDATE_STATUS_LABEL_LENGTH_MAX];

        result = le_fwupdate_GetUpdateStatus(
            &updateStatus,
            &statusLabel[0],
            LE_FWUPDATE_STATUS_LABEL_LENGTH_MAX);

        if (LE_OK == result)
        {
            snprintf(string,
                STRING_SIZE,
                "Update status\n\tresult: %d\n\tstatus: %s\n\tstatus code: %d\n",
                result, &statusLabel[0], (int)updateStatus);
            Print(string);
        }
        else
        {
            Print("ERROR: Unable to get the update status!");
        }
        exit(0);
    }
    else if (0 == strcmp(testString, "list_parts"))
    {
        BuildMtdList(&PartList);
        DisplayMtd(&PartList);
        DeleteMtdList(&PartList);
        exit(0);
    }
    else
    {
        LE_DEBUG ("fwupdateTest: not supported arg");
        PrintUsage();
        exit(EXIT_FAILURE);
    }
}
