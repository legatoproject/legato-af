 /**
  * This module implements the le_port's tests for switching between the command and data modes and
  * transferring raw data in the data mode.
  *
  * To run this test app, add the following lines in testApps.sdef:
  * @verbatim
    apps:
    {
        $LEGATO_ROOT/apps/platformServices/atService
        $LEGATO_ROOT/apps/platformServices/portService
        portService/portServiceDataModeTest/portServiceDataModeTest
    }
    interfaceSearch:
    {
        $LEGATO_ROOT/interfaces/atServices
    }
    @endverbatim
  *
  * Copyright (C) Sierra Wireless Inc.
  */

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Standard final response when switching to the command mode
 */
//--------------------------------------------------------------------------------------------------
#define ATSERVERUTIL_OK "\r\nOK\r\n"

//--------------------------------------------------------------------------------------------------
/**
 * Standard response when switching to the data mode
 */
//--------------------------------------------------------------------------------------------------
#define ATSERVERUTIL_CONNECT "\r\nCONNECT\r\n"

//--------------------------------------------------------------------------------------------------
/**
 * The number of random AT command execution
 */
//--------------------------------------------------------------------------------------------------
#define TEST_COUNT          1000

//--------------------------------------------------------------------------------------------------
/**
 * Default buffer size for device information and error messages
 */
//--------------------------------------------------------------------------------------------------
#define DSIZE               256

// -------------------------------------------------------------------------------------------------
/**
 *  Test PIPE path length.
 */
// -------------------------------------------------------------------------------------------------
#define PATH_LENGTH 128

//--------------------------------------------------------------------------------------------------
/**
 * Byte length to read from fd.
 */
//--------------------------------------------------------------------------------------------------
#define READ_BYTES          100

//--------------------------------------------------------------------------------------------------
/**
 * Maximum length of monitor name
 */
//--------------------------------------------------------------------------------------------------
#define MAX_LEN_MONITORNAME 64

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of testing AT commands
 */
//--------------------------------------------------------------------------------------------------
#define MAX_LEN_CMDS 4

//--------------------------------------------------------------------------------------------------
/**
 * Thread and semaphore reference.
 */
//--------------------------------------------------------------------------------------------------
static le_sem_Ref_t SemaphoreATServerReady;
static le_sem_Ref_t SemaphoreRawDataReady;
static le_thread_Ref_t AppThreadRef;
static le_thread_Ref_t FdMonitorThreadRef;

//--------------------------------------------------------------------------------------------------
/**
 * Test raw data from simulated MCU to app
 */
//--------------------------------------------------------------------------------------------------
static char WriteBuf[] = {
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyz0123456789+++"
};

//--------------------------------------------------------------------------------------------------
/**
 * AtCmd_t definition
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    const char*                         atCmdPtr;
    le_atServer_CmdRef_t                cmdRef;
    le_atServer_CommandHandlerFunc_t    handlerPtr;
    void*                               contextPtr;
}
AtCmd_t;

//--------------------------------------------------------------------------------------------------
/**
 * AT Commands definition
 */
//--------------------------------------------------------------------------------------------------
static AtCmd_t AtCmdCreation;

//--------------------------------------------------------------------------------------------------
/**
 * This function is called when data are available to be read on fd
 */
