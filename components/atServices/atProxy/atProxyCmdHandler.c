/**
 * @file atProxyCmdHandler.c
 *
 * AT Proxy Command Handler implementation.   Responible for
 * 1) Parse the STDIN/console to identify incoming AT commands,
 * 2) Create and manage the AT Command session for the tracking AT command being processed,
 * 3) Trigger the IPC Command Handler callback function associated with the AT command
 *    to notify the local Back-end that an AT command has arrived.
 * 4) Store and output URCs.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"
#include "atProxy.h"
#include "pa_port.h"
#include "pa_remote.h"
#include "atProxyCmdHandler.h"
#include "atProxyCmdRegistry.h"


#define AT_PROXY_PARAMETER_NONE      -1


//--------------------------------------------------------------------------------------------------
/**
 * Static map for AT Command Session references
 */
//--------------------------------------------------------------------------------------------------
LE_REF_DEFINE_STATIC_MAP(AtCmdSessionRefMap, LE_CONFIG_ATSERVER_DEVICE_POOL_SIZE);

//--------------------------------------------------------------------------------------------------
/**
 * Map for AT Command Sessions
 */
//--------------------------------------------------------------------------------------------------
le_ref_MapRef_t  atCmdSessionRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Pool for AT Command Sessions
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t    AtCmdSessionPoolRef;

//--------------------------------------------------------------------------------------------------
/**
 * Static pool for AT sessions
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(AtCmdSessions,
                          LE_CONFIG_ATSERVER_DEVICE_POOL_SIZE,
                          sizeof(le_atProxy_AtCommandSession_t));

//--------------------------------------------------------------------------------------------------
/**
 * Unsolicited responses pool size
 * Number of maximum URCs can be stored in atProxy at any time from all possible AT sessions.
 */
//--------------------------------------------------------------------------------------------------
#define AT_PROXY_UNSOLICITED_RESPONSE_COUNT         MK_CONFIG_UNSOLICITED_RESPONSE_COUNT

// Large size of one unsolicitied response string
#define AT_PROXY_UNSOLICITED_RESPONSE_LARGE_BYTES   LE_ATDEFS_COMMAND_MAX_BYTES

// Typical size of one unsolicitied response string
#define AT_PROXY_UNSOLICITED_RESPONSE_TYPICAL_BYTES 50

// Pool size for large-size response string
#define AT_PROXY_UNSOLICITED_RESPONSE_COUNT_LARGE   2

// Count of typical-size unsolicited responses in pool
//      the count = (# of blks in large pool) / 2 * (# of small blks in one large blk)
#define AT_PROXY_UNSOLICITED_RESPONSE_COUNT_TYPICAL                                             \
    (((LE_MEM_BLOCKS(UnsoliRspDataPoolRef, AT_PROXY_UNSOLICITED_RESPONSE_COUNT_LARGE) / 2)      \
        * AT_PROXY_UNSOLICITED_RESPONSE_LARGE_BYTES)                                            \
            / AT_PROXY_UNSOLICITED_RESPONSE_TYPICAL_BYTES)

// Static pool for response data/string
LE_MEM_DEFINE_STATIC_POOL(UnsoliRspDataPool,
                          AT_PROXY_UNSOLICITED_RESPONSE_COUNT_LARGE,
                          AT_PROXY_UNSOLICITED_RESPONSE_LARGE_BYTES);

// Pool for response string content
static le_mem_PoolRef_t  UnsoliRspDataPoolRef;

