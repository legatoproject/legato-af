//--------------------------------------------------------------------------------------------------
/**
 *  Data Channel Server's C code implementation of the data structure support & utilities for the
 *  le_dcs APIs, including creation, deletion, lookup, mapping, etc.
 *
 *  Copyright (C) Sierra Wireless Inc.
 *
 */
//--------------------------------------------------------------------------------------------------

#include <stdlib.h>
#include <string.h>
#include "legato.h"
#include "interfaces.h"
#include "dcs.h"


//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for channel database objects
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t ChannelRefMap;


//--------------------------------------------------------------------------------------------------
/**
 * The memory pools for channel DB objects
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t ChannelDbPool;
static le_mem_PoolRef_t ChannelDbEvtHdlrPool;


//--------------------------------------------------------------------------------------------------
/**
 * Destructor function that runs when a channel event handler is deallocated
 */
//--------------------------------------------------------------------------------------------------
static void DcsChannelDbEventHandlersDestructor
(
    void *objPtr
)
{
    le_dcs_channelDbEventHdlr_t *channelAppEvt = (le_dcs_channelDbEventHdlr_t *)objPtr;
    LE_DEBUG("Releasing event handler with reference %p", channelAppEvt->hdlrRef);
    le_event_RemoveHandler((le_event_HandlerRef_t)channelAppEvt->hdlrRef);
    channelAppEvt->hdlrRef = NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Delete all the event handlers of the given channel
 */
//--------------------------------------------------------------------------------------------------
static void DcsDeleteAllChannelEventHandlers
(
    le_dcs_channelDb_t *channelDbPtr
)
{
    le_dls_Link_t *evtHdlrPtr;
    le_dcs_channelDbEventHdlr_t *channelAppEvt;

    evtHdlrPtr = le_dls_Peek(&channelDbPtr->evtHdlrs);
    while (evtHdlrPtr)
    {
        channelAppEvt = CONTAINER_OF(evtHdlrPtr, le_dcs_channelDbEventHdlr_t, hdlrLink);
        if (!channelAppEvt)
        {
            continue;
        }
        le_dls_Remove(&channelDbPtr->evtHdlrs, &channelAppEvt->hdlrLink);
        le_mem_Release(channelAppEvt);
        evtHdlrPtr = le_dls_PeekNext(&channelDbPtr->evtHdlrs, evtHdlrPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Destructor function that runs when a channelDb is deallocated
 */
//--------------------------------------------------------------------------------------------------
static void DcsChannelDbDestructor
(
    void *objPtr
)
{
    le_dcs_channelDb_t *channelDb = (le_dcs_channelDb_t *)objPtr;

    DcsDeleteAllChannelEventHandlers(channelDb);
    channelDb->evtHdlrs = LE_DLS_LIST_INIT;
    le_dcsTech_ReleaseTechRef(channelDb->technology, channelDb->techRef);
    channelDb->techRef = NULL;
    le_ref_DeleteRef(ChannelRefMap, channelDb->channelRef);
    channelDb->channelRef = NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Find the channel event handler of the given channel for the given owner app by its sessionRef
 */
//--------------------------------------------------------------------------------------------------
le_dcs_channelDbEventHdlr_t *le_dcs_GetChannelAppEvtHdlr
(
    le_dcs_channelDb_t *channelDb,
    le_msg_SessionRef_t appSessionRef
)
{
    le_dls_Link_t *evtHdlrPtr;
    le_dcs_channelDbEventHdlr_t *channelAppEvt;

    evtHdlrPtr = le_dls_Peek(&channelDb->evtHdlrs);
    while (evtHdlrPtr)
    {
        channelAppEvt = CONTAINER_OF(evtHdlrPtr, le_dcs_channelDbEventHdlr_t, hdlrLink);
        if (channelAppEvt && (channelAppEvt->appSessionRef == appSessionRef))
        {
            return channelAppEvt;
        }
        evtHdlrPtr = le_dls_PeekNext(&channelDb->evtHdlrs, evtHdlrPtr);
    }
    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Find the channel event handler of the given channel for the given owner app by its sessionRef
 * and generate an event notification to it
 */
//--------------------------------------------------------------------------------------------------
void dcsChannelEvtHdlrSendNotice
(
    le_dcs_channelDb_t *channelDb,
    le_msg_SessionRef_t appSessionRef,
    le_dcs_Event_t evt
)
{
    le_dcs_channelDbEventHdlr_t *channelAppEvt =
        le_dcs_GetChannelAppEvtHdlr(channelDb, appSessionRef);
    le_dcs_channelDbEventReport_t evtReport;

    if (!channelAppEvt)
    {
        LE_DEBUG("No app event handler with session reference %p found for channel %s",
                 appSessionRef, channelDb->channelName);
        return;
    }

    LE_DEBUG("Send %s event notice for channel %s to app with session reference %p",
             (evt == LE_DCS_EVENT_DOWN) ? "Down" : "Up", channelDb->channelName,
             appSessionRef);
    evtReport.channelDb = channelDb;
    evtReport.event = evt;
    le_event_Report(channelAppEvt->channelEventId, &evtReport, sizeof(evtReport));
}


//--------------------------------------------------------------------------------------------------
/**
 * Function for walking the channel's event IDs and post an event to all of them to invoke
 * their corresponding event handlers about a system-wide state change; in this case is a down
 * transition for all.
 */
//--------------------------------------------------------------------------------------------------
static void DcsApplyTechSystemDownEventAction
(
    le_dcs_channelDb_t *channelDb
)
{
    le_dls_Link_t *evtHdlrPtr;
    le_dcs_channelDbEventHdlr_t *channelAppEvt;
    le_dcs_channelDbEventReport_t evtReport;

    evtHdlrPtr = le_dls_Peek(&channelDb->evtHdlrs);
    while (evtHdlrPtr)
    {
        // traverse all event handlers to trigger an event notification
        channelAppEvt = CONTAINER_OF(evtHdlrPtr, le_dcs_channelDbEventHdlr_t, hdlrLink);
        LE_DEBUG("Send Down event notice for channel %s to app with session reference %p",
                 channelDb->channelName, channelAppEvt->appSessionRef);
        evtReport.channelDb = channelDb;
        evtReport.event = LE_DCS_EVENT_DOWN;
        le_event_Report(channelAppEvt->channelEventId, &evtReport, sizeof(evtReport));
        evtHdlrPtr = le_dls_PeekNext(&channelDb->evtHdlrs, evtHdlrPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Function for walking the channel's list of all channels of the given technology & trigger
 * the posting of a system-wide event. The 2nd argument conveys the new system-wide state of the
 * technology.
 */
//--------------------------------------------------------------------------------------------------
void le_dcs_EventNotifierTechStateTransition
(
    le_dcs_Technology_t tech,
    bool techState
)
{
    le_ref_IterRef_t iterRef = le_ref_GetIterator(ChannelRefMap);
    le_dcs_channelDb_t *channelDb;
    void (* func)(le_dcs_channelDb_t *);

    LE_INFO("Notify all channels of technology %d of system state transition to %s",
            tech, techState ? "up" : "down");
    if (!techState)
    {
        func = &DcsApplyTechSystemDownEventAction;
    }
    else
    {
        func = &le_dcsTech_RetryChannel;
    }
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        // traverse all channels of the given technology type
        channelDb = (le_dcs_channelDb_t *)le_ref_GetValue(iterRef);
        if (channelDb->technology != tech)
        {
            continue;
        }
        if (channelDb->refCount > 0)
        {
            (*func)(channelDb);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Function for walking the given channel's list of apps that have an event handler registered
 * and posting an event to its event ID.
 * This function is called from one of DCS's technology event handler in the southbound after it
 * received an event there for this given channel.  Basically, after DCS is notified, now here
 * DCS notifies all the apps that have registered with it
 */
//--------------------------------------------------------------------------------------------------
void le_dcs_ChannelEventNotifier
(
    le_dcs_ChannelRef_t channelRef,
    le_dcs_Event_t evt
)
{
    le_dls_Link_t *evtHdlrPtr;
    le_dcs_channelDbEventHdlr_t *channelAppEvt;
    le_event_Id_t channelEventId;
    le_dcs_channelDbEventReport_t evtReport;

    le_dcs_channelDb_t *channelDb = le_dcs_GetChannelDbFromRef(channelRef);
    if (!channelDb)
    {
        LE_ERROR("Invalid channel reference %p for event notification", channelRef);
        return;
    }

    evtHdlrPtr = le_dls_Peek(&channelDb->evtHdlrs);
    while (evtHdlrPtr)
    {
        channelAppEvt = CONTAINER_OF(evtHdlrPtr, le_dcs_channelDbEventHdlr_t, hdlrLink);
        if (channelAppEvt && (channelEventId = channelAppEvt->channelEventId))
        {
            LE_DEBUG("Notify app of event %d on channel %s with eventID %p, handler %p",
                     (uint16_t)evt, channelDb->channelName, channelEventId,
                     channelAppEvt->channelEventHdlr);
            evtReport.channelDb = channelDb;
            evtReport.event = evt;
            le_event_Report(channelEventId, &evtReport, sizeof(evtReport));
        }
        evtHdlrPtr = le_dls_PeekNext(&channelDb->evtHdlrs, evtHdlrPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Delete a channel's event handler referred to by the given handler reference in the input
 *
 * @return
 *     - In the function return value, return the found channel db of the deleted event handler
 *     - If not found, NULL is returned
 */
//--------------------------------------------------------------------------------------------------
le_dcs_channelDb_t *DcsDelChannelEvtHdlr
(
    le_dcs_EventHandlerRef_t hdlrRef
)
{
    le_ref_IterRef_t iterRef = le_ref_GetIterator(ChannelRefMap);
    le_dcs_channelDb_t *channelDb;
    le_dls_Link_t *evtHdlrPtr;
    le_dcs_channelDbEventHdlr_t *channelAppEvt;

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        channelDb = (le_dcs_channelDb_t *)le_ref_GetValue(iterRef);
        evtHdlrPtr = le_dls_Peek(&channelDb->evtHdlrs);
        while (evtHdlrPtr)
        {
            channelAppEvt = CONTAINER_OF(evtHdlrPtr, le_dcs_channelDbEventHdlr_t, hdlrLink);
            if (channelAppEvt && (channelAppEvt->hdlrRef == hdlrRef))
            {
                LE_DEBUG("Removing event handler with reference %p with event ID %p", hdlrRef,
                         channelAppEvt->channelEventId);
                le_dls_Remove(&channelDb->evtHdlrs, &channelAppEvt->hdlrLink);
                le_mem_Release(channelAppEvt);
                return channelDb;
            }
            evtHdlrPtr = le_dls_PeekNext(&channelDb->evtHdlrs, evtHdlrPtr);
        }
    }

    LE_DEBUG("Failed to find event handler with reference %p to delete", hdlrRef);
    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Search for the object reference of the channelDb from its tech Db object reference
 *
 * @return
 *     - The found channelDb will be returned in the function's return value; otherwise, 0
 */
//--------------------------------------------------------------------------------------------------
le_dcs_ChannelRef_t le_dcs_GetChannelRefFromTechRef
(
    void *techRef
)
{
    le_ref_IterRef_t iterRef = le_ref_GetIterator(ChannelRefMap);
    le_dcs_channelDb_t *channelDb;

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        channelDb = (le_dcs_channelDb_t *)le_ref_GetValue(iterRef);
        if (channelDb->techRef == techRef)
        {
            return channelDb->channelRef;
        }
    }
    return 0;
}


//--------------------------------------------------------------------------------------------------
/**
 * Search for a channelDb from its channel name
 *
 * @return
 *     - The found channelDb will be returned in the function's return value; otherwise, NULL
 */
//--------------------------------------------------------------------------------------------------
le_dcs_channelDb_t *dcsGetChannelDbFromName
(
    const char *channelName,
    le_dcs_Technology_t tech
)
{
    le_ref_IterRef_t iterRef = le_ref_GetIterator(ChannelRefMap);
    le_dcs_channelDb_t *channelDb;

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        channelDb = (le_dcs_channelDb_t *)le_ref_GetValue(iterRef);
        if (channelDb->technology != tech)
        {
            continue;
        }
        if (strncmp(channelDb->channelName, channelName, LE_DCS_CHANNEL_NAME_MAX_LEN) == 0)
        {
            return channelDb;
        }
    }
    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Search for the given channel reference's channelDb from its reference map
 *
 * @return
 *     - The found channelDb will be returned in the function's return value; otherwise, NULL
 */
//--------------------------------------------------------------------------------------------------
le_dcs_channelDb_t *le_dcs_GetChannelDbFromRef
(
    le_dcs_ChannelRef_t channelRef
)
{
    return (le_dcs_channelDb_t *)le_ref_Lookup(ChannelRefMap, channelRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Search for the reference count of the channelDb given by its tech Db object reference
 *
 * @return
 *     - The reference count of the found channelDb will be returned in the function's 2nd argument
 *       as output and LE_OK as its return value; otherwise
 *     - Otherwise, 0 as output and LE_FAULT as the function's return value
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_dcs_GetChannelRefCountFromTechRef
(
    void *techRef,
    uint16_t *refCount
)
{
    le_ref_IterRef_t iterRef = le_ref_GetIterator(ChannelRefMap);
    le_dcs_channelDb_t *channelDb;

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        channelDb = (le_dcs_channelDb_t *)le_ref_GetValue(iterRef);
        if (channelDb->techRef == techRef)
        {
            *refCount = channelDb->refCount;
            return LE_OK;
        }
    }
    *refCount = 0;
    return LE_FAULT;
}


//--------------------------------------------------------------------------------------------------
/**
 * Allocate & initialize a new channelDb's event handler struct
 *
 * @return
 *     - Return the newly allocated & initialized channelDb handler back to the function caller
 */
//--------------------------------------------------------------------------------------------------
le_dcs_channelDbEventHdlr_t *dcsChannelDbEvtHdlrInit
(
    void
)
{
    le_dcs_channelDbEventHdlr_t *channelEvtHdlr = le_mem_ForceAlloc(ChannelDbEvtHdlrPool);
    if (!channelEvtHdlr)
    {
        return NULL;
    }
    memset(channelEvtHdlr, 0, sizeof(le_dcs_channelDbEventHdlr_t));
    channelEvtHdlr->hdlrLink = LE_DLS_LINK_INIT;
    return channelEvtHdlr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Create a channelDb for the given channel in the argument
 *
 * @return
 *     - The function returns the channelDb if it is found existing; otherwise, a new one will be
 *       created anew and returned as the function's return value
 */
//--------------------------------------------------------------------------------------------------
le_dcs_ChannelRef_t le_dcs_CreateChannelDb
(
    le_dcs_Technology_t tech,
    const char *channelName
)
{
    le_dcs_channelDb_t *channelDb = dcsGetChannelDbFromName(channelName, tech);
    if (channelDb)
    {
        LE_DEBUG("ChannelDb reference %p present for channel %s", channelDb->channelRef,
                 channelName);
        return channelDb->channelRef;
    }

    channelDb = le_mem_ForceAlloc(ChannelDbPool);
    if (!channelDb)
    {
        LE_ERROR("Unable to alloc channelDb for channel %s", channelName);
        return NULL;
    }
    memset(channelDb, 0, sizeof(le_dcs_channelDb_t));
    channelDb->technology = tech;
    strncpy(channelDb->channelName, channelName, LE_DCS_CHANNEL_NAME_MAX_LEN);
    // Create a safe reference for this data profile object.
    channelDb->channelRef = le_ref_CreateRef(ChannelRefMap, channelDb);

    channelDb->techRef = le_dcsTech_CreateTechRef(channelDb->technology, channelName);
    if (!channelDb->techRef)
    {
        LE_ERROR("Failed to create tech db for channel %s", channelName);
        le_ref_DeleteRef(ChannelRefMap, channelDb->channelRef);
        le_mem_Release(channelDb);
        return NULL;
    }

    channelDb->evtHdlrs = LE_DLS_LIST_INIT;

    LE_DEBUG("ChannelRef %p created for channel %s", channelDb->channelRef, channelName);
    return channelDb->channelRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Allocate memory pools, event pools and reference maps during DCS' init
 */
//--------------------------------------------------------------------------------------------------
void dcsCreateDbPool
(
    void
)
{
    // Allocate the channelDb pool, and set the max number of objects
    ChannelDbPool = le_mem_CreatePool("ChannelDbPool", sizeof(le_dcs_channelDb_t));
    le_mem_ExpandPool(ChannelDbPool, LE_DCS_CHANNELDBS_MAX);
    le_mem_SetDestructor(ChannelDbPool, DcsChannelDbDestructor);

    // Allocate the channel Db app event pool, and set the max number of objects
    ChannelDbEvtHdlrPool = le_mem_CreatePool("ChannelDbEvtHdlrPool",
                                             sizeof(le_dcs_channelDbEventHdlr_t));
    le_mem_ExpandPool(ChannelDbEvtHdlrPool, LE_DCS_CHANNELDB_EVTHDLRS_MAX);
    le_mem_SetDestructor(ChannelDbEvtHdlrPool, DcsChannelDbEventHandlersDestructor);

    // Create a safe reference map for data channel objects
    ChannelRefMap = le_ref_CreateMap("Channel Reference Map", LE_DCS_CHANNELDBS_MAX);
}
