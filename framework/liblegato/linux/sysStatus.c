//--------------------------------------------------------------------------------------------------
/**
 * @file sysStatus.c
 *
 * Functions to do with manipulating the status of a system (and typically
 * the current system) used by updateDaemon and supervisor
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "limit.h"
#include "sysStatus.h"
#include "file.h"
#include "fileDescriptor.h"
#include "sysPaths.h"
#include "properties.h"

//--------------------------------------------------------------------------------------------------
/**
 * Absolute file system path to where the current running system's "status" file is.
 */
//--------------------------------------------------------------------------------------------------
static const char* CurrentStatusPath = CURRENT_SYSTEM_PATH "/status";


//--------------------------------------------------------------------------------------------------
/**
 * Get the status of the current system.
 *
 * @return The system status (SYS_BAD on error).
 */
//--------------------------------------------------------------------------------------------------
sysStatus_Status_t sysStatus_GetStatus
(
    const char* systemNamePtr
)
//--------------------------------------------------------------------------------------------------
{
    char path[PATH_MAX];
    LE_ASSERT(snprintf(path, sizeof(path), "%s/%s/status", SYSTEM_PATH, systemNamePtr)
              < sizeof(path));

    char statusBuffer[100] = "";

    if (file_Exists(path) == false)
    {
        LE_DEBUG("System status file '%s' does not exist, assuming untried system.", path);
        return SYS_PROBATION;
    }

    if (file_ReadStr(path, statusBuffer, sizeof(statusBuffer)) == -1)
    {
        LE_DEBUG("The system status file could not be read, '%s', assuming a bad system.", path);

        return SYS_BAD;
    }

    if (strcmp("good", statusBuffer) == 0)
    {
        return SYS_GOOD;
    }

    if (strcmp("bad", statusBuffer) == 0)
    {
        return SYS_BAD;
    }

    char* triedStr = "tried ";
    size_t triedSize = strlen(triedStr);

    if (strncmp(triedStr, statusBuffer, triedSize) == 0)
    {
        return SYS_PROBATION;
    }

    LE_ERROR("Unknown system status '%s' found in file '%s'.", statusBuffer, path);

    return SYS_BAD;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the status of the current system.
 *
 * @return the system status.
 */
//--------------------------------------------------------------------------------------------------
sysStatus_Status_t sysStatus_Status
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    sysStatus_Status_t status = sysStatus_GetStatus("current");

    if (status == SYS_BAD)
    {
        LE_FATAL("Currently running a 'bad' system!");
    }

    return status;
}


//--------------------------------------------------------------------------------------------------
/**
 * Remove the status file thus setting the try status to untried
 */
//--------------------------------------------------------------------------------------------------
void sysStatus_SetUntried
(
    void
)
{
    LE_FATAL_IF((unlink(CurrentStatusPath) == -1) && (errno != ENOENT),
                "Unable to delete '%s' (%m).",
                CurrentStatusPath);
}


//--------------------------------------------------------------------------------------------------
/**
 * If the current system status is in Probation, then this returns the number of times the system
 * has been tried while in probation.
 *
 * Do not call this if you are not in probation!
 *
 * @return A count of the number of times the system has been "tried."
 */
//--------------------------------------------------------------------------------------------------
int sysStatus_TryCount
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    char statusBuffer[100] = "";

    if (file_ReadStr(CurrentStatusPath, statusBuffer, sizeof(statusBuffer)) == -1)
    {
        LE_INFO("The system status file could not be found, '%s', assuming untried system.",
                CurrentStatusPath);
        return 0;
    }

    char* triedStr = "tried ";
    size_t triedSize = strlen(triedStr);

    if (strncmp(triedStr, statusBuffer, triedSize) == 0)
    {
        int tryCount;

        le_result_t result = le_utf8_ParseInt(&tryCount, &statusBuffer[triedSize]);

        if (result != LE_OK)
        {
            LE_FATAL("System try count '%s' is not a valid integer. (%s)",
                     &statusBuffer[triedSize],
                     LE_RESULT_TXT(result));
        }
        else
        {
            return tryCount;
        }
    }

    LE_FATAL("Current system not in probation, so try count is invalid.");
    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Increment the try count.
 */
