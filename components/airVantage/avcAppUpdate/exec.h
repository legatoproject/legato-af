
//--------------------------------------------------------------------------------------------------
/**
 *  Subsystem to help with executing external programs.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_EXEC_INCLUDE_GUARD
#define LEGATO_EXEC_INCLUDE_GUARD




//--------------------------------------------------------------------------------------------------
/**
 *  Execution finished callback handler.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*exec_ResultHandler_t)(int exitStatus, void* contextPtr);




//--------------------------------------------------------------------------------------------------
/**
 *  Initialize the Exec subsystem.
 */
//--------------------------------------------------------------------------------------------------
void exec_Init
(
    void
);




//--------------------------------------------------------------------------------------------------
/**
 *  Execute a new process asynchronously, optionally giving it custom handles for it's standard I/O.
 *  If a callback is given, then that function is called when the calling process returns.
 *
 *  @warn The child process will inherit all of the file descriptors of the parent.  Descriptors
 *        that shouldn't be inherited should be marked as O_CLOEXEC.
 *
 *  @return LE_OK if all goes to plan, LE_FAULT if it does not.
 */
//--------------------------------------------------------------------------------------------------
le_result_t exec_RunProcess
(
    const char* commandStr,              ///< The name/path of the program to exec.
    const char* arguments[],             ///< The command line arguments for the program.
    int stdInDescriptor,                 ///< The file descriptor for the processes standard IN.
                                         ///< Pass -1 if not used.
    int stdOutDescriptor,                ///< The file descriptor for the processes standard OUT.
                                         ///< Pass -1 if not used.
    int stdErrDescriptor,                ///< The file descriptor for the processes standard ERR.
                                         ///< Pass -1 if not used.
    exec_ResultHandler_t resultHandler,  ///< Function to be called when the program completes.
    void* contextPtr                     ///< Remembered context for the handler function.
);




#endif