//--------------------------------------------------------------------------------------------------
/**
 * AT command response structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_atProxy_RspString
{
    le_dls_Link_t   link;               ///< link for list
    char* resp;                         ///< string pointer
} le_atProxy_RspString_t;

//--------------------------------------------------------------------------------------------------
/**
 * Static pool for unsolicited response
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(UnsoliRspPool,
                          AT_PROXY_UNSOLICITED_RESPONSE_COUNT,
                          sizeof(le_atProxy_RspString_t));

//--------------------------------------------------------------------------------------------------
/**
 * Pool for unsolicited response
 * w string
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t  UnsoliRspPoolRef;


//--------------------------------------------------------------------------------------------------
/**
 * Send backed-up unsolicited responses
 *
 * @return
 *      - LE_OK     Successfully processed URCs
 *      - LE_FAULT  Otherwise
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ProcessStoredURC
(
    struct le_atProxy_AtCommandSession* atSession      ///< [IN] AT Command Session Pointer
)
{
    le_dls_Link_t* linkPtr;

    if (!atSession)
    {
        LE_ERROR("Bad parameter!");
        return LE_FAULT;
    }

    while ( (linkPtr = le_dls_Pop(&atSession->unsolicitedList)) != NULL )
    {
        le_atProxy_RspString_t* rspStringPtr = CONTAINER_OF(linkPtr,
                                                            le_atProxy_RspString_t,
                                                            link);

        pa_port_Write(atSession->port, (char *)rspStringPtr->resp, strlen(rspStringPtr->resp));
        pa_port_Write(atSession->port, "\r\n", strlen("\r\n"));

        // Release string content
        le_mem_Release(rspStringPtr->resp);

        // Release URC item
        le_mem_Release(rspStringPtr);
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Allocate and initialize a new unsolicited response structure
 *
 * @return pointer of created le_atProxy_RspString_t instance if successful, NULL otherwise
 */
//--------------------------------------------------------------------------------------------------
static le_atProxy_RspString_t* CreateResponse
(
    const char* rspStr                  ///< [IN] Unsolicited Response String
)
{
    le_atProxy_RspString_t* rspStringPtr = le_mem_Alloc(UnsoliRspPoolRef);

    memset(rspStringPtr, 0, sizeof(le_atProxy_RspString_t));

    // Allocate space for response content
    int size = strnlen(rspStr, AT_PROXY_UNSOLICITED_RESPONSE_LARGE_BYTES - 1) + 1;
    rspStringPtr->resp = le_mem_VarAlloc(UnsoliRspDataPoolRef, size);
    if (NULL == rspStringPtr->resp)
    {
        LE_CRIT("Could not allocate space for string of size %d", size);
        le_mem_Release(rspStringPtr);
        return NULL;
    }

    memset(rspStringPtr->resp, 0, size);
    memcpy(rspStringPtr->resp, rspStr, size);

    return rspStringPtr;
}

//--------------------------------------------------------------------------------------------------
/**
 * Packs an AT Command parameter string into the Parameter List array
 *
 * @return
 *      - LE_OK         Successfully packed a parameter
 *      - LE_OVERFLOW   Parameter string is too long
 */
