/**
 * @page c_atcmd AT Commands
 *
 * This module provide API to manage command send to @ref atmgr_Ref_t
 *
 * @section atcmd_toc Table of Contents
 *
 * - @ref atcmd_intro
 * - @ref atcmd_rational
 *      - @ref atcmd_handler
 *      - @ref atcmd_functions
 *      - @ref atcmd_examples
 *
 * @section atcmd_intro Introduction
 *
 * The main idea is to provide the possibility to subscribe to string pattern that correspond
 * to an AT Command.
 *
 * AT command definition:
 * - must have a "commandString"
 * - can have a "final string pattern"
 * - can have an "intermediate string pattern"
 * - if there is a "final string patter", it must have a timer
 * - can have some data so send
 *
 * @section atcmd_rational Rational
 *
 * As we work with event approch, we must be aware that some @ref le_event_Id_t must be create
 * to be able to be wake up by the "atMgr thread".
 * These @ref le_event_Id_t will be used for intermediate or final pattern matching.
 *
 * @subsection atcmd_handler Handler implementation
 *
 * As a developper, we need to add a handler to @ref le_event_Id_t and its report pointer will be
 * an @ref atcmd_Response_t.
 * With that the developper can know from which AT Command there is an intermediate or a final
 * pattern.
 *
 * There is another handler to implement, this is the @ref le_timer_ExpiryHandler_t.
 * The developper should implement this handler, and he has to know that when calling
 * @ref le_timer_GetContextPtr, he will have a reference on an @ref atcmd_Ref_t
 *
 * @subsection atcmd_functions Functions
 *
 * - @ref atcmd_AddCommand
 *
 * This function provide the possibility to set the must have "commandString", we can ask it
 * to have extra data, that mean that the intermediate will be with 2 lines.
 *
 * - @ref atcmd_AddFinalResp
 *
 * This function provide the possibility to add the pattern that will trigger an event on
 * @ref le_event_Id_t provided.
 *
 * - @ref atcmd_AddIntermediateResp
 *
 * This function provide the possibility to add the pattern that will trigger an event on
 * @ref le_event_Id_t provided.
 *
 * - @ref atcmd_SetTimer
 *
 * This function provide the possibility to set the handler that will be called when the timer
 * expired. If it occurs be aware to tell @ref atmgr_Ref_t that the command should be deleted with
 * @ref atmgr_CancelCommandRequest
 *
 * - @ref atcmd_AddData
 *
 * This function provide the possibility to set the data that will be send after a first response
 * of the modem (usually it is the character '>')
 *
 * @subsection atcmd_examples Examples
 *
 * Here are some example of AT Command settings:
 *
 * - ATE0 : set no echo
 *     - commandString -> "ATE0"
 *     - intermediate pattern -> none
 *     - final pattern -> "OK", "ERROR", "+CME ERROR:", "+CMS ERROR:"
 *     - timer -> 30
 *     - data -> none
 *
 * will be coded like that:
 *
 * @code

 const char* intermediatePatternPtr[] = {NULL};
 const char* finalPatternPtr[] = {"OK","ERROR","+CME ERROR:","+CMS ERROR:",NULL};

 atcmd_Ref_t atReqRef = atcmd_Create();

 atcmd_AddCommand(atReqRef,"ATE0",false); // No need of extra data
 atcmd_AddIntermediateResp(atReqRef,eventIdIntermediate,intermediatePatternPtr); // eventIdIntermediate should exist
 atcmd_AddFinalResp(atReqRef,eventIdFinal,finalPatternPtr); // eventIdFinal should exist
 atcmd_SetTimer(atReqRef,30,TimerHandler); // TimerHandler should be defined
 atcmd_AddData(atReqRef,NULL,0);    // No data

 * @endcode
 *
 * - AT+CREG? : Get network signal information
 *     - commandString -> "AT+CREG?"
 *     - intermediate pattern -> "+CREG:"
 *     - final pattern -> "OK", "ERROR", "+CME ERROR:", "+CMS ERROR:"
 *     - timer -> 30
 *     - data -> none
 *
 * will be coded like that:
 *
 * @code

 const char* intermediatePatternPtr[] = {"+CREG:", NULL};
 const char* finalPatternPtr[] = {"OK","ERROR","+CME ERROR:","+CMS ERROR:",NULL};

 atcmd_Ref_t atReqRef = atcmd_Create();

 atcmd_AddCommand(atReqRef,"AT+CREG?",false); // No need of extra data
 atcmd_AddIntermediateResp(atReqRef,eventIdIntermediate,intermediatePatternPtr); // eventIdIntermediate should exist
 atcmd_AddFinalResp(atReqRef,eventIdFinal,finalPatternPtr); // eventIdFinal should exist
 atcmd_SetTimer(atReqRef,30,TimerHandler); // TimerHandler should be defined
 atcmd_AddData(atReqRef,NULL,0);    // No data

 * @endcode
 *
 * - AT+CMGS=XXXXXXXXXX : Send message "THIS MESSAGE" to XXXXXXXXXX
 *     - commandString -> "AT+CMGS=XXXXXXXXXX"
 *     - intermediate pattern -> "+CMGS:" (this is to get the refence of the sending message)
 *     - final pattern -> "OK", "ERROR", "+CME ERROR:", "+CMS ERROR:"
 *     - timer -> 30
 *     - data -> "THIS MESSAGE"
 *
 * will be coded like that:
 *
 * @code

 char commandString="AT+CMGS=XXXXXXXXXX";
 char dataString="THIS MESSAGE";

 const char* intermediatePatternPtr[] = {"+CMGS:", NULL};
 const char* finalPatternPtr[] = {"OK","ERROR","+CME ERROR:","+CMS ERROR:",NULL};

 atcmd_Ref_t atReqRef = atcmd_Create();

 atcmd_AddCommand(atReqRef,commandString,false); // No need of extra data
 atcmd_AddIntermediateResp(atReqRef,eventIdIntermediate,intermediatePatternPtr); // eventIdIntermediate should exist
 atcmd_AddFinalResp(atReqRef,eventIdFinal,finalPatternPtr); // eventIdFinal should exist
 atcmd_SetTimer(atReqRef,30,TimerHandler); // TimerHandler should be defined
 atcmd_AddData(atReqRef,dataString,strlen(dataString));    // message to send

 * @endcode
 *
 * - AT+CMGR=1 : Read message at index 1, we suppose that we are in PDU mode
 *     - commandString -> "AT+CMGR=1" with extra data
 *     - intermediate pattern -> "+CMGR:" (because of extra data, there will be 2 line read send)
 *     - final pattern -> "OK", "ERROR", "+CME ERROR:", "+CMS ERROR:"
 *     - timer -> 30
 *     - data -> none
 *
 * will be coded like that:
 *
 * @code

 char commandString="AT+CMGR=1";

 const char* intermediatePatternPtr[] = {"+CMGS:", NULL};
 const char* finalPatternPtr[] = {"OK","ERROR","+CME ERROR:","+CMS ERROR:",NULL};

 atcmd_Ref_t atReqRef = atcmd_Create();

 atcmd_AddCommand(atReqRef,commandString,true); // intermediate will have extra data
 atcmd_AddIntermediateResp(atReqRef,eventIdIntermediate,intermediatePatternPtr); // eventIdIntermediate should exist
 atcmd_AddFinalResp(atReqRef,eventIdFinal,finalPatternPtr); // eventIdFinal should exist
 atcmd_SetTimer(atReqRef,30,TimerHandler); // TimerHandler should be defined
 atcmd_AddData(atReqRef,NULL,0);    // message to send

 * @endcode
 *
 * In the previous example the intermediate handler will be trigger 2 times
 * (one the the intermediate, one for the extra data)
 *
 */

