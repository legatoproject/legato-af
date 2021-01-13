/**
 * @file atProxy.c
 *
 * AT Proxy interface implementation.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"
#include "atProxy.h"
#include "atProxyCmdHandler.h"
#include "atProxySerialUart.h"
#include "atProxyAdaptor.h"

#define AT_PROXY_CMD_REGISTRY_IMPL   1
#include "atProxyCmdRegistry.h"


//--------------------------------------------------------------------------------------------------
/**
 * Static map for AT Command references
 */
//--------------------------------------------------------------------------------------------------
LE_REF_DEFINE_STATIC_MAP(AtCmdRefMap, AT_CMD_MAX);

//--------------------------------------------------------------------------------------------------
/**
 * Map for AT commands
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t  AtCmdRefMap;


//--------------------------------------------------------------------------------------------------
/**
 * Map for AT Command Sessions
 */
//--------------------------------------------------------------------------------------------------
extern le_ref_MapRef_t  atCmdSessionRefMap;


//--------------------------------------------------------------------------------------------------
/**
 * Function to retrieve the AT Command Registry
 */
//--------------------------------------------------------------------------------------------------
struct le_atProxy_StaticCommand* le_atProxy_GetCmdRegistry
(
    void
)
{
    return &AtCmdRegistry[0];
}

//--------------------------------------------------------------------------------------------------
/**
 * Function to retrieve the AT Command Registry entry for a specific command
 */
//--------------------------------------------------------------------------------------------------
struct le_atProxy_StaticCommand* le_atProxy_GetCmdRegistryEntry
(
    uint32_t command
)
{
    // Verify the command index is within range
    LE_ASSERT(command < AT_CMD_MAX);

    return &AtCmdRegistry[command];
}


//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'le_atProxy_Command'
 *
 * This event provides information when the AT command is detected.
 *
 */
