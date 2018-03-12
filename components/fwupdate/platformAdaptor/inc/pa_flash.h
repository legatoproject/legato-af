/**
 * @file pa_flash.h
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#ifndef LEGATO_PA_FLASH_INCLUDE_GUARD
#define LEGATO_PA_FLASH_INCLUDE_GUARD

#include "legato.h"

// Physical & Logical partitions:
// Physical partition is a "whole" flash partition
// Logical partition is a physical partition "logically" splitted into two partitions
//     - The first from 0 to (PartitionSize / 2) - 1
//     - The second (dual) from PartitionSize / 2 to PartitionSize
//
//     Physical     Logical
//     +------+     +------+
//     |      |     |      |
//     |      |     |______|
//     |      |     |      |
//     |      |     | DUAL |
//     +------+     +------+
//

// PEB (physical erase block) and LEB (logical erase block):
// PEB are physical blocks inside a flash partition. The first is 0 and the last is N
// if a partition stands with N+1 erase blocks.
// LEB are referencing PEB in a "continous" order, even if PEB are not in the sorted
// order, or if they are between bad blocks. For example, a partition with 8 PEB
// and 3 bad blocks (2, 3 and 5), will be in LEB view.
//     LEB 0 = PEB 0
//     LEB 1 = PEB 1
//     LEB 2 = PEB 4
//     LEB 3 = PEB 6
//     LEB 4 = PEB 7
// The number of LEB decreases when a bad block is found or marked.
// A flash partition is opened in PEB accessed until a call to pa_flash_Scan is done.
// After this call the partition is accessed in LEB.
// To go back to a PEB access, a call to pa_flash_Unscan is mandatory
//

//--------------------------------------------------------------------------------------------------
/**
 * Define the open mode options and type for pa_flash_Open
 * Open mode: Read-Only (No write allowed)
 *            Write-Only (No read allowed)
 *            Read-and-write (read or/and write allowed)
 */
//--------------------------------------------------------------------------------------------------
#define PA_FLASH_OPENMODE_READONLY          0x1U ///< Mode for Read-Only
#define PA_FLASH_OPENMODE_WRITEONLY         0x2U ///< Mode for Write-Only
#define PA_FLASH_OPENMODE_READWRITE         0x4U ///< Mode for Read-and-Write
#define PA_FLASH_OPENMODE_LOGICAL          0x10U ///< This is a "logical" partition
#define PA_FLASH_OPENMODE_LOGICAL_DUAL     0x30U ///< This is a "logical and dual" partition
#define PA_FLASH_OPENMODE_UBI              0x40U ///< Mode for UBI block management
#define PA_FLASH_OPENMODE_MARKBAD          0x80U ///< Mark bad block and use next block

//--------------------------------------------------------------------------------------------------
/**
 * Open mode bits type by doing a bit-wise or of several values listed above
 */
//--------------------------------------------------------------------------------------------------
typedef unsigned int pa_flash_OpenMode_t;

//--------------------------------------------------------------------------------------------------
/**
 * Define the value of erased byte (all bits to 1)
 */
//--------------------------------------------------------------------------------------------------
#define PA_FLASH_ERASED_VALUE  0xFFU

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of volume ID (from 0 to 127)
 */
//--------------------------------------------------------------------------------------------------
#define PA_FLASH_UBI_MAX_VOLUMES  128

//--------------------------------------------------------------------------------------------------
/**
 * Define the maximum number of LEB (Logical Erase Block)
 */
//--------------------------------------------------------------------------------------------------
#define PA_FLASH_MAX_LEB 2048

//--------------------------------------------------------------------------------------------------
/**
 * Define the maximum length of a partition name
 */
//--------------------------------------------------------------------------------------------------
#define PA_FLASH_MAX_INFO_NAME (128)

//--------------------------------------------------------------------------------------------------
/**
 * Define the type of UBI volume: dynamic for UBIFS, static for SQUASHFS
 */
//--------------------------------------------------------------------------------------------------
#define PA_FLASH_VOLUME_DYNAMIC    1
#define PA_FLASH_VOLUME_STATIC     2

//--------------------------------------------------------------------------------------------------
/**
 * LEB to PEB translation array
 * Map of logical erase block (LEB) to physical erase block (PEB)
 * If a bad block is found, the PEB is incremented, but not the LEB
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint32_t lebToPeb[PA_FLASH_MAX_LEB]; ///< PEB corresponding to LEB index
}
pa_flash_LebToPeb_t;

//--------------------------------------------------------------------------------------------------
/**
 * Information of a flash partition
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint32_t size;            ///< Total size of the partition, in bytes.
    uint32_t writeSize;       ///< Minimal writable flash unit size i.e. min I/O size.
    uint32_t eraseSize;       ///< Erase block size for the device.
    uint32_t startOffset;     ///< In case of logical partition, the offset in the physical partition
    uint32_t nbBlk;           ///< number of physical blocks
    uint32_t nbLeb;           ///< number of logical blocks (= nbBlk until pa_flash_Scan is called)
    bool     logical;         ///< flag for logical partitions
    bool     ubi;             ///< flag for UBI management on physical partition
    uint32_t ubiPebFreeCount; ///< Free UBI PEB counter, available only if ubi is true
    size_t   ubiVolFreeSize;  ///< Free size for an UBI volume, available only if ubi is true
    char     name[PA_FLASH_MAX_INFO_NAME];
                              ///< name of the partition
}
pa_flash_Info_t;

//--------------------------------------------------------------------------------------------------
/**
 * ECC and bad blocks statistics
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint32_t corrected;    ///< Number of bits corrected for ECC
    uint32_t failed;       ///< Number of unrecoverable error for ECC
    uint32_t badBlocks;    ///< Number of bad blocks currently marked
}
pa_flash_EccStats_t;

//--------------------------------------------------------------------------------------------------
/**
 * Flash opaque descriptor for flash operation access
 */
