/** @file bridgeTest.c
 *
 * Unit tests for le_atServer bridge APIs.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"
#include "defs.h"

//--------------------------------------------------------------------------------------------------
/**
 * AT command description structure
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    const char* commandNamePtr;
    const char* intermediateRspPtr[5];
    const char* finalRspPtr;
    int         readIndex;
}
AtCommandDesc_t;

//--------------------------------------------------------------------------------------------------
/**
 * AT commands strings
 *
 */
//--------------------------------------------------------------------------------------------------
static const char AtPlusCgdcontPara[] = "AT+CGDCONT=1,\"I,P\",\"orange\"";
static const char AtPlusCpinRead[] = "AT+CPIN?";
static const char AtPlusCgdcontRead[] = "AT+CGDCONT?";
static const char AtPlusCgdcontTest[] = "AT+CGDCONT=?";
static const char AtQ1[] = "ATQ1";
static const char AtPlusBad[] = "AT+BAD";
static const char AtPlusUnknown[] = "AT+UNKNOWN";
static const char ConcatCpinReadCgdcontRead[]="AT+CPIN?;+CGDCONT?";
static const char ConcatQ1CpinReadAbcdReadCgdcontTest[]="ATQ1;+CPIN?;+ABCD?;+CGDCONT=?";
static const char OkRsp[] = "OK";
static const char ErrorRsp[] = "ERROR";

//--------------------------------------------------------------------------------------------------
/**
 * AT commands description list
 *
 */
//--------------------------------------------------------------------------------------------------
static AtCommandDesc_t AtCommandList[] = {
    {
        .commandNamePtr = AtPlusCgdcontPara,
        .intermediateRspPtr = { NULL },
        .finalRspPtr = OkRsp,
        .readIndex = 0
    },
    {
        .commandNamePtr = AtPlusCpinRead,
        .intermediateRspPtr = { "+CPIN: READY", NULL },
        .finalRspPtr = OkRsp,
        .readIndex = 0
    },
    {
        .commandNamePtr = AtPlusCgdcontRead,
        .intermediateRspPtr = { "+CGDCONT: 1,\"IP\",\"orange\"",
                                "+CGDCONT: 2,\"IP\",\"bouygues\"",
                                "+CGDCONT: 3,\"IP\",\"sfr\"",
                                NULL },
        .finalRspPtr = OkRsp,
        .readIndex = 0
    },
    {
        .commandNamePtr = AtPlusCgdcontTest,
        .intermediateRspPtr = { "+CGDCONT: (1-16),\"IP\",,,(0-2),(0-4)",
                                "+CGDCONT: (1-16),\"PPP\",,,(0-2),(0-4)",
                                "+CGDCONT: (1-16),\"IPV6\",,,(0-2),(0-4)",
                                "+CGDCONT: (1-16),\"IPV4V6\",,,(0-2),(0-4)",
                                NULL },
        .finalRspPtr = OkRsp,
        .readIndex = 0
    },
    {
        .commandNamePtr = AtQ1,
        .intermediateRspPtr = { "Q1", NULL },
        .finalRspPtr = OkRsp,
        .readIndex = 0
    },
    {
        .commandNamePtr = AtPlusBad,
        .intermediateRspPtr = { NULL },
        .finalRspPtr = ErrorRsp,
        .readIndex = 0
    },
};

//--------------------------------------------------------------------------------------------------
/**
 * static variables
 *
 */
//--------------------------------------------------------------------------------------------------
static AtCommandDesc_t* CurrentCmdPtr = NULL;
static le_sem_Ref_t     BridgeSemaphore;
static const le_atClient_DeviceRef_t  AtClientDeviceRef = (le_atClient_DeviceRef_t) 0x12345678;
static le_atClient_UnsolicitedResponseHandlerFunc_t UnsolHandler = NULL;
static void* UnsolHandlerContextPtr = NULL;
static int FdAtClient = -1;

//--------------------------------------------------------------------------------------------------
/**
 * extern functions
 *
 */
//--------------------------------------------------------------------------------------------------
extern le_result_t SendCommandsAndTest(
    int fd,
    int epollFd,
    const char* commandsPtr,
    const char* expectedResponsePtr
);
extern le_result_t TestResponses
(
    int fd,
    int epollFd,
    const char* expectedResponsePtr
);

