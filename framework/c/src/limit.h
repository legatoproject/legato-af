/** @file limit.h
 *
 * This module contains limit definitions used internally by the Legato framework.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#ifndef LE_SRC_LIMITS_INCLUDE_GUARD
#define LE_SRC_LIMITS_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Maximum string length and byte storage size of application names.
 */
//--------------------------------------------------------------------------------------------------
#define LIMIT_MAX_APP_NAME_LEN                  31
#define LIMIT_MAX_APP_NAME_BYTES                (LIMIT_MAX_APP_NAME_LEN + 1)


//--------------------------------------------------------------------------------------------------
/**
 * Maximum string length and byte storage size of user names.
 */
//--------------------------------------------------------------------------------------------------
#define LIMIT_MAX_USER_NAME_LEN                 63
#define LIMIT_MAX_USER_NAME_BYTES               (LIMIT_MAX_USER_NAME_LEN + 1)


//--------------------------------------------------------------------------------------------------
/**
 * Maximum string length and byte storage size of process names.
 */
//--------------------------------------------------------------------------------------------------
#define LIMIT_MAX_PROCESS_NAME_LEN              31
#define LIMIT_MAX_PROCESS_NAME_BYTES            (LIMIT_MAX_PROCESS_NAME_LEN + 1)


//--------------------------------------------------------------------------------------------------
/**
 * Maximum string length and byte storage size of thread names.
 */
//--------------------------------------------------------------------------------------------------
#define LIMIT_MAX_THREAD_NAME_LEN               31
#define LIMIT_MAX_THREAD_NAME_BYTES             (LIMIT_MAX_THREAD_NAME_LEN + 1)


//--------------------------------------------------------------------------------------------------
/**
 * Maximum string length and byte storage size of component names.
 */
//--------------------------------------------------------------------------------------------------
#define LIMIT_MAX_COMPONENT_NAME_LEN            31
#define LIMIT_MAX_COMPONENT_NAME_BYTES          (LIMIT_MAX_COMPONENT_NAME_LEN + 1)


//--------------------------------------------------------------------------------------------------
/**
 * Maximum string length and byte storage size of log trace keyword strings.
 */
//--------------------------------------------------------------------------------------------------
#define LIMIT_MAX_LOG_KEYWORD_LEN               31
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
#define LIMIT_MAX_PATH_LEN                      254
#define LIMIT_MAX_PATH_BYTES                    (LIMIT_MAX_PATH_LEN + 1)


//--------------------------------------------------------------------------------------------------
/**
 * Maximum string length and byte storage size of argument lists.
 */
//--------------------------------------------------------------------------------------------------
#define LIMIT_MAX_ARGS_STR_LEN                  127
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
#define LIMIT_MAX_TIMER_NAME_BYTES              (LIMIT_MAX_TIMER_NAME_LEN+1)


//--------------------------------------------------------------------------------------------------
/**
 * Maximum string length and byte storage size of environment variable names.
 */
//--------------------------------------------------------------------------------------------------
#define LIMIT_MAX_ENV_VAR_NAME_LEN              31
#define LIMIT_MAX_ENV_VAR_NAME_BYTES            (LIMIT_MAX_TIMER_NAME_LEN+1)


//--------------------------------------------------------------------------------------------------
/**
 * Maximum string length and byte storage size of priority names.
 */
//--------------------------------------------------------------------------------------------------
#define LIMIT_MAX_PRIORITY_NAME_LEN             4
#define LIMIT_MAX_PRIORITY_NAME_BYTES           (LIMIT_MAX_PRIORITY_NAME_LEN+1)


//--------------------------------------------------------------------------------------------------
/**
 * Maximum string length and byte storage size of fault action strings.
 */
//--------------------------------------------------------------------------------------------------
#define LIMIT_MAX_FAULT_ACTION_NAME_LEN         20
#define LIMIT_MAX_FAULT_ACTION_NAME_BYTES       (LIMIT_MAX_FAULT_ACTION_NAME_LEN+1)


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


#endif // LE_SRC_LIMITS_INCLUDE_GUARD
