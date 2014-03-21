/** @file atMachineManager.h
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#ifndef LEGATO_ATMACHINEMANAGER_INCLUDE_GUARD
#define LEGATO_ATMACHINEMANAGER_INCLUDE_GUARD

#include "legato.h"
#include "atMachineFsm.h"

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to save the line to process, and execute the ATManager FSM
 *
 */
//--------------------------------------------------------------------------------------------------
void atmachinemanager_ProcessLine
(
    ATManagerStateMachineRef_t smRef,
    char *  linePtr,
    size_t  lineSize
);

//--------------------------------------------------------------------------------------------------
/**
 * This function is to resume the current ATManager
 *
 */
//--------------------------------------------------------------------------------------------------
void atmachinemanager_Resume
(
    void *report    ///< Callback parameter
);

//--------------------------------------------------------------------------------------------------
/**
 * This function is to suspend the current ATManager
 *
 */
//--------------------------------------------------------------------------------------------------
void atmachinemanager_Suspend
(
    void *report    ///< Callback parameter
);

//--------------------------------------------------------------------------------------------------
/**
 * This function is to add unsolicited string into current atmanager
 *
 */
//--------------------------------------------------------------------------------------------------
void atmachinemanager_AddUnsolicited
(
    void *report    ///< Callback parameter
);

//--------------------------------------------------------------------------------------------------
/**
 * This function is to remove unsolicited string from current atmanager
 *
 */
//--------------------------------------------------------------------------------------------------
void atmachinemanager_RemoveUnsolicited
(
    void *report    ///< Callback parameter
);

//--------------------------------------------------------------------------------------------------
/**
 * This function is to send a new AT command
 *
 */
//--------------------------------------------------------------------------------------------------
void atmachinemanager_SendCommand
(
    void *report    ///< Callback parameter
);

//--------------------------------------------------------------------------------------------------
/**
 * This function is to cancel an AT command
 *
 */
//--------------------------------------------------------------------------------------------------
void atmachinemanager_CancelCommand
(
    void *report    ///< Callback parameter
);

#endif /* LEGATO_ATMACHINEMANAGER_INCLUDE_GUARD */
