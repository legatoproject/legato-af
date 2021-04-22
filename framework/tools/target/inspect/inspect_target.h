/**
 * @file inspect_target.h
 *
 * Interface between main platform independent inspect code and the target-specific
 * lowers.  Contains interfaces for stopping, starting and reading addresses from
 * the target process.
 *
 */

#ifndef LEGATO_INSPECT_TARGET_INCLUDE_GUARD
#define LEGATO_INSPECT_TARGET_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Prints a generic message on stderr so that the user is aware there is a problem, logs the
 * internal error message and exit.
 */
//--------------------------------------------------------------------------------------------------
#define INTERNAL_ERR(formatString, ...)                                                 \
            { fprintf(stderr, "Internal error check logs for details.\n");              \
              LE_FATAL(formatString, ##__VA_ARGS__); }


//--------------------------------------------------------------------------------------------------
/**
 * If the condition is true, print a generic message on stderr so that the user is aware there is a
 * problem, log the internal error message and exit.
 */
//--------------------------------------------------------------------------------------------------
#define INTERNAL_ERR_IF(condition, formatString, ...)                                   \
        if (condition) { INTERNAL_ERR(formatString, ##__VA_ARGS__); }


//--------------------------------------------------------------------------------------------------
/**
 * Gets the counterpart address of the specified local reference in the address space of the
 * specified process.
 *
 * @return
 *      Remote address that is the counterpart of the local address.
 */
//--------------------------------------------------------------------------------------------------
uintptr_t target_GetRemoteAddress
(
    pid_t pid,              ///< [IN] Remote process to to get the address for.
    void* localAddrPtr      ///< [IN] Local address to get the offset with.
);

//--------------------------------------------------------------------------------------------------
/**
 * Attach to the target process in order to gain control of its execution and access its memory
 * space.
 */
//--------------------------------------------------------------------------------------------------
void target_Attach
(
    pid_t pid              ///< [IN] Remote process to attach to
);

//--------------------------------------------------------------------------------------------------
/**
 * Detach from a process that we had previously attached to, and exit
 */
//--------------------------------------------------------------------------------------------------
void target_DetachAndExit
(
    pid_t pid              ///< [IN] Remote process to detach from
);

//--------------------------------------------------------------------------------------------------
/**
 * Pause execution of a running process which we had previously attached to.
 */
//--------------------------------------------------------------------------------------------------
void target_Stop
(
    pid_t pid              ///< [IN] Remote process to stop.
);

//--------------------------------------------------------------------------------------------------
/**
 * Resume execution of a previously paused process.
 */
//--------------------------------------------------------------------------------------------------
void target_Start
(
    pid_t pid              ///< [IN] Remote process to restart
);

//--------------------------------------------------------------------------------------------------
/**
 * Read from the memory of an attached target process.
 */
//--------------------------------------------------------------------------------------------------
le_result_t target_ReadAddress
(
    pid_t pid,              ///< [IN] Remote process to read address
    uintptr_t remoteAddr,   ///< [IN] Remote address to read from target
    void* buffer,           ///< [OUT] Destination to read into
    size_t size             ///< [IN] Number of bytes to read
);

#endif /* LEGATO_INSPECT_TARGET_INCLUDE_GUARD */
