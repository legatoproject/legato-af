//--------------------------------------------------------------------------------------------------
/**
 * Delta patch utilities implementation
 *
 * Copyright (C) Sierra Wireless Inc.
 **/
//--------------------------------------------------------------------------------------------------

#define _LARGEFILE64_SOURCE
#include "legato.h"
#include <endian.h>

#include "flash-ubi.h"
#include "patch_utils.h"

//--------------------------------------------------------------------------------------------------
/**
 * Defines the value of flash erased byte, ie, all bits to 1
 */
//--------------------------------------------------------------------------------------------------
#define ERASED_VALUE        0xFFU

//--------------------------------------------------------------------------------------------------
/**
 * Structure to get retreive informations about a volume from an UBI image
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    struct ubi_vtbl_record *vtblPtr;
    size_t imageSize;
    uint32_t lebToPeb[2048];
}
VtblMap_t;

//--------------------------------------------------------------------------------------------------
/**
 * Map array for all volumes of an UBI image
 */
//--------------------------------------------------------------------------------------------------
static VtblMap_t VtblMap[UBI_MAX_VOLUMES];

//--------------------------------------------------------------------------------------------------
/**
 * Array for all volumes records of an UBI image
 */
//--------------------------------------------------------------------------------------------------
static struct ubi_vtbl_record Vtbl[UBI_MAX_VOLUMES];

//--------------------------------------------------------------------------------------------------
/**
 * Verbose mode enabled or disabled (default)
 */
//--------------------------------------------------------------------------------------------------
static bool IsVerbose = false;

