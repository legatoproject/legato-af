/** @file le_remoteMgmt.c
 *
 * This file contains the data structures and the source code of the high level
 * Modem Remote Management APIs.
 *
 * Copyright (C) Sierra Wireless, Inc. 2014. All rights reserved.
 */

#include "legato.h"
#include "interfaces.h"
#include "pa_remoteMgmt.h"

//--------------------------------------------------------------------------------------------------
// Symbols and enums.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Event ID for a new wake up notification
 *
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t NewWakeUpIndicationId;

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for le_remoteMgmt_DndRef_t objects.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t DndRefMap;
#define REMOTEMGMT_DND_MAX          5

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for le_remoteMgmt_WakeUpIndicHandlerRef_t objects.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t WakeUpIndicationHandlerRefMap;
#define REMOTEMGMT_WAKEUPINDICATIONHANDLER_MAX          5

//--------------------------------------------------------------------------------------------------
/**
 * Count number of handler already added
 *
 */
//--------------------------------------------------------------------------------------------------
static uint32_t WakeUpIndicationRegisteredCount=0;

//--------------------------------------------------------------------------------------------------
/**
 * Boolean to check if a Wake Up Indication occurs before any handler is registered
 *
 */
//--------------------------------------------------------------------------------------------------
static bool WakeUpMessageOccur = false;
//--------------------------------------------------------------------------------------------------
// Data structures.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Structure for 'Do Not Disturb Sign' objects.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_remoteMgmt_Dnd
{
    uint32_t counter;
} le_remoteMgmt_Dnd_t;

static le_remoteMgmt_Dnd_t DoNotDisturb; ///< The Dnd object

//--------------------------------------------------------------------------------------------------
//                                       Static declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * The first-layer Wake-Up indications Change Handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static void FirstLayerWakeUpIndicChangeHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    le_remoteMgmt_WakeUpIndicHandlerFunc_t clientHandlerFunc = secondLayerHandlerFunc;

    clientHandlerFunc(le_event_GetContextPtr());
}

//--------------------------------------------------------------------------------------------------
/**
 * New Wake-Up indications handler function.
 *
 */
