/**
 * @page c_atcmdsync AT Commands Synchronisation
 *
 * This module provide an API that do blocking call on an @ref atmgr_Ref_t
 *
 * @section atcmdsync_toc Table of Contents
 *
 * - @ref atcmdsync_intro
 * - @ref atcmdsync_rational
 *      - @ref atcmdsync_commandLayer
 *      - @ref atcmdsync_response
 *      - @ref atcmdsync_morefunction
 *
 * @section atcmdsync_intro Introduction
 *
 * This is a layer on the @ref c_atcmd and @ref c_atmgr module.
 *
 * @section atcmdsync_rational Rational
 *
 * This provide one usefull API @ref atcmdsync_SendCommand that is a synchronous call of
 * @ref atmgr_SendCommandRequest
 *
 * @subsection atcmdsync_commandLayer Layer for AT Command
 *
 *  - @ref atcmdsync_GetIntermediateEventId
 *  - @ref atcmdsync_GetFinalEventId
 *  - @ref atcmdsync_GetTimerExpiryHandler
 *
 * These API are here to use them when preparing a command with @ref atcmd_AddIntermediateResp,
 * @ref atcmd_AddFinalResp, @ref atcmd_SetTimer
 *
 * Example:
 * @code

 const char* intermediatePatternPtr[] = {NULL};
 const char* finalPatternPtr[] = {"OK","ERROR","+CME ERROR:","+CMS ERROR:","TIMEOUT",NULL};

 atcmd_Ref_t atReqRef = atcmd_Create();

 atcmd_AddCommand(atReqRef,"ATE0",false); // No need of extra data
 atcmd_AddIntermediateResp(atReqRef,atcmdsync_GetIntermediateEventId(),intermediatePatternPtr);
 atcmd_AddFinalResp(atReqRef,atcmdsync_GetFinalEventId(),finalPatternPtr);
 atcmd_SetTimer(atReqRef,30,atcmdsync_GetTimerExpiryHandler());
 atcmd_AddData(atReqRef,NULL,0);    // No data

 * @endcode
 *
 * After this preparation you can call @ref atcmdsync_SendCommand.
 *
 * Example:
 *
 * @code

 atcmdsync_ResultRef_t* resPtr = atcmdsync_SendCommand(interfacePtr,atReqRef);

 * @endcode
 *
 * @subsection atcmdsync_response Modem Response
 *
 * When using @ref atmgr_SendCommandRequest you will get a response of the modem into
 * @ref atcmdsync_ResultRef_t
 *
 * This structure can be read with these API:
 *  - @ref atcmdsync_GetNumLines
 *
 * This function provide the number of line into the response.
 *
 *  - @ref atcmdsync_GetFinalLine
 *
 * This function provide the line that make the @ref atcmdsync_SendCommand finishing.
 *
 *  - @ref atcmdsync_GetLine
 *
 * This function provide one line.
 *
 * Example:
 *
 * @code

 char* lineCount = atcmdsync_GetNumLines(resPtr);

 char *lastLine = atcmdsync_GetFinalLine(resPtr);

 char *firstline = atcmdsync_GetLine(resPtr,0);

 * @endcode
 *
 * Now that you have a line, you can parse it with the @ref atcmd_CountLineParameter,
 * @ref atcmd_GetLineParameter, @ref atcmd_CopyStringWithoutQuote, or any other you may create
 *
 * @subsection atcmdsync_morefunction More usefull functions
 *
 * - @ref atcmdsync_PrepareString
 * - @ref atcmdsync_CheckCommandResult
 * - @ref atcmdsync_PrepareStandardCommand
 * - @ref atcmdsync_SendStandard
 *
 */

/** @file atCmdSync.h
 *
 * Legato @ref c_atcmdsync include file.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#ifndef LEGATO_ATCMDSYNC_INCLUDE_GUARD
#define LEGATO_ATCMDSYNC_INCLUDE_GUARD


#include "legato.h"

#include "atMgr.h"

#define PA_AT_BUFFER_SIZE   2048

#define ATCMDSENDER_LINE        512

//--------------------------------------------------------------------------------------------------
/**
 * This will be the reference for the response of the modem.
 */
//--------------------------------------------------------------------------------------------------
typedef struct atcmdsync_Result* atcmdsync_ResultRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * This function Initialize the platform adapter layer.
 * Must be call only once
 *
 * @return LE_OK           The function succeeded.
 * @return LE_DUPLICATE    Module is already initialized
 */
