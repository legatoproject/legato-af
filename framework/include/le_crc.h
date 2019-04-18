/**
 * @page c_crc CRC32 API
 *
 * @subpage le_crc.h "API Reference"
 *
 * <HR>
 *
 * This module provides APIs for computing CRC of a binary buffer.
 *
 * CRC (Cyclic Redundancy Check) are used to verify the integrity of data, like file, messages, etc...
 *
 * @section Crc32 Computing a CRC32
 *
 *   - @c le_crc_Crc32() - Compute the CRC32 of a memory buffer
 *
 * The CRC32 is computed by the function @ref le_crc_Crc32. It takes a base buffer address, a length
 * and a CRC32. When the CRC32 is expected to be first computed, the value @ref LE_CRC_START_CRC32
 * needs to be set as crc32 value.
 *
 * @note It is possible to compute a "global" CRC32 of a huge amount of data by splitting into small
 * blocks and continue computing the CRC32 on each one.
 *
 * This code compute the whole CRC32 of amount of data splitted into an array of small blocks:
 * @code
 * #define MAX_BLOCKS 4  // Maximum number of blocks
 *
 * typedef struct
 * {
 *     size_t   blockSize;  // size of the block
 *     uint8_t *blockPtr;   // pointer of the data
 * }
 * block_t;
 *
 * uint32_t ComputeArrayCRC32
 * (
 *     block_t blockArray[]
 * )
 * {
 *     uint32_t crc = LE_CRC_START_CRC32;  // New CRC initialized
 *     int iBlock;
 *
 *     for (iBlock = 0; iBlock < MAX_BLOCKS; iBlock++)
 *     {
 *         if (blockArray[iBlock].blockPtr)
 *         {
 *             crc = le_crc_Crc32(blockArray[iBlock].blockPtr,
 *                                blockArray[iBlock].blockSize,
 *                                crc);
 *         }
 *     }
 *     return crc;
 * }
 * @endcode
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

//--------------------------------------------------------------------------------------------------
/** @file le_crc.h
 *
 * Legato @ref c_crc include file.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_CRC_INCLUDE_GUARD
#define LEGATO_CRC_INCLUDE_GUARD

#ifndef LE_COMPONENT_NAME
#define LE_COMPONENT_NAME
#endif

//--------------------------------------------------------------------------------------------------
/**
 * Initial value for CRC32 calculation
 */
//--------------------------------------------------------------------------------------------------
#define LE_CRC_START_CRC32         0xFFFFFFFFU

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to calculate a CRC-32
 *
 * @return
 *      - 32-bit CRC
 */
//--------------------------------------------------------------------------------------------------
uint32_t le_crc_Crc32
(
    uint8_t* addressPtr,///< [IN] Input buffer
    size_t   size,      ///< [IN] Number of bytes to read
    uint32_t crc        ///< [IN] Starting CRC seed
);

#endif // LEGATO_CRC_INCLUDE_GUARD
