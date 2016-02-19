//--------------------------------------------------------------------------------------------------
/** @file start.c
 *
 * The start program is the entry point for the legato framework. When start is called it first
 * checks to see whether there is an uncompleted update that needs to be finished. If one is found
 * then the update is completed.
 *
 * The current legato install (which at this point could be a freshly updated one) is then started
 * and monitored for failure. Each time a framework install that is not yet known to be good
 * is started, a try count is incremented. The framework is then monitored for a probationary
 * period and, if it passes without fault, will then be marked "good".
 *
 * If the framework fails before completing the probationary period then it will be restarted a
 * given number of times to see if the failure was caused by a temporary issue.
 *
 * If the framework fails beyond the fail count limit without surviving the probationary period
 * it will be marked bad and rolled back to the previous version.
 *
 * start will daemonize itself so that it can monitor the state of the system it runs while the
 * startup sequence completes. If supervisor exits with EXIT_SUCCESS, start will exit (since this
 * was an intentional shut-down) else if with EXIT_ERROR the system will be rebooted.
 * (though there is room for debate on that last point)
 *
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 *
 */

#include "legato.h"
#include <ftw.h>
#include <mntent.h>
#include <linux/limits.h>

#include <syslog.h>

#define DEFAULT_PERMS (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)

//--------------------------------------------------------------------------------------------------
/**
 * MAX_TRIES denotes the maximum number of times a new system can be tried (unless it becomes
 * marked "good") before it is reverted.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_TRIES 4

//--------------------------------------------------------------------------------------------------
/**
 * Return values for revert function
 */
//--------------------------------------------------------------------------------------------------
typedef enum {
    REVERT_OK,                  ///< The system has been reverted to the previous good system
    TRIED_TO_REVERT_GOOD_ERROR, ///< No snapshot was found
    NO_PREVIOUS_VERSION_ERROR,  ///< No previous version was found
    REVERT_ERROR                ///< Some other error was encountered. System was not reverted.
    } revertResultType;

//--------------------------------------------------------------------------------------------------
/**
 * return values for status test function
 */
//--------------------------------------------------------------------------------------------------
typedef enum {
    STATUS_GOOD,        ///< System is in "good" state
    STATUS_BAD,         ///< System is bad and should be reverted.
    STATUS_TRYABLE,     ///< System has been tried fewer than MAX_TRIES times.
    STATUS_NEW,         ///< System is new (has never been tried).
    STATUS_ERROR        ///< Some error has happened and the status cannot be determined.
    } statusType;

//--------------------------------------------------------------------------------------------------
/**
 * A collection of meaningful paths in the system
 */
//--------------------------------------------------------------------------------------------------
static const char* SystemsDir = "/legato/systems";
static const char* CurrentSystem = "/legato/systems/current";
static const char* AppsDir = "/legato/apps";
static const char* SystemsUnpackDir = "/legato/systems/unpack";
static const char* AppsUnpackDir = "/legato/apps/unpack";
static const char* OldConfigDir = "/mnt/flash/opt/legato/configTree";
static const char* OldFwDir = "/mnt/flash/opt/legato";
static const char* LdconfigNotDoneMarkerFile = "/legato/systems/needs_ldconfig";


//--------------------------------------------------------------------------------------------------
/**
 * The important indexes to know about the installed systems.
 * Set in FindNewestSystemIndex()
 */
//--------------------------------------------------------------------------------------------------
static int PreviousIndex = -1;
static int NewestGoodIndex = -1;

//--------------------------------------------------------------------------------------------------
/**
 * Remember the last exit code form the supervisor. This helps determine what to do on restart
 */
//--------------------------------------------------------------------------------------------------
static int LastExitCode = -1;

//--------------------------------------------------------------------------------------------------
/**
 * Check if a file exists.
 *
 * @return
 *          True if file exists else
 *          False
 */
