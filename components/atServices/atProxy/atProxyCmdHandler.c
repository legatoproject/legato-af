/**
 * @file atProxyCmdHandler.c
 *
 * AT Proxy Command Handler implementation.   Responible for
 * 1) Parse the STDIN/console to identify incoming AT commands,
 * 2) Create and manage the AT Command session for the tracking AT command being processed,
 * 3) Trigger the IPC Command Handler callback function associated with the AT command
 *    to notify the local Back-end that an AT command has arrived.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"
#include "atProxy.h"
#include "atProxySerialUart.h"
#include "atProxyCmdHandler.h"
#include "atProxyCmdRegistry.h"
#include "atProxyRemote.h"


//--------------------------------------------------------------------------------------------------
/**
 * AT parser tokens
 */
//--------------------------------------------------------------------------------------------------
#define AT_TOKEN_EQUAL  '='
#define AT_TOKEN_CR  0x0D
#define AT_TOKEN_BACKSPACE  0x08
#define AT_TOKEN_QUESTIONMARK  '?'
#define AT_TOKEN_SEMICOLON  ';'
#define AT_TOKEN_COMMA  ','
#define AT_TOKEN_QUOTE  0x22
#define AT_TOKEN_BACKSLASH  0x5C
#define AT_TOKEN_SPACE  0x20


#define AT_PROXY_PARAMETER_NONE      -1


//--------------------------------------------------------------------------------------------------
/**
 * Static map for AT Command Session references
 */
//--------------------------------------------------------------------------------------------------
LE_REF_DEFINE_STATIC_MAP(AtCmdSessionRefMap, 1);

//--------------------------------------------------------------------------------------------------
/**
 * Map for AT Command Sessions
 */
//--------------------------------------------------------------------------------------------------
le_ref_MapRef_t  atCmdSessionRefMap;


// Static AT Command Session
static struct le_atProxy_AtCommandSession  AtCmd;


// AT Command Session Reference
static void* AtCmdRef = NULL;

// Semephore to allow to receive input from AT port
static le_sem_Ref_t AtSem = NULL;

// AT Echo Command Mode (Global)
static bool AtEchoMode = true;

//--------------------------------------------------------------------------------------------------
/**
 * Command responses pool size
 */
//--------------------------------------------------------------------------------------------------
#define UNSOLICITED_RSP_COUNT       2

//--------------------------------------------------------------------------------------------------
/**
 * AT command response structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_atProxy_RspString
{
    le_dls_Link_t   link;               ///< link for list
    le_atServer_ServerCmdRef_t cmdRef;   ///< Asynchronous Server Command Reference
    const char* resp;                   ///< string pointer
} le_atProxy_RspString_t;

//--------------------------------------------------------------------------------------------------
/**
 * Static pool for response
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(UnsoliRspPool,
                          UNSOLICITED_RSP_COUNT,
                          sizeof(le_atProxy_RspString_t));

//--------------------------------------------------------------------------------------------------
/**
 * Pool for response
 * w string
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t  UnsoliRspPoolRef;

//--------------------------------------------------------------------------------------------------
/**
 * Send backed-up unsolicited responses
 *
 */
