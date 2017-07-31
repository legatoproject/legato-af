/**
 * sever.c implements the server part of the unit test
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */
#include "legato.h"
#include "interfaces.h"
#include "defs.h"
#include "strerror.h"

//--------------------------------------------------------------------------------------------------
/**
 * Maximum supported commands
 */
//--------------------------------------------------------------------------------------------------
#define COMMANDS_MAX    50

//--------------------------------------------------------------------------------------------------
/**
 * Maximum supported parameters
 */
//--------------------------------------------------------------------------------------------------
#define PARAM_MAX       24

//--------------------------------------------------------------------------------------------------
/**
 * Strtol base 10
 */
//--------------------------------------------------------------------------------------------------
#define BASE10  10

//--------------------------------------------------------------------------------------------------
/**
 * Extended error codes levels
 */
//--------------------------------------------------------------------------------------------------
#define DISABLE_EXTENDED_ERROR_CODES            0
#define ENABLE_NUMERIC_EXTENDED_ERROR_CODES     1
#define ENABLE_STRING_EXTENDED_ERROR_CODES      2

//--------------------------------------------------------------------------------------------------
/**
 * ServerData_t definition
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    int socketFd;
    int connFd;
}
ServerData_t;

//--------------------------------------------------------------------------------------------------
/**
 * AtCmd_t definition
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    const char*                      atCmdPtr;
    le_atServer_CmdRef_t             cmdRef;
    le_atServer_CommandHandlerFunc_t handlerPtr;
}
AtCmd_t;

//--------------------------------------------------------------------------------------------------
/**
 * AtSession_t definition
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_atServer_DeviceRef_t devRef;
    int                     fd;
    int                     cmdsCount;
    AtCmd_t                 atCmds[COMMANDS_MAX];
}
AtSession_t;

//--------------------------------------------------------------------------------------------------
/**
 * Server thread shared data
 */
//--------------------------------------------------------------------------------------------------
static ServerData_t ServerData;

//--------------------------------------------------------------------------------------------------
/**
 * AtSession shared data
 */
//--------------------------------------------------------------------------------------------------
static AtSession_t AtSession;

//--------------------------------------------------------------------------------------------------
/**
 * Extended error code value
 */
//--------------------------------------------------------------------------------------------------
static int ExtendedErrorCode = 0;

//--------------------------------------------------------------------------------------------------
/**
 * AT command handler
 *
 * tested APIs:
 *      le_atServer_GetCommandName
 *      le_atServer_SendIntermediateResponse
 *      le_atServer_GetParameter
 *      le_atServer_SendFinalResponse
 *
 */
