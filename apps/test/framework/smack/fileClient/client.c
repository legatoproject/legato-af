//--------------------------------------------------------------------------------------------------
/**
 * Test passing of file descriptors over IPC.
 *
 * Also tests the creation and sharing of mqueues.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"
#include <mqueue.h>



COMPONENT_INIT
{
    // Test file descriptor passing.
    LE_INFO("Open the test file and send the fd to the server.");

    int fd = open("/bin/testFile", O_RDONLY);

    LE_ASSERT(fd >= 0);

    LE_INFO("Passing the fd to the server.");

    filePasser_PassFd(fd);
}