//--------------------------------------------------------------------------------------------------
static void ProcessStoredURC
(
    struct le_atProxy_AtCommandSession* atSession      ///< [IN] AT Command Session Pointer
)
{
    le_dls_Link_t* linkPtr;

    if (!atSession)
    {
        LE_ERROR("Bad parameter!");
        return;
    }

    while ( (linkPtr = le_dls_Pop(&atSession->unsolicitedList)) != NULL )
    {
        le_atProxy_RspString_t* rspStringPtr = CONTAINER_OF(linkPtr,
                                                            le_atProxy_RspString_t,
                                                            link);

        le_atServer_SendUnsolicitedResponse(rspStringPtr->cmdRef,
                                            rspStringPtr->resp,
                                            LE_ATSERVER_ALL_DEVICES,
                                            NULL);

        le_mem_Release(rspStringPtr);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Allocate and initialize a new unsolicited response structure
 */
//--------------------------------------------------------------------------------------------------
static le_atProxy_RspString_t* CreateResponse
(
    const char* rspStr                  ///< [IN] Unsolicited Response String
)
{
    le_atProxy_RspString_t* rspStringPtr = le_mem_Alloc(UnsoliRspPoolRef);

    memset(rspStringPtr, 0, sizeof(le_atProxy_RspString_t));
    rspStringPtr->resp = rspStr;

    return rspStringPtr;
}

//--------------------------------------------------------------------------------------------------
/**
 * Packs an AT Command parameter string into the Parameter List array
 */
//--------------------------------------------------------------------------------------------------
static le_result_t PackParameterList
(
    struct le_atProxy_AtCommandSession* atCmdPtr,
    char* parameters,
    uint16_t startIndex,
    uint16_t endIndex
)
{
    uint16_t parameterLength = (endIndex - startIndex);

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
                                     i);

                        if (result != LE_OK)
                        {
                            return result;
                        }

                        // Reset the startIndex
                        startIndex = AT_PROXY_PARAMETER_NONE;
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
                                 i);

                    if (result != LE_OK)
                    {
                        return result;
                    }

                    // Reset the startIndex
                    startIndex = AT_PROXY_PARAMETER_NONE;
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
    struct le_atProxy_AtCommandSession* atCmdPtr   ///< AT Command Session Pointer
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
            atProxySerialUart_write(
                LE_AT_PROXY_ERROR,
                sizeof(LE_AT_PROXY_ERROR));

            LE_ERROR("Error parsing parameter list, result [%d]", result);
            return;
        }

        if (atCmdRegistryPtr[atCmdPtr->registryIndex].commandHandlerPtr != NULL)
        {
            atCmdPtr->active = true;
            atProxySerialUart_write("\r\n", strlen("\r\n"));
            atProxySerialUart_disable();

            // Trigger the AT Command Handler callback registered for this "local"
            // AT Command
            atCmdRegistryPtr[atCmdPtr->registryIndex].commandHandlerPtr(
                AtCmdRef,  // Command Reference
                atCmdPtr->type,    // Type
                atCmdPtr->parameterIndex,   // Number of parameters
                atCmdRegistryPtr->contextPtr);  // Callback context pointer
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
        atProxySerialUart_disable();

        result = atProxyRemote_send(atCmdPtr->command,
                                  strnlen(atCmdPtr->command, LE_ATDEFS_COMMAND_MAX_BYTES));

        if (LE_OK != result)
        {
            LE_ERROR("Failed sending command to MAP!");
            atProxySerialUart_enable();
            return;
        }

        le_sem_Wait(AtSem);
        atProxySerialUart_enable();
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
    struct le_atProxy_AtCommandSession* atCmdPtr   ///< AT Command Session Pointer
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
 * Parse incoming characters
 */
//--------------------------------------------------------------------------------------------------
static void ParseBuffer
(
    struct le_atProxy_AtCommandSession* atCmdPtr,  ///< AT Command Session Pointer
    uint32_t count                                 ///< Number of new characters
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
        atProxySerialUart_write(
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
 * Callback registered to fd monitor that gets called whenever there's an event on the fd
 */
//--------------------------------------------------------------------------------------------------
void atProxyCmdHandler_AsyncRecvHandler
(
    int handle,             ///< [IN] the fd that this callback is being called for
    short events            ///< [IN] events that has happened on this fd
)
{
    LE_DEBUG("Handle provided to fd monitor got called, fd [%d]", (intptr_t) handle);
    if (handle == -1)
    {
        LE_ERROR("Invalid serial handle fd");
        return;
    }

    if (events & POLLIN)
    {
        // Wait until a character arrives on the AT Port to be read
        uint32_t count = atProxySerialUart_read(&AtCmd.command[AtCmd.index], 1);
        while (count > 0)
        {
            // Only display incoming characters from the AT Port if
            // Echo Command mode is enabled
            if (AtEchoMode)
            {
                if (AtCmd.command[AtCmd.index] != AT_TOKEN_CR)
                {
                    atProxySerialUart_write(&AtCmd.command[AtCmd.index], 1);
                }
            }

            if (AtCmd.dataMode)
            {
                atProxyRemote_send(&AtCmd.command[AtCmd.index], 1);
            }
            else
            {
                // Parse the incoming string (character)
                ParseBuffer(&AtCmd, count);
            }

            // Read the next character if available
            count = atProxySerialUart_read(&AtCmd.command[AtCmd.index], 1);
        }
    }

}

//--------------------------------------------------------------------------------------------------
/**
 * ATE0 Handler
 */
//--------------------------------------------------------------------------------------------------
static void atProxyCmdHandler_echoModeDisable
(
    le_atServer_CmdRef_t commandRef,        ///< [IN] AT Command Reference
    le_atServer_Type_t   type,              ///< [IN] AT Command Type
    uint32_t             parametersNumber,  ///< [IN] Number of parameters
    void                *contextPtr         ///< [IN] Context pointer
)
{
    LE_UNUSED(contextPtr);
    LE_UNUSED(parametersNumber);

    struct le_atProxy_AtCommandSession* atCmdSessionPtr =
        le_ref_Lookup(atCmdSessionRefMap, commandRef);

    // Verify AT Command Session pointer is valid
    if (atCmdSessionPtr == NULL)
    {
        LE_ERROR("AT Command Session reference pointer is NULL");
        atProxySerialUart_write(LE_AT_PROXY_ERROR, strlen(LE_AT_PROXY_ERROR));
    }
    else if (type != LE_ATSERVER_TYPE_ACT)
    {
        LE_DEBUG("Unsupported command type %d", type);
        atProxySerialUart_write(LE_AT_PROXY_ERROR, strlen(LE_AT_PROXY_ERROR));
    }
    else
    {
        // Disable AT Echo Command mode
        AtEchoMode = false;

        LE_INFO("Setting AtEchoMode OFF");

        atProxySerialUart_write(LE_AT_PROXY_OK, strlen(LE_AT_PROXY_OK));
    }

    // After sending out final response, set current AT session to complete
    atProxyCmdHandler_complete();
}


//--------------------------------------------------------------------------------------------------
/**
 * ATE1 Handler
 */
//--------------------------------------------------------------------------------------------------
static void atProxyCmdHandler_echoModeEnable
(
    le_atServer_CmdRef_t commandRef,        ///< [IN] AT Command Reference
    le_atServer_Type_t   type,              ///< [IN] AT Command Type
    uint32_t             parametersNumber,  ///< [IN] Number of parameters
    void                *contextPtr         ///< [IN] Context pointer
)
{
    LE_UNUSED(contextPtr);
    LE_UNUSED(parametersNumber);

    struct le_atProxy_AtCommandSession* atCmdSessionPtr =
        le_ref_Lookup(atCmdSessionRefMap, commandRef);

    // Verify AT Command Session pointer is valid
    if (atCmdSessionPtr == NULL)
    {
        LE_ERROR("AT Command Session reference pointer is NULL");
        atProxySerialUart_write(LE_AT_PROXY_ERROR, strlen(LE_AT_PROXY_ERROR));
    }
    else if (type != LE_ATSERVER_TYPE_ACT)
    {
        LE_DEBUG("Unsupported command type %d", type);
        atProxySerialUart_write(LE_AT_PROXY_ERROR, strlen(LE_AT_PROXY_ERROR));
    }
    else
    {
        // Enable AT Echo Command mode
        AtEchoMode = true;

        LE_INFO("Setting AtEchoMode ON");

        atProxySerialUart_write(LE_AT_PROXY_OK, strlen(LE_AT_PROXY_OK));
    }

    // After sending out final response, set current AT session to complete
    atProxyCmdHandler_complete();
}


//--------------------------------------------------------------------------------------------------
/**
 * Initialize the AT Proxy Command Handler
 */
//--------------------------------------------------------------------------------------------------
void atProxyCmdHandler_init(void)
{
    LE_INFO("Starting AT Proxy Command Handler");

    // AT Command Session Reference pool allocation
    atCmdSessionRefMap = le_ref_InitStaticMap(AtCmdSessionRefMap, 1);

    // Create a Reference to the At Command Session
    AtCmdRef = le_ref_CreateRef(atCmdSessionRefMap, &AtCmd);


    // Initialize the AT Command Sesison record
    memset(&AtCmd, 0, sizeof(AtCmd));

    // Parameters pool allocation
    // Typically, we only need cache one unsolicited response for one AT backend (such as ORP),
    // but we use memory pool here in case there are multiple AT backends.
    UnsoliRspPoolRef = le_mem_InitStaticPool(
                                    UnsoliRspPool,
                                    UNSOLICITED_RSP_COUNT,
                                    sizeof(le_atProxy_RspString_t));

    AtSem = le_sem_Create("AtSem", 0);


    // Set pointer to AT Command Register entry for 'ATE0' Command
    struct le_atProxy_StaticCommand* atCmdRegistryPtr = le_atProxy_GetCmdRegistryEntry(AT_CMD_ATE0);

    // Set the Command Handler Callback function
    atCmdRegistryPtr->commandHandlerPtr = atProxyCmdHandler_echoModeDisable;

    // Set pointer to AT Command Register entry for 'ATE1' Command
    atCmdRegistryPtr = le_atProxy_GetCmdRegistryEntry(AT_CMD_ATE1);

    // Set the Command Handler Callback function
    atCmdRegistryPtr->commandHandlerPtr = atProxyCmdHandler_echoModeEnable;
}

//--------------------------------------------------------------------------------------------------
/**
 * Complete the current AT command session
 *
 * @return none
 */
//--------------------------------------------------------------------------------------------------
void atProxyCmdHandler_complete
(
    void
)
{
    AtCmd.active = false;
    AtCmd.dataMode = false;

    // Process unsolicited message from local
    ProcessStoredURC(&AtCmd);

#ifndef MK_CONFIG_ATSERVER_LITE
    // Process unsolicited messages from remote
    atProxyRemote_processUnsolicitedMsg();
#endif

    if (AtCmd.local)
    {
        atProxySerialUart_enable();
    }
    else
    {
        le_sem_Post(AtSem);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Start AT command data mode
 *
 * @return none
 */
//--------------------------------------------------------------------------------------------------
void atProxyCmdHandler_startDataMode
(
    void
)
{
    // Currently only remote MAP commands support data mode.
    if (!AtCmd.local)
    {
        AtCmd.dataMode = true;
        le_sem_Post(AtSem);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if the current session is local and active
 *
 * @return true if current session is local and active, false otherwise
 */
//--------------------------------------------------------------------------------------------------
bool atProxyCmdHandler_isLocalSessionActive
(
    void
)
{
    return AtCmd.local && AtCmd.active;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if the current session is active
 *
 * @return true if current session is active, false otherwise
 */
//--------------------------------------------------------------------------------------------------
bool atProxyCmdHandler_isActive
(
    void
)
{
    return AtCmd.active;
}

//--------------------------------------------------------------------------------------------------
/**
 * Queue the unsolicited response
 *
 * @return none
 */
//--------------------------------------------------------------------------------------------------
void atProxyCmdHandler_StoreUnsolicitedResponse
(
    le_atServer_ServerCmdRef_t cmdRef,   ///< [IN] Asynchronous Server Command Reference
    const char* responseStr             ///< [IN] Unsolicited Response String
)
{
    if (!cmdRef || !responseStr)
    {
        LE_ERROR("Bad parameter!");
        return;
    }

    le_atProxy_RspString_t* rspStringPtr = CreateResponse(responseStr);

    rspStringPtr->cmdRef = cmdRef;
    le_dls_Queue(&(AtCmd.unsolicitedList), &(rspStringPtr->link));
}