//--------------------------------------------------------------------------------------------------
static void NewWakeUpIndicHandler
(
    void
)
{
    LE_DEBUG("Handler Function called");

    // If there is at least one handler
    if ( WakeUpIndicationRegisteredCount > 0 )
    {
        le_event_Report(NewWakeUpIndicationId, NULL, 0);
    }
    else
    {
        // Save that a wakeup indication has occur because there is no handler registered
        WakeUpMessageOccur = true;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * handler function to the close session service
 *
 */
//--------------------------------------------------------------------------------------------------
static void closeSessionEventHandler
(
    le_msg_SessionRef_t sessionRef,
    void*               contextPtr
)
{
    // Clear the DoNotDistrub sign as the session is closed
    le_remoteMgmt_ClearDoNotDisturbSign( (le_remoteMgmt_DndRef_t) contextPtr );
}

//--------------------------------------------------------------------------------------------------
// APIs.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the Modem Remote Management component
 *
 * @note If the initialization failed, it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_remoteMgmt_Init
(
    void
)
{
    // Create an event Id for a wake up notification
    NewWakeUpIndicationId = le_event_CreateId("NewWakeUpIndication",0);

    // Create the Safe Reference Map to use for Dnd object Safe References.
    DndRefMap = le_ref_CreateMap("remoteMgmtDndMap", REMOTEMGMT_DND_MAX);

    // Create the Safe Reference Map to use for Dnd object Safe References.
    WakeUpIndicationHandlerRefMap = le_ref_CreateMap("remoteMgmtWakeUpHandlerMap",
                                                     REMOTEMGMT_WAKEUPINDICATIONHANDLER_MAX);

    WakeUpIndicationRegisteredCount=0;
    WakeUpMessageOccur = false;

    // Set number of DoNotDisturbSign
    DoNotDisturb.counter = 0;

    // Register a handler function for new Wake-Up indications.
    LE_DEBUG("Set pa_remoteMgmt_AddMessageWakeUpHandler");
    pa_remoteMgmt_SetMessageWakeUpHandler(NewWakeUpIndicHandler);

    LE_DEBUG("Modem Remote Management component initialized");
}

//--------------------------------------------------------------------------------------------------
/**
 * Called to add an event handler for Wake-Up indications.
 *
 * @return A reference to the new event handler object.
 *
 * @note It is a fatal error if this function does succeed. If this function fails, it will not
 *       return.
 */
//--------------------------------------------------------------------------------------------------
le_remoteMgmt_WakeUpIndicHandlerRef_t le_remoteMgmt_AddWakeUpIndicHandler
(
    le_remoteMgmt_WakeUpIndicHandlerFunc_t  handlerFuncPtr,  ///< [IN] The event handler function.
    void*                                    contextPtr       ///< [IN] The handlers context.
)
{
    le_event_HandlerRef_t        handlerRef;

    if (handlerFuncPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", handlerFuncPtr);
        return NULL;
    }

    handlerRef = le_event_AddLayeredHandler("NewNetRegStateHandler",
                                            NewWakeUpIndicationId,
                                            FirstLayerWakeUpIndicChangeHandler,
                                            (le_event_HandlerFunc_t)handlerFuncPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    WakeUpIndicationRegisteredCount++;

    // If a wake up occured before the adding handler, then we catch it now.
    if ( WakeUpMessageOccur )
    {
        NewWakeUpIndicHandler();
        WakeUpMessageOccur = false;
    }

    return le_ref_CreateRef(WakeUpIndicationHandlerRefMap, handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove the registered event handler. Call this function when you no longer desire to receive
 * Wake-Up indication events.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
void le_remoteMgmt_RemoveWakeUpIndicHandler
(
    le_remoteMgmt_WakeUpIndicHandlerRef_t handlerRef   ///< [IN] The handler object to remove.
)
{
    le_event_RemoveHandler(le_ref_Lookup(WakeUpIndicationHandlerRefMap, handlerRef));
    if ( WakeUpIndicationRegisteredCount > 0 )
    {
        WakeUpIndicationRegisteredCount--;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function set a 'Do Not Disturb' sign, it indicates when the caller is busy doing something
 * critical that should not be interrupted by a firmware update (which generally results in a
 * lengthy reboot cycle).
 *
 * @return A reference to the 'Do Not Disturb Sign' object.
 */
//--------------------------------------------------------------------------------------------------
le_remoteMgmt_DndRef_t le_remoteMgmt_SetDoNotDisturbSign
(
    void
)
{
    le_remoteMgmt_DndRef_t remoteMgmt_DndRef;

    DoNotDisturb.counter++;
    // Disable firmware update
    pa_remoteMgmt_FirmwareUpdateActivate(false);

    remoteMgmt_DndRef = le_ref_CreateRef(DndRefMap, &DoNotDisturb);

    // Add a handler to the close session service
    le_msg_AddServiceCloseHandler( le_remoteMgmt_GetServiceRef(),
                                   closeSessionEventHandler,
                                   remoteMgmt_DndRef );

    return remoteMgmt_DndRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function clears a 'Do Not Disturb' sign: when no more signs are still set, the modem has
 * permission to apply a firmware update.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_remoteMgmt_ClearDoNotDisturbSign
(
    le_remoteMgmt_DndRef_t   dndRef   ///< [IN] The 'Do Not Disturb Sign' object reference.
)
{
    le_remoteMgmt_Dnd_t* dndPtr = le_ref_Lookup(DndRefMap, dndRef);

    if (dndPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", dndRef);
        return;
    }

    // Remove the reference
    le_ref_DeleteRef(DndRefMap, dndRef);

    if ( dndPtr->counter > 0)
    {
        dndPtr->counter--;

        LE_DEBUG("DoNotDisturb %d", dndPtr->counter);

        if (dndPtr->counter==0)
        {
            // Enable firmware update
            pa_remoteMgmt_FirmwareUpdateActivate(true);
        }
    }
}
