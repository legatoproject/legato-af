/**
 * @file sysResets.c
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include <legato.h>
#include <interfaces.h>
#include "sysResets.h"

//--------------------------------------------------------------------------------------------------
/**
 * Lock file location
 *
 */
//--------------------------------------------------------------------------------------------------
#define LOCKFILE                "/var/lock/modemDeamon.lock"

//--------------------------------------------------------------------------------------------------
/**
 * Expected resets file location
 *
 */
//--------------------------------------------------------------------------------------------------
#define EXPECTED_RESETS       "/resets/expected"

//--------------------------------------------------------------------------------------------------
/**
 * Unexpected resets file location
 *
 */
//--------------------------------------------------------------------------------------------------
#define UNEXPECTED_RESETS     "/resets/unexpected"

//--------------------------------------------------------------------------------------------------
/**
 * Default buffer size
 *
 */
//--------------------------------------------------------------------------------------------------
#define BUFSIZE                 32

//--------------------------------------------------------------------------------------------------
/**
 * strtoll base 10
 *
 */
//--------------------------------------------------------------------------------------------------
#define BASE10                  10

//--------------------------------------------------------------------------------------------------
/**
 * Close a fd and log a warning message in case of an error
 *
 */
//--------------------------------------------------------------------------------------------------
static void CloseWarn
(
    int fd
)
{
    if (-1 == close(fd))
    {
        LE_ERROR("Failed to close fd %d: %m", fd);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Update lock file
 *
 */
//--------------------------------------------------------------------------------------------------
static int UpdateLockFile
(
    void
)
{
    int fd;
    int flags = O_CREAT | O_WRONLY;
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    char pid[BUFSIZE] = {0};

    fd = open(LOCKFILE, flags, mode);
    if (-1 == fd)
    {
        LE_ERROR("Failed to open `%s': %m", LOCKFILE);
        return -1;
    }

    snprintf(pid, BUFSIZE, "%u\n", getpid());

    if (-1 == write(fd, pid, strlen(pid)+1))
    {
        LE_ERROR("Failed to update `%s': %m", LOCKFILE);
        CloseWarn(fd);
        return -1;
    }

    CloseWarn(fd);

    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read from file using Legato le_fs API
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ReadFs
(
    const char* pathPtr,
    uint8_t*    bufPtr,
    size_t      size
)
{
    le_fs_FileRef_t fileRef;
    le_result_t result;

    result = le_fs_Open(pathPtr, LE_FS_RDONLY, &fileRef);
    if (LE_OK != result)
    {
        LE_ERROR("failed to open %s: %s", pathPtr, LE_RESULT_TXT(result));
        return result;
    }

    result = le_fs_Read(fileRef, bufPtr, &size);
    if (LE_OK != result)
    {
        LE_ERROR("failed to read %s: %s", pathPtr, LE_RESULT_TXT(result));
        if (LE_OK != le_fs_Close(fileRef))
        {
            LE_ERROR("failed to close %s", pathPtr);
        }
        return result;
    }

    result = le_fs_Close(fileRef);
    if (LE_OK != result)
    {
        LE_ERROR("failed to close %s: %s", pathPtr, LE_RESULT_TXT(result));
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Write to file using Legato le_fs API
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t WriteFs
(
    const char* pathPtr,
    uint8_t*    bufPtr,
    size_t      size
)
{
    le_fs_FileRef_t fileRef;
    le_result_t result;

    result = le_fs_Open(pathPtr, LE_FS_WRONLY | LE_FS_CREAT, &fileRef);
    if (LE_OK != result)
    {
        LE_ERROR("failed to open %s: %s", pathPtr, LE_RESULT_TXT(result));
        return result;
    }

    result = le_fs_Write(fileRef, bufPtr, size);
    if (LE_OK != result)
    {
        LE_ERROR("failed to write %s: %s", pathPtr, LE_RESULT_TXT(result));
        if (LE_OK != le_fs_Close(fileRef))
        {
            LE_ERROR("failed to close %s", pathPtr);
        }
        return result;
    }

    result = le_fs_Close(fileRef);
    if (LE_OK != result)
    {
        LE_ERROR("failed to close %s: %s", pathPtr, LE_RESULT_TXT(result));
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set system resets count
 *
 */
//--------------------------------------------------------------------------------------------------
static int SetResetsCount
(
    const char* filePath,
    uint64_t    value
)
{
    char buf[BUFSIZE] = {0};

    snprintf(buf, BUFSIZE, "%lu\n", (long unsigned)value);

    if (LE_OK != WriteFs(filePath, (uint8_t *)buf, sizeof(buf)))
    {
        LE_ERROR("Failed to write to `%s'", filePath);
        return -1;
    }

    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get system resets count
 *
 */
//--------------------------------------------------------------------------------------------------
static int64_t GetResetsCount
(
    const char* filePath
)
{
    char buf[BUFSIZE] = {0};
    le_result_t result;
    int64_t ret;

    result = ReadFs(filePath, (uint8_t *)buf, sizeof(buf));

    switch (result)
    {
        case LE_OK:
            ret = (int64_t)strtoll(buf, NULL, BASE10);
            break;
        case LE_NOT_FOUND:
            LE_DEBUG("File `%s' not found", filePath);
            ret = 0;
            break;
        default:
            LE_ERROR("Failed to read from `%s'", filePath);
            ret = -1;
            break;
    }

    return ret;
}

//--------------------------------------------------------------------------------------------------
/**
 * Update reset information
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t UpdateResetInfo
(
    void
)
{
    le_result_t result;
    le_info_Reset_t reset;
    char resetInfo[LE_INFO_MAX_RESET_BYTES] = {0};
    uint64_t expected, unexpected;

    expected = GetResetsCount(EXPECTED_RESETS);
    unexpected = GetResetsCount(UNEXPECTED_RESETS);

    if ( (-1 == expected) || (-1 == unexpected) )
    {
        return LE_FAULT;
    }

    result = le_info_GetResetInformation(&reset, resetInfo, sizeof(resetInfo));
    if (LE_OK != result)
    {
        LE_ERROR("Failed to get reset info: %s", LE_RESULT_TXT(result));
        return LE_FAULT;
    }

    switch (reset)
    {
        case LE_INFO_RESET_USER:
        case LE_INFO_RESET_HARD:
        case LE_INFO_RESET_UPDATE:
        case LE_INFO_POWER_DOWN:
            expected++;
            break;
        case LE_INFO_RESET_UNKNOWN:
        case LE_INFO_RESET_CRASH:
        case LE_INFO_TEMP_CRIT:
        case LE_INFO_VOLT_CRIT:
            unexpected++;
            break;
        default:
            break;
    }

    if(SetResetsCount(EXPECTED_RESETS, expected))
    {
        return LE_FAULT;
    }

    if(SetResetsCount(UNEXPECTED_RESETS, unexpected))
    {
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the number of expected resets
 *
 * @return
 *      - Number of expected resets
 */
//--------------------------------------------------------------------------------------------------
int64_t sysResets_GetExpectedResetsCount
(
    void
)
{
    return GetResetsCount(EXPECTED_RESETS);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the number of unexpected resets
 *
 * @return
 *      - Number of unexpected resets
 */
//--------------------------------------------------------------------------------------------------
int64_t sysResets_GetUnexpectedResetsCount
(
    void
)
{
    return GetResetsCount(UNEXPECTED_RESETS);
}

//--------------------------------------------------------------------------------------------------
/**
 * Init system resets counter
 *
 * @return
 *      - LE_OK     Init succeded
 *      - LE_FAULT  Init failed
 */
//--------------------------------------------------------------------------------------------------
le_result_t sysResets_Init
(
    void
)
{
    if (!access(LOCKFILE, W_OK | R_OK))
    {
        if (UpdateLockFile())
        {
            return LE_FAULT;
        }
        return LE_OK;
    }

    if (UpdateLockFile())
    {
        return LE_FAULT;
    }

    return UpdateResetInfo();
}