//--------------------------------------------------------------------------------------------------
typedef void *pa_flash_Desc_t;

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
LE_SHARED le_result_t pa_flash_GetInfo
(
    int partNum,              ///< [IN] Partition number
    pa_flash_Info_t *infoPtr, ///< [IN] Pointer to copy the flash information
    bool isLogical,           ///< [IN] Logical partition
    bool isDual               ///< [IN] Dual of a logical partition
);

//--------------------------------------------------------------------------------------------------
/**
 * Retrieve flash information of opening a flash device
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If desc is NULL or not a valid flash descriptor or infoPtr is NULL
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_flash_RetrieveInfo
(
    pa_flash_Desc_t desc,     ///< [IN] Private flash descriptor
    pa_flash_Info_t **infoPtr ///< [IN] Pointer to copy the flash information
);

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
LE_SHARED le_result_t pa_flash_GetEccStats
(
    pa_flash_Desc_t      desc,       ///< [IN] Private flash descriptor
    pa_flash_EccStats_t *eccStatsPtr ///< [IN] Pointer to copy the ECC and bad blocks statistics
);

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
LE_SHARED le_result_t pa_flash_Open
(
    int partNum,              ///< [IN] Partition number
    pa_flash_OpenMode_t mode, ///< [IN] Open mode for this flash partition
    pa_flash_Desc_t *descPtr, ///< [OUT] Private flash descriptor
    pa_flash_Info_t **infoPtr ///< [OUT] Pointer to the flash information (may be NULL)
);

//--------------------------------------------------------------------------------------------------
/**
 * Close a flash descriptor
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If desc is NULL or not a valid flash descriptor
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_flash_Close
(
    pa_flash_Desc_t desc      ///< [IN] Private flash descriptor
);

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
LE_SHARED le_result_t pa_flash_Scan
(
    pa_flash_Desc_t desc,     ///< [IN] Private flash descriptor
    pa_flash_LebToPeb_t **lebToPebPtr
                              ///< [OUT] Pointer to a LEB to PEB table (may be NULL)
);

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
LE_SHARED le_result_t pa_flash_Unscan
(
    pa_flash_Desc_t desc      ///< [IN] Private flash descriptor
);

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
LE_SHARED le_result_t pa_flash_CheckBadBlock
(
    pa_flash_Desc_t desc,     ///< [IN] Private flash descriptor
    uint32_t blockIndex,      ///< [IN] PEB or LEB to be checked
    bool *isBadBlockPtr       ///< [OUT] true if bad block, false else
);

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
LE_SHARED le_result_t pa_flash_MarkBadBlock
(
    pa_flash_Desc_t desc,     ///< [IN] Private flash descriptor
    uint32_t blockIndex       ///< [IN] PEB or LEB to be marked bad
);

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
LE_SHARED le_result_t pa_flash_EraseBlock
(
    pa_flash_Desc_t desc,     ///< [IN] Private flash descriptor
    uint32_t blockIndex       ///< [IN] PEB or LEB to erase
);

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
LE_SHARED le_result_t pa_flash_SeekAtOffset
(
    pa_flash_Desc_t desc,     ///< [IN] Private flash descriptor
    off_t offset              ///< [IN] Physical or Logical offset to seek
);

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
LE_SHARED le_result_t pa_flash_SeekAtBlock
(
    pa_flash_Desc_t desc,     ///< [IN] Private flash descriptor
    uint32_t blockIndex       ///< [IN] PEB or LEB to seek
);

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
LE_SHARED le_result_t pa_flash_Read
(
    pa_flash_Desc_t desc,     ///< [IN] Private flash descriptor
    uint8_t *dataPtr,         ///< [IN] Pointer to data to be read
    size_t dataSize           ///< [IN] Size of data to read
);

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
LE_SHARED le_result_t pa_flash_Write
(
    pa_flash_Desc_t desc,     ///< [IN] Private flash descriptor
    uint8_t *dataPtr,         ///< [IN] Pointer to data to be written
    size_t dataSize           ///< [IN] Size of data to write
);

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
LE_SHARED le_result_t pa_flash_ReadAtBlock
(
    pa_flash_Desc_t desc,     ///< [IN] Private flash descriptor
    uint32_t blockIndex,      ///< [IN] PEB or LEB to read
    uint8_t *dataPtr,         ///< [IN] Pointer to data to be read
    size_t dataSize           ///< [IN] Size of data to read
);

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
LE_SHARED le_result_t pa_flash_WriteAtBlock
(
    pa_flash_Desc_t desc,     ///< [IN] Private flash descriptor
    uint32_t blockIndex,      ///< [IN] PEB or LEB to write
    uint8_t *dataPtr,         ///< [IN] Pointer to data to be written
    size_t dataSize           ///< [IN] Size of data to write
);

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
LE_SHARED le_result_t pa_flash_CheckUbi
(
    pa_flash_Desc_t desc,    ///< [IN]  Private flash descriptor
    bool *isUbiPtr           ///< [OUT] true if the partition is an UBI container, false otherwise
);

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
LE_SHARED le_result_t pa_flash_ScanUbiForVolumes
(
    pa_flash_Desc_t desc,            ///< [IN] Private flash descriptor
    uint32_t*       ubiVolNumberPtr, ///< [OUT] UBI volume number found
    char            ubiVolName[PA_FLASH_UBI_MAX_VOLUMES][PA_FLASH_UBI_MAX_VOLUMES]
                                     ///< [OUT] UBI volume name array
);

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
LE_SHARED le_result_t pa_flash_ScanUbi
(
    pa_flash_Desc_t desc,     ///< [IN] Private flash descriptor
    uint32_t ubiVolId         ///< [IN] UBI volume ID
);

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
LE_SHARED le_result_t pa_flash_UnscanUbi
(
    pa_flash_Desc_t desc      ///< [IN] Private flash descriptor
);

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
LE_SHARED le_result_t pa_flash_ReadUbiAtBlock
(
    pa_flash_Desc_t desc,     ///< [IN] Private flash descriptor
    uint32_t leb,             ///< [IN] LEB to read
    uint8_t *dataPtr,         ///< [IN] Pointer to data to be read
    size_t *dataSizePtr       ///< [IN][OUT] Pointer to size to read
);

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
LE_SHARED le_result_t pa_flash_WriteUbiAtBlock
(
    pa_flash_Desc_t desc,     ///< [IN] Private flash descriptor
    uint32_t leb,             ///< [IN] LEB to write
    uint8_t *dataPtr,         ///< [IN] Pointer to data to be written
    size_t dataSize,          ///< [IN] Size to be written
    bool isExtendUbiVolume    ///< [IN] True if the volume may be extended by one block if write
                              ///<      is the leb is outside the current volume
);

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
LE_SHARED le_result_t pa_flash_AdjustUbiSize
(
    pa_flash_Desc_t desc,     ///< [IN] Private flash descriptor
    size_t newSize            ///< [IN] Final size of the UBI volume
);

//--------------------------------------------------------------------------------------------------
/**
 * Get UBI volume information
 *
 * @return
 *      - LE_OK            On success
 *      - LE_BAD_PARAMETER If desc is NULL or is not a valid descriptor
 *      - LE_FORMAT_ERROR  If the flash is not in UBI format
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_flash_GetUbiInfo
(
    pa_flash_Desc_t desc,         ///< [IN] Private flash descriptor
    uint32_t*       freeBlockPtr, ///< [OUT] Free blocks number in the UBI partition
    uint32_t*       volBlockPtr,  ///< [OUT] Allocated blocks number belonging to the volume
    uint32_t*       volSizePtr    ///< [OUT] Real volume size
);

//--------------------------------------------------------------------------------------------------
/**
 * pa_flash_CheckUbiMagic - check if the buffer contains the UBI magic number
 *
 * @return
 *      - LE_OK             On success and found the magic number in buffer
 *      - LE_NOT_FOUND      Cannot find the magic number in buffer
 *      - LE_BAD_PARAMETER  If desc is NULL or is not a valid descriptor
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_flash_CheckUbiMagic
(
    void*    data,       ///< [IN] buffer to check
    uint32_t pattern     ///< [IN] the pattern to check
);

//--------------------------------------------------------------------------------------------------
/**
 * pa_flash_CalculateDataLength - calculate how much real data is stored in the buffer
 *
 * This function calculates how much "real data" is stored in @data and
 * returns the length @dataSize (align with pages size). Continuous 0xFF bytes at the end
 * of the buffer are not considered as "real data".
 *
 * @return
 *      - LE_OK             On success
 *      - LE_BAD_PARAMETER  If desc is NULL or is not a valid descriptor
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_flash_CalculateDataLength
(
    int         pageSize,       ///< [IN] min I/O of the device
    const void* data,           ///< [IN] a buffer with the contents of the physical eraseblock
    uint32_t*   dataSize        ///< [INOUT] input : the buffer length
                                ///<         output: real data length align with pages size
);

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
LE_SHARED le_result_t pa_flash_CreateUbi
(
    pa_flash_Desc_t desc,           ///< [IN] Private flash descriptor
    bool            isForcedCreate  ///< [IN] If set to true the UBI partition is overwriten and the
                                    ///<      previous content is lost
);

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
);

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
);

#endif // LEGATO_PA_FLASH_INCLUDE_GUARD
