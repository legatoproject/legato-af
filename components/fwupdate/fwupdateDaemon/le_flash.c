/**
 * @file le_flash.c
 *
 * Implementation of flash definition
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */
#include "legato.h"
#include "interfaces.h"
#include "pa_fwupdate.h"
#include "pa_flash.h"

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of partitions references.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_PARTITION_REF             18

//--------------------------------------------------------------------------------------------------
/**
 * Event ID on bad image notification.
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t BadImageEventId = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Internal request counter by client.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_msg_SessionRef_t client;           ///< Client session reference
    bool                isRequested;      ///< true if a request access was already done by client
}
Request_t;

//--------------------------------------------------------------------------------------------------
/**
 * Internal partition information and descriptor to access the Flash MTD or UBI.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_flash_PartitionRef_t ref;           ///< Reference for this
    le_msg_SessionRef_t     client;        ///< Client owner
    pa_flash_Desc_t         desc;          ///< Descriptor for PA flash calls
    int                     mtdNum;        ///< MTD number of the partition
    int                     ubiVolume;     ///< UBI volume number inside the UBI partition
    int                     ubiVolSize;    ///< UBI volume size to be set when UBI is closed
    uint32_t                ubiVolNb;      ///< UBI volume number present on the partition
    char                    ubiVolNames[PA_FLASH_UBI_MAX_VOLUMES][PA_FLASH_UBI_MAX_VOLUMES];
                                           ///< UBI volumes name currently present in partition
    uint32_t                mode;          ///< Current open mode
    char                    partitionName[LE_FLASH_PARTITION_NAME_MAX_BYTES];
                                           ///< MTD name
    bool                    isLogical;     ///< True if it is a "logical" partition
    bool                    isDual;        ///< True if it is the dual for the logical partition
    bool                    isRead;        ///< True if the partition is open for reading
    bool                    isWrite;       ///< True if the partition is open for writting
    bool                    isUbi;         ///< True if the partition is open for UBI access
    le_dualsys_System_t     systemMask;    ///< System owning the partion, (modem, lk or linux)
    pa_flash_Info_t*        mtdInfo;       ///< Global MTD information
    pa_flash_EccStats_t     statsAtOpen;   ///< ECC stats and bad block at open time
}
Partition_t;

//--------------------------------------------------------------------------------------------------
/**
 * Memory pool for allocating partitions ref.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t PartitionPool = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * A map of safe references to partitions ref.
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t PartitionRefMap = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Memory pool for allocating request count by client.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t RequestPool = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Hash map of request count by client.
 */
//--------------------------------------------------------------------------------------------------
static le_hashmap_Ref_t RequestHashMap = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * A counter to follow the number of FlashRequest() [+1] and FlashRelease() [-1] calls.
 */
//--------------------------------------------------------------------------------------------------
static uint32_t FlashRequestedCount = 0;

//--------------------------------------------------------------------------------------------------
/**
 * The currently active systems mask: Modem, LK and Linux
 */
//--------------------------------------------------------------------------------------------------
static le_dualsys_System_t ActiveSystemMask = 0;

//--------------------------------------------------------------------------------------------------
// Private functions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * First layer Bad image handler
 */
