/** @file atmachineparser.c
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
#include "atMachineParser.h"

/*
 * ATParser state machine
 *
 * @verbatim
 *
 *    ---------------                                           ---------------------
 *   |               |           EVENT_PARSER_CHAR             |                     |
 *   | StartingState |   ---------------------------------->   |  InitializingState  |
 *   |               |                                         |                     |
 *    ---------------                                           ---------------------
 *          |                                                            |
 *          |                                                            |
 *          |                                                            |
 *          |                     -----------------    EVENT_PARSER_CRLF |
 *          |                    |                 | <-------------------
 *           ---------------->   | ProcessingState | -----------------------
 *           EVENT_PARSER_CRLF   |                 | --------------------   |
 *                                -----------------                      |  |
 *                                    /\       /\      EVENT_PARSER_CRLF |  |
 *                                    |        |                         |  |
 *                                    |         -------------------------   |
 *                                     -------------------------------------
 *                                             EVENT_PARSER_PROMPT
 *
 * @verbatim
 *
 */

static void StartingState      (ATParserStateMachineRef_t smRef,EIndicationATParser input);
static void InitializingState  (ATParserStateMachineRef_t smRef,EIndicationATParser input);
static void ProcessingState    (ATParserStateMachineRef_t smRef,EIndicationATParser input);

static void SendLine(ATParserStateMachineRef_t smRef);
static void SendData(ATParserStateMachineRef_t smRef);

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to do a transition between two state with one event.
 *
 */
//--------------------------------------------------------------------------------------------------
static void updateTransition
(
    ATParserStateMachineRef_t   smRef,
    EIndicationATParser         input,
    ATParserStateProc_func_t    newState
)
{
    smRef->prevState   = smRef->curState;
    smRef->curState    = newState;
    smRef->lastEvent   = input;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is a state of the ATParser FSM.
 *
 */
//--------------------------------------------------------------------------------------------------
static void StartingState
(
    ATParserStateMachineRef_t smRef,
    EIndicationATParser input
)
{
    switch (input)
    {
        case EVENT_PARSER_CRLF:
            smRef->curContext.idx_lastCRLF = smRef->curContext.idx;
            updateTransition(smRef,input,ProcessingState);
            break;
        case EVENT_PARSER_CHAR:
            updateTransition(smRef,input,InitializingState);
            break;
        default:
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is a state of the ATParser FSM.
 *
 */
//--------------------------------------------------------------------------------------------------
static void InitializingState
(
    ATParserStateMachineRef_t smRef,
    EIndicationATParser input
)
{
    switch (input)
    {
        case EVENT_PARSER_CRLF:
            smRef->curContext.idx_lastCRLF = smRef->curContext.idx;
            updateTransition(smRef,input,ProcessingState);
            break;
        default:
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is a state of the ATParser FSM.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ProcessingState
(
    ATParserStateMachineRef_t smRef,
    EIndicationATParser input
)
{
    switch (input)
    {
        case EVENT_PARSER_CRLF:
            SendLine(smRef);
            updateTransition(smRef,input,ProcessingState);
            break;
        case EVENT_PARSER_PROMPT:
            SendData(smRef);
            updateTransition(smRef,input,ProcessingState);
            break;
        default:
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is a state of the ATParser FSM.
 *
 */
//--------------------------------------------------------------------------------------------------
static void SendData
(
    ATParserStateMachineRef_t smRef)
{
    LE_DEBUG("SEND DATA");
    (smRef->atManagerPtr->curState)(smRef->atManagerPtr,EVENT_MANAGER_SENDDATA);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to send the Line found between to CRLF (\\r\\n)
 *
 */
//--------------------------------------------------------------------------------------------------
static void SendLine
(
    ATParserStateMachineRef_t smRef
)
{
    int32_t newCRLF = smRef->curContext.idx-2;
    size_t lineSize = newCRLF - smRef->curContext.idx_lastCRLF;

    LE_DEBUG("%d [%d] ... [%d]",
                smRef->curContext.idx,
                smRef->curContext.idx_lastCRLF,
                newCRLF
    );

    if (lineSize > 0) {

        atmachinemanager_ProcessLine(
                        smRef->atManagerPtr ,
                        (char*)&(smRef->curContext.buffer[smRef->curContext.idx_lastCRLF]),
                        lineSize);

    }
    smRef->curContext.idx_lastCRLF = smRef->curContext.idx;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the next event to send to the ATParser FSM
 *
 */
//--------------------------------------------------------------------------------------------------
static bool GetNextEvent(
    ATParserStateMachineRef_t smRef,
    EIndicationATParser   *evPtr
)
{
    int32_t idx = smRef->curContext.idx++;
    if (idx < smRef->curContext.endbuffer) {
        if (smRef->curContext.buffer[idx] == '\r') {
            idx = smRef->curContext.idx++;
            if (idx < smRef->curContext.endbuffer) {
                if ( smRef->curContext.buffer[idx] == '\n') {
                    *evPtr = EVENT_PARSER_CRLF;
                    return true;
                } else {
                    return false;
                }
            } else {
                smRef->curContext.idx--;
                return false;
            }
        } else if (smRef->curContext.buffer[idx] == '\n') {
            if ( idx-1 > 0 ) {
                if (smRef->curContext.buffer[idx-1] == '\r') {
                    *evPtr = EVENT_PARSER_CRLF;
                    return true;
                } else {
                    return false;
                }
            } else {
                return false;
            }
        } else if (smRef->curContext.buffer[idx] == '>') {
            *evPtr = EVENT_PARSER_PROMPT;
            return true;
        } else {
            *evPtr = EVENT_PARSER_CHAR;
            return true;
        }
    } else {
        return false;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the ATParser FSM
 *
 */
//--------------------------------------------------------------------------------------------------
void atmachineparser_InitializeState
(
    ATParserStateMachineRef_t  smRef
)
{
    memset(smRef,0,sizeof(*smRef));
    smRef->curState = StartingState;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to read and send event to the ATParser FSM
 *
 */
//--------------------------------------------------------------------------------------------------
void atmachineparser_ReadBuffer
(
    ATParserStateMachineRef_t smRef
)
{
    EIndicationATParser event;
    for (;smRef->curContext.idx < smRef->curContext.endbuffer;)
    {
        if (GetNextEvent(smRef, &event)) {
            (smRef->curState)(smRef,event);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to delete character that where already read.
 *
 */
//--------------------------------------------------------------------------------------------------
void atmachineparser_ResetBuffer
(
    ATParserStateMachineRef_t smRef
)
{
    if (smRef->curState == ProcessingState)
    {
        size_t idx,sizeToCopy;
        sizeToCopy = smRef->curContext.endbuffer-smRef->curContext.idx_lastCRLF+2;
        LE_DEBUG("%d sizeToCopy %zd from %d",smRef->curContext.idx,sizeToCopy,smRef->curContext.idx_lastCRLF-2);
        for (idx = 0; idx < sizeToCopy; idx++) {
            smRef->curContext.buffer[idx] = smRef->curContext.buffer[idx+smRef->curContext.idx_lastCRLF-2];
        }
        smRef->curContext.idx_lastCRLF = 2;
        smRef->curContext.endbuffer = sizeToCopy;
        smRef->curContext.idx = smRef->curContext.endbuffer;
        LE_DEBUG("new idx %d, startLine %d",
                 smRef->curContext.idx,
                 smRef->curContext.idx_lastCRLF);
    } else {
        LE_DEBUG("Nothing should be copied in ATParser\n");
    }
}
