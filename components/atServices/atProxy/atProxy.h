/**
 * @file atProxy.h
 *
 * AT Proxy defines and constants.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LE_AT_PROXY_H_INCLUDE_GUARD
#define LE_AT_PROXY_H_INCLUDE_GUARD

#include "interfaces.h"

// Final response.
#define LE_AT_PROXY_ERROR       "\r\nERROR\r\n"
#define LE_AT_PROXY_OK          "\r\nOK\r\n"
#define LE_AT_PROXY_NO_CARRIER  "\r\nNO CARRIER\r\n"
#define LE_AT_PROXY_NO_DIALTONE "\r\nNO DIALTONE\r\n"
#define LE_AT_PROXY_BUSY        "\r\nBUSY\r\n"
#define LE_AT_PROXY_NO_ANSWER   "\r\nNO ANSWER\r\n"
#define LE_AT_PROXY_CME_ERROR   "+CME ERROR: "
#define LE_AT_PROXY_CMS_ERROR   "+CMS ERROR: "

// Intermediate response.
#define LE_AT_PROXY_CONNECT     "\r\nCONNECT\r\n"

// Static registration entry.
struct le_atProxy_StaticCommand
{
    const char *commandStr;
    le_atServer_CommandHandlerFunc_t commandHandlerPtr;
    void* contextPtr;
};


// Function to retrieve the AT Command Registry
struct le_atProxy_StaticCommand* le_atProxy_GetCmdRegistry(void);

// Function to retrieve the AT Command Registry entry for a specific command
struct le_atProxy_StaticCommand* le_atProxy_GetCmdRegistryEntry(uint32_t command);


#endif /* LE_AT_PROXY_H_INCLUDE_GUARD */
