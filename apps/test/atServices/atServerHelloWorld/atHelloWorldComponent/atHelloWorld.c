/**
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"

#include "atServerIF.h"

#include "cmsis_os2.h"


//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of parameters
 */
//--------------------------------------------------------------------------------------------------
#ifdef MK_CONFIG_PARAMETER_LIST_MAX
#   define PARAM_MAX            MK_CONFIG_PARAMETER_LIST_MAX
#else
#   define PARAM_MAX            10
#endif

#define TIME_TO_WAIT_SEC        3

//--------------------------------------------------------------------------------------------------
/**
 * AtCmd_t definition
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    const char*                      cmdPtr;
    le_atServer_CmdRef_t             cmdRef;
    le_atServer_CommandHandlerFunc_t handlerPtr;
    void*                            contextPtr;
}
AtCmd_t;

// Length of escape sequence +++
#define ESCAPE_SEQUENCE_LENGTH_MAX                  32

// Invalid data mode FD value
#define LE_FILE_STREAM_INVALID_FD                   -1


// fdMonitor reference for reading data from POLLIN event on data mode fd
static le_fdMonitor_Ref_t DataModeFdMonitorRef = NULL;

// Data mode FD
static int DataModeFD = LE_FILE_STREAM_INVALID_FD;

le_atServer_CmdRef_t CommandRef = NULL;

// Data structure for use in creating an fdMonitor
typedef struct FDMonitorInfo
{
    const char                  *name;
    int                          fd;
    le_fdMonitor_HandlerFunc_t   handlerFunc;
    short                        events;
    le_fdMonitor_Ref_t           ref;
} FDMonitorInfo_t;

// Forward declaration
static void DataModeEventHandlerMonitor(int fd, short events);


////////////////////////////////////////////////////////////////////////////////////////////////////
// The following code is from ATIP, these are helper to switch to data mode and command with old
// le_atServer.api API
////////////////////////////////////////////////////////////////////////////////////////////////////
#define ATSERVERUTIL_CONNECT "\r\nCONNECT\r\n"
#define ATSERVERUTIL_NOCARRIER "NO CARRIER"
#define LE_ATSERVER_CME_ERROR "+CME ERROR: "

typedef enum
{
    ATSERVERUTIL_OK,
    ATSERVERUTIL_NO_CARRIER,
    ATSERVERUTIL_ERROR
}
atServerUtil_FinalRsp_t;
//--------------------------------------------------------------------------------------------------
/**
 * This function is copied from ATIP, which is a helper function to switch to data mode
 *
 * @return
 *      - LE_OK            Function succeeded.
 *      - LE_FAULT         Function failed.
 *      - LE_BAD_PARAMETER Invalid parameter.
 *      - LE_UNAVAILABLE   JSON parsing is not completed.
 *      - LE_DUPLICATE     Device already opened in data mode
 */