//--------------------------------------------------------------------------------------------------
static le_result_t PackParameterList
(
    struct le_atProxy_AtCommandSession* atCmdPtr,   ///< [INOUT] AT session pointer
    char* parameters,                               ///< [IN] String holds the parameter
    uint16_t startIndex,                            ///< [IN] Starting index of the parameter
    uint16_t endIndex,                              ///< [IN] Ending index of the parameter
    bool stringParam                                ///< [IN] Indicator of whether it's a string
                                                    ///<      parameter
)
{
    uint16_t parameterLength = (endIndex - startIndex);

    // Decrease the length by 2 (pair of quotes) if it's a string parameter
    // and skip the starting quote for startIndex
    if (stringParam)
    {
        startIndex += 1;
        parameterLength -= 2;
    }

    if (parameterLength > LE_ATDEFS_PARAMETER_MAX_LEN)
    {
        LE_ERROR("Parameter is too long, length [%" PRIu16 "]",
                    parameterLength);
        return LE_OVERFLOW;
    }

    if (atCmdPtr->parameterIndex >= AT_PROXY_PARAMETER_LIST_MAX)
    {
        LE_ERROR("Too many parameters - maximum number of supported parameters is %d",
                 AT_PROXY_PARAMETER_LIST_MAX);
        return LE_OVERFLOW;
    }

    // Declare a char* to the Parameter List element
    char* paramStr = atCmdPtr->atCmdParameterList[atCmdPtr->parameterIndex];

    // Store the parameter in the parameter list
    strncpy(paramStr, &parameters[startIndex], parameterLength);

    // NULL Terminate the parameter string.
    paramStr[parameterLength] = 0;

    LE_DEBUG("Parameter #%"PRIu32" = [%s]",
             atCmdPtr->parameterIndex,
             atCmdPtr->atCmdParameterList[atCmdPtr->parameterIndex]);

    // Increment the parameter index (count)
    atCmdPtr->parameterIndex++;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Separates the complete AT Command parameter string into individual parameters
 *
 * @return
 *      - LE_OK         Successfully packed a list of parameters
 *      - LE_OVERFLOW   At least one parameter string is too long
 */
//--------------------------------------------------------------------------------------------------
static le_result_t CreateParameterList
(
    struct le_atProxy_AtCommandSession* atCmdPtr   ///< AT Command Session Pointer
)
{
    LE_ASSERT(atCmdPtr->local);  // Should only be here for "local" commands

    le_result_t result = LE_OK;
    int16_t startIndex = AT_PROXY_PARAMETER_NONE;
    bool openQuote = false;
    bool stringParam = false;

    // Declare a string to hold the parameter
    char parameters[LE_ATDEFS_PARAMETER_MAX_BYTES];
    memset(parameters, 0, sizeof(parameters));

    // Initialize parameter index (number)
    atCmdPtr->parameterIndex = 0;

    // Extract the complete list of parameters from the AT Command string
    strncpy(parameters,
            &atCmdPtr->command[atCmdPtr->operatorIndex + 1],
            LE_ATDEFS_PARAMETER_MAX_LEN);

    LE_DEBUG("parameters = %s", parameters);

    // Traverse the entire parameter list string,
    // one character at a time, and separate it into individual parameters.
    for (uint16_t i = 0; i < strlen(parameters); i++)
    {
        char input = parameters[i];
        switch (input)
        {
            case AT_TOKEN_QUOTE:
                if ((i > 0) && (parameters[i - 1] == AT_TOKEN_BACKSLASH))
                {
                    // Escaped quote - ignore
                }
                else
                {
                    if (startIndex == AT_PROXY_PARAMETER_NONE)
                    {
                        startIndex = i;  // Mark the start of a new parameter
                        openQuote = true; // Start of open quote
                    }
                    else
                    {
                        openQuote = false;  // End of open quote
                        stringParam = true;
                    }
                }
            break;

            case AT_TOKEN_QUESTIONMARK:
            break;

            case AT_TOKEN_SPACE:
            break;

            case AT_TOKEN_COMMA:
                if (startIndex != AT_PROXY_PARAMETER_NONE)
                {
                    if (openQuote)
                    {
                        // Ignore this comma, it's inside an open quote
                    }
                    else
                    {
                        // Marks the end of the new parameter (list)
                        // Pack the Parameters in the Parameter List
                        result = PackParameterList(
                                     atCmdPtr,
                                     parameters,
                                     startIndex,
                                     i,
                                     stringParam);
                        if (result != LE_OK)
                        {
                            return result;
                        }
                        // Reset the startIndex and string parameter flag
                        startIndex = AT_PROXY_PARAMETER_NONE;
                        stringParam = false;
                    }
                }
                else
                {
                    if (parameters[i-1] == AT_TOKEN_COMMA)
                    {
                        startIndex = i;
                        result = PackParameterList(
                                     atCmdPtr,
                                     parameters,
                                     startIndex,
                                     i,
                                     stringParam);
                        if (result != LE_OK)
                        {
                            return result;
                        }
                        // Reset the startIndex and string parameter flag
                        startIndex = AT_PROXY_PARAMETER_NONE;
                        stringParam = false;
                    }
                }
            break;

            case AT_TOKEN_CR:
                if (startIndex != AT_PROXY_PARAMETER_NONE)
                {
                    // Marks the end of the new parameter (list)
                    // Pack the Parameters in the Parameter List
                    result = PackParameterList(
                                 atCmdPtr,
                                 parameters,
                                 startIndex,
                                 i,
                                 stringParam);

                    if (result != LE_OK)
                    {
                        return result;
                    }

                    // Reset the startIndex and string parameter flag
                    startIndex = AT_PROXY_PARAMETER_NONE;
                    stringParam = false;
                }
            break;

            default:
                if (startIndex == AT_PROXY_PARAMETER_NONE)
                {
                    startIndex = i;  // Mark the start of a new parameter
                }
            break;
        }
    }

    LE_DEBUG("Parameter count = [%" PRIu32 "]", atCmdPtr->parameterIndex);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Process AT Command by first creating a list of all the comma-separated parameters and then
 * calling the registered AT Command handle callback
 */
//--------------------------------------------------------------------------------------------------
static void ProcessAtCmd
(
    struct le_atProxy_AtCommandSession* atCmdPtr   ///< [IN] AT Command Session Pointer
)
{
    le_result_t result;

    if (atCmdPtr->local == true)
    {
        struct le_atProxy_StaticCommand* atCmdRegistryPtr = le_atProxy_GetCmdRegistry();

        result = CreateParameterList(atCmdPtr);
        if (result != LE_OK)
        {
            // Send an error to the Serial UART
            pa_port_Write(
                atCmdPtr->port,
                LE_AT_PROXY_ERROR,
                sizeof(LE_AT_PROXY_ERROR));

            LE_ERROR("Error parsing parameter list, result [%d]", result);
            return;
        }

        if (atCmdRegistryPtr[atCmdPtr->registryIndex].commandHandlerPtr != NULL)
        {
            atCmdPtr->active = true;
            pa_port_Write(atCmdPtr->port, "\r\n", strlen("\r\n"));
            pa_port_Disable(atCmdPtr->port);

            // Trigger the AT Command Handler callback registered for this "local"
            // AT Command
            atCmdRegistryPtr[atCmdPtr->registryIndex].commandHandlerPtr(
                atCmdPtr->ref,  // Command Reference
                atCmdPtr->type,    // Type
                atCmdPtr->parameterIndex,   // Number of parameters
                atCmdRegistryPtr[atCmdPtr->registryIndex].contextPtr);  // Callback context pointer
        }
        else
        {
            LE_ERROR("AT Command Registry callback function is NULL, cmd [%s], type [%d]",
                     atCmdRegistryPtr[atCmdPtr->registryIndex].commandStr,
                     atCmdPtr->type);
        }
    }
    else
    {
        // Send AT Command to remote end (MAP)
        LE_DEBUG("Sending AT command [%s] to remote", atCmdPtr->command);

        atCmdPtr->active = true;
        pa_port_Disable(atCmdPtr->port);

        result = pa_remote_Send(atCmdPtr->command,
                                    strnlen(atCmdPtr->command, LE_ATDEFS_COMMAND_MAX_BYTES));

        if (LE_OK != result)
        {
            LE_ERROR("Failed sending command to MAP!");
            pa_port_Enable(atCmdPtr->port);
            return;
        }

        pa_remote_WaitCmdFinish();
        pa_port_Enable(atCmdPtr->port);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Search AT Command Registry
 *
 * This function attempts to identify if the incoming AT command is "local" or "remote"
 */
//--------------------------------------------------------------------------------------------------
static void SearchAtCmdRegistry
(
    struct le_atProxy_AtCommandSession* atCmdPtr   ///< [IN] AT Command Session Pointer
)
{
    struct le_atProxy_StaticCommand* atCmdRegistryPtr = le_atProxy_GetCmdRegistry();

    // Traverse AT Command Registry
    for (uint32_t i = 0; i < AT_CMD_MAX; i++, atCmdRegistryPtr++)
    {
        // Sub-string to hold the AT Command
        char commandStr[atCmdPtr->operatorIndex + 1];

        // Create a sub-string from the parsed AT Command that is the
        // length of the registered AT Command String
        strncpy(commandStr, atCmdPtr->command, atCmdPtr->operatorIndex);

        // Ensure string is NULL terminated
        commandStr[atCmdPtr->operatorIndex] = 0;

        // Convert string to uppercase
        strupr(commandStr);

        LE_DEBUG("Searching buffer [%s] for prefix match str [%s], operatorIndex [%d]",
                 commandStr,
                 atCmdRegistryPtr->commandStr,
                 atCmdPtr->operatorIndex);

        if (strlen(commandStr) != strlen(atCmdRegistryPtr->commandStr))
        {
            // Command String is a different length than
            // the registered command - no need to compare
            continue;
        }

        if (strncmp(atCmdRegistryPtr->commandStr,
                   commandStr,
                   strlen(atCmdRegistryPtr->commandStr)) == 0)
        {
            LE_DEBUG("AT Command match found [%s]", atCmdRegistryPtr->commandStr);

            // Match found in AT Command Registry
            atCmdPtr->local = true;
            atCmdPtr->registryIndex = i;
            return;
        }
    }

    LE_DEBUG("AT Command match not found!");

    // No Match found in local AT Command Registry
    atCmdPtr->local = false;
}

//--------------------------------------------------------------------------------------------------
/**
 * Queue the unsolicited response
 *
 * @return none
 */
//--------------------------------------------------------------------------------------------------
static void StoreUnsolicitedResponse
(
    const char* responseStr,                       ///< [IN] Unsolicited Response String
    struct le_atProxy_AtCommandSession* atCmdPtr   ///< [INOUT] AT Command Session Pointer
)
{
    if (!responseStr || !atCmdPtr)
    {
        LE_ERROR("Bad parameter!");
        return;
    }

    le_atProxy_RspString_t* rspStringPtr = CreateResponse(responseStr);

    if (!rspStringPtr)
    {
        LE_ERROR("Failed to create URC!");
        return;
    }

    le_dls_Queue(&(atCmdPtr->unsolicitedList), &(rspStringPtr->link));
}

//--------------------------------------------------------------------------------------------------
/**
 * Parse incoming characters
 *
 * @return none
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void atProxyCmdHandler_ParseBuffer
(
    struct le_atProxy_AtCommandSession* atCmdPtr,  ///< [IN] AT Command Session Pointer
    uint32_t count                                 ///< [IN] Number of new characters
)
{
    uint16_t startIndex = atCmdPtr->index;

    for (uint16_t i = startIndex; i < (startIndex + count); i++)
    {
        // New input character to be parsed
        char input = atCmdPtr->command[i];
        LE_DEBUG("Processing input character, [%c], buffer len [%" PRIu16 "]",
                 input,
                 atCmdPtr->index);

        switch (atCmdPtr->rxState)
        {
            case PARSER_SEARCH_A:
                if (( input == 'A' ) || ( input == 'a' ))
                {
                    // Advance receive state and index
                    atCmdPtr->rxState = PARSER_SEARCH_T;
                    atCmdPtr->index++;
                }
            break;
            case PARSER_SEARCH_T:
                if (( input == 'T' ) || ( input == 't' ))
                {
                    // Advance receive state and index
                    atCmdPtr->rxState = PARSER_SEARCH_CR;
                    atCmdPtr->index++;
                }
                else
                {
                    // Something not expected.  Throw-away buffered AT command
                    // and start again
                    atCmdPtr->rxState = PARSER_SEARCH_A;
                    atCmdPtr->index = 0;
                    atCmdPtr->operatorIndex = 0;
                }
            break;
            case PARSER_SEARCH_CR:
            {
                if ( input == AT_TOKEN_CR )
                {
                    atCmdPtr->command[atCmdPtr->index + 1] = 0;

                    if (atCmdPtr->operatorIndex == 0)
                    {
                        // Mark the operator index for the AT Command
                        atCmdPtr->operatorIndex = i;

                        // Set the operation type to 'Action'
                        atCmdPtr->type = LE_ATSERVER_TYPE_ACT;

                        // Try to look for AT Command in the
                        // AT Command Registry
                        SearchAtCmdRegistry(atCmdPtr);
                    }

                    // Process AT Command
                    ProcessAtCmd(atCmdPtr);

                    atCmdPtr->index = 0;
                    atCmdPtr->operatorIndex = 0;
                    atCmdPtr->rxState = PARSER_SEARCH_A;
                }
                // backspace character
                else if ( input == AT_TOKEN_BACKSPACE )
                {
                    if (atCmdPtr->index > 0)
                    {
                        atCmdPtr->index--;
                        if (atCmdPtr->index == atCmdPtr->operatorIndex)
                        {
                            atCmdPtr->operatorIndex = 0;
                        }
                    }
                }
                else
                {
                    atCmdPtr->index++;

                    switch (input)
                    {
                        case AT_TOKEN_EQUAL:
                            if (atCmdPtr->operatorIndex == 0)
                            {
                                // Mark the operator index for the AT Command
                                atCmdPtr->operatorIndex = i;

                                // Set the operation type to 'Parameter'
                                atCmdPtr->type = LE_ATSERVER_TYPE_PARA;

                                // Try to look for AT Command in the
                                // AT Command Registry
                                SearchAtCmdRegistry(atCmdPtr);
                            }
                            break;

                        case AT_TOKEN_QUESTIONMARK:
                            if (atCmdPtr->operatorIndex == 0)
                            {
                                // Mark the operator index for the AT Command
                                atCmdPtr->operatorIndex = i;

                                // Set the operation type to 'Read'
                                atCmdPtr->type = LE_ATSERVER_TYPE_READ;

                                // Try to look for AT Command in the
                                // AT Command Registry
                                SearchAtCmdRegistry(atCmdPtr);
                            }
                            else
                            {
                                // Set the operation type to 'Read'
                                atCmdPtr->type = LE_ATSERVER_TYPE_TEST;
                            }
                        break;

                        default:
                        break;
                    }
                }
            }
            break;
            default:
                LE_ERROR("bad state !!");
            break;
        }
    }

    if (atCmdPtr->index >= LE_ATDEFS_COMMAND_MAX_LEN)
    {
        // Send an error to the Serial UART
        pa_port_Write(
            atCmdPtr->port,
            LE_AT_PROXY_ERROR,
            sizeof(LE_AT_PROXY_ERROR));

        LE_ERROR("AT Command string is too long, maximum supported length is %d",
                 LE_ATDEFS_COMMAND_MAX_LEN);

        // Drop the buffer contents and start again
        atCmdPtr->index = 0;
        atCmdPtr->operatorIndex = 0;
        atCmdPtr->rxState = PARSER_SEARCH_A;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Retrieves the AT Command Session record associated with the specified Command Reference
 *
 * @return le_atProxy_AtCommandSession pointer
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED struct le_atProxy_AtCommandSession*  atProxyCmdHandler_GetAtCommandSession
(
    le_atServer_CmdRef_t commandRef  ///< [IN] AT Command Reference
)
{
    return le_ref_Lookup(atCmdSessionRefMap, commandRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Complete the current AT command session
 *
 * @return none
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void atProxyCmdHandler_Complete
(
    struct le_atProxy_AtCommandSession* atCmdPtr   ///< AT Command Session Pointer
)
{
    atCmdPtr->active = false;
    atCmdPtr->dataMode = false;

    // Process unsolicited message from local
    ProcessStoredURC(atCmdPtr);

    // Process unsolicited messages from remote
    pa_remote_ProcessUnsolicitedMsg();

    if (atCmdPtr->local)
    {
        pa_port_Enable(atCmdPtr->port);
    }
    else
    {
        pa_remote_SignalCmdFinish();
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Start AT command data mode
 *
 * @return none
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void atProxyCmdHandler_StartDataMode
(
    struct le_atProxy_AtCommandSession* atCmdPtr   ///< AT Command Session Pointer
)
{
    // Currently only remote MAP commands support data mode.
    if (!atCmdPtr->local)
    {
        atCmdPtr->dataMode = true;
        pa_remote_SignalCmdFinish();
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if the current session is local and active
 *
 * @return true if current session is local and active, false otherwise
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED bool atProxyCmdHandler_IsLocalSessionActive
(
    struct le_atProxy_AtCommandSession* atCmdPtr   ///< AT Command Session Pointer
)
{
    return atCmdPtr->local && atCmdPtr->active;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if the current session is active
 *
 * @return true if current session is active, false otherwise
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED bool atProxyCmdHandler_IsActive
(
    struct le_atProxy_AtCommandSession* atCmdPtr   ///< AT Command Session Pointer
)
{
    return atCmdPtr->active;
}

//--------------------------------------------------------------------------------------------------
/**
 * Send or queue the unsolicited response
 *
 * @return none
 */
//--------------------------------------------------------------------------------------------------
void atProxyCmdHandler_SendUnsolicitedResponse
(
    const char* responseStr,                       ///< [IN] Unsolicited Response String
    struct le_atProxy_AtCommandSession* atCmdPtr   ///< [IN] AT Command Session Pointer
)
{
    if (!responseStr || !atCmdPtr)
    {
        LE_ERROR("Bad parameter!");
        return;
    }

    // Queue the response and defer outputting it if current AT session is active (in process)
    if (atProxyCmdHandler_IsActive(atCmdPtr))
    {
        StoreUnsolicitedResponse(responseStr, atCmdPtr);
        return;
    }

    pa_port_Write(atCmdPtr->port, (char *)responseStr, strlen(responseStr));
    pa_port_Write(atCmdPtr->port, "\r\n", strlen("\r\n"));
}

//--------------------------------------------------------------------------------------------------
/**
 * Open an AT command session
 *
 * @return Reference to the requested AT command session if successful, NULL otherwise
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_atProxy_AtCommandSession_t* atProxyCmdHandler_OpenSession
(
    le_atProxy_PortRef_t port   ///< [IN] AT port for AT session to be opened
)
{
    le_atProxy_AtCommandSession_t* atSessionPtr = le_mem_Alloc(AtCmdSessionPoolRef);
    if (!atSessionPtr)
    {
        LE_ERROR("Cannot allocate an AT session from pool!");
        return NULL;
    }

    // Initialize the AT Command Sesison record
    memset(atSessionPtr, 0, sizeof(le_atProxy_AtCommandSession_t));

    // Create a Reference to the At Command Session
    atSessionPtr->ref = le_ref_CreateRef(atCmdSessionRefMap, atSessionPtr);

    atSessionPtr->port = port;

    return atSessionPtr;
}

//--------------------------------------------------------------------------------------------------
/**
 * Close an AT command session
 *
 * @return
 *      - LE_OK                Function succeeded.
 *      - LE_BAD_PARAMETER     Invalid parameter.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t atProxyCmdHandler_CloseSession
(
    le_atProxy_AtCommandSession_t* atCmdPtr   ///< [IN] AT Command Session Pointer
)
{
    if (!atCmdPtr)
    {
        LE_ERROR("AT Command Session is NULL");
        return LE_BAD_PARAMETER;
    }

    // Delete the Safe Reference to the AT Command Session
    le_ref_DeleteRef(atCmdSessionRefMap, atCmdPtr->ref);

    // Free the memory for the AT Command Session
    le_mem_Release(atCmdPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Send backed-up unsolicited responses
 *
 * @return
 *      - LE_OK     Successfully flushed URCs out
 *      - LE_FAULT  Otherwise
 */
//--------------------------------------------------------------------------------------------------
le_result_t atProxyCmdHandler_FlushStoredURC
(
    le_atServer_CmdRef_t cmdRef     ///< [IN] AT Command Session Reference
)
{
    le_atProxy_AtCommandSession_t* atCmdSessionPtr =
        le_ref_Lookup(atCmdSessionRefMap, cmdRef);

    return ProcessStoredURC(atCmdSessionPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the AT Proxy Command Handler
 */
//--------------------------------------------------------------------------------------------------
void atProxyCmdHandler_Init(void)
{
    LE_INFO("Starting AT Proxy Command Handler");

    // AT Command Session Reference pool allocation
    atCmdSessionRefMap = le_ref_InitStaticMap(AtCmdSessionRefMap,
                                              LE_CONFIG_ATSERVER_DEVICE_POOL_SIZE);

    // Device pool allocation
    AtCmdSessionPoolRef = le_mem_InitStaticPool(AtCmdSessions,
                                                LE_CONFIG_ATSERVER_DEVICE_POOL_SIZE,
                                                sizeof(le_atProxy_AtCommandSession_t));

    // Initialize memory pools for URC string content
    UnsoliRspDataPoolRef = le_mem_InitStaticPool(
                                    UnsoliRspDataPool,
                                    AT_PROXY_UNSOLICITED_RESPONSE_COUNT_LARGE,
                                    AT_PROXY_UNSOLICITED_RESPONSE_LARGE_BYTES);

    UnsoliRspDataPoolRef = le_mem_CreateReducedPool(
                                    UnsoliRspDataPoolRef,
                                    "TypicalURCPool",
                                    AT_PROXY_UNSOLICITED_RESPONSE_COUNT_TYPICAL,
                                    AT_PROXY_UNSOLICITED_RESPONSE_TYPICAL_BYTES);

    // Initialize memory pool for URC items, all atProxy clients will share this pool for atProxy
    // to store their URCs
    UnsoliRspPoolRef = le_mem_InitStaticPool(
                                    UnsoliRspPool,
                                    AT_PROXY_UNSOLICITED_RESPONSE_COUNT,
                                    sizeof(le_atProxy_RspString_t));
}