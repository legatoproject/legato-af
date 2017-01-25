//--------------------------------------------------------------------------------------------------
/**
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"


static void System(const char* pathPtr)
{
    LE_FATAL_IF(system(pathPtr) != 0, "Could not execute %s.", pathPtr);
}


static void* SubThread(void* contextPtr)
{
    LE_INFO("***** SubThread ****");

    System("ls");

    // tests execve in sub thread.
    execlp("fileAccess1", "fileAccess1", (char*)NULL);

    LE_FATAL("Could not exec fileAccess1.  %m.");
}


COMPONENT_INIT
{
    LE_INFO("***** Parent thread ****");
    le_thread_Start(le_thread_Create("SubThread", SubThread, NULL));
}
