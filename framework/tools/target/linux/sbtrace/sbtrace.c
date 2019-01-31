//--------------------------------------------------------------------------------------------------
/** @file sbtrace.c
 *
 * Tool to help import files a Legato sandboxed app requires.
 *
 * Dynamically determines which files an app needs by running the app and tracing the app's system
 * calls.  Assumes that the app is already installed.
 *
 * Must be run as root.
 *
 * @todo Add support for dynamically determining limits.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"
#include "sysPaths.h"
#include <sys/ptrace.h>
#include <unistd.h>


//--------------------------------------------------------------------------------------------------
/**
 * Structure to hold register values.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    // This is ARM specific.
    int regs[18];
}
Registers_t;


//--------------------------------------------------------------------------------------------------
/**
 * Tracee object.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    pid_t tid;                  ///< Thread ID of the tracee.
    bool inSyscall;             ///< Flag that indicates if the tracee has entered a system call.
    bool needInit;              ///< Flag that indicates whether the tracee must be initialized.
}
Tracee_t;


//--------------------------------------------------------------------------------------------------
/**
 * Pool of tracee objects.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t TraceePool = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Hash map of tracees.
 */
//--------------------------------------------------------------------------------------------------
static le_hashmap_Ref_t TraceeMap = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Estimated maximum number of tracees.
 */
//--------------------------------------------------------------------------------------------------
#define ESTIMATED_NUM_TRACEES               17


//--------------------------------------------------------------------------------------------------
/**
 * Object for system calls that access a file or directory.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    int sysCallNum;                 ///< The system call number.
    uint srcPathArgIndex;           ///< The argument index that contains the path to the file/dir.
    char sysCallName[50];           ///< The system call name.
}
FileAccessSysCall_t;


//--------------------------------------------------------------------------------------------------
/**
 * Array of system calls that access a file or directory.
 */
//--------------------------------------------------------------------------------------------------
static const FileAccessSysCall_t FileAccessSysCalls[] =
{
    {__NR_open,             0,      "open"},
    {__NR_creat,            0,      "creat"},
    {__NR_link,             0,      "link"},
    {__NR_unlink,           0,      "unlink"},
    {__NR_execve,           0,      "execve"},
    {__NR_chdir,            0,      "chdir"},
    {__NR_mknod,            0,      "mknod"},
    {__NR_chmod,            0,      "chmod"},
    {__NR_lchown,           0,      "lchown"},
    {__NR_mount,            0,      "mount"},
    {__NR_access,           0,      "access"},
    {__NR_rename,           0,      "rename"},
    {__NR_mkdir,            0,      "mkdir"},
    {__NR_rmdir,            0,      "rmdir"},
    {__NR_acct,             0,      "acct"},
    {__NR_umount2,          0,      "umount2"},
    {__NR_chroot,           0,      "chroot"},
    {__NR_symlink,          0,      "symlink"},
    {__NR_readlink,         0,      "readlink"},
    {__NR_uselib,           0,      "uselib"},
    {__NR_swapon,           0,      "swapon"},
    {__NR_truncate,         0,      "truncate"},
    {__NR_statfs,           0,      "statfs"},
    {__NR_stat,             0,      "stat"},
    {__NR_lstat,            0,      "lstat"},
    {__NR_swapoff,          0,      "swapoff"},
    {__NR_quotactl,         1,      "quotactl"},
    {__NR_chown,            0,      "chown"},
    {__NR_setxattr,         0,      "setxattr"},
    {__NR_lsetxattr,        0,      "lsetxattr"},
    {__NR_getxattr,         0,      "getxattr"},
    {__NR_lgetxattr,        0,      "lgetxattr"},
    {__NR_listxattr,        0,      "listxattr"},
    {__NR_llistxattr,       0,      "llistxattr"},
    {__NR_removexattr,      0,      "removexattr"},
    {__NR_lremovexattr,     0,      "lremovexattr"},

#ifdef __NR_recvfrom
    {__NR_recvfrom,         4,      "recvfrom"},
#endif

#ifdef __NR_sendto
    {__NR_sendto,           4,      "sendto"},
#endif

#ifdef __NR_connect
    {__NR_connect,          1,      "connect"},
#endif

#ifdef __NR_bind
    {__NR_bind,             1,      "bind"},
#endif

#ifdef __NR_truncate64
    {__NR_truncate64,       0,      "truncate64"},
#endif

#ifdef __NR_stat64
    {__NR_stat64,           0,      "stat64"},
#endif

#ifdef __NR_lstat64
    {__NR_lstat64,          0,      "lstat64"},
#endif

#ifdef __NR_lchown32
    {__NR_lchown32,         0,      "lchown32"},
#endif

#ifdef __NR_chown32
    {__NR_chown32,          0,      "chown32"},
#endif

#ifdef __NR_statfs64
    {__NR_statfs64,         0,      "statfs64"},
#endif
};


//--------------------------------------------------------------------------------------------------
/**
 * Name of the application being traced.
 */
//--------------------------------------------------------------------------------------------------
static const char* AppNamePtr = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Application reference.
 */
//--------------------------------------------------------------------------------------------------
static le_appCtrl_AppRef_t AppRef = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Maximum path bytes.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_PATH_BYTES          (PATH_MAX + 1)


//--------------------------------------------------------------------------------------------------
/**
 * Apps working directory.
 */
//--------------------------------------------------------------------------------------------------
static char AppWorkingDir[MAX_PATH_BYTES] = APPS_WRITEABLE_DIR;


