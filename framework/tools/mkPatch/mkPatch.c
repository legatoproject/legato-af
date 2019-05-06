//--------------------------------------------------------------------------------------------------
/**
 * @file mkPatch.c  Build delta patches between several images
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#define _LARGEFILE64_SOURCE

#include <ctype.h>
#include <endian.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>


// Can't include legato.h because this doesn't run on target
// Include just these so we have access to Legato CRC function.
#include <le_basics.h>
#include <le_crc.h>

#include "flash-ubi.h"

//--------------------------------------------------------------------------------------------------
/**
 * Defines some executables requested by the tool
 */
//--------------------------------------------------------------------------------------------------
#define BSDIFF "bsdiff"
#define HDRCNV "hdrcnv"

//--------------------------------------------------------------------------------------------------
/**
 * Defines the value of flash erased byte, ie, all bits to 1
 */
//--------------------------------------------------------------------------------------------------
#define ERASED_VALUE        0xFFU

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
    uint32_t ubiVolId;       ///< UBI volume ID if patch concerns an UBI volume, -1 else
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
 * Structure to get correspondance between a partition name and image type for the CWE headers
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char *partName;          ///< Partition name where apply the patch
    char *imageType;         ///< Image type for the CWE
    char *spkgImageType;     ///< Image type for the SPKG CWE
    bool isUbiImage;              ///< Image is expected to be an UBI
}
PartToSpkg_t;

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
 * MDM9x40 and MDM9x28 partition scheme. This is platform dependant
 */
//--------------------------------------------------------------------------------------------------
static PartToSpkg_t mdm9x40_PartToSpkg[] =
{
    { "lefwkro",   "USER", "APPL", true,  },
    { "system",    "SYST", "APPL", true,  },
    { "boot",      "APPS", "APPL", false, },
    { "aboot",     "APBL", "APPL", false, },
    { "modem",     "DSP2", "MODM", true,  },
    { "sbl",       "SBL1", "BOOT", false, },
    { "aboot",     "APBL", "BOOT", false, },
    { "tz",        "TZON", "BOOT", false, },
    { "rpm",       "QRPM", "BOOT", false, },
    { "customer0", "CUS0", "APPL", false, },
    { "customer1", "CUS1", "APPL", false, },
    { NULL,        NULL,   NULL,   false, },
};

//--------------------------------------------------------------------------------------------------
/**
 * Our name
 */
//--------------------------------------------------------------------------------------------------
static char *ProgName;

//--------------------------------------------------------------------------------------------------
/**
 * Verbose mode enabled or disabled (default)
 */
//--------------------------------------------------------------------------------------------------
static bool IsVerbose = false;

//--------------------------------------------------------------------------------------------------
/**
 * Original and destination name pointer
 */
//--------------------------------------------------------------------------------------------------
static char *OrigPtr, *DestPtr;

//--------------------------------------------------------------------------------------------------
/**
 * Original name path
 */
//--------------------------------------------------------------------------------------------------
static char OrigName[PATH_MAX];

//--------------------------------------------------------------------------------------------------
/**
 * Destination name path
 */
//--------------------------------------------------------------------------------------------------
static char DestName[PATH_MAX];

//--------------------------------------------------------------------------------------------------
/**
 * Meta header for the delta patch
 */
//--------------------------------------------------------------------------------------------------
static DeltaPatchMetaHeader_t PatchMetaHeader;

//--------------------------------------------------------------------------------------------------
/**
 * Header for a segment patch
 */
//--------------------------------------------------------------------------------------------------
static DeltaPatchHeader_t PatchHeader;

//--------------------------------------------------------------------------------------------------
/**
 * Buffer for the segment to read
 */
//--------------------------------------------------------------------------------------------------
static uint8_t Chunk[SEGMENT_SIZE];

//--------------------------------------------------------------------------------------------------
/**
 * Buffer for the segment patch
 */
//--------------------------------------------------------------------------------------------------
static uint8_t PatchedChunk[2 * SEGMENT_SIZE];

//--------------------------------------------------------------------------------------------------
/**
 * Flash device page size
 */
//--------------------------------------------------------------------------------------------------
static int32_t FlashPageSize = -1;

//--------------------------------------------------------------------------------------------------
/**
 * Flash device Physical Erase Block size
 */
//--------------------------------------------------------------------------------------------------
static int32_t FlashPEBSize = -1;

