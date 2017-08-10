//--------------------------------------------------------------------------------------------------
/**
 * This is a rogue process that runs outside of any sandbox and is attempting to access files in
 * other applications.
 *
 * @todo Currently this app must run as a non-root user but once onlycap is set this process should
 *       be run as root.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "user.h"

//--------------------------------------------------------------------------------------------------
/**
 * Log fatal errors by killing the calling process after closing the opened file descriptor and
 * logging the message at EMERGENCY level.  This macro never returns.
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
    int fd =0;
    fd = open(pathString, flags);
    if (-1 == fd)
    {
       LE_INFO(" %s ", successString);
    }
    else
    {
       close (fd);
       LE_FATAL(" %s", errorString)
    }
}

COMPONENT_INIT
{
    uid_t uid;
    LE_ASSERT(user_GetAppUid("fileClient", &uid) == LE_OK);

    // Set our UID to non-root.
    LE_FATAL_IF(setgid(uid) == -1, "Could not set the group ID.  %m.");
    LE_FATAL_IF(setuid(uid) == -1, "Could not set the user ID.  %m.");

    // Attempt to access installed app file.
    CheckFdOpen("/legato/systems/current/apps/fileClient/root.cfg", O_RDONLY,
                "Success: Rogue process could not access installed file.",
                "Rogue process accessed installed file.");

    // Attempt to access sandboxed file.
    CheckFdOpen("/tmp/legato/sandboxes/fileClient/testDir/testFile2", O_RDONLY,
                "Success: Rogue process could not access file.",
                "Rogue process accessed sandboxed file.");

    // Attempt to access config file.
    CheckFdOpen("/legato/systems/current/configTree/system.rock", O_RDONLY,
                "Success: Rogue process could not access configTree/system.rock file.",
                "Rogue process accessed sandboxed file.");

    CheckFdOpen("/legato/systems/current/configTree/system.paper", O_RDONLY,
                "Success: Rogue process could not access configTree/system.paper file.",
                "Rogue process accessed sandboxed file.");

    CheckFdOpen("/legato/systems/current/configTree/system.scissors", O_RDONLY,
                "Success: Rogue process could not access config file..",
                "Rogue process accessed sandboxed file.");

    exit(EXIT_SUCCESS);
}
