 /**
  * This module implements the le_port's tests.
  *
  * @note According to your platform, you may have to configure the mapping of the physical UART.
  * Please refer to https://source.sierrawireless.com/resources/legato/howtos/customizeuart/ for
  * full details.
  * On host, open a TTY terminal to connect to the device with the following configuration:
  * stty -F /dev/ttyUSB0 (ttyUSB0 is the serial device on the host from which the target board is
  * connected. User has to select the device according to the serial port hardware.)
  * Get the baud rate from this command and enter below command on the host:
  * minicom -D /dev/ttyUSB0 -b 9600
  *
  * Issue the following commands:
  * @verbatim
   $ app start portServiceIntegrationTest
   $ app runProc portServiceIntegrationTest --exe=portServiceTest
   @endverbatim
  *- Serial devices:
  * Provide the TTY name. (tty device of target)
  *
  * To run port service integration test, enter below commands on the the tty device of host which
  * has been opend by minicom command as above.
  * - AT+TESTCMDMODE: Test AT command mode.
  * - AT+TESTDATAMODE: Switch to data mode.
  * - +++: Exit from data mode.
  *
  * Copyright (C) Sierra Wireless Inc.
  */

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Byte length to read from fd.
 */
//--------------------------------------------------------------------------------------------------
#define READ_BYTES          100

//--------------------------------------------------------------------------------------------------
/**
 * Default buffer size for device information and error messages
 */
//--------------------------------------------------------------------------------------------------
#define DSIZE               256

//--------------------------------------------------------------------------------------------------
/**
 * String size for the buffer that contains a summary of all the device information available
 */
//--------------------------------------------------------------------------------------------------
#define DSIZE_INFO_STR      1600

//--------------------------------------------------------------------------------------------------
/**
 * Maximum length of monitor name
 */
//--------------------------------------------------------------------------------------------------
#define MAX_LEN_MONITORNAME 64


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
 * Thread and semaphore reference.
 */
//--------------------------------------------------------------------------------------------------
static le_sem_Ref_t Semaphore;
static le_thread_Ref_t AppThreadRef;

//--------------------------------------------------------------------------------------------------
/**
 * Device reference.
 */
//--------------------------------------------------------------------------------------------------
static le_port_DeviceRef_t devRef;

//--------------------------------------------------------------------------------------------------
/**
 * Prepare handler.
 */
