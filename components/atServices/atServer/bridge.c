/** @file bridge.c
 *
 * Implementation of the AT commands server <-> AT commands client bridge.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "interfaces.h"
#include "bridge.h"
#include "le_atServer_local.h"

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Bridge pool size
 */
//--------------------------------------------------------------------------------------------------
#define BRIDGE_POOL_SIZE    1

//--------------------------------------------------------------------------------------------------
/**
 * AT Command client timeout for sending the command
 * The timeout is long as some AT commands are long to be executed
 */
//--------------------------------------------------------------------------------------------------
#define AT_CLIENT_TIMEOUT 5*60*1000

//--------------------------------------------------------------------------------------------------
/**
 * Responses codes definition
 */
//--------------------------------------------------------------------------------------------------
#define ERROR_CODE  \
    "ERROR",        \
    "+CME ERROR",   \
    "+CMS ERROR",   \

#define SUCCESS_CODE \
    "OK",           \
    "NO CARRIER",   \
    "CONNECT",      \
    "NO CARRIER",   \
    "NO DIALTONE",  \
    "BUSY",         \
    "NO ANSWER",    \


//--------------------------------------------------------------------------------------------------
// Data structures.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 *  Bridge context structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_thread_Ref_t                             threadRef;          ///< AT client thread reference
    le_thread_Ref_t                             mainThreadRef;      ///< main thread reference
    le_atServer_BridgeRef_t                     bridgeRef;          ///< Bridge reference
    le_dls_List_t                               devicesList;        ///< list of devices bridged
                                                                    ///< with the current bridge
    le_atClient_DeviceRef_t                     atClientRef;        ///< AT client device reference
    le_atClient_UnsolicitedResponseHandlerRef_t unsolHandlerRef;    ///< AT cleint unsolicited
                                                                    ///< handler refenrece
    le_sem_Ref_t                                semRef;             ///< semaphore reference
    le_msg_SessionRef_t                         sessionRef;         ///< session reference
}
BridgeCtx_t;

//--------------------------------------------------------------------------------------------------
/**
 *  Modem AT command structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_atServer_CommandHandlerRef_t  commandHandlerRef;             /// AT server command handler
    le_atServer_CmdRef_t             atServerCmdRef;                ///< AT server command reference
    le_atClient_CmdRef_t             atClientCmdRef;                ///< AT client command reference
    char                             cmd[LE_ATDEFS_COMMAND_MAX_BYTES];  ///< cmd to be sent to AT
                                                                    ///< client
    void*                            refPtr;                        ///< self reference
}
ModemCmdDesc_t;

//--------------------------------------------------------------------------------------------------
/**
 *  Device link structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_atServer_DeviceRef_t deviceRef; ///< device reference of the current link
    le_dls_Link_t           link;      ///< link for list
}
DeviceLink_t;

//--------------------------------------------------------------------------------------------------
//                                       Static declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Map for bridges
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t  BridgesRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Pool for bridges context
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t    BridgesPool;

//--------------------------------------------------------------------------------------------------
/**
 * Pool for devices link
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t    DeviceLinkPool;

//--------------------------------------------------------------------------------------------------
/**
 * Pool for modem AT commands description
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t    ModemCmdPool;

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for modem AT commands description
 *
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t ModemCmdRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Bridge mutex
 *
 */
//--------------------------------------------------------------------------------------------------
static le_mutex_Ref_t BridgeMutexRef;

//--------------------------------------------------------------------------------------------------
/**
 * Success responses strings array
 */
//--------------------------------------------------------------------------------------------------
static char const* const SuccessRspCode[] =
{
     SUCCESS_CODE
};

//--------------------------------------------------------------------------------------------------
/**
 * All responses strings array
 */
//--------------------------------------------------------------------------------------------------
static char const* const AllRspCode[] =
{
     SUCCESS_CODE
     ERROR_CODE
};

//--------------------------------------------------------------------------------------------------
/**
 *  AT command Error string.
 */
//--------------------------------------------------------------------------------------------------
const char ErrorString[] = "ERROR";

//--------------------------------------------------------------------------------------------------
/**
 * Final response string to send to the AT command client
 */
//--------------------------------------------------------------------------------------------------
char AtClientFinalResponse[LE_ATDEFS_RESPONSE_MAX_BYTES];

//--------------------------------------------------------------------------------------------------
/**
 * This function is the destructor for ModemCmdDesc_t struct
 *
 */
