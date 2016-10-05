/**
 * This module implements the unit tests for AT server API.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 *
 */
#include "legato.h"
#include "defs.h"

#define DSIZE           512     /* default buffer size */
#define SERVER_TIMEOUT  2000    /* server timeout in milliseconds */

//--------------------------------------------------------------------------------------------------
/**
 * list of legato error messages
 *
 */
//--------------------------------------------------------------------------------------------------
static const char* ErrorMsg[] =
{
    [0] = "Successful.",
    [1] = "Referenced item does not exist or could not be found.",
    [2] = "LE_NOT_POSSIBLE",
    [3] = "An index or other value is out of range.",
    [4] = "Insufficient memory is available.",
    [5] = "Current user does not have permission to perform requested action.",
    [6] = "Unspecified internal error.",
    [7] = "Communications error.",
    [8] = "A time-out occurred.",
    [9] = "An overflow occurred or would have occurred.",
    [10] = "An underflow occurred or would have occurred.",
    [11] = "Would have blocked if non-blocking behaviour was not requested.",
    [12] = "Would have caused a deadlock.",
    [13] = "Format error.",
    [14] = "Duplicate entry found or operation already performed.",
    [15] = "Parameter is invalid.",
    [16] = "The resource is closed.",
    [17] = "The resource is busy.",
    [18] = "The underlying resource does not support this operation.",
    [19] = "An IO operation failed.",
    [20] = "Unimplemented functionality.",
    [21] = "A transient or temporary loss of a service or resource.",
    [22] = "The process, operation, data stream, session, etc. has stopped.",
};

//--------------------------------------------------------------------------------------------------
/**
 * shared data between threads
 *
 */
//--------------------------------------------------------------------------------------------------
static SharedData_t SharedData;

void* AtServer(void* contextPtr);

//--------------------------------------------------------------------------------------------------
/**
 * convert \r\n into <>
 * example: at\r => at<
 *          \r\nOK\r\n => <>OK<>
 *
 */
//--------------------------------------------------------------------------------------------------
static char* PrettyPrint
(
    char* strPtr
)
{
    static char copy[DSIZE];
    char* swapPtr;

    memset(copy, 0, DSIZE);
    strncpy(copy, strPtr, strlen(strPtr));

    swapPtr = copy;

    while(*swapPtr)
    {
        switch (*swapPtr) {
        case '\r':
            *swapPtr = '<';
            break;
        case '\n':
            *swapPtr = '>';
            break;
        default: break;
        }
        swapPtr++;
    }

    return copy;
}

//--------------------------------------------------------------------------------------------------
/**
 * send an AT command and test on an expected result
 *
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
    char errMsg[DSIZE];
    char buf[DSIZE];
    struct epoll_event ev;
    int ret = 0;
    int offset = 0;
    int count = 0;
    ssize_t size = 0;


    memset(buf, 0 , DSIZE);

    snprintf(buf, strlen(commandsPtr)+2, "%s\r", commandsPtr);

    LE_DEBUG("Commands: %s", PrettyPrint(buf));

    if (write(fd, buf, (size_t) strlen(buf)) == -1)
    {
        memset(errMsg, 0, DSIZE);
        LE_ERROR("write failed: %s",
            strerror_r(errno, errMsg, DSIZE));
        return LE_IO_ERROR;
    }

    count = strlen(expectedResponsePtr);
    memset(buf, 0 , DSIZE);

    while (count > 0)
    {
        ret = epoll_wait(epollFd, &ev, 1, SERVER_TIMEOUT);
        if (ret == -1)
        {
            memset(errMsg, 0, DSIZE);
            LE_ERROR("epoll wait failed: %s",
                strerror_r(errno, errMsg, DSIZE));
            return LE_IO_ERROR;
        }

        if  (!ret)
        {
            LE_ERROR("Timed out waiting for server's response");
            return LE_TIMEOUT;
        }

        if (ev.data.fd != fd)
        {
            LE_ERROR("%s", strerror_r(EBADF, errMsg, DSIZE));
            return LE_IO_ERROR;
        }

        if (ev.events & EPOLLRDHUP)
        {
            LE_ERROR("%s", strerror_r(ECONNRESET, errMsg, DSIZE));
            return LE_TERMINATED;
        }

        size = read(fd, buf+offset, DSIZE);
        if (size == -1)
        {
            memset(errMsg, 0, DSIZE);
            LE_ERROR("read failed: %s",
                strerror_r(errno, errMsg, DSIZE));
            return LE_IO_ERROR;
        }

        offset += (int)size;
        count -= (int)size;
    }

    LE_DEBUG("Response: %s", PrettyPrint(buf));
    LE_DEBUG("Expected: %s", PrettyPrint((char *)expectedResponsePtr));

    if (expectedResponsePtr) {
        if (strcmp(buf, expectedResponsePtr))
        {
            LE_ERROR("response %s", PrettyPrint(buf));
            LE_ERROR("expected %s", PrettyPrint((char*)expectedResponsePtr));
            return LE_FAULT;
        }
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * host thread function
 *
 */
