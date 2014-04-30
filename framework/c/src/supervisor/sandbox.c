//--------------------------------------------------------------------------------------------------
/** @file sandbox.c
 *
 * API for creating Legato Sandboxes.
 *
 *  - @ref c_sand_intro
 *  - @ref c_sand_sandboxSetup
 *  - @ref c_sand_resourceLimits
 *  - @ref c_sand_confineProcess
 *  - @ref c_sand_accessServices
 *  - @ref c_sand_removeSandbox
 *
 * @section c_sand_intro Introduction
 *
 * Legato sandboxes are intended to provide a layer of security to the system from untrusted (or
 * not necessarily trusted) applications running in the same system.  Legato sandboxes isolate
 * applications from the rest of the system and makes it more difficult for a misbehaving
 * application from doing damage to the system and/or other applications.
 *
 * A Legato sandbox is not a virtualized environment.  It is suitable for isolating applications
 * that do not require root privileges and have minimal system requirements.
 *
 * For example, a system analysis utility that is used to gather statistics about which applications
 * use which resources is not a good candidate for a Legato sandboxed application.  However, an
 * application that displays the analyzed data to a web server would be a good candidate.
 *
 * Legato sandboxes are based on chroot jails.  Chroot changes the root directory of a process to
 * a specified location.  The process then only has access to files and directories under its root
 * directory.  Only processes with certain capabilities can find their way outside of their chrooted
 * root directory.
 *
 * Another important aspect of Legato sandboxes is resource limitations.  Resource limitations place
 * limits on the amount of system resources an application may consume.  Without resource limits an
 * isolated application could still damage the system by consuming all available resources.
 *
 * Essentially, there are three steps to sandboxing an application, sandbox setup, setting resource
 * limits, confining processes in the sandbox.
 *
 * @section c_sand_sandboxSetup Setting Up a Sandbox
 *
 * All sandboxes are created in non-persistent memory under /tmp/Legato/sandboxes.  Using
 * non-persistent memory has the benefit of automatically removing all sandboxes on system shutdown.
 *
 * The process for setting up a sandbox for an application is:
 *
 *      1) Create the directory /tmp/Legato/sandboxes/appName.  This is the root of the sandbox.
 *         Creating a new directory gives us a way to separate applications from each other.
 *
 *      2) Mount a ramfs with a fixed size at the root of the sandbox.  Ramfs is used because it
 *         does not grow.  This gives control over how much ram the application can use for files.
 *         This is essentially the maximum size of the application's root file system.
 *
 *      3) Create standard directories in the sandbox, such as /tmp, /home/appName, /dev, etc.
 *
 *      4) Bind mount in standard files and devices into the sandbox, such as /dev/null, the Service
 *         Directory sockets, etc.
 *
 *      5) Bind mount in all other required files into the sandbox specific to the application.
 *
 * Legato sandboxes use bind mounts for importing files from the root file system into sandboxes.
 * Bind mounted files are similar to hard links and so are updated when the file is updated in the
 * root file system.  The advantage of this over copying the file into the sandbox is memory usage
 * managing updates.
 *
 * Bind mounts also work better than hard links because when a sym link is bind mounted into a
 * sandbox the sym link is followed and the actual file is mounted in the sandbox.  Also,
 * permissions on bind mounted files can be modified by re-mounting the file.
 *
 * @todo Remount is not currently being used but should be possible.  Need a way to set file
 *       permissions appropriately either by the Supervisor or the Installer.
 *
 *
 * @section c_sand_resourceLimits Setting Resource Limits
 *
 * Resource limits are set using the resource limits API and are set for both the application as a
 * whole and the individual processes in the application.
 *
 * The current implementation for setting resource limits uses Linux's rlimits.  With rlimits the
 * resource limits are always placed on the processes themselves rather than on the application or
 * user.  This is not an ideal situation because some limits such as the number of processes in an
 * application should apply directly to the application.  As a consequence there is a bit of
 * awkwardness when defining and setting application level resource limits.
 *
 * @todo Use cgroups to set limits for a group of processes and/or a specific user.
 *
 *
 * @section c_sand_confineProcess Confining Processes in Legato Sandboxes
 *
 * After the sandbox has been setup the application processes must be confined in the sandbox.  The
 * process for confining a process in a sandbox is:
 *
 *      1) Change the process's working directory to somewhere inside the sandbox.  This is
 *         important because if the working directory is left outside the sandbox the process can
 *         take advantage of this to escape the sandbox.
 *
 *      2) Chroot to the sandbox root.
 *
 *      3) Clear the supplementary groups list.
 *
 *      4) Set the primary group ID.
 *
 *      5) Set the user ID (effective, real and saved user IDs) to the application's unprivileged
 *         user ID.
 *
 * The process must start with root privileges (or the proper capibilities) to perform the above
 * functions.  After the user ID is set to the unprivileged user ID the process cannot regain root
 * privileges and cannot perform the above operations.
 *
 * The main vulnerability of Legato sandboxes is that once a process regains root privileges
 * (possibly through kernel bugs) the process can easily escape the sandbox and do damage to the
 * system.
 *
 * @todo Use a mandatory access control system like AppArmour to prevent damage even if root
 *       privileges are gained.
 *
 *
 * @section c_sand_accessServices Access Services from within a Sandbox
 *
 * A Legato sandboxed application is still able to access services outside of its sandbox albeit
 * indirectly.  All available services are advertised by the Service Directory.  Applications
 * connect to services by making a request to the Service Directory.  The Service Directory grants
 * access to the application if the application is in the ACL for the specified service.  The
 * Legato sandboxes is only one part of the Legato framework that helps make running applications
 * more secure.
 *
 *
 * @section c_sand_removeSandbox Removing Sandboxes
 *
 * The following procedure is used to remove a sandbox:
 *
 *      1) All application processes are killed.
 *
 *      2) All mounts are undone.
 *
 *      3) Created directories are deleted.
 *
 * @todo Use lazy unmount so unmounts will always succeed.
 *
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#include "legato.h"
#include "sandbox.h"
#include "limit.h"
#include "le_cfg_interface.h"
#include "serviceDirectoryProtocol.h"
#include "resourceLimits.h"
#include <grp.h>
#include <sys/mount.h>
#include <mntent.h>


//--------------------------------------------------------------------------------------------------
/**
 * Location for all sandboxed apps.
 */
