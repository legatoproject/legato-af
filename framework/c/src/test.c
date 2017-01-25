//--------------------------------------------------------------------------------------------------
/** @file test.c
 *
 * Implements the Legato Test Framework.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"


/// Maximum number of arguments that can be passed to a child process.
#define MAX_CHILD_ARGS 255


/// If child unit test processes are used, this is the default number of children.
#define DEFAULT_NUM_CHILDREN 23  // NOTE: 23 is pretty arbitrary.


//--------------------------------------------------------------------------------------------------
/**
 * The number of test failures.  This may be accessed by multiple threads but since an int is atomic
 * we do not need to protect it with a mutex.
 */
//--------------------------------------------------------------------------------------------------
static int NumFailures = 0;


//--------------------------------------------------------------------------------------------------
/**
 * Controls whether the process exits when there is a failure or if the number of failures is
 * incremented and the testing continues (pass through).
 */
//--------------------------------------------------------------------------------------------------
static bool PassThrough = false;


//--------------------------------------------------------------------------------------------------
/**
 * The pass through argument strings.
 */
//--------------------------------------------------------------------------------------------------
static const char PassThroughArg[] = "-p";
static const char PassThroughArgLongForm[] = "--pass-through";


//--------------------------------------------------------------------------------------------------
/**
 * Mutex for protect multithreaded access to our data.
 */
//--------------------------------------------------------------------------------------------------
static le_mutex_Ref_t Mutex;


//--------------------------------------------------------------------------------------------------
/**
 * Lock the mutex, or fail with a helpful message if they forgot to run LE_TEST_INIT.
 **/
//--------------------------------------------------------------------------------------------------
static void Lock
(
    void
)
{
    if (Mutex == NULL)
    {
        LE_FATAL("You forgot to initialize the unit test framework with LE_TEST_INIT.");
    }
    le_mutex_Lock(Mutex);
}

/// Lock the mutex.
#define LOCK    Lock();

/// Unlock the mutex.
#define UNLOCK  le_mutex_Unlock(Mutex);


//--------------------------------------------------------------------------------------------------
/**
 * Represents a single child unit test process that was created using LE_TEST_FORK().
 **/
//--------------------------------------------------------------------------------------------------
typedef struct
{
    pid_t pid;                  ///< Process ID.
    char exePath[PATH_MAX];     ///< The executable's file system path.
    void* safeRef;              ///< Safe reference for this object.
}
Child_t;


//--------------------------------------------------------------------------------------------------
/**
 * Safe reference map used for references to Child objects.
 **/
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t ChildRefMap = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Pool from which Child objects are allocated.
 **/
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t ChildPool = NULL;



//--------------------------------------------------------------------------------------------------
/**
 * Initialize the child process tracking data structures, if that hasn't been done already.
 *
 * Most processes don't use this stuff, so we defer this until needed so we don't eat CPU and
 * memory for this stuff unless we need to.
 **/
//--------------------------------------------------------------------------------------------------
static void InitChildTracking
(
    void
)
{
    LOCK

    if (ChildRefMap == NULL)
    {
        // Get a reference to the Child Pool.
        ChildPool = le_mem_CreatePool("unitTestChildren", sizeof(Child_t));

        // Check how many blocks the Child Pool can hold.
        size_t poolSize = le_mem_GetObjectCount(ChildPool);

        // Make sure the Child Pool can hold at least DEFAULT_NUM_CHILDREN children.
        if (poolSize < DEFAULT_NUM_CHILDREN)
        {
            le_mem_ExpandPool(ChildPool, DEFAULT_NUM_CHILDREN - poolSize);
            poolSize = DEFAULT_NUM_CHILDREN;
        }

        // Make the safe reference map.
        ChildRefMap = le_ref_CreateMap("unitTestChildren", poolSize);
    }

    UNLOCK
}


//--------------------------------------------------------------------------------------------------
/**
 * Initializes the Legato test framework.  This should be called once at the beginning of test
 * process.
 */
