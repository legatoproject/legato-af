/** @file limit.h
 *
 * This module contains limit definitions used internally by the Legato framework.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LE_SRC_LIMITS_INCLUDE_GUARD
#define LE_SRC_LIMITS_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Maximum string length and byte storage size of SMACK labels.
 *
 * @note This limit must match the SMACK limit.  It must not be changed arbitrarily.
 */
//--------------------------------------------------------------------------------------------------
#define LIMIT_MAX_SMACK_LABEL_LEN               255
#define LIMIT_MAX_SMACK_LABEL_BYTES             (LIMIT_MAX_SMACK_LABEL_LEN + 1)


//--------------------------------------------------------------------------------------------------
/**
 * Maximum string length and byte storage size of application names.
 */
//--------------------------------------------------------------------------------------------------
#define LIMIT_MAX_APP_NAME_LEN                  47
#define LIMIT_MAX_APP_NAME_BYTES                (LIMIT_MAX_APP_NAME_LEN + 1)


//--------------------------------------------------------------------------------------------------
/**
 * Maximum string length and byte storage size of application hash.
 */
//--------------------------------------------------------------------------------------------------
#define LIMIT_MAX_APP_HASH_LEN                  100
#define LIMIT_MAX_APP_HASH_BYTES                (LIMIT_MAX_APP_HASH_LEN + 1)


//--------------------------------------------------------------------------------------------------
/**
 * Maximum string length and byte storage size of system names.
 */
//--------------------------------------------------------------------------------------------------
#define LIMIT_MAX_SYSTEM_NAME_LEN               63
#define LIMIT_MAX_SYSTEM_NAME_BYTES             (LIMIT_MAX_SYSTEM_NAME_LEN + 1)


//--------------------------------------------------------------------------------------------------
/**
 * Maximum string length and byte storage size of user names.
 */
//--------------------------------------------------------------------------------------------------
#define LIMIT_MAX_USER_NAME_LEN                 (LIMIT_MAX_APP_NAME_LEN + 10)
#define LIMIT_MAX_USER_NAME_BYTES               (LIMIT_MAX_USER_NAME_LEN + 1)


//--------------------------------------------------------------------------------------------------
/**
 * Maximum string length and byte storage size of process names.
 */
//--------------------------------------------------------------------------------------------------
#define LIMIT_MAX_PROCESS_NAME_LEN              47
#define LIMIT_MAX_PROCESS_NAME_BYTES            (LIMIT_MAX_PROCESS_NAME_LEN + 1)


//--------------------------------------------------------------------------------------------------
/**
 * Maximum string length and byte storage size of thread names.
 */
//--------------------------------------------------------------------------------------------------
#define LIMIT_MAX_THREAD_NAME_LEN               47
#define LIMIT_MAX_THREAD_NAME_BYTES             (LIMIT_MAX_THREAD_NAME_LEN + 1)


//--------------------------------------------------------------------------------------------------
/**
 * Maximum string length and byte storage size of component names.
 */
//--------------------------------------------------------------------------------------------------
#define LIMIT_MAX_COMPONENT_NAME_LEN            47
#define LIMIT_MAX_COMPONENT_NAME_BYTES          (LIMIT_MAX_COMPONENT_NAME_LEN + 1)


//--------------------------------------------------------------------------------------------------
/**
 * Maximum string length and byte storage size of log trace keyword strings.
 */
//--------------------------------------------------------------------------------------------------
#define LIMIT_MAX_LOG_KEYWORD_LEN               63
#define LIMIT_MAX_LOG_KEYWORD_BYTES             (LIMIT_MAX_LOG_KEYWORD_LEN + 1)


//--------------------------------------------------------------------------------------------------
/**
 * Maximum string length and byte storage size of socket names.
 */
//--------------------------------------------------------------------------------------------------
#define LIMIT_MAX_SOCKET_NAME_LEN               63
#define LIMIT_MAX_SOCKET_NAME_BYTES             (LIMIT_MAX_SOCKET_NAME_LEN + 1)


//--------------------------------------------------------------------------------------------------
/**
 * Maximum string length and byte storage size of path strings.
 */
//--------------------------------------------------------------------------------------------------
#define LIMIT_MAX_PATH_LEN                      511
#define LIMIT_MAX_PATH_BYTES                    (LIMIT_MAX_PATH_LEN + 1)


//--------------------------------------------------------------------------------------------------
/**
 * Maximum string length and byte storage size of argument lists.
 */
//--------------------------------------------------------------------------------------------------
#define LIMIT_MAX_ARGS_STR_LEN                  511
#define LIMIT_MAX_ARGS_STR_BYTES                (LIMIT_MAX_ARGS_STR_LEN + 1)


