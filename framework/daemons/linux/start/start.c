//--------------------------------------------------------------------------------------------------
/** @file start.c
 *
 * The start program is the entry point for the legato framework.  It's primary job is to select
 * a system (under /legato/systems) to run and run it.
 *
 * If there is a new image in /mnt/legato, it will be made the current system (marked "good").
 * Otherwise, it will select the newest non-bad system to run, where "non-bad" means its
 * status file has valid contents that are not either "bad" or "tried N", where N is > MAX_TRIES.
 *
 * Each time a system that is not yet known to be "good" is started, its "tries N" count is
 * incremented.
 *
 * Once the running system's Supervisor indicates that it has finished its start sequence,
 * the start program will daemonize itself so that the init scripts can continue running.
 * It stays in the foreground in the meantime to allow the Legato system to get up and running
 * as soon as possible, without having to contend for CPU and flash bandwidth with other less
 * time-critical things.
 *
 * When the system is running, the start program remains alive so it can listen for the
 * death of the Supervisor.  If the Supervisor exits, the status is checked and the start
 * program either exits or selects a system to run again.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "start.h"
#include "limit.h"
#include "file.h"
#include "dir.h"
#include "installer.h"
#include "fileDescriptor.h"
#include "smack.h"
#include "daemon.h"
#include "fileSystem.h"
#include "sysPaths.h"
#include "sysStatus.h"
#include "ima.h"
#include <mntent.h>
#include <linux/limits.h>
#include <dlfcn.h>

/// Default DAC permissions for directory creation.
#define DEFAULT_PERMS (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)

/// Initalize function for liblegato called when liblegato is linked statically.
extern void InitFramework(void);

//--------------------------------------------------------------------------------------------------
/**
 * MAX_TRIES denotes the maximum number of times a new system can be tried (unless it becomes
 * marked "good") before it is reverted.
 *
 * It is also the maximum number of times in a row a good system will be rebooted before reverting
 * to the golden master.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_TRIES 4

//--------------------------------------------------------------------------------------------------
/**
 * Times before this mean time is unreliable.
 *
 * Time is chosen to be slightly after BIOS reset time of Jan 1 1980
 */
//--------------------------------------------------------------------------------------------------
#define BIOS_RESET_TIME      315532900


//--------------------------------------------------------------------------------------------------
/**
 * Any boots slower than 70s are not boot loops.
 *
 * Expected maximum time before boot loop is 30s to boot Legato + 30s runtime.
 */
//--------------------------------------------------------------------------------------------------
#define BOOT_LOOP_TIME              70

//--------------------------------------------------------------------------------------------------
/**
 * return values for status test function
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    STATUS_GOOD,        ///< System is in "good" state
    STATUS_BAD,         ///< System is bad and should be reverted.
    STATUS_TRYABLE,     ///< System has been tried fewer than MAX_TRIES times.
}
SystemStatus_t;

//--------------------------------------------------------------------------------------------------
/**
 * Reset reasons.  Copied from le_info.api; must be kept in sync with those.
 *
 * Cannot use le_info.api directly here as we are at a lower layer.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_INFO_RESET_UNKNOWN = 0,
        ///< Unknown
    LE_INFO_RESET_USER = 1,
        ///< User request
    LE_INFO_RESET_HARD = 2,
        ///< Hardware switch
    LE_INFO_RESET_UPDATE = 3,
        ///< Software update
    LE_INFO_RESET_CRASH = 4,
        ///< Software crash
    LE_INFO_POWER_DOWN = 5,
        ///< Power Down
    LE_INFO_VOLT_CRIT = 6,
        ///< Power Down due to a critical voltage level
    LE_INFO_TEMP_CRIT = 7
        ///< Power Down due to a critical temperature level
}
le_info_Reset_t;

//--------------------------------------------------------------------------------------------------
/**
 * A collection of meaningful paths in the system
 */
//--------------------------------------------------------------------------------------------------
static const char SystemsDir[] = "/legato/systems";
static const char CurrentSystemDir[] = "/legato/systems/current";
static const char AppsDir[] = "/legato/apps";
static const char SystemsUnpackDir[] = "/legato/systems/unpack";
static const char AppsUnpackDir[] = "/legato/apps/unpack";
static const char OldFwDir[] = "/mnt/flash/opt/legato";

static const char LdconfigNotDoneMarkerFile[] = "/legato/systems/needs_ldconfig";
static const char GoldenVersionFile[] = "/mnt/legato/system/version";
static const char CurrentVersionFile[] = "/legato/systems/current/version";

static const char NoRebootFile[] = "/tmp/legato/.DEBUG_NO_REBOOT";

static const char GoldenAppsPath[] = "/mnt/legato/apps/";
static const char GoldenModemService[] = "/mnt/legato/system/apps/modemService";
static const char ModemPAPath[] = "/read-only/lib/libComponent_le_pa.so";

//--------------------------------------------------------------------------------------------------
/**
 * Portion of the path to ld.so.conf/ld.so.cache.
 */
//--------------------------------------------------------------------------------------------------
#define LDSO_ROOT_FILE  "/tmp/ld.so"

//--------------------------------------------------------------------------------------------------
/**
 * Path to obsolete legato lib directory.
 */
//--------------------------------------------------------------------------------------------------
#define MNT_LIB_DIR     "/mnt/legato/system/lib"

//--------------------------------------------------------------------------------------------------
/**
 * Pointer to modem PA (assuming modem PA exists).
 */
//--------------------------------------------------------------------------------------------------
static void* ModemPASoPtr;

//--------------------------------------------------------------------------------------------------
/**
 * Whether loading modem PA resulted in timeout
 */
//--------------------------------------------------------------------------------------------------
static bool IsModemPATimedOut = false;

//--------------------------------------------------------------------------------------------------
/**
 * The current start program being used.
 **/
//--------------------------------------------------------------------------------------------------
static const char* CurrentStartVersion = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Semaphore to synchronize with Modem PA loading thread.
 */
//--------------------------------------------------------------------------------------------------
static le_sem_Ref_t ModemPASemRef;

//--------------------------------------------------------------------------------------------------
/**
 * Fix up the contents of ld.so.conf to remove an obsolete and problematic line.
 */
