/**
 * @page c_le_at AT Client
 *
 * This module provides API for the AT Client.
 *
 *
 * The following functions allow the User to prepare and send an AT command:
 *
 * le_atClient_Create() creates the command reference.
 * le_atClient_SetCommand() set the AT command to send (ex: "AT+CREG?").
 * le_atClient_SetIntermediateResponse() set the expected Intermediate Response(s) separated with '|' character. (ex: "+CREG:").
 * le_atClient_SetFinalResponse() set the expected Final Response(s) separated with '|' character. (ex: "OK|ERROR|+CME ERROR:").
 * le_atClient_SetData() set specific data information (contents of an SMS for example).
 * le_atClient_SetTimeout() set the time out in ms.
 * le_atClient_Send() sends the command to the AT modem.
 *
 * Example:
 * @code

    const char* commandPtr   = "AT+CFUN?";
    const char* interRespPtr = "+CFUN:";
    const char* respPtr      = "OK|ERROR|+CME ERROR:";

    le_atClient_CmdRef_t cmdRef = le_atClient_Create();
    LE_DEBUG("New command ref (%p) created",cmdRef);

    le_atClient_SetCommand(cmdRef,commandPtr);
    le_atClient_SetIntermediateResponse(cmdRef,interRespPtr);
    le_atClient_SetFinalResponse(cmdRef,respPtr);
    le_atClient_Send(cmdRef);

 * @endcode
 *
 *
 * Modem Response:
 *
 * le_atClient_GetFirstIntermediateResponse() retrieves the first intermediate response.
 *
 * le_atClient_GetNextIntermediateResponse() retrieves the next intermediate response.
 *
 * le_atClient_GetFinalResponse() retrieves the final response (generally "OK").
 *
 *
 * More usefull functions :
 * le_atClient_SetCommandAndSend() allows to send an AT Command with just one function.
 *
 * Example:
 * @code

    le_atClient_CmdRef_t cmdRef = NULL;
    le_atClient_SetCommandAndSend(&cmdRef, "AT+COPS?", "+COPS:", "OK|ERROR|+CME ERROR:", 30000);

 * @endcode
 *
 */

/** @file le_atClient.h
 *
 * Legato @ref c_le_at include file.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#define LEGATO_AT_CLIENT_INCLUDE_GUARD

#include "legato.h"

/**
 * Maximum number of bytes in an AT Command (not including the null-terminator).
 */
#define LE_ATCLIENT_CMD_SIZE_MAX_LEN            63

/**
 * Maximum number of bytes in an AT Command (including the null-terminator).
 */
#define LE_ATCLIENT_CMD_SIZE_MAX_BYTES          (LE_ATCLIENT_CMD_SIZE_MAX_LEN+1)


/**
 * Maximum number of bytes in an AT Command Response (not including the null-terminator).
 */
#define LE_ATCLIENT_RESPLINE_SIZE_MAX_LEN       511

/**
 * Maximum number of bytes in an AT Command Response (including the null-terminator).
 */
#define LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES     (LE_ATCLIENT_RESPLINE_SIZE_MAX_LEN+1)


/**
 * Define the different ports available to send data
 */
typedef enum
{
    LE_ATCLIENT_PORT_COMMAND, ///< Port where AT Command must be send
    LE_ATCLIENT_PORT_PPP,     ///< Port that will be used for PPP/Data connection
    LE_ATCLIENT_PORT_MAX      ///< Do not use this one
}
le_atClient_Ports_t;