//--------------------------------------------------------------------------------------------------
static void ModemCmdPoolDestructor
(
    void *ptr
)
{
    ModemCmdDesc_t* modemCmdDescPtr = ptr;

    if (NULL == modemCmdDescPtr)
    {
        LE_ERROR("Bad parameter");
        return;
    }

    // Clean AT server contexts
    if (modemCmdDescPtr->atServerCmdRef)
    {
        if (LE_OK != le_atServer_Delete(modemCmdDescPtr->atServerCmdRef))
        {
            LE_ERROR("Error in le_atServer_Delete");
        }
    }

    // Clean AT client contexts
    if (modemCmdDescPtr->atClientCmdRef)
    {
        if (LE_OK != le_atClient_Delete(modemCmdDescPtr->atClientCmdRef))
        {
            LE_ERROR("Error in le_atClient_Delete");
        }
    }

    // Clean self reference
    le_ref_DeleteRef(ModemCmdRefMap, modemCmdDescPtr->refPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is the destructor for BridgeCtx_t struct
 *
 */
//--------------------------------------------------------------------------------------------------
static void BridgePoolDestructor
(
    void *ptr
)
{
    BridgeCtx_t* bridgePtr = ptr;

    // Remove the trhread
    if (bridgePtr->threadRef)
    {
        le_thread_Cancel(bridgePtr->threadRef);
        le_thread_Join(bridgePtr->threadRef, NULL);
    }

    // Remove bridge reference
    if (bridgePtr->bridgeRef)
    {
        le_ref_DeleteRef(BridgesRefMap, bridgePtr->bridgeRef);
    }

    // Remove AT client unsolicited handler
    if (bridgePtr->unsolHandlerRef)
    {
        le_atClient_RemoveUnsolicitedResponseHandler(bridgePtr->unsolHandlerRef);
    }

    // Close AT commands client
    if (bridgePtr->atClientRef)
    {
        le_atClient_Stop(bridgePtr->atClientRef);
    }

    if (bridgePtr->semRef)
    {
        le_sem_Delete(bridgePtr->semRef);
    }

    // Release devices list
    le_dls_Link_t* linkPtr = le_dls_Pop(&bridgePtr->devicesList);

    while (NULL != linkPtr)
    {
        DeviceLink_t* devLinkPtr = CONTAINER_OF(linkPtr,
                                                DeviceLink_t,
                                                link);

        le_mem_Release(devLinkPtr);

        linkPtr = le_dls_Pop(&bridgePtr->devicesList);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Bridge Thread Destructor
 * This function is called in bridge thread when the thread gets cancelled.
 */
//--------------------------------------------------------------------------------------------------
static void BridgeThreadDestructor
(
    void* paramPtr
 )
{
    // Set thread destructor to disconnect the service to avoid memory leak
    // when the thread gets cancelled.
    le_atClient_DisconnectService();
}

//--------------------------------------------------------------------------------------------------
/**
 * Treat error
 * This function is called in the main thread
 *
 */
//--------------------------------------------------------------------------------------------------
static void TreatCommandError
(
    void* param1Ptr,
    void* param2Ptr
)
{
    void* modemCmdDescRef = param1Ptr;

    ModemCmdDesc_t* modemCmdDescPtr = le_ref_Lookup(ModemCmdRefMap, modemCmdDescRef);
    if (NULL == modemCmdDescPtr)
    {
        LE_ERROR("modem command is not found");
        return;
    }

    // Send error to the host
    if (modemCmdDescPtr->atServerCmdRef)
    {
        le_result_t res = le_atServer_SendFinalResponse(modemCmdDescPtr->atServerCmdRef,
                                                        LE_ATSERVER_ERROR,
                                                        false,
                                                        "");

        if (LE_OK != res)
        {
            LE_ERROR("Error to send final response, %d", res);
        }
    }

    // Release the command has it occurs an error
    le_mem_Release(modemCmdDescPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Treat response of the AT command (coming from modem)
 * This function is called in the main thread (mandatory as this function calls le_atServer_xxx
 * functions).
 *
 */
//--------------------------------------------------------------------------------------------------
static void TreatResponse
(
    void* param1Ptr,
    void* param2Ptr
)
{
    void* modemCmdDescRef = param1Ptr;

    ModemCmdDesc_t* modemCmdDescPtr = le_ref_Lookup(ModemCmdRefMap, modemCmdDescRef);
    if (NULL == modemCmdDescPtr)
    {
        LE_ERROR("modem command is not found");
        return;
    }

    le_atServer_CmdRef_t atServerCmdRef = modemCmdDescPtr->atServerCmdRef;
    le_atClient_CmdRef_t atClientCmdRef = modemCmdDescPtr->atClientCmdRef;
    char rsp[LE_ATDEFS_RESPONSE_MAX_BYTES];

    // Get all intermediate responses from the AT client, and send back to the host through the AT
    // server.
    if (LE_OK == le_atClient_GetFirstIntermediateResponse(atClientCmdRef,
                                                          rsp,
                                                          LE_ATDEFS_RESPONSE_MAX_BYTES))
    {
        do
        {
            if (LE_OK != le_atServer_SendIntermediateResponse(atServerCmdRef, rsp))
            {
                LE_ERROR("Failed to send intermediate response");
                TreatCommandError(modemCmdDescRef, NULL);
                return;
            }
            memset(rsp, 0, LE_ATDEFS_RESPONSE_MAX_BYTES);
        }
        while (LE_OK == le_atClient_GetNextIntermediateResponse(atClientCmdRef,
                                                                rsp,
                                                                LE_ATDEFS_RESPONSE_MAX_BYTES));
    }

    // Get the final response from the AT client, and send back to the host through the AT
    // server.
    if (LE_OK == le_atClient_GetFinalResponse(atClientCmdRef,
                                              rsp,
                                              LE_ATDEFS_RESPONSE_MAX_BYTES))
    {
        int i;
        le_atServer_FinalRsp_t finalRsp = LE_ATSERVER_ERROR;

        // check if the response code is an error
        for (i=0; i < NUM_ARRAY_MEMBERS(SuccessRspCode); i++)
        {
            if (0 == strncmp(SuccessRspCode[i], rsp, strlen(rsp)))
            {
                finalRsp = LE_ATSERVER_OK;
                break;
            }
        }

        // Need free the atClientCmdRef before processing next concatenated AT bridge command
        // otherwise atClientCmdRef of next command will be cleared and cause atServer crash.
        if (LE_OK != le_atClient_Delete(modemCmdDescPtr->atClientCmdRef))
        {
            LE_ERROR("Error in deleting atClient reference");
        }
        else
        {
            modemCmdDescPtr->atClientCmdRef = 0;
        }

        LE_DEBUG("finalRsp = %s", (finalRsp == LE_ATSERVER_OK) ? "ok": "error");

        if (LE_OK != le_atServer_SendFinalResponse(atServerCmdRef,
                                                   finalRsp,
                                                   true,
                                                   rsp))
        {
            LE_ERROR("Failed to send final response");
            TreatCommandError(modemCmdDescRef, NULL);
            return;
        }

        // "ERROR" final response could mean that the AT command doesn't exist => delete it in this
        // case
        if (0 == strncmp(rsp, ErrorString, sizeof(ErrorString)))
        {
            LE_DEBUG("Remove AT command");
            le_mem_Release(modemCmdDescPtr);
        }
    }
    else
    {
        LE_ERROR("Failed to get final response");
        TreatCommandError(modemCmdDescRef, NULL);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Send the AT command to the modem through the AT client
 * This function is called in a separate thread: le_atClient_SetCommandAndSend is synchronous, and
 * can be locked many seconds (>30s for some of them).
 *
 */
//--------------------------------------------------------------------------------------------------
static void SendAtCommand
(
    void* param1Ptr,
    void* param2Ptr
)
{
    void* modemCmdDescRef = param1Ptr;
    BridgeCtx_t* bridgeRef = param2Ptr;

    le_mutex_Lock(BridgeMutexRef);

    ModemCmdDesc_t* modemCmdDescPtr = le_ref_Lookup(ModemCmdRefMap, modemCmdDescRef);
    BridgeCtx_t* bridgePtr = le_ref_Lookup(BridgesRefMap, bridgeRef);
    if (( NULL == modemCmdDescPtr ) || ( NULL == bridgePtr ))
    {
        LE_ERROR("bridge resources are not found");
        le_mutex_Unlock(BridgeMutexRef);
        return;
    }

    LE_DEBUG("AT command to be sent to the modem: %s", modemCmdDescPtr->cmd);

    // At this point, modemCmdDescPtr and bridgePtr are available , make a local copy for
    // some parameters passed to le_atClient_SetCommandAndSend().
    le_atClient_CmdRef_t atClientCmdRef = NULL;
    le_atClient_DeviceRef_t atClientDevicetRef = bridgePtr->atClientRef;
    le_thread_Ref_t mainThreadRef = bridgePtr->mainThreadRef;
    char   atClientCmd[LE_ATDEFS_COMMAND_MAX_BYTES] = {0};
    snprintf(atClientCmd, LE_ATDEFS_COMMAND_MAX_BYTES, "%s", modemCmdDescPtr->cmd);
    le_mutex_Unlock(BridgeMutexRef);

    // Send AT command to the modem
    le_result_t result = le_atClient_SetCommandAndSend(&atClientCmdRef,
                                                       atClientDevicetRef,
                                                       atClientCmd,
                                                       "",
                                                       AtClientFinalResponse,
                                                       AT_CLIENT_TIMEOUT);

    // Since le_atClient_SetCommandAndSend() is a blocking API which is executed in
    // bridge thread, it's possible the brige device is already released in main thread
    // after the API is returned, thus we need check again if bridge pointers are
    // still available before using them.
    le_mutex_Lock(BridgeMutexRef);

    modemCmdDescPtr = le_ref_Lookup(ModemCmdRefMap, modemCmdDescRef);
    bridgePtr = le_ref_Lookup(BridgesRefMap, bridgeRef);
    if (( NULL == modemCmdDescPtr ) || ( NULL == bridgePtr ))
    {
        LE_ERROR("bridge resources are not found");
        le_mutex_Unlock(BridgeMutexRef);
        return;
    }

    if (result != LE_OK)
    {
        LE_ERROR("Error in sending AT command");
        // Treat the error in the main thread
        le_event_QueueFunctionToThread(mainThreadRef,
                                       TreatCommandError,
                                       modemCmdDescRef,
                                       NULL);

        le_mutex_Unlock(BridgeMutexRef);
        return;
    }

    // Copy back atClientCmdRef which will be used in response.
    modemCmdDescPtr->atClientCmdRef = atClientCmdRef;

    // Treat the send of responses in the main thread.
    le_event_QueueFunctionToThread(mainThreadRef,
                                   TreatResponse,
                                   modemCmdDescRef,
                                   NULL);

    le_mutex_Unlock(BridgeMutexRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Build the final AT command to the modem.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t BuildAtCommand
(
    ModemCmdDesc_t* CmdDescPtr,
    le_atServer_Type_t type,
    uint32_t parametersNumber
)
{
    if ( CmdDescPtr == NULL )
    {
        LE_ERROR("CmdDescPtr is NULL");
        return LE_BAD_PARAMETER;
    }

    char* fullCmdPtr = CmdDescPtr->cmd;
    char cmdName[LE_ATDEFS_COMMAND_MAX_BYTES] = "";
    char para0[LE_ATDEFS_PARAMETER_MAX_BYTES];
    char para1[LE_ATDEFS_PARAMETER_MAX_BYTES];
    bool isBasicCmd = false;

    memset(fullCmdPtr, 0, LE_ATDEFS_COMMAND_MAX_BYTES);
    memset(para0, 0, LE_ATDEFS_PARAMETER_MAX_BYTES);
    memset(para1, 0, LE_ATDEFS_PARAMETER_MAX_BYTES);

    if (LE_OK != le_atServer_GetCommandName(CmdDescPtr->atServerCmdRef, cmdName,
                                            sizeof(cmdName)))
    {
        LE_ERROR("Impossible to get the command name");
        return LE_FAULT;
    }

    // Check if the command handler is for a basic command.
    if (IS_BASIC(cmdName[2]))
    {
        isBasicCmd = true;
    }

    // Get parameter 0.
    if (parametersNumber > 0)
    {
        if (LE_OK != le_atServer_GetParameter(CmdDescPtr->atServerCmdRef,
                                              0,
                                              para0,
                                              LE_ATDEFS_PARAMETER_MAX_BYTES))
        {
            LE_ERROR("Error in get parameter 0");
            return LE_FAULT;
        }
    }

    // Get parameter 1.
    if (parametersNumber > 1)
    {
        if (LE_OK != le_atServer_GetParameter(CmdDescPtr->atServerCmdRef,
                                              1,
                                              para1,
                                              LE_ATDEFS_PARAMETER_MAX_BYTES))
        {
            LE_ERROR("Error in get parameter 1");
            return LE_FAULT;
        }
    }

    LE_DEBUG("AT command: %s, nb param = %d, type = %d", cmdName, parametersNumber, type);

    le_result_t res = LE_OK;
    switch(type)
    {
        // type 0
        case LE_ATSERVER_TYPE_ACT:
            res = le_utf8_Copy(fullCmdPtr, cmdName, LE_ATDEFS_COMMAND_MAX_BYTES, NULL);
            if (res != LE_OK)
            {
                return res;
            }
        break;

        // type 1
        case LE_ATSERVER_TYPE_PARA:
            if (isBasicCmd)
            {
                // For AT basic command, we need consider two scenarios:
                // 1:  "AT<command>[<number>]", here the <number> is in parameter 0.
                // 2:  "ATS<number>=<value>", here the <number> is in parameter 0,
                // <value> is in parameter 1 .
                res = le_utf8_Copy(fullCmdPtr, cmdName, LE_ATDEFS_COMMAND_MAX_BYTES, NULL);
                if (res != LE_OK)
                {
                    return res;
                }
                if (parametersNumber > 0)
                {
                    res = le_utf8_Append(fullCmdPtr, para0, LE_ATDEFS_COMMAND_MAX_BYTES, NULL);
                    if (res != LE_OK)
                    {
                        return res;
                    }
                }
                if (parametersNumber > 1)
                {
                    res = le_utf8_Append(fullCmdPtr, "=", LE_ATDEFS_COMMAND_MAX_BYTES, NULL);
                    if (res != LE_OK)
                    {
                        return res;
                    }
                    res = le_utf8_Append(fullCmdPtr, para1, LE_ATDEFS_COMMAND_MAX_BYTES, NULL);
                    if (res != LE_OK)
                    {
                        return res;
                    }
                }
            }
            else
            {
                // For AT extended command  AT+<name>=<value1>[,<value2>[,<value3>[...]]] ,
                // <value1> is in parameter 0, <value2> is in parameter 1, <value3> is in
                // parameter 2 ...
                res = le_utf8_Copy(fullCmdPtr, cmdName, LE_ATDEFS_COMMAND_MAX_BYTES, NULL);
                if (res != LE_OK)
                {
                    return res;
                }
                res = le_utf8_Append(fullCmdPtr, "=", LE_ATDEFS_COMMAND_MAX_BYTES, NULL);
                if (res != LE_OK)
                {
                    return res;
                }

                uint32_t i = 0;
                char para[LE_ATDEFS_PARAMETER_MAX_BYTES];
                for (i = 0; i < parametersNumber; i++)
                {
                    memset(para, 0, LE_ATDEFS_PARAMETER_MAX_BYTES);
                    if (LE_OK != le_atServer_GetParameter(CmdDescPtr->atServerCmdRef,
                                                          i,
                                                          para,
                                                          LE_ATDEFS_PARAMETER_MAX_BYTES))
                    {
                        LE_ERROR("Error in get parameter %d", i);
                        return LE_FAULT;
                    }

                    res = le_utf8_Append(fullCmdPtr, para, LE_ATDEFS_COMMAND_MAX_BYTES, NULL);
                    if (res != LE_OK)
                    {
                        return res;
                    }
                    if (i < (parametersNumber-1))
                    {
                        res = le_utf8_Append(fullCmdPtr, ",", LE_ATDEFS_COMMAND_MAX_BYTES, NULL);
                        if (res != LE_OK)
                        {
                            return res;
                        }
                    }
                }
            }
        break;

        // type 2
        case LE_ATSERVER_TYPE_TEST:
            res = le_utf8_Copy(fullCmdPtr, cmdName, LE_ATDEFS_COMMAND_MAX_BYTES, NULL);
            if (res != LE_OK)
            {
                return res;
            }
            res = le_utf8_Append(fullCmdPtr, "=?", LE_ATDEFS_COMMAND_MAX_BYTES, NULL);
            if (res != LE_OK)
            {
                return res;
            }
        break;

        // type 3
        case LE_ATSERVER_TYPE_READ:
            if (parametersNumber > 0)
            {
                if (isBasicCmd)
                {
                    // For AT basic command format ATS<parameter_number>?
                    // The <parameter_number> is in parameter 0.
                    res = le_utf8_Copy(fullCmdPtr, cmdName, LE_ATDEFS_COMMAND_MAX_BYTES, NULL);
                    if (res != LE_OK)
                    {
                        return res;
                    }
                    res = le_utf8_Append(fullCmdPtr, para0, LE_ATDEFS_COMMAND_MAX_BYTES, NULL);
                    if (res != LE_OK)
                    {
                        return res;
                    }
                    res = le_utf8_Append(fullCmdPtr, "?", LE_ATDEFS_COMMAND_MAX_BYTES, NULL);
                    if (res != LE_OK)
                    {
                        return res;
                    }
                }
                else
                {
                    // For AT extended command format AT+<name>?[<value>]
                    // If exists, the <value> is in parameter 0.
                    res = le_utf8_Copy(fullCmdPtr, cmdName, LE_ATDEFS_COMMAND_MAX_BYTES, NULL);
                    if (res != LE_OK)
                    {
                        return res;
                    }
                    res = le_utf8_Append(fullCmdPtr, "?", LE_ATDEFS_COMMAND_MAX_BYTES, NULL);
                    if (res != LE_OK)
                    {
                        return res;
                    }
                    res = le_utf8_Append(fullCmdPtr, para0, LE_ATDEFS_COMMAND_MAX_BYTES, NULL);
                    if (res != LE_OK)
                    {
                        return res;
                    }
                }
            }
            else
            {
                res = le_utf8_Copy(fullCmdPtr, cmdName, LE_ATDEFS_COMMAND_MAX_BYTES, NULL);
                if (res != LE_OK)
                {
                    return res;
                }
                res = le_utf8_Append(fullCmdPtr, "?", LE_ATDEFS_COMMAND_MAX_BYTES, NULL);
                if (res != LE_OK)
                {
                    return res;
                }
            }
        break;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * AT command handler (call when an modem AT command is detected)
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
    void* modemCmdDescRef = contextPtr;

    ModemCmdDesc_t* modemCmdDescPtr = le_ref_Lookup(ModemCmdRefMap, modemCmdDescRef);
    if (NULL == modemCmdDescPtr)
    {
        LE_ERROR("Bad context");
        le_result_t res = le_atServer_SendFinalResponse(commandRef,
                                                        LE_ATSERVER_ERROR,
                                                        false,
                                                        "");

        if (LE_OK != res)
        {
            LE_ERROR("Error to send final response, %d", res);
        }

        return;
    }

    le_atServer_BridgeRef_t bridgeRef = NULL;
    modemCmdDescPtr->atServerCmdRef = commandRef;

    // Get the bridge reference of the on going at command
    if (LE_OK != le_atServer_GetBridgeRef(commandRef, &bridgeRef))
    {
        LE_ERROR("Impossible to get the bridge reference");
        TreatCommandError(modemCmdDescRef, NULL);
        return;
    }

    BridgeCtx_t* bridgePtr = le_ref_Lookup(BridgesRefMap, bridgeRef);

    if (NULL == bridgePtr)
    {
        LE_ERROR("No bridge device is found");
        TreatCommandError(modemCmdDescRef, NULL);
        return;
    }

    // Build the final bridge command to modem.
    if (LE_OK != BuildAtCommand(modemCmdDescPtr, type, parametersNumber))
    {
        LE_ERROR("Error in building AT bridge command");
        TreatCommandError(modemCmdDescRef, NULL);
        return;
    }

    // Treat the AT commands in the Bridge thread in order to not lock the main thread
    le_event_QueueFunctionToThread(bridgePtr->threadRef,
                                   SendAtCommand,
                                   modemCmdDescRef,
                                   bridgeRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * AT client unsolicited handler
 * All unsolicited coming from the AT client are sent to the hosts
 *
 */
//--------------------------------------------------------------------------------------------------
static void UnsolicitedResponseHandler
(
    const char* unsolicitedRsp,
    void* contextPtr
)
{
    BridgeCtx_t* bridgeCtxPtr = contextPtr;

    if (NULL == bridgeCtxPtr)
    {
        LE_ERROR("Bad parameter");
        return;
    }

    le_dls_Link_t* linkPtr = le_dls_Peek(&bridgeCtxPtr->devicesList);


    while (NULL != linkPtr)
    {
        DeviceLink_t* devLinkPtr = CONTAINER_OF(linkPtr,
                                   DeviceLink_t,
                                   link);

        if (LE_OK != le_atServer_SendUnsolicitedResponse(unsolicitedRsp,
                                                         LE_ATSERVER_SPECIFIC_DEVICE,
                                                         devLinkPtr->deviceRef))
        {
            LE_ERROR("Error during sending unsol on %p", devLinkPtr->deviceRef);
        }

        linkPtr = le_dls_PeekNext(&bridgeCtxPtr->devicesList,linkPtr);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Thread used for the bridge
 * This thread is used to send the AT command to the modem (called function is synchronous, and
 * may take a lot of time).
 */
//--------------------------------------------------------------------------------------------------
static void *BridgeThread
(
    void* context
)
{
    le_atClient_ConnectService();

    le_thread_AddDestructor(BridgeThreadDestructor, (void *)NULL);

    BridgeCtx_t* bridgeCtxPtr = context;

    le_sem_Post(bridgeCtxPtr->semRef);

    le_event_RunLoop();
    return NULL;
}

//--------------------------------------------------------------------------------------------------
//                                       Public declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Create a modem AT command and return the reference of the command description pointer
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to create the command.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t bridge_Create
(
    char*  atCmdPtr,
    void** descRefPtr
)
{
    if ((NULL == atCmdPtr) || (NULL == descRefPtr))
    {
        LE_ERROR("Bad parameter");
        return LE_FAULT;
    }

    LE_DEBUG("Create bridge for %s", atCmdPtr);

    ModemCmdDesc_t* modemCmdDescPtr = le_mem_ForceAlloc(ModemCmdPool);
    memset(modemCmdDescPtr, 0, sizeof(ModemCmdDesc_t));

    modemCmdDescPtr->refPtr = le_ref_CreateRef(ModemCmdRefMap, modemCmdDescPtr);

     // Add the AT command in the parser
    modemCmdDescPtr->atServerCmdRef = le_atServer_Create(atCmdPtr);

    if (NULL == modemCmdDescPtr->atServerCmdRef)
    {
        LE_ERROR("Error in AT command creation");
        le_mem_Release(modemCmdDescPtr);
        return LE_FAULT;
    }
    else
    {
        // Subsribe the handler to treat the created AT command
        if (NULL == (modemCmdDescPtr->commandHandlerRef = le_atServer_AddCommandHandler(
                                            modemCmdDescPtr->atServerCmdRef,
                                            AtCmdHandler,
                                            modemCmdDescPtr->refPtr)))
        {
            LE_ERROR("Impossible to add the handler");
            le_mem_Release(modemCmdDescPtr);
            return LE_FAULT;
        }
    }

    *descRefPtr = modemCmdDescPtr->refPtr;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Bridge initialization
 *
 *
 */
//--------------------------------------------------------------------------------------------------
void bridge_Init
(
    void
)
{
    // Bridge context pool allocation
    BridgesPool = le_mem_CreatePool("BridgeContextPool", sizeof(BridgeCtx_t));
    le_mem_ExpandPool(BridgesPool, BRIDGE_POOL_SIZE);
    BridgesRefMap = le_ref_CreateMap("BridgesRefMap", BRIDGE_POOL_SIZE);
    le_mem_SetDestructor(BridgesPool, BridgePoolDestructor);

    // Device link pool
    DeviceLinkPool = le_mem_CreatePool("BridgeDevicePool", sizeof(DeviceLink_t));
    le_mem_ExpandPool(DeviceLinkPool, DEVICE_POOL_SIZE);

    // modem AT commands pool
    ModemCmdPool =  le_mem_CreatePool("BridgeModemCmdPool", sizeof(ModemCmdDesc_t));
    le_mem_ExpandPool(ModemCmdPool, CMD_POOL_SIZE);
    ModemCmdRefMap = le_ref_CreateMap("BridgeModemCmdRefMap", CMD_POOL_SIZE);
    le_mem_SetDestructor(ModemCmdPool, ModemCmdPoolDestructor);

    // Create bridge mutex
    BridgeMutexRef =  le_mutex_CreateRecursive("BridgeMutex");

    // Build string for AT client final response
    int i = 0;
    bool firstLoop = true;
    memset(AtClientFinalResponse, 0, LE_ATDEFS_RESPONSE_MAX_BYTES);
    for (i=0; i < NUM_ARRAY_MEMBERS(AllRspCode); i++)
    {
        if (firstLoop)
        {
            snprintf(AtClientFinalResponse,
                     LE_ATDEFS_RESPONSE_MAX_BYTES,
                     "%s", AllRspCode[i]);
            firstLoop = false;
        }
        else
        {
            snprintf(AtClientFinalResponse+strlen(AtClientFinalResponse),
                     LE_ATDEFS_RESPONSE_MAX_BYTES-strlen(AtClientFinalResponse),
                     "|%s", AllRspCode[i]);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function opens a bridge with the modem.
 *
 * @return
 *      - Reference to the requested bridge.
 *      - NULL if the device is not available.
 */
//--------------------------------------------------------------------------------------------------
le_atServer_BridgeRef_t bridge_Open
(
    int fd
)
{
    char name[THREAD_NAME_MAX_LENGTH];
    static uint32_t threadNumber = 0;

    snprintf(name, THREAD_NAME_MAX_LENGTH, "BridgeThread-%d", threadNumber);

    BridgeCtx_t* bridgeCtxPtr = le_mem_ForceAlloc(BridgesPool);
    memset(bridgeCtxPtr, 0, sizeof(BridgeCtx_t));


    bridgeCtxPtr->bridgeRef = le_ref_CreateRef(BridgesRefMap, bridgeCtxPtr);
    bridgeCtxPtr->devicesList = LE_DLS_LIST_INIT;

    bridgeCtxPtr->threadRef = le_thread_Create(name, BridgeThread, bridgeCtxPtr);

    if (!bridgeCtxPtr->threadRef)
    {
        LE_ERROR("Error in thread creation");
        return NULL;
    }

    le_thread_SetJoinable(bridgeCtxPtr->threadRef);

    memset(name, 0, THREAD_NAME_MAX_LENGTH);
    snprintf(name, THREAD_NAME_MAX_LENGTH, "BridgeSem-%d", threadNumber);

    bridgeCtxPtr->semRef = le_sem_Create(name, 0);

    le_thread_Start(bridgeCtxPtr->threadRef);

    le_clk_Time_t timeToWait={30,0};
    if (le_sem_WaitWithTimeOut(bridgeCtxPtr->semRef, timeToWait) != LE_OK)
    {
        LE_ERROR("Semaphore error");
        le_mem_Release(bridgeCtxPtr);
        return NULL;
    }

    bridgeCtxPtr->mainThreadRef = le_thread_GetCurrent();

    // Create the bridge with the AT client
    // fd now belongs to AT command client
    bridgeCtxPtr->atClientRef = le_atClient_Start(fd);

    if (NULL == bridgeCtxPtr->atClientRef)
    {
        LE_ERROR("ATClient error");
        le_mem_Release(bridgeCtxPtr);
        return NULL;
    }

    // Subscribe to all unsolicited responses
    bridgeCtxPtr->unsolHandlerRef = le_atClient_AddUnsolicitedResponseHandler(
                                                                        "",
                                                                        bridgeCtxPtr->atClientRef,
                                                                        UnsolicitedResponseHandler,
                                                                        bridgeCtxPtr,
                                                                        1);

    threadNumber++;
    bridgeCtxPtr->sessionRef = le_atServer_GetClientSessionRef();

    return bridgeCtxPtr->bridgeRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function closes an open bridge.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to close the bridge.
 *      - LE_BUSY          The bridge is in use.
 */
//--------------------------------------------------------------------------------------------------
le_result_t bridge_Close
(
    le_atServer_BridgeRef_t bridgeRef
)
{
    BridgeCtx_t* bridgePtr = le_ref_Lookup(BridgesRefMap, bridgeRef);

    if (NULL == bridgePtr)
    {
        return LE_FAULT;
    }

    if (le_dls_NumLinks(&bridgePtr->devicesList) != 0)
    {
        return LE_BUSY;
    }

    le_mem_Release(bridgePtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function adds a device to the bridge.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_BUSY          The device is already used by the bridge.
 *      - LE_FAULT         The function failed to create the command.
 */
//--------------------------------------------------------------------------------------------------
le_result_t bridge_AddDevice
(
    le_atServer_DeviceRef_t deviceRef,
    le_atServer_BridgeRef_t bridgeRef
)
{
    BridgeCtx_t* bridgePtr = le_ref_Lookup(BridgesRefMap, bridgeRef);

    if (NULL == bridgePtr)
    {
        return LE_FAULT;
    }

    le_dls_Link_t* linkPtr = le_dls_Peek(&bridgePtr->devicesList);

    while (NULL != linkPtr)
    {
        DeviceLink_t* devLinkPtr = CONTAINER_OF(linkPtr,
                                                DeviceLink_t,
                                                link);

        if (devLinkPtr->deviceRef == deviceRef)
        {
            LE_ERROR("Error, device already bridged %p", deviceRef);
            return LE_BUSY;
        }

        linkPtr = le_dls_PeekNext(&bridgePtr->devicesList,linkPtr);
    }

    DeviceLink_t* deviceLinkPtr = le_mem_ForceAlloc(DeviceLinkPool);
    memset(deviceLinkPtr, 0, sizeof(DeviceLink_t));

    deviceLinkPtr->deviceRef = deviceRef;
    le_dls_Queue(&bridgePtr->devicesList, &(deviceLinkPtr->link));

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function removes a device from the bridge.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to create the command.
 *      - LE_NOT_FOUND     The requested device is not found.
 */
//--------------------------------------------------------------------------------------------------
le_result_t bridge_RemoveDevice
(
    le_atServer_DeviceRef_t deviceRef,
    le_atServer_BridgeRef_t bridgeRef
)
{
    BridgeCtx_t* bridgePtr = le_ref_Lookup(BridgesRefMap, bridgeRef);

    if (NULL == bridgePtr)
    {
        return LE_FAULT;
    }

    le_dls_Link_t* linkPtr = le_dls_Peek(&bridgePtr->devicesList);

    while (NULL != linkPtr)
    {
        DeviceLink_t* devLinkPtr = CONTAINER_OF(linkPtr,
                                                DeviceLink_t,
                                                link);

        if (devLinkPtr->deviceRef == deviceRef)
        {
            le_dls_Remove(&bridgePtr->devicesList, linkPtr);
            le_mem_Release(devLinkPtr);
            return LE_OK;
        }

        linkPtr = le_dls_PeekNext(&bridgePtr->devicesList, linkPtr);
    }

    return LE_NOT_FOUND;
}

//--------------------------------------------------------------------------------------------------
/**
 * Clean the bridge context when the close session service handler is invoked
 *
 */
//--------------------------------------------------------------------------------------------------
void bridge_CleanContext
(
    le_msg_SessionRef_t sessionRef
)
{
    le_ref_IterRef_t iter;

    iter = le_ref_GetIterator(BridgesRefMap);

    while (LE_OK == le_ref_NextNode(iter))
    {
        BridgeCtx_t* bridgePtr = (BridgeCtx_t*) le_ref_GetValue(iter);

        if (!bridgePtr)
        {
            return;
        }

        if (sessionRef == bridgePtr->sessionRef)
        {
            // Release devices list
            le_dls_Link_t* linkPtr = le_dls_Pop(&bridgePtr->devicesList);

            while (NULL != linkPtr)
            {
                DeviceLink_t* devLinkPtr = CONTAINER_OF(linkPtr,
                                                        DeviceLink_t,
                                                        link);

                if (LE_OK != le_atServer_UnlinkDeviceFromBridge(devLinkPtr->deviceRef,
                                                                bridgePtr->bridgeRef))
                {
                    LE_ERROR("Unable to unlink deviceRef %p from bridgeRef %p",
                             devLinkPtr->deviceRef,
                             bridgePtr->bridgeRef);
                }

                le_mem_Release(devLinkPtr);

                linkPtr = le_dls_Pop(&bridgePtr->devicesList);
            }

            LE_DEBUG("deleting bridgeRef %p", bridgePtr->bridgeRef);

            le_mutex_Lock(BridgeMutexRef);
            le_mem_Release(bridgePtr);
            le_mutex_Unlock(BridgeMutexRef);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the session reference of the bridge device
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t bridge_GetSessionRef
(
    le_atServer_BridgeRef_t  bridgeRef,
    le_msg_SessionRef_t* sessionRefPtr
)
{
    if (NULL == sessionRefPtr)
    {
        LE_ERROR("Bad parameter");
        return LE_FAULT;
    }

    BridgeCtx_t* bridgePtr = le_ref_Lookup(BridgesRefMap, bridgeRef);

    if (NULL == bridgePtr)
    {
        LE_ERROR("No bridge device is found");
        return LE_FAULT;
    }

    *sessionRefPtr = bridgePtr->sessionRef;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Release bridge command description for a give command reference
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t bridge_ReleaseModemCmd
(
    void* descRefPtr
)
{
    ModemCmdDesc_t* cmdDescPtr = le_ref_Lookup(ModemCmdRefMap, descRefPtr);
    if ( NULL == cmdDescPtr)
    {
        LE_ERROR("No cmdDescPtr is found");
        return LE_FAULT;
    }

    le_mutex_Lock(BridgeMutexRef);
    le_mem_Release(cmdDescPtr);
    le_mutex_Unlock(BridgeMutexRef);
    return LE_OK;
}
