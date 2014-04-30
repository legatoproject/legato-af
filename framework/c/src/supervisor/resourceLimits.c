//--------------------------------------------------------------------------------------------------
/** @file resourceLimits.c
 *
 * Currently we use Linux's rlimits to set resource limits.
 *
 * @todo Switch to using cgroups for more control.  rlimits are a little awkward to use as they are
 *       really only apply to the processes, not to an application.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#include "legato.h"
#include "resourceLimits.h"
#include "le_cfg_interface.h"
#include "limit.h"
#include "user.h"
#include "cgroups.h"


//--------------------------------------------------------------------------------------------------
/**
 * The name of the node in the config tree that contains an application's limit on the application's
 * file system size (in bytes).
 *
 * If this entry in the config tree is missing or is empty, then DEFAULT_LIMIT_FILE_SYSTEM_SIZE
 * will be used.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_LIMIT_FILE_SYSTEM_SIZE                 "fileSystemSizeLimit"


//--------------------------------------------------------------------------------------------------
/**
 * The name of the node in the config tree that contains a process's user's POSIX message queue size
 * limit.
 *
 * If this entry in the config tree is missing or is empty, then
 * DEFAULT_LIMIT_TOTAL_POSIX_MSG_QUEUE_SIZE will be used.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_LIMIT_TOTAL_POSIX_MSG_QUEUE_SIZE       "totalPosixMsgQueueSizeLimit"


//--------------------------------------------------------------------------------------------------
/**
 * The name of the node in the config tree that contains a process's user's limit on the maximum
 * number of processes.
 *
 * If this entry in the config tree is missing or is empty, then DEFAULT_LIMIT_NUM_PROCESSES
 * will be used.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_LIMIT_NUM_PROCESSES                    "numProcessesLimit"


//--------------------------------------------------------------------------------------------------
/**
 * The name of the node in the config tree that contains a process's user's limit on the maximum
 * number of realtime signals that can be queued.
 *
 * If this entry in the config tree is missing or is empty, then DEFAULT_LIMIT_RT_SIGNAL_QUEUE_SIZE
 * will be used.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_LIMIT_RT_SIGNAL_QUEUE_SIZE             "rtSignalQueueSizeLimit"


//--------------------------------------------------------------------------------------------------
/**
 * The name of the node in the config tree that contains an application's memory limit.
 *
 * If this entry in the config tree is missing or is empty, DEFAULT_LIMIT_MEMORY is used.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_LIMIT_MEMORY                           "memLimit"


//--------------------------------------------------------------------------------------------------
/**
 * The name of the node in the config tree that contains an application's cpu share.
 *
 * If this entry in the config tree is missing or is empty, DEFAULT_LIMIT_CPU_SHARE is used.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_LIMIT_CPU_SHARE                        "cpuShare"


//--------------------------------------------------------------------------------------------------
/**
 * The name of the node in the config tree that contains a process's core dump file size limit.
 *
 * If this entry in the config tree is missing or is empty, DEFAULT_LIMIT_CORE_DUMP_FILE_SIZE is
 * used.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_LIMIT_CORE_DUMP_FILE_SIZE              "coreDumpFileSizeLimit"


//--------------------------------------------------------------------------------------------------
/**
 * The name of the node in the config tree that contains a process's limit on the size of files that
 * it can create/expand.
 *
 * If this entry in the config tree is missing or is empty, DEFAULT_LIMIT_MAX_FILE_SIZE is used.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_LIMIT_MAX_FILE_SIZE                    "maxFileSizeLimit"


//--------------------------------------------------------------------------------------------------
/**
 * The name of the node in the config tree that contains a process's limit on the number of bytes of
 * memory that may be locked into RAM.
 *
 * In effect this limit is rounded down to the nearest multiple of the system page size.
 *
 * If this entry in the config tree is missing or is empty, DEFAULT_LIMIT_MEM_LOCK_SIZE is used.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_LIMIT_MEM_LOCK_SIZE                    "memLockSizeLimit"


//--------------------------------------------------------------------------------------------------
/**
 * The name of the node in the config tree that contains a process's limit on the number of file
 * descriptors that the process can have open.
 *
 * The configured value must be less than MAX_LIMIT_NUM_FD.
 *
 * If this entry in the config tree is missing or is empty, DEFAULT_LIMIT_NUM_FD is used.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_LIMIT_NUM_FD                           "numFileDescriptorsLimit"


//--------------------------------------------------------------------------------------------------
/**
 * Resource limit defaults.
 */
