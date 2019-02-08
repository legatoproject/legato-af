//--------------------------------------------------------------------------------------------------
/** @file resourceLimits.c
 *
 * Currently we use Linux's rlimits to set resource limits.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "resourceLimits.h"
#include "interfaces.h"
#include "limit.h"
#include "user.h"
#include "cgroups.h"


//--------------------------------------------------------------------------------------------------
/**
 * The name of the node in the config tree that contains an application's limit on the application's
 * file system size (in bytes).
 *
 * If this entry in the config tree is missing or is empty, then DEFAULT_LIMIT_MAX_FILE_SYSTEM_BYTES
 * will be used.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_LIMIT_MAX_FILE_SYSTEM_BYTES            "maxFileSystemBytes"


//--------------------------------------------------------------------------------------------------
/**
 * The name of the node in the config tree that contains a process's user's POSIX message queue size
 * limit.
 *
 * If this entry in the config tree is missing or is empty, then
 * DEFAULT_LIMIT_MAX_MQUEUE_BYTES will be used.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_LIMIT_MAX_MQUEUE_BYTES                 "maxMQueueBytes"


//--------------------------------------------------------------------------------------------------
/**
 * The name of the node in the config tree that contains a process's user's limit on the maximum
 * number of threads.
 *
 * If this entry in the config tree is missing or is empty, then DEFAULT_LIMIT_MAX_THREADS
 * will be used.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_LIMIT_MAX_THREADS                      "maxThreads"


//--------------------------------------------------------------------------------------------------
/**
 * The name of the node in the config tree that contains a process's user's limit on the maximum
 * number of signals that can be queued using sigqueue().
 *
 * If this entry in the config tree is missing or is empty, then DEFAULT_LIMIT_MAX_QUEUED_SIGNALS
 * will be used.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_LIMIT_MAX_QUEUED_SIGNALS               "maxQueuedSignals"


//--------------------------------------------------------------------------------------------------
/**
 * The name of the node in the config tree that contains an application's memory limit.
 *
 * If this entry in the config tree is missing or is empty, DEFAULT_LIMIT_MAX_MEMORY_BYTES is used.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_LIMIT_MAX_MEMORY_BYTES                 "maxMemoryBytes"


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
 * If this entry in the config tree is missing or is empty, DEFAULT_LIMIT_MAX_CORE_DUMP_FILE_BYTES
 * is used.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_LIMIT_MAX_CORE_DUMP_FILE_BYTES         "maxCoreDumpFileBytes"


//--------------------------------------------------------------------------------------------------
/**
 * The name of the node in the config tree that contains a process's limit on the size of files that
 * it can create/expand.
 *
 * If this entry in the config tree is missing or is empty, DEFAULT_LIMIT_MAX_FILE_BYTES is used.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_LIMIT_MAX_FILE_BYTES                    "maxFileBytes"


//--------------------------------------------------------------------------------------------------
/**
 * The name of the node in the config tree that contains a process's limit on the number of bytes of
 * memory that may be locked into RAM.
 *
 * In effect this limit is rounded down to the nearest multiple of the system page size.
 *
 * If this entry in the config tree is missing or is empty, DEFAULT_LIMIT_MAX_LOCKED_MEMORY_BYTES
 * is used.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_LIMIT_MAX_LOCKED_MEMORY_BYTES          "maxLockedMemoryBytes"


//--------------------------------------------------------------------------------------------------
/**
 * The name of the node in the config tree that contains a process's limit on the number of file
 * descriptors that the process can have open.
 *
 * The configured value must be less than MAX_LIMIT_FILE_DESCRIPTORS.
 *
 * If this entry in the config tree is missing or is empty, DEFAULT_LIMIT_MAX_FILE_DESCRIPTORS
 * is used.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_LIMIT_MAX_FILE_DESCRIPTORS             "maxFileDescriptors"


//--------------------------------------------------------------------------------------------------
/**
 * The name of the node in the config tree that contains a process's stack size limit
 *
 * The configured value must be less than MAX_LIMIT_MAX_MEMORY_BYTES.
 *
 * If this entry in the config tree is missing or is empty, DEFAULT_LIMIT_MAX_STACK_BYTES
 * is used.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_LIMIT_MAX_STACK_BYTES             "maxStackBytes"


//--------------------------------------------------------------------------------------------------
/**
 * Resource limit defaults.
 *
 * @warning These limits are only used if the limits are missing from the application's
 *          configuration.  However, they should always be present in the app config.  So, to
 *          change the defaults, modify the build tools.
 */