//--------------------------------------------------------------------------------------------------
static void PrepareHandler
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

    LE_INFO("commandRef %p", commandRef);

    memset(atCommandName, 0, LE_ATDEFS_COMMAND_MAX_BYTES);

    // Get command name
    LE_ASSERT_OK(le_atServer_GetCommandName(commandRef, atCommandName,
                 LE_ATDEFS_COMMAND_MAX_BYTES));

    LE_INFO("AT command name %s", atCommandName);

    memset(rsp, 0, LE_ATDEFS_RESPONSE_MAX_BYTES);
    snprintf(rsp, LE_ATDEFS_RESPONSE_MAX_BYTES, "%s TYPE: ", atCommandName + 2);

    switch (type)
    {
        case LE_ATSERVER_TYPE_PARA:
            LE_INFO("Type PARA");
            snprintf(rsp + strlen(rsp), LE_ATDEFS_RESPONSE_MAX_BYTES-strlen(rsp), "PARA");
        break;

        case LE_ATSERVER_TYPE_TEST:
            LE_INFO("Type TEST");
            snprintf(rsp + strlen(rsp), LE_ATDEFS_RESPONSE_MAX_BYTES-strlen(rsp), "TEST");
        break;

        case LE_ATSERVER_TYPE_READ:
            LE_INFO("Type READ");
            snprintf(rsp + strlen(rsp), LE_ATDEFS_RESPONSE_MAX_BYTES-strlen(rsp), "READ");
        break;

        case LE_ATSERVER_TYPE_ACT:
            LE_INFO("Type ACT");
            snprintf(rsp + strlen(rsp), LE_ATDEFS_RESPONSE_MAX_BYTES-strlen(rsp), "ACT");
        break;

        default:
            LE_ERROR("AT command type is not proper!");
            exit(EXIT_FAILURE);
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
    PrepareHandler(commandRef, type, parametersNumber, contextPtr);

    // Send Final response
    LE_ASSERT_OK(le_atServer_SendFinalResponse(commandRef, LE_ATSERVER_OK, false, ""));

    le_sem_Post(Semaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Return a description string of err
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
 * This function is called when data are available to be read on fd
 */
//--------------------------------------------------------------------------------------------------
static void RxNewData
(
    int fd,      ///< [IN] File descriptor to read on
    short events ///< [IN] Event reported on fd
)
{
    char buffer[READ_BYTES];
    ssize_t count;

    if (events & (POLLIN | POLLPRI))
    {
        count = read(fd, buffer, READ_BYTES);
        if (-1 == count)
        {
            LE_ERROR("read error: %s", StrError(errno));
            return;
        }
        else if (count > 0)
        {
            if (0 == strcmp(buffer, "+++"))
            {
                LE_INFO("Data received: %s", buffer);
                le_sem_Post(Semaphore);
                return;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Data Mode handler
 */
//--------------------------------------------------------------------------------------------------
static void DataModeHandler
(
    le_atServer_CmdRef_t commandRef,
    le_atServer_Type_t   type,
    uint32_t             parametersNumber,
    void*                contextPtr
)
{
    int fd;
    char monitorName[MAX_LEN_MONITORNAME];

    le_port_ConnectService();

    PrepareHandler(commandRef, type, parametersNumber, contextPtr);

    // Send Final response
    LE_ASSERT_OK(le_atServer_SendFinalResponse(commandRef, LE_ATSERVER_OK, false, ""));

    if (LE_OK == le_port_SetDataMode(devRef, &fd))
    {
        LE_INFO("fd from port service is %d. le_port_SetDataMode() API success...", fd);
    }
    else
    {
        LE_ERROR("le_port_SetDataMode() API fails !");
        exit(EXIT_FAILURE);
    }

    // Create a File Descriptor Monitor object for the file descriptor.
    snprintf(monitorName, sizeof(monitorName), "Monitor-%d", fd);

    le_fdMonitor_Create(monitorName, fd, RxNewData, POLLIN | POLLPRI | POLLRDHUP);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add AtServer handler
 */
//--------------------------------------------------------------------------------------------------
static void* ATServerAddHandler
(
    void* contexPtr
)
{
    le_atServer_ConnectService();

    le_atServer_AddCommandHandler(AtCmdCreation.cmdRef,
                                  AtCmdCreation.handlerPtr,
                                  AtCmdCreation.contextPtr);
    le_event_RunLoop();
}

//--------------------------------------------------------------------------------------------------
/**
 * Main of the test. This test code has been written to test the port service stub code APIs.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    le_atServer_DeviceRef_t atServerDevRef;

    Semaphore = le_sem_Create("HandlerSem",0);

    AtCmdCreation.atCmdPtr = "AT+TESTCMDMODE";

    AtCmdCreation.cmdRef = le_atServer_Create(AtCmdCreation.atCmdPtr);
    LE_ASSERT(AtCmdCreation.cmdRef != NULL);

    AtCmdCreation.handlerPtr = AtCmdModeHandler;
    AtCmdCreation.contextPtr = NULL;

    AppThreadRef = le_thread_Create("ATServerHandler", ATServerAddHandler, NULL);

    le_thread_Start(AppThreadRef);

    devRef = le_port_Request("uart");
    if (NULL != devRef)
    {
        LE_INFO("le_port_Request() API success...");
    }
    else
    {
        LE_ERROR("Device reference is NULL ! le_port_Request() API fails !");
        exit(EXIT_FAILURE);
    }

    // Wait untill AT command not received on uart.
    le_sem_Wait(Semaphore);
    le_thread_Cancel(AppThreadRef);

    AtCmdCreation.atCmdPtr = "AT+TESTDATAMODE";

    AtCmdCreation.cmdRef = le_atServer_Create(AtCmdCreation.atCmdPtr);
    LE_ASSERT(AtCmdCreation.cmdRef != NULL);

    AtCmdCreation.handlerPtr = DataModeHandler;
    AtCmdCreation.contextPtr = NULL;

    AppThreadRef = le_thread_Create("ATServerHandler", ATServerAddHandler, NULL);

    le_thread_Start(AppThreadRef);

    // Wait untill data mode testing completes.
    le_sem_Wait(Semaphore);
    le_thread_Cancel(AppThreadRef);

    if (LE_OK == le_port_SetCommandMode(devRef, &atServerDevRef))
    {
        LE_INFO("le_port_SetCommandMode() API success...");
        LE_INFO("atServerDevRef is %p", atServerDevRef);
    }
    else
    {
        LE_ERROR("le_port_SetCommandMode() API fails !");
        exit(EXIT_FAILURE);
    }

    if (LE_OK == le_port_Release(devRef))
    {
        LE_INFO("le_port_Release() API success...");
    }
    else
    {
        LE_ERROR("le_port_Release() API fails !");
        exit(EXIT_FAILURE);
    }

    LE_INFO("======= Port service Integration Test completes =======");

    exit(EXIT_SUCCESS);
}
