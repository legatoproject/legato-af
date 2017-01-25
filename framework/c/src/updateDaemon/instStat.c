//--------------------------------------------------------------------------------------------------
/**
 * @file instStat.c
 *
 * The instStat functions are used to let interested third parties know if an application has been
 * installed or removed.  These applications may not be directly involved in the install process,
 * but may just need to know that the system has changed.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "limit.h"


//--------------------------------------------------------------------------------------------------
/**
 * Estimated maximum number of handlers at a time.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_ESTIMATED_NUM_HANDLERS  20


//--------------------------------------------------------------------------------------------------
/**
 * The type of event registered for.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    INSTALL_HANDLER,   ///< The client is interested in application install events.
    UNINSTALL_HANDLER  ///< The client wants to know about applications being uninstalled.
}
HanderRegType_t;


//--------------------------------------------------------------------------------------------------
/**
 * The function to be called on application install/uninstall.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*HandlerFunction_t)(const char* appNamePtr, void* contextPtr);


//--------------------------------------------------------------------------------------------------
/**
 * Storage for the client registration info.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    HanderRegType_t type;          ///< The type of event this handler is for.
    HandlerFunction_t handlerPtr;  ///< The function to call on the event.
    void* contextPtr;              ///< The context to provide the called function.
}
HandlerRegisration_t;


//--------------------------------------------------------------------------------------------------
/**
 * Reference map of handler functions.
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t HandlerRefMap;


//--------------------------------------------------------------------------------------------------
/**
 * Memory pool of application names.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t HandlerRegPool;


//--------------------------------------------------------------------------------------------------
/**
 * Trigger all of the event handlers for the given type of event.
 */
//--------------------------------------------------------------------------------------------------
static void TriggerHandlers
(
    HanderRegType_t type,   ///< [IN] Trigger all the handlers for this event type.
    const char* appNamePtr  ///< [IN] The name of the application involved in the event.
)
//--------------------------------------------------------------------------------------------------
{
    le_ref_IterRef_t iterRef = le_ref_GetIterator(HandlerRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        HandlerRegisration_t const* regPtr = le_ref_GetValue(iterRef);

        if (regPtr->type == type)
        {
            regPtr->handlerPtr(appNamePtr, regPtr->contextPtr);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Register a handler for the given type of event.
 *
 * @return A handler reference that can be returned to the client.
 */
//--------------------------------------------------------------------------------------------------
static void* RegisterHandler
(
    HanderRegType_t type,          ///< [IN] The type of event being registered for.
    HandlerFunction_t handlerPtr,  ///< [IN] The function to call when this event occurs.
    void* contextPtr               ///< [IN] Context to give the called function.
)
//--------------------------------------------------------------------------------------------------
{
    if (handlerPtr == NULL)
    {
        LE_KILL_CLIENT("Bad handler supplied.");
        return NULL;
    }

    HandlerRegisration_t* regPtr = le_mem_ForceAlloc(HandlerRegPool);

    regPtr->type = type;
    regPtr->handlerPtr = handlerPtr;
    regPtr->contextPtr = contextPtr;

    return le_ref_CreateRef(HandlerRefMap, regPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Remove a handler from the registration list.
 */
//--------------------------------------------------------------------------------------------------
static void UnregisterHandler
(
    HanderRegType_t type,  ///< [IN] The type of event the handler was registered for.
    void* handlerRef       ///< [IN] The safe reference that was given to the client.
)
//--------------------------------------------------------------------------------------------------
{
    HandlerRegisration_t* regPtr = le_ref_Lookup(HandlerRefMap, handlerRef);

    if (   (regPtr == NULL)
        || (regPtr->type != type))
    {
        LE_KILL_CLIENT("Bad handle, %p, from client.", handlerRef);
        return;
    }

    le_mem_Release(regPtr);
    le_ref_DeleteRef(HandlerRefMap, handlerRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Initialize the instStat subsystem so that it is ready to report install and uninstall activity.
 */
//--------------------------------------------------------------------------------------------------
void instStat_Init
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    HandlerRefMap = le_ref_CreateMap("InstAppHandlers", MAX_ESTIMATED_NUM_HANDLERS);
    HandlerRegPool = le_mem_CreatePool("InstHandlerInfo", sizeof(HandlerRegisration_t));
}


//--------------------------------------------------------------------------------------------------
/**
 * Report to anyone who may be listening that an application has just been installed in the system.
 */
//--------------------------------------------------------------------------------------------------
void instStat_ReportAppInstall
(
    const char* appNamePtr  ///< [IN] The name of the application to report.
)
//--------------------------------------------------------------------------------------------------
{
    TriggerHandlers(INSTALL_HANDLER, appNamePtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Report that an application has been removed from the system.
 */
//--------------------------------------------------------------------------------------------------
void instStat_ReportAppUninstall
(
    const char* appNamePtr  ///< [IN] The name of the application to report.
)
//--------------------------------------------------------------------------------------------------
{
    TriggerHandlers(UNINSTALL_HANDLER, appNamePtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'le_instStat_AppInstallEvent'
 *
 * This event provides a notification of when an application is installed.
 */
//--------------------------------------------------------------------------------------------------
le_instStat_AppInstallEventHandlerRef_t le_instStat_AddAppInstallEventHandler
(
    le_instStat_AppInstallHandlerFunc_t handlerPtr,  ///< [IN] Client handler function.
    void* contextPtr                                 ///< [IN] Context for the handler.
)
//--------------------------------------------------------------------------------------------------
{
    return RegisterHandler(INSTALL_HANDLER, handlerPtr, contextPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'le_instStat_AppInstallEvent'
 */
//--------------------------------------------------------------------------------------------------
void le_instStat_RemoveAppInstallEventHandler
(
    le_instStat_AppInstallEventHandlerRef_t addHandlerRef  ///< [IN] The client handler to
                                                           ///<      unregister.
)
//--------------------------------------------------------------------------------------------------
{
    UnregisterHandler(INSTALL_HANDLER, addHandlerRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'le_instStat_AppUninstallEvent'
 *
 * This event provides a notification of when an application is uninstalled.
 */
//--------------------------------------------------------------------------------------------------
le_instStat_AppUninstallEventHandlerRef_t le_instStat_AddAppUninstallEventHandler
(
    le_instStat_AppUninstallHandlerFunc_t handlerPtr,  ///< [IN] Client handler function.
    void* contextPtr                                   ///< [IN] Context for the handler.
)
//--------------------------------------------------------------------------------------------------
{
    return RegisterHandler(UNINSTALL_HANDLER, handlerPtr, contextPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'le_instStat_AppUninstallEvent'
 */
//--------------------------------------------------------------------------------------------------
void le_instStat_RemoveAppUninstallEventHandler
(
    le_instStat_AppUninstallEventHandlerRef_t addHandlerRef  ///< [IN] The client handler to
                                                             ///<      unregister.
)
//--------------------------------------------------------------------------------------------------
{
    UnregisterHandler(UNINSTALL_HANDLER, addHandlerRef);
}