//--------------------------------------------------------------------------------------------------
/**
 * Buffer for the commands launched by system(3)
 */
//--------------------------------------------------------------------------------------------------
static char CmdBuf[4096];

//--------------------------------------------------------------------------------------------------
/**
 * Original working directory. The tool creates and goes to a temporary work directory due to
 * some tools issues with global path.
 */
//--------------------------------------------------------------------------------------------------
static char CurrentWorkDir[PATH_MAX];

//--------------------------------------------------------------------------------------------------
/**
 * Functions to handle big-endian <> little-endian conversion
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
 * Scan a partition for the UBI volume ID given. Update the LebToPeb array field with LEB for this
 * volume ID.
 *
 * @return
 *      - LE_OK            On success
 *      - LE_FAULT         On failure
 *      - LE_FORMAT_ERROR  If the flash is not in UBI format
 */
//--------------------------------------------------------------------------------------------------
le_result_t ScanUbi
(
    int fd,             ///< [IN] Private flash descriptor
    size_t imageLength, ///< [IN] Length of UBI image to scan
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

    for( peb = 0; peb < (imageLength / FlashPEBSize); peb++ )
    {
        pebOffset = peb * FlashPEBSize;
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
                    (FlashPEBSize - (2 * FlashPageSize));
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
static le_result_t ExtractUbiData
(
    int       fd,           ///< [IN] Minor of the MTD device to check
    uint32_t  ubiVolId,     ///< [IN] UBI Volume ID to check
    char*     fileNamePtr,  ///< [IN] File name where to store the extracted image from UBI volume
    size_t*   sizePtr,      ///< [OUT] Returned size of the extracted image
    uint32_t* crc32Ptr      ///< [OUT] Returned computed CRC32 of the extracted image
)
{
    // We use malloc(3). No alternative to this within the tool
    uint8_t* checkBlockPtr = (uint8_t*)malloc(FlashPEBSize);

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
        size = ((imageSize > (FlashPEBSize - (2 * FlashPageSize)))
                  ? (FlashPEBSize - (2 * FlashPageSize))
                  : imageSize);
        if ( 0 > lseek64( fd,
                        (VtblMap[ubiVolId].lebToPeb[blk] * FlashPEBSize) + (2 * FlashPageSize),
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
 * Call at exit(3) to perform all clean-up actions
 */
//--------------------------------------------------------------------------------------------------
static void ExitHandler
(
    void
)
{
    chdir(CurrentWorkDir);
    snprintf( CmdBuf, sizeof(CmdBuf),
              "rm -rf /tmp/patchdir.%u",
              getpid() );
    (void)system( CmdBuf );
}

//--------------------------------------------------------------------------------------------------
/**
 * Call system(3) with the command given. In case of error, call exit(3). Return only if system(3)
 * has succeed
 */
//--------------------------------------------------------------------------------------------------
static void ExecSystem
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
 * Print usage and exit...
 */
//--------------------------------------------------------------------------------------------------
static void Usage
(
    void
)
{
    fprintf(stderr,
            "usage: %s -T TARGET [-o patchname] [-S 4K|2K] [-E 256K|128K] [-N] [-v]\n"
            "        {-p PART {[-U VOLID] file-orig file-dest}}\n",
            ProgName );
    fprintf(stderr, "\n");
    fprintf(stderr, "   -T, --target <TARGET>\n"
                    "        Specify the TARGET (mandatory - specified only one time).\n");
    fprintf(stderr, "   -o, <patchname>\n"
                    "        Specify the output name of the patch."
                           " Else use patch-<TARGET>.cwe as default.\n");
    fprintf(stderr, "   -S, --pagesize <4K|2K>\n"
                    "        Specify another page size (optional - specified only one time).\n");
    fprintf(stderr, "   -E, --pebsize <256K|128K>\n"
                    "        Specify another PEB size (optional - specified only one time).\n");
    fprintf(stderr, "   -N, --no-spkg-header\n"
                    "        Do not generate the CWE SPKG header.\n");
    fprintf(stderr, "   -v, --verbose\n"
                    "        Be verbose.\n");
    fprintf(stderr, "   -p, --partition <PART>\n"
                    "        Specify the partition where apply the patch.\n");
    fprintf(stderr, "   -U, --ubi <VOLID>\n"
                    "        Specify the UBI volume ID where apply the patch.\n");
    fprintf(stderr, "\n");
    exit(1);
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if tool exists inside the PATH list and is executable. Else raise a message explaining
 * the missing tool and the way to solve it.
 */
//--------------------------------------------------------------------------------------------------
static void CheckForTool
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

//--------------------------------------------------------------------------------------------------
/**
 * Main :)
 */
//--------------------------------------------------------------------------------------------------
int main
(
    int    argc,
    char** argv
)
{
    char tmpName[PATH_MAX];
    int fdr, fdw, fdp;
    int len, patchNum = 0;
    int iargc = argc;
    char** argvPtr = &argv[1];
    struct stat st;
    unsigned int crc32Orig, crc32Dest;
    unsigned int ubiVolId = (uint32_t)-1;
    size_t chunkLen;
    char* partPtr = NULL;
    char* pckgPtr = NULL;
    char* productPtr = NULL;
    char* targetPtr = NULL;
    char* outPtr = NULL;
    PartToSpkg_t* partToSpkgPtr = NULL;
    bool noSpkgHeader = false;
    bool isUbiImage = false;
    int nbVolumeOrig, nbVolumeDest, nbVolume;
    int ubiIdx;
    pid_t pid = getpid();
    char env[4096];
    char* toolchainPtr = NULL;
    char* toolchainEnvPtr = NULL;
    char* envPtr = NULL;
    int ret;

    ProgName = argv[0];

    CheckForTool( BSDIFF, NULL );

    getcwd(CurrentWorkDir, sizeof(CurrentWorkDir));
    atexit( ExitHandler );
    snprintf( CmdBuf, sizeof(CmdBuf), "/tmp/patchdir.%u", pid );
    if( -1 == mkdir(CmdBuf, (mode_t)(S_IRWXU | S_IRWXG | S_IRWXO)) )
    {
        fprintf(stderr, "Failed to create directory '%s': %m\n", CmdBuf);
        exit( 1 );
    }
    if( -1 == chdir(CmdBuf) )
    {
        fprintf(stderr, "Failed to change directory to '%s': %m\n", CmdBuf);
        exit( 1 );
    }

    while( argc > 1 )
    {
        if( (iargc >= 5) &&
            ((0 == strcmp(*argvPtr, "--target")) || (0 == strcmp(*argvPtr, "-T"))) )
        {
            ++argvPtr;
            if( 0 == strcasecmp( *argvPtr, "ar759x" ))
            {
                productPtr = "9X40";
                targetPtr = "ar759x";
                partToSpkgPtr = mdm9x40_PartToSpkg;
                FlashPageSize = FLASH_PAGESIZE_4K;
                FlashPEBSize = FLASH_PEBSIZE_256K;
                toolchainPtr = "AR759X_TOOLCHAIN_DIR";
            }
            else if( 0 == strcasecmp( *argvPtr, "ar758x" ))
            {
                productPtr = "9X28";
                targetPtr = "ar758x";
                partToSpkgPtr = mdm9x40_PartToSpkg;
                FlashPageSize = FLASH_PAGESIZE_4K;
                FlashPEBSize = FLASH_PEBSIZE_256K;
                toolchainPtr = "AR758X_TOOLCHAIN_DIR";
            }
            else
            {
                fprintf(stderr, "Unsupported target %s\n", *argvPtr );
                exit(1);
            }
            envPtr = getenv( "PATH" );
            if (!envPtr)
            {
               fprintf(stderr,
                        "Variable 'PATH' is not set for target %s\n",
                        targetPtr);
               exit(1);
            }
            toolchainEnvPtr = getenv( toolchainPtr );
            if( toolchainEnvPtr )
            {
                snprintf( env, sizeof(env), "PATH=%s:%s/..", envPtr, toolchainEnvPtr );
                putenv( env );
            }
            else
            {
                fprintf(stderr,
                        "Variable '%s' is not set for target %s\n",
                        toolchainPtr, targetPtr);
                exit(1);
            }
            CheckForTool( HDRCNV, toolchainPtr );
            ++argvPtr;
            iargc -= 2;
        }

        else if( (iargc >= 5) && (0 == strcmp(*argvPtr, "-o")) )
        {
            if( outPtr )
            {
                fprintf(stderr, "Output file %s is already specified\n", outPtr);
                exit(1);
            }
            ++argvPtr;
            outPtr = *argvPtr;
            ++argvPtr;
            iargc -= 2;
        }

        else if( (iargc >= 5) &&
                 ((0 == strcmp(*argvPtr, "--pagesize")) || (0 == strcmp(*argvPtr, "-S"))) )
        {
            ++argvPtr;
            if( 0 == strcmp( *argvPtr, "4K" ) )
            {
                FlashPageSize = FLASH_PAGESIZE_4K;
            }
            else if( 0 == strcmp( *argvPtr, "2K" ) )
            {
                FlashPageSize = FLASH_PAGESIZE_2K;
            }
            else
            {
                fprintf(stderr, "Unsupported page size %s\n", *argvPtr );
                exit(1);
            }
            ++argvPtr;
            iargc -= 2;
        }

        else if( (iargc >= 5) &&
                 ((0 == strcmp(*argvPtr, "--pebsize")) || (0 == strcmp(*argvPtr, "-E"))) )
        {
            ++argvPtr;
            if( 0 == strcmp( *argvPtr, "256K" ) )
            {
                FlashPEBSize = FLASH_PEBSIZE_256K;
            }
            else if( 0 == strcmp( *argvPtr, "128K" ) )
            {
                FlashPEBSize = FLASH_PEBSIZE_128K;
            }
            else
            {
                 fprintf(stderr, "Unsupported PEB size %s\n", *argvPtr );
                 exit(1);
            }
            ++argvPtr;
            iargc -= 2;
        }

        else if( (iargc >= 4) &&
                 ((0 == strcmp(*argvPtr, "--no-spkg-header")) || (0 == strcmp(*argvPtr, "-N"))) )
        {
            noSpkgHeader = true;
            ++argvPtr;
            iargc--;
        }

        else if( (iargc >= 4) &&
                 ((0 == strcmp(*argvPtr, "--verbose")) || (0 == strcmp(*argvPtr, "-v"))) )
        {
            IsVerbose = true;
            ++argvPtr;
            iargc--;
        }

        else
        {
            break;
        }
    }

    if( !productPtr )
    {
        fprintf(stderr, "Missing TARGET\n");
        Usage();
    }

    while( iargc > 1 )
    {
        int notUbiOpt;

        nbVolumeOrig = 1;
        nbVolumeDest = 1;

        if( (iargc >= 5) &&
            ((0 == strcmp(*argvPtr, "--partition")) || (0 == strcmp(*argvPtr, "-p"))) )
        {
            int ip;

            ++argvPtr;

            if (!partToSpkgPtr)
            {
                fprintf(stderr, "No Partition to spkg mapping available\n");
                exit(1);
            }

            for( ip = 0; partToSpkgPtr[ip].partName; ip++ )
            {
                if( 0 == strcmp( *argvPtr, partToSpkgPtr[ip].partName ) )
                {
                    partPtr = partToSpkgPtr[ip].imageType;
                    pckgPtr = partToSpkgPtr[ip].spkgImageType;
                    isUbiImage = partToSpkgPtr[ip].isUbiImage;
                    break;
                }
            }
            if( !partToSpkgPtr[ip].partName )
            {
                fprintf(stderr, "Unknown partition %s for target %s\n", *argvPtr, targetPtr );
                exit(1);
            }
            ++argvPtr;
            iargc -= 2;
            continue;
        }

        notUbiOpt = (strcmp(*argvPtr, "--ubi")) && (strcmp(*argvPtr, "-U"));

        if( !(((iargc >= 5) && (!notUbiOpt)) || iargc >= 3) )
        {
            Usage();
        }

        if( !notUbiOpt )
        {
            char *endPtr;

            errno = 0;
            ubiVolId = strtoul( *(++argvPtr), &endPtr, 10 );
            if( (errno) || (*endPtr) || (UBI_MAX_VOLUMES <= ubiVolId) )
            {
                fprintf(stderr, "Incorrect UBI volume ID '%s'\n", *argvPtr );
                exit( 1 );
            }
            argvPtr++;
            OrigPtr = *(argvPtr++);
            DestPtr = *(argvPtr++);
            iargc -= 4;
            chunkLen = FlashPEBSize - (2 * FlashPageSize);
        }
        else
        {
            OrigPtr = *(argvPtr++);
            DestPtr = *(argvPtr++);
            iargc -= 2;
            chunkLen = sizeof(Chunk);
            ubiVolId = ((uint32_t)-1);
        }

        if( (!partPtr) )
        {
            fprintf(stderr, "Missing PART\n");
            Usage();
        }

        if( notUbiOpt && isUbiImage )
        {
            int i;
            size_t size;

            chunkLen = FlashPEBSize - (2 * FlashPageSize);
            if( OrigPtr[0] != '/' )
            {
                ret = snprintf(OrigName, sizeof(OrigName), "%s/%s", CurrentWorkDir, OrigPtr);
                if (ret >= sizeof(OrigName))
                {
                    fprintf(stderr, "string truncated in file %s at line # %d", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                ret = snprintf(OrigName, sizeof(OrigName), "%s", OrigPtr);
                if (ret >= sizeof(OrigName))
                {
                    fprintf(stderr, "string truncated in file %s at line # %d", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            fdr = open( OrigName, O_RDONLY );
            if( 0 > fdr )
            {
                fprintf(stderr, "Unable to open origin file %s: %m\n", OrigName);
                exit(1);
            }
            fstat( fdr, &st );
            ScanUbi(fdr, st.st_size, &nbVolumeOrig);
            for( i = 0; i < nbVolumeOrig; i++ )
            {
                ret = snprintf(OrigName, sizeof(OrigName), "%s.orig.%u.%u", partPtr, i, pid );
                if (ret >= sizeof(OrigName))
                {
                    fprintf(stderr, "string truncated in file %s at line # %d", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                ExtractUbiData( fdr, i, OrigName, &size, &crc32Orig );
                PatchMetaHeader.origSize = htobe32(size);
            }
            close(fdr);
            if( DestPtr[0] != '/' )
            {
                ret = snprintf(DestName, sizeof(DestName), "%s/%s", CurrentWorkDir, DestPtr);
                if (ret >= sizeof(DestName))
                {
                    fprintf(stderr, "string truncated in file %s at line # %d", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                ret = snprintf(DestName, sizeof(DestName), "%s", DestPtr);
                if (ret >= sizeof(DestName))
                {
                    fprintf(stderr, "string truncated in file %s at line # %d", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            fdr = open( DestName, O_RDONLY );
            if( 0 > fdr )
            {
                fprintf(stderr, "(1)Unable to open destination file %s: %m\n", DestName);
                exit(1);
            }
            fstat( fdr, &st );

            ScanUbi(fdr, st.st_size, &nbVolumeDest);
            for( i = 0; i < nbVolumeDest; i++ )
            {
                ret = snprintf(DestName, sizeof(DestName), "%s.dest.%u.%u", partPtr, i, pid );
                if (ret >= sizeof(DestName))
                {
                    fprintf(stderr, "string truncated in file %s at line # %d", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                ExtractUbiData( fdr, i, DestName, &size, &crc32Orig );
                PatchMetaHeader.destSize = htobe32(size);
            }
            close(fdr);
            if( nbVolumeOrig != nbVolumeDest )
            {
                char yn;

                fprintf(stderr,
                        "Number of volumes differs between original (%u) and destination (%u)\n",
                        nbVolumeOrig, nbVolumeDest);
                fprintf(stderr, "Build patch anyway [y/N] ? ");
                yn = getchar();
                if( toupper(yn) != 'Y' )
                {
                    exit(0);
                }
            }
        }
        ubiIdx = 0;
        nbVolume = (nbVolumeOrig > nbVolumeDest) ? nbVolumeDest : nbVolumeOrig;
        do
        {
            patchNum = 0;

            memset( &PatchMetaHeader, 0, sizeof(PatchMetaHeader) );
            memset( &PatchHeader, 0, sizeof(PatchHeader) );

            if( notUbiOpt && isUbiImage )
            {
                ret = snprintf(OrigName, sizeof(OrigName), "%s.orig.%u.%u", partPtr, ubiIdx, pid );
                ubiVolId = ubiIdx;
            }
            else if( OrigPtr[0] != '/' )
            {
                ret = snprintf(OrigName, sizeof(OrigName), "%s/%s", CurrentWorkDir, OrigPtr);
            }
            else
            {
                ret = snprintf(OrigName, sizeof(OrigName), "%s", OrigPtr);
            }

            if (ret >= sizeof(OrigName))
            {
                fprintf(stderr, "string truncated in file %s at line # %d", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }

            fdr = open( OrigName, O_RDONLY );
            if( 0 > fdr )
            {
                fprintf(stderr, "Unable to open origin file %s: %m\n", OrigName);
                exit(1);
            }
            fstat( fdr, &st );
            PatchMetaHeader.origSize = htobe32(st.st_size);

            crc32Orig = LE_CRC_START_CRC32;

            while( 0 < (len = read( fdr, Chunk, chunkLen )) )
            {
                crc32Orig = le_crc_Crc32( Chunk, len, crc32Orig );
            }
            close( fdr );
            PatchMetaHeader.origCrc32 = htobe32(crc32Orig);

            if( notUbiOpt && isUbiImage )
            {
                ret = snprintf(DestName, sizeof(DestName), "%s.dest.%u.%u", partPtr, ubiIdx, pid );
            }
            else if( DestPtr[0] != '/' )
            {
                ret = snprintf(DestName, sizeof(DestName), "%s/%s", CurrentWorkDir, DestPtr);
            }
            else
            {
                ret = snprintf(DestName, sizeof(DestName), "%s", DestPtr);
            }

            if (ret >= sizeof(DestName))
            {
                fprintf(stderr, "string truncated in file %s at line # %d", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }

            fdr = open( DestName, O_RDONLY );
            if( 0 > fdr )
            {
                fprintf(stderr, "Unable to open destination file %s: %m\n", DestPtr);
                exit(1);
            }
            fstat( fdr, &st );
            PatchMetaHeader.destSize = htobe32(st.st_size);

            PatchMetaHeader.ubiVolId = htobe32(ubiVolId);

            crc32Dest = LE_CRC_START_CRC32;

            snprintf( tmpName, sizeof(tmpName),
                      "patch.%u.bin",
                      pid );
            fdp = open( tmpName, O_WRONLY | O_TRUNC | O_CREAT, S_IWUSR | S_IRUSR );
            if( 0 > fdp )
            {
                fprintf(stderr, "Unable to open patch file %s: %m\n", tmpName);
                exit(1);
            }
            write( fdp, &PatchMetaHeader, sizeof(PatchMetaHeader) );

            while( 0 < (len = read( fdr, Chunk, chunkLen)) )
            {
                crc32Dest = le_crc_Crc32( Chunk, len, crc32Dest );
                snprintf( tmpName, sizeof(tmpName), "patchdest.%u.bin.%d", pid, patchNum );
                fdw = open( tmpName, O_WRONLY | O_TRUNC | O_CREAT, S_IWUSR | S_IRUSR );
                if( 0 <= fdw )
                {
                    write( fdw, Chunk, len );
                    close( fdw );
                }
                else {
                    fprintf( stderr, "Open of patch file %s fails: %m\n", tmpName );
                    exit(3);
                }

                ret = snprintf(CmdBuf, sizeof(CmdBuf),
                               BSDIFF " %s %s patched.%u.bin.%d",
                               OrigName, tmpName, pid, patchNum);
                if (ret >= sizeof(CmdBuf))
                {
                    fprintf(stderr, "command truncated in file %s at line # %d", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }

                if( IsVerbose )
                {
                    printf( "%s\n", CmdBuf );
                }
                ExecSystem( CmdBuf );

                ret = snprintf( tmpName, sizeof(tmpName), "patched.%u.bin.%d", pid, patchNum );
                if (ret >= sizeof(tmpName))
                {
                    fprintf(stderr, "string truncated in file %s at line # %d", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }

                fdw = open( tmpName, O_RDONLY );
                if( 0 > fdw )
                {
                    fprintf(stderr, "Unable to open destination file %s: %m\n", tmpName);
                    exit(1);
                }
                fstat( fdw, &st );
                PatchHeader.offset = htobe32(patchNum * chunkLen);
                patchNum++;
                PatchHeader.number = htobe32(patchNum);
                PatchHeader.size = htobe32(st.st_size);
                printf("Patch Header: offset 0x%x number %d size %u (0x%x)\n",
                       be32toh(PatchHeader.offset), be32toh(PatchHeader.number),
                       be32toh(PatchHeader.size), be32toh(PatchHeader.size));
                len = read( fdw, PatchedChunk, st.st_size );
                if( 0 > len )
                {
                    fprintf(stderr, "read() fails: %m\n" );
                    exit(4);
                }
                close(fdw);
                write( fdp, &PatchHeader, sizeof(PatchHeader) );
                write( fdp, PatchedChunk, st.st_size );
            }
            if( len < 0 )
            {
                fprintf(stderr, "read() fails: %m\n" );
                exit(4);
            }

            PatchMetaHeader.destCrc32 = htobe32(crc32Dest);
            PatchMetaHeader.numPatches = htobe32(patchNum);
            PatchMetaHeader.segmentSize = htobe32(chunkLen);
            memcpy( PatchMetaHeader.diffType, "BSDIFF40", 8 );

            lseek64( fdp, 0, SEEK_SET );
            write( fdp, &PatchMetaHeader, sizeof(PatchMetaHeader) );

            printf( "PATCH METAHEADER: segsize %x numpat %x ubiVolId %u "
                    "origsz %x origcrc %x destsz %x descrc %x\n",
                    be32toh(PatchMetaHeader.segmentSize), be32toh(PatchMetaHeader.numPatches),
                    be32toh(PatchMetaHeader.ubiVolId),
                    be32toh(PatchMetaHeader.origSize), be32toh(PatchMetaHeader.origCrc32),
                    be32toh(PatchMetaHeader.destSize), be32toh(PatchMetaHeader.destCrc32));
            close( fdr );
            close( fdp );

            snprintf( CmdBuf, sizeof(CmdBuf),
                      HDRCNV " patch.%u.bin -OH patch.%u.hdr -IT %s -PT %s -V \"1.0\" -B 00000001",
                      pid, pid, partPtr, productPtr );
            if( IsVerbose )
            {
                printf("%s\n", CmdBuf);
            }
            ExecSystem( CmdBuf );
            snprintf( tmpName, sizeof(tmpName), "patch.%u.hdr", pid );
            fdw = open( tmpName, O_RDWR );
            if( 0 > fdw )
            {
                fprintf(stderr, "failed to open patch header file %s: %m\n", tmpName );
                exit(5);
            }
            if (0 > lseek64( fdw, MISC_OPTS_OFFSET, SEEK_SET ))
            {
                fprintf(stderr, "%s %d; lseek64() fails: %m\n", __func__, __LINE__);
                close(fdw);
                exit(6);
            }
            read( fdw, Chunk, 1 );
            Chunk[0] |= MISC_OPTS_DELTAPATCH;
            if (0 > lseek64( fdw, MISC_OPTS_OFFSET, SEEK_SET ))
            {
                fprintf(stderr, "%s %d; lseek64() fails: %m\n", __func__, __LINE__);
                close(fdw);
                exit(7);
            }
            write( fdw, Chunk, 1 );
            close(fdw);
            ubiIdx++;

            if( noSpkgHeader )
            {
                ret = snprintf(CmdBuf, sizeof(CmdBuf),
                               "cat patch.%u.hdr patch.%u.bin >>%s/patch-%s.cwe",
                               pid, pid, CurrentWorkDir, productPtr);
            }
            else
            {
                ret = snprintf(CmdBuf, sizeof(CmdBuf),
                               "cat patch.%u.hdr patch.%u.bin >>patch.%u.cwe",
                               pid, pid, pid);
            }

            if (ret >= sizeof(CmdBuf))
            {
                fprintf(stderr, "command truncated in file %s at line # %d", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }

            ExecSystem( CmdBuf );

            snprintf( CmdBuf, sizeof(CmdBuf), "rm -f patch*.%u.bin*", pid );
            ExecSystem( CmdBuf );
        }
        while( (ubiIdx < nbVolume) );
    }

    if( !noSpkgHeader )
    {
        ret = snprintf(CmdBuf, sizeof(CmdBuf),
                       HDRCNV " patch.%u.cwe -OH patch.%u.cwe.hdr -IT %s -PT %s -V \"1.0\" -B 00000001",
                       pid, pid, pckgPtr, productPtr);
        if (ret >= sizeof(CmdBuf))
        {
            fprintf(stderr, "command truncated in file %s at line # %d", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        ExecSystem( CmdBuf );

        ret = snprintf(CmdBuf, sizeof(CmdBuf),
                       "cat patch.%u.cwe.hdr patch.%u.cwe >%s/patch-%s.cwe",
                       pid, pid, CurrentWorkDir, targetPtr);
        if (ret >= sizeof(CmdBuf))
        {
            fprintf(stderr, "command truncated in file %s at line # %d", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        ExecSystem( CmdBuf );
    }
    chdir(CurrentWorkDir);
    snprintf( CmdBuf, sizeof(CmdBuf), "rm -rf /tmp/patchdir.%u", pid );
    ExecSystem( CmdBuf );
    if( outPtr )
    {
        snprintf( CmdBuf, sizeof(CmdBuf), "mv patch-%s.cwe %s", targetPtr, outPtr );
        ExecSystem( CmdBuf );
    }

    exit( 0 );
}
