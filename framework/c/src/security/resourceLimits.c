//--------------------------------------------------------------------------------------------------
/** @file resourceLimits.c
 *
 * Currently we use Linux's rlimits to set resource limits.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013-2014. Use of this work is subject to license.
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
#define DEFAULT_LIMIT_NUM_PROCESSES                     20
#define DEFAULT_LIMIT_RT_SIGNAL_QUEUE_SIZE              100
#define DEFAULT_LIMIT_MEMORY                            40960
#define DEFAULT_LIMIT_CPU_SHARE                         1024
#define DEFAULT_LIMIT_CORE_DUMP_FILE_SIZE               8192
#define DEFAULT_LIMIT_MAX_FILE_SIZE                     90112
#define DEFAULT_LIMIT_MEM_LOCK_SIZE                     8192
#define DEFAULT_LIMIT_NUM_FD                            256


//--------------------------------------------------------------------------------------------------
/**
 * Maximum value that the limit on the number of file descriptors can be set to.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_LIMIT_NUM_FD                                1024


//--------------------------------------------------------------------------------------------------
/**
 * Gets the resource limit value from the config tree.
 *
 * @return
 *      The resource limit from the config tree if it is valid.  If the value in the config tree is
 *      invalid the default value is returned.
 */
//--------------------------------------------------------------------------------------------------
static int GetCfgResourceLimit
(
    le_cfg_IteratorRef_t limitCfg,  // The iterator to use to read the configured limit.  This
                                    // iterator is owned by the caller and should not be deleted
                                    // in this function.
    const char* nodeName,           // The name of the node in the config tree that holds the value.
    int defaultValue                // The default value to use if the config value is invalid.
)
{
    int limitValue = le_cfg_GetInt(limitCfg, nodeName, defaultValue);

    if (!le_cfg_NodeExists(limitCfg, nodeName))
    {
        LE_INFO("Configured resource limit %s is not available.  Using the default value %d.",
                 nodeName, defaultValue);

        return defaultValue;
    }

    if (le_cfg_IsEmpty(limitCfg, nodeName))
    {
        LE_WARN("Configured resource limit %s is empty.  Using the default value %d.",
                 nodeName, defaultValue);

        return defaultValue;
    }

    if (le_cfg_GetNodeType(limitCfg, nodeName) != LE_CFG_TYPE_INT)
    {
        LE_ERROR("Configured resource limit %s is the wrong type.  Using the default value %d.",
                 nodeName, defaultValue);

        return defaultValue;
    }

    if (limitValue < 0)
    {
        LE_ERROR("Configured resource limit %s is negative.  Using the default value %d.",
                 nodeName, defaultValue);

        return defaultValue;
    }

    return limitValue;
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
    // Create a config iterator to get the file system limit from the config tree.
    le_cfg_IteratorRef_t appCfg = le_cfg_CreateReadTxn(app_GetConfigPath(appRef));

    // Get the resource limit from the config tree.
    int fileSysLimit = GetCfgResourceLimit(appCfg,
                                           CFG_NODE_LIMIT_FILE_SYSTEM_SIZE,
                                           DEFAULT_LIMIT_FILE_SYSTEM_SIZE);

    if (fileSysLimit == 0)
    {
        // Zero means unlimited for tmpfs mounts and is not allowed.  Use the default limit.
        LE_ERROR("Configured resource limit %s is zero, which is invalid.  Assuming the default value %d.",
                 CFG_NODE_LIMIT_FILE_SYSTEM_SIZE,
                 DEFAULT_LIMIT_FILE_SYSTEM_SIZE);

        fileSysLimit = DEFAULT_LIMIT_FILE_SYSTEM_SIZE;
    }

    le_cfg_CancelTxn(appCfg);

    return (rlim_t)fileSysLimit;
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
    int defaultValue                // The default value for this resource limit.
)
{
    // Get the limit value from the config tree.
    int limit = GetCfgResourceLimit(procCfg, resourceName, defaultValue);

    // Check that the limit does not exceed the maximum.
    if ( (resourceID == RLIMIT_NOFILE) && (limit > MAX_LIMIT_NUM_FD) )
    {
        LE_ERROR("Resource limit %s is greater than the maximum allowed limit (%d).  Using the \
maximum allowed value.", resourceName, MAX_LIMIT_NUM_FD);

        limit = MAX_LIMIT_NUM_FD;
    }

    // Hard and soft limits are the same.
    struct rlimit lim = {limit, limit};

    LE_INFO("Setting resource limit %s to value %d.", resourceName, (int)lim.rlim_max);
    LE_ERROR_IF(prlimit(pid, resourceID, &lim, NULL) == -1,
                "Could not set resource limit %s (%d).  %m.", resourceName, resourceID);
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the resource limits for the specified application.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t resLim_SetAppLimits
(
    app_Ref_t appRef                ///< [IN] The application to set resource limits for.
)
{
    // Get the application's user name for use with cgroups.
    char userName[LIMIT_MAX_USER_NAME_BYTES];
    LE_ASSERT(user_AppNameToUserName(app_GetName(appRef), userName, sizeof(userName)) == LE_OK);

    // Create cgroups for this application in each of the cgroup subsystems.
    cgrp_SubSys_t subSys = 0;
    while (subSys < CGRP_NUM_SUBSYSTEMS)
    {
        switch(cgrp_Create(subSys, userName))
        {
            case LE_FAULT:
                return LE_FAULT;

            case LE_DUPLICATE:
                // Try to delete the cgroup.
                if (cgrp_Delete(subSys, userName) != LE_OK)
                {
                    return LE_FAULT;
                }

                // Go on and try to create it again.
                break;

            default:
                // Successfully created the cgroup go onto the next sub system.
                subSys++;
                break;
        }
    }

    // Create a config iterator for this app.
    le_cfg_IteratorRef_t appCfg = le_cfg_CreateReadTxn(app_GetConfigPath(appRef));

    // Get the cpu share value from the config.
    int cpuShare = GetCfgResourceLimit(appCfg, CFG_NODE_LIMIT_CPU_SHARE, DEFAULT_LIMIT_CPU_SHARE);

    // Set the cpu limit.
    if (cgrp_cpu_SetShare(userName, cpuShare) != LE_OK)
    {
        le_cfg_CancelTxn(appCfg);
        return LE_FAULT;
    }

    // Set the memory limit.
    int memLimit = GetCfgResourceLimit(appCfg, CFG_NODE_LIMIT_MEMORY, DEFAULT_LIMIT_MEMORY);

    if (cgrp_mem_SetLimit(userName, memLimit) != LE_OK)
    {
        le_cfg_CancelTxn(appCfg);
        return LE_FAULT;
    }

    le_cfg_CancelTxn(appCfg);
    return LE_OK;
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
    LE_ASSERT(user_AppNameToUserName(proc_GetAppName(procRef), userName, sizeof(userName)) == LE_OK);

    // Add the process to its app's cgroups in each of the cgroup subsystems.
    cgrp_SubSys_t subSys = 0;
    for (; subSys < CGRP_NUM_SUBSYSTEMS; subSys++)
    {
        // Do not add realtime processes to the cpu cgroup.
        if ( (subSys != CGRP_SUBSYS_CPU) || (!proc_IsRealtime(procRef)) )
        {
            LE_ASSERT(cgrp_AddProc(subSys, userName, pid) == LE_OK);
        }
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
    LE_ASSERT(user_AppNameToUserName(appNamePtr, userName, sizeof(userName)) == LE_OK);

    // Remove cgroups for this app in each of the cgroup subsystems.
    cgrp_SubSys_t subSys = 0;
    for (; subSys < CGRP_NUM_SUBSYSTEMS; subSys++)
    {
        LE_ERROR_IF(cgrp_Delete(subSys, userName) != LE_OK,
                    "Could not remove %s cgroup for application '%s'.",
                    cgrp_SubSysName(subSys), appNamePtr);
    }
}
