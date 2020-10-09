/**
 * @file atProxy.h
 *
 * AT Proxy defines and constants.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LE_AT_PROXY_H_INCLUDE_GUARD
#define LE_AT_PROXY_H_INCLUDE_GUARD

#define LE_AT_PROXY_ERROR       "\r\nERROR\r\n"
#define LE_AT_PROXY_OK          "\r\nOK\r\n"
#define LE_AT_PROXY_NO_CARRIER  "\r\nNO CARRIER\r\n"
#define LE_AT_PROXY_NO_DIALTONE "\r\nNO DIALTONE\r\n"
#define LE_AT_PROXY_BUSY        "\r\nBUSY\r\n"

// Static registration entry.
struct le_atProxy_StaticCommand
{
    const char *commandStr;
    le_atProxy_CommandHandlerFunc_t commandHandlerPtr;
    void* contextPtr;
};


// Function to retrieve the AT Command Registry
struct le_atProxy_StaticCommand* le_atProxy_GetCmdRegistry(void);

#define AT_PROXY_PARAMETER_LIST_MAX   5   ///< Maximum number of parameters supported per AT Cmd

#endif /* LE_AT_PROXY_H_INCLUDE_GUARD */
