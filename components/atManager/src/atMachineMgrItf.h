/** @file atMachineMgrItf.h
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#ifndef LEGATO_ATMACHINEMGRITF_INCLUDE_GUARD
#define LEGATO_ATMACHINEMGRITF_INCLUDE_GUARD

#include "legato.h"
#include "atMachineManager.h"

//--------------------------------------------------------------------------------------------------
/**
 * ATManager interface structure
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct atmgr {
    struct ATManagerStateMachine atManager;     ///< FSM
    le_event_FdHandlerRef_t fdHandlerRef;       ///< fd handler of the device
    le_event_Id_t resumeInterfaceId;            ///< event to start an interface
    le_event_Id_t suspendInterfaceId;           ///< event to stop an interface
    le_event_Id_t subscribeUnsolicitedId;       ///< event to add unsolicited to the FSM
    le_event_Id_t unSubscribeUnsolicitedId;     ///< event to remove unsoilicited for the FSM
    le_event_Id_t sendCommandId;                ///< event to send a command
    le_event_Id_t cancelCommandId;              ///< event to cancel a command
    le_sem_Ref_t  waitingSemaphore;             ///< semaphore used to synchronize atMgr_ API
}
atmgr_t;

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to initialize the atmanageritf
 *
 */
//--------------------------------------------------------------------------------------------------
void atmachinemgritf_Init
(
    void
);

#endif /* LEGATO_ATMACHINEMGRITF_INCLUDE_GUARD */
