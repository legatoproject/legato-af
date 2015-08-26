//--------------------------------------------------------------------------------------------------
/** @file sysInfo.c
 *
 * This app exposes assets over lwm2m that allow for inspection of system CPU usage, Memory
 * usage and available flash space.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include <sys/statfs.h>

#define CPU_LOAD_FIELD "CpuLoad"
#define MEM_FREE_FIELD "FreeMem"

//--------------------------------------------------------------------------------------------------
/**
 * A structure to store some local information about each flash partition
 */
//--------------------------------------------------------------------------------------------------
typedef struct PartitionInfo {
    char partition[32];
    char name[32];
    char mountpoint[256]; /* so we can statfs - and if we have one then this partition is mounted */
    char filesystem[32];
    uint32_t size;
    uint32_t eraseBlockSize;
    int nonBlankBlocks;
    le_avdata_AssetInstanceRef_t partitionAssetRef;
} PartitionInfo_t;

//--------------------------------------------------------------------------------------------------
/**
 * We build with seven partitions - a customer could repartition and have a different number, but
 * rather than dynamically allocate I'll just make this static and chose a reasonable upper bound
 * on the number of partitions.
 */
//--------------------------------------------------------------------------------------------------
static PartitionInfo_t FlashInfo[16];


//--------------------------------------------------------------------------------------------------
/**
 * The reference forthe single instance of the sysinfo asset. It's not actually necessary to
 * keep track of this as it is all automatically cleaned up in the end but it is used in test code.
 */
//--------------------------------------------------------------------------------------------------
static le_avdata_AssetInstanceRef_t AssetRef;

//--------------------------------------------------------------------------------------------------
/**
 * Returns the position of the nth occurance of a character or max
 *
 * @return
 *      The index at which the nth occurance of the given character of max if not found.
 */
//--------------------------------------------------------------------------------------------------

static int IndexOfNthChar(const char* haystack, char needle, int n, int max)
{
    int index = 0;
    while ((n > 0) && (index < max))
    {
        if (haystack[index] == needle)
        {
            n--;
        }
        index++;
    }
    return index;
}

//--------------------------------------------------------------------------------------------------
/**
 * If the given line matches the target string capture the memory size.
 *
 * @return
 *      true if the memory line matched the target and the pointed to int contains valid data
 */