//--------------------------------------------------------------------------------------------------
/**
 * Reference on an AT Command
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_atClient_cmd* le_atClient_CmdRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to create a new AT command.
 *
 * @return pointer to the new AT Command reference
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_atClient_CmdRef_t le_atClient_Create
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to delete an AT command reference.
 *
 * @return
 *      - LE_FAULT when function failed
 *      - LE_NOT_FOUND when Invalid reference
 *      - LE_OK when function succeed
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void le_atClient_Delete
(
    le_atClient_CmdRef_t     cmdRef       ///< [IN] AT Command
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the AT command string to be send.
 *
 * @return
 *      - LE_FAULT when function failed
 *      - LE_NOT_FOUND when Invalid reference
 *      - LE_OK when function succeed
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_atClient_SetCommand
(
    le_atClient_CmdRef_t     cmdRef,      ///< [IN] AT Command
    const char*              commandPtr   ///< [IN] Set Command
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the waiting intermediate responses.
 * Several intermediate responses can be specified separated by a '|' character into the string given in parameter.
 *
 * @return
 *      - LE_FAULT when function failed
 *      - LE_NOT_FOUND when Invalid reference
 *      - LE_OK when function succeed
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_atClient_SetIntermediateResponse
(
    le_atClient_CmdRef_t     cmdRef,          ///< [IN] AT Command
    const char*              intermediatePtr  ///< [IN] Set Intermediate
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the final response(s) of the AT command execution.
 * Several final responses can be specified separated by a '|' character into the string given in parameter.
 *
 * @return
 *      - LE_FAULT when function failed
 *      - LE_NOT_FOUND when Invalid reference
 *      - LE_OK when function succeed
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_atClient_SetFinalResponse
(
    le_atClient_CmdRef_t     cmdRef,         ///< [IN] AT Command
    const char*              responsePtr     ///< [IN] Set Response
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the data when the prompt is expected.
 *
 * @return
 *      - LE_FAULT when function failed
 *      - LE_NOT_FOUND when Invalid reference
 *      - LE_OK when function succeed
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_atClient_SetData
(
    le_atClient_CmdRef_t     cmdRef,      ///< [IN] AT Command
    const char*              dataPtr,     ///< [IN] The AT Data to send
    uint32_t                 dataSize     ///< [IN] Size of AT Data to send
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the timeout of the AT command execution.
 *
 * @return
 *      - LE_FAULT when function failed
 *      - LE_NOT_FOUND when Invalid reference
 *      - LE_OK when function succeed
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_atClient_SetTimeout
(
    le_atClient_CmdRef_t     cmdRef,        ///< [IN] AT Command
    uint32_t                 timer          ///< [IN] Set Timer
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the port used for the AT command.
 *
 * @return
 *      - LE_FAULT when function failed
 *      - LE_NOT_FOUND when Invalid reference
 *      - LE_OK when function succeed
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_atClient_SetPort
(
    le_atClient_CmdRef_t    cmdRef,        ///< [IN] AT Command
    le_atClient_Ports_t     port           ///< [IN] Set port
);


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to send an AT Command and wait for response.
 *
 * @return
 *      - LE_FAULT when function failed
 *      - LE_NOT_FOUND when Invalid reference
 *      - LE_TIMEOUT when a timeout occur
 *      - LE_OK when function succeed
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_atClient_Send
(
    le_atClient_CmdRef_t     cmdRef         ///< [IN] AT Command
);

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to get the first intermediate response
 *
 * @return
 *      - LE_FAULT when function failed
 *      - LE_NOT_FOUND when Invalid reference
 *      - LE_OK when function succeed
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_atClient_GetFirstIntermediateResponse
(
    le_atClient_CmdRef_t        cmdRef,                         ///< [IN] AT Command
    char*                       intermediateRspPtr,             ///< [OUT] Get Next Line
    size_t                      intermediateRspNumElements      ///< [IN] Size of response
);

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to get the intermediate response
 *
 * @return
 *      - LE_FAULT when function failed
 *      - LE_NOT_FOUND when Invalid reference
 *      - LE_OK when function succeed
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_atClient_GetNextIntermediateResponse
(
    le_atClient_CmdRef_t        cmdRef,                         ///< [IN] AT Command
    char*                       intermediateRspPtr,             ///< [OUT] Get Next Line
    size_t                      intermediateRspNumElements      ///< [IN] Size of response
);

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to get the final response
 *
 * @return
 *      - LE_FAULT when function failed
 *      - LE_NOT_FOUND when Invalid reference
 *      - LE_OK when function succeed
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_atClient_GetFinalResponse
(
    le_atClient_CmdRef_t          cmdRef,                   ///< [IN] AT Command
    char*                         finalRspPtr,              ///< [OUT] Get Final Line
    size_t                        finalRspNumElements       ///< [IN] Size of response
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to automatically set and send an AT Command.
 *
 * @return
 *      - LE_FAULT when function failed
 *      - LE_NOT_FOUND when Invalid reference
 *      - LE_TIMEOUT when a timeout occur
 *      - LE_OK when function succeed
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_atClient_SetCommandAndSend
(
    le_atClient_CmdRef_t*               cmdRefPtr,        ///< [OUT] Command reference
    const char*                         commandPtr,       ///< [IN] AT Command
    const char*                         interRespPtr,     ///< [IN] Expected Intermediate Response
    const char*                         finalRespPtr,     ///< [IN] Expected Final Response
    uint32_t                            timeout           ///< [IN] Timeout
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to Set an unsolicited pattern to match.
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void le_atClient_AddUnsolicitedResponseHandler
(
    le_event_Id_t               unsolicitedReportId,        ///< [IN] Event Id to report to
    char*                       unsolRspPtr,                ///< [IN] Pattern to match
    bool                        withExtraData               ///< [IN] Indicate if the unsolicited has more than one line
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to remove an unsolicited pattern to match.
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void le_atClient_RemoveUnsolicitedResponseHandler
(
    le_event_Id_t               unsolicitedReportId,        ///< [IN] Event Id to report to
    char*                       unsolRspPtr                 ///< [IN] Pattern to match
);