//--------------------------------------------------------------------------------------------------
static void* AtHost
(
    void* contextPtr
)
{
    SharedData_t* sharedDataPtr;
    char buf[DSIZE];
    char errMsg[DSIZE];
    struct sockaddr_un addr;
    int socketFd;
    int epollFd;
    struct epoll_event event;
    struct timespec ts;
    int ret;

    LE_DEBUG("Host Started");

    sharedDataPtr = (SharedData_t *)contextPtr;

    memset(buf, 0, DSIZE);

    epollFd = epoll_create1(0);
    if (epollFd == -1)
    {
        memset(errMsg, 0, DSIZE);
        LE_ERROR("epoll_create1 failed: %s",
            strerror_r(errno, errMsg, DSIZE));
        errno = LE_TERMINATED;
        return (void *) &errno;
    }

    socketFd = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (socketFd == -1)
    {
        memset(errMsg, 0, DSIZE);
        LE_ERROR("socket failed: %s",
            strerror_r(errno, errMsg, DSIZE));

        close(socketFd);
        close(epollFd);
        errno = LE_TERMINATED;
        return (void *) &errno;
    }

    event.events = EPOLLIN | EPOLLRDHUP;
    event.data.fd = socketFd;

    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, socketFd, &event))
    {
        memset(errMsg, 0, DSIZE);
        LE_ERROR("epoll_ctl failed: %s",
            strerror_r(errno, errMsg, DSIZE));

        close(socketFd);
        close(epollFd);
        errno = LE_TERMINATED;
        return (void *) &errno;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, sharedDataPtr->devPathPtr, sizeof(addr.sun_path)-1);

    /* wait for the server to bind to the socket */
    pthread_mutex_lock(&sharedDataPtr->mutex);
    while(!sharedDataPtr->ready)
    {
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += SERVER_TIMEOUT/1000;

        errno = pthread_cond_timedwait(
                    &sharedDataPtr->cond, &sharedDataPtr->mutex, &ts);
        if (errno)
        {
            memset(errMsg, 0, DSIZE);
            LE_ERROR("pthread_cond_timedwait failed: %s",
                strerror_r(errno, errMsg, DSIZE));

            close(socketFd);
            close(epollFd);
            errno = LE_TIMEOUT;
            return (void *) &errno;
        }
    }

    pthread_mutex_unlock(&sharedDataPtr->mutex);

    if (connect(socketFd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        memset(errMsg, 0, DSIZE);
        LE_ERROR("connect failed: %s",
            strerror_r(errno, errMsg, DSIZE));

        close(socketFd);
        close(epollFd);
        errno = LE_COMM_ERROR;
        return (void *) &errno;
    }

    ret = SendCommandsAndTest(socketFd, epollFd, "AT",
                "\r\n TYPE: ACT\r\n"
                "\r\nOK\r\n");
    if (ret)
    {
        goto err;
    }

    ret = SendCommandsAndTest(socketFd, epollFd, "ATI",
                "\r\nManufacturer: Sierra Wireless, Incorporated\r\n"
                "Model: WP8548\r\n"
                "Revision: SWI9X15Y_07.10.04.00 "
                "12c1700 jenkins 2016/06/02 02:52:45\r\n"
                "IMEI: 359377060009700\r\n"
                "IMEI SV: 42\r\n"
                "FSN: LL542500111503\r\n"
                "+GCAP: +CGSM\r\n"
                "\r\nOK\r\n");
    if (ret)
    {
        goto err;
    }

    ret = SendCommandsAndTest(socketFd, epollFd, "ATASVE",
                "\r\nA TYPE: ACT\r\n"
                "\r\nS TYPE: ACT\r\n"
                "\r\nV TYPE: ACT\r\n"
                "\r\nE TYPE: ACT\r\n"
                "\r\nOK\r\n");
    if (ret)
    {
        goto err;
    }

    ret = SendCommandsAndTest(socketFd, epollFd, "ATASVEB",
                "\r\nA TYPE: ACT\r\n"
                "\r\nS TYPE: ACT\r\n"
                "\r\nV TYPE: ACT\r\n"
                "\r\nE TYPE: ACT\r\n"
                "\r\nERROR\r\n");
    if (ret)
    {
        goto err;
    }

    ret = SendCommandsAndTest(socketFd, epollFd, "AT+ABCD;+ABCD=?;+ABCD?",
                "\r\n+ABCD TYPE: ACT\r\n"
                "\r\n+ABCD TYPE: TEST\r\n"
                "\r\n+ABCD TYPE: READ\r\n"
                "\r\nOK\r\n");
    if (ret)
    {
        goto err;
    }

    ret = SendCommandsAndTest(socketFd, epollFd,
                "ATE0S3?;+ABCD?;S0?S0=2E1;V0S0=\"3\"+ABCD",
                "\r\nE TYPE: PARA\r\n"
                "E PARAM 0: 0\r\n"
                "\r\nS TYPE: READ\r\n"
                "S PARAM 0: 3\r\n"
                "\r\n+ABCD TYPE: READ\r\n"
                "\r\nS TYPE: READ\r\n"
                "S PARAM 0: 0\r\n"
                "\r\nS TYPE: PARA\r\n"
                "S PARAM 0: 0\r\n"
                "S PARAM 1: 2\r\n"
                "\r\nE TYPE: PARA\r\n"
                "E PARAM 0: 1\r\n"
                "\r\nV TYPE: PARA\r\n"
                "V PARAM 0: 0\r\n"
                "\r\nS TYPE: PARA\r\n"
                "S PARAM 0: 0\r\n"
                "S PARAM 1: 3\r\n"
                "\r\n+ABCD TYPE: ACT\r\n"
                "\r\nOK\r\n");
    if (ret)
    {
        goto err;
    }

    ret = SendCommandsAndTest(socketFd, epollFd,"AT&FE0V1&C1&D2S95=47S0=0",
                "\r\n&F TYPE: ACT\r\n"
                "\r\nE TYPE: PARA\r\n"
                "E PARAM 0: 0\r\n"
                "\r\nV TYPE: PARA\r\n"
                "V PARAM 0: 1\r\n"
                "\r\n&C TYPE: PARA\r\n"
                "&C PARAM 0: 1\r\n"
                "\r\n&D TYPE: PARA\r\n"
                "&D PARAM 0: 2\r\n"
                "\r\nS TYPE: PARA\r\n"
                "S PARAM 0: 95\r\n"
                "S PARAM 1: 47\r\n"
                "\r\nS TYPE: PARA\r\n"
                "S PARAM 0: 0\r\n"
                "S PARAM 1: 0\r\n"
                "\r\nOK\r\n");
    if (ret)
    {
        goto err;
    }

    ret = SendCommandsAndTest(socketFd, epollFd, "AT+CBC=?",
                "\r\n+CBC: (0-2),(1-100),(voltage)\r\n"
                "\r\nOK\r\n");
    if (ret)
    {
        goto err;
    }

    ret = SendCommandsAndTest(socketFd, epollFd, "AT+CBC",
                "\r\n+CBC: 1,50,4190\r\n"
                "\r\nOK\r\n"
                "\r\n+CBC: 1,70,4190\r\n"
                "\r\n+CBC: 2,100,4190\r\n");
    if (ret)
    {
        goto err;
    }

    ret = SendCommandsAndTest(socketFd, epollFd, "AT+DEL="
                "\"AT\",\"ATI\",\"AT+CBC\",\"AT+ABCD\",\"ATA\",\"AT&F\","
                "\"ATS\",\"ATV\",\"AT&C\",\"AT&D\",\"ATE\"",
                "\r\nOK\r\n");
    if (ret)
    {
        goto err;
    }

    ret = SendCommandsAndTest(socketFd, epollFd, "AT+STOP?",
                "\r\nERROR\r\n");
    if (ret)
    {
        goto err;
    }

    ret = SendCommandsAndTest(socketFd, epollFd, "AT+STOP",
                "");
    if (ret)
    {
        goto err;
    }

    return NULL;

    err:
        close(socketFd);
        close(epollFd);
        errno = ret;
        return (void *) &errno;
}

