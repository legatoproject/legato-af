//--------------------------------------------------------------------------------------------------
/**
 * Tests IMA SMACK protection.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"
#include "user.h"
#include "smack.h"
#include <mqueue.h>


#define CMD_LEN                1024


#define FILE_MSG               "Message from client"
#define CLIENT_RO_DIR          "/legato/systems/current/apps/imaFileClient/read-only/"
#define CLIENT_WR_DIR          "/legato/systems/current/appsWriteable/imaFileClient/"
#define SHELL_FILE             "helloShell"
#define BIN_FILE               "client"
#define TEST_FILE2             "testFile2"


#define CLIENT_RO_SHELL_FILE   CLIENT_RO_DIR "usr/bin/" SHELL_FILE
#define CLIENT_WR_SHELL_FILE   CLIENT_WR_DIR SHELL_FILE


#define CLIENT_RO_BIN_FILE     CLIENT_RO_DIR "bin/" BIN_FILE
#define CLIENT_WR_BIN_FILE     CLIENT_WR_DIR BIN_FILE


#define CLIENT_RO_TEST_FILE2   CLIENT_RO_DIR "testDir/" TEST_FILE2
#define CLIENT_WR_TEST_FILE2   CLIENT_WR_DIR TEST_FILE2


//--------------------------------------------------------------------------------------------------
/**
 * Copy IMA protected file to writable area and then try to write. Log fatal error if write can be
 * done
 */