//--------------------------------------------------------------------------------------------------
#define DEFAULT_LIMIT_MAX_FILE_SYSTEM_BYTES             131072
#define DEFAULT_LIMIT_MAX_MQUEUE_BYTES                  512
#define DEFAULT_LIMIT_MAX_THREADS                       20
#define DEFAULT_LIMIT_MAX_QUEUED_SIGNALS                100
#define DEFAULT_LIMIT_MAX_MEMORY_BYTES                  40960000
#define DEFAULT_LIMIT_CPU_SHARE                         1024
#define DEFAULT_LIMIT_MAX_CORE_DUMP_FILE_BYTES          8192
#define DEFAULT_LIMIT_MAX_FILE_BYTES                    90112
#define DEFAULT_LIMIT_MAX_LOCKED_MEMORY_BYTES           8192
#define DEFAULT_LIMIT_MAX_FILE_DESCRIPTORS              256
#define DEFAULT_LIMIT_MAX_STACK_BYTES                   0   /* implies OS limit */


//--------------------------------------------------------------------------------------------------
/**
 * Maximum value that the limit on the number of file descriptors can be set to.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_LIMIT_FILE_DESCRIPTORS                      1024


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
    // No open config -- just use default value
    if (!limitCfg)
    {
        return defaultValue;
    }

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
                                           CFG_NODE_LIMIT_MAX_FILE_SYSTEM_BYTES,
                                           DEFAULT_LIMIT_MAX_FILE_SYSTEM_BYTES);

    if (fileSysLimit == 0)
    {
        // Zero means unlimited for tmpfs mounts and is not allowed.  Use the default limit.
        LE_ERROR("Configured resource limit %s is zero, which is invalid.  Assuming the default value %d.",
                 CFG_NODE_LIMIT_MAX_FILE_SYSTEM_BYTES,
                 DEFAULT_LIMIT_MAX_FILE_SYSTEM_BYTES);

        fileSysLimit = DEFAULT_LIMIT_MAX_FILE_SYSTEM_BYTES;
    }

    le_cfg_CancelTxn(appCfg);

    return (rlim_t)fileSysLimit;
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the specified Linux resource limit value for a process.
 */