//--------------------------------------------------------------------------------------------------
static bool ParseMemInfoLine(const char* target, const char* line, int* sizeKb)
{
    int len = strlen(target);
    *sizeKb = -1;
    if (strncmp(target, line, len) == 0)
    {
        *sizeKb = atoi(line+len);
    }
    /* while a value of 0 could also indicate no valid number was found ... */
    return (sizeKb >= 0);
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse out the values we want from the partial read of /proc/meminfo
 *
 * @note
 *      meminfo returns a multiline file that needs parsing to get the information we want
 *
 * @return
 *      true if values were found for both free memory and total memory
 */
//--------------------------------------------------------------------------------------------------
static bool ParseMemTotalAndFree(char* input, int* memTotal, int* memFree)
{
    char* token = strtok(input, "\n");
    bool foundTotal = false;
    bool foundFree = false;


    while (!(foundTotal && foundFree)  && (token != NULL))
    {
        if (!foundTotal)
        {
            foundTotal = ParseMemInfoLine("MemTotal:", token, memTotal);
        }
        else if (!foundFree)
        {
            foundFree = ParseMemInfoLine("MemFree:", token, memFree);
        }
        token = strtok(NULL, "\n");
    }
    return foundTotal && foundFree;
}

//--------------------------------------------------------------------------------------------------
/**
 * Handle the request to update the system memory asset
 *
 * @note
 *      We are only returning free memory and total memory values from meminfo which are found
 *      within the first 256 bytes. We don't need to read more from /proc/meminfo than that.
 *
 * @return
 *      None
 */
//--------------------------------------------------------------------------------------------------
static void HandleFreeMemory(
    le_avdata_AssetInstanceRef_t instRef,
    const char* fieldName,
    void* contextPtr
)
{
    char freeMem[256];
    int memTotal = -1;
    int memFree = -1;

    strncpy(freeMem, "Could not determine free memory", sizeof(freeMem) - 1);
    FILE* f = fopen("/proc/meminfo", "r");
    if (f != NULL)
    {
        int bytes = fread(freeMem, 1, 255, f);
        fclose(f);
        freeMem[bytes] = '\0';
        ParseMemTotalAndFree(freeMem, &memTotal, &memFree);
        snprintf(freeMem, sizeof(freeMem), "Used %d of %d kB : %2.2f%%", memFree, memTotal, \
                ((float)memFree * 100)/memTotal);
        freeMem[sizeof(freeMem) - 1] = '\0';
        le_avdata_SetString(instRef, fieldName, freeMem);
    }
    else
    {
        LE_WARN("Failed to open meminfo");
        LE_WARN("%s", strerror(errno));
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Handle the request to update the flash usage
 *
 * @note
 *      This is only called for flash partition with file systems mounted at the time this app
 *      was started. Unmounted partitions are assumed to remain unmounted and to be immutable,
 *      plus the method of calculating free space is different.
 *
 * @return
 *      None.
 */
//--------------------------------------------------------------------------------------------------
static void HandleFreeFlash(
    le_avdata_AssetInstanceRef_t instRef,
    const char* fieldName,
    void* contextPtr
)
{
    struct statfs statfsStruct;
    char outStr[256];
    char* mountpoint = NULL;
    PartitionInfo_t* PartitionInfoPtr = (PartitionInfo_t*)contextPtr;

    mountpoint = PartitionInfoPtr->mountpoint;

    if (*mountpoint)
    {
        if ( statfs(mountpoint, &statfsStruct) == 0)
        {
            int freeSize = (int)statfsStruct.f_bfree * (int)statfsStruct.f_bsize;
            int totalSize = (int)statfsStruct.f_blocks * (int)statfsStruct.f_bsize;
            int usedSize = totalSize - freeSize;

            snprintf(outStr, sizeof(outStr), "Used %d of %d kB : %2.2f%%", usedSize / 1024, \
                    totalSize / 1024, 100.0 * usedSize / totalSize);
            outStr[sizeof(outStr) - 1 ] = '\0';

            le_avdata_SetString(instRef, fieldName, outStr);
        }
        else
        {
            LE_WARN("Trouble statting '%s': %s", mountpoint, strerror(errno));
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Handle the request to show current CPU load
 *
 * @note
 *      The load figures are read directly from /proc/loadavg
 *
 * @return
 *      None
 */
//--------------------------------------------------------------------------------------------------
static void HandleCpuLoad
(
    le_avdata_AssetInstanceRef_t instRef,
    const char* fieldName,
    void* contextPtr
)
{
    char cpuLoad[256];

    strncpy(cpuLoad, "Could not read cpu load", sizeof(cpuLoad) - 1);

    FILE* f = fopen("/proc/loadavg", "r");
    if (f != NULL)
    {
        int bytes = fread(cpuLoad, 1, 255, f);
        fclose(f);
        cpuLoad[bytes] = '\0';
        /* Just report the load values as per uptime */
        /* These are the first three space separated numbers */
        if (bytes > 3)
        {
            cpuLoad[IndexOfNthChar(cpuLoad, ' ', 3, sizeof(cpuLoad))] = '\0';
        }

    }
    else
    {
        LE_WARN("Failed to open loadavg");
        LE_WARN("%s", strerror(errno));
    }
    le_avdata_SetString(instRef, fieldName, cpuLoad);
}

//--------------------------------------------------------------------------------------------------
/**
 * Copies upto delimiter character or to max characters or end of string, whichever comes first.
 *
 * @note
 *     'endptr' is set to the source position after the delimiter or max characters or
 *     to the null character if the end of the string was reached.
 *     The resultant string is null terminated.
 *
 * @return
 *      the pointer to the copied string data
 */
//--------------------------------------------------------------------------------------------------
char* strncpyto(char* dest, const char* src, char delim, int max, char** endptr)
{
    char * originaldest = dest;
    int count = 0;
    /* skip spaces - acts like the atoi type funcs */
    while (*src && isspace(*src))
    {
        src++;
    }
    while ((*src != delim) && (*src != '\0') && (max > count))
    {
        *dest = *src;
        dest++;
        src++;
        count++;
    }
    *dest = '\0';
    if (*src == '\0')
    {
        *endptr = (char*)src;
    }
    else
    {
        *endptr = (char*)(src+1);
    }
    return originaldest;
}

//--------------------------------------------------------------------------------------------------
/**
 * Split the parts of a line from mtd and store into the FlashInfo structure
 *
 * @return
 *      None
 */
//--------------------------------------------------------------------------------------------------
static void ParseMtdLine(char* line)
{
    static int PartitionNumber = 0;
    /* Parse the pure gold we have been given into our magic pouch */
    if (strstr(line, "mtd") == line)
    {
        char* endptr = NULL;
        strncpyto(FlashInfo[PartitionNumber].partition, line, ':', sizeof(FlashInfo[PartitionNumber].partition), &endptr);
        FlashInfo[PartitionNumber].size = strtoul(endptr, &endptr, 16);
        FlashInfo[PartitionNumber].eraseBlockSize = strtoul(endptr, &endptr, 16);
        strncpyto(FlashInfo[PartitionNumber].name, endptr, ' ', sizeof(FlashInfo[PartitionNumber].name), &endptr);
        FlashInfo[PartitionNumber].nonBlankBlocks = 0; /* will calculate later if not mounted */

        /* name may have newline shars at the end so trim them off if found */
        endptr = strpbrk(FlashInfo[PartitionNumber].name, "\r\n");
        if (endptr)
        {
            *endptr = '\0';
        }

        PartitionNumber ++;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 *
 * read /proc/mtd and parse out the partitions, sizes, erase sizes and name.
 *
 * @return
 *      None
 */
//--------------------------------------------------------------------------------------------------
static void ReadMtdFile
(
    void
)
{
    char line[256];
    FILE* f = fopen("/proc/mtd", "r");
    if (f != NULL)
    {
        while (fgets(line, 255, f) != NULL)
        {
            ParseMtdLine(line);
        }
        /* we got a NULL pointer - either end of data or an error */
        /* Not sure whether it's useful returning an error. */
        /* If we can't read something we just can't read it so mention that
           and then carry on like a trooper */
        if (ferror(f) != 0){ LE_WARN("error while reading /proc/mtd"); }
        if (feof(f) != 0){ LE_DEBUG("finished reading /proc/mtd"); }
        fclose(f);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Use the avdata API to create assets for every found flash partition
 *
 * @return
 *      None
 */
//--------------------------------------------------------------------------------------------------
static void CreateFlashPartitionAssets
(
    void
)
{
    /* For any entry in the FlashInfo array that has a partition name */
    int i;
    for (i = 0; i < NUM_ARRAY_MEMBERS(FlashInfo); i++)
    {
        if( *(FlashInfo[i].partition) != '\0')
        {
            FlashInfo[i].partitionAssetRef = le_avdata_Create("Flash_Partition");
            if (FlashInfo[i].partitionAssetRef)
            {
                /* The following values will not change */
                le_avdata_SetString(FlashInfo[i].partitionAssetRef, "Partition", FlashInfo[i].partition);
                le_avdata_SetString(FlashInfo[i].partitionAssetRef, "Name", FlashInfo[i].name);
                le_avdata_SetInt(FlashInfo[i].partitionAssetRef, "Size", FlashInfo[i].size);
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Match partition name to block device name e.g. mtd0 -> /dev/mtdblock0
 *
 * @note
 *      This is a task that can be done with regular c string functions but it's convoluted. This
 *      implementation is probably easier to understand and therefore to prove correct. The
 *      return value will be true only if the basename of the block device string matches the
 *      partition name with the word "block" inserted.
 *
 * @return
 *      bool - true if the names are considered a match by the above criteria
 */
//--------------------------------------------------------------------------------------------------
static bool MatchDeviceToPartition(char* devName, char* partName)
{
    char* b="kcolb"; /* block (but reversed) */
    char* pend = partName + strlen(partName);
    char* dend = devName + strlen(devName);
    while(*pend == *dend)
    {
        pend--;
        dend--;
    }
    while(*b == *dend)
    {
        b++;
        dend--;
    }
    while((*pend == *dend) && (pend >= partName))
    {
        pend--;
        dend--;
    }
    return pend < partName;
}


//--------------------------------------------------------------------------------------------------
/**
 * Scan the flash info trying to match a partition name to a device name
 * and if found, note the mount point. Save only the first found mount point.
 * We don't need any more than that for statvfs()
 *
 * @note
 *      Side effect - updates FlashInfo
 *
 * @return
 *      None
 */
//--------------------------------------------------------------------------------------------------
static void IfMountedFlashSetUpHandler(struct mntent *mount_entry)
{
    /* For any entry in the FlashInfo array that has a partition name */
    int i;
    for (i = 0; i < NUM_ARRAY_MEMBERS(FlashInfo); i++)
    {
        if( (*(FlashInfo[i].partition) != '\0') && (*(FlashInfo[i].mountpoint) == '\0'))
        {
            if (MatchDeviceToPartition(mount_entry->mnt_fsname, FlashInfo[i].partition))
            {
                strncpy(FlashInfo[i].mountpoint, mount_entry->mnt_dir, sizeof(FlashInfo[i].mountpoint));
                FlashInfo[i].mountpoint[sizeof(FlashInfo[i].mountpoint) - 1] = '\0';
                strncpy(FlashInfo[i].filesystem, mount_entry->mnt_type, sizeof(FlashInfo[i].filesystem));
                FlashInfo[i].filesystem[sizeof(FlashInfo[i].filesystem) - 1] = '\0';
                le_avdata_SetString(FlashInfo[i].partitionAssetRef, "FileSystem", FlashInfo[i].filesystem);
                le_avdata_AddFieldEventHandler(FlashInfo[i].partitionAssetRef, "Used",
                                                HandleFreeFlash, &FlashInfo[i]);
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Scan the /etc/mtab to find file systems on flash partitions.
 *
 * @note
 *      Looks for occurances of block device drivers with names derived from flash partition names.
 *      If found, the device driver and mount point are noted. FlashInfo is updated.
 *
 * @return
 *      None
 */
//--------------------------------------------------------------------------------------------------
static void CheckForMountedFileSystems
(
    void
)
{
    FILE *mount_table;
	struct mntent *mount_entry;
    mount_table = setmntent("/etc/mtab", "r");
    if (!mount_table)
    {
        LE_WARN("Trouble reading mount table: %s",strerror(errno));
    }
    while (mount_table)
    {
        mount_entry = getmntent(mount_table);
        if (!mount_entry)
        {
            endmntent(mount_table);
            break;
        }
        IfMountedFlashSetUpHandler(mount_entry);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Find where blank flash begins in a given partition by reading the associated character
 * device stream supplied by the caller.
 *
 * @note
 *      Flash reads are slow and erase blocks are large but seeking is fast. We can get a quick
 *      estimate by skipping to the next erase block if we find a non-empty byte until we find
 *      a completely empty erase block. To save a bit more time at the risk of being completely
 *      wrong we can just check the first 8kB of an erase block for blankness. For practical
 *      cases this should be pretty safe.
 *
 * @return
 *      the number of the first erase block to be found blank. (since erase blocks are counted
 *      from 0, the number of the first blank block is effectively the same as the number of
 *      non-blank blocks.
 */
//--------------------------------------------------------------------------------------------------
static int FindFirstBlankBlock(FILE* flashCharacterStream, int eraseBlockSize, int blockTotal)
{
    const int blockCheckLimit = 8 * 1024; /* how many kB to check before we decide an erase block is  empty */
    int block = 0;
    int count = 0;

    uint8_t b;
    while (fread(&b, sizeof(b), 1, flashCharacterStream))
    {
        if (b == 0xff)
        {
            count ++;
            if (count >= blockCheckLimit)
            {
                /*This is the first blank block */
                break;
            }
        }
        else
        {
            block ++;
            if (block >= blockTotal)
            {
                break;
            }
            count = 0;
            fseek(flashCharacterStream, (block * eraseBlockSize),SEEK_SET);
        }
    }
    return block;
}

//--------------------------------------------------------------------------------------------------
/**
 * For flash partitions not mounted with a filesystem, assume data is BLOB starting at beginning
 * of flash then scan until you find empty flash (0xff for NAND)
 *
 * @note
 *      Side effect - results are recorded in FlashInfo
 *
 * @return
 *      None
 */
//--------------------------------------------------------------------------------------------------
static void EstimateRawFlashUsed
(
    void
)
{
    /* For any entry in the FlashInfo array that has a partition name */
    int i;
    char string[256];
    for (i = 0; i < NUM_ARRAY_MEMBERS(FlashInfo); i++)
    {
        /* Find flash devices that don't have a mountpoint */
        if ((*(FlashInfo[i].partition) != '\0') && (*(FlashInfo[i].mountpoint) == '\0'))
        {
            int eraseBlockSize = FlashInfo[i].eraseBlockSize;
            int blockTotal = FlashInfo[i].size / eraseBlockSize;
            char flashCharacterDevice[64];
            snprintf(flashCharacterDevice, sizeof(flashCharacterDevice), "/dev/%s", FlashInfo[i].partition);
            FILE* f = fopen(flashCharacterDevice, "r");
            if (f)
            {

                int blankBlock = FindFirstBlankBlock(f, eraseBlockSize, blockTotal);
                FlashInfo[i].nonBlankBlocks = blankBlock;
                snprintf(string, sizeof(string), "Used %d of %d kB : %2.2f%%", \
                    (blankBlock * FlashInfo[i].eraseBlockSize) / 1024, \
                    FlashInfo[i].size / 1024, \
                    ((float)(blankBlock * FlashInfo[i].eraseBlockSize) * 100) / FlashInfo[i].size);
                string[sizeof(string) - 1] = '\0';
                le_avdata_SetString(FlashInfo[i].partitionAssetRef, "Used", string);
                fclose(f);
            }
            else
            {
                LE_WARN("Trouble opening %s: %s", flashCharacterDevice, strerror(errno));
            }
        }
    }
}

#ifdef TEST
/* Test code - calling the various handlers */
static void CallEventHandlers
(
)
{
    int i;
    HandleCpuLoad(AssetRef, CPU_LOAD_FIELD, NULL);
    for (i = 0; i < NUM_ARRAY_MEMBERS(FlashInfo); i++)
    {
        /* Dump all of the flash bits */
        if (FlashInfo[i].name[0] != '\0')
        {
            HandleFreeFlash(FlashInfo[i].partitionAssetRef, "Used", &FlashInfo[i]);
        }
    }
    HandleFreeMemory(AssetRef, MEM_FREE_FIELD, NULL);
}

static int DiagOutput()
{
    /* Just diagnostic. To be removed later */
    int i;
    for (i = 0; i < NUM_ARRAY_MEMBERS(FlashInfo); i++)
    {
        if( *(FlashInfo[i].partition) != '\0' )
        {
            LE_INFO("slot %d:\n", i);
            LE_INFO("partition: '%s'\n", FlashInfo[i].partition);
            LE_INFO("name: '%s'\n", FlashInfo[i].name);
            LE_INFO("size: %u\n", FlashInfo[i].size);
            LE_INFO("eraseBlockSize: %u\n", FlashInfo[i].eraseBlockSize);
            LE_INFO("Non blank: %u\n", FlashInfo[i].nonBlankBlocks);
            LE_INFO("mpoint: %s\n", FlashInfo[i].mountpoint);
            LE_INFO("filesys: %s\n", FlashInfo[i].filesystem);
        }
        else
        {
            LE_INFO("slot %d is empty\n", i);
        }
    }

    return 1;
}
#endif

//--------------------------------------------------------------------------------------------------
/**
 *
 * Determines the number of assets to create and populates them with initial data.
 *
 * @note
 *      The initial data is data we'll assume does not change over time particularly partition
 *      sizes, names which partitions are unmounted and how much space is used in them. If these
 *      things are changed the app will have to be restarted to see the new values.
 *      For flash file systems, free space is calculated on demand.
 *
 * @return
 *      None
 */
//--------------------------------------------------------------------------------------------------
static void SetUpFlashPartitionAssets
(
    void
)
{
    /* using static FlashInfo[16] in these calls */
    /* We could store everything in assets but then we'd have to create them in ReadMtdFile
       and reading what we need from assets has IPC overhead */

    ReadMtdFile();
    CreateFlashPartitionAssets();
    /* Check in /etc/mtab for mounted mtds */
    /* Mounted ones, query statfs on demand */
    CheckForMountedFileSystems();
    /* Unmounted ones try to get an estimate by counting 0xffs */
    EstimateRawFlashUsed();
}

COMPONENT_INIT
{
    LE_INFO("======== Start ======== ");

    AssetRef = le_avdata_Create("System_Info");

    le_avdata_AddFieldEventHandler( AssetRef, CPU_LOAD_FIELD, HandleCpuLoad, NULL);
    le_avdata_AddFieldEventHandler( AssetRef, MEM_FREE_FIELD, HandleFreeMemory, NULL);

    SetUpFlashPartitionAssets();

    /* Test code */
#ifdef TEST
    CallEventHandlers();  	
    DiagOutput();
#endif
    LE_INFO("====== All set up and running =========");
}