/** @file atCmd.h
 *
 * Legato @ref c_atcmd include file.
 *
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#ifndef LEGATO_ATCMD_INCLUDE_GUARD
#define LEGATO_ATCMD_INCLUDE_GUARD

#include "legato.h"

#define ATCOMMAND_SIZE              64
#define ATCOMMAND_DATA_SIZE         (36+140)*2
#define ATCMD_RESPONSELINE_SIZE     512

//--------------------------------------------------------------------------------------------------
/**
 * Reference on an AT Command
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct atcmd *atcmd_Ref_t;

//--------------------------------------------------------------------------------------------------
/**
 * typedef of atcmd_Response_t
 *
 * This struct is used the send a line when an Intermediate or a final pattern matched.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct atresponse {
    atcmd_Ref_t  fromWhoRef;
    char         line[ATCMD_RESPONSELINE_SIZE];
} atcmd_Response_t;

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to create a new AT command.
 *
 * @return pointer to the new AT Command
 */
//--------------------------------------------------------------------------------------------------
atcmd_Ref_t atcmd_Create
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set when the final response must be sent with string matched.
 *
 */
//--------------------------------------------------------------------------------------------------
void atcmd_AddFinalResp
(
    atcmd_Ref_t     atCommandRef,  ///< AT Command
    le_event_Id_t   reportId,      ///< Event Id to report to
    const char    **listFinalPtr   ///< List of pattern to match (must finish with NULL)
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set when the intermediate response must be sent with string
 * matched.
 *
 */
//--------------------------------------------------------------------------------------------------
void atcmd_AddIntermediateResp
(
    atcmd_Ref_t     atCommandRef,          ///< AT Command
    le_event_Id_t   reportId,              ///< Event Id to report to
    const char    **listIntermediatePtr    ///< List of pattern to match (must finish with NULL)
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the AT Command to send
 *
 */
//--------------------------------------------------------------------------------------------------
void atcmd_AddCommand
(
    atcmd_Ref_t     atCommandRef,  ///< AT Command
    const char     *commandPtr,    ///< the command to send
    bool            extraData      ///< Indicate if cmd has additionnal data without prefix.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the AT Data with prompt expected
 *
 */
//--------------------------------------------------------------------------------------------------
void atcmd_AddData
(
    atcmd_Ref_t     atCommandRef,  ///< AT Command
    const char     *dataPtr,       ///< the data to send if expectPrompt is true
    uint32_t        dataSize      ///< Size of AtData to send
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set timer when a command is finished
 *
 */
//--------------------------------------------------------------------------------------------------
void atcmd_SetTimer
(
    atcmd_Ref_t              atCommandRef,  ///< AT Command
    uint32_t                 timer,          ///< the timer
    le_timer_ExpiryHandler_t handlerRef      ///< the handler to call if the timer expired
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the ID of a command
 *
 */
//--------------------------------------------------------------------------------------------------
uint32_t atcmd_GetId
(
    atcmd_Ref_t atCommandRef  ///< AT Command
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the command string
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t atcmd_GetCommand
(
    atcmd_Ref_t  atCommandRef,  ///< AT Command
    char        *commandPtr,    ///< [IN/OUT] command buffer
    size_t       commandSize    ///< [IN] size of commandPtr
);

/* ------------------------------------------------------------------------------------------------
 * ------------------------------------------------------------------------------------------------
 *
 * atstring utils
 *
 * ------------------------------------------------------------------------------------------------
 * ------------------------------------------------------------------------------------------------
 */

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to count the number of paramter between ',' and ':' and to set
 * all ',' with '\0' and the new char after ':' to '\0'
 *
 * @return the number of paremeter in the line
 */
//--------------------------------------------------------------------------------------------------
uint32_t atcmd_CountLineParameter
(
    char        *linePtr       ///< [IN/OUT] line to parse
);



//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to count the get the @ref pos parameter of Line
 *
 *
 * @return pointer to the string at pos position in Line
 */
//--------------------------------------------------------------------------------------------------
char* atcmd_GetLineParameter
(
    const char* linePtr,   ///< [IN] Line to read
    uint32_t    pos     ///< [IN] Position to read
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to copy string from inBuffer to outBuffer without '"'
 *
 *
 * @return number of char really copy
 */
//--------------------------------------------------------------------------------------------------
uint32_t atcmd_CopyStringWithoutQuote
(
    char*       outBufferPtr,  ///< [OUT] final buffer
    const char* inBufferPtr,   ///< [IN] initial buffer
    uint32_t    inBufferSize ///< [IN] Max char to copy from inBuffer to outBuffer
);

#endif /* LEGATO_ATCMD_INCLUDE_GUARD */
