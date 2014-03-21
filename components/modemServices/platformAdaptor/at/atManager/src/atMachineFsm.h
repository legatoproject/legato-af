/** @file atMachineFsm.h
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#ifndef LEGATO_ATMACHINESM_INCLUDE_GUARD
#define LEGATO_ATMACHINESM_INCLUDE_GUARD

#include "legato.h"
#include "atMgr.h"
#include "atMachineDevice.h"

/*
 * ATParser State Machine reference
 */
typedef struct ATParserStateMachine* ATParserStateMachineRef_t;

/*
 * ATManager State Machine reference
 */
typedef struct ATManagerStateMachine* ATManagerStateMachineRef_t;

/* ------------------------------------------------------------------------------------------------
 * ------------------------------------------------------------------------------------------------
 *
 * State Machine ATPARSER
 *
 * ------------------------------------------------------------------------------------------------
 * ------------------------------------------------------------------------------------------------
 */
#define ATFSMPARSER_BUFFER_MAX  1024
#define ATPARSER_LINE_MAX       (36+140)*2

//--------------------------------------------------------------------------------------------------
/**
 * ATParser structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct ATParser {
    uint8_t  buffer[ATFSMPARSER_BUFFER_MAX];    ///< buffer read
    int32_t  idx;                               ///< index of parsing the buffer
    size_t   endbuffer;                         ///< index where the read was finished (idx < endbuffer)
    int32_t  idx_lastCRLF;                      ///< index where the last CRLF has been found
} ATParser_t;

//--------------------------------------------------------------------------------------------------
/**
 * Enumeration of Event for ATParser
 *
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    EVENT_PARSER_CHAR=0,    ///< Any character except CRLF('\r\n') or PROMPT('>')
    EVENT_PARSER_CRLF,      ///< CRLF ('\r\n')
    EVENT_PARSER_PROMPT,    ///< PROMPT ('>')
    EVENT_PARSER_MAX        ///< unused
} EIndicationATParser;

/*
 * ATParserStateProc_func_t is the function pointer type that
 * is used to represent each state of our machine.
 * The ATParserStateMachineRef_t *sm argument holds the current
 * information about the machine most importantly the
 * current state.  A ATParserStateProc_func_t function may receive
 * input that forces a change in the current state.  This
 * is done by setting the curState member of the StateMachine.
 */
typedef void (*ATParserStateProc_func_t)(ATParserStateMachineRef_t sm, EIndicationATParser input);

/*
 * Now that ATParserStateProc_func_t is defined, we can define the
 * actual layout of StateMachine.  Here we have the
 * current state of the ATParser StateMachine.
 */
typedef struct ATParserStateMachine
{
    ATParserStateProc_func_t    prevState;      ///< Previous state for debuging purpose
    ATParserStateProc_func_t    curState;       ///< Current state
    EIndicationATParser         lastEvent;      ///< Last event received for debugging purpose
    ATParser_t                  curContext;     ///< ATParser structure
    ATManagerStateMachineRef_t  atManagerPtr;   ///< Reference of which ATManager it belongs
} ATParserStateMachine_t;


/* ------------------------------------------------------------------------------------------------
 * ------------------------------------------------------------------------------------------------
 *
 * state machine ATMANAGER
 *
 * ------------------------------------------------------------------------------------------------
 * ------------------------------------------------------------------------------------------------
 */
//--------------------------------------------------------------------------------------------------
/**
 * ATManager structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct ATManager {
    ATParserStateMachine_t  atParser;           ///< AT Machine Parser

    struct atdevice         atDevice;           ///< AT device

    char            atLine[ATPARSER_LINE_MAX];  ///< Last line retreived in atParser

    atcmd_Ref_t     atCommandInProgressRef;     ///< current command executed
    le_dls_List_t   atCommandList;              ///< List of command waiting for execution
    le_timer_Ref_t  atCommandTimer;             ///< command timer

    le_dls_List_t   atUnsolicitedList;          ///< List of unsolicited pattern
} ATManager_t;

//--------------------------------------------------------------------------------------------------
/**
 * Enumeration of Event for ATManager
 *
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    EVENT_MANAGER_SENDCMD=0,    ///< Send command
    EVENT_MANAGER_SENDDATA,     ///< Send data
    EVENT_MANAGER_PROCESSLINE,  ///< Process line event
    EVENT_MANAGER_CANCELCMD,    ///< Cancel command event
    EVENT_MANAGER_MAX           ///< Unused
} EIndicationATManager;

/*
 * ATManagerStateProc_Func_t is the function pointer type that
 * is used to represent each state of our machine.
 * The ATManagerStateMachineRef_t *smRef argument holds the current
 * information about the machine most importantly the
 * current state.  A ATManagerStateProc_Func_t function may receive
 * input that forces a change in the current state.  This
 * is done by setting the curState member of the ATManager StateMachine.
 */
typedef void (*ATManagerStateProc_Func_t)(ATManagerStateMachineRef_t smRef, EIndicationATManager input);

/*
 * Now that ATManagerStateProc_Func_t is defined, we can define the
 * actual layout of StateMachine.  Here we have the
 * current state of the ATManager StateMachine.
 */
typedef struct ATManagerStateMachine
{
    ATManagerStateProc_Func_t   prevState;      ///< Previous state for debugging purpose
    ATManagerStateProc_Func_t   curState;       ///< Current state
    EIndicationATManager        lastEvent;      ///< Last event received for debugging purpose
    ATManager_t                 curContext;     ///< ATManager structure
} ATManagerStateMachine_t;

#endif /* LEGATO_ATMACHINESM_INCLUDE_GUARD */
