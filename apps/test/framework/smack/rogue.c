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


COMPONENT_INIT
{
    uid_t uid;
    LE_ASSERT(user_GetAppUid("fileClient", &uid) == LE_OK);

    // Set our UID to non-root.
    LE_FATAL_IF(setgid(uid) == -1, "Could not set the group ID.  %m.");
    LE_FATAL_IF(setuid(uid) == -1, "Could not set the user ID.  %m.");

    // Attempt to access installed app file.
    LE_FATAL_IF(open("/legato/systems/current/apps/fileClient/root.cfg", O_RDONLY) == 0,
                "Rogue process accessed installed file.");

    LE_INFO("Success: Rogue process could not access installed file.  %m.");

    // Attempt to access sandboxed file.
    LE_FATAL_IF(open("/tmp/legato/sandboxes/fileClient/testDir/testFile2", O_RDONLY) == 0,
                "Rogue process accessed sandboxed file.");

    LE_INFO("Success: Rogue process could not access sandboxed file.  %m.");

    // Attempt to access config file.
    LE_FATAL_IF(open("/legato/systems/current/configTree/system.rock", O_RDONLY) == 0,
                "Rogue process accessed sandboxed file.");

    LE_FATAL_IF(open("/legato/systems/current/configTree/system.paper", O_RDONLY) == 0,
                "Rogue process accessed sandboxed file.");

    LE_FATAL_IF(open("/legato/systems/current/configTree/system.scissors", O_RDONLY) == 0,
                "Rogue process accessed sandboxed file.");

    LE_INFO("Success: Rogue process could not access config file.  %m.");

    exit(EXIT_SUCCESS);
}
