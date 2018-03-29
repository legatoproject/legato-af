//--------------------------------------------------------------------------------------------------
/**
 * Test passing of file descriptors over IPC. Also tests IMA smack label protection.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"



COMPONENT_INIT
{
    // Test file descriptor passing.
    LE_INFO("Open the test file and send the fd to the server.");

    int fd = open("/bin/testFile", O_RDONLY);

    LE_ASSERT(fd >= 0);

    LE_INFO("Passing the fd to the server.");

    // No need to close the fd as it will be closed by underlying IPC messaging code.
    filePasser_PassFd(fd);

    LE_INFO("Testing whether server can perform some rogue writing to client protected files");
    filePasser_RogueWrite();

    LE_INFO("Testing whether server can perform some rogue access to client files");
    filePasser_RogueAccess();

}