//--------------------------------------------------------------------------------------------------
/**
 * Timeout timer used for when there is no activities by any tracee.
 */
//--------------------------------------------------------------------------------------------------
static le_timer_Ref_t NoActivityTimer = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Flag that indicates whether to timeout when there is no activity by any tracee.
 */
//--------------------------------------------------------------------------------------------------
static bool CheckNoActivity = true;


//--------------------------------------------------------------------------------------------------
/**
 * Timer used for checking when the app has stopped.
 */
//--------------------------------------------------------------------------------------------------
static le_timer_Ref_t ShutdownTimer = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Added files list.
 */
//--------------------------------------------------------------------------------------------------
static le_sls_List_t AddedFilesList = LE_SLS_LIST_INIT;


//--------------------------------------------------------------------------------------------------
/**
 * Added devices list.
 */
//--------------------------------------------------------------------------------------------------
static le_sls_List_t AddedDevicesList = LE_SLS_LIST_INIT;


//--------------------------------------------------------------------------------------------------
/**
 * File object.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char path[MAX_PATH_BYTES];          ///< File's path.
    char permStr[3];                    ///< Permission string.
    le_sls_Link_t link;                 ///< Link in the list of added files.
}
File_t;


//--------------------------------------------------------------------------------------------------
/**
 * Memory pool of file objects.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t FilePool = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Path to use when creating the requires section file.
 */
//--------------------------------------------------------------------------------------------------
static const char* RequiresPathPtr = NULL;


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
 * Prints help to stdout and exits.
 */
