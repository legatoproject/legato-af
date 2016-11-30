//--------------------------------------------------------------------------------------------------
/**
 * @file pipeline.c Implementation of the pipeline API.
 *
 * There are two classes of objects:
 * - Pipeline
 * - Process
 *
 * Each is allocated from its own pool.
 *
 * Pipelines are kept on a global list of pipelines.
 *
 * Processes are kept on a Pipeline's list of processes.
 *
 * Thread destructors are used to clean up everything if the client thread dies without deleting
 * pipelines first.
 *
 * Pub-sub event reporting is used to trigger reaping of child processes.
 * pipeline_CheckChildren() reports the event, and each pipeline registers a handler when it
 * starts. This ensures that all callbacks from pipelines happen under the correct thread.
 * It also ensures that all data structure access is single-threaded.
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "pipeline.h"
#include "fileDescriptor.h"


//--------------------------------------------------------------------------------------------------
/// Event ID for triggering reaping of dead child processes when SIGCHLD signals are received.
//--------------------------------------------------------------------------------------------------
static le_event_Id_t SignalEventId;


//--------------------------------------------------------------------------------------------------
/// Pipeline class
//--------------------------------------------------------------------------------------------------
typedef struct pipeline
{
    le_dls_Link_t link;             ///< Used to link onto the Pipeline List.
    pipeline_TerminationHandler_t terminationFunc;  ///< Termination callback (could be NULL).
    le_sls_List_t processList;      ///< List of processes in the pipeline (first to last order).
    le_thread_DestructorRef_t threadDestructor;     ///< Thread death destructor ref.
    int inputFd;        ///< File descriptor to use as the first process's standard input.
    int outputFd;       ///< File descriptor to use as the last process's standard output.
    le_thread_Ref_t attachedThread; ///< Reference to the thread that created this pipeline.
    le_event_HandlerRef_t eventHandler; ///< Ref to signal event handler (NULL if not started).
    size_t numRunningProcs;     ///< Number of processes running in this pipeline.
}
Pipeline_t;


//--------------------------------------------------------------------------------------------------
/// Pipeline memory pool.
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t PipelinePool;


//--------------------------------------------------------------------------------------------------
/// Process class
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_sls_Link_t link;     ///< Used to link into Pipeline_t's processList.
    pipeline_ProcessFunc_t func;    ///< Function to call in the child after setting up stdin/out.
    void* param;            ///< Parameter to pass to func when it is called in the child process.
    pid_t pid;              ///< Process ID of running process (0 if not running).
}
Process_t;


//--------------------------------------------------------------------------------------------------
/// Process memory pool.
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t ProcessPool;


//--------------------------------------------------------------------------------------------------
/**
 * Initialize the pipeline module.
 *
 * This should be called by liblegato's init.c module at start-up.
 */
//--------------------------------------------------------------------------------------------------
void pipeline_Init()
{
    SignalEventId = le_event_CreateId("PipelineSIGCHLD", 0);
    PipelinePool = le_mem_CreatePool("Pipeline", sizeof(Pipeline_t));
    ProcessPool = le_mem_CreatePool("PipelineProcess", sizeof(Process_t));
}


//--------------------------------------------------------------------------------------------------
/**
 * Delete a process object.
 *
 * @warning Assumes it has been removed from the Pipeline's process list.
 */