//--------------------------------------------------------------------------------------------------
/**
 *
 * Connect the current client thread to the service providing this API. Block until the service is
 * available.
 *
 * For each thread that wants to use this API, either ConnectService or TryConnectService must be
 * called before any other functions in this API.  Normally, ConnectService is automatically called
 * for the main thread, but not for any other thread. For details, see @ref apiFilesC_client.
 *
 * This function is created automatically.
 */
//--------------------------------------------------------------------------------------------------
void le_atClient_ConnectService
(
    void
)
{
}

//--------------------------------------------------------------------------------------------------
/**
 *
 * Disconnect the service from current client thread providing this API.
 */
//--------------------------------------------------------------------------------------------------
void le_atClient_DisconnectService
(
    void
)
{
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to automatically set and send an AT Command.
 *
 * @return
 *      - LE_FAULT when function failed
 *      - LE_NOT_FOUND when the AT Command reference is invalid
 *      - LE_TIMEOUT when a timeout occur
 *      - LE_OK when function succeed
 *
 * @note If the AT command is invalid, a fatal error occurs,
 *       the function won't return.
 *
 * @note The AT command reference is created and returned by this API, there's no need to call
 * le_atClient_Create() first.
 *
 */
//--------------------------------------------------------------------------------------------------
le_atClient_DeviceRef_t le_atClient_Start
(
    int32_t              fd          ///< The file descriptor
)
{
    LE_DEBUG("le_atClient_Start");

    LE_ASSERT(fd == FdAtClient);

    return AtClientDeviceRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to automatically set and send an AT Command.
 *
 * @return
 *      - LE_FAULT when function failed
 *      - LE_TIMEOUT when a timeout occur
 *      - LE_OK when function succeed
 *
 * @note This command creates a command reference when called
 *
 * @note In case of an Error the command reference will be deleted and though
 *       not usable. Make sure to test the return code and not use the reference
 *       in other functions.
 *
 * @note If the AT command is invalid, a fatal error occurs,
 *       the function won't return.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atClient_SetCommandAndSend
(
    le_atClient_CmdRef_t* cmdRefPtr,
        ///< [OUT] Command reference

    le_atClient_DeviceRef_t devRef,
        ///< [IN] Device reference

    const char* commandPtr,
        ///< [IN] AT Command

    const char* interRespPtr,
        ///< [IN] Expected Intermediate Response

    const char* finalRespPtr,
        ///< [IN] Expected Final Response

    uint32_t timeout
        ///< [IN] Timeout
)
{
    LE_ASSERT(devRef == AtClientDeviceRef);

    int i = 0;

    CurrentCmdPtr = NULL;

    while ( i < NUM_ARRAY_MEMBERS(AtCommandList) )
    {
        if (strncmp(commandPtr,
            AtCommandList[i].commandNamePtr,
            strlen(AtCommandList[i].commandNamePtr)) == 0)
        {
            CurrentCmdPtr = &AtCommandList[i];
            *cmdRefPtr = (le_atClient_CmdRef_t) CurrentCmdPtr;
            return LE_OK;
        }
        i++;
    }

    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to get the first intermediate response.
 *
 * @return
 *      - LE_FAULT when function failed
 *      - LE_OK when function succeed
 *
 * @note If the AT Command reference is invalid, a fatal error occurs,
 *       the function won't return.
*/
//--------------------------------------------------------------------------------------------------
le_result_t le_atClient_GetFirstIntermediateResponse
(
    le_atClient_CmdRef_t cmdRef,
        ///< [IN] AT Command

    char* intermediateRspPtr,
        ///< [OUT] Get Next Line

    size_t intermediateRspNumElements
        ///< [IN]
)
{
    LE_ASSERT(cmdRef == (le_atClient_CmdRef_t) CurrentCmdPtr);
    LE_ASSERT(CurrentCmdPtr != NULL);
    LE_ASSERT(CurrentCmdPtr->readIndex == 0);

    if (CurrentCmdPtr->intermediateRspPtr[0] != NULL)
    {
        snprintf(intermediateRspPtr,
                 intermediateRspNumElements,
                 "%s",
                 CurrentCmdPtr->intermediateRspPtr[0]);

        CurrentCmdPtr->readIndex++;

        return LE_OK;
    }

    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to get the next intermediate response.
 *
 * @return
 *      - LE_NOT_FOUND when there are no further results.
 *      - LE_OK when function succeed
 *
 * @note If the AT Command reference is invalid, a fatal error occurs,
 *       the function won't return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atClient_GetNextIntermediateResponse
(
    le_atClient_CmdRef_t cmdRef,
        ///< [IN] AT Command

    char* intermediateRspPtr,
        ///< [OUT] Get Next Line

    size_t intermediateRspNumElements
        ///< [IN]
)
{
    LE_ASSERT(cmdRef == (le_atClient_CmdRef_t) CurrentCmdPtr);
    LE_ASSERT(CurrentCmdPtr != NULL);
    LE_ASSERT(CurrentCmdPtr->readIndex != 0);

    if (CurrentCmdPtr->intermediateRspPtr[CurrentCmdPtr->readIndex] != NULL)
    {
        snprintf(intermediateRspPtr,
                 intermediateRspNumElements,
                 "%s",
                 CurrentCmdPtr->intermediateRspPtr[CurrentCmdPtr->readIndex]);

        CurrentCmdPtr->readIndex++;

        return LE_OK;
    }


    return LE_NOT_FOUND;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to get the final response
 *
 * @return
 *      - LE_FAULT when function failed
 *      - LE_OK when function succeed
 *
 * @note If the AT Command reference is invalid, a fatal error occurs,
 *       the function won't return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atClient_GetFinalResponse
(
    le_atClient_CmdRef_t cmdRef,
        ///< [IN] AT Command

    char* finalRspPtr,
        ///< [OUT] Get Final Line

    size_t finalRspNumElements
        ///< [IN]
)
{
    LE_ASSERT(cmdRef == (le_atClient_CmdRef_t) CurrentCmdPtr);
    LE_ASSERT(CurrentCmdPtr != NULL);

    if (CurrentCmdPtr->finalRspPtr)
    {
        snprintf(finalRspPtr,
                 finalRspNumElements,
                 "%s",
                 CurrentCmdPtr->finalRspPtr);

        CurrentCmdPtr->readIndex = 0;

        return LE_OK;
    }

    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This event provides information on a subscribed unsolicited response when this unsolicited
 * response is received.
 *
 */
//--------------------------------------------------------------------------------------------------
le_atClient_UnsolicitedResponseHandlerRef_t le_atClient_AddUnsolicitedResponseHandler
(
    const char* unsolRsp,
        ///< [IN] Pattern to match

    le_atClient_DeviceRef_t devRef,
        ///< [IN] Device to listen

    le_atClient_UnsolicitedResponseHandlerFunc_t handlerPtr,
        ///< [IN]

    void* contextPtr,
        ///< [IN]

    uint32_t lineCount
)
{
    LE_ASSERT(devRef == AtClientDeviceRef);

    UnsolHandler = handlerPtr;
    UnsolHandlerContextPtr = contextPtr;

    return (le_atClient_UnsolicitedResponseHandlerRef_t) UnsolHandler;
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'le_atClient_UnsolicitedResponse'
 */
//--------------------------------------------------------------------------------------------------
void le_atClient_RemoveUnsolicitedResponseHandler
(
    le_atClient_UnsolicitedResponseHandlerRef_t addHandlerRef
        ///< [IN]
)
{
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to delete an AT command reference.
 *
 * @return
 *      - LE_OK when function succeed
 *
 * @note If the AT Command reference is invalid, a fatal error occurs,
 *       the function won't return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atClient_Delete
(
    le_atClient_CmdRef_t cmdRef
        ///< [IN] AT Command
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to stop the ATClient session on the specified device.
 *
 * @return
 *      - LE_FAULT when function failed
 *      - LE_OK when function succeed
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atClient_Stop
(
    le_atClient_DeviceRef_t devRef
)
{
    LE_DEBUG("le_atClient_Stop");

    LE_ASSERT(devRef == AtClientDeviceRef);
    close(FdAtClient);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Start the bridge
 *
 */
//--------------------------------------------------------------------------------------------------
static void StartBridge
(
    void *param1Ptr,
    void *param2Ptr
)
{
    // dup stdout
    FdAtClient = dup(2);
    le_atServer_BridgeRef_t bridgeRef = le_atServer_OpenBridge(FdAtClient);
    LE_ASSERT(bridgeRef != NULL);
    *(le_atServer_BridgeRef_t*) param1Ptr = bridgeRef;
    le_atServer_DeviceRef_t devRef = param2Ptr;

    LE_ASSERT_OK(le_atServer_AddDeviceToBridge(devRef, bridgeRef));
    LE_ASSERT(le_atServer_AddDeviceToBridge(devRef, bridgeRef) == LE_BUSY);

    le_sem_Post(BridgeSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to test the AT server bridge feature.
 *
 * APIs tested:
 * - le_atServer_OpenBridge
 * - le_atServer_CloseBridge
 * - le_atServer_AddDeviceToBridge
 * - le_atServer_RemoveDeviceFromBridge
 *
 * @return
 *      - LE_FAULT when function failed
 *      - LE_OK when function succeed
 */
//--------------------------------------------------------------------------------------------------
le_result_t Testle_atServer_Bridge
(
    int socketFd,
    int epollFd,
    SharedData_t* sharedDataPtr
)
{
    LE_INFO("======== Test AT server bridge API ========");
    le_result_t ret;
    le_atServer_BridgeRef_t bridgeRef = NULL;

    BridgeSemaphore = le_sem_Create("BridgeSem", 0);

    le_event_QueueFunctionToThread( sharedDataPtr->atServerThread,
                                    StartBridge,
                                    (void*) &bridgeRef,
                                    (void*) sharedDataPtr->devRef);

    le_sem_Wait(BridgeSemaphore);

    ret = SendCommandsAndTest(socketFd, epollFd, AtPlusCpinRead,
                "\r\n+CPIN: READY\r\n"
                "\r\nOK\r\n");

    if (ret != LE_OK)
    {
        return ret;
    }

    ret = SendCommandsAndTest(socketFd, epollFd, AtPlusCgdcontPara,
                "\r\nOK\r\n");

    if (ret != LE_OK)
    {
        return ret;
    }

    ret = SendCommandsAndTest(socketFd, epollFd, AtPlusBad,
                "\r\nERROR\r\n");

    if (ret != LE_OK)
    {
        return ret;
    }

    ret = SendCommandsAndTest(socketFd, epollFd, AtPlusUnknown,
                "\r\nERROR\r\n");

    if (ret != LE_OK)
    {
        return ret;
    }

    ret = SendCommandsAndTest(socketFd, epollFd, ConcatCpinReadCgdcontRead,
                "\r\n+CPIN: READY\r\n"
                "\r\n+CGDCONT: 1,\"IP\",\"orange\"\r\n"
                "+CGDCONT: 2,\"IP\",\"bouygues\"\r\n"
                "+CGDCONT: 3,\"IP\",\"sfr\"\r\n"
                "\r\nOK\r\n");

    if (ret != LE_OK)
    {
        return ret;
    }

    ret = SendCommandsAndTest(socketFd, epollFd, ConcatQ1CpinReadAbcdReadCgdcontTest,
                "\r\nQ1\r\n"
                "\r\n+CPIN: READY\r\n"
                "\r\n+ABCD TYPE: READ\r\n"
                "\r\n+CGDCONT: (1-16),\"IP\",,,(0-2),(0-4)\r\n"
                "+CGDCONT: (1-16),\"PPP\",,,(0-2),(0-4)\r\n"
                "+CGDCONT: (1-16),\"IPV6\",,,(0-2),(0-4)\r\n"
                "+CGDCONT: (1-16),\"IPV4V6\",,,(0-2),(0-4)\r\n"
                "\r\nOK\r\n");

    if (ret != LE_OK)
    {
        return ret;
    }

    // Test unsolicited handler
    LE_ASSERT(UnsolHandler != NULL);
    UnsolHandler( "+CREG: 1", UnsolHandlerContextPtr);
    ret = TestResponses(socketFd, epollFd, "\r\n+CREG: 1\r\n");

    if (ret != LE_OK)
    {
        return ret;
    }

    LE_ASSERT_OK(le_atServer_RemoveDeviceFromBridge(sharedDataPtr->devRef, bridgeRef));

    LE_ASSERT_OK(le_atServer_CloseBridge(bridgeRef));

    LE_INFO("======== AT server bridge API test success ========");

    return ret;
}
