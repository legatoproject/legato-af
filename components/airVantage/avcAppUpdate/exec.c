
//--------------------------------------------------------------------------------------------------
/**
 *  Subsystem to help with executing external programs.
 *
 *  Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "log.h"
#include "exec.h"




//--------------------------------------------------------------------------------------------------
/**
 *  Structure used to keep track of a command being executed.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    pid_t pid;                          ///< PID of the child process.
    exec_ResultHandler_t handlerPtr;    ///< Pointer to be called when the child process exits.
    void* contextPtr;                   ///< Pointer to give to the handler function.
}
CommandInfo;




// TODO: Remove the handler singleton, and make more flexible.
//ProcessPool;
//ProcessHash;
// Info for the command currently being executed.
CommandInfo Current;




//--------------------------------------------------------------------------------------------------
/**
 *  Signal handler that's called when a child process finishes.
 */
//--------------------------------------------------------------------------------------------------
static void OnChildSignal
(
    int sigNum  ///< Id of the signal that fired.
)
//--------------------------------------------------------------------------------------------------
{
    pid_t pid;
    int status;

    LE_INFO("Child signal received.");

    // Get the result code from the child process.
    do
    {
        pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED);
    }
    while (   (pid < 0)
           && (errno == EINTR));

    LE_DEBUG("Server: Child PID %d, exit status: %d.", pid, status);

    // Now, if this is the child process we launched earlier, attempt to call the registered
    // callback.
    if (pid == Current.pid)
    {
        if (Current.handlerPtr != NULL)
        {
            Current.handlerPtr(status, Current.contextPtr);
        }

        Current.pid = -1;
        Current.handlerPtr = NULL;
        Current.contextPtr = NULL;
    }
}




//--------------------------------------------------------------------------------------------------
/**
 *  Redirect one of the given file descriptors to the one given to us by the calling process.  This
 *  function is only executed in the child process.
 */
//--------------------------------------------------------------------------------------------------
static void RedirectDescriptor
(
    int newDesc,  ///< The file descriptor we were given.  (If -1, then leave at default.)
    int origDesc  ///< The descriptor we're remapping.
)
//--------------------------------------------------------------------------------------------------
{
    if (   (newDesc != -1)
        && (newDesc != origDesc))
    {
        LE_DEBUG("CHILD: Remapping file descriptor, %d", origDesc);

        if (dup2(newDesc, origDesc) == -1)
        {
            LE_ERROR("CHILD: Could not redirect child's descriptor %d to %d.", origDesc, newDesc);
            return;
        }

        close(newDesc);
    }
}




//--------------------------------------------------------------------------------------------------
/**
 *  Initialize the Exec subsystem.
 */
//--------------------------------------------------------------------------------------------------
void exec_Init
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    le_sig_Block(SIGCHLD);
    le_sig_SetEventHandler(SIGCHLD, OnChildSignal);
}




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
)
//--------------------------------------------------------------------------------------------------
{
    Current.handlerPtr = resultHandler;
    Current.contextPtr = contextPtr;
    Current.pid = fork();

    if (Current.pid == 0)
    {
        log_ReInit();

        LE_DEBUG("Child: process started.");

        RedirectDescriptor(stdInDescriptor, STDIN_FILENO);
        RedirectDescriptor(stdOutDescriptor, STDOUT_FILENO);
        RedirectDescriptor(stdErrDescriptor, STDERR_FILENO);

        execv(commandStr, (char * const *)arguments);

        LE_FATAL("CHILD: Could not exec '%s'.  %m.\n", commandStr);
    }

    LE_DEBUG("PARENT: Started child process %d.", Current.pid);

    if (Current.pid < 0)
    {
        return LE_FAULT;
    }

    return LE_OK;
}