//--------------------------------------------------------------------------------------------------
/**
 * Private functions
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Read the UBI EC (Erase Count) header at the given block, check for validity and store it into
 * the buffer pointer.
 *
 * @return
 *      - LE_OK            On success
 *      - LE_FAULT         On failure
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ReadEcHeader
(
    int   fd,                      ///< [IN] File descriptor to the flash device
    off64_t physEraseOffset,       ///< [IN] Physcal erase block (PEB) to read
    struct ubi_ec_hdr *ecHeaderPtr ///< [IN] Buffer to store read data
)
{
    uint32_t crc;
    int i;
    int len;

    if( 0 > lseek64( fd, physEraseOffset, SEEK_SET ) )
    {
        fprintf(stderr, "%s: lseek64() fails: %m\n", (__func__));
        return LE_FAULT;
    }
    len = read( fd, (uint8_t*)ecHeaderPtr, UBI_EC_HDR_SIZE );
    if ( 0 > len )
    {
        fprintf(stderr, "read() fails: %m\n");
        return LE_FAULT;
    }
    if( !len )
    {
        return LE_OK;
    }
    if( UBI_EC_HDR_SIZE != len )
    {
        fprintf(stderr, "Read only %d bytes, expected %u:\n", len, (uint32_t)UBI_EC_HDR_SIZE);
        return LE_FAULT;
    }

    for( i = 0; i < UBI_EC_HDR_SIZE; i++ )
    {
        // Check for all bytes in the EC header
        if( ERASED_VALUE != ((uint8_t*)ecHeaderPtr)[i] )
        {
            break;
        }
    }
    if (UBI_EC_HDR_SIZE == i)
    {
        fprintf(stderr, "Block %x is erased\n", (uint32_t)physEraseOffset );
        return LE_FORMAT_ERROR;
    }

    if (be32toh(ecHeaderPtr->magic) != (uint32_t)UBI_EC_HDR_MAGIC)
    {
        fprintf(stderr, "Bad magic at %x: Expected %x, received %x\n",
                (uint32_t)physEraseOffset, (uint32_t)UBI_EC_HDR_MAGIC,
                be32toh(ecHeaderPtr->magic));
        return LE_FAULT;
    }

    if (UBI_VERSION != ecHeaderPtr->version)
    {
        fprintf(stderr, "Bad version at %x: Expected %d, received %d\n",
                (uint32_t)physEraseOffset, UBI_VERSION, ecHeaderPtr->version);
        return LE_FAULT;
    }

    crc = le_crc_Crc32((uint8_t*)ecHeaderPtr, UBI_EC_HDR_SIZE_CRC, LE_CRC_START_CRC32);
    if (be32toh(ecHeaderPtr->hdr_crc) != crc)
    {
        fprintf(stderr, "Bad CRC at %x: Calculated %x, received %x\n",
                (uint32_t)physEraseOffset, crc, be32toh(ecHeaderPtr->hdr_crc));
        return LE_FAULT;
    }

    if( IsVerbose )
    {
        fprintf(stderr,
                "PEB %x : MAGIC %c%c%c%c, VID %x DATA %x CRC %x\n",
                (uint32_t)physEraseOffset,
                ((char *)&(ecHeaderPtr->magic))[0],
                ((char *)&(ecHeaderPtr->magic))[1],
                ((char *)&(ecHeaderPtr->magic))[2],
                ((char *)&(ecHeaderPtr->magic))[3],
                be32toh(ecHeaderPtr->vid_hdr_offset),
                be32toh(ecHeaderPtr->data_offset),
                be32toh(ecHeaderPtr->hdr_crc) );
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the UBI Volume ID header at the given block + offset, check for validity and store it into
 * the buffer pointer.
 *
 * @return
 *      - LE_OK            On success
 *      - LE_FAULT         On failure
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ReadVidHeader
(
    int fd,          ///< [IN] File descriptor to the flash device
    off64_t physEraseOffset,          ///< [IN] Physcal erase block (PEB) to read
    struct ubi_vid_hdr *vidHeaderPtr,  ///< [IN] Pointer to the VID header
    off64_t vidOffset                ///< [IN] Offset of VID header in physical block
)
{
    uint32_t crc;
    int i;
    int len;

    if( 0 > lseek64( fd, physEraseOffset + vidOffset, SEEK_SET ) )
    {
        fprintf(stderr, "%s: lseek64() fails: %m\n", (__func__));
        return LE_FAULT;
    }
    len = read( fd, (uint8_t*)vidHeaderPtr, UBI_VID_HDR_SIZE );
    if ( 0 > len )
    {
        fprintf(stderr, "read() fails: %m\n");
        return LE_FAULT;
    }
    if( UBI_VID_HDR_SIZE != len )
    {
        fprintf(stderr, "Read only %d bytes, expected %u:\n", len, (uint32_t)UBI_VID_HDR_SIZE);
        return LE_FAULT;
    }

    for( i = 0; i < UBI_VID_HDR_SIZE; i++ )
    {
        // Check for all bytes in the Volume ID header
        if( ERASED_VALUE != ((uint8_t*)vidHeaderPtr)[i] )
        {
            break;
        }
    }
    if (UBI_VID_HDR_SIZE == i)
    {
        fprintf(stderr, "Block %x is erased\n", (uint32_t)physEraseOffset );
        return LE_FORMAT_ERROR;
    }

    if (be32toh(vidHeaderPtr->magic) != (uint32_t)UBI_VID_HDR_MAGIC)
    {
        fprintf(stderr, "Bad magic at %x: Expected %x, received %x\n",
                (uint32_t)physEraseOffset, (uint32_t)UBI_VID_HDR_MAGIC,
                be32toh(vidHeaderPtr->magic));
        return LE_FAULT;
    }

    if (UBI_VERSION != vidHeaderPtr->version)
    {
        fprintf(stderr, "Bad version at %x: Expected %d, received %d\n",
                (uint32_t)physEraseOffset, UBI_VERSION, vidHeaderPtr->version);
        return LE_FAULT;
    }

    crc = LE_CRC_START_CRC32;
    crc = le_crc_Crc32((uint8_t*)vidHeaderPtr, UBI_VID_HDR_SIZE_CRC, crc);
    if (be32toh(vidHeaderPtr->hdr_crc) != crc)
    {
        fprintf(stderr, "Bad CRC at %x: Calculated %x, received %x\n",
                (uint32_t)physEraseOffset, crc, be32toh(vidHeaderPtr->hdr_crc));
        return LE_FAULT;
    }

    if( be32toh(vidHeaderPtr->vol_id) < UBI_MAX_VOLUMES )
    {
        if( IsVerbose )
        {
            fprintf(stderr,
                    "PEB : %x, MAGIC %c%c%c%c, VER %hhd, VT %hhd CP %hhd CT %hhd VID "
                    "%x LNUM %x DSZ %x EBS %x DPD %x DCRC %x CRC %x\n",
                    (uint32_t)physEraseOffset,
                    ((char *)&(vidHeaderPtr->magic))[0],
                    ((char *)&(vidHeaderPtr->magic))[1],
                    ((char *)&(vidHeaderPtr->magic))[2],
                    ((char *)&(vidHeaderPtr->magic))[3],
                    (vidHeaderPtr->version),
                    (vidHeaderPtr->vol_type),
                    (vidHeaderPtr->copy_flag),
                    (vidHeaderPtr->compat),
                    be32toh(vidHeaderPtr->vol_id),
                    be32toh(vidHeaderPtr->lnum),
                    be32toh(vidHeaderPtr->data_size),
                    be32toh(vidHeaderPtr->used_ebs),
                    be32toh(vidHeaderPtr->data_pad),
                    be32toh(vidHeaderPtr->data_crc),
                    be32toh(vidHeaderPtr->hdr_crc) );
        }
    }
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the UBI Volume Table at the given block + offset, check for validity and store it into the
 * buffer pointer.
 *
 * @return
 *      - LE_OK            On success
 *      - LE_FAULT         On failure
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ReadVtbl
(
    int fd,          ///< [IN] File descriptor to the flash device
    off64_t physEraseOffset,          ///< [IN] Physcal erase block (PEB) to read
    struct ubi_vtbl_record *vtblPtr,  ///< [IN] Pointer to the VTBL
    off64_t vtblOffset               ///< [IN] Offset of VTBL in physical block
)
{
    uint32_t crc;
    int i;
    int len;

    if( 0 > lseek64( fd, physEraseOffset + vtblOffset, SEEK_SET ) )
    {
        fprintf(stderr, "%s: lseek64() fails: %m\n", (__func__));
        return LE_FAULT;
    }
    len = read( fd, (uint8_t*)vtblPtr, UBI_MAX_VOLUMES * UBI_VTBL_RECORD_HDR_SIZE );
    if( 0 > len )
    {
        fprintf(stderr, "read() fails: %m\n");
        return LE_FAULT;
    }
    if( (UBI_MAX_VOLUMES * UBI_VTBL_RECORD_HDR_SIZE) != len )
    {
        fprintf(stderr, "Read only %d bytes, expected %u:\n",
                len, (uint32_t)(UBI_MAX_VOLUMES * UBI_VTBL_RECORD_HDR_SIZE));
        return LE_FAULT;
    }

    for( i = 0; i < UBI_MAX_VOLUMES; i++ )
    {
        if( (be32toh(vtblPtr[i].reserved_pebs) == (uint32_t)-1))
        {
            continue;
        }
        crc = le_crc_Crc32((uint8_t*)&vtblPtr[i], UBI_VTBL_RECORD_SIZE_CRC, LE_CRC_START_CRC32);
        if( be32toh(vtblPtr[i].crc) != crc )
        {
            fprintf(stderr, "VID %d : Bad CRC %x expected %x\n", i, crc, be32toh(vtblPtr[i].crc));
            return LE_FAULT;
        }
        if( vtblPtr[i].vol_type )
        {
            if( IsVerbose )
            {
                fprintf(stderr,
                        "VID %d RPEBS %u AL %X RPD %X VT %X UPDM %X NL %X \"%s\" FL %X CRC %X\n",
                        i,
                        be32toh(vtblPtr[i].reserved_pebs),
                        be32toh(vtblPtr[i].alignment),
                        be32toh(vtblPtr[i].data_pad),
                        vtblPtr[i].vol_type,
                        vtblPtr[i].upd_marker,
                        be16toh(vtblPtr[i].name_len),
                        vtblPtr[i].name,
                        vtblPtr[i].flags,
                        be32toh(vtblPtr[i].crc));
            }
        }
        VtblMap[i].vtblPtr = &vtblPtr[i];
    }
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Public functions
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Set verbose flag
 */