//--------------------------------------------------------------------------------------------------
/**
 * main of the test
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    le_thread_Ref_t atServerThread;
    le_thread_Ref_t atHostThread;
    int* retVal;

    // To reactivate for all DEBUG logs
#ifdef DEBUG
    LE_INFO("DEBUG MODE");
    le_log_SetFilterLevel(LE_LOG_DEBUG);
#endif

    SharedData.devPathPtr = "\0at-dev";
    pthread_mutex_init(&SharedData.mutex, NULL);
    pthread_cond_init(&SharedData.cond, NULL);
    SharedData.ready = false;

    atServerThread = le_thread_Create(
        "atServerThread", AtServer, (void *)&SharedData);
    atHostThread = le_thread_Create(
        "atHostThread", AtHost, (void *)&SharedData);

    le_thread_SetJoinable(atHostThread);

    le_thread_Start(atServerThread);
    le_thread_Start(atHostThread);

    le_thread_Join(atHostThread, (void *) &retVal);

    pthread_mutex_destroy(&SharedData.mutex);
    pthread_cond_destroy(&SharedData.cond);

    if (retVal)
    {
        LE_ERROR("atServer Unit Test: FAIL: %s", ErrorMsg[-(*retVal)]);
        exit(*retVal);
    }

    LE_INFO("atServer Unit Test: PASS");
    exit(0);
}
