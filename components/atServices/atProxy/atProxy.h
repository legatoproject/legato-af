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
#include "pa_port.h"

// Final response.
#define LE_AT_PROXY_ERROR       "\r\nERROR\r\n"
#define LE_AT_PROXY_OK          "\r\nOK\r\n"
#define LE_AT_PROXY_NO_CARRIER  "\r\nNO CARRIER\r\n"
#define LE_AT_PROXY_NO_DIALTONE "\r\nNO DIALTONE\r\n"
#define LE_AT_PROXY_BUSY        "\r\nBUSY\r\n"
#define LE_AT_PROXY_NO_ANSWER   "\r\nNO ANSWER\r\n"
#define LE_AT_PROXY_CME_ERROR   "+CME ERROR: "
#define LE_AT_PROXY_CMS_ERROR   "+CMS ERROR: "

#define LE_AT_PROXY_CME_ERROR_RESP "\r\n+CME ERROR: "
#define LE_AT_PROXY_CMS_ERROR_RESP "\r\n+CMS ERROR: "

// Intermediate response.
#define LE_AT_PROXY_CONNECT     "\r\nCONNECT\r\n"

// Commonly used CME Error Codes
#define LE_AT_PROXY_CME_ERROR_OPER_NOT_ALLOWED     3
#define LE_AT_PROXY_CME_ERROR_OPER_NOT_SUPPORTED   4


// Bit-mask for Static Commands
#define LE_AT_PROXY_CMD_FLAG_NONE        0x00
#define LE_AT_PROXY_CMD_FLAG_CONDENSED   0x01


//--------------------------------------------------------------------------------------------------
/**
 * Error codes modes enum
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    MODE_DISABLED,          ///< Disable extended error code
    MODE_EXTENDED,          ///< Enable extended error code
    MODE_VERBOSE            ///< Enable verbose error details
}
ErrorCodesMode_t;

// Static registration entry.
struct le_atProxy_StaticCommand
{
    const char *commandStr;
    le_atServer_CommandHandlerFunc_t commandHandlerPtr;
    void* contextPtr;
    uint8_t flags;
};


//--------------------------------------------------------------------------------------------------
/**
 * Function to retrieve the AT Command Registry
 *
 * @return none
 */
//--------------------------------------------------------------------------------------------------
struct le_atProxy_StaticCommand* atProxy_GetCmdRegistry
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Function to retrieve the AT Command Registry entry for a specific command
 *
 * @return Pointer of the command entry in registry
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED struct le_atProxy_StaticCommand* atProxy_GetCmdRegistryEntry
(
    uint32_t command        ///< [IN] Index of AT command in Registry
);

//--------------------------------------------------------------------------------------------------
/**
 * Helper function to generate and send the final result code.
 *
 * @return none
 */
//--------------------------------------------------------------------------------------------------
void atProxy_SendFinalResultCode
(
    le_atProxy_PortRef_t portRef,       ///< [IN] Port Reference
    uint32_t errorCode,                 ///< [IN] CME Error Code
    ErrorCodesMode_t errorCodeMode,     ///< [IN] Error Code Mode
    le_atServer_FinalRsp_t finalResult, ///< [IN] Final Response Result
    const char* pattern,                ///< [IN] Pattern string indicating type of verbose error
    size_t patternLen                   ///< [IN] Pattern string length
);

#endif /* LE_AT_PROXY_H_INCLUDE_GUARD */
