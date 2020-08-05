#include "legato.h"
#include "interfaces.h"


static int fifoFd = -1;
static int fileFd = -1;

//--------------------------------------------------------------------------------------------------
/**
 * Handler for fdMonitor on the received fifo.
 */
//--------------------------------------------------------------------------------------------------
static void FdMonitorHandler
(
    int fd,
    short events
)
{
    if (events & POLLIN)
    {
        ssize_t bytesRead;
        do
        {
            char c;
            bytesRead = le_fd_Read(fd, &c, 1);
            if ( bytesRead > 0)
            {
                le_fd_Write(fileno(stdout), &c, 1);
            }
        } while(bytesRead > 0);
    }

    if (events & POLLHUP || events & POLLERR || events & POLLRDHUP)
    {
        LE_INFO("Closing file descriptor: %d", fd);
        le_fdMonitor_Delete(le_fdMonitor_GetMonitor());
        le_fd_Close(fd);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets our fifo fd to the one received from remote side.
 */
//--------------------------------------------------------------------------------------------------
void fStream_SetFifoFd(int fd)
{
    if (fd < 0)
    {
        LE_INFO("Invalid fifo fd received");
        return;
    }
    LE_INFO("fSreamServer: received this fd: %d", fd);
    fifoFd = fd;
    // setup fd monitor on fd and print whatever is read into standard out:
    le_fdMonitor_Ref_t fdMonitor = le_fdMonitor_Create("rpcTestFdMon", fifoFd, FdMonitorHandler , POLLIN);
    LE_INFO("Create fdMonitor: %x", (unsigned int)fdMonitor);
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets our fifo fd to the one received from remote side.
 */
//--------------------------------------------------------------------------------------------------
void fStream_SetFileFd(int fd)
{
    if (fd < 0)
    {
        LE_INFO("Invalid file fd received");
        return;
    }
    fileFd = fd;
    // read file and print into standard out until we reach EOF:
    LE_INFO("File Starts:");
    ssize_t bytesRead;
    do
    {
        char c;
        bytesRead = le_fd_Read(fd, &c, 1);
        if ( bytesRead > 0)
        {
            le_fd_Write(fileno(stdout), &c, 1);
        }
    } while(bytesRead > 0);
    LE_INFO("File ends");
    le_fd_Close(fileFd);
}

//--------------------------------------------------------------------------------------------------
/**
 * Give our standard out fd to the remote side to be used for printing logs
 */
//--------------------------------------------------------------------------------------------------
void fStream_GetStdoutFd(int* fd)
{
    *fd = fileno(stderr);
}

COMPONENT_INIT
{
}