//--------------------------------------------------------------------------------------------------
static le_result_t atServerUtil_SwitchToDataMode
(
    le_atServer_CmdRef_t commandRef,        ///< [IN] AT command reference
    int *atSockFd                           ///< [OUT] fd to use for read/write in data mode
)
{
#ifndef LE_CONFIG_RTOS
    #define le_fd_Write write
#endif

    le_atServer_DeviceRef_t atServerDevRef = NULL;
    le_port_DeviceRef_t devRefPtr = NULL;
    le_result_t result;

    if(commandRef == NULL)
    {
        LE_ERROR("commandRef null");
        result = LE_BAD_PARAMETER;
        goto error;

    }

    // Get the device reference
    result = le_atServer_GetDevice(commandRef, &atServerDevRef);
    if (LE_OK != result)
    {
        LE_ERROR("Cannot get device information! Result: %d", result);
        goto error;
    }
    // Get the port reference
    result = le_port_GetPortReference(atServerDevRef, &devRefPtr);
    if (LE_OK != result)
    {
        LE_ERROR("Cannot get port reference! Result: %d", result);
        goto error;
    }

    /* With Altair AT parser, it is impossible to send intermediate response CONNECT
     * without sending final result code. So CONNECT is sent after switching to data mode
     * by directly writing to data fd
     */

    // Suspend AT commands monitoring
    result = le_port_SetDataMode(devRefPtr, atSockFd);
    if(LE_OK != result)
    {
        LE_ERROR("le_port_SetDataMode API usage error");
        goto error;
    }
    LE_DEBUG("At socket FD by le_port_SetDataMode: %d", (int)(*atSockFd));
    // TODO: add DCD management depending on AT&C configuration

    if(-1 == le_fd_Write(*atSockFd,ATSERVERUTIL_CONNECT,strlen(ATSERVERUTIL_CONNECT)))
    {
        LE_ERROR("CONNECT write error");
        result = LE_FAULT;
    }
error:
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is copied from ATIP, which is a helper function to switch to command mode
 *
 * @return
 *      - LE_OK            Function succeeded.
 *      - LE_FAULT         Function failed.
 *      - LE_BAD_PARAMETER Invalid parameter.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t atServerUtil_SwitchToCommandMode
(
    le_atServer_CmdRef_t commandRef,
            ///< [IN] AT command reference
    atServerUtil_FinalRsp_t final,
            ///< [IN] final result to send after switch back to AT command mode
    int32_t errorCode,
            ///< [IN] final result to send numeric error code
    int atSockFd
            ///< [IN] fd to close the data port
)
{
    le_atServer_DeviceRef_t atServerDevRef = NULL;
    le_port_DeviceRef_t devRefPtr = NULL;
    le_result_t result;

    le_atServer_FinalRsp_t finalRsp = (le_atServer_FinalRsp_t)LE_OK;
    char *rspPattern = "";

    if(commandRef == NULL)
    {
        LE_ERROR("commandRef null");
        result = LE_BAD_PARAMETER;
        goto error;
    }

    // Close data port
    le_fd_Close(atSockFd);

    // Get the device reference
    result = le_atServer_GetDevice(commandRef, &atServerDevRef);
    if (LE_OK != result)
    {
        LE_ERROR("Cannot get device information! Result: %d", result);
        goto error;
    }
    // Get the port reference
    result = le_port_GetPortReference(atServerDevRef, &devRefPtr);
    if (LE_OK != result)
    {
        LE_ERROR("Cannot get port reference! Result: %d", result);
        goto error;
    }

    // TODO: add DCD management depending on AT&C configuration

    // Resume AT commands monitoring
    result = le_port_SetCommandMode(devRefPtr, &atServerDevRef);
    if(LE_OK != result)
    {
        LE_ERROR("le_port_SetDataMode API usage error");
        goto error;
    }

    switch(final)
    {
        case ATSERVERUTIL_OK:
            finalRsp = LE_ATSERVER_OK;
            break;

        case ATSERVERUTIL_NO_CARRIER:
            finalRsp = LE_ATSERVER_NO_CARRIER;
            rspPattern = ATSERVERUTIL_NOCARRIER;
            break;

        case ATSERVERUTIL_ERROR:
        default:
            finalRsp = LE_ATSERVER_ERROR;
            break;
    }
    if(errorCode >= 0)
    {
        finalRsp = LE_ATSERVER_ERROR;
        rspPattern = LE_ATSERVER_CME_ERROR;
    }
    else
    {
        errorCode = 0;
    }
    result = le_atServer_SendFinalResultCode(commandRef, finalRsp, rspPattern, errorCode);
    if(LE_OK!=result)
    {
        LE_ERROR("Failed to send final result code. Return value:%d",result);
    }
error:
    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Monitor data mode fd for incoming events (inputs)
 *
 * @return none
 */
//--------------------------------------------------------------------------------------------------
static void DataModeEventHandlerMonitor
(
    int fd,                         ///< [IN] Data mode fd
    short events                    ///< [IN] Events
)
{
    static char buf[ESCAPE_SEQUENCE_LENGTH_MAX] = { 0 };
    static int ind = 0;
    int readCount = 0;

//    LE_INFO("received EVENT: %d", events);

    if (events & POLLIN)
    {
        readCount = le_fd_Read(fd, &buf[ind], 1);

        if (readCount <= 0)
        {
            LE_ERROR("Cannot read data from data mode!\r\n");
        }
        else
        {

//            LE_INFO("Received %d bytes: '%c'\r\n", readCount, buf[ind]);

            // echo the character
            le_fd_Write(fd, &buf[ind], readCount);
        }
    }

    if (events & POLLHUP)
    {
        le_atProxy_SwitchToCommandMode(CommandRef, LE_ATSERVER_OK, -1, DataModeFD);
        // Also can call the following function, the old way instead
        //atServerUtil_SwitchToCommandMode(CommandRef, (int32_t)LE_ATSERVER_OK, -1, DataModeFD);

        if (DataModeFdMonitorRef)
        {
            le_fdMonitor_Delete(DataModeFdMonitorRef);
            DataModeFdMonitorRef = NULL;
        }
    }
}


void HelloDataCmdHandler
(
    le_atServer_CmdRef_t atSessionPtr,
    le_atServer_Type_t type,
    uint32_t parametersNumber,
    void* contextPtr
)
{
    LE_UNUSED(type);
    LE_UNUSED(parametersNumber);
    LE_UNUSED(contextPtr);
    LE_UNUSED(atServerUtil_SwitchToCommandMode);
    LE_UNUSED(atServerUtil_SwitchToDataMode);

    le_result_t res;

    res = le_atProxy_SwitchToDataMode(atSessionPtr, &DataModeFD);
    // Also can call the following function, the old way instead
    //res = atServerUtil_SwitchToDataMode(atSessionPtr, &DataModeFD);
    if (LE_OK != res)
    {
        LE_ERROR("Failed to switch to data mode");
        return;
    }

    // Create fdMonitor in Legato thread by queuing it to the this atProxy client thread
    DataModeFdMonitorRef = le_fdMonitor_Create("DataModeFDMon",
                                               DataModeFD,
                                               DataModeEventHandlerMonitor,
                                               POLLIN | POLLHUP);

    CommandRef = atSessionPtr;

#define NUM_OUTPUT          3
#define MAX_RESP_BYTES      30
#define STR_TEST "This is in data mode[%d/%d]\r\n"

    char resp[MAX_RESP_BYTES];

    for (int i = 0; i < NUM_OUTPUT; i++)
    {
        snprintf(resp, MAX_RESP_BYTES, STR_TEST, i + 1, NUM_OUTPUT);
        le_fd_Write(DataModeFD, resp, strlen(resp));
        snprintf(resp, MAX_RESP_BYTES, "This is urc %d", i);
        le_atServer_SendUnsolicitedResponse(resp, LE_ATSERVER_SPECIFIC_DEVICE, atSessionPtr);

        le_thread_Sleep(1);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Generic command handler which outputs the type of AT command. This can be mapped to any white
 * listed AT command in the modem. Example:
 * AT+HELLOWORLD?         --> READ
 * AT+HELLOWORLD=?        --> TEST
 * AT+HELLOWORLD=1,2,3,4  --> PARA
 * AT+HELLOWORLD          --> ACTION
 */
//--------------------------------------------------------------------------------------------------
void HelloWorldCmdHandler
(
    le_atServer_CmdRef_t commandRef,
    le_atServer_Type_t type,
    uint32_t parametersNumber,
    void* contextPtr
)
{
    LE_UNUSED(contextPtr);

    char rsp[LE_ATDEFS_RESPONSE_MAX_BYTES];
    char atCommandName[LE_ATDEFS_COMMAND_MAX_BYTES];
    char param[LE_ATDEFS_PARAMETER_MAX_BYTES];
    size_t i = 0;

    memset(atCommandName, 0, LE_ATDEFS_COMMAND_MAX_BYTES);

    /* Get command name */
    LE_ASSERT(le_atServer_GetCommandName(commandRef,
                                         (char*)atCommandName,
                                         LE_ATDEFS_COMMAND_MAX_BYTES) == LE_OK);

    LE_INFO("AT command name %s", atCommandName);

    memset(rsp, 0, LE_ATDEFS_RESPONSE_MAX_BYTES);
    snprintf(rsp, LE_ATDEFS_RESPONSE_MAX_BYTES, "%s AT COMMAND TYPE: ", atCommandName+2);

    switch (type)
    {
        case LE_ATSERVER_TYPE_PARA:
            le_utf8_Append(rsp, "PARA", LE_ATDEFS_RESPONSE_MAX_BYTES, NULL);
        break;

        case LE_ATSERVER_TYPE_TEST:
            le_utf8_Append(rsp, "TEST", LE_ATDEFS_RESPONSE_MAX_BYTES, NULL);
        break;

        case LE_ATSERVER_TYPE_READ:
            le_utf8_Append(rsp, "READ", LE_ATDEFS_RESPONSE_MAX_BYTES, NULL);
        break;

        case LE_ATSERVER_TYPE_ACT:
            le_utf8_Append(rsp, "ACT", LE_ATDEFS_RESPONSE_MAX_BYTES, NULL);
        break;

        default:
            LE_ASSERT(0);
        break;
    }

    /* Send the command type into an intermediate response */
    LE_ASSERT(le_atServer_SendIntermediateResponse(commandRef, rsp) == LE_OK);

    /* Send parameters into an intermediate response */
    for (i = 0; i < parametersNumber && parametersNumber <= PARAM_MAX; i++)
    {
        LE_ASSERT(le_atServer_GetParameter(commandRef,
                                           i,
                                           (char*)param,
                                           LE_ATDEFS_PARAMETER_MAX_BYTES) == LE_OK);

        snprintf(rsp,LE_ATDEFS_RESPONSE_MAX_BYTES,
            "%s PARAM %d: %s", atCommandName+2, i, param);

        LE_ASSERT(le_atServer_SendIntermediateResponse(commandRef, rsp) == LE_OK);

        LE_INFO("param %d \"%s\"", i, param);
    }

    LE_INFO("Sleep for a few seconds to observe the intermediate reponse before final response");
    le_thread_Sleep(TIME_TO_WAIT_SEC);

    /* Send Final response */
    LE_ASSERT(le_atServer_SendFinalResultCode(commandRef, LE_ATSERVER_OK, "", 0)== LE_OK);
    LE_INFO("Command handler completed");
}


//--------------------------------------------------------------------------------------------------
/**
 * AT Commands definition
 *
 */
//--------------------------------------------------------------------------------------------------
static AtCmd_t AtCmdCreation[] =
{
    {
        .cmdPtr = "AT+HELLOWORLD",
        .cmdRef = NULL,
        .handlerPtr = HelloWorldCmdHandler,
        .contextPtr = NULL
    },
    {
        .cmdPtr = "AT+HELLODATA",
        .cmdRef = NULL,
        .handlerPtr = HelloDataCmdHandler,
        .contextPtr = NULL
    },
};

static le_result_t InstallCmdHandler
(
    AtCmd_t* atCmdPtr               ///< [IN] AT command structure list
)
{
    atCmdPtr->cmdRef = le_atServer_Create(atCmdPtr->cmdPtr);
    if (NULL == atCmdPtr->cmdRef)
    {
        LE_ERROR("Cannot create a command: atCmdPtr->cmdRef is NULL!");
        return LE_FAULT;
    }
    le_atServer_AddCommandHandler(atCmdPtr->cmdRef, atCmdPtr->handlerPtr, atCmdPtr->contextPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Main of the test
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("============== AT command initialization starts =================");

    size_t i = 0;
    le_result_t result = LE_OK;

    while (i < NUM_ARRAY_MEMBERS(AtCmdCreation))
    {
        result = InstallCmdHandler(&AtCmdCreation[i]);
        if(LE_OK != result)
        {
            LE_ERROR("Handler subscription failed. Return value: %d", result);
            return;
        }
        i++;
    }
}
