//--------------------------------------------------------------------------------------------------
/**
 * Sample command-line application.
 *
 * Demonstrates use of the Legato command-line argument parsing API and the Legato framework's
 * support for command-line apps that run inside sandboxes.
 *
 * Simple program to print information about files or directories specified on the command line.
 * Takes a command, followed by a list of file/directory paths.  Accepts additional options like
 * -x or --max-count.
 *
 * Copyright (C) Sierra Wireless Inc.
 **/
//--------------------------------------------------------------------------------------------------

#include "legato.h"

/// Set to true if the list of files/directories should be printed in reverse of the order in
/// which it was specified on the command-line.
static bool InReverse = false;

/// Will stop after this many files/directories if more than that are specified on the command-line.
/// -1 = no limit.
static int MaxCount = -1;   // Default is "no limit".

/// The number of files/directories specified on the command-line.
static size_t Count = 0;

/// true if the -x or --extreme options have been given.
static bool IsExtreme = false;

/// When a file or directory path is found on the command-line, it will be stored in one of
/// these structures and placed on the PathList linked list.
typedef struct
{
    le_dls_Link_t link;     ///< Used to link the record into the PathList.
    const char* pathPtr;    ///< Pointer to the file/directory path.
}
PathRecord_t;

/// Linked list on which path records are kept.
static le_dls_List_t PathList = LE_DLS_LIST_INIT;
/// The memory pool for Path Record objects
static le_mem_PoolRef_t PathRecordMemoryPool;

/// Pointer to the information printing function to be used.  This pointer is set during
/// command-line argument parsing, when the command argument is seen.
static void (*PrintInfo)(struct stat* fileInfoPtr) = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Prints the on-line help and exits.  This function is called when "-h" or "--help" appears on
 * the command-line or when the help command is invoked.
 **/
