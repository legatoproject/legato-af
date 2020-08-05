#include "legato.h"
#include "interfaces.h"

static int stdoutFd = -1;

//--------------------------------------------------------------------------------------------------
/**
 * Path of test file for fStream_SetFileFd
 */
//--------------------------------------------------------------------------------------------------
#define FILE_NAME "fStreamTestFile"

//--------------------------------------------------------------------------------------------------
/**
 * fifo path for fStream_SetFifoFd
 */
//--------------------------------------------------------------------------------------------------
#define FIFO_NAME "/tmp/rpcTest"

//--------------------------------------------------------------------------------------------------
/**
 * Strings used for writing to standard out of remote process
 */
//--------------------------------------------------------------------------------------------------
#define STEP1_SUCCESS "Received your stdoutFd successfully\n"
#define STEP2_ERROR "Error in opening file to send\n"
#define STEP2_SUCCESS "Successfully opened file to send\n"
#define STEP3_ERROR "Error in creating fifo to send\n"
#define STEP3_SUCCESS "Sucessfully created fifo to send\n"
#define STEP4_ERROR "Error in opening fifo to send\n"
#define STEP4_SUCCESS "Sucessfully opened fifo to send\n"

#define FIFO_DATA_FORMAT "FIFO DATA ROUND %d\n"
#define MAX_INT_DIGITS (241 * sizeof(int) / 100 + 1)

COMPONENT_INIT
{
    // get the other sides stdout and use for printing info through the process
    fStream_GetStdoutFd(&stdoutFd);
    if (stdoutFd < 0)
    {
        LE_ERROR("Received Invalid stdout file descriptor");
        goto end;
        return;
    }
    else
    {
        LE_INFO("Received Standard out file descriptor:%d", stdoutFd);
        le_fd_Write(stdoutFd, STEP1_SUCCESS, strlen(STEP1_SUCCESS));
    }
    /*
     * le_fd interface on RTOS does not support regular files at the moment.
     */
#if LE_CONFIG_LINUX
    // first open a file and give the fd to the other side:
    int fileFd = le_fd_Open(FILE_NAME, O_RDONLY);
    if (fileFd < 0)
    {
        LE_ERROR("Error in opening file: %s",FILE_NAME);
        le_fd_Write(stdoutFd, STEP2_ERROR, strlen(STEP2_ERROR));
        goto end;
    }
    else
    {
        LE_INFO("Successfully open file %s to send", FILE_NAME);
        le_fd_Write(stdoutFd, STEP2_SUCCESS, strlen(STEP2_SUCCESS));
    }
    fStream_SetFileFd(fileFd);
#endif

    // then open a pipe and give the fd to the other side
    if ( (-1 == le_fd_MkFifo(FIFO_NAME, S_IRWXU | S_IRWXG | S_IRWXO)) && (EEXIST != errno) )
    {
        LE_ERROR("Failed to create fifo errno:[%" PRId32 "]", (int32_t)errno);
        le_fd_Write(stdoutFd, STEP3_ERROR, strlen(STEP3_ERROR));
        goto end;
    }
    else
    {
        LE_INFO("Successfully created fifo %s to send", FIFO_NAME);
        le_fd_Write(stdoutFd, STEP3_SUCCESS, strlen(STEP3_SUCCESS));
    }
    int theirFifoFd = le_fd_Open(FIFO_NAME, O_RDONLY | O_NONBLOCK);
    int ourFifoFd = le_fd_Open(FIFO_NAME, O_WRONLY | O_NONBLOCK);
    if ( ourFifoFd < 0 || theirFifoFd < 0)
    {
        LE_ERROR("Error in opening fifo %s", FIFO_NAME);
        le_fd_Write(stdoutFd, STEP4_ERROR, strlen(STEP4_ERROR));
        goto end;
    }
    else
    {
        LE_INFO("Successfully opened fifo %s to send", FIFO_NAME);
        le_fd_Write(stdoutFd, STEP4_SUCCESS, strlen(STEP4_SUCCESS));
    }

    // make our fd blocking now:
    le_fd_Fcntl(ourFifoFd, F_SETFL, le_fd_Fcntl(ourFifoFd, F_GETFL) & (~O_NONBLOCK));

    fStream_SetFifoFd(theirFifoFd);

    for (int i = 0; i < 100 ; i++)
    {
        char fifoData[sizeof(FIFO_DATA_FORMAT) + MAX_INT_DIGITS];
        snprintf(fifoData, sizeof(fifoData), FIFO_DATA_FORMAT, i);
        le_fd_Write(ourFifoFd, fifoData, strlen(fifoData));
    }

    le_fd_Close(ourFifoFd);

end:
    if (stdoutFd >= 0)
    {
        le_fd_Close(stdoutFd);
    }
}
