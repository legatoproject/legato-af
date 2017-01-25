//--------------------------------------------------------------------------------------------------
/**
 * Copyright (C) Sierra Wireless Inc.
 **/
//--------------------------------------------------------------------------------------------------

#include "legato.h"

static void FileEventHandler
(
    int fd,
    short events
)
//--------------------------------------------------------------------------------------------------
{
    LE_ASSERT((events & POLLIN) == events);

    char buff[10];

    int byteCount = read(fd, buff, sizeof(buff));

    if (byteCount == 0)
    {
        exit(EXIT_SUCCESS);
    }

    if (byteCount < 0)
    {
        if (errno != EINTR)
        {
            LE_FATAL("read() failed: %m");
        }
    }

    char* buffPtr = buff;

    while (byteCount > 0)
    {
        int bytesWritten;
        do
        {
            bytesWritten = write(1, buffPtr, byteCount);
        }
        while ((bytesWritten == -1) && (errno == EINTR));

        if (bytesWritten < 0)
        {
            LE_FATAL("write() failed: %m");
        }

        byteCount -= bytesWritten;
        buffPtr += bytesWritten;
    }
}


COMPONENT_INIT
{
    const char* fileName = le_arg_GetArg(0);
    if (fileName == NULL)
    {
        fprintf(stderr, "File name not specified.\n");
        exit(EXIT_FAILURE);
    }

    int fd = open(fileName, O_RDONLY);

    if (fd == -1)
    {
        fprintf(stderr, "Failed to open file '%s' for reading (%m).\n", fileName);
        exit(EXIT_FAILURE);
    }

    le_fdMonitor_Create("fd monitor", fd, FileEventHandler, POLLIN);
}
