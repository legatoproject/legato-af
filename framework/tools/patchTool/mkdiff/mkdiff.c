//--------------------------------------------------------------------------------------------------
/**
 * @file mkDiff.c  Build delta patches between several images. This is an internal tool used by
 * mkDelta
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#define _LARGEFILE64_SOURCE
#include "legato.h"
#include <endian.h>
#include <libgen.h>

#include "flash-ubi.h"
#include "patch_utils.h"

//--------------------------------------------------------------------------------------------------
/**
 * Minimum partition size to compute delta
 */
//--------------------------------------------------------------------------------------------------
#define MIN_PART_SIZE_FOR_DELTA   1024*1024

//--------------------------------------------------------------------------------------------------
/**
 * Don't care value
 */
//--------------------------------------------------------------------------------------------------
#define UNKNOWN_VALUE   0xFFFFFFFF

//--------------------------------------------------------------------------------------------------
/**
 * Squashfs magic
 */
//--------------------------------------------------------------------------------------------------
#define SQUASH_MAGIC     "hsqs"

//--------------------------------------------------------------------------------------------------
/**
 * Temporary patch file prefix
 */
//--------------------------------------------------------------------------------------------------
#define PATCH_FILE_PREFIX     "patch-"

//--------------------------------------------------------------------------------------------------
/**
 * Structure to store extracted ubi volume info
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char volumePath[PATH_MAX];
    size_t imageSize;
    uint32_t crc32;
    uint8_t volType;
    uint8_t volFlags;
}
ExtractInfo_t;

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
 * MDM9x06 and MDM9x07 partition scheme. This is platform dependant
 */
//--------------------------------------------------------------------------------------------------
static PartToSpkg_t mdm9x07_PartToSpkg[] =
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
static char CmdBuf[4*PATH_MAX];

//--------------------------------------------------------------------------------------------------
/**
 * Original working directory. The tool creates and goes to a temporary work directory due to
 * some tools issues with global path.
 */
//--------------------------------------------------------------------------------------------------
static char CurrentWorkDir[PATH_MAX];

//--------------------------------------------------------------------------------------------------
/**
 * Comparison window size. This is needed to specify imgdiff to search for a block in within certain
 * window. This help to reduce the delta computation with price of delta size.
 */
//--------------------------------------------------------------------------------------------------
static uint32_t WindowSize=0;

//--------------------------------------------------------------------------------------------------
/**
 * Call at exit(3) to perform all clean-up actions
 */
//--------------------------------------------------------------------------------------------------
static void Exithandler
(
    void
)
{
    chdir(CurrentWorkDir);
    snprintf( CmdBuf, sizeof(CmdBuf),
              "rm -rf /tmp/patchdir.%u",
              getpid() );
    printf("Removing dir command: %s", CmdBuf);
    (void)system( CmdBuf );
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
            "usage: %s -T TARGET [-o patchname] [-S 4K|2K] [-E 256K|128K]  [-v] \n"
            "        [-w WindowSize] -p PART  file-src file-tgt\n",
            ProgName );
    fprintf(stderr, "\nNote: This is an internal tool which is called by 'mkdelta' tool.\n"
                    "      User should call 'mkdelta' tool to create delta patch.\n");

    fprintf(stderr, "\n");
    fprintf(stderr, "   -T, --target <TARGET>\n"
                    "        Specify the TARGET (mandatory - specified only one time).\n");
    fprintf(stderr, "   -o, <patchname>\n"
                    "        Specify the output name of the patch."
                           " Else use patch-<file-tgt>-<TARGET>.cwe as default.\n");
    fprintf(stderr, "   -S, --pagesize <4K|2K>\n"
                    "        Specify another page size (optional - specified only one time).\n");
    fprintf(stderr, "   -E, --pebsize <256K|128K>\n"
                    "        Specify another PEB size (optional - specified only one time).\n");
    fprintf(stderr, "   -v, --verbose\n"
                    "        Be verbose.\n");
    fprintf(stderr, "   -p, --partition <PART>\n"
                    "        Specify the partition where apply the patch.\n");
    fprintf(stderr, "   -w, --window <WindowSize>\n"
                    "        Specify the comparison window size.\n");
    fprintf(stderr, "\n");
    exit(1);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get path based on current working directory.
 */