//--------------------------------------------------------------------------------------------------
static void BadImageHandler
(
    void *reportPtr,       ///< [IN] Pointer to the event report payload.
    void *secondLayerFunc  ///< [IN] Address of the second layer handler function.
)
{
    le_flash_BadImageDetectionHandlerFunc_t clientHandlerFunc = secondLayerFunc;

    char *imageName = (char*)reportPtr;

    LE_DEBUG("Call client handler bad image name '%s'", imageName);

    clientHandlerFunc(imageName, le_event_GetContextPtr());
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialization function
 */
//--------------------------------------------------------------------------------------------------
static void Init
(
    void
)
{
    BadImageEventId = le_event_CreateId("BadImageEvent", LE_FLASH_IMAGE_NAME_MAX_BYTES);
}

//--------------------------------------------------------------------------------------------------
/**
 * Parse the partition name using the following rules:
 *     - "_1" belongs to system 1
 *     - "_2" belongs to system 2
 *     - "_active" belongs to the current active partition, ie, system 1 or 2
 *     - "_update" belongs to the current upade partition, ie, system 1 or 2
 *     - if no "_" is found in the partition name, it is assumed that this name is the real
 *       partition name
 *
 * Set the real partition name and the MTD number for this partition into the partPrt
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If the partition name is not correct
 *      - LE_FAULT         If the /proc/mtd cannot be open or read
 *      - LE_NOT_FOUND     If the given partition name does not match any real partition
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ParsePartitionNameAndGetMtd
(
    const char* partitionName,
    Partition_t* partPtr
)
{
    char* namePtr = partPtr->partitionName;
    char* markPtr;
    size_t nameLen;
    FILE* mtdFdPtr;
    char mtdBuf[PATH_MAX];
    size_t mtdBufLen;
    int mtdNum = -1;
    int inSystem = 0;
    int iPart;
    pa_fwupdate_MtdPartition_t *mtdPartPtr;

    if (NULL == partitionName)
    {
        return LE_BAD_PARAMETER;
    }

    if (LE_OK != pa_fwupdate_GetMtdPartitionTab(&mtdPartPtr))
    {
        LE_ERROR("Unable to get the MTD partition table");
        return LE_FAULT;
    }

    memset(namePtr, 0, LE_FLASH_PARTITION_NAME_MAX_BYTES);
    snprintf(namePtr, LE_FLASH_PARTITION_NAME_MAX_BYTES, "%s", partitionName);
    markPtr = strchr(namePtr, '_');
    if (markPtr)
    {
        *(markPtr++) = '\0';
    }
    for (iPart = 0; mtdPartPtr[iPart].name; iPart++)
    {
        if (0 == strcmp(mtdPartPtr[iPart].name, namePtr))
        {
            partPtr->systemMask = mtdPartPtr[iPart].systemMask;
            partPtr->isLogical = mtdPartPtr[iPart].isLogical;
            break;
        }
    }
    if (NULL == mtdPartPtr[iPart].name)
    {
        LE_WARN("Partiton \"%s\" is not in dual system table. Assuming full name\n",
                partitionName);
        // Assume this is a "customer partition". So use the whole name passed into argument.
        snprintf(namePtr, LE_FLASH_PARTITION_NAME_MAX_BYTES, "%s", partitionName);
    }
    else
    {
        if (markPtr)
        {
            if (0 == strcmp("1", markPtr))
            {
                inSystem = 0;
            }
            else if (0 == strcmp("2", markPtr))
            {
                inSystem = 1;
            }
            else if (0 == strcmp("active", markPtr))
            {
                inSystem = (ActiveSystemMask & (1 << partPtr->systemMask) ? 1 : 0);
            }
            else if (0 == strcmp("update", markPtr))
            {
                inSystem = (ActiveSystemMask & (1 << partPtr->systemMask) ? 0 : 1);
            }
            else
            {
                LE_ERROR("Badly formed partition name \"%s\"", partitionName);
                return LE_BAD_PARAMETER;
            }
        }

        partPtr->isDual = (((partPtr->isLogical) && (1 == inSystem)) ? true : false);
        memset(namePtr, 0, LE_FLASH_PARTITION_NAME_MAX_BYTES);
        snprintf(namePtr, LE_FLASH_PARTITION_NAME_MAX_BYTES, "%s",
                 mtdPartPtr[iPart].systemName[inSystem]);
    }
    nameLen = strlen(namePtr);

    LE_DEBUG("Fetching partiton \"%s\"", namePtr);
    // Open the /proc/mtd partition
    if (NULL == (mtdFdPtr = fopen( "/proc/mtd", "r" )))
    {
        LE_ERROR( "fopen on /proc/mtd failed: %m" );
        return LE_FAULT;
    }

    // Read all entries until the partition names match
    while (fgets(mtdBuf, sizeof(mtdBuf), mtdFdPtr ))
    {
        // This is the fetched partition
        mtdBufLen = strlen(mtdBuf);
        markPtr = strchr(mtdBuf, '"');
        if ((markPtr) && ((mtdBufLen + 6) > nameLen) &&
            (0 == strncmp( markPtr + 1, namePtr, nameLen )) &&
            ('"' == markPtr[nameLen + 1]))
        {
            // Get the MTD number
            if (1 != sscanf( mtdBuf, "mtd%d", &mtdNum ))
            {
                LE_ERROR( "Unable to scan the mtd number in %s", mtdBuf );
            }
            else
            {
                partPtr->mtdNum = mtdNum;
                LE_DEBUG( "Partition %s is mtd%d", namePtr, partPtr->mtdNum );
            }
            break;
        }
    }
    fclose( mtdFdPtr );

    return -1 == mtdNum ? LE_NOT_FOUND : LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Parse the UBI volume name and return the UBI volume id.
 * Set the real partition name and the MTD number for this partition into the partPrt
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If the UBI volume name is not correct
 *      - LE_FAULT         If the UBI volume table cannot be read
 *      - LE_NOT_FOUND     If the given UBI volume name does not match any volume
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ParseUbiVolumeNameAndGetVolId
(
    const char* volumeName,
    Partition_t* partPtr
)
{
    int iUbiVol;

    partPtr->ubiVolume = -1;
    for (iUbiVol = 0; iUbiVol < PA_FLASH_UBI_MAX_VOLUMES; iUbiVol++)
    {
        char *namePtr = partPtr->ubiVolNames[iUbiVol];

        if (*namePtr)
        {
            if (0 == strcmp(namePtr, volumeName))
            {
                partPtr->ubiVolume = iUbiVol;
                LE_INFO("UBI volume name '%s', volume id %d", namePtr, partPtr->ubiVolume);
                break;
            }
        }
    }
    return -1 != partPtr->ubiVolume ? LE_OK : LE_NOT_FOUND;
}

//--------------------------------------------------------------------------------------------------
/**
 * Open the MTD device and fill the Partition_t structure with all parameters for all functions.
 * Create a new reference for the calling client and register this reference to the list.
 * The MTD and UBI volume (if set) will be close at call of le_flash_Close() or if the client
 * disconnects.
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If the partition name is not correct
 *      - LE_FAULT         If the /proc/mtd cannot be open or read
 *      - LE_NOT_FOUND     If the given partition name does not match any real partition
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t Open
(
    const char*              partitionName,  ///< [IN] Partition to be opened.
    uint32_t                 mode,           ///< [IN] Opening mode.
    bool                     isUbiToCreate,  ///< [IN] Create an UBI partition
    bool                     isForcedCreate, ///< [IN] Force the UBI recreation and
                                             ///<      overwrite the previous content.
    le_flash_PartitionRef_t* partitionRef    ///< [OUT] Partition reference.
)
{
    Partition_t* partPtr;
    le_result_t  res;

    partPtr = le_mem_ForceAlloc(PartitionPool);
    memset(partPtr, 0, sizeof(Partition_t));
    res = ParsePartitionNameAndGetMtd(partitionName, partPtr);
    if (LE_OK != res)
    {
        goto err;
    }

    partPtr->isRead = mode & (PA_FLASH_OPENMODE_READONLY | PA_FLASH_OPENMODE_READWRITE);
    partPtr->isWrite = mode & (PA_FLASH_OPENMODE_WRITEONLY | PA_FLASH_OPENMODE_READWRITE);
    mode |= PA_FLASH_OPENMODE_MARKBAD;
    if (partPtr->isLogical)
    {
        mode |= PA_FLASH_OPENMODE_LOGICAL;
        if (partPtr->isDual)
        {
            mode |= PA_FLASH_OPENMODE_LOGICAL_DUAL;
        }
    }
    partPtr->isUbi = (mode & PA_FLASH_OPENMODE_UBI ? true : false);
    if ((partPtr->isUbi) && (mode & PA_FLASH_OPENMODE_WRITEONLY))
    {
        // Force the open of PA to be read-write as the low layer requires to have read access
        // for block header handling.
        mode = ((mode & ~(PA_FLASH_OPENMODE_WRITEONLY)) | PA_FLASH_OPENMODE_READWRITE);
    }
    res = pa_flash_Open(partPtr->mtdNum,
                        mode,
                        &partPtr->desc,
                        &partPtr->mtdInfo);
    if (LE_OK != res)
    {
        goto err;
    }

    if (isUbiToCreate)
    {
        res = pa_flash_CreateUbi(partPtr->desc, isForcedCreate);
        if (LE_OK != res)
        {
            LE_ERROR("Failed to create UBI partition on MTD%d: %d", partPtr->mtdNum, res);
            goto closeerr;
        }
    }

    partPtr->ubiVolume = -1;
    partPtr->ubiVolSize = -1;
    partPtr->mode = mode;
    if (partPtr->isUbi)
    {
        res = pa_flash_ScanUbiForVolumes(partPtr->desc, &partPtr->ubiVolNb, partPtr->ubiVolNames);
    }
    else
    {
        res = pa_flash_Scan(partPtr->desc, NULL);
    }
    if (LE_OK != res)
    {
        goto closeerr;
    }

    res = pa_flash_GetEccStats(partPtr->desc, &partPtr->statsAtOpen);
    if (LE_OK != res)
    {
        goto closeerr;
    }

    partPtr->client = le_flash_GetClientSessionRef();
    partPtr->ref = le_ref_CreateRef(PartitionRefMap, partPtr);
    *partitionRef = partPtr->ref;

    return LE_OK;

closeerr:
    pa_flash_Close(partPtr->desc);
err:
    LE_ERROR("Failed to open partition \"%s\" MTD %d with mode %x: %d",
             partPtr->partitionName, partPtr->mtdNum, mode, res);
    le_mem_Release(partPtr);
    *partitionRef = NULL;
    return ((LE_NOT_FOUND != res) && (LE_BAD_PARAMETER != res) &&
            (LE_DUPLICATE != res) ? LE_FAULT : res);
}

//--------------------------------------------------------------------------------------------------
/**
 * Close the an UBI volume and adjust UBIvolume size if needed (write mode).
 *
 * @return
 *      - LE_OK            On success
 *      - LE_FAULT         On errors
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t CloseUbiVolume
(
    Partition_t* partPtr    ///< [IN] Partition descriptor
)
{
    le_result_t res;

    // Need to call pa_flash_AdjustUbiSize() to reduce volume space?
    if ((partPtr->isWrite) && (-1 != partPtr->ubiVolSize))
    {
        res = pa_flash_AdjustUbiSize(partPtr->desc, partPtr->ubiVolSize);
        if (LE_OK != res)
        {
            LE_ERROR("Failed to resize UBI volume %d to size %d partition \"%s\" MTD %d: %d",
                     partPtr->ubiVolume, partPtr->ubiVolSize,
                     partPtr->partitionName, partPtr->mtdNum, res);
        }
    }

    res = pa_flash_UnscanUbi(partPtr->desc);
    if (LE_OK != res)
    {
        LE_ERROR("Failed to unscan UBI volume %d partition \"%s\" MTD %d: %d",
                 partPtr->ubiVolume, partPtr->partitionName, partPtr->mtdNum, res);
        res = LE_FAULT;
    }
    partPtr->ubiVolume = -1;
    partPtr->ubiVolSize = -1;
    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * Close the partiton. If an UBI volume is currently open, it will close it also and adjust UBI
 * volume size if needed (write mode).
 * Release the whole Partition_t structure and any dynamic data. Once done, the reference will be
 * invalid.
 *
 * @return
 *      - LE_OK            On success
 *      - LE_FAULT         If the /proc/mtd cannot be open or read
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t Close
(
    Partition_t* partPtr    ///< [IN] Partition descriptor
)
{
    le_result_t res;

    if ((partPtr->isUbi) && (-1 != partPtr->ubiVolume))
    {
        // The UBI volume is open. Force it to be closed.
        // Discard if errors.
        (void)CloseUbiVolume(partPtr);
    }

    res = pa_flash_Close(partPtr->desc);
    if (LE_OK == res)
    {
        le_ref_DeleteRef(PartitionRefMap, partPtr->ref);
        le_mem_Release(partPtr);
    }
    else
    {
        LE_CRIT("Failed to close partition \"%s\" MTD%d: %d",
                partPtr->partitionName, partPtr->mtdNum, res);
    }
    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the Partition_t structure associated to a Partition reference and check if calling client
 * is owner of it.
 * In case of invalid or not owned reference, the calling client is killed.
 *
 * @return
 *      - A pointer to a Partition_t structure if success
 *      - NULL else
 *
 */
//--------------------------------------------------------------------------------------------------
static Partition_t* GetPartitionFromRef
(
    le_flash_PartitionRef_t partitionRef
)
{
    if (NULL == PartitionRefMap)
    {
        LE_KILL_CLIENT("Error: partition ref map is not initialized; "
                       "partition reference %p is illegal", partitionRef);
        return NULL;
    }

    Partition_t* partPtr = (Partition_t *)le_ref_Lookup(PartitionRefMap, partitionRef);
    if (NULL == partPtr)
    {
        LE_KILL_CLIENT("Error: bad partition reference %p.", partitionRef);
        return NULL;
    }

    if (le_flash_GetClientSessionRef() != partPtr->client)
    {
        LE_KILL_CLIENT("Error: client not owner of this partition reference %p.", partitionRef);
        return NULL;
    }

    return partPtr;
}

//--------------------------------------------------------------------------------------------------
/**
 * A handler for client disconnects which frees all resources associated with the client.
 */
//--------------------------------------------------------------------------------------------------
static void CloseClientPartitions
(
    le_msg_SessionRef_t clientSession,
    void* context
)
{
    le_ref_IterRef_t iter = le_ref_GetIterator(PartitionRefMap);
    Partition_t* partPtr;

    (void)context;
    while (LE_OK == le_ref_NextNode(iter))
    {
        partPtr = (Partition_t *)le_ref_GetValue(iter);
        if ((partPtr) && (clientSession == partPtr->client))
        {
            LE_INFO("Closing partition \"%s\" MTD%d for client %p",
                    partPtr->partitionName, partPtr->mtdNum, partPtr->client);
            Close(partPtr);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * A handler for client disconnects which release its access counts and release the flash access
 * when no access is requested.
 */
//--------------------------------------------------------------------------------------------------
static void ReleaseClientAccess
(
    le_msg_SessionRef_t clientSession,
    void* context
)
{
    Request_t* requestPtr = (Request_t*)le_hashmap_Get(RequestHashMap, clientSession);

    (void)context;
    if ((requestPtr) && (requestPtr->client == clientSession))
    {
        LE_INFO("Releasing %d access for client %p",
                requestPtr->isRequested, requestPtr->client);
        if ((requestPtr->isRequested) && (FlashRequestedCount > 0))
        {
            FlashRequestedCount--;
            if (0 == FlashRequestedCount)
            {
                pa_fwupdate_CompleteUpdate();
            }
        }
        le_hashmap_Remove(RequestHashMap, clientSession);
        le_mem_Release(requestPtr);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialization for Partition
 */
//--------------------------------------------------------------------------------------------------
static void PartitionInit
(
    void
)
{
    if (NULL == PartitionPool)
    {
        PartitionPool = le_mem_CreatePool("Flash Partition Pool", sizeof(Partition_t));
        PartitionRefMap = le_ref_CreateMap("Flash Partition Ref Map", MAX_PARTITION_REF);

        // Register a handler to be notified when clients disconnect
        le_msg_AddServiceCloseHandler(le_flash_GetServiceRef(), CloseClientPartitions, NULL);

        // At start-up, need to release the Flash in case of daemon has requested before to restart.
        // Do not check the result as the Flash may be in use by others cores or processes.
        // This is done inside the COMPONENT_INIT of pa_fwupdate.

        le_dualsys_GetCurrentSystem( &ActiveSystemMask );
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialization for Partition
 */
//--------------------------------------------------------------------------------------------------
static void RequestInit
(
    void
)
{
    if (NULL == RequestPool)
    {
        // Create memory pool request/release flash access
        RequestPool = le_mem_CreatePool("Flash Request Pool",
                                           sizeof(Request_t));
        le_mem_ExpandPool(RequestPool, 1);

        // Create table of Request/Release flash access by client
        RequestHashMap = le_hashmap_Create("Flash Request Hash Map",
                                           31,
                                           le_hashmap_HashVoidPointer,
                                           le_hashmap_EqualsVoidPointer);
        if (NULL == RequestHashMap)
        {
            LE_FATAL("Failed to create client hashmap");
        }

        // Register a handler to be notified when clients disconnect
        le_msg_AddServiceCloseHandler(le_flash_GetServiceRef(), ReleaseClientAccess, NULL);
    }
}

//--------------------------------------------------------------------------------------------------
// APIs
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * This function adds a handler
 */
//--------------------------------------------------------------------------------------------------
le_flash_BadImageDetectionHandlerRef_t le_flash_AddBadImageDetectionHandler
(
    le_flash_BadImageDetectionHandlerFunc_t handlerPtr, ///< [IN] Handler pointer
    void* contextPtr                                    ///< [IN] Associated context pointer
)
{
    if (handlerPtr == NULL)
    {
        LE_ERROR("Bad parameters");
        return NULL;
    }

    if (BadImageEventId == NULL)
    {
        Init();
    }

    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler(
        "BadImageDetectionHandler",
        BadImageEventId,
        BadImageHandler,
        (void*)handlerPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    if (LE_OK != pa_fwupdate_StartBadImageIndication(BadImageEventId))
    {
        return NULL;
    }

    return (le_flash_BadImageDetectionHandlerRef_t)(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function removes a handler...
 */
//--------------------------------------------------------------------------------------------------
void le_flash_RemoveBadImageDetectionHandler
(
    le_flash_BadImageDetectionHandlerRef_t handlerRef   ///< [IN] Connection state handler reference
)
{
    if (BadImageEventId && handlerRef)
    {
        le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
        pa_fwupdate_StopBadImageIndication();
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Request the flash access authorization. This is required to avoid race operations.
 *
 * @return
 *      - LE_OK            On success
 *      - LE_UNAVAILABLE   The flash access is temporarily unavailable
 *      - LE_DUPLICATE     If the a request access for the client was already performed
 *      - LE_FAULT         On failure
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_flash_RequestAccess
(
    void
)
{
    le_result_t res = LE_OK;

    RequestInit();

    le_msg_SessionRef_t clientSession = le_flash_GetClientSessionRef();
    Request_t* requestPtr = (Request_t*)le_hashmap_Get(RequestHashMap, clientSession);

    if (NULL == requestPtr)
    {
        requestPtr = le_mem_ForceAlloc(RequestPool);

        if (NULL == requestPtr)
        {
            LE_ERROR("Failed to allocate memory");
            return LE_FAULT;
        }
        requestPtr->client = clientSession;
        requestPtr->isRequested = false;
        le_hashmap_Put(RequestHashMap, clientSession, requestPtr);
    }

    if (requestPtr->client == clientSession)
    {
        if (requestPtr->isRequested)
        {
            LE_ERROR("Client %p already performed a request access", requestPtr->client);
            return LE_DUPLICATE;
        }
        if (0 == FlashRequestedCount)
        {
            // Request the access to the flash
            res = pa_fwupdate_RequestUpdate();
            LE_INFO("RequestAccess: %s",
                    ((LE_OK == res) ? "allowed" :
                        (LE_UNAVAILABLE == res ? "rejected" : "error")));
            if (LE_OK != res)
            {
                return res;
            }
        }
        requestPtr->isRequested = true;
        le_hashmap_Put(RequestHashMap, clientSession, requestPtr);
        FlashRequestedCount++;
        LE_INFO("Request flash access for client %p: global count %u",
                requestPtr->client, FlashRequestedCount);
    }
    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * Release the flash access requested by le_flash_RequestAccess API.
 *
 * @return
 *      - LE_OK            On success
 *      - LE_FAULT         On failure
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_flash_ReleaseAccess
(
    void
)
{
    le_result_t res = LE_OK;

    RequestInit();

    le_msg_SessionRef_t clientSession = le_flash_GetClientSessionRef();
    Request_t* requestPtr = (Request_t*)le_hashmap_Get(RequestHashMap, clientSession);

    if ((requestPtr) && (requestPtr->client == clientSession))
    {
        LE_INFO("Release flash access for client %p: client request %d global count %u",
                requestPtr->client, requestPtr->isRequested, FlashRequestedCount);
        if (requestPtr->isRequested)
        {
            requestPtr->isRequested = false;
            le_hashmap_Put(RequestHashMap, clientSession, requestPtr);
            if (FlashRequestedCount > 0)
            {
                FlashRequestedCount--;
            }
            if (0 == FlashRequestedCount)
            {
                res = pa_fwupdate_CompleteUpdate();
            }
        }
    }
    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * Open a flash partition at the block layer for the given operation and return a descriptor.
 * The read and write operation will be done using MTD.
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If a parameter is invalid
 *      - LE_NOT_FOUND     If the flash partition is not found
 *      - LE_FAULT         On failure
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_flash_OpenMtd
(
    const char*              partitionName,  ///< [IN] Partition to be opened.
    le_flash_OpenMode_t      mode,           ///< [IN] Opening mode.
    le_flash_PartitionRef_t* partitionRef    ///< [OUT] Partition reference.
)
{
    if (partitionRef == NULL)
    {
        LE_KILL_CLIENT("partitionRef is NULL.");
        return LE_FAULT;
    }

    pa_flash_OpenMode_t openMode;

    PartitionInit();

    switch (mode)
    {
        case LE_FLASH_READ_ONLY:
            openMode = PA_FLASH_OPENMODE_READONLY;
            break;
        case LE_FLASH_WRITE_ONLY:
            openMode = PA_FLASH_OPENMODE_WRITEONLY;
            break;
        case LE_FLASH_READ_WRITE:
            openMode = PA_FLASH_OPENMODE_READWRITE;
            break;
        default:
             return LE_BAD_PARAMETER;
    }
    return Open(partitionName, openMode, false, false, partitionRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Open a UBI volume for the given operation and return a descriptor. The read and write
 * operation will be done using MTD, UBI metadata will be updated.
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If a parameter is invalid
 *      - LE_NOT_FOUND     If the flash partition is not found
 *      - LE_FAULT         On failure
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_flash_OpenUbi
(
    const char*              partitionName,  ///< [IN] Partition to be opened.
    le_flash_OpenMode_t      mode,           ///< [IN] Opening mode.
    le_flash_PartitionRef_t* partitionRef    ///< [OUT] Partition reference.
)
{
    if (partitionRef == NULL)
    {
        LE_KILL_CLIENT("partitionRef is NULL.");
        return LE_FAULT;
    }

    pa_flash_OpenMode_t openMode;

    PartitionInit();

    switch (mode)
    {
        case LE_FLASH_READ_ONLY:
            openMode = PA_FLASH_OPENMODE_READONLY;
            break;
        case LE_FLASH_WRITE_ONLY:
            openMode = PA_FLASH_OPENMODE_WRITEONLY;
            break;
        case LE_FLASH_READ_WRITE:
            openMode = PA_FLASH_OPENMODE_READWRITE;
            break;
        default:
             return LE_BAD_PARAMETER;
    }
    return Open(partitionName, openMode | PA_FLASH_OPENMODE_UBI, false, false, partitionRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Open the UBI volume of an UBI image to be used for the read and write operations. When open for
 * writing and a volumeSize is given, the UBI volume will be adjusted to this size by freeing the
 * PEBs over this size. If the data inside the volume require more PEBs, they will be added
 * by the le_flash_Write() API.
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If a parameter is invalid
 *      - LE_FAULT         On failure
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_flash_OpenUbiVolume
(
    le_flash_PartitionRef_t partitionRef,  ///< [IN] Partition reference.
    const char*             volumeName,    ///< [IN] Volume name to be used.
    int32_t                 volumeSize     ///< [IN] Volume size to set if open for writing.
)
{
    Partition_t *partPtr = GetPartitionFromRef(partitionRef);
    le_result_t res;

    if ((NULL == partPtr ) || (!partPtr->isUbi) || (-1 != partPtr->ubiVolume))
    {
        return LE_BAD_PARAMETER;
    }
    LE_INFO("Opening UBI volume '%s' partition \"%s\" MTD%d",
            volumeName, partPtr->partitionName, partPtr->mtdNum);

    partPtr->ubiVolSize = volumeSize;
    res = ParseUbiVolumeNameAndGetVolId(volumeName, partPtr);
    if (LE_OK != res)
    {
        LE_ERROR("Partition \"%s\" MTD%d: No UBI volume name '%s' matches: %d",
                 partPtr->partitionName, partPtr->mtdNum, volumeName, res);
        return LE_FAULT;
    }

    res = pa_flash_ScanUbi(partPtr->desc, partPtr->ubiVolume);
    if (LE_OK != res)
    {
        LE_ERROR("Partition \"%s\" MTD%d: Scan failed for UBI volume name '%s', volume id %d: %d",
                 partPtr->partitionName, partPtr->mtdNum, volumeName, partPtr->ubiVolume, res);
        return LE_FAULT;
    }

    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * Close the used UBI volume of an UBI image.
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If a parameter is invalid
 *      - LE_FAULT         On failure
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_flash_CloseUbiVolume
(
    le_flash_PartitionRef_t partitionRef  ///< [IN] Partition reference.
)
{
    Partition_t *partPtr = GetPartitionFromRef(partitionRef);

    if ((NULL == partPtr) || (!partPtr->isUbi) || (-1 == partPtr->ubiVolume))
    {
        return LE_BAD_PARAMETER;
    }

    LE_INFO("Closing UBI volume %d partition \"%s\" MTD%d",
            partPtr->ubiVolume, partPtr->partitionName, partPtr->mtdNum);

    return CloseUbiVolume(partPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Close a flash partition
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If a parameter is invalid
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_flash_Close
(
    le_flash_PartitionRef_t partitionRef  ///< [IN] Partition reference to be closed.
)
{
    Partition_t *partPtr = GetPartitionFromRef(partitionRef);

    if (NULL == partPtr)
    {
        return LE_BAD_PARAMETER;
    }

    LE_INFO("Closing partition \"%s\" MTD%d",
            partPtr->partitionName, partPtr->mtdNum);

    return Close(partPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Erase a block inside a flash partition.
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If a parameter is invalid
 *      - LE_FAULT         On other error
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_flash_EraseBlock
(
    le_flash_PartitionRef_t partitionRef, ///< [IN] Partition reference to be closed.
    uint32_t                blockIndex    ///< [IN] Logical block index to be erased.
)
{
    Partition_t *partPtr = GetPartitionFromRef(partitionRef);
    le_result_t res;

    if (NULL == partPtr)
    {
        return LE_BAD_PARAMETER;
    }

    LE_INFO("Erasing block %u in partition \"%s\" MTD%d",
            blockIndex, partPtr->partitionName, partPtr->mtdNum);

    res = pa_flash_EraseBlock(partPtr->desc, blockIndex);
    if (LE_OK != res)
    {
        LE_ERROR("Partition \"%s\" MTD%d: Erase block failed at blockIndex %u: %d",
                 partPtr->partitionName, partPtr->mtdNum, blockIndex, res);
        res = LE_FAULT;
    }
    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read data from a flash partition. The read data are:
 * - at the logical block index given by blockIndex.
 * - the maximum read data length is:
 *      - an erase block size for MTD usage partition
 *      - an erase block size minus 2 pages for UBI partitions
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If a parameter is invalid
 *      - LE_NOT_PERMITTED If the partition is an UBI and no UBI volume has been open
 *      - LE_FAULT         On other error
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_flash_Read
(
    le_flash_PartitionRef_t partitionRef,   ///< [IN] Partition reference to be used.
    uint32_t                blockIndex,     ///< [IN] Logical block index to be read.
    uint8_t*                readData,       ///< [OUT] Data buffer to copy the read data.
    size_t*                 readDataSizePtr ///< [INOUT] Data size to be read/data size really read
)
{
    Partition_t *partPtr = GetPartitionFromRef(partitionRef);
    le_result_t res;
    size_t readSize;

    if ((NULL == partPtr) || !(partPtr->isRead) || (NULL == readData) || (NULL == readDataSizePtr))
    {
        return LE_BAD_PARAMETER;
    }

    if (partPtr->isUbi)
    {
        if (-1 == partPtr->ubiVolume)
        {
            return LE_BAD_PARAMETER;
        }
        readSize = partPtr->mtdInfo->eraseSize - (2 * partPtr->mtdInfo->writeSize);
        if (*readDataSizePtr < readSize)
        {
            readSize = *readDataSizePtr;
        }
        res = pa_flash_ReadUbiAtBlock(partPtr->desc, blockIndex, readData, &readSize);
        if (LE_OK != res)
        {
            LE_ERROR("Ubi Volume %u Partition \"%s\" MTD%d: Read failed at blockIndex %u,"
                     " dataSize %zu: %d",
                     partPtr->ubiVolume, partPtr->partitionName, partPtr->mtdNum, blockIndex,
                     readSize, res);
            res = LE_FAULT;
        }
        *readDataSizePtr = readSize;
    }
    else
    {
        res = pa_flash_ReadAtBlock( partPtr->desc, blockIndex, readData, *readDataSizePtr);
        if (LE_OK != res)
        {
            LE_ERROR("Partition \"%s\" MTD%d: Read failed at blockIndex %u, dataSize %zu: %d",
                     partPtr->partitionName, partPtr->mtdNum, blockIndex, *readDataSizePtr, res);
            res = LE_FAULT;
        }
    }
    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * Write data to a flash partition.
 * - the block is firstly erased, so no call to le_flash_EraseBlock() is needed.
 * - if the erase or the write reports an error, the block is marked "bad" and the write starts
 *   again at the next physical block.
 * - the data are programmed at the logical block index given by blockIndex.
 * - the maximum written data length is:
 *      - an erase block size for MTD usage partition. This is the eraseBlockSize returned by
 *        le_flash_GetInformation().
 *      - an erase block size minus 2 pages for UBI partitions. These are the eraseBlockSize and
 *        pageSize returned by le_flash_GetInformation().
 * If the write addresses an UBI volume and more PEBs are required to write the new data, new PEBs
 * will be added into this volume.
 *
 * @note
 *      The addressed block is automatically erased before to be written.
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If a parameter is invalid
 *      - LE_FAULT         On other error
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_flash_Write
(
    le_flash_PartitionRef_t partitionRef, ///< [IN] Partition reference to be used.
    uint32_t                blockIndex,   ///< [IN] Logical block index to be write.
    const uint8_t*          writeData,    ///< [IN] Data buffer to be written.
    size_t                  writeDataSize ///< [IN] Data size to be written
)
{
    Partition_t *partPtr = GetPartitionFromRef(partitionRef);
    le_result_t res;

    if ((NULL == partPtr) || !(partPtr->isWrite) || (NULL == writeData))
    {
        return LE_BAD_PARAMETER;
    }

    if (partPtr->isUbi)
    {
        if (-1 == partPtr->ubiVolume)
        {
            return LE_BAD_PARAMETER;
        }
        LE_INFO("MTD%d BlockIndex %u WriteDataSize %zu", partPtr->mtdNum, blockIndex, writeDataSize);
        res = pa_flash_WriteUbiAtBlock(partPtr->desc, blockIndex,
                                       (uint8_t*)writeData, writeDataSize, true);
        if (LE_OK != res)
        {
            LE_ERROR("Ubi Volume %u Partition \"%s\" MTD%d: Write failed at blockIndex %u,"
                     " dataSize %zu: %d",
                     partPtr->ubiVolume, partPtr->partitionName, partPtr->mtdNum, blockIndex,
                     writeDataSize, res);
            res = LE_FAULT;
        }
    }
    else
    {
        res = pa_flash_EraseBlock( partPtr->desc, blockIndex );
        if (LE_OK != res)
        {
            LE_ERROR("Partition \"%s\" MTD%d: Erase failed at blockIndex %u",
                     partPtr->partitionName, partPtr->mtdNum, blockIndex);
            return LE_FAULT;
        }
        res = pa_flash_WriteAtBlock( partPtr->desc, blockIndex, (uint8_t*)writeData, writeDataSize);
        if (LE_OK != res)
        {
            LE_ERROR("Partition \"%s\" MTD%d: Write failed at blockIndex %u, dataSize %zu: %d",
                     partPtr->partitionName, partPtr->mtdNum, blockIndex, writeDataSize, res);
            res = LE_FAULT;
        }
    }
    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * Retrieve information about the partition opened: the number of bad blocks found inside the
 * partition, the number of erase blocks, the size of the erase block and the size of the page.
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If a parameter is invalid
 *      - LE_FAULT         On other error
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_flash_GetBlockInformation
(
    le_flash_PartitionRef_t partitionRef,         ///< [IN] Partition reference to be used.
    uint32_t*               badBlocksNumberPtr,   ///< [OUT] Bad blocks number inside the partition
    uint32_t*               eraseBlocksNumberPtr, ///< [OUT] Erase blocks number
    uint32_t*               eraseBlockSizePtr,    ///< [OUT] Erase block
    uint32_t*               pageSizePtr           ///< [OUT] Page size
)
{
    Partition_t *partPtr = GetPartitionFromRef(partitionRef);
    pa_flash_EccStats_t eccStat = { 0, 0, 0 };
    le_result_t res;

    if ((NULL == partPtr) || (NULL == badBlocksNumberPtr) || (NULL == eraseBlocksNumberPtr) ||
        (NULL == eraseBlockSizePtr) || (NULL == pageSizePtr))
    {
        return LE_BAD_PARAMETER;
    }

    res = pa_flash_GetEccStats(partPtr->desc, &eccStat);
    *badBlocksNumberPtr = eccStat.badBlocks;
    *eraseBlocksNumberPtr = partPtr->mtdInfo->nbLeb;
    *eraseBlockSizePtr = partPtr->mtdInfo->eraseSize;
    *pageSizePtr = partPtr->mtdInfo->writeSize;
    return (LE_OK == res ? LE_OK : LE_FAULT);
}

//--------------------------------------------------------------------------------------------------
/**
 * Retrieve information about the UBI volume opened: the number of free blocks for the UBI,
 * the number of currently allocated blocks to the volume and its real size in bytes.
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If a parameter is invalid
 *      - LE_FAULT         On other error
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_flash_GetUbiVolumeInformation
(
    le_flash_PartitionRef_t partitionRef,       ///< [IN] Partition reference to be used.
    uint32_t*               freeBlockNumberPtr, ///< [OUT] Free blocks number on the UBI partition
    uint32_t*               allocatedBlockNumberPtr,
                                                ///< [OUT] Allocated blocks number to the UBI volume
    uint32_t*               sizeInBytesPtr      ///< [OUT] Real size in bytes of the UBI volume
)
{
    Partition_t *partPtr = GetPartitionFromRef(partitionRef);

    if ((NULL == partPtr) || (!partPtr->isUbi) || (-1 == partPtr->ubiVolume) ||
        (NULL == freeBlockNumberPtr) || (NULL == allocatedBlockNumberPtr) ||
        (NULL == sizeInBytesPtr))
    {
        return LE_BAD_PARAMETER;
    }

    return pa_flash_GetUbiInfo(partPtr->desc,
                               freeBlockNumberPtr, allocatedBlockNumberPtr, sizeInBytesPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Create an UBI partition.
 * If the partition is already an UBI, an error is raised except if the flag isForcedCreate is set
 * to true. In this case, the whole UBI partition is recreated and the previous content is lost.
 * If the operation succeed, the partition is opened in write-only and this is not necessary to
 * call le_flash_OpenUbi().
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If a parameter is invalid
 *      - LE_NOT_FOUND     If the flash partition is not found
 *      - LE_DUPLICATE     If the partition is already an UBI partition and isForcedCreate is not
 *                         set to true
 *      - LE_FAULT         On failure
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_flash_CreateUbi
(
    const char*              partitionName,  ///< [IN] Partition to be opened.
    bool                     isForcedCreate, ///< [IN] Force the UBI recreation and
                                             ///<      overwrite the previous content.
    le_flash_PartitionRef_t* partitionRef    ///< [OUT] Partition reference.
)
{
    if (NULL == partitionRef)
    {
        return LE_BAD_PARAMETER;
    }

    PartitionInit();

    return Open(partitionName,
                PA_FLASH_OPENMODE_WRITEONLY | PA_FLASH_OPENMODE_UBI,
                true, isForcedCreate,
                partitionRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Create a new UBI volume into an UBI partition.
 * If the volume name or the volume ID already exists, an error is raised except if the flag
 * isForcedCreate is set to true. In this case, the whole UBI volume is recreated and the previous
 * content is lost. If the operation succeed, UBI volume is opened and this is not necessary to
 * call le_flash_OpenUbiVolume().
 * Note that the UBI partition should be opened in write-only or read-write mode, else an error is
 * raised.
 * The volumeName is the same parameter as le_flash_OpenUbiVolume().
 * A static volume cannot be extended when mounted, so it is generally used for SQUASHFS or others
 * immutables and R/O filesystems. A dynamic volume is extensible like UBIFS volumes.
 * The volume ID is the number of the UBI volume to be created. If set to NO_UBI_VOLUME_ID, the
 * first free volume ID will be used.
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If a parameter is invalid
 *      - LE_NOT_PERMITTED If the UBI partition is not opened in write-only or read-write mode
 *      - LE_DUPLICATE     If the UBI volume already exists with a same name or a same volume ID
 *                         and isForcedCreate is not set to true
 *      - LE_FAULT         On failure
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_flash_CreateUbiVolume
(
    le_flash_PartitionRef_t  partitionRef,   ///< [IN] Partition reference.
    bool                     isForcedCreate, ///< [IN] Force the UBI volume recreation and
                                             ///<      overwrite the previous content.
    uint32_t                 volumeId,       ///< [IN] Volume ID to set.
    le_flash_UbiVolumeType_t volumeType,     ///< [IN] Volume type to set.
    const char*              volumeName,     ///< [IN] Volume name to be created.
    int32_t                  volumeSize      ///< [IN] Volume size to set.
)
{
    Partition_t *partPtr = GetPartitionFromRef(partitionRef);
    int iUbiVol;
    le_result_t res;

    if ((NULL == partPtr) || !(partPtr->isWrite) ||
        !(partPtr->isUbi) || (-1 != partPtr->ubiVolume) ||
        ((volumeId != (uint32_t)LE_FLASH_UBI_VOL_NO_ID) && (volumeId > LE_FLASH_UBI_VOL_ID_MAX)))
    {
        return LE_BAD_PARAMETER;
    }

    LE_INFO("Creating UBI volume '%s' type %u partition \"%s\" MTD%d",
            volumeName, volumeType, partPtr->partitionName, partPtr->mtdNum);

    res = ParseUbiVolumeNameAndGetVolId(volumeName, partPtr);
    partPtr->ubiVolume = -1;
    if (LE_OK == res)
    {
        if (!isForcedCreate)
        {
            LE_ERROR("Partition \"%s\" MTD%d: UBI volume name '%s' already exists",
                     partPtr->partitionName, partPtr->mtdNum, volumeName);
            return LE_DUPLICATE;
        }
        res = le_flash_DeleteUbiVolume(partitionRef, volumeName);
        if (LE_OK != res)
        {
          LE_ERROR("Partition \"%s\" MTD%d: Failed to delete volume '%s': res=%d",
                     partPtr->partitionName, partPtr->mtdNum, volumeName, res);
          return LE_FAULT;
        }
    }
    if ((volumeId <= LE_FLASH_UBI_VOL_ID_MAX) && (*partPtr->ubiVolNames[volumeId]))
    {
        if (!isForcedCreate)
        {
          LE_ERROR("Partition \"%s\" MTD%d: UBI volume Id %d already created by volume %s",
                   partPtr->partitionName, partPtr->mtdNum, volumeId,
                   partPtr->ubiVolNames[volumeId]);
          return LE_DUPLICATE;
        }
        res = le_flash_DeleteUbiVolume(partitionRef, partPtr->ubiVolNames[volumeId]);
        if (LE_OK != res)
        {
          LE_ERROR("Partition \"%s\" MTD%d: Failed to delete volume '%s': res=%d",
                   partPtr->partitionName, partPtr->mtdNum, partPtr->ubiVolNames[volumeId], res);
          return LE_FAULT;
        }
    }

    if ((uint32_t)LE_FLASH_UBI_VOL_NO_ID == volumeId)
    {
        for (iUbiVol = 0; iUbiVol < PA_FLASH_UBI_MAX_VOLUMES; iUbiVol++)
        {
            if ('\0' == *partPtr->ubiVolNames[iUbiVol])
            {
                volumeId = partPtr->ubiVolume = iUbiVol;
                LE_INFO("UBI volume id %d is allocated", partPtr->ubiVolume);
                break;
            }
        }
    }
    res = pa_flash_CreateUbiVolume( partPtr->desc,
                                    volumeId,
                                    volumeName,
                                    (LE_FLASH_DYNAMIC == volumeType ? PA_FLASH_VOLUME_DYNAMIC
                                                                    : PA_FLASH_VOLUME_STATIC),
                                    (LE_FLASH_DYNAMIC == volumeType ? volumeSize
                                                                    : LE_FLASH_UBI_VOL_NO_SIZE) );
    if (LE_OK != res)
    {
        return res;
    }

    partPtr->ubiVolSize = (LE_FLASH_DYNAMIC == volumeType ? volumeSize
                                                          : LE_FLASH_UBI_VOL_NO_SIZE);
    partPtr->ubiVolume = volumeId;
    // Copy the created volume name into the partitionRef
    strncpy(partPtr->ubiVolNames[partPtr->ubiVolume], volumeName, PA_FLASH_UBI_MAX_VOLUMES);
    res = pa_flash_ScanUbi(partPtr->desc, partPtr->ubiVolume);
    if (LE_OK != res)
    {
        LE_ERROR("Partition \"%s\" MTD%d: Scan failed for UBI volume name '%s', volume id %d: %d",
                 partPtr->partitionName, partPtr->mtdNum, volumeName, partPtr->ubiVolume, res);
        return LE_FAULT;
    }

    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * Delete an UBI volume from an UBI partition.
 * If the volume is currently opened by le_flash_OpenUbiVolume(), it is closed first.
 * Note that the UBI partition should be opened in write-only or read-write mode, else an error is
 * raised.
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If a parameter is invalid
 *      - LE_NOT_PERMITTED If the UBI partition is not open in write-only or read-write mode
 *      - LE_NOT_FOUND     If the volume name is not found
 *      - LE_FAULT         On failure
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_flash_DeleteUbiVolume
(
    le_flash_PartitionRef_t partitionRef,   ///< [IN] Partition reference.
    const char*             volumeName      ///< [IN] Volume name to be deleted.
)
{
    Partition_t *partPtr = GetPartitionFromRef(partitionRef);
    le_result_t res;

    if ((NULL == partPtr) || !(partPtr->isWrite) ||
        !(partPtr->isUbi) || (-1 != partPtr->ubiVolume))
    {
        return LE_BAD_PARAMETER;
    }

    res = ParseUbiVolumeNameAndGetVolId(volumeName, partPtr);
    if (LE_OK != res)
    {
        LE_ERROR("Partition \"%s\" MTD%d: No UBI volume name '%s' matches: %d",
                 partPtr->partitionName, partPtr->mtdNum, volumeName, res);
        return LE_FAULT;
    }

    res = pa_flash_DeleteUbiVolume(partPtr->desc, partPtr->ubiVolume);
    if (LE_OK != res)
    {
        LE_ERROR("Partition \"%s\" MTD%d: Delete failed for UBI volume name '%s', volume id %d: %d",
                 partPtr->partitionName, partPtr->mtdNum, volumeName, partPtr->ubiVolume, res);
        return (LE_NOT_FOUND == res ? res : LE_FAULT);
    }

    // Remove the volume name from the partitionRef
    memset(partPtr->ubiVolNames[partPtr->ubiVolume], 0, PA_FLASH_UBI_MAX_VOLUMES);

    return LE_OK;
}
