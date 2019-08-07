//--------------------------------------------------------------------------------------------------
/**
 * @file fileStreamDemo.c
 *
 * A client application that demonstrates usage of File Stream Service
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"

#define FILE_NAME "streamFile"
#define CHUNK_SIZE 4096
static int Fd;

//--------------------------------------------------------------------------------------------------
/**
 * Reference to the FD Monitor for the input stream
 */
//--------------------------------------------------------------------------------------------------
static le_fdMonitor_Ref_t StoreFdMonitor = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Stop storing the stream content from fd monitor by deleting it.
 */
//--------------------------------------------------------------------------------------------------
static void StopStoringPackage
(
    void
)
{
    if (StoreFdMonitor != NULL)
    {
        le_fdMonitor_Delete(StoreFdMonitor);
        StoreFdMonitor = NULL;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Event handler for the input fd when storing the bytes to disk.
 */
//--------------------------------------------------------------------------------------------------
static void StoreFdEventHandler
(
    int fd,                 ///< [IN] Read file descriptor
    short events            ///< [IN] FD events
)
{
    uint8_t buffer[CHUNK_SIZE];
    ssize_t readCount, writeCount;
    // First check for POLLIN event in order to read data even if the file descriptor is closed
    // by the other side and POLLHUP is set.
    if (events & POLLIN)
    {
        // Read the bytes, retrying if interrupted by a signal.
        do
        {
            readCount = read(fd, buffer, sizeof(buffer));
        }
        while ((-1 == readCount) && (EINTR == errno));

        if (0 == readCount)
        {
            LE_INFO("File read complete.");
            StopStoringPackage();
        }
        else if (readCount > 0)
        {
            writeCount = write(Fd, buffer, readCount);
            LE_INFO("Writing %d bytes to %s", readCount, FILE_NAME);
            if (writeCount != readCount)
            {
                LE_ERROR("Write error.");
                StopStoringPackage();
            }
        }
        else
        {
            LE_ERROR("Error reading.");
            StopStoringPackage();
        }
    }
    else
    {
        StopStoringPackage();
    }
}

//-------------------------------------------------------------------------------------------------
/**
 * Handler that receives status of the download operation and processes the data received on
 * the pipe
 */
//-------------------------------------------------------------------------------------------------
static void HandleDownloadStream
(
    int fd,
    void* contextPtr
)
{
    LE_INFO("Received fd");
    StoreFdMonitor = le_fdMonitor_Create("Store file", fd, StoreFdEventHandler, POLLIN);
}


COMPONENT_INIT
{
    // Create a file where we will write streamed data to.
    Fd = open(FILE_NAME, O_WRONLY | O_TRUNC | O_CREAT, 0);

    if (Fd == -1)
    {
        // Do not proceed if we cannot create this file
        LE_FATAL("Failed to create file %s.", FILE_NAME);
    }

    LE_INFO("Registering download handler on test topic");
    le_fileStreamClient_AddStreamEventHandler("test",
                                              HandleDownloadStream,
                                              NULL);

    LE_INFO("Ready to receive stream");
}