//--------------------------------------------------------------------------------------------------
void sysStatus_IncrementTryCount
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    int tryCount = sysStatus_TryCount();
    char statusBuffer[100] = "";

    ++tryCount;

    int len = snprintf(statusBuffer, sizeof(statusBuffer), "tried %d", tryCount);

    LE_ASSERT(len < sizeof(statusBuffer));

    file_WriteStrAtomic(CurrentStatusPath, statusBuffer, 0644 /* rw-r--r-- */);
}

//--------------------------------------------------------------------------------------------------
/**
 * Decrement the try count.
 * If the system is still under probation the try count will be decremented, else
 * there is no effect. Will FATAL if the system has already been marked bad.
 */
//--------------------------------------------------------------------------------------------------
void sysStatus_DecrementTryCount
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    if (sysStatus_Status() == SYS_PROBATION)
    {
        int tryCount = sysStatus_TryCount();

        if (tryCount <= 1)
        {
            // we have no status file or we have tried just once - delete any status file
            // so we look like an untried system
            sysStatus_SetUntried();
        }
        else
        {
            char statusBuffer[100] = "";
            int len = snprintf(statusBuffer, sizeof(statusBuffer), "tried %d", tryCount - 1);

            LE_ASSERT(len < sizeof(statusBuffer));

            file_WriteStrAtomic(CurrentStatusPath, statusBuffer, 0644 /* rw-r--r-- */);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Returns the number of boot count
 *
 * @return The number of consecutive reboots by the current system."
 */
//--------------------------------------------------------------------------------------------------
int sysStatus_BootCount
(
    void
)
{
    char bootCountBuffer[100] = "";

    if (file_ReadStr(BOOT_COUNT_PATH, bootCountBuffer, sizeof(bootCountBuffer)) == -1)
    {
        LE_INFO("The boot count  could not be found, '%s'.", BOOT_COUNT_PATH);
        return 0;
    }

    int tryCount;

    // Max value for bootCount is 4
    bootCountBuffer[1] = '\0';
    le_result_t result = le_utf8_ParseInt(&tryCount, bootCountBuffer);

    if (result != LE_OK)
    {
        LE_FATAL("Boot count '%s' is not a valid integer. (%s)",
                 bootCountBuffer,
                 LE_RESULT_TXT(result));
    }
    else
    {
        return tryCount;
    }

    return 0;
}


//--------------------------------------------------------------------------------------------------
/**
 * Decrement the boot count.
 */
//--------------------------------------------------------------------------------------------------
void sysStatus_DecrementBootCount
(
    void
)
{
    int bootCount = sysStatus_BootCount();
    char bootBuffer[100] = "";

    --bootCount;

    int len = snprintf(bootBuffer, sizeof(bootBuffer), "%d %"PRIu64,
                       bootCount, (uint64_t)time(NULL));

    LE_ASSERT(len < sizeof(bootBuffer));

    file_WriteStrAtomic(BOOT_COUNT_PATH, bootBuffer, 0644 /* rw-r--r-- */);
}


//--------------------------------------------------------------------------------------------------
/**
 * Mark the system "bad".
 */
//--------------------------------------------------------------------------------------------------
void sysStatus_MarkBad
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    file_WriteStrAtomic(CurrentStatusPath, "bad", 0644 /* rw-r--r-- */);
}


//--------------------------------------------------------------------------------------------------
/**
 * Mark the system "tried 1".
 */
//--------------------------------------------------------------------------------------------------
void sysStatus_MarkTried
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    file_WriteStrAtomic(CurrentStatusPath, "tried 1", 0644 /* rw-r--r-- */);

    LE_INFO("Current system has been marked \"tried 1\".");
}


//--------------------------------------------------------------------------------------------------
/**
 * Mark the system "good".
 */
//--------------------------------------------------------------------------------------------------
void sysStatus_MarkGood
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    file_WriteStrAtomic(CurrentStatusPath, "good", 0644 /* rw-r--r-- */);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the status of the current system.
 *
 * @return true if the system is marked "good", false otherwise (e.g., if "tried 2").
 */
//--------------------------------------------------------------------------------------------------
bool sysStatus_IsGood
(
    void
)
{
    return sysStatus_Status() == SYS_GOOD;
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks whether legato system is Read-Only or not.
 *
 * @return
 *     true if the system is Read-Only
 *     false otherwise
 */
//--------------------------------------------------------------------------------------------------
bool sysStatus_IsReadOnly
(
    void
)
{
    return (0 == access(READ_ONLY_FLAG_PATH, R_OK));
}