//--------------------------------------------------------------------------------------------------
#define SANDBOXES_DIR                   LE_RUNTIME_DIR "sandboxes/"


//--------------------------------------------------------------------------------------------------
/**
 * The location where all applications are installed.
 */
//--------------------------------------------------------------------------------------------------
#define APPS_INSTALL_DIR                "/opt/legato/apps"


//--------------------------------------------------------------------------------------------------
/**
 * The maximum portion of available memory that an application can use as it's file system is
 * calculated as:
 *
 *      free memory / APP_FS_MEM_LIM_DIVISOR
 */
//--------------------------------------------------------------------------------------------------
#define APP_FS_MEM_LIM_DIVISOR          4


//--------------------------------------------------------------------------------------------------
/**
 * The likely limit on number of possible file descriptors in the calling process.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_PROCESS_FD                  1024


//--------------------------------------------------------------------------------------------------
/**
 * The name of the node in the config tree that specifies whether the app should be run in debug
 * mode.
 *
 * If this entry in the config tree is missing or empty the application will not be run in debug
 * mode.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_APP_DEBUG                              "debug"


//--------------------------------------------------------------------------------------------------
/**
 * The name of the node in the config tree that contains the list of import directives for all files
 * that an application needs.
 *
 * An import directive consists of a source file and the destination path.
 *
 * If this entry in the config tree is missing or empty the application will not be launched.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_IMPORT_FILES                           "files"


//--------------------------------------------------------------------------------------------------
/**
 * The name of the node in the config tree that contains the source file to import to the sandbox.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_SRC_FILE                               "src"


//--------------------------------------------------------------------------------------------------
/**
 * The name of the node in the config tree that contains the destination path to import to the
 * sandbox.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_DEST_PATH                              "dest"


//--------------------------------------------------------------------------------------------------
/**
 * Helper macro that returns from the function if the result is not LE_OK.
 */
//--------------------------------------------------------------------------------------------------
#define RETURN_IF_NOT_OK(func)          {   le_result_t r = func; \
                                            if (r != LE_OK) { return r; } \
                                        }


