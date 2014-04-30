//--------------------------------------------------------------------------------------------------
/** @file cgroups.c
 *
 * API for creating and managing cgroups.
 *
 * Copyright (C) Sierra Wireless, Inc. 2014. All rights reserved.
 */

#include "legato.h"
#include "cgroups.h"
#include "limit.h"
#include "fileDescriptor.h"


//--------------------------------------------------------------------------------------------------
/**
 * Cgroup sub-system names.
 */
//--------------------------------------------------------------------------------------------------
static const char* SubSysName[CGRP_NUM_SUBSYSTEMS] = {"cpu", "memory"};


//--------------------------------------------------------------------------------------------------
/**
 * Root path for all cgroups.
 */
//--------------------------------------------------------------------------------------------------
#define ROOT_PATH           "/sys/fs/cgroup"
#define ROOT_NAME           "cgroupsRoot"


//--------------------------------------------------------------------------------------------------
/**
 * Checks if a file system is mounted at the specified location.
 */
//--------------------------------------------------------------------------------------------------
static bool IsMounted
(
    const char* fileSysName,        ///< [IN] Name of the mounted filesystem.
    const char* path                ///< [IN] Path of the mounted filesystem.
)
{
    // Open /proc/mounts file to check where all the mounts are.  This sets the entry to the top
    // of the file.
    FILE* mntFilePtr = setmntent("/proc/mounts", "r");
    LE_FATAL_IF(mntFilePtr == NULL, "Could not read '/proc/mounts'.");

    char buf[LIMIT_MAX_MNT_ENTRY_BYTES];
    struct mntent mntEntry;
    bool result = false;

    while (getmntent_r(mntFilePtr, &mntEntry, buf, LIMIT_MAX_MNT_ENTRY_BYTES) != NULL)
    {
        if ( (strcmp(fileSysName, mntEntry.mnt_fsname) == 0) &&
             (strcmp(path, mntEntry.mnt_dir) == 0) )
        {
            result = true;
            break;
        }
    }

    // Close the file descriptor.
    endmntent(mntFilePtr);

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Initializes cgroups for the system.  Sets up a hierarchy for each supported subsystem.
 *
 * @note Should be called once for the entire system, subsequent calls to this function will have no
 *       effect.  Must be called before any of the other functions in this API is called.
 *
 * @note Failures will cause the calling process to exit.
 */
//--------------------------------------------------------------------------------------------------
void cgrp_Init
(
    void
)
{
    // Setup the cgroup root directory if it does not already exist.
    if (!IsMounted(ROOT_NAME, ROOT_PATH))
    {
        LE_FATAL_IF(mount(ROOT_NAME, ROOT_PATH, "tmpfs", 0, NULL) != 0,
                    "Could not mount cgroup root file system.  %m.");
    }

    // Setup a separate cgroup hierarch for each supported subsystem.
    cgrp_SubSys_t subSys = 0;
    for (; subSys < CGRP_NUM_SUBSYSTEMS; subSys++)
    {
        char dir[LIMIT_MAX_PATH_BYTES] = ROOT_PATH;

        LE_ASSERT(le_path_Concat("/", dir, sizeof(dir), SubSysName[subSys], (char*)NULL) == LE_OK);

        LE_ASSERT(le_dir_Make(dir, S_IRWXU) != LE_FAULT);

        if (!IsMounted(SubSysName[subSys], dir))
        {
            LE_FATAL_IF(mount(SubSysName[subSys], dir, "cgroup", 0, SubSysName[subSys]) != 0,
                        "Could not mount cgroup subsystem '%s'.  %m.", SubSysName[subSys]);

            LE_INFO("Mounted cgroup hiearchy for subsystem '%s'.", SubSysName[subSys]);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Writes a string to a file.  Overwrites what is currently in the file.
 *
 * @note  Certain file types cannot accept certain types of data, and the write may fail with a
 *        specific errno value.  If the write fails with errno ESRCH this function will return
 *        LE_OUT_OF_RANGE.
 *
 * @return
 *      LE_OK if successful.
 *      LE_OUT_OF_RANGE if an attempt was made to write a value that the file cannot accept.
 *      LE_FAULT if there was some other error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t WriteToFile
(
    const char* filename,           ///< The file to write to.
    const char* string              ///< The string to write into the file.
)
{
    // Get the length of the string.
    size_t len = strlen(string);
    LE_ASSERT(len > 0);

    // Open the file.
    int fd;

    do
    {
        fd = open(filename, O_WRONLY);
    }
    while ((fd < 0) && (errno == EINTR));

    if (fd < 0)
    {
        LE_ERROR("Could not open file '%s'.  %m.", filename);
        return LE_FAULT;
    }

    // Write the string to the file.
    le_result_t result = LE_FAULT;
    ssize_t numBytesWritten = 0;

    do
    {
        numBytesWritten = write(fd, string, len);
    }
    while ((numBytesWritten == -1) && (errno == EINTR));

    if (numBytesWritten != len)
    {
        LE_ERROR("Could not write '%s' to file '%s'.  %m.", string, filename);

        if (errno == ESRCH)
        {
            result = LE_OUT_OF_RANGE;
        }
    }
    else
    {
        result = LE_OK;
    }

    fd_Close(fd);

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates a cgroup with the specified name in the specified sub-system.  If the cgroup already
 * exists this function has no effect.
 *
 * Sub-groups can be created by providing a path as the name.  For example,
 * cgrp_Create(CGRP_SUBSYS_CPU, "Students/Undergrads"); will create a cgroup called "Undergrads"
 * that is a sub-group of "Students".  Note that all parent groups must first exist before a
 * sub-group can be created.
 *
 * @note Failures will cause the calling process to exit.
 */
//--------------------------------------------------------------------------------------------------
void cgrp_Create
(
    cgrp_SubSys_t subsystem,        ///< Sub-system the cgroup belongs to.
    const char* cgroupNamePtr       ///< Name of the cgroup to create.
)
{
    // Create the path to the cgroup.
    char path[LIMIT_MAX_PATH_BYTES] = ROOT_PATH;
    LE_ASSERT(le_path_Concat("/", path, sizeof(path), SubSysName[subsystem], cgroupNamePtr,
                             (char*)NULL) == LE_OK);

    // Create the cgroup.
    LE_ASSERT(le_dir_Make(path, S_IRWXU) != LE_FAULT);
}


//--------------------------------------------------------------------------------------------------
/**
 * Adds a process to a cgroup.
 *
 * @return
 *      LE_OK if successful.
 *      LE_OUT_OF_RANGE if the process doesn't exist.
 *      LE_FAULT if there was some other error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t cgrp_AddProc
(
    cgrp_SubSys_t subsystem,        ///< Sub-system of the cgroup.
    const char* cgroupNamePtr,      ///< Name of the cgroup to add the process to.
    pid_t pidToAdd                  ///< PID of the process to add.
)
{
    // Construct the path to the 'tasks' file for this cgroup.
    char filename[LIMIT_MAX_PATH_BYTES] = ROOT_PATH;

    LE_ASSERT(le_path_Concat("/", filename, sizeof(filename), SubSysName[subsystem], cgroupNamePtr,
                             "tasks", (char*)NULL) == LE_OK);

    // Convert the pid to a string.
    char pidStr[100];

    LE_ASSERT(snprintf(pidStr, sizeof(pidStr), "%d", pidToAdd) < sizeof(pidStr));

    // Write the pid to the file.
    return WriteToFile(filename, pidStr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes a cgroup.
 *
 * @note A cgroup can only be removed when there are no processes in the group.  Ensure there are no
 *       processes in a cgroup (by killing the processes) before attempting to delete it.
 *
 * @return
 *      LE_OK if the cgroup was successfully deleted.
 *      LE_BUSY if the cgroup could not be deleted because there are still processes in the cgroup.
 *      LE_FAULT if there was some other error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t cgrp_Delete
(
    cgrp_SubSys_t subsystem,        ///< Sub-system of the cgroup.
    const char* cgroupNamePtr       ///< Name of the cgroup to delete.
)
{
    // Create the path to the cgroup.
    char path[LIMIT_MAX_PATH_BYTES] = ROOT_PATH;
    LE_ASSERT(le_path_Concat("/", path, sizeof(path), SubSysName[subsystem], cgroupNamePtr,
                             (char*)NULL) == LE_OK);

    // Attempt to remove the cgroup directory.
    if (rmdir(path) != 0)
    {
        if (errno == EBUSY)
        {
            LE_ERROR("Could not remove cgroup '%s'.  Tasks (process) list may not be empty.  %m.",
                     path);
            return LE_BUSY;
        }
        else
        {
            LE_ERROR("Could not remove cgroup '%s'.  %m.", path);
            return LE_FAULT;
        }
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the name of sub-system.
 *
 * @note Do not attempt to modify the returned name in place.  If you need to make modifications
 *       copy the name into your own buffer.
 *
 * @return
 *      The name of the sub-system.
 */
//--------------------------------------------------------------------------------------------------
const char* cgrp_SubSysName
(
    cgrp_SubSys_t subsystem         ///< Sub-system.
)
{
    return SubSysName[subsystem];
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the cpu share of a cgroup.
 *
 * Cpu share is used to calculate the cpu percentage for a process relative to all other processes
 * in the system.  Newly created cgroups and processes not belonging to a cgroup are given a default
 * value of 1024.  The actual percentage of the cpu given to a process is calculated as:
 *
 * (share value of process) / (sum of shares from all processes contending for the cpu)
 *
 * All processes within a cgroup share the available cpu share for that cgroup.
 *
 * For example:
 *
 * cgroupA is configured with the default share value, 1024.
 * cgroupB is configured with 512 as its share value.
 * cgroupC is configured with 2048 as its share value.
 *
 * cgroupA has one process running.
 * cgroupB has two processes running.
 * cgroupC has one process running.
 *
 * Assuming that all processes in cgroupA, cgroupB and cgroupC are running and not blocked waiting
 * for some I/O or timer event and that another system process is also running.
 *
 * Sum of all shares (including the one system process) is 1024 + 512 + 2048 + 1024 = 4608
 *
 * The process in cgroupA will get 1024/4608 = 22% of the cpu.
 * The two processes in cgroupB will share 512/4608 = 11% of the cpu, each process getting 5.5%.
 * The process in cgroupC will get 2048/4608 = 44% of the cpu.
 * The system process will get 1024/4608 = 22% of the cpu.
 *
 * @note Failures will cause the calling process to exit.
 */
//--------------------------------------------------------------------------------------------------
void cgrp_cpu_SetShare
(
    const char* cgroupNamePtr,      ///< Name of the cgroup to set the share for.
    size_t share                    ///< Share value to set.  See the function header for more
                                    ///  details.
)
{
    // Create the path to the 'cpu.shares' file for this cgroup.
    char filename[LIMIT_MAX_PATH_BYTES] = ROOT_PATH;

    LE_ASSERT(le_path_Concat("/", filename, sizeof(filename), SubSysName[CGRP_SUBSYS_CPU],
                             cgroupNamePtr, "cpu.shares", (char*)NULL) == LE_OK);

    // Convert the value to a string.
    char shareStr[100];
    LE_ASSERT(snprintf(shareStr, sizeof(shareStr), "%zd", share) < sizeof(shareStr));

    // Write the share value to the file.
    LE_ASSERT(WriteToFile(filename, shareStr) == LE_OK);
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the memory limit for a cgroup.
 *
 * @note All processes in a cgroup share the available memory for that cgroup.
 *
 * @note Failures will cause the calling process to exit.
 */
//--------------------------------------------------------------------------------------------------
void cgrp_mem_SetLimit
(
    const char* cgroupNamePtr,      ///< Name of the cgroup to set the limit for.
    size_t limit                    ///< Memory limit in kilobytes.
)
{
    // Construct the path to the 'memory.limit_in_bytes' file for this cgroup.
    char filename[LIMIT_MAX_PATH_BYTES] = ROOT_PATH;

    LE_ASSERT(le_path_Concat("/", filename, sizeof(filename), SubSysName[CGRP_SUBSYS_MEM],
                             cgroupNamePtr, "memory.limit_in_bytes", (char*)NULL) == LE_OK);

    // Convert the limit to a string.
    char limitStr[100];

    LE_ASSERT(snprintf(limitStr, sizeof(limitStr), "%zdk", limit) < sizeof(limitStr));

    // Write the limit to the file.
    LE_ASSERT(WriteToFile(filename, limitStr) == LE_OK);
}





