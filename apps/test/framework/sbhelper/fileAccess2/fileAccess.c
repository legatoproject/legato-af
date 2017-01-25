//--------------------------------------------------------------------------------------------------
/**
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"


static void Access(const char* pathPtr, int mode)
{
    LE_FATAL_IF(open(pathPtr, mode) == -1, "Could not open %s.  %m.", pathPtr);
}


COMPONENT_INIT
{
    pid_t pid = fork();

    if (pid == 0)
    {
        Access("/bin/cat", O_RDONLY);
        Access("/dev/random", O_RDONLY);
    }
    else
    {
        Access("/usr/bin/id", O_RDONLY);
        Access("/dev/urandom", O_WRONLY);
        Access("/dev/urandom", O_RDONLY);
    }
}