//--------------------------------------------------------------------------------------------------
/**
 * Maximum string length and byte storage size of semaphore names.
 */
//--------------------------------------------------------------------------------------------------
#define LIMIT_MAX_SEMAPHORE_NAME_LEN            31
#define LIMIT_MAX_SEMAPHORE_NAME_BYTES          (LIMIT_MAX_SEMAPHORE_NAME_LEN + 1)


//--------------------------------------------------------------------------------------------------
/**
 * Maximum string length and byte storage size of timer names.
 */
//--------------------------------------------------------------------------------------------------
#define LIMIT_MAX_TIMER_NAME_LEN                31
#define LIMIT_MAX_TIMER_NAME_BYTES              (LIMIT_MAX_TIMER_NAME_LEN + 1)


//--------------------------------------------------------------------------------------------------
/**
 * Maximum string length and byte storage size of memory pool names (excluding the component
 * name prefix).
 */
//--------------------------------------------------------------------------------------------------
#define LIMIT_MAX_MEM_POOL_NAME_LEN             31
#define LIMIT_MAX_MEM_POOL_NAME_BYTES           (LIMIT_MAX_MEM_POOL_NAME_LEN + 1)


//--------------------------------------------------------------------------------------------------
/**
 * Maximum string length and byte storage size of environment variable names.
 */
//--------------------------------------------------------------------------------------------------
#define LIMIT_MAX_ENV_VAR_NAME_LEN              31
#define LIMIT_MAX_ENV_VAR_NAME_BYTES            (LIMIT_MAX_TIMER_NAME_LEN + 1)


//--------------------------------------------------------------------------------------------------
/**
 * Maximum string length and byte storage size of priority names.
 */
//--------------------------------------------------------------------------------------------------
#define LIMIT_MAX_PRIORITY_NAME_LEN             6
#define LIMIT_MAX_PRIORITY_NAME_BYTES           (LIMIT_MAX_PRIORITY_NAME_LEN + 1)


//--------------------------------------------------------------------------------------------------
/**
 * Maximum string length and byte storage size of fault action strings.
 */
//--------------------------------------------------------------------------------------------------
#define LIMIT_MAX_FAULT_ACTION_NAME_LEN         20
#define LIMIT_MAX_FAULT_ACTION_NAME_BYTES       (LIMIT_MAX_FAULT_ACTION_NAME_LEN + 1)


//--------------------------------------------------------------------------------------------------
/**
 * The likely limit on number of possible open file descriptors in a process.
 */
//--------------------------------------------------------------------------------------------------
#define LIMIT_MAX_NUM_PROCESS_FD                1024


//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of command line arguments that can be passed to a process.
 */
//--------------------------------------------------------------------------------------------------
#define LIMIT_MAX_NUM_CMD_LINE_ARGS             20


//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of environment variables for a process.
 */
//--------------------------------------------------------------------------------------------------
#define LIMIT_MAX_NUM_ENV_VARS                  30


//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of supplementary groups for a process.
 */
//--------------------------------------------------------------------------------------------------
#define LIMIT_MAX_NUM_SUPPLEMENTARY_GROUPS      30


//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of bytes in a mount entry.
 */
//--------------------------------------------------------------------------------------------------
#define LIMIT_MAX_MNT_ENTRY_BYTES               (3 * LIMIT_MAX_PATH_BYTES)


//--------------------------------------------------------------------------------------------------
/**
 * Maximum size of an IPC protocol identity string, including the null terminator byte.
 */
//--------------------------------------------------------------------------------------------------
#define LIMIT_MAX_PROTOCOL_ID_BYTES             128


//--------------------------------------------------------------------------------------------------
/**
 * Maximum size of an IPC interface name string, including the null terminator byte.
 */
//--------------------------------------------------------------------------------------------------
#define LIMIT_MAX_IPC_INTERFACE_NAME_BYTES      128


//--------------------------------------------------------------------------------------------------
/**
 * Maximum size of an event handler name.
 **/
//--------------------------------------------------------------------------------------------------
#define LIMIT_MAX_EVENT_HANDLER_NAME_BYTES      32


//--------------------------------------------------------------------------------------------------
/**
 * Maximum size of an event name.
 **/
//--------------------------------------------------------------------------------------------------
#define LIMIT_MAX_EVENT_NAME_BYTES              LIMIT_MAX_EVENT_HANDLER_NAME_BYTES + 15


//--------------------------------------------------------------------------------------------------
/**
 * Size of a MD5 string.
 **/
//--------------------------------------------------------------------------------------------------
#define LIMIT_MD5_STR_BYTES       (33)


#endif // LE_SRC_LIMITS_INCLUDE_GUARD