//--------------------------------------------------------------------------------------------------
void CheckFdWrite
(
    const char *srcPathString, ///<[IN] file path to be copied
    const char *destPathString,///<[IN] where file should be copied
    char *successString,       ///<[IN] Success String to be printed
    char *errorString          ///<[IN] Error String to be printed
)
{
    char cmd[CMD_LEN] = "";
    int fdMods[] = {O_WRONLY, O_RDWR, O_APPEND};
    snprintf(cmd, sizeof(cmd), "cp -p %s %s", srcPathString, destPathString);
    LE_FATAL_IF(0 != system(cmd), "Failed to copy, cmd: '%s'", cmd);

    int i = 0;

    for (i = 0; i < sizeof(fdMods)/sizeof(int); i++)
    {
        if (O_WRONLY == fdMods[i])
        {
            LE_INFO("Test writing file by opening O_WRONLY mode");
        }
        else if (O_RDWR == fdMods[i])
        {
            LE_INFO("Test writing file by opening O_RDWR mode");
        }
        else
        {
            LE_INFO("Test writing file by opening O_APPEND mode");
        }

        int fd = -1;

        fd = open(destPathString, fdMods[i]);

        if (fd < 0)
        {
            LE_ERROR("Failed to open file: %s(%m)", destPathString);
            continue;
        }

        // Try to write something on fd.
        int result = write(fd, FILE_MSG, sizeof(FILE_MSG));

        // No need of fd anymore. Close it.
        close(fd);

        if (-1 == result)
        {
           LE_INFO(" %s '%s' (%m)", successString, destPathString);
        }
        else
        {
           LE_FATAL(" %s '%s' (%m)", errorString, destPathString);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Log fatal errors by killing the calling process after closing the opened file descriptor and
 * logging the message at EMERGENCY level.
 *
 * Accepts printf-style arguments, consisting of a path string which is used by open, flags followed
 * by successString and errorString to be printed.
 */
//--------------------------------------------------------------------------------------------------
void CheckFdOpen
(
    const char *pathString, ///<[IN] file path to be opened
    int flags,              ///<[IN] file access modes
    char *successString,    ///<[IN] Success String to be printed
    char *errorString       ///<[IN] Error String to be printed
)
{
    int fd = -1;
    fd = open(pathString, flags);
    if (-1 == fd)
    {
       LE_INFO(" %s '%s' (%m)", successString, pathString);
    }
    else
    {
       close (fd);
       LE_FATAL(" %s '%s' (%m)", errorString, pathString);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Change the app-process's uid and gid and try to access some SMACK protected file Log fatal errors
 * if the process is able to access those files.
 */
//--------------------------------------------------------------------------------------------------
void filePasser_RogueAccess
(
    void
)
{
    // Change UID and GID and try to access some internal files inside fileClient
    // It should be ok to have hardcoded username in unit-test app.
    const char* userName = "appLegato100";
    uid_t uid;
    gid_t gid;
    LE_FATAL_IF(user_Create(userName, &uid, &gid) == LE_FAULT, "Can't create user: %s", userName);

    // Clear our supplementary groups list.
    LE_FATAL_IF(setgroups(0, NULL) == -1, "Could not set the supplementary groups list.  %m.");

    // Set our process's primary group ID.
    LE_FATAL_IF(setgid(gid) == -1, "Could not set the group ID.  %m.");

    LE_FATAL_IF(setuid(uid) == -1, "Could not set the user ID.  %m.");

    // Attempt to access installed app file.
    CheckFdOpen(CLIENT_RO_SHELL_FILE, O_RDONLY,
               "Success: Could not access installed file",
               "Failed: Accessed file");

    // Attempt to access installed app file.
    CheckFdOpen(CLIENT_RO_BIN_FILE, O_RDONLY,
                "Success: Could not access installed file",
                "Failed: Accessed file");

    // Attempt to access config file.
    CheckFdOpen(CLIENT_RO_TEST_FILE2, O_RDONLY,
                "Success: Could not access installed file",
                "Failed: Accessed file");
}


//--------------------------------------------------------------------------------------------------
/**
 * Try to write something in IMA-SMACK protected files Log fatal errors if write succeeds.
 */
//--------------------------------------------------------------------------------------------------
void filePasser_RogueWrite
(
    void
)
{
    // Attempt to access installed app file.
    CheckFdWrite(CLIENT_RO_SHELL_FILE, CLIENT_WR_SHELL_FILE,
               "Success: Could not write installed file",
               "Failed: Wrote file");

    // Attempt to access installed bundled file.
    CheckFdWrite(CLIENT_RO_TEST_FILE2, CLIENT_WR_TEST_FILE2,
                "Success: Could not write installed file",
                "Failed: Wrote file");
}


//--------------------------------------------------------------------------------------------------
/**
 * Try to access client fd passed via IPC. Log a fatal error if server can't access and read it.
 */
//--------------------------------------------------------------------------------------------------
void filePasser_PassFd(int fileDescriptor)
{
    LE_INFO("Received the file descriptor from the client.");
    LE_INFO("Reading the file to see what it said.");

    char buf[1000] = "\0";

    ssize_t readRet = read(fileDescriptor, buf, sizeof(buf)/2);
    LE_FATAL_IF (readRet < 0, "Unable to read on file descriptor %d (%m)", fileDescriptor);

    LE_INFO("Text in file: '%s'", buf);

    LE_FATAL_IF(strstr(FILE_MSG, buf) != 0,
                "Text in file should be '%s' but was '%s'", FILE_MSG, buf);

    LE_INFO("File descriptor was passed correctly.");

    close(fileDescriptor);
}


COMPONENT_INIT
{
    LE_INFO("Opening '%s'", CLIENT_RO_SHELL_FILE);
    int fd = open(CLIENT_RO_SHELL_FILE, O_RDONLY);

    if (fd < 0)
    {
        LE_ERROR("Failed to open '%s' (%m)", CLIENT_RO_SHELL_FILE);
    }
    else
    {
        char buf[1024] = "";
        ssize_t result = read(fd, buf, sizeof(buf));
        if (result < 0)
        {
            LE_ERROR("Error in reading file: '%s' fd=%d (%m)", CLIENT_RO_SHELL_FILE, fd);
        }
        else
        {
            LE_INFO("Read file '%s', buf: %s, fd=%d", CLIENT_RO_SHELL_FILE, buf, fd);
        }
        close(fd);
    }
}
