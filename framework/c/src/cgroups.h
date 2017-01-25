/** @file cgroups.h
 *
 * @ref c_cgrp_layout <br>
 * @ref c_cgrp_init <br>
 * @ref c_cgrp_create <br>
 * @ref c_cgrp_settingAttributes <br>
 * @ref c_cgrp_addingProcesses <br>
 * @ref c_cgrp_delete <br>
 * @ref c_cgrp_threadSafety <br>
 *
 *
 * Cgroups, short for control groups, is a Linux kernel feature that allows hierarchal groupings of
 * processes.  Each group can then be configured with specific attributes that apply to the entire
 * group.  Control groups should not be confused with process groups as this is a different concept.
 *
 * A cgroup can contain other sub-groups and can be arranged in a tree structure similar to
 * directories in a file system.  However, unlike a file system cgroups can have multiple roots.
 * These separate cgroup trees are called hierarchies.  For example, a system's cgroups could be
 * arranged in the following manner:
 *
 * @verbatim
                   Hierarchy1                      Hierarchy2
                     /    \                          /     \
                group1    group2                 group1    group2
                /    \                                         \
          subgroup1  subgroup2                             subgroup1
@endverbatim
 *
 * Attributes that a cgroup can have are available through kernel sub-systems.  For example,
 * the memory sub-system can be used to set the memory limit for all processes in a cgroup.  A
 * hierarchy must have at least one sub-system attached to it and in general a sub-system can only
 * be attached to a single hierarchy.
 *
 * A process can only be a part of one cgroup in each hierarchy.  Having separate hierarchies allows
 * for more flexible control of cgroups.
 *
 * In practice cgroups are used mainly for limiting system resources.
 *
 *
 * @section c_cgrp_layout Hierarchy Layout
 *
 * In this implementation of cgroups each sub-system is attached to its own hierarchy.  In other
 * words there is a one-to-one mapping of hierarchy and sub-systems so the terms hierarchy and
 * sub-system will be used interchangeably henceforth.
 *
 *
 * @section c_cgrp_init Initialization
 *
 * On system start-up the cgrp_Init() function must be called to setup the hierarchies.  Cgroups are
 * by default non-persistent so cgrp_Init() must called every time the system starts.
 *
 *
 * @section c_cgrp_create Creating cgroups
 *
 * To create a cgroup for a sub-system call cgrp_Create().
 *
 *
 * @section c_cgrp_settingAttributes Setting cgroup Attributes
 *
 * Cgroups created for a specific sub-system can only set attributes specific to that sub-system.
 * For example:
 *
 * @code
 *      // cgroup created for the cpu sub-system.
 *      cgrp_Create(CGRP_SUBSYS_CPU, "MyApp");
 *
 *      // cgroup created for the memory sub-system with the same name.  This is a separate cgroup
 *      // but it can have the same name because it is in a different hierarchy.
 *      cgrp_Create(CGRP_SUBSYS_MEM, "MyApp");
 *
 *      // Set the cpu share for the cgroup in the cpu sub-system to half of the default value.
 *      cgrp_cpu_SetShare("MyApp", 512);
 *
 *      // Set the memory limit for the cgroup in the memory sub-system.
 *      cgrp_mem_SetLimit("MyApp", 100);
 * @endcode
 *
 *
 * @section c_cgrp_addingProcesses Adding Processes to a cgroup
 *
 * Processes can be added to a cgroup, by PID, using cgrp_AddProc().  If a process already belonging
 * to a cgroup is added to another cgroup in the same hierarchy, the process is moved but not copied
 * to the second cgroup, because processes can only be in one cgroup per hierarchy.
 *
 * Processes that are forked by other processes always inherit the cgroup of their parent.
 *
 * When a process dies it is automatically removed from all cgroups it belongs to.
 *
 * @section c_cgrp_delete Deleting cgroups
 *
 * To delete a cgroup call cgrp_Delete().  Cgroups can only be deleted if they do not contain any
 * processes.
 *
 *
 * @section c_cgrp_threadSafety Thread Safety
 *
 * The functions in this API are not thread safe.  Other synchronization methods must be used to
 * control concurrent access to the cgroups.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_SRC_CGROUPS_INCLUDE_GUARD
#define LEGATO_SRC_CGROUPS_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Cgroup sub-systems.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    CGRP_SUBSYS_CPU = 0,        ///< CPU sub-system.
    CGRP_SUBSYS_MEM,            ///< Memory sub-system.
    CGRP_SUBSYS_FREEZE,         ///< Freezer sub-system.
    CGRP_NUM_SUBSYSTEMS         ///< Number of sub-systems.  Must be the last item in this enum.
}
cgrp_SubSys_t;


//--------------------------------------------------------------------------------------------------
/**
 * Cgroup freeze state.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    CGRP_FROZEN = 0,            ///< All tasks in the cgroup are frozen.
    CGRP_THAWED                 ///< All tasks in the cgroup are not frozen.
}
cgrp_FreezeState_t;


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
);


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
 * @return
 *      LE_OK if successful.
 *      LE_DUPLICATE if the cgroup already exists.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t cgrp_Create
(
    cgrp_SubSys_t subsystem,        ///< [IN] Sub-system the cgroup belongs to.
    const char* cgroupNamePtr       ///< [IN] Name of the cgroup to create.
);


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
    cgrp_SubSys_t subsystem,        ///< [IN] Sub-system of the cgroup.
    const char* cgroupNamePtr,      ///< [IN] Name of the cgroup to add the process to.
    pid_t pidToAdd                  ///< [IN] PID of the process to add.
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets a list of threads that are in a cgroup.  The number of threads in the cgroup may be
 * larger than maxTids, in which case tidListPtr will be filled with the first maxTids TIDs.
 *
 * @return
 *      The number of threads that are in the cgroup if successful.
 *      LE_FAULT if there was some other error.
 */