//--------------------------------------------------------------------------------------------------
/**
 * Import object.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char src[LIMIT_MAX_PATH_BYTES];
    char dest[LIMIT_MAX_PATH_BYTES];
}
ImportObj_t;


//--------------------------------------------------------------------------------------------------
/**
 * Files and directories to import into all sandboxes by default.
 */
//--------------------------------------------------------------------------------------------------
const ImportObj_t DefaultImportObjs[] = { {.src = LE_SVCDIR_SERVER_SOCKET_NAME, .dest = "/tmp/legato/"},
                                          {.src = LE_SVCDIR_CLIENT_SOCKET_NAME, .dest = "/tmp/legato/"},
                                          {.src = "/dev/log", .dest = "/dev/"},
                                          {.src = "/dev/null", .dest = "/dev/"},
                                          {.src = "/dev/zero", .dest = "/dev/"}, };


//--------------------------------------------------------------------------------------------------
/**
 * Files and directories to import in debug mode.
 */
//--------------------------------------------------------------------------------------------------
const ImportObj_t DebugImportObjs[] = { {.src = DEBUG_PROGRAM, .dest = "/bin/"},
                                        {.src = "/lib/libdl.so.2", .dest = "/lib/"},
                                        {.src = "/proc", .dest = "/"},
                                        {.src = "/lib/libgcc_s.so.1", .dest = "/lib"} };


//--------------------------------------------------------------------------------------------------
/**
 * Gets the sandbox location path string.  The sandbox does not have to exist before this function
 * is called.  This function gives the expected location of the sandbox by simply appending the
 * appName to the sandbox root path.
 *
 * @return
 *      LE_OK if successful.
 *      LE_OVERFLOW the provided buffer is too small.
 */
