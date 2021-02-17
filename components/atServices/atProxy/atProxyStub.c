/**
 * @file atProxyStub.c
 *
 * Stub functions for le_atServer.api
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"
#include "atProxy.h"
#include "atProxyCmdHandler.h"


//--------------------------------------------------------------------------------------------------
/**
 * Suspend server / enter data mode
 *
 * When this function is called the server stops monitoring the fd for events
 * hence no more I/O operations are done on the fd by the server.
 *
 * @return
 *      - LE_OK             Success.
 *      - LE_BAD_PARAMETER  Invalid device reference.
 *      - LE_FAULT          Device not monitored
 *
 */
//--------------------------------------------------------------------------------------------------
void le_atServer_Suspend
(
    le_atServer_ServerCmdRef_t cmdRef,  ///< [IN] Asynchronous Server Command Reference
    le_atServer_DeviceRef_t devRef      ///< [IN] device to be unbinded
)
{
    LE_UNUSED(devRef);

    LE_WARN("Not implemented!");
    le_atServer_SuspendRespond(cmdRef, LE_FAULT);
}

//--------------------------------------------------------------------------------------------------
/**
 * Resume server / enter command mode
 *
 * When this function is called the server resumes monitoring the fd for events
 * and is able to interpret AT commands again.
 *
 * @return
 *      - LE_OK             Success.
 *      - LE_BAD_PARAMETER  Invalid device reference.
 *      - LE_FAULT          Device not monitored
 *
 */