//--------------------------------------------------------------------------------------------------
static void RxNewData
(
    int fd,      ///< [IN] File descriptor to read on
    short events ///< [IN] Event reported on fd
)
{
    char buf[READ_BYTES];
    ssize_t count;

    if (events & POLLIN)
    {
        memset(buf, 0, READ_BYTES);
        count = le_fd_Read(fd, buf, READ_BYTES);
        if (-1 == count)
        {
            LE_ERROR("read error: %d", errno);
            return;
        }
        else if (count > 0)
        {
            if (0 == strcmp(buf, "\r\nCONNECT\r\n"))
            {
                LE_INFO("Raw data received by MCU:\n%s", buf);
                LE_TEST_OK(strcmp(buf, ATSERVERUTIL_CONNECT) == 0, "MCU receives raw data");
                ssize_t wc = le_fd_Write(fd, WriteBuf, sizeof(WriteBuf));
                LE_TEST_OK(wc == sizeof(WriteBuf), "MCU sends raw data back");
                le_sem_Post(SemaphoreRawDataReady);
                return;
            }
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * AT command Mode handler
 */
//--------------------------------------------------------------------------------------------------
static void AtCmdModeHandler
(
    le_atServer_CmdRef_t commandRef,
    le_atServer_Type_t   type,
    uint32_t             parametersNumber,
    void*                contextPtr
)
{
    char rsp[LE_ATDEFS_RESPONSE_MAX_BYTES];
    char atCommandName[LE_ATDEFS_COMMAND_MAX_BYTES];
    char param[LE_ATDEFS_PARAMETER_MAX_BYTES];
    int i = 0;

    memset(atCommandName, 0, LE_ATDEFS_COMMAND_MAX_BYTES);

    // Get command name
    LE_ASSERT_OK(le_atServer_GetCommandName(commandRef, atCommandName,
                 LE_ATDEFS_COMMAND_MAX_BYTES));

    snprintf(rsp, LE_ATDEFS_RESPONSE_MAX_BYTES, "%s TYPE: ", atCommandName + 2);

    switch (type)
    {
        case LE_ATSERVER_TYPE_PARA:
            LE_INFO("Type PARA");
            le_utf8_Append(rsp, "PARA", LE_ATDEFS_RESPONSE_MAX_BYTES, NULL);
        break;

        case LE_ATSERVER_TYPE_TEST:
            LE_INFO("Type TEST");
            le_utf8_Append(rsp, "TEST", LE_ATDEFS_RESPONSE_MAX_BYTES, NULL);
        break;

        case LE_ATSERVER_TYPE_READ:
            LE_INFO("Type READ");
            le_utf8_Append(rsp, "READ", LE_ATDEFS_RESPONSE_MAX_BYTES, NULL);
        break;

        case LE_ATSERVER_TYPE_ACT:
            LE_INFO("Type ACT");
            le_utf8_Append(rsp, "ACT", LE_ATDEFS_RESPONSE_MAX_BYTES, NULL);
        break;

        default:
            LE_TEST_INFO("AT command type is not proper!");
            LE_ERROR("AT command type is not proper!");
            pthread_exit(NULL);
        break;
    }

    // Send the command type into an intermediate response
    LE_ASSERT_OK(le_atServer_SendIntermediateResponse(commandRef, rsp));

    // Send parameters into an intermediate response
    for (i = 0; i < parametersNumber; i++)
    {
        memset(param, 0, LE_ATDEFS_PARAMETER_MAX_BYTES);
        LE_ASSERT_OK(le_atServer_GetParameter(commandRef,
                                           i,
                                           param,
                                           LE_ATDEFS_PARAMETER_MAX_BYTES));

        memset(rsp, 0, LE_ATDEFS_RESPONSE_MAX_BYTES);
        snprintf(rsp, LE_ATDEFS_RESPONSE_MAX_BYTES, "%s PARAM %d: %s", atCommandName + 2, i, param);

        LE_ASSERT_OK(le_atServer_SendIntermediateResponse(commandRef, rsp));
        sched_yield();
    }

    if (type != LE_ATSERVER_TYPE_ACT)
    {
        LE_ASSERT_OK(le_atServer_SendFinalResultCode(commandRef, LE_ATSERVER_OK, "", 0));
        sched_yield();
        return;
    }

    le_atServer_DeviceRef_t atServerDevRef = NULL;
    le_result_t result = le_atServer_GetDevice(commandRef, &atServerDevRef);
    if (LE_OK != result)
    {
        LE_ERROR("Cannot get device information! Result: %d", result);
        LE_ASSERT_OK(le_atServer_SendFinalResultCode(commandRef, LE_ATSERVER_ERROR, "", 0));
        return;
    }

    le_port_DeviceRef_t devRefPtr = NULL;
    result = le_port_GetPortReference(atServerDevRef, &devRefPtr);
    if (LE_OK != result)
    {
        LE_ERROR("Cannot get port reference! Result: %d", result);
        LE_ASSERT_OK(le_atServer_SendFinalResultCode(commandRef, LE_ATSERVER_ERROR, "", 0));
        return;
    }

    int atSockFd;
    result = le_port_SetDataMode(devRefPtr, &atSockFd);
    if (LE_OK != result)
    {
        LE_ERROR("le_port_SetDataMode API usage error");
        LE_ASSERT_OK(le_atServer_SendFinalResultCode(commandRef, LE_ATSERVER_ERROR, "", 0));
        return;
    }

    if(-1 == le_fd_Write(atSockFd, ATSERVERUTIL_CONNECT, strlen(ATSERVERUTIL_CONNECT)))
    {
        LE_ERROR("CONNECT write error");
        LE_ASSERT_OK(le_atServer_SendFinalResultCode(commandRef, LE_ATSERVER_ERROR, "", 0));
        return;
    }

    le_sem_Wait(SemaphoreRawDataReady);

    char buf[128] = {0};
    ssize_t ret = le_fd_Read(atSockFd, buf, sizeof(buf) - 1);
    if (ret <= 0)
    {
        LE_ERROR("Fail to read raw data from MCU: %d", errno);
        LE_ASSERT_OK(le_atServer_SendFinalResultCode(commandRef, LE_ATSERVER_ERROR, "", 0));
        return;
    }

    LE_INFO("Raw data received by app:\n%s\n", buf);
    LE_TEST_OK(strcmp(buf, WriteBuf) == 0, "App receives raw data");

    // Close data port
    le_fd_Close(atSockFd);
    result = le_port_SetCommandMode(devRefPtr, &atServerDevRef);
    if(LE_OK != result)
    {
        LE_ERROR("le_port_SetDataMode API usage error");
        LE_ASSERT_OK(le_atServer_SendFinalResultCode(commandRef, LE_ATSERVER_ERROR, "", 0));
        return;
    }

    LE_ASSERT_OK(le_atServer_SendFinalResultCode(commandRef, LE_ATSERVER_OK, "", 0));
}

//--------------------------------------------------------------------------------------------------
/**
 * Add AtServer handler
 */
//--------------------------------------------------------------------------------------------------
static void* ATServerHandler
(
    void* contexPtr
)
{
    le_atServer_ConnectService();
    le_port_ConnectService();

    AtCmdCreation.atCmdPtr = "AT+CLVL";
    AtCmdCreation.handlerPtr = AtCmdModeHandler;
    AtCmdCreation.contextPtr = NULL;

    AtCmdCreation.cmdRef = le_atServer_Create(AtCmdCreation.atCmdPtr);
    LE_ASSERT(AtCmdCreation.cmdRef != NULL);

    le_atServer_AddCommandHandler(AtCmdCreation.cmdRef,
                                  AtCmdCreation.handlerPtr,
                                  AtCmdCreation.contextPtr);

    le_sem_Post(SemaphoreATServerReady);

    le_event_RunLoop();
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Add fdMonitor handler
 */
//--------------------------------------------------------------------------------------------------
static void* FdMonitorHandler
(
    void* contexPtr
)
{
    const char pipePath[PATH_LENGTH] = "/tmp/sock1";
    int fdPipeSrv = le_fd_MkPipe(pipePath, O_RDWR);
    LE_TEST_ASSERT(fdPipeSrv != -1, "Virtual serial device '%s' server-side opened", pipePath);

    char monitorName[MAX_LEN_MONITORNAME];
    snprintf(monitorName, sizeof(monitorName), "Monitor-%d", fdPipeSrv);
    le_fdMonitor_Create(monitorName, fdPipeSrv, RxNewData, POLLIN);

    le_event_RunLoop();
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Execute an AT command and return the command response
 */
//--------------------------------------------------------------------------------------------------
static int ExecATCommand
(
    int fd,
    const char* cmd,
    char* rbuf,
    int rbuf_len
)
{
    ssize_t rc = le_fd_Write(fd, cmd, strlen(cmd));
    if (rc == -1)
    {
        LE_TEST_INFO("Fail to write: %d", errno);
        return -1;
    }

    memset(rbuf, 0, rbuf_len);
    ssize_t ret = 0;
    ssize_t totalRead = 0;
    char buf[128] = {0};
    do
    {
        memset(buf, 0, sizeof(buf));
        ret = le_fd_Read(fd, buf, sizeof(buf) - 1);
        if (ret == -1 && errno != EAGAIN)
        {
            LE_TEST_INFO("Fail to read: %d", errno);
            totalRead = -2;
            break;
        }
        else if (ret > 0)
        {
            if ((totalRead + ret) >= rbuf_len)
            {
                break;
            }

            memcpy(rbuf + totalRead, buf, ret);
            totalRead += ret;

            if (strcmp("\r\nOK\r\n", buf) == 0 || strcmp("\r\nERROR\r\n", buf) == 0)
            {
                break;
            }
        }
    }
    while (ret != 0);

    return totalRead;
}

//--------------------------------------------------------------------------------------------------
/**
 * Main of the test. This test is used to test the port service switching between the Command Mode
 * and the Data Mode. It is also used to test transfering data in the Data Mode.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    SemaphoreATServerReady = le_sem_Create("ATServerReadySem", 0);
    SemaphoreRawDataReady = le_sem_Create("RawDataReadySem", 0);

    // Register AT command handlers and wait for AT commands in a child thread
    AppThreadRef = le_thread_Create("ATServerHandler", ATServerHandler, NULL);
    le_thread_Start(AppThreadRef);

    // Simulate an external MCU in the Data Mode in a child thread
    FdMonitorThreadRef = le_thread_Create("FdMonitorHandler", FdMonitorHandler, NULL);
    le_thread_Start(FdMonitorThreadRef);

    le_sem_Wait(SemaphoreATServerReady);

    int fd = le_fd_Open("/tmp/sock0", O_RDWR);
    if (fd == -1)
    {
        LE_TEST_INFO("Fail to get pipe file descriptor");
        pthread_exit(NULL);
    }

    const char* cmds[MAX_LEN_CMDS] =
    {
        "AT+CLVL\r",
        "AT+CLVL?\r",
        "AT+CLVL=?\r",
        "AT+CLVL\r"
    };

    // Intializes random number generator
    time_t t;
    srand((unsigned) time(&t));

    char buf[1024] = {0};
    int i = 0;
    for (i = 0; i < TEST_COUNT; i++)
    {
        int j = rand() % MAX_LEN_CMDS;

        memset(buf, 0, sizeof(buf));
        LE_INFO("Execute: %s", cmds[j]);
        ssize_t totalRead = ExecATCommand(fd, cmds[j], buf, sizeof(buf));
        LE_INFO("Response (%d bytes):\n%s", (int)totalRead, buf);
        LE_TEST_OK(strstr(buf, ATSERVERUTIL_OK) != NULL, "AT command executed successfully: %s", cmds[j]);
    }

    pthread_exit(NULL);
}
