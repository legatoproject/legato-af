/**
 * This module implements the unit test for port service API.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */


#include "legato.h"
#include "interfaces.h"


//--------------------------------------------------------------------------------------------------
/**
 * Maximum length of buffer.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_LEN_BUFFER            50

//--------------------------------------------------------------------------------------------------
/**
 * Maximum length of monitor name.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_LEN_MONITORNAME       64

//--------------------------------------------------------------------------------------------------
/**
 * Device path for AT command mode and DATA mode.
 */
//--------------------------------------------------------------------------------------------------
#define DEVICE_PATH_ATCMD_MODE    "/tmp/sock0"
#define DEVICE_PATH_DATA_MODE     "/tmp/sock1"

//--------------------------------------------------------------------------------------------------
/**
 * Default buffer size for device information and error messages
 */
//--------------------------------------------------------------------------------------------------
#define DSIZE                     256

//--------------------------------------------------------------------------------------------------
/**
 * Epoll wait timeout in milliseconds.
 */
//--------------------------------------------------------------------------------------------------
#define SERVER_TIMEOUT            10000

//--------------------------------------------------------------------------------------------------
/**
 * Byte length to read from fd.
 */
//--------------------------------------------------------------------------------------------------
#define READ_BYTES                100

//--------------------------------------------------------------------------------------------------
/**
 * Device pool size
 */
//--------------------------------------------------------------------------------------------------
#define DEVICE_POOL_SIZE          2

//--------------------------------------------------------------------------------------------------
/**
 * Client connection timeout in seconds.
 */
//--------------------------------------------------------------------------------------------------
#define CLIENT_CONNECTION_TIMEOUT 5


//--------------------------------------------------------------------------------------------------
/**
 * Port service device reference
 */
//--------------------------------------------------------------------------------------------------
static le_port_DeviceRef_t DeviceRef;

//--------------------------------------------------------------------------------------------------
/**
 * Socket Fd and epoll Fd
 */
//--------------------------------------------------------------------------------------------------
static int AtCmdSockFd = -1;
static int DataSockFd = -1;
static int AtCmdEpollFd = -1;
static int DataEpollFd = -1;

//--------------------------------------------------------------------------------------------------
/**
 * Semaphore reference
 */
//--------------------------------------------------------------------------------------------------
static le_sem_Ref_t Semaphore;

//--------------------------------------------------------------------------------------------------
/**
 * device structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct Device
{
    int32_t            fd;          ///< The file descriptor.
    le_fdMonitor_Ref_t fdMonitor;   ///< fd event monitor associated to Handle.
}
Device_t;

//--------------------------------------------------------------------------------------------------
/**
 * Pool for device context.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t DevicesPool;

//--------------------------------------------------------------------------------------------------
/**
 * File descriptor for data mode.
 */
//--------------------------------------------------------------------------------------------------
static int DataModeFd = -1;


//--------------------------------------------------------------------------------------------------
/**
 * Close a fd and raise a warning in case of an error.
 */