//--------------------------------------------------------------------------------------------------
static void GetDirPath
(
    const char* inputPathPtr,
    char* outPathPtr,
    size_t outLen
)
{
    if( inputPathPtr[0] != '/' )
    {
       snprintf(outPathPtr, outLen, "%s/%s", CurrentWorkDir, inputPathPtr);
    }
    else
    {
       snprintf(outPathPtr, outLen, "%s", inputPathPtr);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Extract all the ubi volumes from supplied file. No need to check return value. It will exit on
 * failure.
 */
//--------------------------------------------------------------------------------------------------
static ExtractInfo_t* SplitUbiImage
(
    const char* filePtr,      // [IN] File to split
    int* nbVolumePtr          // [OUT] No of ubi volumes
)
{
    ExtractInfo_t* volumeInfo = NULL;
    char filePath[PATH_MAX] = "";
    int noUbiVolume = -1;
    char *pathPtr, *bname;
    le_result_t result = LE_FAULT;
    struct stat st;

    GetDirPath(filePtr, filePath, sizeof(filePath));

    int fd = open( filePath, O_RDONLY );

    if( 0 > fd )
    {
       fprintf(stderr, "Unable to open file %s: %m\n", filePath);
       exit(1);
    }

    if (fstat( fd, &st ) < 0)
    {
        fprintf(stderr, "Failed to obtain info of file: %s\n", filePath);
        exit(1);
    }

    result = utils_ScanUbi(fd,
                           st.st_size,
                           FlashPEBSize,
                           FlashPageSize,
                           &noUbiVolume);

    if ((LE_OK != result) || (noUbiVolume < 0))
    {
        fprintf(stderr, "Failed extract ubi volume info from file: %s\n", filePath);
        exit(1);
    }

    volumeInfo = calloc(noUbiVolume, sizeof(ExtractInfo_t));

    if (NULL == volumeInfo)
    {
        fprintf(stderr, "calloc() failed\n");
        exit(1);
    }

    pathPtr = strdupa(filePath);   //strdupa should be ok as it is running on host machine.
    bname = basename(pathPtr);

    int i = 0;

    for( i = 0; i < noUbiVolume; i++ )
    {
       snprintf(volumeInfo[i].volumePath,
                sizeof(volumeInfo[i].volumePath),
                "%s.ubiVol.%u", bname, i);
       if (IsVerbose)
       {
           printf("Extracting volume: %d to file: %s\n", i, volumeInfo[i].volumePath);
       }
       result = utils_ExtractUbiData( fd,
                                      i,
                                      volumeInfo[i].volumePath,
                                      FlashPEBSize,
                                      FlashPageSize,
                                      &volumeInfo[i].volType,
                                      &volumeInfo[i].volFlags,
                                      &volumeInfo[i].imageSize,
                                      &volumeInfo[i].crc32 );
       if (LE_OK != result)
       {
           fprintf(stderr, "Failed to extract ubi volume: %d from file: %s",
                   i,
                   filePath);
           exit(1);
       }
    }
    close(fd);

    *nbVolumePtr = noUbiVolume;

    return volumeInfo;
}

//--------------------------------------------------------------------------------------------------
/**
 * Prepend patch meta data with patch file
 */
//--------------------------------------------------------------------------------------------------
static void PrependMetaData
(
    char *patchFile,                          // Patch file path.
    DeltaPatchMetaHeader_t* patchHeader       // Patch meta data.
)
{
    char patchMeta[NAME_MAX] = "patch-Meta-XXXXXX";
    char patchTemp[NAME_MAX] = "patch-Temp-XXXXXX";
    mode_t currentUmask;

    printf( "PATCH METAHEADER:\n"
            "\t\t\t\t DiffType: %s\n"
            "\t\t\t\t segsize %x\n"
            "\t\t\t\t numpat %x \n"
            "\t\t\t\t ubiVolId %hu \n"
            "\t\t\t\t ubiVoltype %hhu \n"
            "\t\t\t\t ubiVolFlags %hhx \n"
            "\t\t\t\t origSize %x\n"
            "\t\t\t\t origCrc %x\n"
            "\t\t\t\t destSize %x\n"
            "\t\t\t\t desCrc %x\n",
            patchHeader->diffType,
            be32toh(patchHeader->segmentSize),
            be32toh(patchHeader->numPatches),
            be16toh(patchHeader->ubiVolId),
            patchHeader->ubiVolType,
            patchHeader->ubiVolFlags,
            be32toh(patchHeader->origSize),
            be32toh(patchHeader->origCrc32),
            be32toh(patchHeader->destSize),
            be32toh(patchHeader->destCrc32));

    currentUmask = umask(S_IRWXO | S_IRWXG);
    int fd = mkstemp(patchMeta);
    umask(currentUmask);

    if (fd < 0)
    {
        fprintf(stderr, "Failed to create temporary file for writing metadata: %s\n", patchMeta);
        exit(1);
    }
    lseek( fd, 0, SEEK_SET );
    write( fd, patchHeader, sizeof(DeltaPatchMetaHeader_t) );
    close(fd);

    currentUmask = umask(S_IRWXO | S_IRWXG);
    fd = mkstemp(patchTemp);
    umask(currentUmask);

    if (fd < 0)
    {
        fprintf(stderr, "Failed to create temporary file for appending metadata: %s\n", patchTemp);
        exit(1);
    }

    snprintf(CmdBuf, sizeof(CmdBuf), "cat %s %s > %s", patchMeta, patchFile, patchTemp);
    utils_ExecSystem(CmdBuf);

    snprintf(CmdBuf, sizeof(CmdBuf), "cat %s > %s", patchTemp, patchFile);
    utils_ExecSystem(CmdBuf);

    unlink(patchMeta);
    unlink(patchTemp);
}

//--------------------------------------------------------------------------------------------------
/**
 * Compute delta using imgdiff
 */
//--------------------------------------------------------------------------------------------------
static void ComputeDeltaSqsh
(
    ExtractInfo_t *srcVolumeInfo,  // Source file ubi volume info
    ExtractInfo_t *tgtVolumeInfo,  // Target file ubi volume info
    int ubiIndex,                  // UBI volume index
    char *patchFile                // Patch file path.

)
{
    char options[NAME_MAX] = "";
    char windowOption[NAME_MAX] = "";
    char verboseOption[NAME_MAX] = "";
    DeltaPatchMetaHeader_t patchHeader = {{0}, 0, 0, 0, 0, 0, 0, 0};


    if (WindowSize > 0)
    {
        snprintf(windowOption, sizeof(windowOption), " -w %u", WindowSize);
    }

    if (IsVerbose)
    {
        snprintf(verboseOption, sizeof(verboseOption), " -v");
    }

    snprintf(options, sizeof(options), "%s %s", windowOption, verboseOption);

    snprintf( CmdBuf, sizeof(CmdBuf),
                      IMGDIFF " %s %s %s %s",
                      srcVolumeInfo[ubiIndex].volumePath,
                      tgtVolumeInfo[ubiIndex].volumePath,
                      patchFile,
                      options);

    utils_ExecSystem( CmdBuf );

    patchHeader.origSize = htobe32(srcVolumeInfo[ubiIndex].imageSize);
    patchHeader.origCrc32 = htobe32(srcVolumeInfo[ubiIndex].crc32);
    patchHeader.destSize = htobe32(tgtVolumeInfo[ubiIndex].imageSize);
    patchHeader.destCrc32 = htobe32(tgtVolumeInfo[ubiIndex].crc32);
    patchHeader.ubiVolId = htobe16(ubiIndex);
    patchHeader.ubiVolType = tgtVolumeInfo[ubiIndex].volType;
    patchHeader.ubiVolFlags = tgtVolumeInfo[ubiIndex].volFlags;
    patchHeader.numPatches = -1;     // Not used in imgdiff
    patchHeader.segmentSize = -1;    // Not used in imgdiff
    memcpy( patchHeader.diffType, "IMGDIFF2", 8 );

    PrependMetaData(patchFile, &patchHeader);
}

//--------------------------------------------------------------------------------------------------
/**
 * Append small partitions
 */
//--------------------------------------------------------------------------------------------------
static void AppendSmallVolumes
(
    ExtractInfo_t *tgtVolumeInfo,  // Target file ubi volume info
    int ubiIndex,                  // UBI volume index
    char *patchFile                // Patch file path.

)
{
    DeltaPatchMetaHeader_t patchHeader = {{0}, 0, 0, 0, 0, 0, 0, 0};

    snprintf( CmdBuf, sizeof(CmdBuf),
                      "cp %s %s",
                      tgtVolumeInfo[ubiIndex].volumePath,
                      patchFile);

    utils_ExecSystem( CmdBuf );

    patchHeader.origSize = UNKNOWN_VALUE;
    patchHeader.origCrc32 = UNKNOWN_VALUE;
    patchHeader.destSize = htobe32(tgtVolumeInfo[ubiIndex].imageSize);
    patchHeader.destCrc32 = htobe32(tgtVolumeInfo[ubiIndex].crc32);
    patchHeader.ubiVolId = htobe16(ubiIndex);
    patchHeader.ubiVolType = (uint8_t)-1;
    patchHeader.ubiVolFlags = (uint8_t)-1;
    patchHeader.numPatches = UNKNOWN_VALUE;
    patchHeader.segmentSize = UNKNOWN_VALUE;
    memcpy( patchHeader.diffType, NODIFF, 8 );

    PrependMetaData(patchFile, &patchHeader);
}

//--------------------------------------------------------------------------------------------------
/**
 * Check whether imgdiff can be applied or not
 *
 * return
 *    - true if imgdiff can be applied
 *    - false if can't
 */
//--------------------------------------------------------------------------------------------------
static bool CanApplyImgdiff
(
    char *srcFile             // Source file
)
{
    char str[NAME_MAX] = "";
    // Check whether it is squashfs file system or not
    FILE* file = fopen(srcFile, "r");

    if (IsVerbose)
    {
        printf("Checking squashfs flag for source file: %s\n", srcFile);
    }

    if (NULL == file)
    {
        fprintf(stderr, "Failed to open: %s. %m\n", srcFile);
        exit( 1 );
    }

    if (fread(str, 1, strlen(SQUASH_MAGIC), file) < strlen(SQUASH_MAGIC))
    {
        fprintf(stderr, "Failed to read: %s. %m\n", srcFile);
        exit( 1 );
    }

    str[strlen(SQUASH_MAGIC)] = '\0';

    if (IsVerbose)
    {
        printf("Squashfs magic read: %s, sizeof(magic) = %zd\n", str, strlen(SQUASH_MAGIC));
    }

    fclose(file);

    return 0 == strcmp(str, SQUASH_MAGIC);

}

//--------------------------------------------------------------------------------------------------
/**
 * Update cwe header to mark the patch as delta patch.
 *
 */
//--------------------------------------------------------------------------------------------------
static void MarkPatchAsDelta
(
    const char* patchHdrPathPtr
)
{
    char chunk[NAME_MAX] = "";

    int fd = open( patchHdrPathPtr, O_RDWR );

    if( 0 > fd )
    {
        fprintf(stderr, "failed to open patch header file %s: %m\n", patchHdrPathPtr );
        exit(5);
    }

    if (0 > lseek64( fd, MISC_OPTS_OFFSET, SEEK_SET ))
    {
        fprintf(stderr, "%s %d; lseek64() fails: %m\n", __func__, __LINE__);
        close(fd);
        exit(6);
    }

    if (0 > read( fd, chunk, 1 ))
    {
        fprintf(stderr, "failed to read patch header file %s: %m\n", patchHdrPathPtr );
        exit(1);
    }
    chunk[0] |= MISC_OPTS_DELTAPATCH;

    if (0 > lseek64( fd, MISC_OPTS_OFFSET, SEEK_SET ))
    {
        fprintf(stderr, "%s %d; lseek64() fails: %m\n", __func__, __LINE__);
        close(fd);
        exit(7);
    }

    write( fd, chunk, 1 );
    close(fd);

}

//--------------------------------------------------------------------------------------------------
/**
 * Generate cwe file and append it to patch file
 */
//--------------------------------------------------------------------------------------------------
static void AppendCweHeader
(
    const char* patchPathPtr,  // Patch file path.
    const char* outPatchPtr,   // Output patch file
    const char* partPtr,       // Partition name
    const char* productPtr     // Product name
)
{
    char patchHdrPath[PATH_MAX] = "";
    char *pathPtr, *bname;

    pathPtr = strdupa(patchPathPtr);   //strdupa should be ok as it is running on host machine.
    bname = basename(pathPtr);

    // Now create cwe header and append it.

    snprintf(patchHdrPath, sizeof(patchHdrPath), "%s.hdr", bname);

    snprintf( CmdBuf, sizeof(CmdBuf),
              HDRCNV " %s -OH %s -IT %s -PT %s -V \"1.0\" -B 00000001",
              patchPathPtr, patchHdrPath, partPtr, productPtr );
    utils_ExecSystem(CmdBuf);

    MarkPatchAsDelta(patchHdrPath);

    // Append the cwe file to patch file
    snprintf(CmdBuf, sizeof(CmdBuf), "cat %s %s > %s", patchHdrPath, patchPathPtr, outPatchPtr);
    utils_ExecSystem(CmdBuf);
}

//--------------------------------------------------------------------------------------------------
/**
 * Compute delta ubi partitions
 */
//--------------------------------------------------------------------------------------------------
static void ComputeDeltaUBI
(
    const char* srcFile,             // Source file
    const char* tgtFile,             // Target file
    const char* partPtr,             // Partition name
    const char* productPtr,          // Product name
    const char* patchFile            // file containing cwe header
)
{

    // Split the ubi partition
    int i;
    int noVolSrc;
    int nbVolTgt;

    ExtractInfo_t* srcVolumeInfo = SplitUbiImage(srcFile, &noVolSrc);
    ExtractInfo_t* destVolumeInfo = SplitUbiImage(tgtFile, &nbVolTgt);

    if( noVolSrc != nbVolTgt )
    {
       int yn;

       fprintf(stderr,
               "Number of volumes differs between source (%u) and target (%u)\n",
               noVolSrc, nbVolTgt);
       fprintf(stderr, "Build patch anyway [y/N] ? ");
       yn = getchar();
       if( toupper(yn) != (int)'Y' )
       {
           exit(0);
       }
    }

    // Compute delta for squashfs
    // Append other dm-verity stuffs to ubi
    char tmpPatchPath[PATH_MAX] = "";
    char tmpVolPatchPath[PATH_MAX] = "";
    char *pathPtr, *bname;

    pathPtr = strdupa(tgtFile);   //strdupa should be ok as it is running on host machine.
    bname = basename(pathPtr);

    snprintf(tmpPatchPath, sizeof(tmpPatchPath), PATCH_FILE_PREFIX"ubi-%s", bname);

    // No need to check the existence of temporary files as all of them created in /tmp directory
    // and will be deleted when this tool exits.

    for (i = 0; i < nbVolTgt; i++)
    {
        snprintf(tmpVolPatchPath, sizeof(tmpVolPatchPath), PATCH_FILE_PREFIX"vol-%d-%s",
                 i, bname);

        if (destVolumeInfo[i].imageSize > MIN_PART_SIZE_FOR_DELTA)
        {
            if (CanApplyImgdiff(destVolumeInfo[i].volumePath))
            {
                ComputeDeltaSqsh(srcVolumeInfo, destVolumeInfo, i, tmpVolPatchPath);
            }
            else
            {
                fprintf(stderr,
                        "Delta for only squashfs over ubi is supported\n");
                exit(1);
            }
        }
        else
        {
            AppendSmallVolumes(destVolumeInfo, i, tmpVolPatchPath);
        }
        snprintf(CmdBuf, sizeof(CmdBuf), "cat %s >> %s", tmpVolPatchPath, tmpPatchPath);
        utils_ExecSystem(CmdBuf);

    }

    // No need to unlink temporary files as they will be deleted once the tool exits.

    AppendCweHeader(tmpPatchPath, patchFile, partPtr, productPtr);

    free(srcVolumeInfo);
    free(destVolumeInfo);
}

//--------------------------------------------------------------------------------------------------
/**
 * Compute delta using bsdiff. This is mainly used for images stored in raw flash(kernel, initramfs)
 */
//--------------------------------------------------------------------------------------------------
static void ComputeDeltaRawFlash
(
    const char *srcPathPtr,    // Source file path
    const char *tgtPathPtr,    // Target file path
    const char *patchPathPtr   // Patch file path.

)
{
    DeltaPatchMetaHeader_t patchMetaHeader = {{0}, 0, 0, 0, 0, 0, 0, 0};
    DeltaPatchHeader_t     patchHeader     = {0, 0, 0};
    uint32_t crc32Orig, crc32Dest;
    uint32_t chunkLen = SEGMENT_SIZE;
    uint32_t patchNum = 0;
    char tmpName[NAME_MAX] = "";
    int pid = getpid();
    int len = 0;
    struct stat st;


    int fdr = open( srcPathPtr, O_RDONLY );
    if( 0 > fdr )
    {
        fprintf(stderr, "Unable to open origin file %s: %m\n", srcPathPtr);
        exit(1);
    }
    if (fstat( fdr, &st ) < 0)
    {
        fprintf(stderr, "Failed to obtain info of origin file: %s\n", srcPathPtr);
        exit(1);
    }
    patchMetaHeader.origSize = htobe32(st.st_size);
    crc32Orig = LE_CRC_START_CRC32;

    while( 0 < (len = read( fdr, Chunk, chunkLen )) )
    {
        crc32Orig = le_crc_Crc32( Chunk, len, crc32Orig );
    }
    close( fdr );
    patchMetaHeader.origCrc32 = htobe32(crc32Orig);

    fdr = open( tgtPathPtr, O_RDONLY );
    if( 0 > fdr )
    {
        fprintf(stderr, "Unable to open destination file %s: %m\n", tgtPathPtr);
        exit(1);
    }
    if (fstat( fdr, &st ) < 0)
    {
        fprintf(stderr, "Failed to obtain info of destination file: %s\n", tgtPathPtr);
        exit(1);
    }
    patchMetaHeader.destSize = htobe32(st.st_size);
    patchMetaHeader.ubiVolId = htobe16(((uint16_t)-1));
    patchMetaHeader.ubiVolType = (uint8_t)-1;
    patchMetaHeader.ubiVolFlags = (uint8_t)-1;

    crc32Dest = LE_CRC_START_CRC32;

    int fdp = open( patchPathPtr, O_WRONLY | O_TRUNC | O_CREAT, S_IWUSR | S_IRUSR );
    if( 0 > fdp )
    {
        fprintf(stderr, "Unable to open patch file %s: %m\n", patchPathPtr);
        exit(1);
    }
    write( fdp, &patchMetaHeader, sizeof(patchMetaHeader) );

    while( 0 < (len = read( fdr, Chunk, chunkLen)) )
    {
        crc32Dest = le_crc_Crc32( Chunk, len, crc32Dest );
        snprintf( tmpName, sizeof(tmpName), "patchdest.%u.bin.%d", pid, patchNum );
        int fdw = open( tmpName, O_WRONLY | O_TRUNC | O_CREAT, S_IWUSR | S_IRUSR );
        if( 0 <= fdw )
        {
            write( fdw, Chunk, len );
            close( fdw );
        }
        else {
            fprintf( stderr, "Open of patch file %s fails: %m\n", tmpName );
            exit(3);
        }
        snprintf( CmdBuf, sizeof(CmdBuf),
                  BSDIFF " %s %s patched.%u.bin.%d",
                  srcPathPtr, tmpName, pid, patchNum );

        utils_ExecSystem( CmdBuf );
        snprintf( tmpName, sizeof(tmpName), "patched.%u.bin.%d", pid, patchNum );
        fdw = open( tmpName, O_RDONLY );
        if( 0 > fdw )
        {
            fprintf(stderr, "Unable to open destination file %s: %m\n", tmpName);
            exit(1);
        }
        if (fstat( fdw, &st ))
        {
            fprintf(stderr, "Failed to obtain info of file %s: %m\n", tmpName);
            exit(1);
        }
        patchHeader.offset = htobe32(patchNum * chunkLen);
        patchNum++;
        patchHeader.number = htobe32(patchNum);
        patchHeader.size = htobe32(st.st_size);
        printf("Patch Header: offset 0x%x number %d size %u (0x%x)\n",
               be32toh(patchHeader.offset), be32toh(patchHeader.number),
               be32toh(patchHeader.size), be32toh(patchHeader.size));
        len = read( fdw, PatchedChunk, st.st_size );
        if( 0 > len )
        {
            fprintf(stderr, "read() fails: %m\n" );
            exit(4);
        }
        close(fdw);
        write( fdp, &patchHeader, sizeof(patchHeader) );
        write( fdp, PatchedChunk, st.st_size );
    }
    if( len < 0 )
    {
        fprintf(stderr, "read() fails: %m\n" );
        exit(4);
    }

    patchMetaHeader.destCrc32 = htobe32(crc32Dest);
    patchMetaHeader.numPatches = htobe32(patchNum);
    patchMetaHeader.segmentSize = htobe32(chunkLen);
    memcpy( patchMetaHeader.diffType, "BSDIFF40", 8 );

    if (0 > lseek64( fdp, 0, SEEK_SET ))
    {
        fprintf(stderr, "%s %d; lseek64() fails: %m\n", __func__, __LINE__);
        close(fdp);
        exit(6);
    }
    write( fdp, &patchMetaHeader, sizeof(patchMetaHeader) );

    printf( "PATCH METAHEADER: segsize %x numpat %x ubiVolId %hu "
            "ubiVolType %hhu ubiVolFlags %hhX "
            "origsz %x origcrc %x destsz %x descrc %x\n",
            be32toh(patchMetaHeader.segmentSize), be32toh(patchMetaHeader.numPatches),
            be16toh(patchMetaHeader.ubiVolId),
            patchMetaHeader.ubiVolType, patchMetaHeader.ubiVolFlags,
            be32toh(patchMetaHeader.origSize), be32toh(patchMetaHeader.origCrc32),
            be32toh(patchMetaHeader.destSize), be32toh(patchMetaHeader.destCrc32));
    close( fdr );
    close( fdp );
}

//--------------------------------------------------------------------------------------------------
/**
 * Compute delta WP non-ubi partitions
 */
//--------------------------------------------------------------------------------------------------
static void ComputeDeltaNonUBI
(
    const char* srcFilePtr,          // Source file
    const char* tgtFilePtr,          // Target file
    const char* partPtr,             // Partition name
    const char* productPtr,          // Product name
    const char* patchFilePtr         // Output patch file
)
{
    char srcFilePath[PATH_MAX] = "";
    char tgtFilePath[PATH_MAX] = "";

    GetDirPath(srcFilePtr, srcFilePath, sizeof(srcFilePath));
    GetDirPath(tgtFilePtr, tgtFilePath, sizeof(tgtFilePath));

    char tmpPatchPath[PATH_MAX] = "";
    char *pathPtr, *bname;

    pathPtr = strdupa(tgtFilePtr);   //strdupa should be ok as it is running on host machine.
    bname = basename(pathPtr);

    snprintf(tmpPatchPath, sizeof(tmpPatchPath), PATCH_FILE_PREFIX"nonubi-%s", bname);

    ComputeDeltaRawFlash(srcFilePath, tgtFilePath, tmpPatchPath);

    AppendCweHeader(tmpPatchPath, patchFilePtr, partPtr, productPtr);
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
    int iargc = argc;
    char** argvPtr = &argv[1];

    char* partPtr = NULL;
    char* productPtr = NULL;
    char* targetPtr = NULL;
    char* outPtr = NULL;
    PartToSpkg_t* partToSpkgPtr = NULL;
    bool isUbiImage = false;
    pid_t pid = getpid();
    char env[4096];
    char* toolchainPtr = NULL;
    char* toolchainEnvPtr = NULL;
    char* envPtr = NULL;
    char* srcPathPtr = NULL;
    char* tgtPathPtr = NULL;

    ProgName = argv[0];

    utils_CheckForTool( BSDIFF, NULL );
    utils_CheckForTool( IMGDIFF, NULL );


    getcwd(CurrentWorkDir, sizeof(CurrentWorkDir));
    atexit( Exithandler );

    while( argc > 1 )
    {
        if( (iargc >= 5) &&
            ((0 == strcmp(*argvPtr, "--target")) || (0 == strcmp(*argvPtr, "-T"))) )
        {
            ++argvPtr;
            if( 0 == strcasecmp( *argvPtr, "wp76xx" ))
            {
                productPtr = "9X28";
                targetPtr = "wp76xx";
                partToSpkgPtr = mdm9x07_PartToSpkg;
                FlashPageSize = FLASH_PAGESIZE_4K;
                FlashPEBSize = FLASH_PEBSIZE_256K;
                toolchainPtr = "WP76XX_TOOLCHAIN_DIR";
            }
            else if( 0 == strcasecmp( *argvPtr, "wp77xx" ))
            {
                productPtr = "9X06";
                targetPtr = "wp77xx";
                partToSpkgPtr = mdm9x07_PartToSpkg;
                FlashPageSize = FLASH_PAGESIZE_4K;
                FlashPEBSize = FLASH_PEBSIZE_256K;
                toolchainPtr = "WP77XX_TOOLCHAIN_DIR";
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
#ifdef __COVERITY__
                __coverity_tainted_data_sanitize__(env);
#endif // __COVERITY__
                putenv( env );
            }
            else
            {
                fprintf(stderr,
                        "Variable '%s' is not set for target %s\n",
                        toolchainPtr, targetPtr);
                exit(1);
            }
            utils_CheckForTool( HDRCNV, toolchainPtr );
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
        else if( (iargc >= 5) &&
                 ((0 == strcmp(*argvPtr, "--window")) || (0 == strcmp(*argvPtr, "-w"))) )
        {
            ++argvPtr;
            char *endPtr;
            errno = 0;
            WindowSize = strtoul( *argvPtr, &endPtr, 10 );
            if( (errno) || (*endPtr))
            {
                fprintf(stderr, "Incorrect window size '%s'\n", *argvPtr );
                exit( 1 );
            }
            ++argvPtr;
            iargc -= 2;
        }
        else if( (iargc >= 5) &&
            ((0 == strcmp(*argvPtr, "--partition")) || (0 == strcmp(*argvPtr, "-p"))) )
        {
            int ip;

            ++argvPtr;

            if (!targetPtr)
            {
                fprintf(stderr, "Target should be specified before partition\n");
                exit(1);
            }

            if (!partToSpkgPtr)
            {
                fprintf(stderr, "Partition to spkg mapping not done\n");
                exit(1);
            }

            for( ip = 0; partToSpkgPtr[ip].imageType; ip++ )
            {
                if( 0 == strcasecmp( *argvPtr, partToSpkgPtr[ip].imageType ) )
                {
                    partPtr = partToSpkgPtr[ip].imageType;
                    isUbiImage = partToSpkgPtr[ip].isUbiImage;
                    break;
                }
            }
            if( !partToSpkgPtr[ip].imageType )
            {
                fprintf(stderr, "Unknown partition %s for target: %s\n", *argvPtr, targetPtr);
                exit(1);
            }
            ++argvPtr;
            iargc -= 2;
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

    if( (!partPtr) )
    {
        fprintf(stderr, "Missing PART\n");
        Usage();
    }

    // We still need source and target image. Check whether it is supplied or not
    if( iargc != 3)
    {
        Usage();
    }

    // Set the verbose level
    utils_beVerbose(IsVerbose);

    srcPathPtr = *(argvPtr++);
    tgtPathPtr = *(argvPtr++);

    if( (!srcPathPtr) )
    {
        fprintf(stderr, "Missing source file\n");
        Usage();
    }

    if( (!tgtPathPtr) )
    {
        fprintf(stderr, "Missing target file\n");
        Usage();
    }

    struct stat st;

    if ( stat(tgtPathPtr, &st) < 0 )
    {
        fprintf(stderr, "Failed to stat file: %s", tgtPathPtr);
        exit(1);
    }

    if (st.st_size <= MIN_PART_SIZE_FOR_DELTA)
    {
        fprintf(stderr, "Delta generation isn't supported for partition < %d bytes",
                MIN_PART_SIZE_FOR_DELTA);
        exit(1);
    }


    // Now change directory to some temp directory

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

    char tmpPatchPath[PATH_MAX] = "";
    char *pathPtr, *bname;

    pathPtr = strdupa(tgtPathPtr);   //strdupa should be ok as it is running on host machine.
    bname = basename(pathPtr);

    snprintf(tmpPatchPath, sizeof(tmpPatchPath), "/tmp/patchdir.%u/"PATCH_FILE_PREFIX"%s-%s.cwe",
             pid, bname, targetPtr);

    if (isUbiImage)
    {
        ComputeDeltaUBI(srcPathPtr, tgtPathPtr, partPtr, productPtr, tmpPatchPath);
    }
    else
    {
        ComputeDeltaNonUBI(srcPathPtr, tgtPathPtr, partPtr, productPtr, tmpPatchPath);
    }

    chdir(CurrentWorkDir);

    if( outPtr )
    {
        snprintf( CmdBuf, sizeof(CmdBuf), "mv %s %s", tmpPatchPath, outPtr );
    }
    else
    {
        snprintf( CmdBuf, sizeof(CmdBuf), "mv %s "PATCH_FILE_PREFIX"%s.cwe",
                  tmpPatchPath, productPtr);
    }
#ifdef __COVERITY__
    __coverity_tainted_data_sanitize__(CmdBuf);
#endif // __COVERITY__
    utils_ExecSystem( CmdBuf );

    snprintf( CmdBuf, sizeof(CmdBuf), "rm -rf /tmp/patchdir.%u", pid );
    utils_ExecSystem( CmdBuf );

    exit( 0 );
}