//--------------------------------------------------------------------------------------------------
static bool FileExists
(
    const char* pathname
)
{
    struct stat sb;
    bool result = false;
    // perhaps log an error but if we can't reach the file it effectively doesn't exist to us
    if (stat(pathname, &sb) == 0)
    {
        if ((sb.st_mode & S_IFMT) == S_IFREG)
        {
            result = true;
        }
    }
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if a directory exists.
 *
 * @return
 *          True if directory exists else
 *          False
 */
//--------------------------------------------------------------------------------------------------
static bool DirExists
(
    const char* pathname
)
{
    struct stat sb;
    bool result = false;
    // perhaps log an error but if we can't reach the file it effectively doesn't exist to us
    if (stat(pathname, &sb) == 0)
    {
        if ((sb.st_mode & S_IFMT) == S_IFDIR)
        {
            result = true;
        }
    }
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Call back for nftw to remove files and directories.
 *
 * @return
 *           0 Success
 *          -1 Error
 */
//--------------------------------------------------------------------------------------------------
static int UnlinkCb
(
    const char *fpath,
    const struct stat *sb,
    int typeflag,
    struct FTW *ftwbuf
)
{
    int rv = remove(fpath);

    if (rv)
    {
        syslog(LOG_ERR, "Failed to remove %s - %s\n", fpath, strerror(errno));
    }

    return rv;
}

//--------------------------------------------------------------------------------------------------
/**
 * Recursively remove a directory but don't follow links and don't cross mount points.
 *
 * @return
 *           0 Success
 *          -1 Error
 */
//--------------------------------------------------------------------------------------------------
static int RecursiveDelete
(
    const char *path
)
{
    return nftw(path, UnlinkCb, 64, FTW_DEPTH | FTW_PHYS | FTW_MOUNT);
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
        syslog(LOG_ERR, "path to system too long\n");
        exit(EXIT_FAILURE);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Given the path to the status file in a given system (given the system dir) into the buffer
 * given by fileName ensuring that it does not exceed size
 */
//--------------------------------------------------------------------------------------------------
static void CreateStatusFileName
(
    const char* systemDir,
    char* fileName,
    size_t size
)
{
    if (snprintf(fileName, PATH_MAX, "%s/status", systemDir) >= PATH_MAX)
    {
        syslog(LOG_ERR, "CheckStatus - path too long\n");
        exit(EXIT_FAILURE);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Create a file named fileName (or truncate any such existing file) and write bufferSize
 * bytes of data from the given buffer, then close the file.
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
    int result = -1;


    do
    {
        fd = open(fileName, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    } while (fd == -1 && errno == EINTR);

    if (fd != -1)
    {
        while (written < bufferSize)
        {
            int n;
            if ((n = write(fd, buffer + written, bufferSize - written)) < 0) {
                if (errno == EINTR || errno == EAGAIN)
                    continue;
                syslog(LOG_ERR, "WriteToFile - couldn't write to file");
                exit(EXIT_FAILURE);
            }
            written += n;
        }
        close(fd);
        result = written;
    }
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * read up to size characters from a file given by fileName into a buffer provided by the caller
 *
 * @return
 *      -1 fail
 *       # of bytes read.
 */
//--------------------------------------------------------------------------------------------------
static int ReadFromFile
(
    const char* fileName,
    char* buffer,
    size_t size
)
{
    int fd;
    int inch;
    int len = -1;

    fd = open(fileName, O_RDONLY);
    if (fd != -1)
    {
        len = 0;
        while (1)
        {
            inch = read(fd, buffer + len, size - len);
            if( inch == -1 && errno == EINTR )
            {
                continue;
            }

            if (inch == -1)
            {
                // read just plain old failed. Terminate the buffer.
                buffer[len] = '\0';
                break;
            }

            if (inch == 0)
            {
                // finished file
                buffer[len] = '\0';
                break;
            }

            len += inch;

            if (len >= size)
            {
                // read enough (or too much which is bad)
                buffer[size - 1] = '\0';
                break;
            }
        }
    }
    else
    {
        buffer[0] = '\0';
    }
    close(fd);
    return len;
}

//--------------------------------------------------------------------------------------------------
/**
 * Copy a file given fromPath and toPath
 * Reusing the ReadFromFile and WriteToFile to do a full read then write therefore file size is
 * limited by buffer size. The files we are dealing with here are small but this method will
 * become unworkable for large files.
 * @ returns
 *               0 success
 *              -1 failure
 */
//--------------------------------------------------------------------------------------------------
static int Copy
(
    const char* fromPath,
    const char* toPath
)
{
    char buffer[1024];
    int result = -1;
    int inLength = ReadFromFile(fromPath, buffer, sizeof(buffer));
    if (inLength >= sizeof(buffer))
    {
        // we filled the buffer. There is a small chance the file is OK. Warn or Error ??
        syslog(LOG_WARNING, "File '%s' may be truncated by copying\n", fromPath);
    }

    if (inLength < 0)
    {
        // read failed
        result = -1;
    }
    else
    {
        if (WriteToFile(toPath, buffer, inLength) < inLength)
        {
            // technically this can't happen with WriteToFile as written because
            // it will EXIT_FAILURE if it can't write out it's buffer.
            result = -1;
        }
        else
        {
            result = 0;
        }
    }
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Create a fresh legato directory structure in the unpack directory and symlink the correct
 * paths from /mnt/legato
 */
//--------------------------------------------------------------------------------------------------
static int MakeUnpackDirFromMntLegato
(
    void
)
{
    // Create directories (ignore errors because it's fine if the directory already exists).
    mkdir("/legato/systems", DEFAULT_PERMS);
    mkdir("/legato/systems/unpack", DEFAULT_PERMS);
    mkdir("/legato/systems/unpack/config", DEFAULT_PERMS);

    // Create symlinks:
    if (symlink("/mnt/legato/system/bin", "/legato/systems/unpack/bin") ||
        symlink("/mnt/legato/system/lib", "/legato/systems/unpack/lib") ||
        symlink("/mnt/legato/system/config/apps.cfg", "/legato/systems/unpack/config/apps.cfg") ||
        symlink("/mnt/legato/system/config/users.cfg", "/legato/systems/unpack/config/users.cfg"))
    {
        syslog(LOG_ERR, "Could not create symlinks (%m)\n");
        exit(EXIT_FAILURE);
    }

    // Copy files:
    if (Copy("/mnt/legato/system/version", "/legato/systems/unpack/version") ||
        Copy("/mnt/legato/system/info.properties", "/legato/systems/unpack/info.properties"))
    {
        syslog(LOG_ERR, "Could not copy needed files\n");
        exit(EXIT_FAILURE);
    }

    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Import the last good configuration into the unpack directory.
 * NB: The last indexed system should be good else we would not have been allowed to update
 * (in theory - else this becomes a bit more complicated as we have to search backwards through
 * indices and snapshots checking for "good" status.
 */
//--------------------------------------------------------------------------------------------------
static int ImportOldConfigs
(
    const char* previousSystem
)
{
    // if a previous good is passed then there is either an old config in /legato/systems
    // or the previous system is an old style system.
    char commandString[PATH_MAX];

    if (previousSystem)
    {
        // check if path is to new style
        if(strstr(previousSystem, SystemsDir) == previousSystem)
        {
            // At this point the path goes to /apps so we can cheaply get someone else
            // to figure out the path with a ..
            snprintf(commandString, PATH_MAX, "cp %s/config/system.* %s/config/", previousSystem, SystemsUnpackDir);
            system(commandString);
        }
        else if (DirExists(OldConfigDir))
        {
            snprintf(commandString, PATH_MAX, "cp %s/system.* %s/config/", OldConfigDir, SystemsUnpackDir);
            system(commandString);
        }
    }
    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the system index number from the name of the directory. The directory name
 * should contain no extra path as it is read at the level of the containing directory
 * - so we don't need to deal with path trimming. We just need to ensure the name
 * contains only digits. We don't currently zero pad the left side but we will allow
 * zero padding anyway.
 */
//--------------------------------------------------------------------------------------------------
static int GetSystemIndex
(
    const char* dirName
)
{
    int index = -1;

    if( dirName )
    {
        char* ch;
        const char* base = strrchr(dirName, '/');
        if (NULL == base)
        {
            base = dirName;
        }
        else
        {
            base ++;
        }
        index = (int)strtol(base, &ch, 10);
        if (*ch != '\0')
        {
            // This decode stopped on a non-numeric character so is not a valid index
            index = -1;
        }
    }
    return index;
}

//--------------------------------------------------------------------------------------------------
/**
 * Callback from nftw that deletes directories with a name containing an index less
 * than NewestGoodIndex.
 */
//--------------------------------------------------------------------------------------------------
static int TrimOldDirsCb
(
    const char *fpath,
    const struct stat *sb,
    int typeflag,
    struct FTW *ftwbuf
)
{
    // GetSystemIndex extracts index from the directory name, not the index file.
    // Therefore systems with non-numeric names return -1
    if (ftwbuf->level == 1)
    {
        int index = GetSystemIndex(fpath);
        if ((index > -1) && (index < NewestGoodIndex))
        {
            RecursiveDelete(fpath);
        }
    }
    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Starting from the given index (but not including it) count backwards removing any old systems
 * found.
 *
 * @return
 *           0      success
 *          -1      fail
 */
//--------------------------------------------------------------------------------------------------
static int TrimOldDirs
(
)
{
    // Remove any old style firmware and visit the dirs in systems
    // and remove the ones with a smaller index.
    if (DirExists(OldFwDir))
    {
        RecursiveDelete(OldFwDir);
    }
    // Scan through the systems dir deleting anything older than the previous good.
    nftw(SystemsDir, TrimOldDirsCb, 64, FTW_PHYS | FTW_MOUNT);

    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Rename the system unpack directory to a system directory with the index given by index.
 */
//--------------------------------------------------------------------------------------------------
static void Rename
(
    const char* fromName,
    const char* toName
)
{
    // TODO: check return
    if (rename(fromName, toName) == -1)
    {
        if (errno == ENOTEMPTY)
        {
            // The old name is a non empty directory. Blow it away.
            syslog(LOG_WARNING, "'%s' is non-empty directory. Deleting it.\n",
                 toName);
            RecursiveDelete(toName);
            rename(fromName, toName);
        }
        else
        {
            // don't know how to handle anything else. Just report it.
            syslog(LOG_ERR, "Cannot rename directory '%s' to %s: %s\n",
                 fromName, toName, strerror(errno));
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Mark the system in the unpack directory as good. This system has not actually been tried but
 * since we are in the start program we know that it has been set up from the built in system and is
 * therefore assumed de facto good.
 */
//--------------------------------------------------------------------------------------------------
static int MarkUnpackGood
(
    void
)
{
    char systemsUnpackStatus[PATH_MAX];
    char* buff = "good";
    int length = strlen(buff);

    CreateStatusFileName(SystemsUnpackDir, systemsUnpackStatus, PATH_MAX);

    return WriteToFile(systemsUnpackStatus, buff, length);
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
    const char* text;
    // create marker file to say we are doing ldconfig
    text = "start_ldconfig";
    WriteToFile(LdconfigNotDoneMarkerFile, text, strlen(text));
    // write /legato/systems/current/lib to /etc/ld.so.conf
    text = "/legato/systems/current/lib\n";
    WriteToFile("/etc/ld.so.conf", text, strlen(text));
    if (0 == system("ldconfig > /dev/null"))
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
    WriteToFile(LdconfigNotDoneMarkerFile, text, strlen(text));
}
//--------------------------------------------------------------------------------------------------
/**
 * Given a buffer of characters, scan the buffer for a given name and return in another buffer
 * the value given on the other side of the equal sign (up to a line return character).
 */
//--------------------------------------------------------------------------------------------------
static int GetPropertyValue
(
    const char* fullBuffer,
    const char* propertyName,
    char* valueBuffer,
    size_t bufferSize
)
{
    // Find the requested value
    int result = -1;
    char * posn = strstr(fullBuffer, propertyName);
    if (posn)
    {
        posn += strlen(propertyName);
        // the next character should be '='. Should we allow spaces around this? We don't create files
        // with spaces right now.
        if (*posn == '=')
        {
            posn++;
            while (bufferSize && *posn != '\n')
            {
                *valueBuffer = *posn;
                valueBuffer++;
                posn++;
                bufferSize--;
            }
            // terminate for safety.
            if (bufferSize == 0)
            {
                // stomp last char in the buffer
                *(valueBuffer - 1) = '\0';
            }
            else
            {
                *valueBuffer = '\0';
            }
            result = 0;
        }
        else
        {
            syslog(LOG_ERR, "Expected '=' but found '%c' after %s\n", *posn, propertyName);
        }
    }
    else
    {
        syslog(LOG_INFO, "Property %s not found\n", propertyName);
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the value of a given property from in info.property file
 */
//--------------------------------------------------------------------------------------------------
static int ReadInfoProperty
(
    const char* infoFileName,
    const char* propertyName,
    char* valueBuffer,
    size_t bufferSize
)
{
    // Most of these files are less than 200 bytes. When this buffer starts being too small we
    // will have to get smarter
    char inputBuffer[1024];
    int result = -1;

    int bytesRead = ReadFromFile(infoFileName, inputBuffer, sizeof(inputBuffer));
    //information reporting - should never be longer than sizeof(inputBuffer) that would be quite bad!
    if (bytesRead >= sizeof(inputBuffer))
    {
        syslog(LOG_INFO,
                "Filled buffer reading %d bytes from %s. There may be unread data remaining.\n",
                bytesRead, infoFileName);
    }
    else if (bytesRead < 0)
    {
        // This could be a bit fatal but is it really? Probably should abort installation at this point.
        syslog(LOG_ERR, "Error reading data from %s\n", infoFileName);
    }
    else if (bytesRead == 0)
    {
        syslog(LOG_ERR, "File %s is empty.\n", infoFileName);
    }
    if (bytesRead > 0)
    {
        // force a null terminator on this else strstr could potentially go on forever.
        inputBuffer[sizeof(inputBuffer) - 1] = '\0';
        result = GetPropertyValue(inputBuffer, propertyName, valueBuffer, bufferSize);
    }
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get an app name from an info.property file
 */
//--------------------------------------------------------------------------------------------------
static int ReadAppNameFromInfo
(
    const char* path,
    char* nameBuffer,
    size_t bufferSize
)
{
    char infoPath[PATH_MAX];
    int result = -1;

    if (snprintf(infoPath, sizeof(infoPath), "%s/info.properties", path) < sizeof(infoPath))
    {
        result = ReadInfoProperty(infoPath, "app.name", nameBuffer, bufferSize);
    }
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Callback for ntwf that copies an app
 */
//--------------------------------------------------------------------------------------------------
static int AppCopyCb
(
    const char *fpath,
    const struct stat *sb,
    int typeflag,
    struct FTW *ftwbuf
)
{
    // Only good for single threaded. Would be nice if there was a good way to pass
    // data through to the callback
    static const char* previousSystem = NULL;
    static const char* destinationPath = NULL;

    if (sb == NULL && ftwbuf == NULL)
    {
        switch(typeflag)
        {
            case 1:
                previousSystem = fpath;
                break;
            case 2:
                destinationPath = fpath;
                break;
            case 0:
            default:
                previousSystem = NULL;
                destinationPath = NULL;
                break;
        }
    }
    else
    {
        const char* source = fpath;
        if (destinationPath)
        {
            // buffers for creating the full paths
            char sourcePathBuffer[PATH_MAX];
            char destinationPathBuffer[PATH_MAX];

            int level = ftwbuf->level;
            const char* pathFromBase = NULL;
            pathFromBase = fpath + strlen(fpath);
            while (level && pathFromBase != fpath)
            {
                pathFromBase--;
                if (*pathFromBase == '/')
                {
                    level--;
                }
            }
            snprintf(destinationPathBuffer, PATH_MAX, "%s%s", destinationPath, pathFromBase);
            if (DirExists(source))
            {
                mkdir(destinationPathBuffer, DEFAULT_PERMS);
            }
            else
            {
                if (previousSystem)
                {
                    snprintf(sourcePathBuffer, PATH_MAX, "%s/appsWriteable%s", previousSystem, pathFromBase);
                    if (FileExists(sourcePathBuffer))
                    {
                        source = sourcePathBuffer;
                    }
                }

                if (link(source, destinationPathBuffer) == -1 && errno == EXDEV)
                {
                    // shouldn't need this on the device
                    symlink(source, destinationPathBuffer);
                }
            }
        }
    }
    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Create the writable directory for an app and copy any files to it that are found in the update
 * or, if they exist, from a previous good system.
 */
//--------------------------------------------------------------------------------------------------
static void SetUpAppWritable
(
    const char* appSource,
    const char* appName,
    const char* previousSystem
)
{
    char appsUnpackWritable[PATH_MAX];
    char appsWriteableSource[PATH_MAX];

    //TODO: err check
    // Create path to where we will copy writables.
    snprintf(appsUnpackWritable, PATH_MAX, "%s/appsWriteable/%s", SystemsUnpackDir, appName);
    mkdir(appsUnpackWritable, DEFAULT_PERMS);

    // reuse this buffer - create the path to copy from
    snprintf(appsWriteableSource, PATH_MAX, "%s/writeable", appSource);

    // Call the callback like this first to set some path information.
    // A bit of an odd way to feed this to the call back but it serves.
    AppCopyCb(previousSystem, NULL, 1, NULL);
    AppCopyCb(appsUnpackWritable, NULL, 2, NULL);
    nftw(appsWriteableSource, AppCopyCb, 64, FTW_PHYS | FTW_MOUNT);
    AppCopyCb(NULL, NULL, 0, NULL);

}

//--------------------------------------------------------------------------------------------------
/**
 * Create the required directories and links to install an app in the system
 */
//--------------------------------------------------------------------------------------------------
static void SetUpApp
(
    const char* appHash,
    const char* previousSystem
)
{
    char appSource[PATH_MAX];
    char appDest[PATH_MAX];
    char appName[1024];

    snprintf(appSource, PATH_MAX, "/mnt/legato/apps/%s", appHash);
    snprintf(appDest, PATH_MAX, "%s/%s", AppsDir, appHash);
    ReadAppNameFromInfo(appSource, appName, sizeof(appName));

    if (FileExists(appDest))
    {
        unlink(appDest);
    }

    symlink(appSource, appDest);

    SetUpAppWritable(appSource, appName, previousSystem);
    // link the system app name to the hash app hash
    // Just reusing the appSource buffer because we don't need it anymore
    snprintf(appSource, PATH_MAX, "%s/apps/%s", SystemsUnpackDir, appName);
    symlink(appDest, appSource);
}

//--------------------------------------------------------------------------------------------------
/**
 * Install all the apps found in /mnt/legato/apps
 * Assume that everything directory in apps is named with an md5 sum.
 * We could do a quick check that it's a real app dir by checking the sum or looking for
 * particular contents but that's a lot more work.
 */
//--------------------------------------------------------------------------------------------------
static void InstallApps
(
    const char* previousSystem
)
{
    DIR* d;
    char* dirName = "/mnt/legato/apps";

    /* Open the current directory. */

    d = opendir(dirName);

    if (! d && errno != ENOENT)
    {
        syslog(LOG_ERR, "Cannot open directory '%s': %s\n",
                 dirName, strerror(errno));
        exit(EXIT_FAILURE);
    }
    while (d) {
        struct dirent * entry;

        entry = readdir(d);
        if (! entry)
        {
            break;
        }
        if (DT_DIR == entry->d_type && entry->d_name[0] != '.')
        {
            SetUpApp(entry->d_name, previousSystem);
        }
    }
    /* Close the directory. closedir is not smart enough to ignore NULL pointers so we check */
    if (d && closedir(d)) {
        syslog(LOG_ERR, "Could not close '%s': %s\n",
                 dirName, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the index for the given system from it's index file
 */
//--------------------------------------------------------------------------------------------------
static int ReadIndexFile
(
    const char* systemPath
)
{
    int index = 0;
    char inputBuffer[512];
    char indexFilePath[PATH_MAX];

    snprintf(indexFilePath, sizeof(indexFilePath), "%s/index", systemPath);
    if (ReadFromFile(indexFilePath, inputBuffer, sizeof(inputBuffer)) > 0)
    {
        // some bytes were read. Try to get a number out of them!
        index = atoi(inputBuffer);
    }
    return index;
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
    snprintf(indexFileBuffer, sizeof(indexFileBuffer), "%s/index", SystemsUnpackDir);
    snprintf(indexString, sizeof(indexString), "%d", newIndex);
    WriteToFile(indexFileBuffer, indexString, strlen(indexString));
}

//--------------------------------------------------------------------------------------------------
/**
 * Move the current system aside (to a directory named for it's index)
 */
//--------------------------------------------------------------------------------------------------
static void BackupCurrent
(
    void
)
{
    char backupPath[PATH_MAX];
    int index = ReadIndexFile(CurrentSystem);

    CreateSystemPathName(index, backupPath, sizeof(backupPath));
    Rename(CurrentSystem, backupPath);
}

//--------------------------------------------------------------------------------------------------
/**
 * If there is no version installed we have to set one up in mtd4 from mtd3
 * Clone the required directories and populate with symlinks for everything.
 * Walk the /mnt/legato system tree and create the dirs and symlink the files.
 * Copy from /mnt/legato/system to /legato/systems/<index>
 */
//--------------------------------------------------------------------------------------------------
static void SetUpGoldenFromMntLegato
(
    int newIndex,
    const char* previousSystem
)
{
    if (DirExists("/mnt/legato/system"))
    {
        MakeUnpackDirFromMntLegato();
        ImportOldConfigs(previousSystem);
        mkdir(AppsDir, DEFAULT_PERMS);
        mkdir("/legato/systems/unpack/apps", DEFAULT_PERMS);
        mkdir("/legato/systems/unpack/appsWriteable", DEFAULT_PERMS);
        InstallApps(previousSystem);
        WriteUnpackIndexFile(newIndex);
        MarkUnpackGood();
        BackupCurrent();
        Rename(SystemsUnpackDir, CurrentSystem);
        NewestGoodIndex = newIndex;
        TrimOldDirs();
    }
    else
    {
        // There is no point going on. There is no system to install!!
        syslog(LOG_ERR, "No installable system found\n");
        exit(EXIT_FAILURE);
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
    int x;
    int tries = -1;
    if ((x = strncmp(buff, "tried ", 6)) == 0)
    {
        tries = strtol(buff + 6, NULL, 10);
        if (tries < -1)
        {
            // someone has been messing with the status file.
            tries = -1;
        }
    }
    return tries;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the status to indicate how many times this system has been tried.
 */
//--------------------------------------------------------------------------------------------------
static void MarkStatusTried
(
    int fd,
    int numTry
)
{

    int written = 0;
    int n, length;
    char buff[100];

    length = snprintf(buff, 100, "tried %d", numTry);
    if (length > 100)
    {
        syslog(LOG_ERR, "markStatusTried - 'tried' string too long");
        exit(EXIT_FAILURE);
    }
    lseek(fd, 0, SEEK_SET);
    ftruncate(fd, 0);
    while (written < length)
    {
        if ((n = write(fd, buff + written, length - written)) < 0) {
            if (errno == EINTR || errno == EAGAIN)
                continue;
            syslog(LOG_ERR, "markStatusTried - couldn't write status");
            exit(EXIT_FAILURE);
        }
        written += n;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse what is in the status file. Determine if it is good or bad or tried x.
 * If tried and updateTries is true the try count will be incremented as a side-effect
 *
 * @return
 *          STATUS_GOOD     the system has passed probation or is builtin
 *          STATUS_BAD      the system has been marked bad
 *          STATUS_TRYABLE  the system has failed less than 4 times. Try it again.
 *          STATUS_ERROR    some other error occurred
 *
 */
//--------------------------------------------------------------------------------------------------
static statusType ParseStatus
(
    int fd,
    bool updateTries
)
{
    statusType status = STATUS_ERROR;
    int inch;
    char buff[100];
    inch = read(fd, buff, sizeof(buff) - 1);
    buff[inch] = '\0';

    if (IsGood(buff))
    {
        status = STATUS_GOOD;
    }
    else if (IsBad(buff))
    {
        status = STATUS_BAD;
    }
    else
    {
        int tries = GetNumTries(buff);
        if (tries < 0)
        {
            syslog(LOG_ERR, "something is wrong with tries\n");
        }
        else if (tries < 1)
        {
            syslog(LOG_ERR, "Tried has a value of 0 which should not happen\n");
        }
        else if (tries >= MAX_TRIES)
        {
            syslog(LOG_ERR, "Too many tries. We need to revert\n");
            status = STATUS_BAD;
        }
        else
        {
            syslog(LOG_INFO, "Tried = %d\n", tries);
            if (updateTries)
            {
                MarkStatusTried(fd, tries + 1);
            }
            status = STATUS_TRYABLE;
        }
    }
    return status;
}

//--------------------------------------------------------------------------------------------------
/**
 * There is currently no status file. Create one and set "tried" to 1
 */
//--------------------------------------------------------------------------------------------------
static statusType CreateNewStatus
(
    const char* systemDir
)
{
    statusType status = STATUS_ERROR;
    int fd;
    char statusPath[PATH_MAX];

    CreateStatusFileName(systemDir, statusPath, PATH_MAX);
    syslog(LOG_INFO, "creating %s\n", statusPath);
    do
    {
        fd = open(statusPath, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    } while (fd == -1 && errno == EINTR);
    if (fd == -1)
    {
        syslog(LOG_ERR, "Cannot create status file");
        exit(EXIT_FAILURE);
    }
    // here I can just write that we are on try one.
    MarkStatusTried(fd, 1);
    status = STATUS_TRYABLE;
    close(fd);
    return status;
}

//--------------------------------------------------------------------------------------------------
/**
 * chackStatus will determine if the current system status is new, good or tryable.
 * If the status is tryable and updateTries is true it has the side-effect of incrementing
 * the "tried" status.
 */
//--------------------------------------------------------------------------------------------------
static statusType CheckStatus
(
    const char* systemDir,
    bool updateTries
)
{
    statusType status = STATUS_ERROR;
    int fd;
    char statusPath[PATH_MAX];

    CreateStatusFileName(systemDir, statusPath, PATH_MAX);
    do
    {
        fd = open(statusPath, O_RDWR);
    } while (fd == -1 && errno == EINTR);

    if (fd == -1)
    {
        if (errno == ENOENT)
        {
            status = STATUS_NEW;
        }
    }
    else
    {
        status = ParseStatus(fd, updateTries);
    }

    close(fd);
    return status;
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
    // Run some extra startup stuff in the startup script - don't know what to do if this fails.
    // Shouldn't ever, of course, because it's part of the "good" stuff, but if this script has
    // been whittled away to nothing we needn't care if it is gone in some version.
    int result = system("/mnt/legato/startupScript");

    if (WIFSIGNALED(result))
    {
        syslog(LOG_CRIT, "startupScript was killed by a signal %d.\n", WTERMSIG(result));
    }
    else if (WEXITSTATUS(result) != EXIT_SUCCESS)
    {
        syslog(LOG_CRIT, "startupScript exited with error code %d.\n", WEXITSTATUS(result));
    }

    // Run Supervisor but ask it not to daemonize itself so that we can see if it dies.
    result = system("/legato/systems/current/bin/supervisor --no-daemonize");

    if (WIFEXITED(result))
    {
        return WEXITSTATUS(result);
    }
    else
    {
        if (WIFSIGNALED(result))
        {
            syslog(LOG_CRIT, "Supervisor was killed by a signal %d.\n", WTERMSIG(result));
        }
        else
        {
            syslog(LOG_CRIT, "Supervisor exited with code %d.\n", WEXITSTATUS(result));
        }

        return EXIT_FAILURE;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Scans the contents of the systems directory.
 * Finds the directory name with the highest index number. The directory name must consist only of
 * base 10 digits.
 * We don't need to search recursively - all system index dirs will be at the same level.
 *
 * @return
 *          the system index or
 *          -1 if no system index found.
 */
//--------------------------------------------------------------------------------------------------
static int FindNewestSystemIndex
(
    void
)
{
    DIR* d;
    const char* dirName = SystemsDir;
    int systemIndex = -1;
    int previousGoodIndex = -1;

    d = opendir(dirName);

    if (! d && errno != ENOENT)
    {
        syslog(LOG_ERR, "Cannot open directory '%s': %s\n",
                 dirName, strerror(errno));
        if (mkdir(dirName, DEFAULT_PERMS) == 0)
        {
            // The directory is newly created. There is no existing good system index.
            return -1;
        }
        else
        {
            // If we can't make system directory something is very wrong with the setup.
            exit(EXIT_FAILURE);
        }
    }
    while (d) {
        struct dirent * entry;

        entry = readdir(d);
        if (! entry)
        {
            break;
        }
        if (DT_DIR == entry->d_type)
        {
            int tempIndex = GetSystemIndex(entry->d_name);
            if (tempIndex > systemIndex)
            {
                systemIndex = tempIndex;
                if (CheckStatus(entry->d_name, false) == STATUS_GOOD)
                {
                    previousGoodIndex = tempIndex;
                }
            }
        }
    }
    /* Close the directory. closedir is not smart enough to ignore NULL pointers so we check */
    if (d && closedir(d)) {
        syslog(LOG_ERR, "Could not close '%s': %s\n",
                 dirName, strerror(errno));
        exit(EXIT_FAILURE);
    }

    PreviousIndex = systemIndex;
    NewestGoodIndex = previousGoodIndex;

    return systemIndex;
}

//--------------------------------------------------------------------------------------------------
/**
 * Revert to the previous numbered system. Check that there is no snapshot version
 * before calling this.
 * @return
 *          NO_PREVIOUS_VERSION_ERROR   if there is there is no previous version to restore
 *          REVERT_OK        if the snapshot is restored.
 *          REVERT_ERROR        if revert failed.
 */
//--------------------------------------------------------------------------------------------------
static revertResultType RevertToPreviousVersion
(
)
{
    revertResultType result = REVERT_ERROR;

    // Substitute the previous version for the current version.
    int previousIndex = FindNewestSystemIndex();

    if (previousIndex > -1)
    {
        char previousSystemPath[PATH_MAX];

        CreateSystemPathName(previousIndex, previousSystemPath, sizeof(previousSystemPath));
        // By renaming current to unpack, even if we die now, it will be deleted when we restart.
        rename(CurrentSystem, SystemsUnpackDir);
        rename(previousSystemPath, CurrentSystem);
        DeleteSystemUnpack();
        result = REVERT_OK;
    }
    else
    {
        syslog(LOG_ERR, "Trying to revert but no previous system to revert to\n");
        result = NO_PREVIOUS_VERSION_ERROR;
    }
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Revert to a previous working system by going back to the snapshot from which this system is
 * derived or, if not found, going back to the previous installed system.
 */
//--------------------------------------------------------------------------------------------------
static revertResultType Revert
(
)
{
    revertResultType result = REVERT_ERROR;

    if (CheckStatus(CurrentSystem, false) == STATUS_GOOD)
    {
        syslog(LOG_ERR, "Cannot revert good system\n");
        result = TRIED_TO_REVERT_GOOD_ERROR;
    }
    else
    {
        result = RevertToPreviousVersion();

        if (result == REVERT_OK)
        {
            const char* text = "revert_ldconfig";
            WriteToFile(LdconfigNotDoneMarkerFile, text, strlen(text));
        }
    }
    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks version files for the system to determine whether the cwe version has been installed
 * at some time.
 *
 * @return
 *          true    if the version files differ
 *          false   if they are the same
 */
//--------------------------------------------------------------------------------------------------
static bool BuiltInVersionsDiffer
(
    void
)
{
    char builtInVersion[255];
    char goldenSystemVersion[255];

    ReadFromFile("/legato/mntLegatoVersion", builtInVersion, sizeof(builtInVersion) - 1);
    ReadFromFile("/mnt/legato/system/version", goldenSystemVersion, sizeof(goldenSystemVersion) -1 );

    return strcmp(builtInVersion, goldenSystemVersion) != 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * at some time.
 */
//--------------------------------------------------------------------------------------------------
static void MarkUpdateComplete
(
    void
)
{
    system("cp /mnt/legato/system/version /legato/mntLegatoVersion");
}

//--------------------------------------------------------------------------------------------------
/**
 * If something else is mounted on that mountpoint - unmount it.
 */
//--------------------------------------------------------------------------------------------------
void static CheckMount(char* mountedPoint)
{
    struct mntent *mountEntry;
    FILE *mtabFile = setmntent("/etc/mtab", "r");

    mountEntry = getmntent(mtabFile);
    while (mountEntry)
    {
        if (strcmp(mountEntry->mnt_dir, mountedPoint) == 0)
        {
            umount(mountedPoint);
        }
        mountEntry = getmntent(mtabFile);
    }

    endmntent(mtabFile);
}

//--------------------------------------------------------------------------------------------------
/**
 * Bind mount the given path to the mount point
 */
//--------------------------------------------------------------------------------------------------
void static BindMount(char* path, char* mountedAt)
{
    CheckMount(mountedAt);
    // This should be a recursive call to make a full path. libLegato has one.
    mkdir(path, DEFAULT_PERMS);
    if (mount(path, mountedAt, NULL, MS_BIND, NULL))
    {
        exit(EXIT_FAILURE);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Runs the current system.  Returns when a new system needs to be selected to run.
 */
//--------------------------------------------------------------------------------------------------
static void RunCurrentSystem
(
    void
)
{
    int exitCode = TryToRun();
    LastExitCode = exitCode;

    switch (exitCode)
    {
        case EXIT_FAILURE:

            // Sync file systems before rebooting.
            sync();

            // Reboot the system.
            if (reboot(RB_AUTOBOOT) == -1)
            {
                syslog(LOG_CRIT, "Failed to reboot. Errno = %s.\n", strerror(errno));
            }
            else
            {
                syslog(LOG_CRIT, "Failed to reboot. Errno = Success?!\n");
            }

            exit(EXIT_FAILURE);

        case EXIT_SUCCESS:

            syslog(LOG_INFO, "Supervisor exited with EXIT_SUCCESS.  Legato framework stopped.\n");

            exit(EXIT_SUCCESS);

        case 2:

            syslog(LOG_INFO, "Supervisor exited with 2.  Legato framework restarting.\n");
            break;
            
        case 3:

            syslog(LOG_INFO, "Supervisor exited with 3.  Legato framework restarting.\n");
            break;

        default:

            syslog(LOG_CRIT, "Unexpected exit code (%d) from the Supervisor.\n", exitCode);
            break;
    }

    // Returning from this function will loop back around and select the appropriate system,
    // incrementing the try counter if appropriate, or reverting if necessary.
}

//--------------------------------------------------------------------------------------------------
/**
 * If a system has been updated, or any updates have been interrupted, fix everything to be in
 * a consistent state. If any of these fixes are applied return true so that we can retry startup
 * again from a good state.
 */
//--------------------------------------------------------------------------------------------------
static bool FixUpPendingActions
(
    int currentIndex,
    int newestIndex
)
{
    bool restart = false;
    char pathBuffer[PATH_MAX];

    if (DirExists(CurrentSystem))
    {
        if (newestIndex == currentIndex)
        {
            // failed "modified" in current. Roll back and try again.
            syslog(LOG_ERR, "System failed modification. Reverting.\n");
            Revert();
            restart = true;
        }
        else if (newestIndex > currentIndex)
        {
            char pathBuffer[PATH_MAX];

            syslog(LOG_INFO, "Finishing system update\n");
            CreateSystemPathName(currentIndex, pathBuffer, sizeof(pathBuffer));
            Rename(CurrentSystem, pathBuffer);
            CreateSystemPathName(newestIndex, pathBuffer, sizeof(pathBuffer));
            Rename(pathBuffer, CurrentSystem);
            RequestLdSoConfig();
            restart = true;;
        }
    }
    else
    {
        if (newestIndex > -1)
        {
            // We could have an interrupted rename when swapping index dir and current
            syslog(LOG_WARNING, "Previous update interrupted. Attempting to recover.\n");
            CreateSystemPathName(newestIndex, pathBuffer, sizeof(pathBuffer));
            Rename(pathBuffer, CurrentSystem);
            restart = true;
        }
    }

    return restart;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check the status and if everything looks good to go, get the ball rolling, else revert!
 */
//--------------------------------------------------------------------------------------------------
static void Launch
(
)
{
    // If the supervisor exited with exit code 3 then don't
    // increment the try count. This means that "legato restart" was used
    bool incrementCount = (LastExitCode != 3);
    
    switch(CheckStatus(CurrentSystem, incrementCount))
    {
        case STATUS_GOOD:
        case STATUS_TRYABLE:
            RunCurrentSystem();
            break;
        case STATUS_BAD:
            if (Revert() != REVERT_OK)
            {
                syslog(LOG_CRIT, "Revert failed!");
                exit(EXIT_FAILURE);
            }
            break;
        case STATUS_NEW:
            // The following won't return if it can't create the file.
            CreateNewStatus(CurrentSystem);
            RunCurrentSystem();
            break;
        case STATUS_ERROR:
        default:
            syslog (LOG_ERR, "status file corrupted.");
            if (Revert() != REVERT_OK)
            {
                syslog(LOG_CRIT, "Revert failed!");
                exit(EXIT_FAILURE);
            }
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * We need to install the system in /mnt/legato/system as the new current system with an index
 * higher than the latest installed index
 */
//--------------------------------------------------------------------------------------------------
static void InstallFromFlash
(
    int newestIndex
)
{
    // get previous system (for copying configs/appWriteable data)
    const char* previousSystem = NULL;
    char previousPath[PATH_MAX];

    // Pick a system to copy from if we are installing something new.
    if (newestIndex >= 0)
    {
        snprintf(previousPath, PATH_MAX, "%s/%d", SystemsDir, newestIndex);
        previousSystem = previousPath;
    }
    else
    {
        if (DirExists(OldFwDir))
        {
            previousSystem = OldFwDir;
        }
    }

    // install after the current newest index
    newestIndex++;
    SetUpGoldenFromMntLegato(newestIndex, previousSystem);
    RequestLdSoConfig();
    MarkUpdateComplete();
}

//--------------------------------------------------------------------------------------------------
/**
 * It all starts here.
 */
//--------------------------------------------------------------------------------------------------
int main
(
    int argv,
    char** argc
)
{
    int newestIndex = -1;
    int currentIndex = -1;

    setlogmask (LOG_UPTO (LOG_NOTICE));
    openlog ("legato_start", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);

    BindMount("/mnt/flash/legato", "/legato");

    while(1)
    {
        // First step is to get rid of any failed unpack. We are root and this shouldn't
        // fail unless there is no upack dir in which case that's good.
        DeleteSystemUnpack();
        DeleteAppsUnpack();

        // See if there are older system (will be -1 if this is a first install)
        // Current system is named current. All systems stored in index dirs are previous systems
        // but if the newest index is greater than current we are waking up after a system update
        // by updateDaemon.

        newestIndex = FindNewestSystemIndex();
        currentIndex = ReadIndexFile(CurrentSystem);

        if (FixUpPendingActions(currentIndex, newestIndex))
        {
            // if any fix ups were done or installs finished, start this cycle again.
            continue;
        }

        if (currentIndex > -1 && STATUS_GOOD == CheckStatus(CurrentSystem, false))
        {
            // This newest good supercedes any good ones found in FindNewestSystemIndex
            NewestGoodIndex = currentIndex;
        }

        // if /legato/mntLegatoVersion does not exist or differs from /mnt/legato/system/version
        // we need to set one up.
        if (BuiltInVersionsDiffer())
        {
            if (DirExists(CurrentSystem))
            {
                char pathBuffer[PATH_MAX];
                CreateSystemPathName(currentIndex, pathBuffer, sizeof(pathBuffer));
                Rename(CurrentSystem, pathBuffer);
                newestIndex = currentIndex;
            }
            InstallFromFlash(newestIndex);
        }

        // We may have installed a new system or we may have died before a previous
        // system update completed ldconfig (which is a relatively long operation of several
        // seconds). The existence of the OldFwDir is another indication that we might not
        // have the correct lib paths cached.
        if (FileExists(LdconfigNotDoneMarkerFile) || DirExists(OldFwDir))
        {
            UpdateLdSoCache(CurrentSystem);
        }
        // If this exists at this point, it needs to be cleaned up.
        if (DirExists(OldFwDir))
        {
            RecursiveDelete(OldFwDir);
        }

        Launch();
    }
    return 0;
}