//--------------------------------------------------------------------------------------------------
static void PrintHelp
(
    void
)
{
    puts(
        "NAME:\n"
        "    sbtrace - Used to help import files into a Legato app's sandbox.  Starts the specified\n"
        "              app and traces the app's system calls to dynamically determine the files\n"
        "              the app is trying to access.  Gives the user the option to allow or deny\n"
        "              access to files that are not already in the app's sandbox.  If the app\n"
        "              is granted access then the file is automatically imported into the app's\n"
        "              sandbox.\n"
        "              The access given to the app is temporary.  This tool does not modify the\n"
        "              app's installation or configuration settings.  However, a 'requires' section\n"
        "              can be generated that can be added to the app's adef/cdef.\n"
        "\n"
        "SYNOPSIS:\n"
        "    sbtrace <appName> [OPTION]...\n"
        "\n"
        "DESCRIPTION:\n"
        "    sbtrace <appName> [OPTIONS]\n"
        "       Starts tracing the specified application.\n"
        "\n"
        "OPTIONS:\n"
        "   --help, -h\n"
        "       Display this help and exit.\n"
        "\n"
        "   -o <PATH>, --output=<PATH>\n"
        "       Writes the 'requires' section to a file specified at PATH.\n"
        "\n"
        );

    exit(EXIT_SUCCESS);
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the path to use for creating the requires section file.
 */
//--------------------------------------------------------------------------------------------------
static void SetRequireFilePath
(
    const char* requiresPathPtr             ///< [IN] Path to the requires file to create.
)
{
    RequiresPathPtr = requiresPathPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the system call number from the registers.
 *
 * This function should be used rather than accessing the registers directly as it handles platform
 * specific differences.
 */
//--------------------------------------------------------------------------------------------------
static int GetSysCallNum
(
    const Registers_t* regPtr           ///< [IN] Set of register values.
)
{
    // This is ARM specific.
    return regPtr->regs[7];
}


//--------------------------------------------------------------------------------------------------
/**
 * Get system call arguments.
 *
 * This function should be used rather than accessing the registers directly as it handles platform
 * specific differences.
 *
 * @return
 *      Returns the arguments value, or the address to the argumnets for arguments that are passed
 *      by reference.
 */
//--------------------------------------------------------------------------------------------------
static int GetSysCallArg
(
    const Registers_t* regPtr,          ///< [IN] Set of register values.
    uint argIndex                       ///< [IN] Index of the argument to get.
)
{
#define MAX_ARGS        7

    if (argIndex < MAX_ARGS)
    {
        // This is ARM specific.
        return regPtr->regs[argIndex];
    }

    LE_FATAL("More than %d arguments is not supported.", MAX_ARGS);
}


//--------------------------------------------------------------------------------------------------
/**
 * Wrapper for ptrace() calls that only return a result code, ie. do not use this for
 * PTRACE_PEEK* calls.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NOT_FOUND if the tracee does not exist, is not being traced, or is not stopped.
 */
//--------------------------------------------------------------------------------------------------
static int Ptrace
(
    enum __ptrace_request request,
    pid_t pid,
    void *addr,
    void *data
)
{

    if (ptrace(request, pid, addr, data) == -1)
    {
        if (errno == ESRCH)
        {
            return LE_NOT_FOUND;
        }

        fprintf(stderr, "Error could not make ptrace() request %d.  %m.\n", request);
        exit(EXIT_FAILURE);
    }

    return LE_OK;
}


#if defined(__NR_recvfrom) && defined(__NR_sendto) && defined(__NR_connect) && defined(__NR_bind)

//--------------------------------------------------------------------------------------------------
/**
 * Reads bufSize number of bytes from the tracee's memory starting at addr.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if the address could not be read.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ReadTraceeBuf
(
    pid_t pid,                  ///< [IN] PID (actually the thread ID) of the tracee.
    int addr,                   ///< [IN] Address to start reading from.
    char* bufPtr,               ///< [OUT] Buffer to store the string in.
    size_t bufSize              ///< [IN] Size of the buffer.
)
{
    int i = 0;

    for (; i < bufSize; i++)
    {
        errno = 0;
        char c = ptrace(PTRACE_PEEKDATA, pid, addr + i, NULL);

        if ( (c == -1) && (errno != 0) )
        {
            if (errno == ESRCH)
            {
                return LE_FAULT;
            }

            INTERNAL_ERR("Could not read tracee's address.  %m.");
        }

        bufPtr[i] = c;
    }

    return LE_OK;
}

#endif

//--------------------------------------------------------------------------------------------------
/**
 * Reads a string of text starting from the specified address in the tracee's memory.
 *
 * @return
 *      LE_OK if successful.
 *      LE_OVERFLOW if the buffer is too small to hold the string.
 *      LE_FAULT if the address could not be read.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ReadTraceeStr
(
    pid_t pid,                  ///< [IN] PID (actually the thread ID) of the tracee.
    int addr,                   ///< [IN] Address to start reading from.
    char* bufPtr,               ///< [OUT] Buffer to store the string in.
    size_t bufSize              ///< [IN] Size of the buffer.
)
{
    int i = 0;

    while (i < bufSize-1)
    {
        errno = 0;
        char c = ptrace(PTRACE_PEEKDATA, pid, addr + i, NULL);

        if ( (c == -1) && (errno != 0) )
        {
            if (errno == ESRCH)
            {
                return LE_FAULT;
            }

            INTERNAL_ERR("Could not read tracee's address.  %m.");
        }

        if (c == 0)
        {
            bufPtr[i] = '\0';
            break;
        }

        bufPtr[i] = c;

        i++;
    }

    if (i >= bufSize-1)
    {
        bufPtr[bufSize-1] = '\0';
        return LE_OVERFLOW;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the path that the system call is trying to access.
 *
 * @return
 *      LE_OK if successful.
 *      LE_OVERFLOW if the buffer is too small to hold the path.
 *      LE_FAULT if the path could not be read.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetAccessPath
(
    pid_t pid,                          ///< [IN] PID of the tracee.
    Registers_t* regPtr,                ///< [IN] The tracee's set of registers when the system call
                                        ///       was made.
    const FileAccessSysCall_t* sysCallPtr, ///< [IN] The system call the tracee made.
    char* bufPtr,                       ///< [OUT] Buffer to hold the path.
    size_t bufSize                      ///< [IN] Buffer size.
)
{
    switch (sysCallPtr->sysCallNum)
    {
#if defined(__NR_recvfrom) && defined(__NR_sendto) && defined(__NR_connect) && defined(__NR_bind)
        case __NR_recvfrom:
        case __NR_sendto:
        case __NR_connect:
        case __NR_bind:
        {
            // Get the address of the struct sockaddr
            int sockAddr = GetSysCallArg(regPtr, sysCallPtr->srcPathArgIndex);

            // Get the address family.
            sa_family_t sun_family;

            if ( (ReadTraceeBuf(pid, sockAddr, (char*)&sun_family, sizeof(sun_family)) == LE_OK) &&
                 (sun_family == AF_UNIX) )
            {
                // Get the length of the sockaddr_un.
                socklen_t addrLen;

                int addrLenAddr = GetSysCallArg(regPtr, sysCallPtr->srcPathArgIndex + 1);

                // Check that this is not an unnamed socket.
                if ( (ReadTraceeBuf(pid, addrLenAddr, (char*)&addrLen, sizeof(addrLen)) == LE_OK) &&
                     (addrLen != sizeof(sa_family_t)) )
                {
                    // Get the start of the sun_path in sockaddr_un.
                    size_t pathAddr = sockAddr + offsetof(struct sockaddr_un, sun_path);

                    return ReadTraceeStr(pid, pathAddr, bufPtr, bufSize);
                }
            }
            return LE_FAULT;
        }
#endif

        default:
        {
            int pathAddr = GetSysCallArg(regPtr, sysCallPtr->srcPathArgIndex);
            return ReadTraceeStr(pid, pathAddr, bufPtr, bufSize);
        }
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Gets a the tracee object for the specified pid.  If the object does not exist it will becreated.
 *
 * @return
 *      Tracee object.
 */
//--------------------------------------------------------------------------------------------------
static Tracee_t* GetTracee
(
    pid_t pid               ///< [IN] PID of the tracee.
)
{
    // Get the tracee object.
    Tracee_t* traceePtr = le_hashmap_Get(TraceeMap, &pid);

    if (traceePtr == NULL)
    {
        // This is a new tracee, create an object for it.
        traceePtr = le_mem_ForceAlloc(TraceePool);

        traceePtr->tid = pid;
        traceePtr->inSyscall = false;
        traceePtr->needInit = true;

        le_hashmap_Put(TraceeMap, &(traceePtr->tid), traceePtr);
    }

    return traceePtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Attaches to the app's process.
 */
//--------------------------------------------------------------------------------------------------
static void AttachHandler
(
    le_appCtrl_AppRef_t appRef,         ///< [IN] App reference.
    int32_t pid,                        ///< [IN] PID of the process to attach to.
    const char* procNamePtr,            ///< [IN] Name of the process.
    void* contextPtr                    ///< [IN] Not used.
)
{
    LE_INFO("Attaching to process %d.", pid);

    // Attach to the process.
    if (Ptrace(PTRACE_ATTACH, pid, NULL, NULL) == LE_NOT_FOUND)
    {
        fprintf(stderr, "Error could not attach to %d.  %m.\n", pid);
        exit(EXIT_FAILURE);
    }

    // Create an object for this tracee.
    GetTracee(pid);

    // Request the supervisor to unblock the process.
    LE_INFO("Unblocking process %d.", pid);

    le_appCtrl_TraceUnblock(appRef, pid);
}


//--------------------------------------------------------------------------------------------------
/**
 * Stores the application name from the command line.
 */
//--------------------------------------------------------------------------------------------------
static void StoreAppName
(
    const char* appNamePtr              ///< [IN] App name from the command line.
)
{
    AppNamePtr = appNamePtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Start tracing the app.
 */
//--------------------------------------------------------------------------------------------------
static void StartAppTrace
(
    void
)
{
    printf("Tracing app '%s'\n\n", AppNamePtr);

    // Get the app's working directory.
    INTERNAL_ERR_IF(le_path_Concat("/", AppWorkingDir, sizeof(AppWorkingDir),
                                   AppNamePtr, NULL) != LE_OK,
                    "Directory path '%s...' is too long.", AppWorkingDir);

    // Connect to the service.
    le_result_t result = le_appCtrl_TryConnectService();

    if (result == LE_UNAVAILABLE)
    {
        fprintf(stderr, "le_appCtrl service is not available.  The current version of the Legato "
                        "framework does not support this service.");
        exit(EXIT_FAILURE);
    }
    else if (result == LE_COMM_ERROR)
    {
        fprintf(stderr, "le_appCtrl service is not available.  The Legato framework may not be "
                        "running.");
        exit(EXIT_FAILURE);
    }

    INTERNAL_ERR_IF(result == LE_NOT_PERMITTED,
                    "sbtrace is not bound to a le_appCtrl service.");

    // Stop the app if in case it is already running.
    le_appCtrl_Stop(AppNamePtr);

    // Get a reference to the app.
    AppRef = le_appCtrl_GetRef(AppNamePtr);

    if (AppRef == NULL)
    {
        fprintf(stderr, "App '%s' could not be started. Check logs for more info.\n", AppNamePtr);
        exit(EXIT_FAILURE);
    }

    // Set an attach handler.
    le_appCtrl_AddTraceAttachHandler(AppRef, AttachHandler, NULL);

    // Start the app.
    result = le_appCtrl_Start(AppNamePtr);

    if (result != LE_OK)
    {
        fprintf(stderr, "App '%s' could not be started. Check logs for more info.\n", AppNamePtr);
        exit(EXIT_FAILURE);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Finds the system call object for the system call number.
 *
 * @return
 *      File access system call object if successful.
 *      NULL otherwise.
 */
//--------------------------------------------------------------------------------------------------
static const FileAccessSysCall_t* FindFileAccessSysCallObj
(
    int sysCallNum                  ///< [IN] System call number.
)
{
    int i;
    for (i = 0; i < NUM_ARRAY_MEMBERS(FileAccessSysCalls); i++)
    {
        if (sysCallNum == FileAccessSysCalls[i].sysCallNum)
        {
            return &FileAccessSysCalls[i];
        }
    }

    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks whether the new path conflicts with anything under the specified working directory.
 *
 * @return
 *      true if there is a conflict.
 *      false if there are no conflicts.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t DoesPathConflict
(
    const char* newPathPtr,             ///< [IN] New path relative to the working directory.
    const char* workingDirPtr           ///< [IN] Working directory to check.
)
{
    // Iterate through the nodes of the specified path checking if there are conflicts.
    le_pathIter_Ref_t pathIter = le_pathIter_CreateForUnix(newPathPtr);

    if (le_pathIter_GoToStart(pathIter) != LE_OK)
    {
        return true;
    }

    char currPath[MAX_PATH_BYTES] = "";
    INTERNAL_ERR_IF(le_utf8_Copy(currPath, workingDirPtr, sizeof(currPath), NULL) != LE_OK,
                    "Path '%s...' is too long.", currPath);

    while (1)
    {
        // Get the current path.
        char currNode[MAX_PATH_BYTES] = "";
        le_result_t r = le_pathIter_GetCurrentNode(pathIter, currNode, sizeof(currNode));
        INTERNAL_ERR_IF(r == LE_OVERFLOW, "Path '%s...' is too long.", currNode);

        if (r == LE_NOT_FOUND)
        {
            // This is the last node of the destination path so there must be a conflict.
            le_pathIter_Delete(pathIter);
            return true;
        }

        INTERNAL_ERR_IF(le_path_Concat("/", currPath, sizeof(currPath), currNode, NULL) != LE_OK,
                        "Path '%s...' is too long.", currPath);

        // Check the working directory for items at the current path.
        struct stat statBuf;

        if (lstat(currPath, &statBuf) == -1)
        {
            if (errno == ENOENT)
            {
                // Current path does not exist so there are no conflicts.
                le_pathIter_Delete(pathIter);
                return false;
            }
            INTERNAL_ERR("Could not stat path '%s'.  %m.", currPath);
        }

        if (!S_ISDIR(statBuf.st_mode))
        {
            // Conflict.
            le_pathIter_Delete(pathIter);
            return true;
        }

        if (le_pathIter_GoToNext(pathIter) == LE_NOT_FOUND)
        {
            // This is the last node of the destination path so there must be a conflict.
            le_pathIter_Delete(pathIter);
            return true;
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks if the file specified by path can be added to the app's working directory.
 *
 * @return
 *      true if the file can be added to the app's working directory.
 *      false if the file cannot be added to the app's working directory because the file either
 *            does not exist or conflicts with something already in the app's working directory.
 */
//--------------------------------------------------------------------------------------------------
static bool CanAddFile
(
    const char* appNamePtr,                 ///< [IN] App name.
    const char* pathPtr                     ///< [IN] Path to the file.
)
{
    // Check if there would be a conflict with items in the app's working directory.
    if (DoesPathConflict(pathPtr, AppWorkingDir))
    {
        return false;
    }

    // Check if the file actually exists.
    struct stat statBuf;

    if (stat(pathPtr, &statBuf) != 0)
    {
        if ( (errno == ENOENT) || (errno == ENOTDIR) )
        {
            return false;
        }

        INTERNAL_ERR("Could not stat %s.  %m", pathPtr);
    }

    return true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Prints a warning message if the app is trying to access certain potentially dangerous files.
 */
//--------------------------------------------------------------------------------------------------
static void WarnAgainstAdding
(
    const char* dangerousPathPtr,           ///< [IN] Path to warn against.
    const char* pathPtr                     ///< [IN] Path to be added.

)
{
    if (strstr(pathPtr, dangerousPathPtr) == pathPtr)
    {
        printf("*** WARNING: Giving apps access to '%s' can be dangerous! ***\n", dangerousPathPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Asks the user whether the file should be added to the app's working directory.
 *
 * @return
 *      true if the file should be added to the app's working directory.
 *      false if the file should not be added to the app's working directory.
 */
//--------------------------------------------------------------------------------------------------
static bool ShouldAddFile
(
    const char* appNamePtr,                 ///< [IN] App name.
    pid_t tid,                              ///< [IN] TID of the thread that made the sys call.
    const char* sysCallNamePtr,             ///< [IN] Name of the system call.
    const char* pathPtr                     ///< [IN] Path to the file.
)
{
    static bool alwaysYes = false;

    if (alwaysYes)
    {
        return true;
    }

    printf("Thread [%d] in app is trying to %s '%s'\n", tid, sysCallNamePtr, pathPtr);

    WarnAgainstAdding("/proc", pathPtr);
    WarnAgainstAdding("/sys", pathPtr);

    // Ask the user what to do.
    while (1)
    {
        printf("Should the file '%s' be added to the sandbox, [Y]es / [n]o / yesto[a]ll?", pathPtr);

        int choice = getchar();

        // Flush all other characters until the newline.
        if (choice != '\n')
        {
            int c;
            while ( ((c = getchar()) != EOF) && (c != '\n') ) {}
        }

        switch (choice)
        {
            case 'a':
            case 'A':
                alwaysYes = true;
            case '\n':
            case 'y':
            case 'Y':
                printf("\n");
                return true;

            case 'n':
            case 'N':
                printf("\n");
                return false;

            default:
                fprintf(stderr, "Invalid selection.\n");
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks if the path points to a device file.
 *
 * @return
 *      true if the path points to a device.
 *      false otherwise.
 */
//--------------------------------------------------------------------------------------------------
static bool IsDevice
(
    const char* pathPtr             ///< [IN] Path to file.
)
{
    struct stat statBuf;

    if (lstat(pathPtr, &statBuf) == -1)
    {
        if ( (errno == ENOENT) || (errno == ENOTDIR) )
        {
            return false;
        }

        INTERNAL_ERR("Could not stat %s.  %m.", pathPtr);
    }

    if ( S_ISCHR(statBuf.st_mode) || S_ISBLK(statBuf.st_mode) )
    {
        return true;
    }

    return false;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the file object with the specified path in the list.
 *
 * @return
 *      The file object if the path is in the list.
 *      NULL otherwise.
 */
//--------------------------------------------------------------------------------------------------
static File_t* FindPathInList
(
    const char* pathPtr,            ///< [IN] App reference.
    le_sls_List_t* listPtr          ///< [IN] List of paths.
)
{
    le_sls_Link_t* linkPtr = le_sls_Peek(listPtr);

    while (linkPtr != NULL)
    {
        File_t* filePtr = CONTAINER_OF(linkPtr, File_t, link);

        if (strcmp(filePtr->path, pathPtr) == 0)
        {
            return filePtr;
        }

        linkPtr = le_sls_PeekNext(listPtr, linkPtr);
    }

    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the permission for the current open() system call and merges that with what is already in
 * the user's buffer, producing an updated permission string.  For example, if the buffer was "r"
 * and the current open() call has write permission then the buffer would be updated to be "rw".
 *
 * @note This function should only be called if the tracee is in an open() sys call.
 */
//--------------------------------------------------------------------------------------------------
static void GetOpenSysCallPermStr
(
    pid_t pid,              ///< [IN] Process that made the system call.
    Registers_t* regPtr,    ///< [IN] The tracees's set of registers when the system call was made.
    char* bufPtr,           ///< [OUT] Buffer to store the permission string.
    size_t bufSize          ///< [IN] Size of buffer.
)
{
    INTERNAL_ERR_IF(bufSize < 3, "Buffer for permission string is too small.");

    int mode = GetSysCallArg(regPtr, 1) & 0x03;  // Only last two bits are the mode.

    switch (mode)
    {
        case O_RDONLY:
            if (strchr(bufPtr, 'w') != NULL)
            {
                bufPtr[0] = 'r';
                bufPtr[1] = 'w';
                bufPtr[2] = '\0';
            }
            else
            {
                bufPtr[0] = 'r';
                bufPtr[1] = '\0';
            }
            break;

        case O_WRONLY:
            if (strchr(bufPtr, 'r') != NULL)
            {
                bufPtr[0] = 'r';
                bufPtr[1] = 'w';
                bufPtr[2] = '\0';
            }
            else
            {
                bufPtr[0] = 'w';
                bufPtr[1] = '\0';
            }
            break;

        case O_RDWR:
            bufPtr[0] = 'r';
            bufPtr[1] = 'w';
            bufPtr[2] = '\0';
            break;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the permission for the device file.
 */
//--------------------------------------------------------------------------------------------------
static void SetDevicePermissions
(
    pid_t pid,              ///< [IN] Process that made the system call.
    Registers_t* regPtr,    ///< [IN] The process's set of registers when the system call was made.
    File_t* devFilePtr      ///< [IN] The device file object to set permissions on.
)
{
    GetOpenSysCallPermStr(pid, regPtr, devFilePtr->permStr, sizeof(devFilePtr->permStr));

    if (devFilePtr->permStr[0] != '\0')
    {
        if (le_appCtrl_SetDevicePerm(AppRef, devFilePtr->path, devFilePtr->permStr) != LE_OK)
        {
            fprintf(stderr, "Could not set permissions to %s for %s.\n",
                    devFilePtr->permStr,
                    devFilePtr->path);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Check for exceptions, file links that should not be created by default.
 *
 * @return
 *      true if the file should not be added.
 *      false otherwise.
 */
//--------------------------------------------------------------------------------------------------
static bool IsLinkException
(
    const char* pathPtr,            ///< [IN] File path.
    int sysCallNum                  ///< [IN] System call number.
)
{
    // ld.so.cache is generally not needed in a sanbox.
    if (strcmp(pathPtr, "/etc/ld.so.cache") == 0)
    {
        return true;
    }

    // readlink(/proc/self/exe) is done by the dynamic linker and is generally not needed in a
    // sanbox.
    if ( (sysCallNum == __NR_readlink) && (strcmp(pathPtr, "/proc/self/exe") == 0) )
    {
        return true;
    }

    return false;
}


//--------------------------------------------------------------------------------------------------
/**
 * Handles system calls that a process makes.
 */
//--------------------------------------------------------------------------------------------------
static void HandleSysCall
(
    pid_t pid               ///< [IN] Process that made the system call.
)
{
    // Read the register for this system call.
    Registers_t regs;

    if (Ptrace(PTRACE_GETREGS, pid, NULL, &regs) == LE_NOT_FOUND)
    {
        return;
    }

    // Get the system call number.
    int callNum = GetSysCallNum(&regs);

    // Check if the system call is trying to access a file or directory.
    const FileAccessSysCall_t* callObjPtr = FindFileAccessSysCallObj(callNum);

    if (callObjPtr != NULL)
    {
        // Get the path to the file/dir the sys call is trying to access.
        char path[MAX_PATH_BYTES] = "";

        if (GetAccessPath(pid, &regs, callObjPtr, path, sizeof(path)) != LE_OK)
        {
            return;
        }

        LE_DEBUG("[%d] %s(%s)", pid, callObjPtr->sysCallName, path);

        // Handle exceptions.
        if (IsLinkException(path, callNum))
        {
            return;
        }

        // Set permissions for devices that have already been added to the app's working dir.
        if ( (callNum == __NR_open) && (IsDevice(path)) )
        {
            File_t* filePtr = FindPathInList(path, &AddedDevicesList);

            if (filePtr != NULL)
            {
                SetDevicePermissions(pid, &regs, filePtr);
            }
        }

        if (CanAddFile(AppNamePtr, path))
        {
            if (ShouldAddFile(AppNamePtr, pid, callObjPtr->sysCallName, path))
            {
                // Import the file.
                le_result_t r = le_appCtrl_Import(AppRef, path);

                INTERNAL_ERR_IF(r != LE_OK, "Could not import file %s.  %s.",
                                            path, LE_RESULT_TXT(r));

                // Record the added file.
                File_t* filePtr = le_mem_ForceAlloc(FilePool);

                filePtr->link = LE_SLS_LINK_INIT;
                filePtr->permStr[0] = '\0';

                LE_ASSERT(le_utf8_Copy(filePtr->path, path,
                                       sizeof(filePtr->path), NULL) == LE_OK);

                // Set permissions for devices that have already been added to the app's working dir.
                if (IsDevice(path))
                {
                    // Add the file to the list of added devices.
                    le_sls_Queue(&AddedDevicesList, &(filePtr->link));

                    if (callNum == __NR_open)
                    {
                        SetDevicePermissions(pid, &regs, filePtr);
                    }
                }
                else
                {
                    // Add the file to the list of added files.
                    le_sls_Queue(&AddedFilesList, &(filePtr->link));
                }
            }
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Write the list of paths to the open file.
 */
//--------------------------------------------------------------------------------------------------
static void WritePaths
(
    const char* fileNamePtr,        ///< [IN] Name of the file to write to.
    FILE* fileStreamPtr,            ///< [IN] File to write to.
    le_sls_List_t* listPtr          ///< [IN] List of paths.
)
{
    le_sls_Link_t* linkPtr = le_sls_Peek(listPtr);

    if (linkPtr != NULL)
    {
        // Write the section header.
        if (listPtr == &AddedFilesList)
        {
            fputs("    file:\n", fileStreamPtr);
        }
        else
        {
            fputs("    device:\n", fileStreamPtr);
        }

        fputs("    {\n", fileStreamPtr);

        // Write the paths.
        while (linkPtr != NULL)
        {
            File_t* filePtr = CONTAINER_OF(linkPtr, File_t, link);

            if (strcmp(filePtr->permStr, "") == 0)
            {
                if (fprintf(fileStreamPtr, "        %-50s\t%s\n", filePtr->path, filePtr->path) < 0)
                {
                    fprintf(stderr, "Could not write to file %s.  %m.\n", fileNamePtr);
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                if (fprintf(fileStreamPtr, "        [%s] %-50s\t%s\n",
                            filePtr->permStr,
                            filePtr->path,
                            filePtr->path) < 0)
                {
                    fprintf(stderr, "Could not write to file %s.  %m.\n", fileNamePtr);
                    exit(EXIT_FAILURE);
                }
            }

            linkPtr = le_sls_PeekNext(listPtr, linkPtr);
        }

        // Close off the section.
        fputs("    }\n", fileStreamPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Asks the user if we should create a 'requires' section for all files added in this tool.
 *
 * @note This function never returns.
 */
//--------------------------------------------------------------------------------------------------
static void CreateRequiresSection
(
    void
)
{
    const char* fileToUsePtr = RequiresPathPtr;
    char reqFilePath[MAX_PATH_BYTES] = "";

    if (fileToUsePtr == NULL)
    {
        // Ask the user for the path.
        puts(
            "Do you want to create a file with a 'requires' section that contains all the files\n"
            "added in this trace?  If yes, enter the location of the file create.  If no, just\n"
            "hit enter."
            );


        if (fgets(reqFilePath, sizeof(reqFilePath), stdin) != NULL)
        {
            size_t len = strlen(reqFilePath);
            char resolvedFilePath[MAX_PATH_BYTES];
            if (realpath(reqFilePath, resolvedFilePath) == NULL)
            {
                LE_ERROR("No such path: '%s'",reqFilePath);
                exit(EXIT_FAILURE);
            }
            else
            {
                if (len > 1)
                {
                    // Strip the newline char from the file path.
                    if (reqFilePath[len-1] == '\n')
                    {
                        reqFilePath[len-1] = '\0';
                    }

                    fileToUsePtr = reqFilePath;
                }
            }
        }
    }

    if (fileToUsePtr != NULL)
    {
        // Delete the file first.
        if ( (unlink(fileToUsePtr) != 0) && (errno != ENOENT) )
        {
            fprintf(stderr, "Could not delete file '%s'.  %m.\n", fileToUsePtr);
            exit(EXIT_FAILURE);
        }

        if ( (le_sls_Peek(&AddedFilesList) == NULL) && (le_sls_Peek(&AddedDevicesList) == NULL) )
        {
            printf("No paths added.  File not created.\n");
            exit(EXIT_SUCCESS);
        }

        // Create the file.
        FILE* reqFilePtr = fopen(fileToUsePtr, "w");
        if (reqFilePtr == NULL)
        {
            fprintf(stderr, "Could not create file '%s'.  %m.\n", fileToUsePtr);
            exit(EXIT_FAILURE);
        }

        // Start the requires section.
        fputs("requires:\n"
              "{\n",
              reqFilePtr
              );

        // Write the subsections.
        WritePaths(fileToUsePtr, reqFilePtr, &AddedFilesList);
        WritePaths(fileToUsePtr, reqFilePtr, &AddedDevicesList);

        // Close off the requires section.
        fputs("}\n",
              reqFilePtr
              );

        printf("\nRequires section written to %s.\n", fileToUsePtr);

        fclose(reqFilePtr);
    }

    exit(EXIT_SUCCESS);
}


//--------------------------------------------------------------------------------------------------
/**
 * Synchronous handler for SIGCHILD.  Used to trace all attached processes.
 */
//--------------------------------------------------------------------------------------------------
static void SigChildHandler
(
    int sigNum                          ///< [IN] Unused.
)
{
    // Stop the shutdown timer.  No need to check the return value because the timer will be
    // stopped whatever the return value is.
    le_timer_Stop(ShutdownTimer);

    while (1)
    {
        int status;

        pid_t pid = waitpid(-1, &status, WNOHANG | __WALL);

        if (pid < 0)
        {
            if (errno == ECHILD )
            {
                printf("The app has stopped.\n");
                CreateRequiresSection();
                exit(EXIT_SUCCESS);
            }
            else
            {
                fprintf(stderr, "Wait error.  %m.\n");
                exit(EXIT_FAILURE);
            }
        }
        else if (pid == 0)
        {
            // No more attached threads have changed state.
            break;
        }

        // Handle tracees that have stopped due to a signal.
        if (WIFSTOPPED(status))
        {
            // There is activity here so stop the no activity timer.  No need to check the return
            // value because the timer will be stopped whatever the return value is.
            le_timer_Stop(NoActivityTimer);

            // Get the signal.
            int sig = WSTOPSIG(status);

            // Get the tracee object.
            Tracee_t* traceePtr = GetTracee(pid);

            if (traceePtr->needInit)
            {
                // Set ptrace options.
                if (Ptrace(PTRACE_SETOPTIONS, traceePtr->tid, NULL,
                           (void*)(PTRACE_O_TRACESYSGOOD |
                                   PTRACE_O_TRACEEXEC |
                                   PTRACE_O_TRACECLONE |
                                   PTRACE_O_TRACEFORK |
                                   PTRACE_O_TRACEVFORK)) == LE_NOT_FOUND)
                {
                    continue;
                }

                traceePtr->needInit = false;
            }

            // The signal to deliver to the tracee.
            int sigToDeliver = 0;

            // Handle execve().
            if ( (status >> 8) == (SIGTRAP | PTRACE_EVENT_EXEC << 8) )
            {
                // Get the previous pid to see if the pid changed.
                pid_t prevPid;

                if (Ptrace(PTRACE_GETEVENTMSG, traceePtr->tid, NULL, &prevPid) == LE_NOT_FOUND)
                {
                    continue;
                }

                if (prevPid != traceePtr->tid)
                {
                    // If the tracee's ID has changed that means that the exec() call was made in a
                    // subthread which causes all subthreads to be destroyed and the exec() call to
                    // return in the main thread's ID (the PID of the process).
                    traceePtr->inSyscall = true;
                }
            }

            // Handle syscall-stops.
            if (sig == (SIGTRAP | 0x80))
            {
                if (!(traceePtr->inSyscall))
                {
                    HandleSysCall(pid);
                }

                traceePtr->inSyscall = !(traceePtr->inSyscall);
            }
            else if (sig != SIGTRAP)
            {
                // Forward signals to the tracee.
                sigToDeliver = sig;
            }

            // Restart the tracee.
            if (Ptrace(PTRACE_SYSCALL, pid, NULL, (void*)(intptr_t)sigToDeliver) == LE_NOT_FOUND)
            {
                continue;
            }
        }
    }

    // Restart the shut down timer.
    le_timer_Restart(ShutdownTimer);

    // Start the no activity timer if it is not already running.
    if ( (!le_timer_IsRunning(NoActivityTimer)) && (CheckNoActivity) )
    {
        le_timer_Restart(NoActivityTimer);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Periodically checks whether the app has shutdown.
 *
 * @note Checking when all processes/threads using ptrace is difficult and unreliable.  Instead we
 *       simply run the SigChildHandler periodically and if there are no more attached tracees
 *       waitpid() will return -1.
 */
//--------------------------------------------------------------------------------------------------
static void CheckAppShutdown
(
    le_timer_Ref_t timerRef             ///< [IN] Timer refence that triggered this handler.
)
{
    SigChildHandler(SIGCHLD);
}


//--------------------------------------------------------------------------------------------------
/**
 * Asks the user whether we should stop tracing the app because there has been no activity in a
 * while.
 */
//--------------------------------------------------------------------------------------------------
static void AskUserToStop
(
    le_timer_Ref_t timerRef             ///< [IN] Timer refence that triggered this handler.
)
{
    printf("There has been no activity detected in the app for a while.\n");
    printf("Do you want to stop tracing the app (Y/n)?");

    int choice = getchar();

    // Flush all other characters until the newline.
    if (choice != '\n')
    {
        int c;
        while ( ((c = getchar()) != EOF) && (c != '\n') ) {}
    }

    switch (choice)
    {
        case '\n':
        case 'y':
        case 'Y':
            printf("\n");
            CreateRequiresSection();

        default:
            printf("Continuing trace.  Type Ctrl+c to stop.\n\n");
            CheckNoActivity = false;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Synchronous handler for SIGINT.  Used to handle when the user types Ctrl-C.
 */
//--------------------------------------------------------------------------------------------------
static void SigIntHandler
(
    int sigNum                          ///< [IN] Unused.
)
{
    CreateRequiresSection();
}


//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // Block Signals that we are going to use.
    le_sig_Block(SIGCHLD);
    le_sig_Block(SIGINT);

    // Register a signal event handler for SIGCHLD so we know when processes die.
    le_sig_SetEventHandler(SIGCHLD, SigChildHandler);

    // Register a signal event handler for SIGINT so we can do special process when the user types
    // Ctrl-C.
    le_sig_SetEventHandler(SIGINT, SigIntHandler);

    // Create the process memory pool.
    TraceePool = le_mem_CreatePool("TraceePool", sizeof(Tracee_t));

    // Create the file object memory pool.
    FilePool = le_mem_CreatePool("FilePool", sizeof(File_t));

    // Create the shutdown timer.
    ShutdownTimer = le_timer_Create("ShutdownTimer");
    LE_ASSERT(le_timer_SetHandler(ShutdownTimer, CheckAppShutdown) == LE_OK);
    LE_ASSERT(le_timer_SetMsInterval(ShutdownTimer, 1000) == LE_OK);

    // Create the no activity timer.
    NoActivityTimer = le_timer_Create("NoActivityTimer");
    LE_ASSERT(le_timer_SetHandler(NoActivityTimer, AskUserToStop) == LE_OK);
    LE_ASSERT(le_timer_SetMsInterval(NoActivityTimer, 5000) == LE_OK);

    // Create the tracee hash map.
    TraceeMap = le_hashmap_Create("Tracees", ESTIMATED_NUM_TRACEES, le_hashmap_HashUInt32,
                                  le_hashmap_EqualsUInt32);

    // Handle options.
    le_arg_SetFlagCallback(PrintHelp, "h", "help");
    le_arg_SetStringCallback(SetRequireFilePath, "o", "output");

    // Get the app to trace.
    le_arg_AddPositionalCallback(StoreAppName);

    le_arg_Scan();

    StartAppTrace();
}