//--------------------------------------------------------------------------------------------------
le_atProxy_CommandHandlerRef_t le_atProxy_AddCommandHandler
(
    uint32_t command,
        ///< [IN] AT Command index
    le_atProxy_CommandHandlerFunc_t handlerPtr,
        ///< [IN] Handler to called when the AT command is detected
    void* contextPtr
        ///< [IN] Context pointer provided by caller and returned when handlePtr is called
)
{
    LE_DEBUG("Calling le_atProxy_AddCommandHandler");

    if (command >= AT_CMD_MAX)
    {
        return NULL;
    }

    // Set pointer to AT Command Register entry
    struct le_atProxy_StaticCommand* atCmdRegistryPtr = &AtCmdRegistry[command];

    // Set the Command Handler Callback function and Context Pointer
    atCmdRegistryPtr->commandHandlerPtr = handlerPtr;
    atCmdRegistryPtr->contextPtr = contextPtr;

    // Create Safe Reference to AT Command Registry entry
    return le_ref_CreateRef(AtCmdRefMap, atCmdRegistryPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'le_atProxy_Command'
 */
//--------------------------------------------------------------------------------------------------
void le_atProxy_RemoveCommandHandler
(
    le_atProxy_CommandHandlerRef_t handlerRef
        ///< [IN] Reference of Command Handler to be removed
)
{
    // Look-up AT Command Register entry using handlerRef
    struct le_atProxy_StaticCommand* atCmdRegistryPtr = le_ref_Lookup(AtCmdRefMap, handlerRef);

    // Delete Safe Reference to AT Command Registry entry
    le_ref_DeleteRef(AtCmdRefMap, handlerRef);

    if (atCmdRegistryPtr == NULL)
    {
        LE_INFO("Unable to retrieve AT Command Registry entry, handlerRef [%" PRIuPTR "]",
                (uintptr_t) handlerRef);
        return;
    }

    // Reset the Command Handler Callback function and Context Pointer
    atCmdRegistryPtr->commandHandlerPtr = NULL;
    atCmdRegistryPtr->contextPtr = NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function can be used to get the parameters of a received AT command.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to get the requested parameter.
 *
 */
//--------------------------------------------------------------------------------------------------
void le_atProxy_GetParameter
(
    le_atProxy_ServerCmdRef_t _cmdRef,  ///< [IN] Asynchronous Server Command Reference
    le_atProxy_CmdRef_t commandRef, ///< [IN] AT Command Reference
    uint32_t index, ///< [IN] Index of Parameter to be retrieved
    size_t parameterSize  ///< [IN] Size of parameterSize buffer
)
{
    le_result_t result = LE_OK;
    const char* parameter = NULL;

    struct le_atProxy_AtCommandSession* atCmdSessionPtr =
        le_ref_Lookup(atCmdSessionRefMap, commandRef);

    // Verify AT Command Session pointer is valid
    if (atCmdSessionPtr == NULL)
    {
        LE_ERROR("AT Command Session reference pointer is NULL");
        result = LE_FAULT;
    }
    else if (parameterSize < sizeof(atCmdSessionPtr->atCmdParameterList[index]))
    {
        LE_ERROR("Parameter buffer too small");
        result = LE_OVERFLOW;
    }
    else
    {
        // Set pointer to response parameter
        parameter = atCmdSessionPtr->atCmdParameterList[index];
    }

    // Send response to client
    le_atProxy_GetParameterRespond(_cmdRef, result, parameter);
}


//--------------------------------------------------------------------------------------------------
/**
 * This function can be used to get the AT command string.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to get the AT command string.
 *
 */
//--------------------------------------------------------------------------------------------------
void le_atProxy_GetCommandName
(
    le_atProxy_ServerCmdRef_t _cmdRef,  ///< [IN] Asynchronous Server Command Reference
    le_atProxy_CmdRef_t commandRef,  ///< [IN] AT Command Reference
    size_t nameSize  ///< [IN] Size of nameSize buffer
)
{
    le_result_t result = LE_OK;
    const char* name = NULL;

    struct le_atProxy_AtCommandSession* atCmdSessionPtr =
        le_ref_Lookup(atCmdSessionRefMap, commandRef);

    // Verify AT Command Session pointer is valid
    if (atCmdSessionPtr == NULL)
    {
        LE_ERROR("AT Command Session reference pointer is NULL");
        result = LE_FAULT;
    }
    else if (nameSize < sizeof(AtCmdRegistry[atCmdSessionPtr->registryIndex].commandStr))
    {
        LE_ERROR("Name buffer too small");
        result = LE_OVERFLOW;
    }
    else
    {
        // Set pointer to response command name
        name = AtCmdRegistry[atCmdSessionPtr->registryIndex].commandStr;
    }

    // Send response to client
    le_atProxy_GetCommandNameRespond(_cmdRef, result, name);
}


//--------------------------------------------------------------------------------------------------
/**
 * This function can be used to send an intermediate response.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to send the intermediate response.
 *
 */
//--------------------------------------------------------------------------------------------------
void le_atProxy_SendIntermediateResponse
(
    le_atProxy_ServerCmdRef_t _cmdRef,  ///< [IN] Asynchronous Server Command Reference
    le_atProxy_CmdRef_t commandRef,  ///< [IN] AT Command Reference
    const char* LE_NONNULL responseStr  ///< [IN] Intermediate Response String
)
{
    le_result_t result = LE_OK;

    (void) commandRef;

    // Write the responseStr out to the console port
    atProxySerialUart_write((char*) responseStr, strlen(responseStr));
    atProxySerialUart_write("\r\n", strlen("\r\n"));

    le_atProxy_SendIntermediateResponseRespond(_cmdRef, result);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function can be used to send the final result code.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to send the final result code.
 *
 */
//--------------------------------------------------------------------------------------------------
void le_atProxy_SendFinalResultCode
(
    le_atProxy_ServerCmdRef_t _cmdRef,  ///< [IN] Asynchronous Server Command Reference
    le_atProxy_CmdRef_t commandRef,  ///< [IN] AT Command Reference
    le_atProxy_FinalRsp_t finalResult,  ///< [IN] Final Response Result
    const char* LE_NONNULL pattern,  ///< [IN] Final Response Pattern String
    uint32_t errorCode  ///< [IN] Final Response Error Code
)
{
    le_result_t result = LE_OK;
    char buffer[LE_ATDEFS_RESPONSE_MAX_BYTES];

    struct le_atProxy_AtCommandSession* atCmdSessionPtr =
        le_ref_Lookup(atCmdSessionRefMap, commandRef);

    // Verify AT Command Session pointer is valid
    if (atCmdSessionPtr == NULL)
    {
        LE_ERROR("AT Command Session reference pointer is NULL");
        result = LE_FAULT;
    }

    switch (finalResult)
    {
        case LE_ATPROXY_OK:
            atProxySerialUart_write(LE_AT_PROXY_OK, strlen(LE_AT_PROXY_OK));
            break;

        case LE_ATPROXY_NO_CARRIER:
            atProxySerialUart_write(LE_AT_PROXY_NO_CARRIER, strlen(LE_AT_PROXY_NO_CARRIER));
            break;

        case LE_ATPROXY_NO_DIALTONE:
            atProxySerialUart_write(LE_AT_PROXY_NO_DIALTONE, strlen(LE_AT_PROXY_NO_DIALTONE));
            break;

        case LE_ATPROXY_BUSY:
            atProxySerialUart_write(LE_AT_PROXY_BUSY, strlen(LE_AT_PROXY_BUSY));
            break;

        default:
            snprintf(buffer, LE_ATDEFS_RESPONSE_MAX_LEN, "%s%lu\r\n", pattern, errorCode);
            atProxySerialUart_write(buffer, strlen(buffer));
            break;
    }

    le_atProxy_SendFinalResultCodeRespond(_cmdRef, result);

    // After sending out final response, set current AT session to complete
    atProxyCmdHandler_complete();
}

//--------------------------------------------------------------------------------------------------
/**
 * This function can be used to send the unsolicited response.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to send the unsolicited response.
 *
 */
//--------------------------------------------------------------------------------------------------
void le_atProxy_SendUnsolicitedResponse
(
    le_atProxy_ServerCmdRef_t _cmdRef,  ///< [IN] Asynchronous Server Command Reference
    const char* LE_NONNULL responseStr  ///< [IN] Unsolicited Response String
)
{
    le_result_t result = LE_OK;

    // Queue the response and defer outputting it if current AT session is active (in process)
    if (atProxyCmdHandler_isActive())
    {
        atProxyCmdHandler_StoreUnsolicitedResponse(_cmdRef, responseStr);
        return;
    }

    atProxySerialUart_write((char *)responseStr, strlen(responseStr));
    atProxySerialUart_write("\r\n", strlen("\r\n"));

    le_atProxy_SendUnsolicitedResponseRespond(_cmdRef, result);
}

//-------------------------------------------------------------------------------------------------
/**
 * Component initialisation once for all component instances.
 */
//-------------------------------------------------------------------------------------------------
COMPONENT_INIT_ONCE
{
    le_atProxy_initOnce();
}

//-------------------------------------------------------------------------------------------------
/**
 * Component initialisation.
 */
//-------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("Starting AT Proxy");

    // AT Command Reference pool allocation
    AtCmdRefMap = le_ref_InitStaticMap(AtCmdRefMap, AT_CMD_MAX);

    // Call platform-specific initializer
    le_atProxy_init();
}
