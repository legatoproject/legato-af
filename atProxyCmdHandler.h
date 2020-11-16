/**
 * @file atProxyCmdHandler.h
 *
 * AT Proxy Command Handling interface.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LE_AT_PROXY_CMD_HANDLER_H_INCLUDE_GUARD
#define LE_AT_PROXY_CMD_HANDLER_H_INCLUDE_GUARD

#include "atProxy.h"
typedef enum
{
    PARSER_SEARCH_A,
    PARSER_SEARCH_T,
    PARSER_SEARCH_CR

} le_atProxy_RxParserState_t;

// AT Proxy Command Session structure
struct le_atProxy_AtCommandSession
{
    char command[LE_ATDEFS_COMMAND_MAX_BYTES];  ///< cmd found in input string
    le_atProxy_RxParserState_t rxState;       ///< input string parser state
    uint16_t index;           ///< Parse Buffer index
    uint16_t operatorIndex;   ///< Index of operator ("=" or "?")
    le_atProxy_Type_t type;   ///< AT Command type (i.e. Action, parameter, read, or test)
    bool local;               ///< Indicates if this is a "local" or "remote" AT Command
    uint32_t registryIndex;   ///< For "local" commands, index of AT Cmd in Registry
    char atCmdParameterList[AT_PROXY_PARAMETER_LIST_MAX][LE_ATDEFS_PARAMETER_MAX_BYTES];
                              ///< Parameter list
    uint32_t parameterIndex;  ///< Parameter index (count)
    bool active;              ///< Indicates if this session is active (i.e., in processing)
    bool dataMode;            ///< Indicates if current session is in data mode
    le_dls_List_t unsolicitedList;           ///< unsolicited list to be sent
};

//--------------------------------------------------------------------------------------------------
/**
 * Callback registered to fd monitor that gets called whenever there's an event on the fd
 */
//--------------------------------------------------------------------------------------------------
void atProxyCmdHandler_AsyncRecvHandler
(
    int handle,             ///< [IN] the fd that this callback is being called for
    short events            ///< [IN] events that has happened on this fd
);

// Initialize the AT Port Command Handler
void atProxyCmdHandler_init(void);

// Complete the current AT command session
void atProxyCmdHandler_complete(void);

// Start AT command data mode
void atProxyCmdHandler_startDataMode(void);

// Check if the current session is local and active
bool atProxyCmdHandler_isLocalSessionActive(void);

// Check if the current session is active
bool atProxyCmdHandler_isActive(void);

//--------------------------------------------------------------------------------------------------
/**
 * Queue the unsolicited response
 *
 * @return none
 */
//--------------------------------------------------------------------------------------------------
void atProxyCmdHandler_StoreUnsolicitedResponse
(
    le_atProxy_ServerCmdRef_t cmdRef,   ///< [IN] Asynchronous Server Command Reference
    const char* responseStr             ///< [IN] Unsolicited Response String
);

#endif /* LE_AT_PROXY_CMD_HANDLER_H_INCLUDE_GUARD */