//--------------------------------------------------------------------------------------------------
static void CloseWarn
(
    int fd
)
{
    if (-1 == close(fd))
    {
        LE_WARN("failed to close fd %d: %m", fd);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Open the client socket.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t OpenClientSocket
(
    char* deviceName,    ///< [IN] Device name.
    int* sockFd,         ///< [IN] Socket file descriptor.
    int* epollFd         ///< [IN] Epoll file descriptor.
)
{
    struct epoll_event event = { 0 };
    struct sockaddr_un addr;
    int retVal = -1;

    *epollFd = epoll_create1(0);
    if (-1 == *epollFd)
    {
        LE_ERROR("epoll_create1 failed: %m");
        return LE_FAULT;
    }

    *sockFd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (-1 == *sockFd)
    {
        LE_ERROR("socket failed: %m");
        goto epoll_err;
    }

    event.events = EPOLLIN | EPOLLRDHUP;
    event.data.fd = *sockFd;

    if (-1 == epoll_ctl(*epollFd, EPOLL_CTL_ADD, *sockFd, &event))
    {
        LE_ERROR("epoll_ctl failed: %m");
        goto sock_err;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, deviceName, sizeof(addr.sun_path)-1);

    retVal = connect(*sockFd, (struct sockaddr*)&addr, sizeof(addr));
    if (retVal == -1)
    {
        goto sock_err;
    }

    return LE_OK;

sock_err:
    CloseWarn(*sockFd);
epoll_err:
    CloseWarn(*epollFd);
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Return a description string of error.
 */
//--------------------------------------------------------------------------------------------------
static char* StrError
(
    int err
)
{
    static char errMsg[DSIZE];

#ifdef __USE_GNU
    snprintf(errMsg, DSIZE, "%s", strerror_r(err, errMsg, DSIZE));
#else /* XSI-compliant */
    strerror_r(err, errMsg, sizeof(errMsg));
#endif
    return errMsg;
}

//--------------------------------------------------------------------------------------------------
/**
 * Test on an expected result
 */
//--------------------------------------------------------------------------------------------------
static le_result_t TestResponses
(
    int fd,
    int epollFd,
    const char* expectedResponsePtr
)
{
    char buf[DSIZE];
    struct epoll_event ev;
    int ret = 0;
    int offset = 0;
    int count = 0;
    ssize_t size = 0;

    if (!expectedResponsePtr)
    {
        LE_ERROR("null expected response");
        return LE_FAULT;
    }

    count = strlen(expectedResponsePtr);
    memset(buf, 0 , DSIZE);

    while (count > 0)
    {
        do
        {
            ret = epoll_wait(epollFd, &ev, 1, SERVER_TIMEOUT);
        }
        while ((-1 == ret) && (EINTR == errno));

        if (-1 == ret)
        {
            LE_ERROR("epoll wait failed: %s", strerror(errno));
            return LE_IO_ERROR;
        }

        if (!ret)
        {
            LE_ERROR("Timed out waiting for server's response");
            return LE_TIMEOUT;
        }

        if (ev.data.fd != fd)
        {
            LE_ERROR("%s", strerror(EBADF));
            return LE_IO_ERROR;
        }

        if (ev.events & EPOLLRDHUP)
        {
            LE_ERROR("%s", strerror(ECONNRESET));
            return LE_TERMINATED;
        }

        size = read(fd, buf+offset, DSIZE);
        if (-1 == size)
        {
            LE_ERROR("read failed: %s", strerror(errno));
            return LE_IO_ERROR;
        }

        offset += size;
        count -= size;
    }

    if (strcmp(buf, expectedResponsePtr))
    {
        LE_ERROR("response %s", buf);
        LE_ERROR("expected %s", (char*)expectedResponsePtr);
        return LE_FAULT;
    }
    else
    {
        LE_INFO("AT command send/receive is done.");
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * send an AT command and test on an expected result
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SendCommandsAndTest
(
    int fd,
    int epollFd,
    const char* commandsPtr,
    const char* expectedResponsePtr
)
{
    char buf[DSIZE];
    int size;

    if (NULL == commandsPtr)
    {
        LE_ERROR("commandsPtr is NULL!");
        return LE_FAULT;
    }

    if (NULL == expectedResponsePtr)
    {
        LE_ERROR("expectedResponsePtr is NULL!");
        return LE_FAULT;
    }

    size = strlen(commandsPtr);
    if (DSIZE <= size)
    {
        LE_ERROR("command is too long: %d", size);
        return LE_FAULT;
    }

    memset(buf, 0 , DSIZE);

    snprintf(buf, strlen(commandsPtr) + 2, "%s<", commandsPtr);

    LE_INFO("Commands: %s", buf);

    if (-1 == write(fd, buf, strlen(buf)))
    {
        LE_ERROR("write failed: %s", strerror(errno));
        return LE_IO_ERROR;
    }

    return TestResponses(fd, epollFd, expectedResponsePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Write the data to file descriptor.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SendData
(
    int fd,
    const char* dataPtr
)
{
    if (NULL == dataPtr)
    {
        LE_ERROR("dataPtr is NULL!");
        return LE_FAULT;
    }

    char buf[DSIZE];
    int size;

    size = strlen(dataPtr);
    if (DSIZE <= size)
    {
        LE_ERROR("command is too long: %d", size);
        return LE_FAULT;
    }

    memset(buf, 0 , DSIZE);

    snprintf(buf, strlen(dataPtr)+1, "%s", dataPtr);

    LE_INFO("Data: %s", buf);

    if (-1 == write(fd, buf, strlen(buf)))
    {
        LE_ERROR("write failed: %s", strerror(errno));
        return LE_IO_ERROR;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Testing of le_port_Request() API.
 */
//--------------------------------------------------------------------------------------------------
static void testle_port_Request
(
    void
)
{
    DeviceRef = le_port_Request("undefined");
    LE_ASSERT(NULL == DeviceRef);

    DeviceRef = le_port_Request("modemPort");
    LE_ASSERT(NULL != DeviceRef);

    // Test AT command send/receive on the fd.
    LE_ASSERT_OK(SendCommandsAndTest(AtCmdSockFd, AtCmdEpollFd, "AT+CGDATA=1", "\r\nOK\r\n"));
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is called when data are available to be read on fd.
 */
//--------------------------------------------------------------------------------------------------
static void RxNewData
(
    int fd,      ///< File descriptor to read on
    short events ///< Event reported on fd (expect only POLLIN)
)
{
    char buffer[READ_BYTES];
    ssize_t count;

    memset(buffer, 0x00, READ_BYTES);

    if (events & (POLLIN | POLLPRI))
    {
        count = read(fd, buffer, READ_BYTES);
        if (-1 == count)
        {
            LE_ERROR("read error: %s", StrError(errno));
            le_sem_Post(Semaphore);
            return;
        }
        else if (count > 0)
        {
            if (0 == strcmp(buffer, "+++"))
            {
                le_sem_Post(Semaphore);
                return;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * ClientConnect thread: This function will create the socket and trying to connect the socket with
 * server.
 */
//--------------------------------------------------------------------------------------------------
static void* ClientConnect
(
    void* ctxPtr
)
{
    int counter = 0;

    while (LE_OK != OpenClientSocket(DEVICE_PATH_DATA_MODE, &DataSockFd, &DataEpollFd)
           && (counter < CLIENT_CONNECTION_TIMEOUT))
    {
        LE_INFO("Client socket is not connected!");
        sleep(1);
        counter++;
    }
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * SetDataMode thread: This function will call le_port_SetDataMode() API which will create the
 * socket, monitor the socket events and fetches fd for data mode.
 */
//--------------------------------------------------------------------------------------------------
static void* SetDataMode
(
    void* ctxPtr
)
{
    LE_ASSERT_OK(le_port_SetDataMode(DeviceRef, &DataModeFd));
    LE_INFO("Data mode fd is %d", DataModeFd);
    le_sem_Post(Semaphore);
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * TestDataModeFd thread: This function will send / receive some raw data to data mode fd.
 */
//--------------------------------------------------------------------------------------------------
static void* TestDataModeFd
(
    void* ctxPtr
)
{
    char monitorName[MAX_LEN_MONITORNAME];
    le_fdMonitor_Ref_t fdMonitorRef;

    // Create a File Descriptor Monitor object for the file descriptor.
    snprintf(monitorName, sizeof(monitorName), "Monitor-%d", DataModeFd);

    // Device pool allocation
    DevicesPool = le_mem_CreatePool("DevicesPool", sizeof(Device_t));
    le_mem_ExpandPool(DevicesPool, DEVICE_POOL_SIZE);

    Device_t* devPtr = le_mem_ForceAlloc(DevicesPool);

    memset(devPtr, 0, sizeof(Device_t));

    LE_INFO("Create a new interface for %d", DataModeFd);
    devPtr->fd = DataModeFd;

    fdMonitorRef = le_fdMonitor_Create(monitorName, DataModeFd, RxNewData,
                                       POLLIN | POLLPRI | POLLRDHUP);
    le_fdMonitor_SetContextPtr(fdMonitorRef, devPtr);

    // Send "+++" string to come out from DATA mode.
    LE_ASSERT_OK(SendData(DataSockFd, "+++"));
    le_event_RunLoop();
}

//--------------------------------------------------------------------------------------------------
/**
 * Testing of le_port_SetDataMode() API.
 */
//--------------------------------------------------------------------------------------------------
static void testle_port_SetDataMode
(
    void
)
{
    Semaphore = le_sem_Create("HandlerSem",0);

    le_thread_Start(le_thread_Create("ClientConnect", ClientConnect, NULL));
    le_thread_Start(le_thread_Create("SetDataMode", SetDataMode, NULL));

    le_clk_Time_t timeToWait = {10, 0};
    LE_ASSERT_OK(le_sem_WaitWithTimeOut(Semaphore, timeToWait));

    le_thread_Start(le_thread_Create("TestDataModeFd", TestDataModeFd, NULL));

    // Wait until valid raw data receives.
    LE_ASSERT_OK(le_sem_WaitWithTimeOut(Semaphore, timeToWait));
}

//--------------------------------------------------------------------------------------------------
/**
 * Testing of le_port_SetCommandMode() API.
 */
//--------------------------------------------------------------------------------------------------
static void testle_port_SetCommandMode
(
    void
)
{
    le_atServer_DeviceRef_t deviceRef;
    LE_ASSERT_OK(le_port_SetCommandMode(DeviceRef, &deviceRef));
    LE_INFO("AtServer device reference is %p", deviceRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Testing of le_port_Release() API.
 */
//--------------------------------------------------------------------------------------------------
static void testle_port_Release
(
    void
)
{
    LE_ASSERT_OK(le_port_Release(DeviceRef));
}

//--------------------------------------------------------------------------------------------------
/**
 * UnitTestInit thread: this function initializes the test and run the unit test cases.
 */
//--------------------------------------------------------------------------------------------------
static void* UnitTestInit
(
    void* ctxPtr
)
{
    while (NULL == le_port_Request("modemPort"))
    {
        LE_INFO("JSON parsing is not completed!");
        sleep(1);
    }

    // Open client socket.
    LE_ASSERT_OK(OpenClientSocket(DEVICE_PATH_ATCMD_MODE, &AtCmdSockFd, &AtCmdEpollFd));

    LE_INFO("======== Start UnitTest of port service API ========");

    LE_INFO("======== Test for le_port_Request() API ========");
    testle_port_Request();

    LE_INFO("======== Test for le_port_SetDataMode() API ========");
    testle_port_SetDataMode();

    LE_INFO("======== Test for le_port_SetCommandMode() API ========");
    testle_port_SetCommandMode();

    LE_INFO("======== Test for le_port_Release() API ========");
    testle_port_Release();

    LE_INFO("======== UnitTest of port service API ends with SUCCESS ========");
    exit(EXIT_SUCCESS);
}

//--------------------------------------------------------------------------------------------------
/**
 * Main of the test.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    le_thread_Start(le_thread_Create("UnitTestInit", UnitTestInit, NULL));
}
