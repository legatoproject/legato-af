/**
 * @file sysResets.c
 *
 * Copyright (C) Sierra Wireless Inc.
 */
#if LE_CONFIG_LINUX
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#endif
#include <legato.h>
#include <interfaces.h>
#include "sysResets.h"

#if LE_CONFIG_LINUX
//--------------------------------------------------------------------------------------------------
/**
 * Lock file location
 *
 */
//--------------------------------------------------------------------------------------------------
#define LOCKFILE                "/var/lock/modemDeamon.lock"
#endif

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
 * Resets counter feature
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ResetCounterFeature = LE_UNSUPPORTED;

#if LE_CONFIG_LINUX
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
#endif

//--------------------------------------------------------------------------------------------------
/**
 * Read from file using Legato le_fs API
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_BAD_PARAMETER  A parameter is invalid.
 *  - LE_OVERFLOW       The file path is too long.
 *  - LE_NOT_FOUND      The file does not exists or a directory in the path does not exist
 *  - LE_NOT_PERMITTED  Access denied to the file or to a directory in the path
 *  - LE_UNSUPPORTED    The prefix cannot be added and the function is unusable
 *  - LE_FAULT          The function failed.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ReadFs
(
    const char* pathPtr,    ///< [IN] File path
    uint8_t*    bufPtr,     ///< [IN] Buffer to write
    size_t      size        ///< [IN] Number of bytes to read when this function is called
)
{
    le_fs_FileRef_t fileRef;
    le_result_t result;
    size_t expectedLen = size;

    result = le_fs_Open(pathPtr, LE_FS_RDONLY, &fileRef);
    if (LE_OK != result)
    {
        LE_ERROR("Failed to open %s: %s", pathPtr, LE_RESULT_TXT(result));
        return result;
    }

    result = le_fs_Read(fileRef, bufPtr, &size);
    if (LE_OK != result)
    {
        LE_ERROR("Failed to read %s: %s", pathPtr, LE_RESULT_TXT(result));
        if (LE_OK != le_fs_Close(fileRef))
        {
            LE_ERROR("Failed to close %s", pathPtr);
        }
        return result;
    }

    if (expectedLen != size)
    {
        LE_ERROR("Read %zu out of the expected length: %zu ", size, expectedLen);
        result = LE_FAULT;
    }

    result = le_fs_Close(fileRef);
    if (LE_OK != result)
    {
        LE_ERROR("Failed to close %s: %s", pathPtr, LE_RESULT_TXT(result));
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Write to file using Legato le_fs API
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_BAD_PARAMETER  A parameter is invalid.
 *  - LE_OVERFLOW       The file path is too long.
 *  - LE_UNDERFLOW      The write succeed but was not able to write all bytes
 *  - LE_NOT_FOUND      The file does not exists or a directory in the path does not exist
 *  - LE_NOT_PERMITTED  Access denied to the file or to a directory in the path
 *  - LE_UNSUPPORTED    The prefix cannot be added and the function is unusable
 *  - LE_FAULT          The function failed.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t WriteFs
(
    const char* pathPtr,    ///< [IN] File path
    uint8_t*    bufPtr,     ///< [IN] Buffer to write
    size_t      size        ///< [IN] Size Value
)
{
    le_fs_FileRef_t fileRef;
    le_result_t result;

    result = le_fs_Open(pathPtr, LE_FS_WRONLY | LE_FS_CREAT | LE_FS_TRUNC, &fileRef);
    if (LE_OK != result)
    {
        LE_ERROR("Failed to open %s: %s", pathPtr, LE_RESULT_TXT(result));
        return result;
    }

    result = le_fs_Write(fileRef, bufPtr, size);
    if (LE_OK != result)
    {
        LE_ERROR("Failed to write %s: %s", pathPtr, LE_RESULT_TXT(result));
        if (LE_OK != le_fs_Close(fileRef))
        {
            LE_ERROR("Failed to close %s", pathPtr);
        }
        return result;
    }

    result = le_fs_Close(fileRef);
    if (LE_OK != result)
    {
        LE_ERROR("Failed to close %s: %s", pathPtr, LE_RESULT_TXT(result));
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set system resets count
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_FAULT          The function failed.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SetResetsCount
(
    const char* filePath,    ///< [IN] Reset file path
    uint64_t    value        ///< [IN] Reset Value
)
{
    char buf[BUFSIZE] = {0};

    snprintf(buf, BUFSIZE, "%"PRIu64, value);

    if (LE_OK != WriteFs(filePath, (uint8_t *)buf, sizeof(buf)))
    {
        LE_ERROR("Failed to write to `%s'", filePath);
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get system resets count
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_BAD_PARAMETER  A parameter is invalid.
 *  - LE_FAULT          The function failed.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetResetsCount
(
    const char* filePath,    ///< [IN] Reset file path
    uint64_t*    valuePtr    ///< [OUT] Reset Value
)
{
    char buf[BUFSIZE] = {0};
    le_result_t result;

    if (NULL == valuePtr)
    {
        return LE_BAD_PARAMETER;
    }

    result = ReadFs(filePath, (uint8_t *)buf, sizeof(buf));

    if (LE_OK == result)
    {
        uint64_t value;
        errno = 0;

        value = strtoull(buf, NULL, BASE10);
        if (0 != errno)
        {
            LE_ERROR("Failed to convert buf value, %s", buf);
            return LE_FAULT;
        }

        *valuePtr = value;
         return LE_OK;
    }
    else if (LE_NOT_FOUND == result)
    {
        LE_DEBUG("File `%s' not found", filePath);

        *valuePtr = 0;
        return LE_OK;
    }
    else
    {
        LE_ERROR("Failed to read from `%s'", filePath);
        return LE_FAULT;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Update reset information
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_FAULT          The function failed.
 *  - LE_UNSUPPORTED    If not supported by the platform
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
    if (LE_OK != GetResetsCount(EXPECTED_RESETS, &expected))
    {
        return LE_FAULT;
    }
    if (LE_OK != GetResetsCount(UNEXPECTED_RESETS, &unexpected))
    {
        return LE_FAULT;
    }

    result = le_info_GetResetInformation(&reset, resetInfo, sizeof(resetInfo));
    if (LE_OK != result)
    {
        LE_ERROR("Failed to get reset info: %s", LE_RESULT_TXT(result));
        return result;
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

    if (LE_OK != SetResetsCount(EXPECTED_RESETS, expected))
    {
        return LE_FAULT;
    }

    if (LE_OK != SetResetsCount(UNEXPECTED_RESETS, unexpected))
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
 *  - LE_OK             The function succeeded.
    - LE_UNSUPPORTED    If not supported by the platform
 *  - LE_FAULT          The function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t sysResets_GetExpectedResetsCount
(
    uint64_t* expectedPtr
)
{
    if (NULL == expectedPtr)
    {
        return LE_BAD_PARAMETER;
    }
    if (LE_UNSUPPORTED == ResetCounterFeature)
    {
        LE_DEBUG("ResetCounterFeature LE_UNSUPPORTED");
        return LE_UNSUPPORTED;
    }

    return GetResetsCount(EXPECTED_RESETS, expectedPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the number of unexpected resets
 *
 * @return
 *  - LE_OK             The function succeeded.
    - LE_UNSUPPORTED    If not supported by the platform
 *  - LE_FAULT          The function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t sysResets_GetUnexpectedResetsCount
(
    uint64_t* unexpectedPtr
)
{
    if (NULL == unexpectedPtr)
    {
        return LE_BAD_PARAMETER;
    }
    if (LE_UNSUPPORTED == ResetCounterFeature)
    {
        LE_DEBUG("ResetCounterFeature LE_UNSUPPORTED");
        return LE_UNSUPPORTED;
    }

    return GetResetsCount(UNEXPECTED_RESETS, unexpectedPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Init system resets counter
 *
 * @return
 *      - LE_OK           Init succeded
 *      - LE_FAULT        Init failed
 *      - LE_UNSUPPORTED  If not supported by the platform
 */
//--------------------------------------------------------------------------------------------------
le_result_t sysResets_Init
(
    void
)
{
#if LE_CONFIG_LINUX
    //This check is done to avoid incrementation of counter if only legato reboots
    if (!access(LOCKFILE, W_OK | R_OK))
    {
        if (UpdateLockFile())
        {
            return LE_FAULT;
        }
        ResetCounterFeature = LE_OK;
        return ResetCounterFeature;
    }
    if (UpdateLockFile())
    {
        return LE_FAULT;
    }
#endif
    ResetCounterFeature = UpdateResetInfo();
    return ResetCounterFeature;

}