//--------------------------------------------------------------------------------------------------
static void DeleteProcess
(
    Process_t* processPtr
)
//--------------------------------------------------------------------------------------------------
{
    if (processPtr->pid != 0)
    {
        // Note: kill() could fail if the process already died (race).  We don't care.
        //       The important thing is that it's dead.
        kill(processPtr->pid, SIGKILL);

        // Wait (blocking) for the child to be dead and clean it up.
        pid_t result;
        do
        {
            result = waitpid(processPtr->pid, NULL, 0);
        }
        while ((result == -1) && (errno == EINTR));

        if (result == -1)
        {
            LE_CRIT("waitpid() failed for pid %d (%m)", processPtr->pid);
        }
    }

    le_mem_Release(processPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Delete a pipeline object.  Assumes the thread destructor has already been deleted.
 */
//--------------------------------------------------------------------------------------------------
static void DeletePipeline
(
    Pipeline_t* pipelinePtr
)
//--------------------------------------------------------------------------------------------------
{
    LE_ASSERT(pipelinePtr->threadDestructor == NULL);

    // Free the signal event handler, if there is one.
    if (pipelinePtr->eventHandler != NULL)
    {
        le_event_RemoveHandler(pipelinePtr->eventHandler);
    }

    // Pop each process off of the process list, send it a SIGKILL if it's not dead yet, and
    // delete its process object.
    le_sls_Link_t* linkPtr;
    while (NULL != (linkPtr = le_sls_Pop(&pipelinePtr->processList)))
    {
        Process_t* processPtr = CONTAINER_OF(linkPtr, Process_t, link);

        DeleteProcess(processPtr);
    }

    // Close the input and output file descriptors.
    if (pipelinePtr->inputFd != -1)
    {
        fd_Close(pipelinePtr->inputFd);
        pipelinePtr->inputFd = -1;
    }
    if (pipelinePtr->outputFd != -1)
    {
        fd_Close(pipelinePtr->outputFd);
        pipelinePtr->outputFd = -1;
    }

    // Delete the pipeline object.
    le_mem_Release(pipelinePtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Function called for each pipeline if the thread is dying.  Deletes the pipeline.
 */
//--------------------------------------------------------------------------------------------------
static void ThreadDeathHandler
(
    void* pipeline
)
//--------------------------------------------------------------------------------------------------
{
    DeletePipeline(pipeline);
}


//--------------------------------------------------------------------------------------------------
/**
 * Copies one open file descriptor to another specific file descriptor number.
 */
//--------------------------------------------------------------------------------------------------
static void CopyFd
(
    int src,     ///<[IN] Source file descriptor.
    int dest     ///<[IN] Destination file descriptor.
)
//--------------------------------------------------------------------------------------------------
{
    int r;

    do
    {
        r = dup2(src, dest);
    }
    while ((r == -1) && (errno == EINTR));

    LE_FATAL_IF(r == -1, "dup2(%d, %d) failed: %m.", src, dest);
}


//--------------------------------------------------------------------------------------------------
/**
 * Create a new pipeline.
 */
//--------------------------------------------------------------------------------------------------
pipeline_Ref_t pipeline_Create
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    Pipeline_t* pipelinePtr = le_mem_ForceAlloc(PipelinePool);

    pipelinePtr->processList = LE_SLS_LIST_INIT;
    pipelinePtr->terminationFunc = NULL;
    pipelinePtr->inputFd = -1;
    pipelinePtr->outputFd = -1;
    pipelinePtr->eventHandler = NULL;
    pipelinePtr->numRunningProcs = 0;

    // Register a thread destructor to be called to clean up this pipeline if the thread dies.
    pipelinePtr->threadDestructor = le_thread_AddDestructor(ThreadDeathHandler, pipelinePtr);

    // Remember the thread that created this pipeline.
    pipelinePtr->attachedThread = le_thread_GetCurrent();

    return pipelinePtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Delete a pipeline.
 *
 * If the processes are still running, force kills them (using SIGKILL).
 *
 * Any data left in pipes will be lost.
 */
//--------------------------------------------------------------------------------------------------
void pipeline_Delete
(
    pipeline_Ref_t pipeline
)
//--------------------------------------------------------------------------------------------------
{
    LE_FATAL_IF(pipeline->attachedThread != le_thread_GetCurrent(),
                "Thread '%s' attempted to delete pipeline created by another thread.",
                le_thread_GetMyName());

    le_thread_RemoveDestructor(pipeline->threadDestructor);
    pipeline->threadDestructor = NULL;

    DeletePipeline(pipeline);
}


//--------------------------------------------------------------------------------------------------
/**
 * Adds a process to the end of the pipeline.
 *
 * @warning Be sure that param is not a pointer to something that will get deallocated before
 *          the process starts.
 */
//--------------------------------------------------------------------------------------------------
void pipeline_Append
(
    pipeline_Ref_t pipeline,
    pipeline_ProcessFunc_t func,    ///< Function to be called in the child process.
    void* param                     ///< Parameter to pass to the func when it is called.
)
//--------------------------------------------------------------------------------------------------
{
    Process_t* processPtr = le_mem_ForceAlloc(ProcessPool);

    processPtr->link = LE_SLS_LINK_INIT;
    processPtr->func = func;
    processPtr->param = param;
    processPtr->pid = 0;

    le_sls_Queue(&pipeline->processList, &processPtr->link);
}


//--------------------------------------------------------------------------------------------------
/**
 * Provides a file descriptor for the pipeline to read its input from.
 */
//--------------------------------------------------------------------------------------------------
void pipeline_SetInput
(
    pipeline_Ref_t pipeline,
    int fd
)
//--------------------------------------------------------------------------------------------------
{
    // Create a duplicate of this fd so that we can close it after launching the first process
    // in the pipeline (otherwise things get complicated elsewhere).
    pipeline->inputFd = dup(fd);
    LE_FATAL_IF(pipeline->inputFd == -1, "dup(%d) failed (%m)", fd);
}


//--------------------------------------------------------------------------------------------------
/**
 * Provides a file descriptor for the pipeline to write its output to.
 */
//--------------------------------------------------------------------------------------------------
void pipeline_SetOutput
(
    pipeline_Ref_t pipeline,
    int fd
)
//--------------------------------------------------------------------------------------------------
{
    // Create a duplicate of this fd so that we can close it after launching the last process
    // in the pipeline (otherwise things get complicated elsewhere).
    pipeline->outputFd = dup(fd);
    LE_FATAL_IF(pipeline->outputFd == -1, "dup(%d) failed (%m)", fd);
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates a pipe for the first process in the pipeline to read from and returns the write end
 * of the pipe.
 *
 * @return The file descriptor of the write end of the pipe.
 *
 * @warning Remember to close the write end of the pipe when you are done.  The read end will
 *          be closed automatically.
 */
//--------------------------------------------------------------------------------------------------
int pipeline_CreateInputPipe
(
    pipeline_Ref_t pipeline
)
//--------------------------------------------------------------------------------------------------
{
    // Create a pipe and make the read end the place that the pipeline will read its input from.
    int writeFd;
    pipeline_CreatePipe(&pipeline->inputFd, &writeFd);

    return writeFd;
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates a pipe for the last process in the pipeline to write into and returns the read end
 * of the pipe.
 *
 * @return The file descriptor of the read end of the pipe.
 *
 * @warning Remember to close the read end of the pipe when you are done.  The write end will
 *          be closed automatically.
 */
//--------------------------------------------------------------------------------------------------
int pipeline_CreateOutputPipe
(
    pipeline_Ref_t pipeline
)
//--------------------------------------------------------------------------------------------------
{
    // Create a pipe and make the write end the place that the pipeline will write its output to.
    int readFd;
    pipeline_CreatePipe(&readFd, &pipeline->outputFd);

    return readFd;
}


//--------------------------------------------------------------------------------------------------
/**
 * Create a pipe.
 */
//--------------------------------------------------------------------------------------------------
void pipeline_CreatePipe
(
    int* readFdPtr,     ///< [out] Ptr to where the fd for the read end of the pipe will be put.
    int* writeFdPtr     ///< [out] Ptr to where the fd for the write end of the pipe will be put.
)
//--------------------------------------------------------------------------------------------------
{
    int fds[2];

    LE_FATAL_IF((pipe(fds) == -1), "Can't create pipe. errno: %d (%m)", errno);

    *readFdPtr = fds[0];
    *writeFdPtr = fds[1];
}


//--------------------------------------------------------------------------------------------------
/**
 * Move a file descriptor to a specific fd number.  Does nothing if src and dest are the same.
 */
//--------------------------------------------------------------------------------------------------
static void MoveFd
(
    int src,     ///<[IN] Source file descriptor (will be closed, unless dest is the same).
    int dest     ///<[IN] Destination file descriptor.
)
//--------------------------------------------------------------------------------------------------
{
    if (src != dest)
    {
        CopyFd(src, dest);

        fd_Close(src);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Fork a child process.
 *
 * In the child, sets up stdin and stdout, and calls the process function.
 * Will not return in the child process.
 *
 * In the parent process, stores the child's process ID (pid) in the Process object and returns.
 **/
//--------------------------------------------------------------------------------------------------
static void Fork
(
    Process_t* processPtr,
    int inFd,
    int outFd
)
//--------------------------------------------------------------------------------------------------
{
    processPtr->pid = fork();

    LE_FATAL_IF(processPtr->pid == -1, "Can't create child process, errno: %d (%m)", errno);

    if (processPtr->pid == 0)   // ** CHILD **
    {
        // Clear the signal mask so the child does not inherit our signal mask.
        sigset_t sigSet;
        LE_FATAL_IF(sigfillset(&sigSet) == -1, "Can't fill sigset. %m");
        LE_FATAL_IF(pthread_sigmask(SIG_UNBLOCK, &sigSet, NULL) != 0, "Can't unblock signals");

        // Move the inFd to stdin.
        MoveFd(inFd, STDIN_FILENO);

        // Move the outFd to stdout.
        MoveFd(outFd, STDOUT_FILENO);

        // Call the process's function.
        // If it returns, exit with its return value as the exit code.
        exit(processPtr->func(processPtr->param));
    }

    // ** PARENT ** - Just return.
}



//--------------------------------------------------------------------------------------------------
/**
 * Open /dev/null.
 *
 * @return fd
 **/
//--------------------------------------------------------------------------------------------------
static int OpenDevNull
(
    int flags
)
//--------------------------------------------------------------------------------------------------
{
    int fd;

    do
    {
        fd = open("/dev/null", flags);
    }
    while ((fd == -1) && (errno == EINTR));

    LE_FATAL_IF(fd == -1, "Failed to open /dev/null (%m)");

    return fd;
}


//--------------------------------------------------------------------------------------------------
/**
 * Handle the death of a child process in a pipeline.
 *
 * @return
 * - LE_OK if it is safe to continue using the pipeline.
 * - LE_TERMINATED if the pipeline has terminated and should not be accessed again.
 **/
//--------------------------------------------------------------------------------------------------
static le_result_t HandleDeadChild
(
    Pipeline_t* pipelinePtr,
    Process_t* processPtr,
    int status  // Status code returned by waitpid()
)
//--------------------------------------------------------------------------------------------------
{
    pid_t pid = processPtr->pid;

    // Mark the process dead so we don't try reaping it again.
    processPtr->pid = 0;

    // Decrement the count of processes that are still running in this pipeline.
    pipelinePtr->numRunningProcs--;

    // If there are no more running processes in this pipeline, deregister the signal event handler.
    if (pipelinePtr->numRunningProcs == 0)
    {
        // Remove the signal event handler.
        le_event_RemoveHandler(pipelinePtr->eventHandler);
        pipelinePtr->eventHandler = NULL;
    }

    // Gather diagnostics for the logs.
    if (WIFEXITED(status) && (WEXITSTATUS(status) == 0))
    {
        LE_DEBUG("Pipeline child process %d successfully completed its task.", pid);
    }
    else
    {
        if (WIFEXITED(status))
        {
            LE_DEBUG("Pipeline child process %d exited with code %d.", pid, WEXITSTATUS(status));
        }
        else
        {
            if (WIFSIGNALED(status))
            {
                LE_DEBUG("Pipeline child process %d killed by signal %d.", pid, WTERMSIG(status));
            }
            else
            {
                LE_CRIT("Unknown failure reason for pipeline child process %d.", pid);
            }
        }
    }

    // If the last process in the pipeline has just died, report termination of the pipeline.
    le_sls_Link_t* lastLinkPtr = le_sls_PeekTail(&pipelinePtr->processList);
    if (processPtr == CONTAINER_OF(lastLinkPtr, Process_t, link))
    {
        // WARNING: Calling the termination function may result in the pipeline being
        //          deleted by the client's termination function.  So, calling the termination
        //          function MUST be the last thing we do with the pipeline.
        if (pipelinePtr->terminationFunc != NULL)
        {
            pipelinePtr->terminationFunc(pipelinePtr, status);
            return LE_TERMINATED;
        }
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function that gets called for each pipeline that has running processes when a signal event
 * is reported.
 */
//--------------------------------------------------------------------------------------------------
static void SignalEventHandler
(
    void* unused
)
//--------------------------------------------------------------------------------------------------
{
    Pipeline_t* pipelinePtr = le_event_GetContextPtr();

    // For each process in the pipeline's list of processes.
    le_sls_Link_t* processLinkPtr;
    for (processLinkPtr = le_sls_Peek(&pipelinePtr->processList);
         processLinkPtr != NULL;
         processLinkPtr = le_sls_PeekNext(&pipelinePtr->processList, processLinkPtr))
    {
        Process_t* processPtr = CONTAINER_OF(processLinkPtr, Process_t, link);

        // If the process has been started and not yet reaped,
        if (processPtr->pid != 0)
        {
            // Check the state of the process and reap if dead.
            int status;
            pid_t result = waitpid(processPtr->pid, &status, WNOHANG);

            if (result == -1)
            {
                if (errno == ECHILD)
                {
                    LE_CRIT("Child with pid %d vanished!", processPtr->pid);
                }
                else
                {
                    LE_FATAL("waitpid(%d, &status, WNOHANG) failed (%m).", processPtr->pid);
                }
            }
            else if (   (result == processPtr->pid) // Process may have died.
                     && (!WIFSTOPPED(status))       // But, this is not death.
                     && (!WIFCONTINUED(status))  )  // Neither is this.
            {
                // WARNING: the pipeline object may be completely gone when ReapChild() returns
                //          if the client's pipeline termination function is called and it calls
                //          pipeline_Delete().
                if (HandleDeadChild(pipelinePtr, processPtr, status) == LE_TERMINATED)
                {
                    // The termination function was called.  Must stop accessing the pipeline now.
                    return;
                }
            }
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Executes the processes in the pipeline.  A completion callback function is
 * provided, which will be called when the pipeline terminates.
 */
//--------------------------------------------------------------------------------------------------
void pipeline_Start
(
    pipeline_Ref_t pipeline,
    pipeline_TerminationHandler_t callback
)
//--------------------------------------------------------------------------------------------------
{
    LE_FATAL_IF(pipeline->eventHandler != NULL, "Pipeline already started.");
    LE_FATAL_IF(pipeline->attachedThread != le_thread_GetCurrent(),
                "Thread '%s' attempted to start pipeline created by another thread.",
                le_thread_GetMyName());

    pipeline->terminationFunc = callback;

    // Register the signal event handler so we get notified if a SIGCHLD is received
    // and we can go check for dead children in this pipeline.
    pipeline->eventHandler = le_event_AddHandler("Pipeline", SignalEventId, SignalEventHandler);
    le_event_SetContextPtr(pipeline->eventHandler, pipeline);

    int nextInFd = pipeline->inputFd;   // fd to use as the input of the next process.

    // If the pipeline doesn't have an input fd, open /dev/null to use.
    if (nextInFd == -1)
    {
        nextInFd = OpenDevNull(O_RDONLY);
    }

    // Walk the process list, from front to back, starting the processes with the appropriate
    // fds for their stdin and stdout.
    le_sls_Link_t* linkPtr = le_sls_Peek(&pipeline->processList);

    // A pipeline must have at least one process.
    LE_FATAL_IF(linkPtr == NULL, "Pipeline has no processes.");

    for ( ; linkPtr != NULL;
            linkPtr = le_sls_PeekNext(&pipeline->processList, linkPtr))
    {
        Process_t* processPtr = CONTAINER_OF(linkPtr, Process_t, link);

        int inFd = nextInFd;   // fd to use as the process's stdin
        int outFd;    // fd to use as the process's stdout

        // If this is the last process in the list,
        // then its output is the pipeline's output.
        if (le_sls_IsTail(&pipeline->processList, linkPtr))
        {
            outFd = pipeline->outputFd;

            // If the pipeline doesn't have an output fd, open /dev/null to use.
            if (outFd == -1)
            {
                outFd = OpenDevNull(O_WRONLY);
            }
        }
        // if there's another process after this one in the pipeline,
        // we need a pipe to connect the output of this process to the input of the next.
        else
        {
            pipeline_CreatePipe(&nextInFd, &outFd);
        }

        // Fork the process.
        Fork(processPtr, inFd, outFd);

        // Close our copy of the file descriptors that the child is now using for stdin and stdout.
        fd_Close(inFd);
        fd_Close(outFd);

        pipeline->numRunningProcs++;
    }

    // Clear the pipeline's inputFd and outputFd so they don't get closed elsewhere.
    // They are the child process(s) responsibilities now.
    pipeline->inputFd = -1;
    pipeline->outputFd = -1;
}


//--------------------------------------------------------------------------------------------------
/**
 * Check all pipelines started by threads in this process for the death of any of its children.
 *
 * If it finds one or more have died, then it will reap them and report results through
 * completion callbacks.
 *
 * This does nothing if there are no pipelines running.
 *
 * @note It's up to the caller to get the SIGCHLD notification somehow (it is recommended to
 *       use the @ref c_signals).
 *
 * @warning We don't know what thread is calling this function.
 */
//--------------------------------------------------------------------------------------------------
void pipeline_CheckChildren
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    le_event_Report(SignalEventId, NULL, 0);
}