//--------------------------------------------------------------------------------------------------
#define DEFAULT_LIMIT_FILE_SYSTEM_SIZE                  131072
#define DEFAULT_LIMIT_TOTAL_POSIX_MSG_QUEUE_SIZE        512
#define DEFAULT_LIMIT_NUM_PROCESSES                     10
#define DEFAULT_LIMIT_RT_SIGNAL_QUEUE_SIZE              100
#define DEFAULT_LIMIT_MEMORY                            40960
#define DEFAULT_LIMIT_CPU_SHARE                         1024
#define DEFAULT_LIMIT_CORE_DUMP_FILE_SIZE               8192
#define DEFAULT_LIMIT_MAX_FILE_SIZE                     90112
#define DEFAULT_LIMIT_MEM_LOCK_SIZE                     8192
#define DEFAULT_LIMIT_NUM_FD                            256


//--------------------------------------------------------------------------------------------------
/**
 * Maximum value that resource limits can be set to.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_LIMIT_NUM_FD                                1024


//--------------------------------------------------------------------------------------------------
/**
 * Gets the resource limit value from the config tree.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetCfgResourceLimit
(
    le_cfg_IteratorRef_t limitCfg,  // The iterator pointing to the configured limit to read.  This
                                    // iterator is a owned by the caller and should not be deleted
                                    // in this function.
    rlim_t* limitValuePtr           // The value read from the config tree.
)
{
    if (le_cfg_IsEmpty(limitCfg, ""))
    {
        return LE_FAULT;
    }

    int limitValue = le_cfg_GetInt(limitCfg, "", 0);

    if (limitValue < 0)
    {
        // Negative values are not allowed.
        return LE_FAULT;
    }

    *limitValuePtr = (rlim_t)limitValue;
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the sandboxed application's tmpfs file system limit.
 *
 * @return
 *      The file system limit for the specified application.
 */