//--------------------------------------------------------------------------------------------------
void _le_test_Init
(
    void
)
{
    Mutex = le_mutex_CreateNonRecursive("UnitTestMutex");

    NumFailures = 0;

    PassThrough = false;

    // Check the command line arguments for the PassThroughArgs strings.
    int i = le_arg_NumArgs() - 1;
    for (; i >= 0; i--)
    {
        const char* argPtr = le_arg_GetArg(i);

        if (   (argPtr != NULL)
            && (   (strcmp(argPtr, PassThroughArg) == 0)
                || (strcmp(argPtr, PassThroughArgLongForm) == 0) ) )
        {
            PassThrough = true;
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Called when a test fails.  Logs an error message and either exits right away or increments the
 * number of failures.
 */
//--------------------------------------------------------------------------------------------------
void _le_test_Fail
(
    void
)
{
    if (PassThrough)
    {
        LOCK

        NumFailures++;

        UNLOCK
    }
    else
    {
        exit(EXIT_FAILURE);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Returns the number of test failures.
 */
//--------------------------------------------------------------------------------------------------
int _le_test_GetNumFailures
(
    void
)
{
    return NumFailures;
}


//--------------------------------------------------------------------------------------------------
/**
 * Fork a child process to run a unit test program.
 *
 * @return A reference to the child process.
 *
 * @note Terminates the calling process on failure to fork.
 **/
//--------------------------------------------------------------------------------------------------
le_test_ChildRef_t _le_test_Fork
(
    const char* exePath,    ///< [in] String containing the path to the program to be executed.
    ...                     ///< [in] A variable list of subsequent parameters is allowed, each
                            ///       of which will be passed as a command-line argument to the
                            ///       child program when it is executed.
)
{
    LE_ASSERT(exePath != NULL);
    LE_ASSERT(exePath[0] != '\0');

    // Make sure the child process tracking data structures have been initialized.
    InitChildTracking();

    // Create a Child object to track the new child process.
    Child_t* childPtr = le_mem_ForceAlloc(ChildPool);
    if (le_utf8_Copy(childPtr->exePath, exePath, sizeof(childPtr->exePath), NULL) != LE_OK)
    {
        LE_FATAL("Path too long ('%s' longer than %u bytes).", exePath, PATH_MAX);
    }
    childPtr->pid = -1;
    childPtr->safeRef = le_ref_CreateRef(ChildRefMap, childPtr);

    // Construct a list of arguments to pass to the child program.
    char* argList[MAX_CHILD_ARGS];
    va_list varArgs;

    va_start(varArgs, exePath);

    argList[0] = childPtr->exePath; // First arg is the program name.
    size_t i;
    for (i = 1; i < MAX_CHILD_ARGS; i++)
    {
        argList[i] = va_arg(varArgs, char*);

        if (argList[i] == NULL)
        {
            break;
        }
    }

    va_end(varArgs);

    // Fork the child process.
    int pid = fork();
    if (pid < 0)
    {
        LE_FATAL("Failed to fork child process '%s' (%m).", exePath);
    }
    else if (pid == 0)
    {
        // I'm the child process.

        // WARNING: Do not try to use anything that might not be stable on the other side of a
        //          fork.  Just exec.

        // Launch the executable!
        execvp(exePath, argList);

        LE_FATAL("Failed to exec '%s' (%m).", exePath);
    }
    else
    {
        // I'm the parent.

        LE_INFO("Forked child with pid %d to run executable '%s'.", pid, exePath);

        // Remember the child process's PID.
        childPtr->pid = pid;

        // Return the Child object's safe reference.
        return childPtr->safeRef;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Wait for a given child process to terminate and add its results to the running test summary.
 **/
//--------------------------------------------------------------------------------------------------
void _le_test_Join
(
    le_test_ChildRef_t child ///< [IN] Child process reference returned by LE_TEST_FORK().
)
{
    Child_t* childPtr = le_ref_Lookup(ChildRefMap, child);
    size_t numFailures = 0;

    if (childPtr == NULL)
    {
        LE_FATAL("Invalid child process reference %p.", child);
    }

    // Keep waiting for child status updates until the child terminates.
    int childStatus;
    do
    {
        pid_t pid;

        // Keep trying to wait for child process state changes until we can get some info
        // without getting interrupted by a signal.
        do
        {
            pid = waitpid(childPtr->pid, &childStatus, 0);

        } while ((pid == -1) && (errno == EINTR));

        LE_FATAL_IF(pid == -1,
                    "waitpid() failed for PID %d (%s) (%m).",
                    childPtr->pid,
                    childPtr->exePath);

        // Check the child's status info and act accordingly.
        if (WIFEXITED(childStatus)) // Did the child process exit?
        {
            // Get the child process's exit code.
            int exitCode = WEXITSTATUS(childStatus);

            LE_INFO("Child with PID %d (%s) exited with result %d.",
                    childPtr->pid,
                    childPtr->exePath,
                    exitCode);

            // A negative exit code is counted as one failure.
            if (exitCode < 0)
            {
                numFailures = 1;
            }
            else
            {
                numFailures = exitCode;
            }
        }
        else if (WIFSIGNALED(childStatus)) // Was the child process killed by a signal?
        {
            LE_ERROR("Child with PID %d (%s) killed by signal %d.",
                     childPtr->pid,
                     childPtr->exePath,
                     WTERMSIG(childStatus));

            // Death by signal counts as one failure.
            numFailures = 1;
        }
        else if (WIFSTOPPED(childStatus))    // Was the child stopped?
        {
            LE_INFO("Child with PID %d (%s) stopped by signal %d.",
                    childPtr->pid,
                    childPtr->exePath,
                    WSTOPSIG(childStatus));
        }
        else if (WIFCONTINUED(childStatus))  // Was the child resumed?
        {
            LE_INFO("Child with PID %d (%s) resumed.", childPtr->pid, childPtr->exePath);
        }

    } while (   ( ! WIFEXITED(childStatus) )
             && ( ! WIFSIGNALED(childStatus) ) );

    // Update the global tally of failures.
    LOCK
        NumFailures += numFailures;
    UNLOCK
}