//--------------------------------------------------------------------------------------------------
static void SetRLimitValue
(
    const char* resourceName,       // The resource name in the config tree.
    int resourceID,                 // The resource ID that setrlimit() expects.
    rlim_t value                  // The value for this resource limit.
)
{
    // Check that the limit does not exceed the maximum.
    if ( (resourceID == RLIMIT_NOFILE) && (value > MAX_LIMIT_FILE_DESCRIPTORS) )
    {
        LE_ERROR("Resource limit %s is greater than the maximum allowed limit (%d).  Using the \
maximum allowed value.", resourceName, MAX_LIMIT_FILE_DESCRIPTORS);

        value = MAX_LIMIT_FILE_DESCRIPTORS;
    }

    // Hard and soft limits are the same.
    struct rlimit lim = {value, value};

    LE_INFO("Setting resource limit %s to value %d.", resourceName, (int)lim.rlim_max);
    LE_ERROR_IF(setrlimit(resourceID, &lim) == -1,
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
    const char* appNamePtr = app_GetName(appRef);

    // Create cgroups for this application in each of the cgroup subsystems.
    cgrp_SubSys_t subSys = 0;
    while (subSys < CGRP_NUM_SUBSYSTEMS)
    {
        switch(cgrp_Create(subSys, appNamePtr))
        {
            case LE_FAULT:
                return LE_FAULT;

            case LE_DUPLICATE:
                // Try to delete the cgroup.
                if (cgrp_Delete(subSys, appNamePtr) != LE_OK)
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
    if (cgrp_cpu_SetShare(appNamePtr, cpuShare) != LE_OK)
    {
        le_cfg_CancelTxn(appCfg);
        return LE_FAULT;
    }

    // Set the memory limit.
    int maxMemoryBytes = GetCfgResourceLimit(appCfg,
                                             CFG_NODE_LIMIT_MAX_MEMORY_BYTES,
                                             DEFAULT_LIMIT_MAX_MEMORY_BYTES);

    if (cgrp_mem_SetLimit(appNamePtr, maxMemoryBytes / 1024) != LE_OK)
    {
        le_cfg_CancelTxn(appCfg);
        return LE_FAULT;
    }

    le_cfg_CancelTxn(appCfg);
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Read the resource limits from the config tree.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
void resLim_GetProcLimits
(
    proc_Ref_t procRef,             ///< [IN] The process to get resource limits for.
    resLim_ProcLimits_t* limitsPtr  ///< [OUT] The limits for the process
)
{
    le_cfg_IteratorRef_t procCfg;

    // Create an iterator for this process.
    if (proc_GetConfigPath(procRef) != NULL)
    {
        procCfg = le_cfg_CreateReadTxn(proc_GetConfigPath(procRef));
    }
    else
    {
        procCfg = NULL;
    }

    // Set the process resource limits.
    limitsPtr->maxCoreDumpFileBytes =
        GetCfgResourceLimit( procCfg, CFG_NODE_LIMIT_MAX_CORE_DUMP_FILE_BYTES,
                             DEFAULT_LIMIT_MAX_CORE_DUMP_FILE_BYTES);

    limitsPtr->maxFileBytes =
        GetCfgResourceLimit( procCfg, CFG_NODE_LIMIT_MAX_FILE_BYTES,
                             DEFAULT_LIMIT_MAX_FILE_BYTES);

    limitsPtr->maxLockedMemoryBytes =
        GetCfgResourceLimit( procCfg, CFG_NODE_LIMIT_MAX_LOCKED_MEMORY_BYTES,
                             DEFAULT_LIMIT_MAX_LOCKED_MEMORY_BYTES);

    limitsPtr->maxFileDescriptors =
        GetCfgResourceLimit( procCfg, CFG_NODE_LIMIT_MAX_FILE_DESCRIPTORS,
                             DEFAULT_LIMIT_MAX_FILE_DESCRIPTORS);

    limitsPtr->maxStackBytes =
        GetCfgResourceLimit( procCfg, CFG_NODE_LIMIT_MAX_STACK_BYTES,
                             DEFAULT_LIMIT_MAX_STACK_BYTES);

    // Set the application limits.
    //
    // @note Even though these are application limits they still need to be set for the process
    //       because Linux rlimits are applied to individual processes.

    if (procCfg)
    {
        // Goto the application config path from the process config path.
        le_cfg_GoToParent(procCfg);
        le_cfg_GoToParent(procCfg);
    }

    limitsPtr->maxMQueueBytes =
        GetCfgResourceLimit( procCfg, CFG_NODE_LIMIT_MAX_MQUEUE_BYTES,
                             DEFAULT_LIMIT_MAX_MQUEUE_BYTES);

    limitsPtr->maxThreads =
        GetCfgResourceLimit( procCfg, CFG_NODE_LIMIT_MAX_THREADS,
                             DEFAULT_LIMIT_MAX_THREADS);

    limitsPtr->maxQueuedSignals =
        GetCfgResourceLimit( procCfg, CFG_NODE_LIMIT_MAX_QUEUED_SIGNALS,
                             DEFAULT_LIMIT_MAX_QUEUED_SIGNALS);

    if (procCfg)
    {
        le_cfg_CancelTxn(procCfg);
    }
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
void resLim_SetProcLimits
(
    resLim_ProcLimits_t* limitPtr ///< [IN] The limits for the process
)
{
    // Set the process resource limits.
    SetRLimitValue(CFG_NODE_LIMIT_MAX_CORE_DUMP_FILE_BYTES, RLIMIT_CORE,
                    limitPtr->maxCoreDumpFileBytes);

    SetRLimitValue(CFG_NODE_LIMIT_MAX_FILE_BYTES, RLIMIT_FSIZE,
                    limitPtr->maxFileBytes);

    SetRLimitValue(CFG_NODE_LIMIT_MAX_LOCKED_MEMORY_BYTES, RLIMIT_MEMLOCK,
                    limitPtr->maxLockedMemoryBytes);

    SetRLimitValue(CFG_NODE_LIMIT_MAX_FILE_DESCRIPTORS, RLIMIT_NOFILE,
                    limitPtr->maxFileDescriptors);

    if (limitPtr->maxStackBytes)
    {
        SetRLimitValue(CFG_NODE_LIMIT_MAX_STACK_BYTES, RLIMIT_STACK,
                       limitPtr->maxStackBytes);
    }

    // Set the application limits.
    //
    // @note Even though these are application limits they still need to be set for the process
    //       because Linux rlimits are applied to individual processes.
    SetRLimitValue(CFG_NODE_LIMIT_MAX_MQUEUE_BYTES, RLIMIT_MSGQUEUE,
                   limitPtr->maxMQueueBytes);

    SetRLimitValue(CFG_NODE_LIMIT_MAX_THREADS, RLIMIT_NPROC,
                   limitPtr->maxThreads);

    SetRLimitValue(CFG_NODE_LIMIT_MAX_QUEUED_SIGNALS, RLIMIT_SIGPENDING,
                   limitPtr->maxQueuedSignals);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add a process to the app's cgroups.
 */
//--------------------------------------------------------------------------------------------------
void resLim_SetCGroups
(
    proc_Ref_t procRef
)
{
    pid_t pid = proc_GetPID(procRef);

    // Add the process to its app's cgroups in each of the cgroup subsystems.
    cgrp_SubSys_t subSys = 0;
    for (; subSys < CGRP_NUM_SUBSYSTEMS; subSys++)
    {
        // Do not add realtime processes to the cpu cgroup.
        if ( (subSys != CGRP_SUBSYS_CPU) || (!proc_IsRealtime(procRef)) )
        {
            LE_ASSERT(cgrp_AddProc(subSys, proc_GetAppName(procRef), pid) == LE_OK);
        }
    }
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

    // Remove cgroups for this app in each of the cgroup subsystems.
    cgrp_SubSys_t subSys = 0;
    for (; subSys < CGRP_NUM_SUBSYSTEMS; subSys++)
    {
        LE_ERROR_IF(cgrp_Delete(subSys, appNamePtr) != LE_OK,
                    "Could not remove %s cgroup for application '%s'.",
                    cgrp_SubSysName(subSys), appNamePtr);
    }
}