//--------------------------------------------------------------------------------------------------
static void PrintHelp(void)
{
    puts(   "NAME\n"
            "        fileInfo - print information about files or directories.\n"
            "\n"
            "SYNOPSIS\n"
            "        fileInfo [OPTION]... COMMAND PATH [PATH]...\n"
            "        fileInfo -h\n"
            "        fileInfo --help\n"
            "\n"
            "COMMANDS\n"
            "       type\n"
            "               Print the type of each file system object.\n"
            "\n"
            "       permissions\n"
            "               Print the permissions of each file system object.\n"
            "\n"
            "       help\n"
            "               Print a help message and exit. Ignore all other arguments.\n"
            "\n"
            "OPTIONS\n"
            "       -x--extreme\n"
            "       --extreme\n"
            "               Print a silly message.\n"
            "\n"
            "       -mc N\n"
            "       --max-count=N\n"
            "               Don't print the status of more than N file system objects, even if\n"
            "               there are more than that on the command line.  -1 = no limit.\n"
        );

    exit(EXIT_SUCCESS);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the next file/directory path to work on.
 *
 * @return A pointer to the path string.
 *
 * @note Will not return (exits with EXIT_SUCCESS) if all paths have been fetched already.
 **/
//--------------------------------------------------------------------------------------------------
static const char* GetPath(void)
{
    le_dls_Link_t* linkPtr;

    if (InReverse == false)
    {
        linkPtr = le_dls_Pop(&PathList);
    }
    else
    {
        linkPtr = le_dls_PopTail(&PathList);
    }

    if (linkPtr != NULL)
    {
        PathRecord_t* recPtr = CONTAINER_OF(linkPtr, PathRecord_t, link);

        return recPtr->pathPtr;

        // Note: Don't have to free the records or the path strings, because this program is very
        //       short-lived and the OS will free all of its memory when it dies.
    }
    else
    {
        // Terminate the program if the list is now empty.
        exit(EXIT_SUCCESS);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Adds a file path to the list of files/directories to work on.
 **/
//--------------------------------------------------------------------------------------------------
static void SetFilePath
(
    const char* filePath
)
{
    PathRecord_t* recPtr = le_mem_ForceAlloc(PathRecordMemoryPool);

    recPtr->link = LE_DLS_LINK_INIT;
    recPtr->pathPtr = filePath;

    le_dls_Queue(&PathList, &recPtr->link);

    Count++;
}


//--------------------------------------------------------------------------------------------------
/**
 * Prints an error message to stderr for a given errno value received from a call to stat() that
 * failed for a given file/directory path.
 **/
//--------------------------------------------------------------------------------------------------
static void PrintStatError
(
    int errorCode,
    const char* pathPtr
)
{
    switch (errorCode)
    {
        case EACCES:
            fprintf(stderr,
                    "A directory in the path prefix of '%s' does not have 'x' permission.\n",
                    pathPtr);
            break;

        case ELOOP:
            fprintf(stderr, "Too many symbolic links in path '%s'.\n", pathPtr);
            break;

        case ENAMETOOLONG:
            fprintf(stderr, "Path too long: '%s'.\n", pathPtr);
            break;

        case ENOENT:
            fprintf(stderr, "Path not found: '%s'.\n", pathPtr);
            break;

        case ENOMEM:
            // The kernel is out of memory!!!
            fprintf(stderr, "You don't want to know what just happened.\n");
            break;

        case ENOTDIR:
            fprintf(stderr,
                    "Some part of the path prefix of '%s' is not a directory.\n",
                    pathPtr);
            break;

        case EOVERFLOW:
            fprintf(stderr, "I can't handle how big the file at path '%s' is!!\n", pathPtr);
            break;

        default:
            fprintf(stderr, "Unexpected errno %d (%s).\n", errorCode, strerror(errorCode));
            break;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Prints to stdout a string representing the type of a file system object.
 **/
//--------------------------------------------------------------------------------------------------
static void PrintType
(
    struct stat* fileInfoPtr    ///< Pointer to the file information structure filled in by stat().
)
{
    const char* typeStringPtr = " is... um... I don't know!";

    if (S_ISREG(fileInfoPtr->st_mode))
    {
        typeStringPtr = " is a regular file";
    }
    else if (S_ISDIR(fileInfoPtr->st_mode))
    {
        typeStringPtr = " is a directory";
    }
    else if (S_ISCHR(fileInfoPtr->st_mode))
    {
        typeStringPtr = " is a character device";
    }
    else if (S_ISBLK(fileInfoPtr->st_mode))
    {
        typeStringPtr = " is a block device";
    }
    else if (S_ISFIFO(fileInfoPtr->st_mode))
    {
        typeStringPtr = " is a named pipe (FIFO)";
    }
    else if (S_ISLNK(fileInfoPtr->st_mode))
    {
        typeStringPtr = " is a symbolic link";
    }
    else if (S_ISSOCK(fileInfoPtr->st_mode))
    {
        typeStringPtr = " is a named socket";
    }

    printf("%s", typeStringPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Prints to stdout a string representing the permissions on a file system object.
 **/
//--------------------------------------------------------------------------------------------------
static void PrintPermissions
(
    struct stat* fileInfoPtr    ///< Pointer to the file information structure filled in by stat().
)
{
    mode_t mode = fileInfoPtr->st_mode;

    printf(" the owner");
    if (mode & S_IRWXU)
    {
        printf(" can");

        if (mode & S_IRUSR)
        {
            printf(" read");
        }
        if (mode & S_IWUSR)
        {
            printf(" write");
        }
        if (mode & S_IXUSR)
        {
            printf(" execute");
        }
    }
    else
    {
        printf(" cannot access");
    }

    printf(", group members");
    if (mode & S_IRWXG)
    {
        printf(" can");

        if (mode & S_IRGRP)
        {
            printf(" read");
        }
        if (mode & S_IWGRP)
        {
            printf(" write");
        }
        if (mode & S_IXGRP)
        {
            printf(" execute");
        }
    }
    else
    {
        printf(" cannot access");
    }

    printf(", and others");
    if (mode & S_IRWXO)
    {
        printf(" can");

        if (mode & S_IROTH)
        {
            printf(" read");
        }
        if (mode & S_IWOTH)
        {
            printf(" write");
        }
        if (mode & S_IXOTH)
        {
            printf(" execute");
        }
    }
    else
    {
        printf(" cannot access");
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the command to be executed for each of the file/directory paths.
 *
 * This function is called when the first positional argument (i.e., the first argument not
 * starting with a '-') is parsed from the command-line argument list.
 **/
//--------------------------------------------------------------------------------------------------
static void SetCommand
(
    const char* arg     ///< Pointer to the argument.
)
{
    // Based on the command argument, set the appropriate information printing function to be used
    // later.
    if (strcmp("type", arg) == 0)
    {
        PrintInfo = PrintType;
    }
    else if (strcmp("permissions", arg) == 0)
    {
        PrintInfo = PrintPermissions;
    }
    else if (strcmp("help", arg) == 0)
    {
        PrintHelp();
    }
    else
    {
        fprintf(stderr, "Unrecognized command: '%s'\n", arg);
        exit(EXIT_FAILURE);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Execute the selected command for all the files/directories specified on the command-line.
 **/
//--------------------------------------------------------------------------------------------------
static void ExecuteCommand(void)
{
    int i = 0;

    for (i = 0; i < MaxCount; i++)
    {
        const char* pathPtr = GetPath();  // This won't return if we've processed all the paths.

        struct stat fileInfo;

        if (stat(pathPtr, &fileInfo) == 0)
        {
            printf("'%s'", pathPtr);

            PrintInfo(&fileInfo);

            if (IsExtreme)
            {
                puts("!!!!!!! 8^O");
            }
            else
            {
                puts(".");
            }
        }
        else
        {
            PrintStatError(errno, pathPtr);
        }
    }

    printf("Maximum file count reached.\n");

    exit(EXIT_SUCCESS);
}


COMPONENT_INIT
{
    // Allocate a pool for the Path Records
    PathRecordMemoryPool = le_mem_CreatePool("PathRecordMemoryPool", sizeof(PathRecord_t));

    // Request 1 block.
    le_mem_ExpandPool(PathRecordMemoryPool, 1);

    // Set IsExtreme to true if the -x or --extreme appears on the command-line.
    le_arg_SetFlagVar(&IsExtreme, "x", "extreme");

    // Set Count to the value N given by "-mc N" or "--max-count=N".
    le_arg_SetIntVar(&MaxCount, "mc", "max-count");

    // Register a function to be called if -h or --help appears on the command-line.
    le_arg_SetFlagCallback(PrintHelp, "h", "help");

    // The first argument that doesn't start with '-' or '--' should be a command.
    le_arg_AddPositionalCallback(SetCommand);

    // All other arguments that don't start with '-' or '--' should be file paths.
    le_arg_AddPositionalCallback(SetFilePath);

    // Allow as many file paths as they want.
    le_arg_AllowMorePositionalArgsThanCallbacks();

    LE_INFO("Scanning.");

    // Perform command-line argument processing.
    le_arg_Scan();

    LE_INFO("Done scanning.");

    // If the PrintInfo function pointer is still NULL, then no command was specified on the
    // command line.
    if (PrintInfo == NULL)
    {
        fprintf(stderr, "Please specify a command.  Try 'fileInfo --help' for more information.\n");
        exit(EXIT_FAILURE);
    }

    // If the Count is still 0, then no paths were specified on the command-line.
    if (Count == 0)
    {
        fprintf(stderr, "At least one file or directory path must be specified.\n");
        exit(EXIT_FAILURE);
    }

    // If the count is less than -1, then the max-count specified is out of range.
    if (MaxCount < -1)
    {
        fprintf(stderr, "Maximum count (%d) is out of range.\n", MaxCount);
        exit(EXIT_FAILURE);
    }

    // If we get here, then we know that all the command-line arguments have been parsed
    // successfully, and all the variables have been set and call-backs have been called.
    ExecuteCommand();
}