//--------------------------------------------------------------------------------------------------
static void AtCmdHandler
(
    le_atServer_CmdRef_t commandRef,
    le_atServer_Type_t type,
    uint32_t parametersNumber,
    void* contextPtr
)
{
    char atCommandName[LE_ATDEFS_COMMAND_MAX_BYTES];
    char rsp[LE_ATDEFS_RESPONSE_MAX_BYTES];
    char param[LE_ATDEFS_PARAMETER_MAX_BYTES];
    int i = 0;

    LE_DEBUG("commandRef %p", commandRef);

    // check whether command's name is registred on the server app
    memset(atCommandName,0,LE_ATDEFS_COMMAND_MAX_BYTES);
    LE_ASSERT(le_atServer_GetCommandName(commandRef,
                                         atCommandName,
                                         LE_ATDEFS_COMMAND_MAX_BYTES) == LE_OK);

    LE_DEBUG("AT command name %s", atCommandName);

    memset(rsp, 0, LE_ATDEFS_RESPONSE_MAX_BYTES);
    snprintf(rsp, LE_ATDEFS_RESPONSE_MAX_BYTES, "%s TYPE: ", atCommandName+2);

    switch (type)
    {
        case LE_ATSERVER_TYPE_PARA:
            LE_DEBUG("Type PARA");
            snprintf(rsp+strlen(rsp), LE_ATDEFS_RESPONSE_MAX_BYTES-strlen(rsp), "PARA");
        break;

        case LE_ATSERVER_TYPE_TEST:
            LE_DEBUG("Type TEST");
            snprintf(rsp+strlen(rsp), LE_ATDEFS_RESPONSE_MAX_BYTES-strlen(rsp), "TEST");
        break;

        case LE_ATSERVER_TYPE_READ:
            LE_DEBUG("Type READ");
            snprintf(rsp+strlen(rsp), LE_ATDEFS_RESPONSE_MAX_BYTES-strlen(rsp), "READ");
        break;

        case LE_ATSERVER_TYPE_ACT:
            LE_DEBUG("Type ACT");
            snprintf(rsp+strlen(rsp), LE_ATDEFS_RESPONSE_MAX_BYTES-strlen(rsp), "ACT");
        break;

        default:
            LE_ASSERT(0);
        break;
    }

    // send an intermediate response with the command type
    LE_ASSERT(le_atServer_SendIntermediateResponse(commandRef, rsp) == LE_OK);

    // get the command parameters and send them in an intermediate response
    for (i = 0; i < parametersNumber && parametersNumber <= PARAM_MAX; i++)
    {
        memset(param,0,LE_ATDEFS_PARAMETER_MAX_BYTES);
        LE_ASSERT(le_atServer_GetParameter(commandRef,
                                        i,
                                        param,
                                        LE_ATDEFS_PARAMETER_MAX_BYTES)
                == LE_OK);
        LE_DEBUG("Param %d: %s", i, param);

        memset(rsp,0,LE_ATDEFS_RESPONSE_MAX_BYTES);
        snprintf(rsp,LE_ATDEFS_RESPONSE_MAX_BYTES, "%s PARAM %d: %s", atCommandName+2, i, param);

        LE_ASSERT(le_atServer_SendIntermediateResponse(commandRef, rsp) == LE_OK);
    }

    // test for bad parameter
    LE_ASSERT(le_atServer_GetParameter(commandRef,
                                       parametersNumber+1,
                                       param,
                                       LE_ATDEFS_PARAMETER_MAX_BYTES) == LE_BAD_PARAMETER);

    // send OK final response
    LE_ASSERT(le_atServer_SendFinalResponse(commandRef, LE_ATSERVER_OK, false, "") == LE_OK);
}

//--------------------------------------------------------------------------------------------------
/**
 * ATI command handler
 *
 * tests long responses and multiple intermediates send
 *
 * tested APIs:
 *      le_atServer_SendIntermediateResponse
 *      le_atServer_SendFinalResponse
 *
 */
