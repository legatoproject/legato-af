/**
 * @page c_remoteMgmt AirVantage Modem Remote Management Services
 *
 *@ref le_remoteMgmt_wakeupindic <br>
 *@ref le_remoteMgmt_dontdisturb <br>
 *
 *
 * The Modem platform embeds a client for wake up message requests from the Air Vantage
 * server.
 *
 * To implement Air Vantage Management Services (AVMS) features, when the embedded client
 * receives a wake up message, an indication must be sent to the Air Vantage Connector allowing it
 * to contact Air Vantage server so it can perform any pending jobs.
 *
 * Any User Space software components need to indicate when they are busy doing something
 * critical, and should not be interrupted by a firmware update (which generally results in a
 * reboot cycle).
 *
 * @section le_remoteMgmt_wakeupindic Wake-Up signaling
 *
 * The Air Vantage Connector can register a handler function to notify when a wake up message
 * is received by the Modem. The wake up notification advises Air Vantage
 * wants to talk to the Air Vantage Connector. A pending indicator can be maintained until the Air Vantage
 * Connector comes alive and asks for wake up registration.
 *
 * Use @c le_remoteMgmt_AddWakeUpIndicHandler() to register the handler function.
 *
 * Use @c le_remoteMgmt_RemoveWakeUpIndicHandler() to uninstall the handler function.
 *
 * @section le_remoteMgmt_dontdisturb Do Not Disturb Signs
 *
 * @c le_remoteMgmt_SetDoNotDisturbSign() allows callers to indicate they are busy doing
 * something critical and can't be interrupted by a firmware update.
 *
 * @c le_remoteMgmt_ClearDoNotDisturbSign() clears a 'Do Not Disturb' sign: when no more signs are
 * still set, the modem has permission to apply a firmware update.
 *
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

/**
 * @file le_remoteMgmt.h
 *
 * Legato @ref c_remoteMgmt include file.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013.  All rights reserved. Use of this work is subject to license.
 */


#ifndef LEGATO_REMOTE_MGMT_INCLUDE_GUARD
#define LEGATO_REMOTE_MGMT_INCLUDE_GUARD



//--------------------------------------------------------------------------------------------------
// Type definitions.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 *  Reference type for Wake-Up indications' Handler.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_remoteMgmt_WakeUpIndicHandler* le_remoteMgmt_WakeUpIndicHandlerRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * Reference type for 'Do Not Disturb Sign' objects.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_remoteMgmt_Dnd* le_remoteMgmt_DndRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * Handler called whenever a Wake-Up indication occurs.
 *
 * @param contextPtr  Context information the event handler may require.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*le_remoteMgmt_WakeUpIndicHandlerFunc_t)
(
    void*  contextPtr
);


//--------------------------------------------------------------------------------------------------
/**
 * Called to add an event handler for Wake-Up indications.
 *
 * @return Reference to the new event handler object.
 *
 * @note It's a fatal error if this function does succeed. If this function fails, it will not
 *       return.
 */
//--------------------------------------------------------------------------------------------------
le_remoteMgmt_WakeUpIndicHandlerRef_t le_remoteMgmt_AddWakeUpIndicHandler
(
    le_remoteMgmt_WakeUpIndicHandlerFunc_t  handlerFuncPtr,  ///< [IN] Event handler function.
    void*                                    contextPtr       ///< [IN] Handlers context.
);

//--------------------------------------------------------------------------------------------------
/**
 * Remove the registered event handler. Call this function stop receiving
 * Wake-Up events.
 *
 * @note Doesn't return on failure, no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
void le_remoteMgmt_RemoveWakeUpIndicHandler
(
    le_remoteMgmt_WakeUpIndicHandlerRef_t handlerRef   ///< [IN] Handler object to remove.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function set a 'Do Not Disturb' sign, it indicates when the caller is busy doing something
 * critical that should not be interrupted by a firmware update (which generally results in a
 * lengthy reboot cycle).
 *
 * @return Reference to the 'Do Not Disturb Sign' object.
 */
//--------------------------------------------------------------------------------------------------
le_remoteMgmt_DndRef_t le_remoteMgmt_SetDoNotDisturbSign
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Clears a 'Do Not Disturb' sign, the modem has permission to apply a firmware update now.
 */
//--------------------------------------------------------------------------------------------------
void le_remoteMgmt_ClearDoNotDisturbSign
(
    le_remoteMgmt_DndRef_t   dndRef   ///< [IN] 'Do Not Disturb Sign' object reference.
);


#endif // LEGATO_REMOTE_MGMT_INCLUDE_GUARD