//--------------------------------------------------------------------------------------------------
rlim_t resLim_GetSandboxedAppTmpfsLimit
(
    app_Ref_t appRef                ///< [IN] The application to set resource limits for.
)
{
    // Get the resource limit from the config tree.  Zero means unlimited for tmpfs mounts and is
    // not allowed.

    // Determine the file system limit to set.
    rlim_t fileSysLimit = DEFAULT_LIMIT_FILE_SYSTEM_SIZE;

    // Create a config iterator to get the file system limit from the config tree.
    le_cfg_IteratorRef_t appCfg = le_cfg_CreateReadTxn(app_GetConfigPath(appRef));
    le_cfg_GoToNode(appCfg, CFG_NODE_LIMIT_FILE_SYSTEM_SIZE);

    if (le_cfg_NodeExists(appCfg, "") == false)
    {
        LE_WARN("No resource limit %s.  Assuming the default value %d.",
                CFG_NODE_LIMIT_FILE_SYSTEM_SIZE,
                DEFAULT_LIMIT_FILE_SYSTEM_SIZE);
    }
    else if ( (GetCfgResourceLimit(appCfg, &fileSysLimit) != LE_OK) ||
              (fileSysLimit == 0) )
    {
        // Use the default limit.
        LE_ERROR("Configured resource limit %s is invalid.  Assuming the default value %d.",
                 CFG_NODE_LIMIT_FILE_SYSTEM_SIZE,
                 DEFAULT_LIMIT_FILE_SYSTEM_SIZE);

        fileSysLimit = DEFAULT_LIMIT_FILE_SYSTEM_SIZE;
    }

    le_cfg_CancelTxn(appCfg);

    return fileSysLimit;
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the specified Linux resource limit (rlimit) for the application/process.
 */
//--------------------------------------------------------------------------------------------------
static void SetRLimit
(
    pid_t pid,                      // The pid of the process to set the limit for.
    le_cfg_IteratorRef_t procCfg,   // The iterator for the process.  This is a iterator is owned by
                                    // the caller and should not be deleted in this function.
    const char* resourceName,       // The resource name in the config tree.
    int resourceID,                 // The resource ID that setrlimit() expects.
    rlim_t defaultValue             // The default value for this resource limit.
)
{
    // Initialize the limit to the defaults.
    struct rlimit lim = {defaultValue, defaultValue};

    // Move the iterator to the resource limit to read.
    le_cfg_GoToNode(procCfg, resourceName);
    if (le_cfg_NodeExists(procCfg, "") == false)
    {
        LE_WARN("No resource limit %s.  Using the default value %lu.", resourceName, defaultValue);
    }
    else
    {
        // Try reading the resource limit from the config tree.
        if (GetCfgResourceLimit(procCfg, &(lim.rlim_max)) != LE_OK)
        {
            LE_ERROR("Configured resource limit %s is invalid.  Using the default value %lu.",
                     resourceName, defaultValue);
        }
        else
        {
            // Check that the limit does not exceed the maximum.
            if ( (resourceID == RLIMIT_NOFILE) && (lim.rlim_max > MAX_LIMIT_NUM_FD) )
            {
                LE_ERROR("Resource limit %s is greater than the maximum allowed limit (%d).  Using the \
    maximum allowed value.", resourceName, MAX_LIMIT_NUM_FD);

                lim.rlim_max = MAX_LIMIT_NUM_FD;
            }
        }

        // Move the iterator back to where it was.
        LE_ASSERT(le_cfg_GoToParent(procCfg) == LE_OK);
    }

    // Hard and soft limits are the same.
    lim.rlim_cur = lim.rlim_max;

    // @todo: Should report the resource name as something more readable than the node name in the
    //        config tree.

    LE_INFO("Setting resource limit %s to value %d.", resourceName, (int)lim.rlim_max);
    LE_ERROR_IF(prlimit(pid, resourceID, &lim, NULL) == -1,
                "Could not set resource limit %s (%d).  %m.", resourceName, resourceID);
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the resource limits for the specified application.
 */
//--------------------------------------------------------------------------------------------------
void resLim_SetAppLimits
(
    app_Ref_t appRef                ///< [IN] The application to set resource limits for.
)
{
    // Get the application's user name for use with cgroups.
    char userName[LIMIT_MAX_USER_NAME_BYTES];
    LE_ASSERT(user_ConvertToUserName(app_GetName(appRef), userName, sizeof(userName)) == LE_OK);

    // Create cgroups for this application in each of the cgroup subsystems.
    cgrp_SubSys_t subSys = 0;
    for (; subSys < CGRP_NUM_SUBSYSTEMS; subSys++)
    {
        cgrp_Create(subSys, userName);
    }

    // Create a config iterator for this app.
    le_cfg_IteratorRef_t appCfg = le_cfg_CreateReadTxn(app_GetConfigPath(appRef));

    // Set the cpu limit.
    int cpuShare = le_cfg_GetInt(appCfg, CFG_NODE_LIMIT_CPU_SHARE, DEFAULT_LIMIT_CPU_SHARE);

    if (le_cfg_NodeExists(appCfg, CFG_NODE_LIMIT_CPU_SHARE) == false)
    {
        LE_WARN("No cpu share limit.  Using the default value %d.", DEFAULT_LIMIT_CPU_SHARE);
    }

    cgrp_cpu_SetShare(userName, cpuShare);

    // Set the memory limit.
    size_t memLimit = le_cfg_GetInt(appCfg, CFG_NODE_LIMIT_MEMORY, DEFAULT_LIMIT_MEMORY);

    if (le_cfg_NodeExists(appCfg, CFG_NODE_LIMIT_MEMORY) == false)
    {
        LE_WARN("No memory limit.  Using the default value %dK.", DEFAULT_LIMIT_MEMORY);
    }

    cgrp_mem_SetLimit(userName, memLimit);

    le_cfg_CancelTxn(appCfg);
}



//--------------------------------------------------------------------------------------------------
/**
 * Sets the resource limits for the specified process.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t resLim_SetProcLimits
(
    proc_Ref_t procRef              ///< [IN] The process to set resource limits for.
)
{
    pid_t pid = proc_GetPID(procRef);

    // Create an iterator for this process.
    le_cfg_IteratorRef_t procCfg = le_cfg_CreateReadTxn(proc_GetConfigPath(procRef));

    // Set the process resource limits.
    SetRLimit(pid, procCfg, CFG_NODE_LIMIT_CORE_DUMP_FILE_SIZE, RLIMIT_CORE,
              DEFAULT_LIMIT_CORE_DUMP_FILE_SIZE);

    SetRLimit(pid, procCfg, CFG_NODE_LIMIT_MAX_FILE_SIZE, RLIMIT_FSIZE,
              DEFAULT_LIMIT_MAX_FILE_SIZE);

    SetRLimit(pid, procCfg, CFG_NODE_LIMIT_MEM_LOCK_SIZE, RLIMIT_MEMLOCK,
              DEFAULT_LIMIT_MEM_LOCK_SIZE);

    SetRLimit(pid, procCfg, CFG_NODE_LIMIT_NUM_FD, RLIMIT_NOFILE,
              DEFAULT_LIMIT_NUM_FD);

    // Set the application limits.
    //
    // @note Even though these are application limits they still need to be set for the process
    //       because Linux rlimits are applied to individual processes.

    // Goto the application config path from the process config path.
    le_cfg_GoToParent(procCfg);
    le_cfg_GoToParent(procCfg);

    SetRLimit(pid, procCfg, CFG_NODE_LIMIT_TOTAL_POSIX_MSG_QUEUE_SIZE, RLIMIT_MSGQUEUE,
              DEFAULT_LIMIT_TOTAL_POSIX_MSG_QUEUE_SIZE);

    SetRLimit(pid, procCfg, CFG_NODE_LIMIT_NUM_PROCESSES, RLIMIT_NPROC,
              DEFAULT_LIMIT_NUM_PROCESSES);

    SetRLimit(pid, procCfg, CFG_NODE_LIMIT_RT_SIGNAL_QUEUE_SIZE, RLIMIT_SIGPENDING,
              DEFAULT_LIMIT_RT_SIGNAL_QUEUE_SIZE);

    le_cfg_CancelTxn(procCfg);

    // Get the application's user name for use with cgroups.
    char userName[LIMIT_MAX_USER_NAME_BYTES];
    LE_ASSERT(user_ConvertToUserName(proc_GetAppName(procRef), userName, sizeof(userName)) == LE_OK);

    // Add the process to its app's cgroups in each of the cgroup subsystems.
    cgrp_SubSys_t subSys = 0;
    for (; subSys < CGRP_NUM_SUBSYSTEMS; subSys++)
    {
        LE_ASSERT(cgrp_AddProc(subSys, userName, pid) == LE_OK);
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Cleans up any resources used to set the resource limits for an application.  This should be
 * called when an app is completely stopped, meaning all processes in the application have been
 * killed.
 */
//--------------------------------------------------------------------------------------------------
void resLim_CleanupApp
(
    app_Ref_t appRef                ///< [IN] Application to clean up resources for.
)
{
    const char* appNamePtr = app_GetName(appRef);

    // Get the application's user name for use with cgroups.
    char userName[LIMIT_MAX_USER_NAME_BYTES];
    LE_ASSERT(user_ConvertToUserName(appNamePtr, userName, sizeof(userName)) == LE_OK);

    // Remove cgroups for this app in each of the cgroup subsystems.
    cgrp_SubSys_t subSys = 0;
    for (; subSys < CGRP_NUM_SUBSYSTEMS; subSys++)
    {
        LE_ERROR_IF(cgrp_Delete(subSys, userName) != LE_OK,
                    "Could not remove %s cgroup for application '%s'.",
                    cgrp_SubSysName(subSys), appNamePtr);
    }
}
