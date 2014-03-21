/** @file atMachineParser.h
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#ifndef LEGATO_ATPARSER_INCLUDE_GUARD
#define LEGATO_ATPARSER_INCLUDE_GUARD

#include "legato.h"
#include "atMachineManager.h"
#include "atMachineFsm.h"

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the ATParser FSM
 *
 */
//--------------------------------------------------------------------------------------------------
void atmachineparser_InitializeState
(
    ATParserStateMachineRef_t  smRef
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to read and send event to the ATParser FSM
 *
 */
//--------------------------------------------------------------------------------------------------
void atmachineparser_ReadBuffer
(
    ATParserStateMachineRef_t smRef
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to delete character that where already read.
 *
 */
//--------------------------------------------------------------------------------------------------
void atmachineparser_ResetBuffer
(
    ATParserStateMachineRef_t smRef
);

#endif /* LEGATO_ATPARSER_INCLUDE_GUARD */