//--------------------------------------------------------------------------------------------------
ssize_t cgrp_GetThreadList
(
    cgrp_SubSys_t subsystem,        ///< [IN] Sub-system of the cgroup.
    const char* cgroupNamePtr,      ///< [IN] Name of the cgroup.
    pid_t* tidListPtr,              ///< [OUT] Buffer that will contain the list of TIDs.
    size_t maxTids                  ///< [IN] The maximum number of tids tidListPtr can hold.
);

//--------------------------------------------------------------------------------------------------
/**
 * Gets a list of threads that are in a cgroup.  The number of threads in the cgroup may be
 * larger than maxTids, in which case tidListPtr will be filled with the first maxTids TIDs.
 *
 * @return
 *      The number of threads that are in the cgroup if successful.
 *      LE_FAULT if there was some other error.
 */
//--------------------------------------------------------------------------------------------------
ssize_t cgrp_GetProcessesList
(
    cgrp_SubSys_t subsystem,        ///< [IN] Sub-system of the cgroup.
    const char* cgroupNamePtr,      ///< [IN] Name of the cgroup.
    pid_t* idListPtr,               ///< [OUT] Buffer that will contain the list of TIDs.
    size_t maxIds                   ///< [IN] The maximum number of tids tidListPtr can hold.
);

//--------------------------------------------------------------------------------------------------
/**
 * Sends the specified signal to all the processes in the specified cgroup.
 *
 * @return
 *      The number of PIDs that are in the cgroup.
 *      LE_FAULT if there an error.
 */
//--------------------------------------------------------------------------------------------------
ssize_t cgrp_SendSig
(
    cgrp_SubSys_t subsystem,        ///< [IN] Sub-system of the cgroup.
    const char* cgroupNamePtr,      ///< [IN] Name of the cgroup.
    int sig                         ///< [IN] The signal to send.
);


//--------------------------------------------------------------------------------------------------
/**
 * Checks if the specified cgroup is empty of all processes.
 *
 * @return
 *      true if the specified cgroup has no processes in it.
 *      false if there are processes in the specified cgroup.
 */
//--------------------------------------------------------------------------------------------------
bool cgrp_IsEmpty
(
    cgrp_SubSys_t subsystem,        ///< [IN] Sub-system of the cgroup.
    const char* cgroupNamePtr       ///< [IN] Name of the cgroup.
);


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
    cgrp_SubSys_t subsystem,        ///< [IN] Sub-system of the cgroup.
    const char* cgroupNamePtr       ///< [IN] Name of the cgroup to delete.
);


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
    cgrp_SubSys_t subsystem         ///< [IN] Sub-system.
);


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
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t cgrp_cpu_SetShare
(
    const char* cgroupNamePtr,      ///< [IN] Name of the cgroup to set the share for.
    size_t share                    ///< [IN] Share value to set.  See the function header for more
                                    ///  details.
);


//--------------------------------------------------------------------------------------------------
/**
 * Sets the memory limit for a cgroup.
 *
 * @note All processes in a cgroup share the available memory for that cgroup.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t cgrp_mem_SetLimit
(
    const char* cgroupNamePtr,      ///< [IN] Name of the cgroup to set the limit for.
    size_t limit                    ///< [IN] Memory limit in kilobytes.
);


//--------------------------------------------------------------------------------------------------
/**
 * Freezes all the tasks in a cgroup.  This is an asynchronous function call that returns
 * immediately at which point the freeze state of the cgroup may not be updated yet.  Check the
 * current state of the cgroup using cgrp_frz_GetState().  Once a cgroup is frozen all tasks in the
 * cgroup are prevented from being scheduled by the kernel.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t cgrp_frz_Freeze
(
    const char* cgroupNamePtr       ///< [IN] Name of the cgroup.
);


//--------------------------------------------------------------------------------------------------
/**
 * Thaws all the tasks in a cgroup.  This is an asynchronous function call that returns
 * immediately at which point the freeze state of the cgroup may not be updated yet.  Check the
 * current state of the cgroup using cgrp_frz_GetState().  Once a cgroup is thawed all tasks in the
 * cgroup are permitted to be scheduled by the kernel.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t cgrp_frz_Thaw
(
    const char* cgroupNamePtr       ///< [IN] Name of the cgroup.
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets the freeze state of the cgroup.
 *
 * @return
 *      Freeze state of the cgroup if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
cgrp_FreezeState_t cgrp_frz_GetState
(
    const char* cgroupNamePtr       ///< [IN] Name of the cgroup.
);

//--------------------------------------------------------------------------------------------------
/**
 * Gets the amount of memory used in bytes by a cgroup
 *
 * @return
 *      Number of bytes in use by the cgroup.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
ssize_t cgrp_GetMemUsed
(
    const char* cgroupNamePtr       ///< [IN] Name of the cgroup.
);

//--------------------------------------------------------------------------------------------------
/**
 * Gets the imum amount of memory used in bytes by a cgroup.
 * @return
 *      Maximum number of bytes used at any time up to now by this cgroup.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
ssize_t cgrp_GetMaxMemUsed
(
    const char* cgroupNamePtr       ///< [IN] Name of the cgroup.
);

#endif // LEGATO_SRC_CGROUPS_INCLUDE_GUARD