//--------------------------------------------------------------------------------------------------
static void AtiCmdHandler
(
    le_atServer_CmdRef_t commandRef,
    le_atServer_Type_t type,
    uint32_t parametersNumber,
    void* contextPtr
)
{
    char rsp[LE_ATDEFS_RESPONSE_MAX_BYTES];

    switch (type)
    {
        // this command cannot be of types parameter, test or read
        case LE_ATSERVER_TYPE_PARA:
        case LE_ATSERVER_TYPE_TEST:
        case LE_ATSERVER_TYPE_READ:
            LE_ASSERT(
                // send and ERROR final response
                le_atServer_SendFinalResponse(commandRef, LE_ATSERVER_ERROR, false, "") == LE_OK);
            break;
        // this is an action type command so send multiple intermediate
        // responses and an OK final response
        case LE_ATSERVER_TYPE_ACT:
            memset(rsp, 0, LE_ATDEFS_RESPONSE_MAX_BYTES);
            sprintf(rsp, "%s",
                "Manufacturer: Sierra Wireless, Incorporated");
            LE_ASSERT(le_atServer_SendIntermediateResponse(commandRef, rsp) == LE_OK);

            memset(rsp, 0, LE_ATDEFS_RESPONSE_MAX_BYTES);
            sprintf(rsp, "%s", "Model: WP8548");
            LE_ASSERT(le_atServer_SendIntermediateResponse(commandRef, rsp) == LE_OK);

            memset(rsp, 0, LE_ATDEFS_RESPONSE_MAX_BYTES);
            sprintf(rsp, "%s",
                "Revision: SWI9X15Y_07.10.04.00 12c1700 jenkins"
                " 2016/06/02 02:52:45");
            LE_ASSERT(le_atServer_SendIntermediateResponse(commandRef, rsp) == LE_OK);

            memset(rsp, 0, LE_ATDEFS_RESPONSE_MAX_BYTES);
            sprintf(rsp, "%s", "IMEI: 359377060009700");
            LE_ASSERT(le_atServer_SendIntermediateResponse(commandRef, rsp) == LE_OK);

            memset(rsp, 0, LE_ATDEFS_RESPONSE_MAX_BYTES);
            sprintf(rsp, "%s", "IMEI SV: 42");
            LE_ASSERT(le_atServer_SendIntermediateResponse(commandRef, rsp) == LE_OK);

            memset(rsp, 0, LE_ATDEFS_RESPONSE_MAX_BYTES);
            sprintf(rsp, "%s", "FSN: LL542500111503");
            LE_ASSERT(le_atServer_SendIntermediateResponse(commandRef, rsp) == LE_OK);

            memset(rsp, 0, LE_ATDEFS_RESPONSE_MAX_BYTES);
            sprintf(rsp, "%s", "+GCAP: +CGSM");
            LE_ASSERT(le_atServer_SendIntermediateResponse(commandRef, rsp) == LE_OK);
            // send an OK final response
            LE_ASSERT(le_atServer_SendFinalResponse(commandRef,
                                                    LE_ATSERVER_OK,
                                                    false,
                                                    "") == LE_OK);
            break;

        default:
            LE_ASSERT(0);
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * AT+ECHO command handler (echo)
 *
 */
//--------------------------------------------------------------------------------------------------
static void AtEchoCmdHandler
(
    le_atServer_CmdRef_t commandRef,
    le_atServer_Type_t   type,
    uint32_t             parametersNumber,
    void*                contextPtr
)
{
    AtSession_t* atSessionPtr = (AtSession_t *)contextPtr;
    char         rsp[LE_ATDEFS_RESPONSE_MAX_BYTES];
    char         atCommandName[LE_ATDEFS_COMMAND_MAX_BYTES];
    char         param[LE_ATDEFS_PARAMETER_MAX_BYTES];
    le_result_t  res = LE_OK;

    LE_DEBUG("commandRef %p", commandRef);

    // check whether command's name is registred on the server app
    memset(atCommandName,0,LE_ATDEFS_COMMAND_MAX_BYTES);
    LE_ASSERT(le_atServer_GetCommandName(commandRef,
                                         atCommandName,
                                         LE_ATDEFS_COMMAND_MAX_BYTES) == LE_OK);

    LE_DEBUG("AT command name %s", atCommandName);

    memset(rsp, 0, LE_ATDEFS_RESPONSE_MAX_BYTES);
    snprintf(rsp, LE_ATDEFS_RESPONSE_MAX_BYTES, "%s TYPE: ", atCommandName+2);

    switch (type)
    {
        case LE_ATSERVER_TYPE_PARA:
            LE_INFO("Type PARA");
            snprintf(rsp+strlen(rsp), LE_ATDEFS_RESPONSE_MAX_BYTES-strlen(rsp), "PARA");
            memset(param,0,LE_ATDEFS_PARAMETER_MAX_BYTES);
            LE_ASSERT(le_atServer_GetParameter(commandRef,
                                               0,
                                               param,
                                               LE_ATDEFS_PARAMETER_MAX_BYTES) == LE_OK);
            if (0 == strncmp(param, "1", strlen("1")))
            {
                LE_ASSERT(le_atServer_EnableEcho((le_atServer_DeviceRef_t)0xdeadbeef)
                                                                            == LE_BAD_PARAMETER);
                LE_ASSERT(le_atServer_EnableEcho(atSessionPtr->devRef) == LE_OK);
            }
            else if (0 == strncmp(param, "0", strlen("0")))
            {
                LE_ASSERT(le_atServer_DisableEcho((le_atServer_DeviceRef_t)0xdeadbeef)
                                                                            == LE_BAD_PARAMETER);

                LE_ASSERT(le_atServer_DisableEcho(atSessionPtr->devRef) == LE_OK);
            }
            else
            {
                res = LE_FAULT;
                LE_ASSERT(le_atServer_SendFinalResponse(commandRef,
                                                        LE_ATSERVER_ERROR,
                                                        false,
                                                        "") == LE_OK);
            }
        break;

        case LE_ATSERVER_TYPE_TEST:
            LE_INFO("Type TEST");
            snprintf(rsp+strlen(rsp), LE_ATDEFS_RESPONSE_MAX_BYTES-strlen(rsp), "TEST");
        break;

        case LE_ATSERVER_TYPE_READ:
            LE_INFO("Type READ");
            snprintf(rsp+strlen(rsp), LE_ATDEFS_RESPONSE_MAX_BYTES-strlen(rsp), "READ");
        break;

        case LE_ATSERVER_TYPE_ACT:
            LE_INFO("Type ACT");
            snprintf(rsp+strlen(rsp), LE_ATDEFS_RESPONSE_MAX_BYTES-strlen(rsp), "ACT");
        break;

        default:
            LE_ASSERT(0);
        break;
    }

    if (LE_OK == res)
    {
        // Send the command type into an intermediate response
        LE_ASSERT(le_atServer_SendIntermediateResponse(commandRef, rsp) == LE_OK);

        // Send Final response
        LE_ASSERT(le_atServer_SendFinalResponse(commandRef, LE_ATSERVER_OK, false, "") == LE_OK);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * get command name refrence
 *
 * atServer API doesn't provide a way to get command's refrence directly
 * loop through the table, if it's a known command return its refrence
 *
 */
//--------------------------------------------------------------------------------------------------
static le_atServer_CmdRef_t GetRef
(
    AtCmd_t*    atCmdsPtr,
    int         cmdsCount,
    const char* cmdNamePtr
)
{
    int i = 0;

    for (i=0; i<cmdsCount; i++)
    {
        if ( ! (strcmp(atCmdsPtr[i].atCmdPtr, cmdNamePtr)) )
        {
            return atCmdsPtr[i].cmdRef;
        }
    }

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * AT+DEL command handler
 *
 * tests commands deletion
 *
 * tested APIs:
 *      le_atServer_GetParameter
 *      le_atServer_SendIntermediateResponse
 *      le_atServer_SendFinalResponse
 *      le_atServer_Delete
 *
 */
//--------------------------------------------------------------------------------------------------
static void DelCmdHandler
(
    le_atServer_CmdRef_t commandRef,
    le_atServer_Type_t type,
    uint32_t parametersNumber,
    void* contextPtr
)
{
    AtSession_t *atSessionPtr = (AtSession_t *)contextPtr;
    le_atServer_FinalRsp_t finalRsp = LE_ATSERVER_OK;
    le_atServer_CmdRef_t cmdRef;
    char param[LE_ATDEFS_PARAMETER_MAX_BYTES];
    int i = 0;

    switch (type)
    {
        case LE_ATSERVER_TYPE_PARA:
            for (i = 0; i < parametersNumber && parametersNumber <= PARAM_MAX; i++)
            {
                memset(param,0,LE_ATDEFS_PARAMETER_MAX_BYTES);
                // get the command to delete
                LE_ASSERT(
                le_atServer_GetParameter(commandRef,
                                        i,
                                        param,
                                        LE_ATDEFS_PARAMETER_MAX_BYTES)
                                    == LE_OK);
                // get its refrence
                cmdRef = GetRef(atSessionPtr->atCmds, atSessionPtr->cmdsCount, param);
                LE_DEBUG("Deleting %p => %s", cmdRef, param);
                // delete the command
                LE_ASSERT(le_atServer_Delete(cmdRef) == LE_OK);
            }
            // send an OK final response
            finalRsp = LE_ATSERVER_OK;
            break;
        // this command doesn't support test and read send an ERROR final
        // reponse
        case LE_ATSERVER_TYPE_TEST:
        case LE_ATSERVER_TYPE_READ:
            finalRsp = LE_ATSERVER_ERROR;
            break;
        // an action command type to verify that AT+DEL command does exist
        // send an OK final response
        case LE_ATSERVER_TYPE_ACT:
            finalRsp = LE_ATSERVER_OK;
            break;

        default:
            LE_ASSERT(0);
        break;
    }
    // send final response
    LE_ASSERT(le_atServer_SendFinalResponse(commandRef, finalRsp, false, "") == LE_OK);
}

//--------------------------------------------------------------------------------------------------
/**
 * CLOSE command handler
 *
 * tests closing server session
 *
 * tested APIs:
 *      le_atServer_SendFinalResponse
 *      le_atServer_Close
 */
//--------------------------------------------------------------------------------------------------
static void CloseCmdHandler
(
    le_atServer_CmdRef_t commandRef,
    le_atServer_Type_t type,
    uint32_t parametersNumber,
    void* contextPtr
)
{
    AtSession_t* atSessionPtr = (AtSession_t *)contextPtr;

    switch (type) {
    // this command doesn't accept parameter, test or read send an ERROR
    // final response
    case LE_ATSERVER_TYPE_PARA:
    case LE_ATSERVER_TYPE_TEST:
    case LE_ATSERVER_TYPE_READ:
        LE_ASSERT(le_atServer_SendFinalResponse(commandRef, LE_ATSERVER_ERROR, false, "") == LE_OK);

        break;
    case LE_ATSERVER_TYPE_ACT:
        LE_ASSERT(le_atServer_SendFinalResponse(commandRef, LE_ATSERVER_OK, false, "") == LE_OK);
        LE_ASSERT(le_atServer_Close(atSessionPtr->devRef) == LE_OK);
        break;

    default:
        LE_ASSERT(0);
        break;
    }
}

//------------------------------------------------------------------------------
/**
 * CBC command handler
 *
 * tests unsolicited responses
 *
 * tested APIs:
 *      le_atServer_SendIntermediateResponse
 *      le_atServer_SendFinalResponse
 *      le_atServer_SendUnsolicitedResponse, specific device and all devices
 *
 */
//------------------------------------------------------------------------------
static void CbcCmdHandler
(
    le_atServer_CmdRef_t commandRef,
    le_atServer_Type_t type,
    uint32_t parametersNumber,
    void* contextPtr
)
{
    AtSession_t* atSessionPtr = (AtSession_t *)contextPtr;
    le_atServer_FinalRsp_t finalRsp = LE_ATSERVER_OK;

    switch (type)
    {
        // this command doesn't support parameter type send an ERROR
        // final response
        case LE_ATSERVER_TYPE_PARA:
            finalRsp = LE_ATSERVER_ERROR;
            break;
        // tell the user/host how to read the command, send an OK final response
        case LE_ATSERVER_TYPE_TEST:
            LE_ASSERT(
                le_atServer_SendIntermediateResponse(commandRef,
                                                     "+CBC: (0-2),(1-100),(voltage)") == LE_OK);
            finalRsp = LE_ATSERVER_OK;
            break;
        // read isn't allowed
        case LE_ATSERVER_TYPE_READ:
            finalRsp = LE_ATSERVER_ERROR;
            break;
        // send an intermediate response containing the values
        // send unsolicited responses with updates
        case LE_ATSERVER_TYPE_ACT:
            LE_ASSERT(le_atServer_SendIntermediateResponse(commandRef,
                                                           "+CBC: 1,50,4190") == LE_OK);
            LE_ASSERT(le_atServer_SendUnsolicitedResponse("+CBC: 1,70,4190",
                                                          LE_ATSERVER_SPECIFIC_DEVICE,
                                                          atSessionPtr->devRef) == LE_OK);
            LE_ASSERT(le_atServer_SendUnsolicitedResponse("+CBC: 2,100,4190",
                                                          LE_ATSERVER_ALL_DEVICES, 0) == LE_OK);
            finalRsp = LE_ATSERVER_OK;
            break;

        default:
            LE_ASSERT(0);
            break;
    }
    // send an OK final response
    LE_ASSERT(le_atServer_SendFinalResponse(commandRef, finalRsp, false, "")
        == LE_OK);
}

//------------------------------------------------------------------------------
/**
 * Data command handler
 *
 * tests suspend/resume functions
 *
 * tested APIs:
 *      le_atServer_Suspend
 *      le_atServer_Resume
 *      le_atServer_SendIntermediateResponse
 *      le_atServer_SendFinalResponse
 *      le_atServer_SendUnsolicitedResponse, specific device
 *
 */
//------------------------------------------------------------------------------
static void DataCmdHandler
(
    le_atServer_CmdRef_t commandRef,
    le_atServer_Type_t type,
    uint32_t parametersNumber,
    void* contextPtr
)
{
    int i;
    AtSession_t* atSessionPtr = (AtSession_t *)contextPtr;

    switch (type)
    {
        // send an ERROR final response
        case LE_ATSERVER_TYPE_READ:
        case LE_ATSERVER_TYPE_PARA:
            LE_ASSERT(
                le_atServer_SendFinalResponse(commandRef, LE_ATSERVER_ERROR, false, "") == LE_OK);
            break;
        // send an OK final response
        case LE_ATSERVER_TYPE_TEST:
            LE_ASSERT(le_atServer_SendFinalResponse(commandRef,
                                                    LE_ATSERVER_OK,
                                                    false,
                                                    "") == LE_OK);
            break;
        case LE_ATSERVER_TYPE_ACT:
            LE_ASSERT(le_atServer_SendIntermediateResponse(commandRef, "CONNECT") == LE_OK);
            LE_ASSERT(le_atServer_Suspend(atSessionPtr->devRef) == LE_OK);

            for (i=0; i<3; i++)
            {
                LE_ASSERT(le_atServer_SendUnsolicitedResponse("CONNECTED",
                            LE_ATSERVER_SPECIFIC_DEVICE, atSessionPtr->devRef)
                    == LE_OK);
            }

            if (write(atSessionPtr->fd, "testing the data mode", 21) == -1)
            {
                LE_ERROR("write failed: %s", strerror(errno));
            }

            LE_ASSERT(le_atServer_Resume(atSessionPtr->devRef) == LE_OK);

            LE_ASSERT(le_atServer_SendFinalResponse(commandRef, LE_ATSERVER_OK, true, "NO CARRIER")
                == LE_OK);
            break;

        default:
            LE_ASSERT(0);
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Dial command handler
 *
 * tests parameter get fro an ATD command
 *
 */
//--------------------------------------------------------------------------------------------------
static void AtdCmdHandler
(
    le_atServer_CmdRef_t commandRef,
    le_atServer_Type_t type,
    uint32_t parametersNumber,
    void* contextPtr
)
{
    char param[LE_ATDEFS_PARAMETER_MAX_BYTES];

    LE_ASSERT(parametersNumber == 1);
    LE_ASSERT(type == LE_ATSERVER_TYPE_PARA);
    // get the phone number
    LE_ASSERT( le_atServer_GetParameter(commandRef,
                                        0,
                                        param,
                                        LE_ATDEFS_PARAMETER_MAX_BYTES) == LE_OK);

    // echo the command in intermediate rsp
    LE_ASSERT(le_atServer_SendIntermediateResponse(commandRef, param) == LE_OK);

    LE_ASSERT(le_atServer_SendFinalResponse(commandRef, LE_ATSERVER_OK, false, "") == LE_OK);
}

//--------------------------------------------------------------------------------------------------
/**
 * Cmee command handler
 *
 * tests:
 *  - le_atServer_EnableExtendedErrorCodes()
 *  - le_atServer_DisableExtendedErrorCodes()
 *
 */
//--------------------------------------------------------------------------------------------------
static void CmeeCmdHandler
(
    le_atServer_CmdRef_t commandRef,
    le_atServer_Type_t type,
    uint32_t parametersNumber,
    void* contextPtr
)
{
    le_atServer_FinalRsp_t finalRsp = LE_ATSERVER_OK;
    char param[LE_ATDEFS_PARAMETER_MAX_BYTES] = {0};
    char rsp[LE_ATDEFS_RESPONSE_MAX_BYTES] = {0};

    switch (type)
    {
        case LE_ATSERVER_TYPE_PARA:
            LE_ASSERT_OK(le_atServer_GetParameter(commandRef,
                                                  0,
                                                  param,
                                                  LE_ATDEFS_PARAMETER_MAX_BYTES));
            ExtendedErrorCode = (int)strtol(param, NULL, BASE10);
            switch (ExtendedErrorCode)
            {
                case DISABLE_EXTENDED_ERROR_CODES:
                    le_atServer_DisableExtendedErrorCodes();
                    break;
                case ENABLE_NUMERIC_EXTENDED_ERROR_CODES:
                case ENABLE_STRING_EXTENDED_ERROR_CODES:
                    le_atServer_EnableExtendedErrorCodes();
                    break;
                default:
                    finalRsp = LE_ATSERVER_ERROR;
                    break;
            }
            break;
        case LE_ATSERVER_TYPE_READ:
            snprintf(rsp, LE_ATDEFS_RESPONSE_MAX_BYTES, "+CMEE: %d", ExtendedErrorCode);
            LE_ASSERT_OK(le_atServer_SendIntermediateResponse(commandRef, rsp));
            break;
        case LE_ATSERVER_TYPE_TEST:
            LE_ASSERT_OK(le_atServer_SendIntermediateResponse(commandRef, "+CMEE: (0-2)"));
            break;
        case LE_ATSERVER_TYPE_ACT:
            finalRsp = LE_ATSERVER_ERROR;
            break;

        default:
            LE_ASSERT(0);
            break;
    }

    LE_ASSERT_OK(le_atServer_SendFinalResponse(commandRef, finalRsp, false, ""));
}

//--------------------------------------------------------------------------------------------------
/**
 * Cleanup thread function
 *
 */
//--------------------------------------------------------------------------------------------------
static void CleanUp
(
    void *contextPtr
)
{
    ServerData_t *serverDataPtr = (ServerData_t *)contextPtr;

    if (close(serverDataPtr->connFd) == -1)
    {
        LE_ERROR("close failed %s", strerror(errno));
    }

    if (close(serverDataPtr->socketFd) == -1)
    {
        LE_ERROR("close failed %s", strerror(errno));
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * server thread function
 *
 * the main function of the thread
 * start the server
 * initialize/create new commands and register them within the server app
 *
 * tested APIs:
 *      le_atServer_Open
 *      le_atServer_Create
 *      le_atServer_AddCommandHandler
 *
 */
//--------------------------------------------------------------------------------------------------
void AtServer
(
    SharedData_t* sharedDataPtr
)
{
    struct sockaddr_un addr;
    int i = 0;

    AtCmd_t atCmdCreation[] =
    {
        {
            .atCmdPtr = "AT+DATA",
            .cmdRef = NULL,
            .handlerPtr = DataCmdHandler,
        },
        {
            .atCmdPtr = "ATI",
            .cmdRef = NULL,
            .handlerPtr = AtiCmdHandler,
        },
        {
            .atCmdPtr = "AT+CBC",
            .cmdRef = NULL,
            .handlerPtr = CbcCmdHandler,
        },
        {
            .atCmdPtr = "AT+DEL",
            .cmdRef = NULL,
            .handlerPtr = DelCmdHandler,
        },
        {
            .atCmdPtr = "AT+CLOSE",
            .cmdRef = NULL,
            .handlerPtr = CloseCmdHandler,
        },
        {
            .atCmdPtr = "AT",
            .cmdRef = NULL,
            .handlerPtr = AtCmdHandler,
        },
        {
            .atCmdPtr = "AT+ABCD",
            .cmdRef = NULL,
            .handlerPtr = AtCmdHandler,
        },
        {
            .atCmdPtr = "ATA",
            .cmdRef = NULL,
            .handlerPtr = AtCmdHandler,
        },
        {
            .atCmdPtr = "AT&F",
            .cmdRef = NULL,
            .handlerPtr = AtCmdHandler,
        },
        {
            .atCmdPtr = "ATS",
            .cmdRef = NULL,
            .handlerPtr = AtCmdHandler,
        },
        {
            .atCmdPtr = "ATV",
            .cmdRef = NULL,
            .handlerPtr = AtCmdHandler,
        },
        {
            .atCmdPtr = "AT&C",
            .cmdRef = NULL,
            .handlerPtr = AtCmdHandler,
        },
        {
            .atCmdPtr = "AT&D",
            .cmdRef = NULL,
            .handlerPtr = AtCmdHandler,
        },
        {
            .atCmdPtr = "ATE",
            .cmdRef = NULL,
            .handlerPtr = AtCmdHandler,
        },
        {
            .atCmdPtr = "AT+ECHO",
            .cmdRef = NULL,
            .handlerPtr = AtEchoCmdHandler,
        },
        {
            .atCmdPtr = "ATD",
            .cmdRef = NULL,
            .handlerPtr = AtdCmdHandler,
        },
        {
            .atCmdPtr = "AT+CMEE",
            .cmdRef = NULL,
            .handlerPtr = CmeeCmdHandler,
        },
    };

    LE_INFO("Server Started");

    le_thread_AddDestructor(CleanUp,(void *)&ServerData);

    ServerData.socketFd = socket(AF_UNIX, SOCK_STREAM, 0);
    LE_ASSERT(ServerData.socketFd != -1);

    memset(&addr, 0, sizeof(addr));
    addr.sun_family= AF_UNIX;
    strncpy(addr.sun_path, sharedDataPtr->devPathPtr, sizeof(addr.sun_path)-1);

    LE_ASSERT(bind(ServerData.socketFd, (struct sockaddr*) &addr, sizeof(addr)) != -1);

    LE_ASSERT(listen(ServerData.socketFd, 1) != -1);

    le_sem_Post(sharedDataPtr->semRef);

    ServerData.connFd = accept(ServerData.socketFd, NULL, NULL);
    LE_ASSERT(ServerData.connFd != -1);

    // test for bad file descriptor
    AtSession.devRef = le_atServer_Open(-1);
    LE_ASSERT(AtSession.devRef == NULL);

    // save a copy of fd and duplicate it before Opening the server after a call to
    // le_atServer_Open the file descriptor will be closed
    AtSession.fd = ServerData.connFd;

    // start the server
    AtSession.devRef = le_atServer_Open(dup(ServerData.connFd));
    LE_ASSERT(AtSession.devRef != NULL);
    sharedDataPtr->devRef = AtSession.devRef;

    // AT commands handling tests
    AtSession.cmdsCount = NUM_ARRAY_MEMBERS(atCmdCreation);

    // AT commands subscriptions
    while (i < AtSession.cmdsCount)
    {
        atCmdCreation[i].cmdRef = le_atServer_Create(atCmdCreation[i].atCmdPtr);
        LE_ASSERT(atCmdCreation[i].cmdRef != NULL);

        AtSession.atCmds[i] = atCmdCreation[i];

        LE_ASSERT(le_atServer_AddCommandHandler(atCmdCreation[i].cmdRef,
                                                atCmdCreation[i].handlerPtr,
                                                (void *)&AtSession) != NULL);

        i++;
    }

    le_sem_Post(sharedDataPtr->semRef);
}
