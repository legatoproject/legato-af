//--------------------------------------------------------------------------------------------------
/**
 * Test passing of file descriptors over IPC.
 *
 * Also tests the sharing of mqueues.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"
#include <mqueue.h>


#define FILE_MSG        "Message from client"

void filePasser_PassFd(int fileDescriptor)
{
    LE_INFO("Received the file descriptor from the client.");
    LE_INFO("Reading the file to see what it said.");

    char buf[1000] = "";

    LE_ASSERT(read(fileDescriptor, buf, sizeof(buf)) > 0);

    LE_INFO("Text in file: '%s'", buf);

    LE_FATAL_IF(strstr(FILE_MSG, buf) != 0,
                "Text in file should be '%s' but was '%s'", FILE_MSG, buf);

    LE_INFO("File descriptor was passed correctly.");

    close(fileDescriptor);
}

COMPONENT_INIT
{
}
