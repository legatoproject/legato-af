//--------------------------------------------------------------------------------------------------
/**
 * Delta patch utilities
 *
 * Copyright (C) Sierra Wireless Inc.
 **/
//--------------------------------------------------------------------------------------------------

#ifndef PATCH_UTILS_H_INCLUDE_GUARD
#define PATCH_UTILS_H_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Defines some executables requested by the tool
 */
//--------------------------------------------------------------------------------------------------
#define BSDIFF  "bsdiff"
#define HDRCNV  "hdrcnv"
#define IMGDIFF "imgdiff"

//--------------------------------------------------------------------------------------------------
/**
 * Option when no delta is used.
 */
//--------------------------------------------------------------------------------------------------
#define NODIFF "NODIFF000"

//--------------------------------------------------------------------------------------------------
/**
 * Defines the size of the patch segment for binary images
 */
//--------------------------------------------------------------------------------------------------
#define SEGMENT_SIZE        (1024U * 1024U)

//--------------------------------------------------------------------------------------------------
/**
 * Defines the page size of flash device. This is the minimum I/O size for writing
 */
//--------------------------------------------------------------------------------------------------
#define FLASH_PAGESIZE_4K    4096U
#define FLASH_PAGESIZE_2K    2048U

//--------------------------------------------------------------------------------------------------
/**
 * Defines the PEB size of flash device. This is the size of physical erase block (PEB)
 */
//--------------------------------------------------------------------------------------------------
#define FLASH_PEBSIZE_256K  (256 * 1024U)
#define FLASH_PEBSIZE_128K  (128 * 1024U)

//--------------------------------------------------------------------------------------------------
/**
 * Defines the offset and the flag bits used to mark the delta patch
 */
//--------------------------------------------------------------------------------------------------
#define MISC_OPTS_OFFSET     0x17C
#define MISC_OPTS_DELTAPATCH 0x08

//--------------------------------------------------------------------------------------------------
/**
 * Meta structure for the delta patch. A delta patch may be splitted into several patches "segments"
 * Note: Structure shared between architectures: Use uint32_t for all 32-bits fields
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t  diffType[16];   ///< Magic marker to identify the meta patch hader
    uint32_t segmentSize;    ///< Size of a patch segment
    uint32_t numPatches;     ///< Total number of patch segments
    uint16_t ubiVolId;       ///< UBI volume ID if patch concerns an UBI volume, -1 else
    uint8_t  ubiVolType;     ///< UBI volume type if patch concerns an UBI volume, -1 else
    uint8_t  ubiVolFlags;    ///< UBI volume flags if patch concerns an UBI volume, -1 else
    uint32_t origSize;       ///< Size of the original image
    uint32_t origCrc32;      ///< CRC32 of the original image
    uint32_t destSize;       ///< Size of the destination image
    uint32_t destCrc32;      ///< CRC32 of the destination image

}
DeltaPatchMetaHeader_t;

//--------------------------------------------------------------------------------------------------
/**
 * Structure for a patch segement. A delta patch may be splitted into several patches "segments"
 * Note: Structure shared between architectures: Use uint32_t for all 32-bits fields
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint32_t offset;         ///< Offset to apply this patch
    uint32_t number;         ///< Number of this patch
    uint32_t size;           ///< Real size of the patch
}
DeltaPatchHeader_t;

//--------------------------------------------------------------------------------------------------
/**
 * Scan a partition for the UBI volume ID given. Update the LebToPeb array field with LEB for this
 * volume ID.
 *
 * @return
 *      - LE_OK            On success
 *      - LE_FAULT         On failure
 *      - LE_FORMAT_ERROR  If the flash is not in UBI format
 */
//--------------------------------------------------------------------------------------------------
le_result_t utils_ScanUbi
(
    int fd,             ///< [IN] Private flash descriptor
    size_t imageLength, ///< [IN] Length of UBI image to scan
    int32_t pebSize,    ///< [IN] Flash physical erase block size
    int32_t pageSize,   ///< [IN] Flash page size
    int *nbVolumePtr    ///< [OUT] Number of UBI volume found
);

//--------------------------------------------------------------------------------------------------
/**
 * Extract the data image belonging to an UBI volume ID.
 *
 * @return
 *      - LE_OK            On success
 *      - LE_FAULT         On failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t utils_ExtractUbiData
(
    int       fd,           ///< [IN] Minor of the MTD device to check
    uint32_t  ubiVolId,     ///< [IN] UBI Volume ID to check
    char*     fileNamePtr,  ///< [IN] File name where to store the extracted image from UBI volume
    int32_t   pebSize,      ///< [IN] Flash physical erase block size
    int32_t   pageSize,     ///< [IN] Flash page size
    uint8_t*  volTypePtr,   ///< [OUT] Returned volume type of the extracted image
    uint8_t*  volFlagsPtr,  ///< [OUT] Returned volume flags of the extracted image
    size_t*   sizePtr,      ///< [OUT] Returned size of the extracted image
    uint32_t* crc32Ptr      ///< [OUT] Returned computed CRC32 of the extracted image
);

//--------------------------------------------------------------------------------------------------
/**
 * Call system(3) with the command given. In case of error, call exit(3). Return only if system(3)
 * has succeed
 */
//--------------------------------------------------------------------------------------------------
void utils_ExecSystem
(
    char* cmdPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Check if tool exists inside the PATH list and is executable. Else raise a message explaining
 * the missing tool and the way to solve it.
 */
//--------------------------------------------------------------------------------------------------
void utils_CheckForTool
(
    char *toolPtr,
    char *toolchainPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Set verbose flag
 */
//--------------------------------------------------------------------------------------------------
void utils_beVerbose
(
    bool isVerbose
);

#endif   // PATCH_UTILS_H_INCLUDE_GUARD