//--------------------------------------------------------------------------------------------------
le_result_t sandbox_GetPath
(
    const char* appName,    ///< [IN] The application name.
    char* pathBuf,          ///< [OUT] The buffer that will contain the path string.
    size_t pathBufSize      ///< [IN] The buffer size.
)
{
    if (snprintf(pathBuf, pathBufSize, "%s%s", SANDBOXES_DIR, appName) >= pathBufSize)
    {
        return LE_OVERFLOW;
    }
    else
    {
        return LE_OK;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Imports the file at srcFilePtr into the sandbox to destPathPtr.  This function can also be used
 * to import whole directories as well.
 *
 * @note Currently we use bind mounts to import the file.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ImportFile
(
    const char* srcPathPtr,     ///< [IN] The source file or directory to import.
    const char* destPathPtr,    ///< [IN] The location in the sandbox to import the file to.
    const char* sandboxRoot     ///< [IN] The root directory of the sandbox to import the file into.
)
{
    // Check if the source is a directory.
    bool isDir = false;

    struct stat stats;
    if (stat(srcPathPtr, &stats) == -1)
    {
        LE_ERROR("Could not access source '%s'.  %m", srcPathPtr);
        return LE_FAULT;
    }

    isDir = S_ISDIR(stats.st_mode);

    // Create the absolute destination path.
    char destPath[LIMIT_MAX_PATH_BYTES];
    int size;

    if (destPathPtr[0] == '/')
    {
        size = snprintf(destPath, sizeof(destPath), "%s%s", sandboxRoot, destPathPtr);
    }
    else
    {
        size = snprintf(destPath, sizeof(destPath), "%s/%s", sandboxRoot, destPathPtr);
    }

    if (size >= sizeof(destPath))
    {
        LE_ERROR("Import destination path '%s' is too long.", destPath);
        return LE_FAULT;
    }

    if (!isDir)
    {
        // Create all the directories in the destination path.
        RETURN_IF_NOT_OK(le_dir_MakePath(destPath, S_IRWXU | S_IROTH | S_IXOTH));
    }

    // Now get the destination's full path.
    char fullDestPath[LIMIT_MAX_PATH_BYTES];

    if (destPath[size - 1] == '/')
    {
        size = snprintf(fullDestPath, sizeof(fullDestPath), "%s%s",
                        destPath, le_path_GetBasenamePtr(srcPathPtr, "/"));
    }
    else
    {
        size = snprintf(fullDestPath, sizeof(fullDestPath), "%s/%s",
                        destPath, le_path_GetBasenamePtr(srcPathPtr, "/"));
    }

    if (size >= sizeof(fullDestPath))
    {
        LE_ERROR("Destination path '%s' is too long.", fullDestPath);
        return LE_FAULT;
    }

    if (isDir)
    {
        // Create all the directories in the full destination path.
        RETURN_IF_NOT_OK(le_dir_MakePath(fullDestPath, S_IRWXU | S_IROTH | S_IXOTH));
    }
    else
    {
        // Create an empty file at the specified path.
        int fd;
        while ( ((fd = open(fullDestPath, O_RDONLY | O_CREAT, S_IRUSR)) == -1) && (errno == EINTR) ) {}

        if (fd == -1)
        {
            LE_ERROR("Could not create file '%s'.  %m", fullDestPath);
            return LE_FAULT;
        }

        int r;
        do
        {
            r = close(fd);
        } while ( (r == -1) && (errno == EINTR) );

        LE_FATAL_IF(r == -1, "Could not close open file descriptor.  %m.");
    }

    // Bind mount file or directory into the sandbox.
    if (mount(srcPathPtr, fullDestPath, NULL, MS_BIND, NULL) != 0)
    {
        LE_ERROR("Could not import '%s' into sandbox destination '%s'.  %m", srcPathPtr, fullDestPath);
        return LE_FAULT;
    }

    LE_INFO("Imported '%s' into sandbox '%s'.", srcPathPtr, fullDestPath);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Import all needed files to the application sandbox.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ImportAllFiles
(
    app_Ref_t appRef
)
{
    const char* appName = app_GetName(appRef);
    const char* sandboxPath = app_GetSandboxPath(appRef);

    // Create an iterator for our app.
    le_cfg_IteratorRef_t appCfg = le_cfg_CreateReadTxn(app_GetConfigPath(appRef));

    // Check if this app should be in debug mode.
    if (le_cfg_GetBool(appCfg, CFG_NODE_APP_DEBUG, false))
    {
        int i;
        for (i = 0; i < NUM_ARRAY_MEMBERS(DebugImportObjs); i++)
        {
            // Import files and directories needed for debug.
            if (ImportFile(DebugImportObjs[i].src, DebugImportObjs[i].dest, sandboxPath) != LE_OK)
            {
                le_cfg_CancelTxn(appCfg);
                return LE_FAULT;
            }
        }
    }

    // Read the files to import from the config tree.
    le_cfg_GoToNode(appCfg, CFG_NODE_IMPORT_FILES);
    if (le_cfg_NodeExists(appCfg, "") == false)
    {
        LE_ERROR("There are no files to import for application '%s'.", appName);

        le_cfg_CancelTxn(appCfg);
        return LE_FAULT;
    }

    if (le_cfg_GoToFirstChild(appCfg) != LE_OK)
    {
        LE_ERROR("No files to import for application '%s'.", appName);

        le_cfg_CancelTxn(appCfg);
        return LE_FAULT;
    }

    do
    {
        char srcFile[LIMIT_MAX_PATH_BYTES];
        if (le_cfg_GetString(appCfg, CFG_NODE_SRC_FILE, srcFile, sizeof(srcFile), "") != LE_OK)
        {
            LE_ERROR("Source file path '%s...' for app '%s' is too long.", srcFile, appName);

            le_cfg_CancelTxn(appCfg);
            return LE_FAULT;
        }

        if (strncmp(srcFile, "", LIMIT_MAX_PATH_BYTES) == 0)
        {
            LE_ERROR("Empty source file path supplied for app %s.", appName);
            return LE_FAULT;
        }

        // Convert the source file path to an absolute path.
        char* sourceFilePtr = srcFile;
        char absSrcPath[LIMIT_MAX_PATH_BYTES] = { 0 };

        if (srcFile[0] != '/')
        {
            if (le_path_Concat("/", absSrcPath, sizeof(absSrcPath),
                               APPS_INSTALL_DIR, appName, srcFile, (char*)NULL) == LE_OVERFLOW)
            {
                LE_ERROR("Absolute file path '%s/%s/%s' for app '%s' is too long.",
                         APPS_INSTALL_DIR, appName, srcFile, appName);

                le_cfg_CancelTxn(appCfg);
                return LE_FAULT;
            }

            sourceFilePtr = absSrcPath;
        }

        char destPath[LIMIT_MAX_PATH_BYTES];
        if (le_cfg_GetString(appCfg, CFG_NODE_DEST_PATH, destPath, sizeof(destPath), "") != LE_OK)
        {
            LE_ERROR("Destination file path '%s...' for app '%s' is too long.", destPath, appName);

            le_cfg_CancelTxn(appCfg);
            return LE_FAULT;
        }

        if (strncmp(destPath, "", LIMIT_MAX_PATH_BYTES) == 0)
        {
            LE_ERROR("Empty dest file path supplied for file '%s' in app %s.", absSrcPath, appName);

            le_cfg_CancelTxn(appCfg);
            return LE_FAULT;
        }

        if (ImportFile(sourceFilePtr, destPath, sandboxPath) != LE_OK)
        {
            le_cfg_CancelTxn(appCfg);
            return LE_FAULT;
        }
    }
    while (le_cfg_GoToNextSibling(appCfg) == LE_OK);

    le_cfg_CancelTxn(appCfg);

    // Import default files.
    int i;
    for (i = 0; i < NUM_ARRAY_MEMBERS(DebugImportObjs); i++)
    {
        if (ImportFile(DefaultImportObjs[i].src, DefaultImportObjs[i].dest, sandboxPath) != LE_OK)
        {
            return LE_FAULT;
        }
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets up a local file system for the application's sandbox.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SetupFileSystem
(
    app_Ref_t appRef                ///< [IN] The application to setup the sandbox for.
)
{
    int fileSysLimit = (int)resLim_GetSandboxedAppTmpfsLimit(appRef);

    // Make the mount options.
    char opt[100];
    if (snprintf(opt, sizeof(opt), "size=%d,mode=%.4o,uid=0",
                 (int)fileSysLimit, S_IRWXU | S_IXOTH) >= sizeof(opt))
    {
        LE_ERROR("Mount options string is too long. '%s'", opt);

        return LE_FAULT;
    }

    // Mount the tmpfs for the sandbox.
    if (mount("none", app_GetSandboxPath(appRef), "tmpfs", MS_NOSUID, opt) == -1)
    {
        LE_ERROR("Could not create mount for sandbox '%s'.  %m.", app_GetName(appRef));

        return LE_FAULT;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets up an application's sandbox.  This function looks at the settings in the config tree and
 * sets up the application's sandbox area.
 *
 *  - Creates the sandbox directory.
 *  - Imports all needed files (libraries, executables, config files, socket files, device files).
 *  - Import syslog socket.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t sandbox_Setup
(
    app_Ref_t appRef                ///< [IN] The application to setup the sandbox for.
)
{
    // Make the sanboxes directory.
    if (le_dir_Make(SANDBOXES_DIR, S_IRWXU) == LE_FAULT)
    {
        return LE_FAULT;
    }

    const char* appName = app_GetName(appRef);
    const char* sandboxPath = app_GetSandboxPath(appRef);

    // Make the app's sandbox directory.
    le_result_t r = le_dir_Make(sandboxPath, S_IRWXU);

    if (r == LE_FAULT)
    {
        goto cleanup;
    }
    else if (r == LE_DUPLICATE)
    {
        // If the sandbox already exists then this was probably some garbage left over from a
        // previous creation of this sandbox.  Attempt to delete the sandbox first and then recreate
        // it.
        LE_WARN("Sandbox for application '%s' already exists.  Attempting to delete it and recreate it.",
                appName);
        sandbox_Remove(appRef);

        if (le_dir_Make(sandboxPath, S_IRWXU) != LE_OK)
        {
            goto cleanup;
        }
    }

    // Setup the sandboxed apps local file system.
    if (SetupFileSystem(appRef) != LE_OK)
    {
        goto cleanup;
    }

    // Create /tmp folder in the sandbox.  This where we put Legato sockets.
    char folderPath[LIMIT_MAX_PATH_BYTES];
    if (snprintf(folderPath, sizeof(folderPath), "%s/tmp", sandboxPath) >= sizeof(folderPath))
    {
        LE_ERROR("Path '%s' is too long.", folderPath);
        goto cleanup;
    }

    if (le_dir_Make(folderPath, S_IRWXU | S_IRWXO | S_ISVTX) != LE_OK)
    {
        goto cleanup;
    }

    // Create /home folder in the sandbox.
    if (snprintf(folderPath, sizeof(folderPath), "%s/home", sandboxPath) >= sizeof(folderPath))
    {
        LE_ERROR("Path '%s' is too long.", folderPath);
        goto cleanup;
    }

    if (le_dir_Make(folderPath, S_IRWXU | S_IROTH|S_IXOTH) != LE_OK)
    {
        goto cleanup;
    }

    // Create the user's home folder.
    if ( snprintf(folderPath, sizeof(folderPath), "%s/home/%s", sandboxPath, appName) >=
         sizeof(folderPath) )
    {
        LE_ERROR("Path '%s' is too long.", folderPath);
        goto cleanup;
    }

    if (le_dir_Make(folderPath, S_IRWXU) != LE_OK)
    {
        goto cleanup;
    }

    // Set the owner of this folder to the application's user.
    if (chown(folderPath, app_GetUid(appRef), app_GetGid(appRef)) != 0)
    {
        LE_ERROR("Could not set ownership of folder '%s' to uid %d.", folderPath, app_GetUid(appRef));
        goto cleanup;
    }

    if (ImportAllFiles(appRef) != LE_OK)
    {
        goto cleanup;
    }

    return LE_OK;

cleanup:
    // Clean up the sandbox if there was an error creating it.
    sandbox_Remove(appRef);
    return LE_FAULT;
}


//--------------------------------------------------------------------------------------------------
/**
 * Modifies a string that contains a path in the first portion of the string by terminating the
 * string at the end of the path, truncating the string to only contain the path.
 */
//--------------------------------------------------------------------------------------------------
static void TruncateToPath
(
    char* str
)
{
    char* endPtr;

    if (str[0] == '"')
    {
        // The path is quoted so terminate the string at the next quote.
        endPtr = strchr(str + 1, '"');

        if (endPtr != NULL)
        {
            *(endPtr + 1) = '\0';
        }
    }
    else
    {
        // Find the first whitespace character.
        int i = 0;

        while (str[i] != '\0')
        {
            if (isspace(str[i]) != 0)
            {
                // Terminate the string at the first whitespace character.
                str[i] = '\0';
            }

            i++;
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Removes an applications sandbox.  Deletes everything in the sandbox area and the sandbox itself.
 * All processes in the sandbox must be killed prior to calling this function.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t sandbox_Remove
(
    app_Ref_t appRef                ///< [IN] The application to remove the sandbox for.
)
{
#define MAX_NUM_UNMNT_LOOPS         20

    const char* appName = app_GetName(appRef);
    const char* sandboxPath = app_GetSandboxPath(appRef);

    // Get a sanbox path with a '/' at the end that we can use for comparison when searching for
    // mount points in our sandbox.
    char sandboxPathRoot[LIMIT_MAX_PATH_BYTES];

    if (snprintf(sandboxPathRoot, sizeof(sandboxPathRoot), "%s/", sandboxPath) >= sizeof(sandboxPathRoot))
    {
        LE_ERROR("Sandbox path '%s' is too long.", sandboxPath);
        return LE_FAULT;
    }

    // Read the mounts and search for the ones in our sandbox.
    struct mntent mntEntry;
    char buf[LIMIT_MAX_MNT_ENTRY_BYTES];

    // Continue to search the mount points until no sandbox entries are found.  We do this because
    // several mounts may have been performed on the same mount point.
    bool foundEntryInSandbox;
    int numUnmountLoops = MAX_NUM_UNMNT_LOOPS;

    do
    {
        // Open /proc/mounts file to check where all the mounts are.  This sets the entry to the top
        // of the file.
        FILE* mntFilePtr = setmntent("/proc/mounts", "r");

        if (mntFilePtr == NULL)
        {
            LE_ERROR("Could not read '/proc/mounts'.");
            return LE_FAULT;
        }

        foundEntryInSandbox = false;

        while (getmntent_r(mntFilePtr, &mntEntry, buf, LIMIT_MAX_MNT_ENTRY_BYTES) != NULL)
        {
            // If neccessary modify the string to only contain the path.
            TruncateToPath(mntEntry.mnt_dir);

            // See if the path contains our sandbox path.  If it does this means that the mount
            // point is in our sandbox and should be unmounted.
            char* sandboxPathPtr = strstr(mntEntry.mnt_dir, sandboxPathRoot);

            if ( (sandboxPathPtr != NULL) &&
                 (strcmp(sandboxPathPtr, sandboxPath) != 0) )  // Do not unmount the sandbox root until after everything else is unmounted.
            {
                foundEntryInSandbox = true;
LE_INFO("Unmounting '%s'", mntEntry.mnt_dir);
                // This mount point is in our sandbox so it needs to be unmounted.  This call could
                // fail if the file was previously deleted so we do not consider that an error.
                LE_DEBUG("Unmounting %s", mntEntry.mnt_dir);
                if ( (umount(mntEntry.mnt_dir) != 0) && (errno != ENOENT) )
                {
                    LE_ERROR("Failed to unmount '%s'.  %m.", mntEntry.mnt_dir);
                    return LE_FAULT;
                }

                // Delete the file.  This call could fail if the mount point still has mounts on it
                // or the file could have been previously deleted so we do not consider these errors.
                if ( (remove(mntEntry.mnt_dir) != 0) && (errno != EBUSY) && (errno != ENOENT) )
                {
                    LE_ERROR("Could not delete file '%s'.  %m.", mntEntry.mnt_dir);
                    return LE_FAULT;
                }
            }
        }

        // Close the file descriptor.
        endmntent(mntFilePtr);

        // Use this loop counter to protect against infinite loops.
        numUnmountLoops--;

    } while ( (foundEntryInSandbox) && (numUnmountLoops > 0) );

    LE_INFO("Unmounting %s", sandboxPath);
    if ( (umount(sandboxPath) != 0) && (errno != ENOENT) && (errno != EINVAL) )
    {
        LE_ERROR("Failed to unmount '%s'.  %m.", sandboxPath);
        return LE_FAULT;
    }

    // Delete apps sandbox directory.
    if (le_dir_RemoveRecursive(sandboxPath) != LE_OK)
    {
        LE_ERROR("Could not delete folder '%s'.  %m.", sandboxPath);
        return LE_FAULT;
    }

    LE_INFO("'%s' sandbox removed.", appName);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Confines the calling process into the sandbox.
 *
 * @note Kills the calling process if there is an error.
 */
//--------------------------------------------------------------------------------------------------
void sandbox_ConfineProc
(
    const char* sandboxRootPtr, ///< [IN] Path to the sandbox root.
    uid_t uid,                  ///< [IN] The user ID the process should be set to.
    gid_t gid,                  ///< [IN] The group ID the process should be set to.
    const char* workingDirPtr   ///< [IN] The directory the process's working directory will be set
                                ///       to relative to the sandbox root.
)
{
    // @Note: The order of the following statements is important and should not be changed carelessly.

    // Change working directory.
    char homeDir[LIMIT_MAX_PATH_BYTES];
    size_t s;

    if (workingDirPtr[0] == '/')
    {
        s = snprintf(homeDir, sizeof(homeDir), "%s%s", sandboxRootPtr, workingDirPtr);
    }
    else
    {
        s = snprintf(homeDir, sizeof(homeDir), "%s/%s", sandboxRootPtr, workingDirPtr);
    }

    if (s >= sizeof(homeDir))
    {
        LE_FATAL("Working directory is too long: '%s'", homeDir);
    }

    LE_FATAL_IF(chdir(homeDir) != 0, "Could not change working directory to '%s'.  %m", homeDir);

    // Chroot to the sandbox.
    LE_FATAL_IF(chroot(sandboxRootPtr) != 0, "Could not chroot to '%s'.  %m", sandboxRootPtr);

    // Clear our supplementary groups list.
    LE_FATAL_IF(setgroups(0, NULL) == -1, "Could not set the supplementary groups list.  %m.");

    // Set our process's primary group ID.
    LE_FATAL_IF(setgid(gid) == -1, "Could not set the group ID.  %m.");

    // Set our process's user ID.  This sets all of our user IDs (real, effective, saved).  This
    // call also clears all cababilities.  This function in particular MUST be called after all
    // the previous system calls because once we make this call we will lose root priviledges.
    LE_FATAL_IF(setuid(uid) == -1, "Could not set the user ID.  %m.");
}
