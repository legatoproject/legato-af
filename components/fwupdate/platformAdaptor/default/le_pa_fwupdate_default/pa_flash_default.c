/**
 * @file pa_flash.c
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include "pa_flash.h"

//--------------------------------------------------------------------------------------------------
/**
 * Public functions for flash access
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Get flash information without opening a flash device
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If infoPtr is NULL
 *      - LE_FAULT         On failure
 *      - LE_UNSUPPORTED   If the flash device informations cannot be read
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_flash_GetInfo
(
    int partNum,              ///< [IN] Partition number
    pa_flash_Info_t *infoPtr, ///< [IN] Pointer to copy the flash information
    bool isLogical,           ///< [IN] Logical partition
    bool isDual               ///< [IN] Dual of a logical partition
)
{
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Retrieve flash information of opening a flash device
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If desc is NULL or not a valid flash descriptor or infoPtr is NULL
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_flash_RetrieveInfo
(
    pa_flash_Desc_t desc,     ///< [IN] Private flash descriptor
    pa_flash_Info_t **infoPtr ///< [IN] Pointer to copy the flash information
)
{
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the ECC and bad blocks statistics
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If desc is NULL or isBadBlockPtr is NULL
 *      - LE_FAULT         On failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_flash_GetEccStats
(
    pa_flash_Desc_t      desc,       ///< [IN] Private flash descriptor
    pa_flash_EccStats_t *eccStatsPtr ///< [IN] Pointer to copy the ECC and bad blocks statistics
)
{
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Open a flash device for the given operation and return a descriptor
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If desc is NULL or if mode is not correct
 *      - LE_FAULT         On failure
 *      - LE_UNSUPPORTED   If the flash device cannot be opened
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_flash_Open
(
    int partNum,              ///< [IN] Partition number
    pa_flash_OpenMode_t mode, ///< [IN] Open mode for this flash partition
    pa_flash_Desc_t *descPtr, ///< [OUT] Private flash descriptor
    pa_flash_Info_t **infoPtr ///< [OUT] Pointer to the flash information (may be NULL)
)
{
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Close a flash descriptor
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If desc is NULL or not a valid flash descriptor
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_flash_Close
(
    pa_flash_Desc_t desc      ///< [IN] Private flash descriptor
)
{
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Scan a flash and produce a list of LEB and PEB. If no bad block is found, LEB = PEB
 * If not called, the functions "work" with PEB
 * After called, the functions "work" with LEB
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If desc is NULL
 *      - LE_FAULT         On failure
 *      - LE_OUT_OF_RANGE  If the partition is too big to fit in LebToPeb array
 *      - LE_IO_ERROR      If a flash IO error occurs
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_flash_Scan
(
    pa_flash_Desc_t desc,     ///< [IN] Private flash descriptor
    pa_flash_LebToPeb_t **lebToPebPtr
                              ///< [OUT] Pointer to a LEB to PEB table (may be NULL)
)
{
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Clear the scanned list of LEB and set all to PEB
 * After called, the functions "work" with PEB
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If desc is NULL
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_flash_Unscan
(
    pa_flash_Desc_t desc      ///< [IN] Private flash descriptor
)
{
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if the given block is marked bad. The isBadBlockPtr is set to true if bad, false if good
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If desc is NULL or isBadBlockPtr is NULL
 *      - LE_FAULT         On failure
 *      - LE_OUT_OF_RANGE  If the block is outside the partition
 *      - LE_NOT_PERMITTED If the LEB is not linked to a PEB
 *      - LE_IO_ERROR      If a flash IO error occurs
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_flash_CheckBadBlock
(
    pa_flash_Desc_t desc,     ///< [IN] Private flash descriptor
    uint32_t blockIndex,      ///< [IN] PEB or LEB to be checked
    bool *isBadBlockPtr       ///< [OUT] true if bad block, false else
)
{
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Mark the given block to bad
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If desc is NULL
 *      - LE_FAULT         On failure
 *      - LE_OUT_OF_RANGE  If the block is outside the partition
 *      - LE_NOT_PERMITTED If the LEB is not linked to a PEB
 *      - LE_IO_ERROR      If a flash IO error occurs
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_flash_MarkBadBlock
(
    pa_flash_Desc_t desc,     ///< [IN] Private flash descriptor
    uint32_t blockIndex       ///< [IN] PEB or LEB to be marked bad
)
{
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Erase the given block. If LE_IO_ERROR is returned, the block * should be assumed as bad
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If desc is NULL
 *      - LE_FAULT         On failure
 *      - LE_OUT_OF_RANGE  If the block is outside the partition
 *      - LE_NOT_PERMITTED If the LEB is not linked to a PEB
 *      - LE_IO_ERROR      If a flash IO error occurs
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_flash_EraseBlock
(
    pa_flash_Desc_t desc,     ///< [IN] Private flash descriptor
    uint32_t blockIndex       ///< [IN] PEB or LEB to erase
)
{
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the current read/write position of the flash to the given offset
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If desc is NULL
 *      - LE_FAULT         On failure
 *      - LE_OUT_OF_RANGE  If the block is outside the partition
 *      - LE_NOT_PERMITTED If the LEB is not linked to a PEB
 *      - LE_IO_ERROR      If a flash IO error occurs
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_flash_SeekAtOffset
(
    pa_flash_Desc_t desc,     ///< [IN] Private flash descriptor
    off_t offset              ///< [IN] Physical or Logical offset to seek
)
{
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the current pointer of the flash to the given block
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If desc is NULL
 *      - LE_FAULT         On failure
 *      - LE_OUT_OF_RANGE  If the block is outside the partition
 *      - LE_NOT_PERMITTED If the LEB is not linked to a PEB
 *      - LE_IO_ERROR      If a flash IO error occurs
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_flash_SeekAtBlock
(
    pa_flash_Desc_t desc,     ///< [IN] Private flash descriptor
    uint32_t blockIndex       ///< [IN] PEB or LEB to seek
)
{
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the data starting at current position. If a Bad block is detected,
 * the error LE_IO_ERROR is returned and operation is aborted.
 * Note that the length should not be greater than eraseSize
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If desc is NULL or dataPtr is NULL
 *      - LE_FAULT         On failure
 *      - LE_OUT_OF_RANGE  If the block is outside the partition
 *      - LE_NOT_PERMITTED If the LEB is not linked to a PEB
 *      - LE_IO_ERROR      If a flash IO error occurs
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_flash_Read
(
    pa_flash_Desc_t desc,     ///< [IN] Private flash descriptor
    uint8_t *dataPtr,         ///< [IN] Pointer to data to be read
    size_t dataSize           ///< [IN] Size of data to read
)
{
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Write the data starting at current position. If a Bad block is detected,
 * the error LE_IO_ERROR is returned and operation is aborted.
 * Note that the block should be erased before the first write (pa_flash_EraseAtBlock)
 * Note that the length should be a multiple of writeSize and should not be greater than eraseSize
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If desc is NULL or dataPtr is NULL
 *      - LE_FAULT         On failure
 *      - LE_OUT_OF_RANGE  If the block is outside the partition
 *      - LE_NOT_PERMITTED If the LEB is not linked to a PEB
 *      - LE_IO_ERROR      If a flash IO error occurs
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_flash_Write
(
    pa_flash_Desc_t desc,     ///< [IN] Private flash descriptor
    uint8_t *dataPtr,         ///< [IN] Pointer to data to be written
    size_t dataSize           ///< [IN] Size of data to write
)
{
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read data starting the given block. If a Bad block is detected,
 * the error LE_IO_ERROR is returned and operation is aborted.
 * Note that the length should not be greater than eraseSize
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If desc is NULL or dataPtr is NULL
 *      - LE_FAULT         On failure
 *      - LE_OUT_OF_RANGE  If the block is outside the partition
 *      - LE_NOT_PERMITTED If the LEB is not linked to a PEB
 *      - LE_IO_ERROR      If a flash IO error occurs
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_flash_ReadAtBlock
(
    pa_flash_Desc_t desc,     ///< [IN] Private flash descriptor
    uint32_t blockIndex,      ///< [IN] PEB or LEB to read
    uint8_t *dataPtr,         ///< [IN] Pointer to data to be read
    size_t dataSize           ///< [IN] Size of data to read
)
{
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Write data starting the given block. If a Bad block is detected,
 * the error LE_IO_ERROR is returned and operation is aborted.
 * Note that the block should be erased before the first write (pa_flash_EraseAtBlock)
 * Note that the length should be a multiple of writeSize and should not be greater than eraseSize
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If desc is NULL or dataPtr is NULL
 *      - LE_FAULT         On failure
 *      - LE_OUT_OF_RANGE  If the block is outside the partition
 *      - LE_NOT_PERMITTED If the LEB is not linked to a PEB
 *      - LE_IO_ERROR      If a flash IO error occurs
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_flash_WriteAtBlock
(
    pa_flash_Desc_t desc,     ///< [IN] Private flash descriptor
    uint32_t blockIndex,      ///< [IN] PEB or LEB to write
    uint8_t *dataPtr,         ///< [IN] Pointer to data to be written
    size_t dataSize           ///< [IN] Size of data to write
)
{
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if the partition is an UBI container and all blocks belonging to this partition are valid.
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If desc is NULL or is not a valid descriptor
 *      - LE_FAULT         On failure
 *      - LE_IO_ERROR      If a flash IO error occurs
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_flash_CheckUbi
(
    pa_flash_Desc_t desc,    ///< [IN]  Private flash descriptor
    bool *isUbiPtr           ///< [OUT] true if the partition is an UBI container, false otherwise
)
{
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Scan an UBI partition for the volumes number and volumes name
 * volume ID.
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If desc is NULL or is not a valid descriptor
 *      - LE_FAULT         On failure
 *      - LE_IO_ERROR      If a flash IO error occurs
 *      - LE_FORMAT_ERROR  If the flash is not in UBI format
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_flash_ScanUbiForVolumes
(
    pa_flash_Desc_t desc,            ///< [IN] Private flash descriptor
    uint32_t*       ubiVolNumberPtr, ///< [OUT] UBI volume number found
    char            ubiVolName[PA_FLASH_UBI_MAX_VOLUMES][PA_FLASH_UBI_MAX_VOLUMES]
                                    ///< [OUT] UBI volume name array
)
{
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Scan a partition for the UBI volume ID given. Update the LebToPeb array field with LEB for this
 * volume ID.
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If desc is NULL or is not a valid descriptor
 *      - LE_FAULT         On failure
 *      - LE_OUT_OF_RANGE  If the UBI volume ID is over its permitted values
 *      - LE_IO_ERROR      If a flash IO error occurs
 *      - LE_FORMAT_ERROR  If the flash is not in UBI format
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_flash_ScanUbi
(
    pa_flash_Desc_t desc,     ///< [IN] Private flash descriptor
    uint32_t ubiVolId         ///< [IN] UBI volume ID
)
{
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Clear the scanned list of an UBI volume ID and reset all LEB to PEB
 * After called, the functions "work" with PEB
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If desc is NULL or is not a valid descriptor
 *      - LE_FORMAT_ERROR  If the flash is not in UBI format
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_flash_UnscanUbi
(
    pa_flash_Desc_t desc      ///< [IN] Private flash descriptor
)
{
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read data from an UBI volume starting the given block. If a Bad block is detected,
 * the error LE_IO_ERROR is returned and operation is aborted.
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If desc is NULL or dataPtr or dataSizePtr is NULL
 *      - LE_FAULT         On failure
 *      - LE_OUT_OF_RANGE  If the block is outside the partition
 *      - LE_NOT_PERMITTED If the LEB is not linked to a PEB
 *      - LE_IO_ERROR      If a flash IO error occurs
 *      - LE_FORMAT_ERROR  If the flash is not in UBI format
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_flash_ReadUbiAtBlock
(
    pa_flash_Desc_t desc,     ///< [IN] Private flash descriptor
    uint32_t leb,             ///< [IN] LEB to read
    uint8_t *dataPtr,         ///< [IN] Pointer to data to be read
    size_t *dataSizePtr       ///< [IN][OUT] Pointer to size to read
)
{
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Write data to an UBI volume starting the given block. If a Bad block is detected,
 * the error LE_IO_ERROR is returned and operation is aborted.
 * Note that the length should be a multiple of writeSize
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If desc is NULL or dataPtr is NULL
 *      - LE_FAULT         On failure
 *      - LE_OUT_OF_RANGE  If the block is outside the partition or no block free to extend
 *      - LE_NOT_PERMITTED If the LEB is not linked to a PEB
 *      - LE_IO_ERROR      If a flash IO error occurs
 *      - LE_FORMAT_ERROR  If the flash is not in UBI format
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_flash_WriteUbiAtBlock
(
    pa_flash_Desc_t desc,     ///< [IN] Private flash descriptor
    uint32_t leb,             ///< [IN] LEB to write
    uint8_t *dataPtr,         ///< [IN] Pointer to data to be written
    size_t dataSize,          ///< [IN][OUT] Size to be written
    bool isExtendUbiVolume    ///< [IN] True if the volume may be extended by one block if write
                              ///<      is the leb is outside the current volume
)
{
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Adjust (reduce) the UBI volume size to the given size.
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If desc is NULL or is not a valid descriptor
 *      - LE_FAULT         On failure
 *      - LE_IO_ERROR      If a flash IO error occurs
 *      - LE_FORMAT_ERROR  If the flash is not in UBI format
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_flash_AdjustUbiSize
(
    pa_flash_Desc_t desc,     ///< [IN] Private flash descriptor
    size_t newSize            ///< [IN] Final size of the UBI volume
)
{
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get UBI volume information
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If desc is NULL or is not a valid descriptor
 *      - LE_FAULT         On failure
 *      - LE_IO_ERROR      If a flash IO error occurs
 *      - LE_FORMAT_ERROR  If the flash is not in UBI format
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_flash_GetUbiInfo
(
    pa_flash_Desc_t desc,         ///< [IN] Private flash descriptor
    uint32_t*       freeBlockPtr, ///< [OUT] Free blocks number in the UBI partition
    uint32_t*       volBlockPtr,  ///< [OUT] Allocated blocks number belonging to the volume
    uint32_t*       volSizePtr    ///< [OUT] Real volume size
)
{
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Create UBI partition
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If desc is NULL or is not a valid descriptor
 *      - LE_BUSY          If desc refers to an UBI volume or an UBI partition
 *      - LE_IO_ERROR      If a flash IO error occurs
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_flash_CreateUbi
(
    pa_flash_Desc_t desc,           ///< [IN] Private flash descriptor
    bool            isForcedCreate  ///< [IN] If set to true the UBI partition is overwriten and the
                                    ///<      previous content is lost
)
{
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Create UBI volume
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If desc is NULL or is not a valid descriptor
 *      - LE_FORMAT_ERROR  If the flash is not in UBI format
 *      - LE_DUPLICATE     If the volume name or volume ID already exists
 *      - LE_IO_ERROR      If a flash IO error occurs
 *      - LE_NO_MEMORY     If a volume requires more PEBs than the partition size
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_flash_CreateUbiVolume
(
    pa_flash_Desc_t desc,      ///< [IN] Private flash descriptor
    uint32_t ubiVolId,         ///< [IN] UBI volume ID
    const char* ubiVolNamePtr, ///< [IN] UBI volume name
    uint32_t ubiVolType,       ///< [IN] UBI volume type: dynamic or static
    uint32_t ubiVolSize        ///< [IN] UBI volume size (for dynamic volumes only)
)
{
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Delete UBI volume
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If desc is NULL or is not a valid descriptor
 *      - LE_FORMAT_ERROR  If the flash is not in UBI format
 *      - LE_NOT_FOUND     If the volume name does not exist
 *      - LE_IO_ERROR      If a flash IO error occurs
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_flash_DeleteUbiVolume
(
    pa_flash_Desc_t desc,     ///< [IN] Private flash descriptor
    uint32_t ubiVolId         ///< [IN] UBI volume ID
)
{
    return LE_UNSUPPORTED;
}