//--------------------------------------------------------------------------------------------------
void utils_beVerbose
(
    bool isVerbose
)
{
    IsVerbose = isVerbose;
}

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
)
{
    int32_t peb;
    struct ubi_ec_hdr ecHeader;
    struct ubi_vid_hdr vidHeader;
    off64_t pebOffset;
    le_result_t res;
    int i, j;

    memset( Vtbl, 0, sizeof(Vtbl));
    memset( VtblMap, -1, sizeof(VtblMap));
    *nbVolumePtr = 0;
    for( i = 0; i < UBI_MAX_VOLUMES; i++ )
    {
        VtblMap[i].imageSize = 0;
    }

    for( peb = 0; peb < (imageLength / pebSize); peb++ )
    {
        pebOffset = peb * pebSize;
        res = ReadEcHeader( fd, pebOffset, &ecHeader );
        if (LE_FORMAT_ERROR == res )
        {
            continue;
        }
        else if (LE_OK != res )
        {
            return LE_FAULT;
        }
        else
        {
            // Do nothing
        }
        res = ReadVidHeader( fd, pebOffset, &vidHeader, be32toh(ecHeader.vid_hdr_offset) );
        if (LE_FORMAT_ERROR == res )
        {
            continue;
        }
        if (LE_OK != res )
        {
            fprintf(stderr, "Error when reading VID Header at %d\n", peb);
            return LE_FAULT;
        }
        if (be32toh(vidHeader.vol_id) == UBI_LAYOUT_VOLUME_ID)
        {
            res = ReadVtbl( fd, pebOffset, Vtbl, be32toh(ecHeader.data_offset) );
            if (LE_OK != res)
            {
                fprintf(stderr, "Error when reading Vtbl at %d\n", peb);
                return LE_FAULT;
            }
        }
        else if (be32toh(vidHeader.vol_id) < UBI_MAX_VOLUMES)
        {
            VtblMap[be32toh(vidHeader.vol_id)].lebToPeb[be32toh(vidHeader.lnum)] = peb;
            if( UBI_VID_STATIC == vidHeader.vol_type )
            {
                VtblMap[be32toh(vidHeader.vol_id)].imageSize += be32toh(vidHeader.data_size);
            }
            else
            {
                VtblMap[be32toh(vidHeader.vol_id)].imageSize +=
                    (pebSize - (2 * pageSize));
            }
        }
        else
        {
            fprintf(stderr, "Unknown vol ID %x\n", be32toh(vidHeader.vol_id));
        }
    }

    for( i = 0; i < UBI_MAX_VOLUMES; i++ )
    {
        if( Vtbl[i].vol_type )
        {
            (*nbVolumePtr)++;
            if( IsVerbose )
            {
                fprintf(stderr, "VOL %i \"%s\" VT %u RPEBS %u\n", i,
                         Vtbl[i].name,
                         Vtbl[i].vol_type,
                         be32toh(Vtbl[i].reserved_pebs));
                for( j = 0;
                     (j < be32toh(Vtbl[i].reserved_pebs));
                     j++ )
                {
                    fprintf(stderr, "%u ", VtblMap[i].lebToPeb[j] );
                }
                fprintf(stderr, "\n");
                fprintf(stderr,
                        "Volume image size = %zx (%zu)\n",
                         VtblMap[i].imageSize, VtblMap[i].imageSize);
            }
        }
    }
    if( IsVerbose )
    {
        fprintf(stderr, "Number of volume(s): %u\n", *nbVolumePtr);
    }
    return LE_OK;
}

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
)
{
    // We use malloc(3). No alternative to this within the tool
    uint8_t* checkBlockPtr = (uint8_t*)malloc(pebSize);

    int fdw, rc, readLen;
    size_t size, imageSize = VtblMap[ubiVolId].imageSize;
    uint32_t blk, crc32 = LE_CRC_START_CRC32;
    le_result_t res = LE_FAULT;

    if( NULL == checkBlockPtr )
    {
        fprintf(stderr, "Malloc fails: %m\n");
        exit(1);
    }

    fdw = open( fileNamePtr, O_WRONLY | O_TRUNC | O_CREAT, S_IWUSR | S_IRUSR );
    if ( 0 > fdw )
    {
        fprintf(stderr, "Open of '%s' fails: %d\n", fileNamePtr, res );
        goto clean_exit;
    }

    for( blk = 0; (uint32_t)-1 != VtblMap[ubiVolId].lebToPeb[blk]; blk++ )
    {
        size = ((imageSize > (pebSize - (2 * pageSize)))
                  ? (pebSize - (2 * pageSize))
                  : imageSize);
        if ( 0 > lseek64( fd,
                        (VtblMap[ubiVolId].lebToPeb[blk] * pebSize) + (2 * pageSize),
                        SEEK_SET ) )
        {
            fprintf(stderr, "%s: lseek64() fails: %m\n", (__func__));
            goto clean_exit;
        }
        readLen = 0;
        do
        {
            rc = read( fd, checkBlockPtr, size );
            if ( (0 > rc) && ((EAGAIN == errno) || (EINTR == errno)) )
            {
                // Do nothing: we redo the loop
            }
            else if ( 0 > rc )
            {
                fprintf(stderr, "read() fails: %m\n");
                goto clean_exit;
            }
            else if( 0 == rc )
            {
                fprintf(stderr, "read() end-of-file. File is corrupted\n");
                goto clean_exit;
            }
            else
            {
                readLen += rc;
            }
        }
        while( readLen < size );

        imageSize -= size;
        crc32 = le_crc_Crc32( checkBlockPtr, (uint32_t)size, crc32);
        if ( 0 > write( fdw, checkBlockPtr, size) )
        {
            fprintf(stderr, "write() fails: %m\n");
            goto clean_exit;
        }
    }
    *sizePtr = VtblMap[ubiVolId].imageSize;
    *crc32Ptr = crc32;
    if( IsVerbose )
    {
        fprintf(stderr, "File '%s', Size %zx CRC %x\n", fileNamePtr, imageSize, crc32);
    }

    *volTypePtr = Vtbl[ubiVolId].vol_type;
    *volFlagsPtr = Vtbl[ubiVolId].flags;
    res = LE_OK;

clean_exit:
    if( fdw > -1)
    {
        close( fdw );
    }
    // We use free(3). No alternative to this within the tool
    free(checkBlockPtr);
    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * Call system(3) with the command given. In case of error, call exit(3). Return only if system(3)
 * has succeed
 */
//--------------------------------------------------------------------------------------------------
void utils_ExecSystem
(
    char* cmdPtr
)
{
    int rc;

    if( IsVerbose )
    {
        printf("system(%s)\n", cmdPtr);
    }
    rc = system( cmdPtr );
    if( (!WIFEXITED(rc)) || WEXITSTATUS(rc) )
    {
        fprintf(stderr,"system(%s) fails: rc=%d\n", cmdPtr, rc);
        exit(2);
    }
}

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
)
{
    FILE* fdPtr;
    char  toolPath[PATH_MAX];
    char* toolPathPtr;

    snprintf( toolPath, sizeof(toolPath), "which %s", toolPtr );
    fdPtr = popen( toolPath, "r" );
    if( NULL == fdPtr )
    {
        fprintf(stderr, "popen to %s fails: %m\n", toolPath);
        exit(1);
    }
    toolPathPtr = fgets( toolPath, sizeof(toolPath), fdPtr );
    fclose( fdPtr );
    if( !toolPathPtr )
    {
        fprintf(stderr,
                "The tool '%s' is required and missing in the PATH environment variable\n"
                "or it is not installed on this host.\n", toolPtr);
        if( !toolchainPtr )
        {
            fprintf(stderr,
                    "Try a 'sudo apt-get install %s' or similar to install this package\n",
                    toolPtr);
        }
        else
        {
            fprintf(stderr,
                    "Try to set the '%s' environment variable for this target\n", toolchainPtr);
        }
        exit(1);
    }
}
