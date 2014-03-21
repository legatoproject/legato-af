/** @file atMachineCommand.h
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#ifndef LEGATO_ATMACHINECOMMAND_INCLUDE_GUARD
#define LEGATO_ATMACHINECOMMAND_INCLUDE_GUARD

#include "legato.h"

#include "atCmd.h"

//--------------------------------------------------------------------------------------------------
/**
 * AT Command structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct atcmd {
    uint32_t                    commandId;  ///< Id for the command
    uint8_t                     command[ATCOMMAND_SIZE+1];  ///< Command string to execute
    size_t                      commandSize;    ///< strlen of the command
    uint8_t                     data[ATCOMMAND_DATA_SIZE+1]; ///< Data to send if needed
    size_t                      dataSize;   ///< sizeof data to send
    le_dls_List_t               intermediateResp;   ///< List of string pattern for intermediate
    le_event_Id_t               intermediateId; ///< Event Id to report to when intermediateResp is found
    le_dls_List_t               finaleResp; ///< List of string pattern for final (ends the command)
    le_event_Id_t               finalId; ///< Event Id to report to when finalResp is found
    bool                        withExtra;  ///< Intermediate response have two lines
    bool                        waitExtra;  ///< Internal use, to get the two lines
    uint32_t                    timer;  ///< timer value in miliseconds (30s -> 30000)
    le_timer_ExpiryHandler_t    timerHandler; ///< timer handler
    le_dls_Link_t               link;   ///< used to add command in a wainting list
}
atcmd_t;

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to initialize the atcommand
 *
 */
//--------------------------------------------------------------------------------------------------
void atmachinecommand_Init
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to check if the line match any of intermediate string of the command
 *
 */
//--------------------------------------------------------------------------------------------------
void atmachinecommand_CheckIntermediate
(
    atcmd_Ref_t  atCommandRef,
    char           *atLinePtr,
    size_t          atLineSize
);

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to check if the line match any of final string of the command
 *
 */
//--------------------------------------------------------------------------------------------------
bool atmachinecommand_CheckFinal
(
    atcmd_Ref_t  atCommandRef,
    char           *atLinePtr,
    size_t          atLineSize
);

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to prepare the atcommand:
 *  - adding CR at the end of the command
 *  - adding Ctrl-Z for data
 *
 */
//--------------------------------------------------------------------------------------------------
void atmachinecommand_Prepare
(
    atcmd_Ref_t atCommandRef
);

#endif /* LEGATO_ATMACHINECOMMAND_INCLUDE_GUARD */