//--------------------------------------------------------------------------------------------------
void le_atServer_Resume
(
    le_atServer_ServerCmdRef_t cmdRef,  ///< [IN] Asynchronous Server Command Reference
    le_atServer_DeviceRef_t devRef      ///< [IN] device to be unbound
)
{
    LE_UNUSED(devRef);

    LE_WARN("Not implemented!");
    le_atServer_ResumeRespond(cmdRef, LE_FAULT);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function opens an AT server session on the requested device.
 *
 * @return
 *      - Reference to the requested device.
 *      - NULL if the device is not available or fd is a BAD FILE DESCRIPTOR.
 */
//--------------------------------------------------------------------------------------------------
void le_atServer_Open
(
    le_atServer_ServerCmdRef_t cmdRef,  ///< [IN] Asynchronous Server Command Reference
    int fd                              ///< The file descriptor
)
{
    LE_UNUSED(fd);

    LE_WARN("Not implemented!");
    le_atServer_OpenRespond(cmdRef, NULL);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function closes the AT server session on the requested device.
 *
 * @return
 *      - LE_OK             The function succeeded.
 *      - LE_BAD_PARAMETER  Invalid device reference.
 *      - LE_BUSY           The requested device is busy.
 *      - LE_FAULT          Failed to stop the server, check logs for more information.
 */
//--------------------------------------------------------------------------------------------------
void le_atServer_Close
(
    le_atServer_ServerCmdRef_t cmdRef,  ///< [IN] Asynchronous Server Command Reference
    le_atServer_DeviceRef_t devRef      ///< [IN] device to be unbinded
)
{
    LE_UNUSED(devRef);

    LE_WARN("Not implemented!");
    le_atServer_CloseRespond(cmdRef, LE_FAULT);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function deletes an AT command (i.e. unregister from the AT parser).
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to delete the command.
 *      - LE_BUSY          Command is in progress.
 *
 */
//--------------------------------------------------------------------------------------------------
void le_atServer_Delete
(
    le_atServer_ServerCmdRef_t cmdRef,  ///< [IN] Asynchronous Server Command Reference
    le_atServer_CmdRef_t commandRef     ///< [IN] AT command reference
)
{
    LE_UNUSED(commandRef);

    LE_WARN("Not implemented!");

    le_atServer_DeleteRespond(cmdRef, LE_FAULT);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'le_atServer_CmdRegistration'
 *
 * This event provides information when a new AT command is subscribed.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_atServer_CmdRegistrationHandlerRef_t le_atServer_AddCmdRegistrationHandler
(
    le_atServer_CmdRegistrationHandlerFunc_t handlerPtr,    ///< [IN] Handler
    void* contextPtr                                        ///< [IN] Context pointer
)
{
    LE_UNUSED(handlerPtr);
    LE_UNUSED(contextPtr);

    LE_WARN("Not implemented!");
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * le_atServer_CmdRegistrationHandler handler REMOVE function
 */
//--------------------------------------------------------------------------------------------------
void le_atServer_RemoveCmdRegistrationHandler
(
    le_atServer_CmdRegistrationHandlerRef_t handlerRef ///< [IN] The handler reference
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * This function enables echo on the selected device.
 *
 * @return
 *      - LE_OK             The function succeeded.
 *      - LE_BAD_PARAMETER  Invalid device reference.
 */
//--------------------------------------------------------------------------------------------------
void le_atServer_EnableEcho
(
    le_atServer_ServerCmdRef_t cmdRef,  ///< [IN] Asynchronous Server Command Reference
    le_atServer_DeviceRef_t device      ///< [IN] device reference

)
{
    LE_UNUSED(device);

    LE_WARN("Not implemented!");
    le_atServer_EnableEchoRespond(cmdRef, LE_FAULT);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function disables echo on the selected device.
 *
 * @return
 *      - LE_OK             The function succeeded.
 *      - LE_BAD_PARAMETER  Invalid device reference.
 */
//--------------------------------------------------------------------------------------------------
void le_atServer_DisableEcho
(
    le_atServer_ServerCmdRef_t cmdRef,  ///< [IN] Asynchronous Server Command Reference
    le_atServer_DeviceRef_t device      ///< [IN] device reference

)
{
    LE_UNUSED(device);

    LE_WARN("Not implemented!");
    le_atServer_DisableEchoRespond(cmdRef, LE_BAD_PARAMETER);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function opens a AT commands server bridge.
 * All unknown AT commands will be sent on this alternative file descriptor thanks to the AT client
 * Service.
 *
 * @return
 *      - Reference to the requested bridge.
 *      - NULL if the device can't be bridged
 */
//--------------------------------------------------------------------------------------------------
void le_atServer_OpenBridge
(
    le_atServer_ServerCmdRef_t cmdRef,  ///< [IN] Asynchronous Server Command Reference
    int fd                              ///< [IN] File descriptor.
)
{
    LE_UNUSED(fd);

#if MK_CONFIG_DISABLE_AT_BRIDGE
    le_atServer_BridgeRef_t bridgeRef = NULL;
    LE_WARN("Not implemented!");
    le_atServer_OpenBridgeRespond(cmdRef, bridgeRef);
#else
    // Do something to open bridge
#endif // MK_CONFIG_DISABLE_AT_BRIDGE
}

//--------------------------------------------------------------------------------------------------
/**
 * This function closes an opened bridge.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to close the bridge.
 *      - LE_BUSY          The bridge is in use (devices references have to be removed first).
 */
//--------------------------------------------------------------------------------------------------
void le_atServer_CloseBridge
(
    le_atServer_ServerCmdRef_t cmdRef,  ///< [IN] Asynchronous Server Command Reference
    le_atServer_BridgeRef_t bridgeRef   ///< [IN] Bridge refence
)
{
#if MK_CONFIG_DISABLE_AT_BRIDGE
    LE_UNUSED(bridgeRef);
    LE_WARN("Not implemented!");
    le_atServer_CloseBridgeRespond(cmdRef, LE_FAULT);
#else
    // Do something to close bridge
#endif // MK_CONFIG_DISABLE_AT_BRIDGE
}

//--------------------------------------------------------------------------------------------------
/**
 * This function adds a device to an opened bridge.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_BUSY          The device is already used by the bridge.
 *      - LE_FAULT         The function failed to add the device to the bridge.
 */
//--------------------------------------------------------------------------------------------------
void le_atServer_AddDeviceToBridge
(
    le_atServer_ServerCmdRef_t cmdRef,  ///< [IN] Asynchronous Server Command Reference
    le_atServer_DeviceRef_t deviceRef,  ///< [IN] Device reference to add to the bridge
    le_atServer_BridgeRef_t bridgeRef   ///< [IN] Bridge refence
)
{
#if MK_CONFIG_DISABLE_AT_BRIDGE
    LE_UNUSED(deviceRef);
    LE_UNUSED(bridgeRef);
    LE_WARN("Not implemented!");
    le_atServer_AddDeviceToBridgeRespond(cmdRef, LE_FAULT);
#else
    // Do something to add device to bridge
#endif // MK_CONFIG_DISABLE_AT_BRIDGE
}

//--------------------------------------------------------------------------------------------------
/**
 * This function removes a device from a bridge
 *
 * @return
 *      - LE_OK            The function succeeded.
        - LE_NOT_FOUND     The device is not isued by the specified bridge
 *      - LE_FAULT         The function failed to add the device to the bridge.
 */
//--------------------------------------------------------------------------------------------------
void le_atServer_RemoveDeviceFromBridge
(
    le_atServer_ServerCmdRef_t cmdRef,  ///< [IN] Asynchronous Server Command Reference
    le_atServer_DeviceRef_t deviceRef,  ///< [IN] Device reference to add to the bridge
    le_atServer_BridgeRef_t bridgeRef   ///< [IN] Bridge refence
)
{
#if MK_CONFIG_DISABLE_AT_BRIDGE
    LE_UNUSED(deviceRef);
    LE_UNUSED(bridgeRef);
    LE_WARN("Not implemented!");
    le_atServer_RemoveDeviceFromBridgeRespond(cmdRef, LE_FAULT);
#else
    // Do something to remove device from bridge
#endif // MK_CONFIG_DISABLE_AT_BRIDGE
}

//--------------------------------------------------------------------------------------------------
/**
 * This function enables verbose error codes on the selected device
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void le_atServer_EnableVerboseErrorCodes
(
    le_atServer_ServerCmdRef_t cmdRef   ///< [IN] Asynchronous Server Command Reference
)
{
    LE_WARN("Not implemented!");
    le_atServer_EnableVerboseErrorCodesRespond(cmdRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function creates a custom error code.
 * @return
 *      - ErrorCode    Reference to the created error code
 *      - NULL         Function failed to create the error code
 *
 * @note This function fails to create the error code if the combinaison (errorCode, pattern)
 * already exists or if the errorCode number is lower than 512.
 *
 */
//--------------------------------------------------------------------------------------------------
void le_atServer_CreateErrorCode
(
    le_atServer_ServerCmdRef_t cmdRef,  ///< [IN] Asynchronous Server Command Reference
    uint32_t errorCode,                 ///< [IN] Numerical error code
    const char* patternPtr              ///< [IN] Prefix of the final response string
)
{
#if LE_CONFIG_ATSERVER_USER_ERRORS
    // Do something
#else
    LE_UNUSED(errorCode);
    LE_UNUSED(patternPtr);
    LE_WARN("Not implemented!");
    le_atServer_CreateErrorCodeRespond(cmdRef, NULL);
#endif // LE_CONFIG_ATSERVER_USER_ERRORS
}

//--------------------------------------------------------------------------------------------------
/**
 * This function deletes a custom error code
 *
 * @return
 *      - LE_OK            The function succeeded
        - LE_FAULT         The function failed to delete the error code
 */
//--------------------------------------------------------------------------------------------------
void le_atServer_DeleteErrorCode
(
    le_atServer_ServerCmdRef_t cmdRef,      ///< [IN] Asynchronous Server Command Reference
    le_atServer_ErrorCodeRef_t errorCodeRef ///< [IN] Reference to a custom error code
)
{
#if LE_CONFIG_ATSERVER_USER_ERRORS
    // Do something
#else
    LE_UNUSED(errorCodeRef);
    LE_WARN("Not implemented!");
    le_atServer_DeleteErrorCodeRespond(cmdRef, LE_FAULT);
#endif // LE_CONFIG_ATSERVER_USER_ERRORS
}

//--------------------------------------------------------------------------------------------------
/**
 * This function adds a verbose message to a specified error code
 *
 * @return
 *      - LE_OK            The function succeeded
 *      - LE_FAULT         The function failed to set the verbose message
 */
//--------------------------------------------------------------------------------------------------
void le_atServer_SetVerboseErrorCode
(
    le_atServer_ServerCmdRef_t  cmdRef,         ///< [IN] Asynchronous Server Command Reference
    le_atServer_ErrorCodeRef_t  errorCodeRef,   ///< [IN] Reference to a custom error code
    const char*  messagePtr                     ///< [IN] Verbose string
)
{
#if LE_CONFIG_ATSERVER_USER_ERRORS
    // Do something
#else
    LE_UNUSED(errorCodeRef);
    LE_UNUSED(messagePtr);
    LE_WARN("Not implemented!");
    le_atServer_SetVerboseErrorCodeRespond(cmdRef, LE_FAULT);
#endif // LE_CONFIG_ATSERVER_USER_ERRORS
}
