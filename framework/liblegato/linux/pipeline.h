//--------------------------------------------------------------------------------------------------
/**
 * @file pipeline.h API for creating pipes and pipelines of processes.
 *
 * The following code creates a pipeline "<c>cat foo | md5sum</c>" where the @c tar process reads
 * its data from the parent process's standard input and the output from the @c md5sum process is
 * delivered to an FD Monitor handler function called PipeOutputHandler() as it arrives.
 *
 * To make the completion callback work, the caller must monitor for SIGCHLD signals and call
 * pipeline_CheckChildren() when it arrives.
 *
 * @code
 *
 * static void ChildSignalHandler(int sigNum)
 * {
 *     pipeline_CheckChildren();
 * }
 *
 * static void CatProcess(void* param)
 * {
 *     const char* filePath = param;
 *
 *     execl("cat", filePath, (char*)NULL);
 * }
 *
 * static void Md5SumProcess(void)
 * {
 *     execl("md5sum", "md5sum", (char*)NULL);
 * }
 *
 * static void CompletionCallback
 * (
 *     pipeline_Ref_t pipeline,
 *     int result // Exit status of the last process in the pipeline (see 'man waitpid').
 * )
 * {
 *     if (WIFEXITED(result) && WEXITSTATUS(EXIT_SUCCESS))
 *     {
 *         // Hooray!
 *     }
 *     else
 *     {
 *         // Boo!  Killed by a signal or exited with non-zero exit code.
 *     }
 *     pipeline_Delete(pipeline);
 * }
 *
 * static void RunPipeline()
 * {
 *     pipeline_PipelineRef_t pipeline = pipeline_Create();
 *
 *     pipeline_Append(pipeline, CatProcess, "foo");
 *
 *     pipeline_Append(pipeline, Md5SumProcess, NULL);
 *
 *     pipeline_SetInput(pipeline, STDIN_FILENO);
 *
 *     fdMonitor_Create("my pipeline", pipeline_CreateOutputPipe(pipeline), PipeOutputHandler, POLLIN);
 *
 *     pipeline_Start(pipeline, CompletionCallback);
 * }
 *
 * COMPONENT_INIT
 * {
 *     le_sig_Block(SIGCHLD);
 *     le_sig_SetEventHandler(SIGCHLD, ChildSignalHandler);
 *
 *     RunPipeline();
 * }
 *
 * @endcode
 *
 * <hr>
 *
 * Summary of available functions in this API:
 *
 * - pipeline_Create() creates a new pipeline.
 *
 * - pipeline_Delete() deletes a pipeline.  Kills all remaining processes in the pipeline.
 *
 * - pipeline_Append() adds a process to the end of the pipeline.
 *
 * - pipeline_SetInput() provides a file descriptor for the pipeline to read its input from.
 *
 * - pipeline_CreateInputPipe() creates a pipe to be connected to the input of the first process
 *   in the pipeline.  Returns the write end for the caller to use to write data into the pipe.
 *
 * - pipeline_SetOutput() provides a file descriptor for the pipeline to write its output to.
 *
 * - pipeline_CreateOutputPipe() creates a pipe to be connected to the output of the last process
 *   in the pipeline.  Returns the read end for the caller to use to read data from the pipe.
 *
 * - pipeline_CreatePipe() can be used to create a pipe.  It's a simple wrapper around pipe().
 *
 * - pipeline_Start() executes the processes in the pipeline.  A completion callback function is
 *   provided, which will be called when the pipeline terminates.
 *
 * - pipeline_CheckChildren() must be called to tell the pipeline to check for the death
 *   of any of its children.  This should be done in response to a SIGCHLD signal.  It is
 *   recommended to use the @ref c_signals to detect SIGCHLD signals.  DO NOT call any of this
 *   API's functions from inside an asynchronous signal handler.  This API is NOT async safe.
 *
 * <b>Future Enhancements:</b>
 *
 * - pipeline_SetTimeout()
 * - pipeline_Set/GetContextPtr()
 * - pipeline_Signal() - Send a signal to all processes in the pipeline.
 * - pipeline_Prepend()
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_PIPELINE_H_INCLUDE_GUARD
#define LEGATO_PIPELINE_H_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Reference to a pipeline.
 */
//--------------------------------------------------------------------------------------------------
typedef struct pipeline* pipeline_Ref_t;


//--------------------------------------------------------------------------------------------------
/**
 * Create a new pipeline.
 */
//--------------------------------------------------------------------------------------------------
pipeline_Ref_t pipeline_Create
(
    void
);


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
);


//--------------------------------------------------------------------------------------------------
/**
 * Process functions look like this.  These are the functions that run in the child process.
 * They can do whatever is needed in that process, e.g., setuid(), setgid(), execl(), or just
 * do something and exit.
 *
 * Standard input and standard output will be properly set up for the pipeline before this
 * function is called.
 *
 * If the function returns, the process will exit.
 *
 * @param Parameter passed to pipeline_Append() for this process.
 *
 * @return Exit code for the process.
 *
 * @warning The Legato app framework is not fully fork-safe yet, so it is recommended that
 *          the process exec a program, unless it just has something trivial to do using POSIX APIs.
 */
//--------------------------------------------------------------------------------------------------
typedef int (*pipeline_ProcessFunc_t)
(
    void* param
);


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
);


//--------------------------------------------------------------------------------------------------
/**
 * Provides a file descriptor for the pipeline to read its input from.
 */
//--------------------------------------------------------------------------------------------------
void pipeline_SetInput
(
    pipeline_Ref_t pipeline,
    int fd
);


//--------------------------------------------------------------------------------------------------
/**
 * Provides a file descriptor for the pipeline to write its output to.
 */
//--------------------------------------------------------------------------------------------------
void pipeline_SetOutput
(
    pipeline_Ref_t pipeline,
    int fd
);


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
);


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
);


//--------------------------------------------------------------------------------------------------
/**
 * Create a pipe.
 */
//--------------------------------------------------------------------------------------------------
void pipeline_CreatePipe
(
    int* readFdPtr,     ///< [out] Ptr to where the fd for the read end of the pipe will be put.
    int* writeFdPtr     ///< [out] Ptr to where the fd for the write end of the pipe will be put.
);


//--------------------------------------------------------------------------------------------------
/**
 * Pipeline termination handler functions must look like this.
 *
 * @param pipeline Reference to the pipeline that terminated.
 *
 * @param status Status code from the last process in the pipeline.  Must be processed using
 *               WIFEXITED(), WEXITSTATUS(), WIFSIGNALED(), WTERMSIG(), etc.  See 'man waitpid'.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*pipeline_TerminationHandler_t)
(
    pipeline_Ref_t pipeline,
    int status
);


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
);


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
 */
//--------------------------------------------------------------------------------------------------
void pipeline_CheckChildren
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Initialize the pipeline module.
 *
 * This should be called by liblegato's init.c module at start-up.
 */
//--------------------------------------------------------------------------------------------------
void pipeline_Init();


#endif // LEGATO_PIPELINE_H_INCLUDE_GUARD