//--------------------------------------------------------------------------------------------------
static void FixLdSoConf
(
    void
)
{
    // Remove the /mnt/legato/system/lib entry from ld.so.conf since by the time supervisor is
    // starting, it is no longer needed anyway.
    int rc = system(
        "if /bin/grep '" MNT_LIB_DIR "' " LDSO_ROOT_FILE ".conf; then\n"
        "    /bin/umount -l /etc/ld.so.conf > /dev/null 2>&1\n"
        "    /bin/umount -l /etc/ld.so.cache > /dev/null 2>&1\n"
        "    /bin/sed -i '\\;" MNT_LIB_DIR ";d' " LDSO_ROOT_FILE ".conf\n"
        "    /usr/sbin/update-ld-cache\n"
        "fi\n"
        "exit 0"
    );

    if (!WIFEXITED(rc) || (WEXITSTATUS(rc) == 1))
    {
        // If we removed the obsolete line, we restart the supervisor to make sure it starts up with
        // the correct libraries.
        exit(EXIT_FAILURE);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Load the golden system modem PA
 */
//--------------------------------------------------------------------------------------------------
static void* LoadModemPA
(
    void* contextPtr
)
{
    char ModemPA[PATH_MAX];
    char GoldenSymlink[PATH_MAX] = "";

    // Build up the path to the golden system's modem PA.  Use the golden system's PA to
    // ensure it is compatible with this code.
    ssize_t size = readlink(GoldenModemService, GoldenSymlink, sizeof(GoldenSymlink) - 1);

    if (size < 0) {
        LE_INFO("Unable to read %s symlink", GoldenModemService);
        goto done;
    }

    // Find app hash -- link will be to current system, so may be broken if current is
    // not the golden system.
    const char *appHash = strrchr(GoldenSymlink, '/');

    if (!appHash) {
        LE_INFO("%s -> %s symlink doesn't contain slash", GoldenModemService, GoldenSymlink);
        goto done;
    }

    appHash++;

    // Assemble modem PA path in the golden system.
    if (snprintf(ModemPA, sizeof(ModemPA), "%s%s%s", GoldenAppsPath, appHash, ModemPAPath)
        >= sizeof(ModemPA))
    {
        LE_INFO("Path %s%s%s exceeds PATH_MAX", GoldenAppsPath, appHash, ModemPAPath);
        goto done;
    }

    LE_INFO("Trying to open modem PA %s", ModemPA);

    ModemPASoPtr = dlopen(ModemPA, RTLD_LAZY);

    if (NULL == ModemPASoPtr)
    {
        LE_INFO("Could not open %s.", ModemPAPath);
        LE_INFO("%s", dlerror());
    }

done:
    if (!ModemPASoPtr)
    {
        LE_WARN("Cannot open modem PA; hardware state detection disabled");
    }

    LE_INFO("Modem PA is loaded - signaling to the main thread.");
    le_sem_Post(ModemPASemRef);

    // Starting the event receiving loop
    le_event_RunLoop();

    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Check if system reboot is due to hardware reason (under voltage, over temperature)
 */
//--------------------------------------------------------------------------------------------------
static bool IsHardwareFaultReset
(
    void
)
{
    static int (*pa_info_GetResetInformation)(int*, char*, size_t) = NULL;
    int resetCode;
    char resetReason[64];

    if (IsModemPATimedOut || !ModemPASoPtr)
    {
        // No way to reset reason information -- assume not a hardware fault.
        return false;
    }

    pa_info_GetResetInformation = dlsym(ModemPASoPtr, "pa_info_GetResetInformation");
    if (!pa_info_GetResetInformation)
    {
        LE_WARN("Could not get function pa_info_GetResetInformation.");
        LE_INFO("%s", dlerror());
        return false;
    }

    if (pa_info_GetResetInformation(&resetCode, resetReason, sizeof(resetReason)))
    {
        return false;
    }
    LE_INFO("Checking reset reason %d", resetCode);

    return (resetCode == LE_INFO_VOLT_CRIT) || (resetCode == LE_INFO_TEMP_CRIT);
}


//--------------------------------------------------------------------------------------------------
/**
 * Check if a file exists and is a regular file.
 *
 * @return
 *          True if file exists, else
 *          False
 */
//--------------------------------------------------------------------------------------------------
static inline bool FileExists
(
    const char* path
)
{
    return file_Exists(path);
}


//--------------------------------------------------------------------------------------------------
/**
 * Check if a directory exists.
 *
 * @return
 *          True if directory exists, else
 *          False
 */
//--------------------------------------------------------------------------------------------------
static inline bool DirExists
(
    const char* path
)
{
    return le_dir_IsDir(path);
}


//--------------------------------------------------------------------------------------------------
/**
 *  Check whether a directory entry is a directory or not.
 *
 *  @return
 *        True if specified entry is a directory
 *        False otherwise.
 */
//--------------------------------------------------------------------------------------------------
static bool IsDir
(
    struct dirent* dirEntryPtr              ///< [IN] Directory entry in question.
)
{
    if (dirEntryPtr->d_type == DT_DIR)
    {
        return true;
    }
    else if (dirEntryPtr->d_type == DT_UNKNOWN)
    {
        // As per man page (http://man7.org/linux/man-pages/man3/readdir.3.html), DT_UNKNOWN
        // should be handled properly for portability purpose. Use lstat(2) to check file info.
        struct stat stbuf;

        if (lstat(dirEntryPtr->d_name, &stbuf) != 0)
        {
            LE_ERROR("Error when trying to lstat '%s'. (%m)", dirEntryPtr->d_name);
            return false;
        }

        return S_ISDIR(stbuf.st_mode);
    }

    return false;
}


//--------------------------------------------------------------------------------------------------
/**
 * Recursively remove a directory but don't follow links and don't cross mount points.
 */
//--------------------------------------------------------------------------------------------------
static void RecursiveDelete
(
    const char* path
)
{
    LE_CRIT_IF(le_dir_RemoveRecursive(path) != LE_OK,
               "Failed to recursively delete '%s'.",
               path);
}

//--------------------------------------------------------------------------------------------------
/**
 * Delete the unpack dir and its contents.
 * It is not an error if there is no unpack to delete and nor does a failure to
 * delete preclude us from trying to start up a system.
 */
//--------------------------------------------------------------------------------------------------
static void DeleteSystemUnpack
(
    void
)
{
    RecursiveDelete(SystemsUnpackDir);
}

//--------------------------------------------------------------------------------------------------
/**
 * Delete the apps unpack directory
 */
//--------------------------------------------------------------------------------------------------
static void DeleteAppsUnpack
(
    void
)
{
    RecursiveDelete(AppsUnpackDir);
}

//--------------------------------------------------------------------------------------------------
/**
 * Given a system index, create the path to that system in the buffer given by systemPath ensuring
 * that the name does not exceed size.
 */
//--------------------------------------------------------------------------------------------------
static void CreateSystemPathName
(
    int index,
    char* systemPath,
    size_t size
)
{
    if (snprintf(systemPath, size, "%s/%d", SystemsDir, index) >= size)
    {
        LE_FATAL("Path to system too long");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the path to the status file in a given system (given the system name). Store into the buffer
 * given by buffPtr ensuring that it does not exceed size.
 */
//--------------------------------------------------------------------------------------------------
static void CreateStatusFilePath
(
    const char* systemName, ///< "current", "unpack", "0", "1", etc.
    char* buffPtr,
    size_t size
)
{
    if (snprintf(buffPtr, size, "/legato/systems/%s/status", systemName) >= size)
    {
        LE_FATAL("Status file path too long for buffer of size %zu", size);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Create a file named fileName (or truncate any such existing file) and write bufferSize
 * bytes of data from the given buffer, then close the file.
 *
 * @return Number of bytes written or -1 on error.
 */
//--------------------------------------------------------------------------------------------------
static int WriteToFile
(
    const char* fileName,
    const char* buffer,
    size_t bufferSize
)
{
    int written = 0;
    int fd;
    char tempFileName[PATH_MAX];

    // first write to a temporary file
    snprintf(tempFileName, PATH_MAX - 1, "%s-", fileName);

    do
    {
        fd = open(tempFileName, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    } while (fd == -1 && errno == EINTR);

    if (fd == -1)
    {
        LE_CRIT("Failed (%m) to open file for writing: '%s' .", fileName);
        return -1;
    }

    while (written < bufferSize)
    {
        int n = write(fd, buffer + written, bufferSize - written);

        if (n < 0)
        {
            if ((errno == EINTR) || (errno == EAGAIN))
            {
                continue;
            }

            LE_ERROR("Couldn't (%m) write to file '%s'", fileName);

            fd_Close(fd);

            return -1;
        }

        written += n;
    }

    fd_Close(fd);

    // Then rename to the real file.  This ensures writes to the file appear atomic.
    if (-1 == rename(tempFileName, fileName))
    {
        LE_ERROR("Error renaming temporary file '%s' to '%s'", tempFileName, fileName);
        return -1;
    }

    return written;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read up to one less than size characters from a file into a buffer provided.
 *
 * Always null-terminates the buffer.
 *
 * @return
 *      -1 fail (errno set to ENOENT if file does not exist)
 *       # of bytes read.
 */
//--------------------------------------------------------------------------------------------------
static int ReadFromFile
(
    const char* filePath,
    char* buffer,   ///< [OUT] Ptr to the buffer to store results into.
    size_t size     ///< Size of the buffer in bytes.
)
{
    int len = -1;

    int fd = open(filePath, O_RDONLY);

    if (fd != -1)
    {
        len = 0;

        while (1)
        {
            int readResult = read(fd, buffer + len, size - len);

            if ((readResult == -1) && (errno == EINTR))
            {
                continue;
            }

            if (readResult == -1)   // Error reading.
            {
                // Terminate the buffer for safety's sake.
                buffer[len] = '\0';

                LE_ERROR("Failed (%m) to read from file '%s'.", filePath);

                len = -1;

                break;
            }

            if (readResult == 0)    // End of file
            {
                buffer[len] = '\0';
                break;
            }

            len += readResult;

            if (len >= size)
            {
                // read enough
                buffer[size - 1] = '\0';    // Null-terminate the buffer.
                break;
            }
        }

        fd_Close(fd);
    }
    else
    {
        buffer[0] = '\0';   // Null terminate the buffer for safety's sake.
    }

    return len;
}


//--------------------------------------------------------------------------------------------------
/**
 * Read the index for the given system from it's index file
 *
 * @return the index, or -1 if failed.
 */
//--------------------------------------------------------------------------------------------------
static int ReadIndexFile
(
    const char* systemDirPath ///< Name of the system directory (e.g., "0", "1", "current").
)
{
    int index = -1;
    char inputBuffer[128];
    char indexFile[PATH_MAX];

    LE_ASSERT(snprintf(indexFile, sizeof(indexFile), "%s/%s/index", SystemsDir, systemDirPath)
              < sizeof(indexFile));

    if (ReadFromFile(indexFile, inputBuffer, sizeof(inputBuffer)) > 0)
    {
        // Some bytes were read. Try to get a number out of them!
        if (le_utf8_ParseInt(&index, inputBuffer) != LE_OK)
        {
            LE_ERROR("Invalid system index '%s' in '%s'.", inputBuffer, indexFile);
            index = -1;
        }
    }
    else
    {
        LE_ERROR("Unable to read from file '%s' (%m).", indexFile);
    }

    return index;
}


//--------------------------------------------------------------------------------------------------
/**
 * Create a directory.  Log an error and exit if unsuccessful.  Do nothing if the directory
 * already exists.
 **/
//--------------------------------------------------------------------------------------------------
static void MakeDir
(
    const char* dirPath
)
{
    le_result_t result = dir_MakeSmack(dirPath, DEFAULT_PERMS, "framework");

    if ((result != LE_OK) && (result != LE_DUPLICATE))
    {
        LE_FATAL("Failed (%m) to create directory '%s'", dirPath);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Write the index for this new install into the index file in the unpack dir
 */
//--------------------------------------------------------------------------------------------------
static void WriteUnpackIndexFile
(
    int newIndex
)
{
    char indexFileBuffer[PATH_MAX];
    char indexString[256];

    LE_ASSERT(snprintf(indexFileBuffer, sizeof(indexFileBuffer), "%s/index", SystemsUnpackDir)
              < sizeof(indexFileBuffer));

    LE_ASSERT(snprintf(indexString, sizeof(indexString), "%d", newIndex) < sizeof(indexString));

    // If this fails, there's not much we can do about it.
    (void)WriteToFile(indexFileBuffer, indexString, strlen(indexString));
}


//--------------------------------------------------------------------------------------------------
/**
 * Mark the system in the unpack directory as good. This system has not actually been tried but
 * since we are in the start program we know that it has been set up from the built in system and is
 * therefore assumed de facto good.
 */
//--------------------------------------------------------------------------------------------------
static void MarkUnpackGood
(
    void
)
{
    char statusFilePath[PATH_MAX];

    CreateStatusFilePath("unpack", statusFilePath, sizeof(statusFilePath));

    if (WriteToFile(statusFilePath, "good", 4) < 0)
    {
        exit(EXIT_FAILURE);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Create a fresh legato directory structure in the unpack directory and symlink the correct
 * paths from /mnt/legato
 */
//--------------------------------------------------------------------------------------------------
static void MakeUnpackDirFromGolden
(
    int index ///< Index to use for this system.
)
{
    // Create directories.
    MakeDir("/legato/systems");
    MakeDir("/legato/systems/unpack");
    MakeDir("/legato/systems/unpack/config");
    MakeDir("/legato/systems/unpack/apps");
    MakeDir("/legato/systems/unpack/appsWriteable");

    // Create symlinks:
    if (symlink("/mnt/legato/system/bin", "/legato/systems/unpack/bin") ||
        symlink("/mnt/legato/system/lib", "/legato/systems/unpack/lib") ||
        symlink("/mnt/legato/system/modules", "/legato/systems/unpack/modules") ||
        symlink("/mnt/legato/system/config/apps.cfg", "/legato/systems/unpack/config/apps.cfg") ||
        symlink("/mnt/legato/system/config/users.cfg", "/legato/systems/unpack/config/users.cfg") ||
        symlink("/mnt/legato/system/config/modules.cfg",
                "/legato/systems/unpack/config/modules.cfg") ||
        symlink("/mnt/legato/system/config/framework.cfg",
                "/legato/systems/unpack/config/framework.cfg"))
    {
        LE_FATAL("Could not create symlinks (%m)");
    }

    // Copy files:
    if (   (file_Copy("/mnt/legato/system/version",
                      "/legato/systems/unpack/version",
                      NULL) != LE_OK)
        || (file_Copy("/mnt/legato/system/info.properties",
                      "/legato/systems/unpack/info.properties",
                      NULL) != LE_OK)   )
    {
        LE_FATAL("Could not copy needed files");
    }

    // Write the index into the system.
    WriteUnpackIndexFile(index);

    // Mark the system "good".
    MarkUnpackGood();

}


//--------------------------------------------------------------------------------------------------
/**
 * Copy the previous system's configuration trees into the new system config directory.
 */
//--------------------------------------------------------------------------------------------------
static void ImportOldConfigTrees
(
    int oldIndex,  ///< Index of system to fetch old config. Nothing will be copied if it is negative.
    int newIndex   ///< Index of new system to transfer. Negative value for system unpack directory.
)
{
    if (oldIndex > -1)
    {
        char srcDir[PATH_MAX];
        char destDir[PATH_MAX];

        if (newIndex <= -1)
        {
            LE_ASSERT(snprintf(destDir, sizeof(destDir), "%s/config", SystemsUnpackDir)
                  < sizeof(destDir));
        }
        else
        {
            LE_ASSERT(snprintf(destDir, sizeof(destDir), "%s/%d/config", SystemsDir, newIndex)
                     < sizeof(destDir));
        }

        LE_ASSERT(snprintf(srcDir, sizeof(srcDir), "%s/%d/config", SystemsDir, oldIndex)
                  < sizeof(srcDir));

        file_CopyRecursive(srcDir, destDir, NULL);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Delete all systems except for the current one.
 */
//--------------------------------------------------------------------------------------------------
static void DeleteAllButCurrent
(
    void
)
{
    // Remove any old-style firmware.
    if (DirExists(OldFwDir))
    {
        RecursiveDelete(OldFwDir);
    }

    // Delete any non-current systems in /legato.
    DIR* d = opendir(SystemsDir);

    if (d == NULL)
    {
        LE_CRIT("Cannot open directory '%s': %m", SystemsDir);
        return;
    }

    for (;;)
    {
        struct dirent* entry;

        entry = readdir(d);

        if (entry == NULL)
        {
            if (errno != 0)
            {
                LE_ERROR("Failed to read directory entry from '%s': %m", SystemsDir);
            }

            break;
        }

        // For every directory other than "current" or anything starting with a '.',
        if (   (IsDir(entry))
            && (entry->d_name[0] != '.')
            && (strcmp(entry->d_name, "current") != 0))
        {
            // Delete the directory and all its contents.
            char path[PATH_MAX];
            LE_ASSERT(snprintf(path, sizeof(path), "%s/%s", SystemsDir, entry->d_name)
                      < sizeof(path));

            // Attempt to umount the system because it may have been mounted when
            // sandboxed apps were created.
            fs_TryLazyUmount(path);

            RecursiveDelete(path);
        }
    }

    // Close the directory.
    if (closedir(d))
    {
        LE_CRIT("Could not close '%s': %m", SystemsDir);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Rename a file or directory.  If toName already exists, delete it first.
 */
//--------------------------------------------------------------------------------------------------
static void Rename
(
    const char* fromName,
    const char* toName
)
{
    if (rename(fromName, toName) == -1)
    {
        if ((errno == ENOTEMPTY) || (errno == EISDIR))
        {
            // The old name is a non empty directory. Blow it away.
            LE_WARN("Destination '%s' exists. Deleting it.", toName);
            RecursiveDelete(toName);

            // Try again.
            if (rename(fromName, toName) == -1)
            {
                LE_FATAL("Cannot rename '%s' to %s: %m", fromName, toName);
            }
        }
        else
        {
            // don't know how to handle anything else.
            LE_FATAL("Cannot rename directory '%s' to %s: %m", fromName, toName);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * create the ld.so.cache for the new install (or reversion).
 */
//--------------------------------------------------------------------------------------------------
static void UpdateLdSoCache
(
    const char* systemPath
)
{
    int rc = 0;
    const char* text;
    // create marker file to say we are doing ldconfig
    text = "start_ldconfig";
    // If this fails, try to limp along anyway.
    (void)WriteToFile(LdconfigNotDoneMarkerFile, text, strlen(text));

    if (FileExists("/usr/sbin/update-ld-cache"))
    {
        rc = system("/usr/sbin/update-ld-cache /legato/systems/current/lib > /dev/null");
    }
    else
    {
        // append /legato/systems/current/lib to /etc/ld.so.conf if it is not present. This path is
        // added at the end of the file to preserve the current paths set.
        rc = system("/bin/grep -q '^/legato/systems/current/lib$' /etc/ld.so.conf 2>/dev/null || "
                    "/bin/echo /legato/systems/current/lib >>/etc/ld.so.conf");
        if (!WIFEXITED(rc) || !WEXITSTATUS(rc))
        {
            LE_ERROR("Add of path /legato/systems/current/lib to /etc/ld.so.conf fails: %d",
                     WEXITSTATUS(rc));
        }

        rc = system("/sbin/ldconfig > /dev/null");
    }

    // If this fails, the system probably won't work, but not much we can do but try.
    if (!WIFEXITED(rc) || !WEXITSTATUS(rc))
    {
        unlink(LdconfigNotDoneMarkerFile);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Create mark indicating that ldconfig is required before we start the system
 */
//--------------------------------------------------------------------------------------------------
static void RequestLdSoConfig
(
    void
)
{
    const char* text;
    text = "need_ldconfig";
    // If this fails, try to limp along anyway.
    (void)WriteToFile(LdconfigNotDoneMarkerFile, text, strlen(text));
}


//--------------------------------------------------------------------------------------------------
/**
 * Attempt to get the writeable files for an app from an old, legacy system in /opt/legato, copy
 * them into the system unpack directory and then update according to the version of the app that
 * is supposed to be in the system.
 **/
//--------------------------------------------------------------------------------------------------
static void GetAppWriteableFilesFromOptLegato
(
    const char* appHash,
    const char* appName,
    const char* smackLabel
)
{
    char oldAppPath[PATH_MAX];

    LE_ASSERT(snprintf(oldAppPath, sizeof(oldAppPath), "%s/appName", OldFwDir) < sizeof(oldAppPath));

    if (DirExists(oldAppPath))
    {
        char dest[PATH_MAX];
        LE_ASSERT(snprintf(dest, sizeof(dest), "/legato/systems/unpack/appsWriteable/%s", appName)
                  < sizeof(dest));

        file_CopyRecursive(oldAppPath, dest, smackLabel);

        installer_UpdateAppWriteableFiles("unpack", appHash, appName);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Create the required directories and links to install an app in the system and import config
 * and writeable files.
 */
//--------------------------------------------------------------------------------------------------
static void SetUpApp
(
    const char* appName,
    int previousSystemIndex
)
{
    // Get the app's hash from the symlink under /mnt/legato/system/apps/<appName>.
    char pathBuff[PATH_MAX];
    char hashBuff[LIMIT_MD5_STR_BYTES];

    LE_ASSERT(snprintf(pathBuff, sizeof(pathBuff), "/mnt/legato/system/apps/%s", appName)
              < sizeof(pathBuff));

    installer_GetAppHashFromSymlink(pathBuff, hashBuff);

    char installedAppPath[PATH_MAX];

    LE_ASSERT(snprintf(pathBuff, sizeof(pathBuff), "/legato/systems/unpack/apps/%s", appName)
              < sizeof(pathBuff));

    // If the app is in "Preloaded Any Version" mode, then the application directory
    // must be inherited from the previous system.
    if (0 == strncmp(hashBuff, PRELOADED_ANY_VERSION, sizeof(hashBuff)))
    {
        bool isFound = false;
        char linkPathBuff[PATH_MAX];
        char linkContentBuff[PATH_MAX];

        if (previousSystemIndex < 0)
        {
            LE_CRIT("Preloaded app %s not found: no previous system!", appName);
            // App not found, further actions aren't applicable
            return;
        }
        LE_ASSERT(snprintf(linkPathBuff,
                           sizeof(linkPathBuff),
                           "/legato/systems/%d/apps/%s",
                           previousSystemIndex,
                           appName)
                           < sizeof(linkPathBuff));

        // Read the content of the symlink pointing to app directory
        ssize_t bytesRead = readlink(linkPathBuff, linkContentBuff, sizeof(linkContentBuff) - 1);
        if (bytesRead < 0)
        {
            LE_ERROR("Error resolving symlink %s", linkPathBuff);
        }
        else if (bytesRead >= sizeof(linkContentBuff))
        {
            LE_ERROR("Contents of symlink %s too long (> %zu).",
                     linkPathBuff, sizeof(linkContentBuff) - 1);
        }
        else
        {
            // Null-terminate the string.
            linkContentBuff[bytesRead] = '\0';
            LE_INFO("Preloaded app %s: found link %s", appName, linkContentBuff);

            // Update the app hash buffer
            LE_ASSERT(le_utf8_Copy(hashBuff,
                                   le_path_GetBasenamePtr(linkContentBuff, "/"),
                                   sizeof(hashBuff),
                                   NULL) == LE_OK);
            isFound = true;
        }
        if (!isFound)
        {
            LE_CRIT("Preloaded app %s not found!", appName);
            // App not found, further actions aren't applicable
            return;
        }
    }

    LE_ASSERT(snprintf(installedAppPath, sizeof(installedAppPath), "/legato/apps/%s", hashBuff)
              < sizeof(installedAppPath));

    // Create a symlink to /legato/apps/<hash> from /legato/systems/unpack/apps/<appName>.
    if (symlink(installedAppPath, pathBuff) != 0)
    {
        LE_CRIT("Failed to create symlink '%s' pointing to '%s': %m.", pathBuff, installedAppPath);
    }

    // If the app isn't already installed in /legato/apps/<hash>,
    // create a symlink: /legato/apps/<hash> -> /mnt/legato/apps/<hash>
    if (!DirExists(installedAppPath))
    {
        // Create a symlink from /legato/apps/<hash> to the "golden" app in /mnt/legato.
        LE_ASSERT(snprintf(pathBuff, sizeof(pathBuff), "/mnt/legato/apps/%s", hashBuff)
                  < sizeof(pathBuff));

        if (symlink(pathBuff, installedAppPath) != 0)
        {
            LE_CRIT("Failed to create symlink '%s' pointing to '%s': %m.",
                    installedAppPath,
                    pathBuff);
        }
    }

    // If there's no "modern" system to copy app writeable files from, then try to get them from
    // a legacy system installed in /opt/legato.
    if (previousSystemIndex == -1)
    {
        char smackLabel[LIMIT_MAX_SMACK_LABEL_BYTES];
        smack_GetAppLabel(appName, smackLabel, sizeof(smackLabel));
        GetAppWriteableFilesFromOptLegato(hashBuff, appName, smackLabel);
    }
    else
    {
        char oldSystemName[100];
        LE_ASSERT(snprintf(oldSystemName, sizeof(oldSystemName), "%d", previousSystemIndex)
                  < sizeof(oldSystemName));
        installer_InstallAppWriteableFiles(hashBuff, appName, oldSystemName);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Install all the apps found in golden system.
 */
//--------------------------------------------------------------------------------------------------
static void InstallGoldenApps
(
    int previousSystemIndex
)
{
    MakeDir(AppsDir);   // Make sure the apps directory in /legato exists.

    // Iterate over the contents of the golden system's apps directory.
    // It should contain symlinks that need to be copied to the system unpack area.
    const char* dirName = "/mnt/legato/system/apps";

    DIR* d = opendir(dirName);

    if (d == NULL)
    {
        if (errno != ENOENT)
        {
            LE_ERROR("Cannot open directory '%s': %m", dirName);
        }

        return;
    }

    for (;;)
    {
        errno = 0;
        struct dirent* entry = readdir(d);

        if (entry == NULL)
        {
            if (errno != 0)
            {
                LE_ERROR("Failed to read directory entry from '%s': %m", dirName);
            }

            break;
        }

        // Ignore anything that starts with a '.'.
        if (entry->d_name[0] != '.')
        {
            // The directory entry name is the app name.
            SetUpApp(entry->d_name, previousSystemIndex);
        }
    }

    // Close the directory.
    if (closedir(d))
    {
        LE_ERROR("Could not close '%s': %m", dirName);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Thin wrapper to test if the buffer contains the string good
 * @return
 *          true    is buffer equals "good"
 *          false   otherwise
 */
//--------------------------------------------------------------------------------------------------
static bool IsGood
(
    const char* buff
)
{
    return (strncmp(buff, "good", 4) == 0);
}

//--------------------------------------------------------------------------------------------------
/**
 * Thin wrapper to test if the buffer contains the string bad
 * @return
 *          true    is buffer equals "bad"
 *          false   otherwise
 */
//--------------------------------------------------------------------------------------------------
static bool IsBad
(
    const char* buff
)
{
    return (strncmp(buff, "bad", 3) == 0);
}

//--------------------------------------------------------------------------------------------------
/**
 * Parse the buffer to a) determine that is is of the form "tried #" where # represents and integer
 * and if so then to parse the integer value and return it.
 * @return
 *          -1  if string does not start "tried "
 *           0  if # is zero or a non-numeric character (0 is an illegal value for tried)
 *           int representing the number of tries.
 */
//--------------------------------------------------------------------------------------------------
static int GetNumTries
(
    const char* buff
)
{
    if (strncmp(buff, "tried ", 6) == 0)
    {
        int tries;
        le_result_t result = le_utf8_ParseInt(&tries, buff + 6);

        if (result == LE_OUT_OF_RANGE)
        {
            LE_CRIT("Tried count '%s' is out of range!", buff + 6);
            return 0;
        }

        if (result != LE_OK)
        {
            LE_CRIT("Tried count is malformed ('%s')", buff + 6);
            return 0;
        }

        return tries;
    }

    return -1;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the status of the current system to indicate how many times this system has been tried.
 */
//--------------------------------------------------------------------------------------------------
static void MarkStatusTried
(
    int numTry
)
{
    int length;
    char status[100];

    length = snprintf(status, sizeof(status), "tried %d", numTry);
    LE_ASSERT(length < sizeof(status));

    char filePath[PATH_MAX];
    CreateStatusFilePath("current", filePath, sizeof(filePath));

    if (WriteToFile(filePath, status, strlen(status)) < 0)
    {
        exit(EXIT_FAILURE);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Read what is in the status file for a given system.
 *
 * @return # of bytes read. On error, -1 is returned and errno is set (ENOENT if file doesn't exist)
 */
//--------------------------------------------------------------------------------------------------
static int ReadStatus
(
    const char* systemName,  ///< E.g., "current", "unpack", "0", "1", etc.
    char* buffPtr,  ///< On success, null-terminated status string is stored here.
    size_t buffSize ///< Size of buffer pointed to by buffPtr.
)
{
    char statusPath[PATH_MAX];

    CreateStatusFilePath(systemName, statusPath, sizeof(statusPath));

    return ReadFromFile(statusPath, buffPtr, buffSize);
}


//--------------------------------------------------------------------------------------------------
/**
 * Determine if a given system's status is new, good, tryable, or bad.
 *
 * @return the status of the system.
 */
//--------------------------------------------------------------------------------------------------
static SystemStatus_t GetStatus
(
    const char* systemName,
    int* triesPtr   ///< [OUT] Ptr to where to store number of tries if STATUS_TRYABLE (or NULL).
)
{
    SystemStatus_t status = STATUS_BAD; // Assume bad unless proven otherwise.

    char buff[100];

    int readResult = ReadStatus(systemName, buff, sizeof(buff));

    if (readResult == -1)
    {
        if (errno == ENOENT)
        {
            LE_INFO("System '%s' is NEW.", systemName);

            status = STATUS_TRYABLE;

            if (triesPtr != NULL)
            {
                *triesPtr = 0;
            }
        }

        LE_ERROR("Failed to read status of system '%s' (%m).", systemName);
    }
    else
    {
        LE_INFO("Status of system '%s' is '%s'.", systemName, buff);

        if (IsGood(buff))
        {
            status = STATUS_GOOD;
        }
        else if (!IsBad(buff))
        {
            int tries = GetNumTries(buff);

            if (tries <= 0)
            {
                LE_ERROR("Something is wrong with tries in system '%s'.", systemName);
            }
            else if (tries < MAX_TRIES)
            {
                LE_INFO("System '%s' has a tried count of %d.", systemName, tries);

                status = STATUS_TRYABLE;

                if (triesPtr != NULL)
                {
                    *triesPtr = tries;
                }
            }
            else
            {
                LE_INFO("System '%s' has been tried more than %d times.", systemName, MAX_TRIES);
            }
        }
    }

    return status;
}

//--------------------------------------------------------------------------------------------------
/**
 * Determine if a system is the golden system
 */
//--------------------------------------------------------------------------------------------------
static bool IsCurrentSystemGolden
(
    void
)
{
    char currentVersion[255] = "";
    char goldenVersion[255] = "";

    // If this fails, then the system in /mnt/legato is malformed -- cannot possibly be the golden
    // system installed.
    if (ReadFromFile(GoldenVersionFile, goldenVersion, sizeof(goldenVersion)) <= 0)
    {
        LE_ERROR("System on /mnt/legato is malformed. Ignoring it.");
        return false;
    }

    // If this fails the system in /legato/systems/current is malformed -- again, not the golden
    // system
    if (ReadFromFile(CurrentVersionFile, currentVersion, sizeof(currentVersion)) <= 0)
    {
        LE_ERROR("System on /legato/systems/current is malformed.  Ignoring it.");
        return false;
    }

    return 0 == strcmp(goldenVersion, currentVersion);
}

//--------------------------------------------------------------------------------------------------
/**
 * Determine the number of consecutive reboots.
 */
//--------------------------------------------------------------------------------------------------
static int ReadBootCount
(
    void
)
{
    char bootCountBuf[64];

    if (ReadFromFile(BOOT_COUNT_PATH,  bootCountBuf, sizeof(bootCountBuf)) < 0)
    {
        // File does not exist means first consecutive boot
        return 0;
    }

    int bootCount;
    uint64_t bootTime;
    int scanned;
    scanned = sscanf(bootCountBuf, " %d %"SCNu64" ", &bootCount, &bootTime);

    if (scanned < 1)
    {
        LE_CRIT("Boot file contents '%s' malformed", bootCountBuf);
        return 0;
    }

    if (scanned == 2)
    {
        time_t now = time(NULL);

        if ((now > (BIOS_RESET_TIME)) && (now > bootTime + BOOT_LOOP_TIME))
        {
            // Too long since last boot -- not a boot loop.
            return 0;
        }
    }

    return bootCount;
}

//--------------------------------------------------------------------------------------------------
/**
 * Write the number of consecutive reboots.
 */
//--------------------------------------------------------------------------------------------------
static void WriteBootCount
(
    int bootCount
)
{
    char bootCountBuf[64];

    snprintf(bootCountBuf, sizeof(bootCountBuf), "%d %"PRIu64, bootCount, (uint64_t)time(NULL));

    if (WriteToFile(BOOT_COUNT_PATH, bootCountBuf, strlen(bootCountBuf)) < 0)
    {
        exit(EXIT_FAILURE);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * returns EXIT_FAILURE on error, otherwise, returns the exit code of the Supervisor.
 */
//--------------------------------------------------------------------------------------------------
static int TryToRun
(
    void
)
{

    // Start the Supervisor.
    pid_t supervisorPid = fork();
    if (supervisorPid == 0)
    {
        // I'm the child. Exec the Supervisor, telling it not to daemonize itself.
        const char supervisorPath[] = "/legato/systems/current/bin/supervisor";

        if (CurrentStartVersion == NULL)
        {
            (void)execl(supervisorPath, supervisorPath, "--no-daemonize", NULL);
            LE_FATAL("Failed to run '%s': %m", supervisorPath);
        }
        else
        {
            (void)execl(supervisorPath, supervisorPath, "--no-daemonize", "-v", CurrentStartVersion, NULL);
            LE_FATAL("Failed to run '%s': %m", supervisorPath);
        }
    }

    // Close our stdin so only the Supervisor has a copy of the write end of the pipe.
    // It will close this when the framework is up, which will trigger our parent process to exit.
    // Reopen our stdin to /dev/null so we can loop back around to this code later without
    // damaging anything.
    LE_FATAL_IF(freopen("/dev/null", "r", stdin) == NULL,
                "Failed to redirect stdin to /dev/null.  %m.");

    // Wait for the Supervisor to exit.
    int result;
    pid_t p = waitpid(supervisorPid, &result, 0);
    if (p != supervisorPid)
    {
        if (p == -1)
        {
            LE_FATAL("waitpid() failed: %m");
        }
        else
        {
            LE_FATAL("waitpid() returned unexpected result %d", p);
        }
    }

    if (WIFEXITED(result))
    {
        return WEXITSTATUS(result);
    }
    else if (WIFSIGNALED(result))
    {
        LE_CRIT("Supervisor was killed by a signal %d.", WTERMSIG(result));
    }
    else
    {
        LE_CRIT("Unexpected Supervisor exit status %d.", result);   // This should never happen.
    }

    return EXIT_FAILURE;
}


//--------------------------------------------------------------------------------------------------
/**
 * Scans the contents of the systems directory and finds the good, new, or tried system with the
 * highest index number.
 *
 * @return the system index or -1 if no system index found.
 */
//--------------------------------------------------------------------------------------------------
static int FindNewestSystemIndex
(
    void
)
{
    DIR* d = opendir(SystemsDir);

    if (d == NULL)
    {
        if (errno != ENOENT)
        {
            LE_ERROR("Cannot open directory '%s': %m", SystemsDir);
        }
        else
        {
            LE_ERROR("No systems yet exist in '%s'", SystemsDir);
        }

        // There is no existing system.
        return -1;
    }

    int highestIndex = -1;

    for (;;)
    {
        struct dirent* entry;

        entry = readdir(d);

        if (entry == NULL)
        {
            if (errno != 0)
            {
                LE_ERROR("Failed to read directory entry from '%s': %m", SystemsDir);
            }

            break;
        }

        // For every directory other than "unpack" or anything starting with a '.',
        if (   (IsDir(entry))
            && (entry->d_name[0] != '.')
            && (strcmp(entry->d_name, "unpack") != 0))
        {
            // Get the index from the index file.
            int index = ReadIndexFile(entry->d_name);

            // Get the status from the status file.
            SystemStatus_t status = GetStatus(entry->d_name, NULL);

            switch (status)
            {
                case STATUS_BAD:

                    // Ignore bad or malformed systems.
                    LE_WARN("System '%s' is bad.", entry->d_name);
                    break;

                case STATUS_GOOD:
                case STATUS_TRYABLE:

                    LE_INFO("System '%s' is OK.", entry->d_name);

                    if (index > highestIndex)
                    {
                        highestIndex = index;
                    }
                    break;
            }
        }
    }

    // Close the directory.
    if (closedir(d))
    {
        LE_ERROR("Could not close '%s': %m", SystemsDir);
    }

    return highestIndex;
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks if the "golden" system in /mnt/legato should be installed.
 *
 * @return
 *  - true if the "golden" system should be installed.
 *  - false if the "golden" system should not be installed.
 */
//--------------------------------------------------------------------------------------------------
static bool ShouldInstallGolden
(
    int newestVersion
)
{
    // If there's no non-bad system installed, install the golden one.
    if (newestVersion == -1)
    {
        LE_INFO("No systems are installed yet.");
        return true;
    }

    // Check the version files to determine whether the version in /mnt/flash has been
    // updated since last start-up.
    char builtInVersion[255] = "";
    char goldenVersion[255] = "";

    // NOTE: Failure will result in an empty string.
    (void)ReadFromFile("/legato/mntLegatoVersion", builtInVersion, sizeof(builtInVersion));

    // If this fails, then the system in /mnt/legato is malformed and should not be installed.
    if (ReadFromFile(GoldenVersionFile, goldenVersion, sizeof(goldenVersion)) <= 0)
    {
        LE_ERROR("System on /mnt/legato is malformed. Ignoring it.");
        return false;
    }

    if (strcmp(builtInVersion, goldenVersion) != 0)
    {
        LE_INFO("System on /mnt/legato is new. Installing it.");
        return true;
    }
    else
    {
        LE_INFO("System on /mnt/legato is old. Ignoring it.");
        return false;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Record the fact that the current contents of /mnt/legato have been installed into /legato
 * so that we won't do it again next time we start.
 *
 * @warning Do this last when installing a "golden" system from /mnt/legato.
 */
//--------------------------------------------------------------------------------------------------
static void MarkGoldenInstallComplete
(
    void
)
{
    if (file_Copy("/mnt/legato/system/version", "/legato/mntLegatoVersion", NULL) != LE_OK)
    {
        LE_ERROR("Failed to mark the 'golden' system successfully installed.");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if something is mounted on mountPoint.
 *
 * @return true if the mount point is mounted already.
 */
//--------------------------------------------------------------------------------------------------
static bool IsMounted
(
    char* mountPoint
)
{
    struct mntent* mountEntry;
    FILE* mtabFile = setmntent("/etc/mtab", "r");

    if (mtabFile == NULL)
    {
        LE_CRIT("Failed to open /etc/mtab for reading: %m");
        return false;
    }

    bool result = false;

    while ((mountEntry = getmntent(mtabFile)) != NULL)
    {
        if (strcmp(mountEntry->mnt_dir, mountPoint) == 0)
        {
            result = true;
            break;
        }
    }

    (void)endmntent(mtabFile);  // Always returns 1

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Bind mount the given path to the mount point
 */
//--------------------------------------------------------------------------------------------------
static void BindMount
(
    char* path,     ///< Directory to be mounted
    char* mountedAt ///< Where is should be mounted
)
{
    if (!IsMounted(mountedAt))
    {
        le_result_t result = le_dir_MakePath(path, DEFAULT_PERMS);

        if ((result != LE_OK) && (result != LE_DUPLICATE))
        {
            LE_ERROR("Failed to create directory '%s'", path);
        }

        if (mount(path, mountedAt, NULL, MS_BIND, NULL))
        {
            LE_FATAL("Failed (%m) to bind mount '%s' at '%s'", path, mountedAt);
        }
    }
    else
    {
        LE_WARN("'%s' is already mounted.", mountedAt);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Runs the current system.  Returns when the Supervisor exits.
 *
 * @return exit code from the Supervisor.
 */
//--------------------------------------------------------------------------------------------------
static int RunCurrentSystem
(
    void
)
{
    int exitCode = TryToRun();
    int retCode;

    switch (exitCode)
    {
        case EXIT_FAILURE:

            // Sync file systems before rebooting.
            sync();

            // Dump the last 100 lines from logread excluding Legato: { INFO, DBUG,-WRN-} to
            // prevent pollution by DEBUG, INFO or WRN messages.
            retCode = system("logread | "
                             "egrep -v 'Legato: [ -][IDW][NBR][FUN][OG-]' | "
                             "tail -n 100 > /dev/console");
            if (WIFEXITED(retCode) && (0 == WEXITSTATUS(retCode)))
            {
                LE_DEBUG("Failed to dump logs");
            }

            // Sync again file systems before rebooting.
            sync();

            // If the file NoRebootFile is present, do not request a reboot
            if (0 == access(NoRebootFile, R_OK))
            {
                LE_FATAL("Reboot is disabled. Exit with failure");
            }

            // Try first the /sbin/reboot, less hard that the reboot(2) system call
            retCode = system("/sbin/reboot");
            if (WIFEXITED(retCode) && (0 == WEXITSTATUS(retCode)))
            {
                LE_FATAL("System will reboot now !");
            }
            // Reboot the system.
            else if (reboot(RB_AUTOBOOT) == -1)
            {
                struct timespec waitBeforeReboot = { 2, 0 };

                LE_EMERG("Failed to reboot. Errno = %s.", strerror(errno));
                // Last chance ! Use /proc/sysrq entries to force reboot...
                // If this fails, no way to reboot from Legato side.
                nanosleep(&waitBeforeReboot, NULL);
                retCode = system("echo 1 > /proc/sys/kernel/sysrq; "
                                 "echo b > /proc/sysrq-trigger");
                LE_FATAL("Failed to reboot using /proc/sysrq-trigger: retCode = %x", retCode);
            }
            else
            {
                LE_FATAL("Failed to reboot. Errno = Success?!");
            }

        case EXIT_SUCCESS:

            LE_INFO("Supervisor exited with EXIT_SUCCESS.  Legato framework stopped.");

            exit(EXIT_SUCCESS);

        case LE_START_EXIT_RESTART:

            LE_INFO("Supervisor exited with EXIT_RESTART(2).  Legato framework restarting.");
            break;

        case LE_START_EXIT_MANUAL_RESTART:

            LE_INFO("Supervisor exited with EXIT_MANUAL_RESTART(3).  Legato framework restarting.");
            break;

        default:

            LE_CRIT("Unexpected exit code (%d) from the Supervisor.", exitCode);
            break;
    }

    // Returning from this function will loop back around and select the appropriate system,
    // incrementing the try counter if appropriate, or reverting if necessary.
    return exitCode;
}


//--------------------------------------------------------------------------------------------------
/**
 * Make a given system into the current system.
 **/
//--------------------------------------------------------------------------------------------------
static void SetCurrent
(
    int newCurrentIndex     ///< Index of the system to be made current.
)
{
    LE_INFO("Selecting system %d.", newCurrentIndex);

    char path[PATH_MAX];
    CreateSystemPathName(newCurrentIndex, path, sizeof(path));

    // Attempt to umount the system because it may have been mounted when
    // sandboxed apps were created.
    fs_TryLazyUmount(path);

    Rename(path, CurrentSystemDir);

    // Before the new current system starts, the dynamic linker's cache must be updated so
    // the system's libraries can be found easily.
    RequestLdSoConfig();
}


//--------------------------------------------------------------------------------------------------
/**
 * Check the status and if everything looks good to go, get the ball rolling, else revert!
 */
//--------------------------------------------------------------------------------------------------
static void Launch
(
    bool isReadOnly
)
{
    static int lastExitCode = EXIT_FAILURE; // Treat a reboot as a fault.
    int bootCount;

    int tries;

    switch (GetStatus("current", &tries))
    {
        case STATUS_TRYABLE:
            // If the supervisor exited with exit code 3 then don't
            // increment the try count, unless the system is new (untried).
            // This means that "legato restart" was used.
            if ((lastExitCode != LE_START_EXIT_MANUAL_RESTART) || (tries == 0))
            {
                MarkStatusTried(tries + 1);
            }

            // *** FALL THROUGH ***

        case STATUS_GOOD:
            // Increment the number of times the system has been booted if system is not
            // read only.  If it is read-only, nothing we can do to recover anyway.
            if (!isReadOnly)
            {
                bootCount = ReadBootCount();
                if (lastExitCode != LE_START_EXIT_MANUAL_RESTART)
                {
                    WriteBootCount(bootCount + 1);
                }
            }

            lastExitCode = RunCurrentSystem();
            break;

        case STATUS_BAD:
        default:
            // This should never happen.  If the current system was bad, it would have been
            // deselected.
            LE_FATAL("Current system is bad!");
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Install the "golden" system in /mnt/legato as the new current system with an index
 * higher than the highest previous valid system index in /legato/systems.
 *
 * @note If there's a bad or malformed system already using that index, that old system will be
 *       deleted first to make way.
 *
 * @return The index of the newly installed golden system.
 */
//--------------------------------------------------------------------------------------------------
static int InstallGolden
(
    int newestIndex,    ///< Index of newest non-bad system in /legato/systems, or -1 if none.
    int currentIndex    ///< Index of current system, or -1 if none.
)
{
    int goldenIndex = newestIndex + 1;

    // Make sure there's nothing in the way.
    char path[PATH_MAX];
    CreateSystemPathName(goldenIndex, path, sizeof(path));
    RecursiveDelete(path);

    // If there is a current system directory, rename it to its index.
    if (currentIndex > -1)
    {
        char pathBuffer[PATH_MAX];
        CreateSystemPathName(currentIndex, pathBuffer, sizeof(pathBuffer));

        // Attempt to umount the system because it may have been mounted when
        // sandboxed apps were created.
        fs_TryLazyUmount(CurrentSystemDir);

        Rename(CurrentSystemDir, pathBuffer);
    }

    // Create the system unpack directory and copy /mnt/legato/system there.
    MakeUnpackDirFromGolden(goldenIndex);

    // Import the old configuration trees into the unpack area.
    ImportOldConfigTrees(newestIndex, -1);

    // Install apps into /legato and the system unpack area.
    InstallGoldenApps(newestIndex);

    // Make the golden system the new current system.
    Rename(SystemsUnpackDir, CurrentSystemDir);

    // Delete old stuff we don't need anymore.
    DeleteAllButCurrent();

    // Before the new current system starts, the dynamic linker's cache must be updated so
    // the system's libraries can be found easily.
    RequestLdSoConfig();

    // Remove boot count -- restart from 0 when installing a new golden image.
    unlink(BOOT_COUNT_PATH);

    // Flush to disk before marking golden install as complete.
    sync();

    // Remember what we just installed so we don't do it again.
    // DO THIS LAST.
    MarkGoldenInstallComplete();

    return goldenIndex;
}

//--------------------------------------------------------------------------------------------------
/**
 * Traverse the current system directory and install all public certificates
 *
 * @returns
 *      - LE_OK if installation of all public certificate passes.
 *      - LE_FAULT if any error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t InstallCurrentSystemCert
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    char path[PATH_MAX] = "";
    snprintf(path, sizeof(path), "%s/%s", CURRENT_SYSTEM_PATH, PUB_CERT_NAME );

    // There is a chance that no certificate may exists in system  directory (e.g. golden image).
    // So don't throw any error if no certificate exists in system directory.

    // Look for certificate in current system directory. If any certificate exists, import it.
    if (file_Exists(path))
    {
        if (LE_OK != ima_ImportPublicCert(path))
        {
            LE_CRIT("Failed to import public certificate '%s'", path);
            return LE_FAULT;
        }
    }

    char* pathArrayPtr[] = {(char *)APPS_INSTALL_DIR,
                            NULL};

    // Open the app directory tree in current system to search. We just need to traverse top level
    // directory.
    FTS* ftsPtr = fts_open(pathArrayPtr,
                           FTS_LOGICAL,
                           NULL);

    if (NULL == ftsPtr)
    {
        LE_ERROR("Could not access dir '%s'.  %m.", pathArrayPtr[0]);
        return LE_FAULT;
    }

    // Traverse through the directory tree.
    FTSENT* entPtr;

    while (NULL != (entPtr = fts_read(ftsPtr)))
    {
        switch (entPtr->fts_info)
        {
            case FTS_D:
                if (1 == entPtr->fts_level)
                {
                    char appPubCertPath[LIMIT_MAX_PATH_BYTES];

                    snprintf(appPubCertPath,
                             sizeof(appPubCertPath),
                             "%s/%s",
                             entPtr->fts_path, PUB_CERT_NAME );

                    if (file_Exists(appPubCertPath))
                    {
                        if (LE_OK != ima_ImportPublicCert(appPubCertPath))
                        {
                            LE_CRIT("Failed to import public certificate '%s'", appPubCertPath);
                            fts_close(ftsPtr);
                            return LE_FAULT;
                        }
                    }
                    // We don't need to go into this directory.
                    fts_set(ftsPtr, entPtr, FTS_SKIP);
                }
                break;
        }

    }

    fts_close(ftsPtr);
    return LE_OK;
}
//--------------------------------------------------------------------------------------------------
/**
 * Verify and install the current system
 *
 * @return None
 */
//--------------------------------------------------------------------------------------------------
static void CheckAndInstallCurrentSystem
(
    void
)
{
    int newestIndex = -1;
    int currentIndex = -1;

    // First step is to get rid of any failed unpack. We are root and this shouldn't
    // fail unless there is no upack dir in which case that's good.
    DeleteSystemUnpack();
    DeleteAppsUnpack();

    // Current system is named "current". All systems stored in index dirs are previous systems
    // except when we are waking up after a system update by updateDaemon, in which case
    // the newest index is greater than the current.

    newestIndex = FindNewestSystemIndex(); // Find newest non-bad system (-1 if none exist).
    currentIndex = ReadIndexFile("current"); // -1 if current system doesn't exist.
    if (currentIndex != -1)
    {
        LE_INFO("The previous 'current' system has index %d.", currentIndex);
    }

    // Hardware faults say nothing about whether we should rollback or not
    if (IsHardwareFaultReset())
    {
        // Do nothing
    }
    // Check if we should fall back to the "golden" system due to a boot loop
    else if ((newestIndex == currentIndex) &&
        (GetStatus("current", NULL) == STATUS_GOOD) &&
        (ReadBootCount() >= MAX_TRIES))
    {
        // If the golden system is boot looping, do not start Legato in an attempt to preserve
        // flash memory.
        if (IsCurrentSystemGolden())
        {
            // Remove the boot count file so Legato will boot normally next reboot.  If the
            // board reboots despite Legato not being started, it means one of two things:
            // 1) Legato is not causing the reset, in which case starting or not starting legato
            //    doesn't matter, or
            // 2) The module has been manually reset (e.g. power cycle) in which case we should
            //    re-attempt to start Legato after manual intervention.
            // Note: if a WDT is used which starts on boot, we should add code here to start
            // a program which kicks the watchdog.  A watchdog reset at this point would defeat
            // the purpose of not starting Legato.
            unlink(BOOT_COUNT_PATH);
            LE_FATAL("Golden system entered boot loop -- not starting Legato");
        }

        LE_INFO("A good system has entered a reboot loop -- reinstalling from golden.");
        currentIndex = InstallGolden(newestIndex, currentIndex);
        newestIndex = currentIndex;
    }
    // Check if we should install the "golden" system from /mnt/legato.
    else if (ShouldInstallGolden(newestIndex))
    {
        currentIndex = InstallGolden(newestIndex, currentIndex);
        newestIndex = currentIndex;
    }
    // If there wasn't a new "golden" system to install,
    // select the newest non-bad system as the current system.
    // If the current system is bad, the newest non-bad will be older than the current.
    // If a new system was just installed by the Update Daemon, the newest non-bad will be
    // newer than the current.
    // If there is no current system, the currentIndex will be -1.
    // But, we are guaranteed that newestIndex > -1, because if there were no non-bad
    // systems in /legato, ShouldInstallGolden() would have returned true and the golden
    // system would have been installed (and currentIndex would be the same as newestIndex).
    else if (newestIndex != currentIndex)
    {
        // If there's a current system, and it's not "good", just delete it.
        // But, if it is "good", save it in case we need to roll-back to it.
        if (currentIndex > -1)
        {
            // Attempt to umount the system because it may have been mounted when
            // sandboxed apps were created.
            fs_TryLazyUmount(CurrentSystemDir);

            SystemStatus_t currentSysStatus = GetStatus("current", NULL);

            char path[PATH_MAX];

            // Rename the current system path.
            CreateSystemPathName(currentIndex, path, sizeof(path));
            Rename(CurrentSystemDir, path);

            switch(currentSysStatus)
            {
                case STATUS_BAD:
                    // System bad. Delete and roll-back (here newestIndex < currentIndex).
                    RecursiveDelete(path);
                    break;

                case STATUS_TRYABLE:
                    // System try-able. Grab config tree from current system and delete it.
                    ImportOldConfigTrees(currentIndex, newestIndex);
                    RecursiveDelete(path);
                    break;

                case STATUS_GOOD:
                    // System good. Grab config tree from current system.
                    ImportOldConfigTrees(currentIndex, newestIndex);
                    break;
            }

        }

        // Make the newest system the current system.
        SetCurrent(newestIndex);
    }

    // If we need to update the dynamic linker's cache, do that now.
    // We can tell that we need to do that if the marker file exists.
    // That file gets deleted after the cache update finishes.
    if (FileExists(LdconfigNotDoneMarkerFile))
    {
        UpdateLdSoCache(CurrentSystemDir);
    }

    if (ima_IsEnabled())
    {
        InstallCurrentSystemCert();
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * It all starts here.
 */
//--------------------------------------------------------------------------------------------------
int main
(
    int argc,
    const char** argv
)
{
    // Initalize legato framework
    InitFramework();

    // Pass the args to the Command Line Arguments API.
    le_arg_SetArgs((size_t)argc, argv);

    le_arg_SetStringVar(&CurrentStartVersion, "v", "version");
    le_arg_Scan();

    bool isReadOnly = sysStatus_IsReadOnly();
    le_thread_Ref_t modemPAThread;
    le_clk_Time_t timeToWait = {10, 0}; // 10s timeout waiting for Modem PA loading thread

    if (!isReadOnly)
    {
        // Bind mount if they are not already mounted.
        BindMount("/mnt/flash/legato", "/legato");
        BindMount("/mnt/flash/home", "/home");
    }
    if (0 == access("/home", W_OK))
    {
        MakeDir("/home/root");
    }

    daemon_Daemonize(5000); // 5 second timeout in case older supervisor is installed.

    ModemPASemRef = le_sem_Create("ModemPA", 0);
    LE_INFO("Starting Modem PA loading thread.");

    modemPAThread = le_thread_Create("ModemPAThread", LoadModemPA, NULL);
    le_thread_Start(modemPAThread);

    // Waiting till Modem PA is loaded
    if (LE_OK != le_sem_WaitWithTimeOut(ModemPASemRef, timeToWait))
    {
        LE_ERROR("Time Out waiting for the Modem PA loader thread.");
        IsModemPATimedOut = true;
    }

    LE_INFO("Installing/launching the system.");
    while(1)
    {
        if (!isReadOnly)
        {
            // Verify and install the current system.
            // R/O system are always ready. So, nothing to do for them.
            CheckAndInstallCurrentSystem();
        }

        // Fix ld.so.conf in case the system is still running an older version of start
        // script that makes legato use the wrong liblegato path.
        FixLdSoConf();

        // Run the current system.
        Launch(isReadOnly);
    }

    return 0;
}
