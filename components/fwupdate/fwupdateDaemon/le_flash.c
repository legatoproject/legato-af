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


//--------------------------------------------------------------------------------------------------
/**
 * Event ID on bad image notification.
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t BadImageEventId = NULL;

//--------------------------------------------------------------------------------------------------
// Private functions
//--------------------------------------------------------------------------------------------------

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
 *      - LE_FAULT         On failure
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_flash_RequestAccess
(
    void
)
{
    return LE_FAULT;
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
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Open a flash partition at the block layer for the given operation and return a descriptor.
 * The read and write operation will be done using MTD.
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If a parameter is invalid
 *      - LE_UNAVAILABLE   The flash access is temporarily unavailable
 *      - LE_UNSUPPORTED   If the flash device cannot be opened
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
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Open a UBI volume for the given operation and return a descriptor. The read and write
 * operation will be done using MTD, UBI metadata will be updated.
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If a parameter is invalid
 *      - LE_UNAVAILABLE   The flash access is temporarily unavailable
 *      - LE_UNSUPPORTED   If the flash device cannot be opened
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
    return LE_FAULT;
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
    return LE_FAULT;
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
    return LE_FAULT;
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
    return LE_FAULT;
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
    return LE_FAULT;
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
    return LE_FAULT;
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
    return LE_FAULT;
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
    return LE_FAULT;
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
    return LE_FAULT;
}