//--------------------------------------------------------------------------------------------------
le_result_t atcmdsync_Init
(
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to send an AT Command and wait for response.
 *
 * @return pointer to the response
 */
//--------------------------------------------------------------------------------------------------
atcmdsync_ResultRef_t  atcmdsync_SendCommand
(
    atmgr_Ref_t  interfaceRef, ///< Interface where to send the sync command
    atcmd_Ref_t  atReqRef      ///< AT Request to execute
);

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to get the number of lines in response
 *
 */
//--------------------------------------------------------------------------------------------------
size_t atcmdsync_GetNumLines
(
    atcmdsync_ResultRef_t  resultRef
);

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to get line in index
 *
 */
//--------------------------------------------------------------------------------------------------
char* atcmdsync_GetLine
(
    atcmdsync_ResultRef_t   resultRef,
    uint32_t                index
);

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to get the final lines
 * ie: sucess or error code
 *
 */
//--------------------------------------------------------------------------------------------------
char* atcmdsync_GetFinalLine
(
    atcmdsync_ResultRef_t   resultRef
);

//--------------------------------------------------------------------------------------------------
/**
 * This function is called when debug
 *
 */
//--------------------------------------------------------------------------------------------------
void atcmdsync_Print
(
    atcmdsync_ResultRef_t   resultRef
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the intermediate event Id
 *
 * @return the event id
 */
//--------------------------------------------------------------------------------------------------
le_event_Id_t atcmdsync_GetIntermediateEventId
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the final event Id
 *
 * @return the event id
 */
//--------------------------------------------------------------------------------------------------
le_event_Id_t atcmdsync_GetFinalEventId
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the timer expiry handler
 *
 * @return le_timer_ExpiryHandler_t hander
 */
//--------------------------------------------------------------------------------------------------
le_timer_ExpiryHandler_t  atcmdsync_GetTimerExpiryHandler
(
);


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to send a standard command
 * This is the default sending with intermediate pattern
 *
 * @return LE_FAULT when pattern match : ERROR, +CME ERROR, +CMS ERROR
 * @return LE_TIMEOUT when pattern match : TIMEOUT
 * @return LE_OK when pattern match : OK
 */
//--------------------------------------------------------------------------------------------------
le_result_t atcmdsync_SendStandard
(
    atmgr_Ref_t             interfaceRef,   ///< [IN] Interface where to send the sync command
    const char             *commandPtr,     ///< [IN] the command string to execute
    atcmdsync_ResultRef_t  *responseRefPtr, ///< [OUT] filled the response if not NULL
    const char            **intermediatePatternPtr, ///< [IN] intermediate pattern expected
    uint32_t                timer           ///< [IN] the timer in seconds
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to create a command with the good parameters
 *
 * @return reference on a new atcommand
 */
//--------------------------------------------------------------------------------------------------
atcmd_Ref_t atcmdsync_PrepareStandardCommand
(
    const char          *commandPtr,    ///< [IN] the command string to execute
    const char         **intermediatePatternPtr, ///< [IN] intermediate pattern expected
    const char         **finalSuccessPatternPtr, ///< [IN] final pattern that will succeed
    const char         **finalFailedPatternPtr, ///< [IN] final pattern that will failed
    uint32_t             timer         ///< [IN] the timer in seconds
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to check the result structure
 *
 * @return LE_FAULT when finalFailedPatternPtr match
 * @return LE_TIMEOUT when a timeout occur
 * @return LE_OK when finalSuccessPattern match
 */
//--------------------------------------------------------------------------------------------------
le_result_t atcmdsync_CheckCommandResult
(
    atcmdsync_ResultRef_t  resultRef,    ///< [IN] the command result to check
    const char           **finalSuccessPatternPtr, ///< [IN] final pattern that will succeed
    const char           **finalFailedPatternPtr ///< [IN] final pattern that will failed
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to fille the buffer commandPtr with the command string
 *
 * @return LE_FAULT when the string is not filled
 * @return LE_OK the string is filled
 */
//--------------------------------------------------------------------------------------------------
void atcmdsync_PrepareString
(
    char        *commandPtr,    ///< [OUT] the command result to check
    uint32_t     commandSize,   ///< [IN] the commandPtr buffer size
    char        *format,        ///< [IN] the format
    ...
);


#endif // LEGATO_ATCMDSYNC_INCLUDE_GUARD
