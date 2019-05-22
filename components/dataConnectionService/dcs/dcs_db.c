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
static le_mem_PoolRef_t StartRequestRefDbPool;


//--------------------------------------------------------------------------------------------------
/**
 * The memory pool for channel query handler dbs as well as the typedef of such db which saves
 * the async callback function and context of each app which has provided them in a channel list
 * query
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t ChannelQueryHandlerDbPool;
static le_dls_List_t DcsChannelQueryHandlerDbList;


//--------------------------------------------------------------------------------------------------
/**
 * DcsChannelQueryHandlerDb_t is the channel query handler db that saves a caller's result handler's
 * callback function, context, as well as a dls link element for inserting this db into a double
 * link list.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_dcs_GetChannelsHandlerFunc_t handlerFunc;     ///< caller's result handler function
    void *handlerContext;                            ///< caller's result handler context
    le_dls_Link_t handlerLink;                       ///< double link list's link element
}
DcsChannelQueryHandlerDb_t;


//--------------------------------------------------------------------------------------------------
/**
 * Channel query time limit enforcer timer:
 */
//--------------------------------------------------------------------------------------------------
static bool ChannelQueryInAction = false;
static bool EnforceChannelQueryTimeLimit = false;
static le_timer_Ref_t ChannelQueryTimeEnforcerTimer = NULL;
#define GETCHANNELS_TIME_ENFORCER_LIMIT (LE_DCS_TECH_MAX * 20)


//--------------------------------------------------------------------------------------------------
/**
 * Channel query time limit enforcer timer handler
 * When the timer expires, that means the max time to wait for all channel scans has been reached
 * and likely some technology failed to report back.  Thus, quit pending & report failure back to
 * the channel list collector so that it can generate a channel list of all the channels collected
 * so far.
 */
//--------------------------------------------------------------------------------------------------
static void ChannelQueryTimeEnforcerTimerHandler
(
    le_timer_Ref_t timerRef     ///< [IN] Timer used to ensure the end of the session
)
{
    uint16_t i;

    LE_DEBUG("ChannelQueryTimeEnforcerTimer expired to enforce channel query completion");

    for (i=0; i<LE_DCS_TECH_MAX; i++)
    {
        if (le_dcsTech_ChannelQueryIsPending(i))
        {
            LE_WARN("Channel query from technology %d unfinished within time limit; DCS quit "
                    "waiting & proceed with result posting", i);
            le_dcsTech_CollectChannelQueryResults(i, LE_FAULT, NULL, 0);
        }
    }
}


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
    if (!channelAppEvt)
    {
        return;
    }
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
        evtHdlrPtr = le_dls_PeekNext(&channelDbPtr->evtHdlrs, evtHdlrPtr);
        if (!channelAppEvt)
        {
            continue;
        }
        le_dls_Remove(&channelDbPtr->evtHdlrs, &channelAppEvt->hdlrLink);
        channelAppEvt->hdlrLink = LE_DLS_LINK_INIT;
        le_mem_Release(channelAppEvt);
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
    uint16_t channelCount;
    le_dls_Link_t *refLink;
    le_dcs_startRequestRefDb_t *refDb;

    le_dcs_channelDb_t *channelDb = (le_dcs_channelDb_t *)objPtr;
    if (!channelDb)
    {
        return;
    }
    if (LE_OK != le_dcs_DecrementChannelCount(channelDb->technology, &channelCount))
    {
        LE_ERROR("Error in decrementing 0 channel count of technology %d", channelDb->technology);
    }

    DcsDeleteAllChannelEventHandlers(channelDb);
    channelDb->evtHdlrs = LE_DLS_LIST_INIT;
    le_dcsTech_ReleaseTechRef(channelDb->technology, channelDb->techRef);
    channelDb->techRef = NULL;
    le_ref_DeleteRef(ChannelRefMap, channelDb->channelRef);
    channelDb->channelRef = NULL;

    refLink = le_dls_Peek(&channelDb->startRequestRefList);
    while (refLink)
    {
        refDb = CONTAINER_OF(refLink, le_dcs_startRequestRefDb_t, refLink);
        refLink = le_dls_PeekNext(&channelDb->startRequestRefList, refLink);
        if (!refDb)
        {
            continue;
        }
        le_dcs_DeleteStartRequestRef(refDb, channelDb);
    }
    channelDb->startRequestRefList = LE_DLS_LIST_INIT;
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
    const char *eventString;

    if (!channelAppEvt)
    {
        LE_DEBUG("No app event handler with session reference %p found for channel %s",
                 appSessionRef, channelDb->channelName);
        return;
    }

    eventString = le_dcs_ConvertEventToString(evt);
    LE_DEBUG("Send %s event notice for channel %s to app with session reference %p",
             eventString, channelDb->channelName, appSessionRef);
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
        evtReport.event = LE_DCS_EVENT_TEMP_DOWN;
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
 * Function for creating a channel query handler db and saving the app provided async callback
 * function and context for posting back results when available
 */
//--------------------------------------------------------------------------------------------------
void le_dcs_AddChannelQueryHandlerDb
(
    le_dcs_GetChannelsHandlerFunc_t channelQueryHandlerFunc,
    void *context
)
{
    DcsChannelQueryHandlerDb_t *channelQueryHdlrDb;

    if (!channelQueryHandlerFunc)
    {
        LE_ERROR("Unable to add a NULL channel query handler function");
        return;
    }

    channelQueryHdlrDb = le_mem_ForceAlloc(ChannelQueryHandlerDbPool);
    if (!channelQueryHdlrDb)
    {
        LE_ERROR("Failed to alloc memory for channel query handler db");
        return;
    }
    memset(channelQueryHdlrDb, 0, sizeof(DcsChannelQueryHandlerDb_t));
    channelQueryHdlrDb->handlerFunc = channelQueryHandlerFunc;
    channelQueryHdlrDb->handlerContext = context;
    channelQueryHdlrDb->handlerLink = LE_DLS_LINK_INIT;
    le_dls_Queue(&DcsChannelQueryHandlerDbList, &channelQueryHdlrDb->handlerLink);
    LE_DEBUG("Added channel query handler %p with context %p", channelQueryHandlerFunc, context);
}


//--------------------------------------------------------------------------------------------------
/**
 * Function for walking the list of handlers registered by various apps for getting the results
 * of the latest channel list query and posting the results to each.
 */
//--------------------------------------------------------------------------------------------------
void le_dcs_ChannelQueryNotifier
(
    le_result_t result,
    le_dcs_ChannelInfo_t *channelList,
    size_t listSize
)
{
    le_dls_Link_t *queryHdlrPtr;
    DcsChannelQueryHandlerDb_t *queryHdlrDb;

    LE_DEBUG("Got channel list query result %d, list size %d", result, (uint16_t)listSize);

    if (le_timer_IsRunning(ChannelQueryTimeEnforcerTimer))
    {
        le_timer_Stop(ChannelQueryTimeEnforcerTimer);
    }

    if (listSize > LE_DCS_CHANNEL_LIST_ENTRY_MAX)
    {
        // Restrict listSize to prevent overrun in sending the notification
        listSize = LE_DCS_CHANNEL_LIST_ENTRY_MAX;
    }

    queryHdlrPtr = le_dls_Peek(&DcsChannelQueryHandlerDbList);
    while (queryHdlrPtr)
    {
        queryHdlrDb = CONTAINER_OF(queryHdlrPtr, DcsChannelQueryHandlerDb_t, handlerLink);
        queryHdlrPtr = le_dls_PeekNext(&DcsChannelQueryHandlerDbList, queryHdlrPtr);
        if (queryHdlrDb)
        {
            LE_DEBUG("Notify app of channel list results thru handler %p",
                     queryHdlrDb->handlerFunc);
            queryHdlrDb->handlerFunc(result, channelList, listSize,
                                     queryHdlrDb->handlerContext);
            le_dls_Remove(&DcsChannelQueryHandlerDbList, &queryHdlrDb->handlerLink);
            queryHdlrDb->handlerLink = LE_DLS_LINK_INIT;
            le_mem_Release(queryHdlrDb);
        }
    }

    // Done with GetChannels API callback; reset the flag to allow another round when called
    ChannelQueryInAction = false;
}


//--------------------------------------------------------------------------------------------------
/**
 * Enforce a time limit for the ChannelQuery query so that if any technology doesn't get back to
 * provide its list of available channels DCS would not get stuck pending forever.
 *
 */
//--------------------------------------------------------------------------------------------------
static void DcsChannelQueryEnforceTimeLimit
(
    void
)
{
    if (!EnforceChannelQueryTimeLimit)
    {
        return;
    }

    if (LE_OK != le_timer_Start(ChannelQueryTimeEnforcerTimer))
    {
        LE_ERROR("Failed to start the ChannelQuery query time limit enforcer timer");
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Enforce a time limit for the ChannelQuery query so that if any technology doesn't get back to
 * provide its list of available channels DCS would not get stuck pending forever.
 *
 */
//--------------------------------------------------------------------------------------------------
bool le_dcs_ChannelQueryIsRunning
(
    void
)
{
    if (ChannelQueryInAction)
    {
        return true;
    }

    ChannelQueryInAction = true;
    DcsChannelQueryEnforceTimeLimit();
    return false;
}


//--------------------------------------------------------------------------------------------------
/**
 * Destructor function that runs when a channel query handler db is deallocated
 */
//--------------------------------------------------------------------------------------------------
static void DcsChannelQueryHandlerDbDestructor
(
    void *objPtr
)
{
    DcsChannelQueryHandlerDb_t *channelQueryHdlrDb = (DcsChannelQueryHandlerDb_t *)objPtr;
    if (!channelQueryHdlrDb)
    {
        return;
    }
    channelQueryHdlrDb->handlerFunc = NULL;
    channelQueryHdlrDb->handlerContext = NULL;
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
            if (evt == LE_DCS_EVENT_DOWN)
            {
                // Reset the refcount upon sending a Down event northbound
                le_dcs_AdjustReqCount(channelDb, false);
            }
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
    le_dcs_Technology_t tech,
    void *techRef
)
{
    le_ref_IterRef_t iterRef = le_ref_GetIterator(ChannelRefMap);
    le_dcs_channelDb_t *channelDb;

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        channelDb = (le_dcs_channelDb_t *)le_ref_GetValue(iterRef);
        if ((channelDb->technology == tech) && (channelDb->techRef == techRef))
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
le_dcs_channelDb_t *le_dcs_GetChannelDbFromName
(
    const char *channelName,
    le_dcs_Technology_t tech
)
{
    le_ref_IterRef_t iterRef = le_ref_GetIterator(ChannelRefMap);
    le_dcs_channelDb_t *channelDb;

    if (!channelName || (strlen(channelName) == 0))
    {
        return NULL;
    }

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
    if (!channelRef)
    {
        return NULL;
    }
    return (le_dcs_channelDb_t *)le_ref_Lookup(ChannelRefMap, channelRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Destructor function that runs when a Start Request reference db is deallocated
 */
//--------------------------------------------------------------------------------------------------
static void DcsStartRequestRefDbDestructor
(
    void *objPtr
)
{
    le_dcs_startRequestRefDb_t *refDb = (le_dcs_startRequestRefDb_t *)objPtr;
    if (!refDb)
    {
        return;
    }
    refDb->refLink = LE_DLS_LINK_INIT;
    refDb->ref = 0;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function adds a Start Request reference onto the given channel db's list of such
 * references so that it can be retrieved back for validation upon its corresponding Stop Request.
 *
 * @return
 *     - true upon successful addition of this reference onto the given channeldb's list
 *     - false upon any failure to add it
 */
//--------------------------------------------------------------------------------------------------
bool le_dcs_AddStartRequestRef
(
    le_dcs_ReqObjRef_t reqRef,
    le_dcs_channelDb_t *channelDb
)
{
    le_dcs_startRequestRefDb_t *refDb;

    if (!channelDb || !reqRef)
    {
        return false;
    }

    refDb = le_mem_ForceAlloc(StartRequestRefDbPool);
    if (!refDb)
    {
        LE_ERROR("Failed to alloc memory for Start Request reference db");
        return false;
    }

    refDb->ref = reqRef;
    refDb->refLink = LE_DLS_LINK_INIT;
    le_dls_Queue(&channelDb->startRequestRefList, &refDb->refLink);
    return true;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function searches for the Start Request reference db of the given Start Request reference
 * from the given channel db's reference list
 *
 * @return
 *     - The found Start Request reference db if successful; otherwise, NULL
 */
//--------------------------------------------------------------------------------------------------
static le_dcs_startRequestRefDb_t *DcsGetStartRequestRefDb
(
    le_dcs_ReqObjRef_t reqRef,
    le_dcs_channelDb_t *channelDb
)
{
    le_dls_Link_t *refLink;
    le_dcs_startRequestRefDb_t *refDb;

    if (!channelDb || !reqRef)
    {
        return NULL;
    }

    refLink = le_dls_Peek(&channelDb->startRequestRefList);
    while (refLink)
    {
        refDb = CONTAINER_OF(refLink, le_dcs_startRequestRefDb_t, refLink);
        if (refDb && (refDb->ref == reqRef))
        {
            return refDb;
        }
        refLink = le_dls_PeekNext(&channelDb->startRequestRefList, refLink);
    }
    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function deletes the given Start Request reference db by first removing it from its channel
 * db's reference list and then releasing it to let its destructor to do the rest of the necessary
 * clean up. 
 *
 * @return
 *     - true upon successful deletion & cleanup; false otherwise
 */
//--------------------------------------------------------------------------------------------------
bool le_dcs_DeleteStartRequestRef
(
    le_dcs_startRequestRefDb_t *refDb,
    le_dcs_channelDb_t *channelDb
)
{
    if (!refDb)
    {
        return false;
    }

    le_dls_Remove(&channelDb->startRequestRefList, &refDb->refLink);
    le_mem_Release(refDb);
    return true;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function searchs for the given Start Request reference its reference db and channel db on
 * which it's found
 *
 * @return
 *     - If found, it returns the Start Request reference db in its return value and also the
 *       channel db in the 2nd function argument as output.
 *     - Otherwise, it returns NULL in both its function return value & 2nd output argument
 */
//--------------------------------------------------------------------------------------------------
le_dcs_channelDb_t *le_dcs_GetChannelDbFromStartRequestRef
(
    le_dcs_ReqObjRef_t reqRef,
    le_dcs_startRequestRefDb_t **reqRefDb
)
{
    le_ref_IterRef_t iterRef;
    le_dcs_channelDb_t *channelDb;
    le_dcs_startRequestRefDb_t *refDb;

    *reqRefDb = NULL;

    if (!reqRef)
    {
        return NULL;
    }

    iterRef = le_ref_GetIterator(ChannelRefMap);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        channelDb = (le_dcs_channelDb_t *)le_ref_GetValue(iterRef);
        refDb = DcsGetStartRequestRefDb(reqRef, channelDb);
        if (refDb)
        {
            LE_DEBUG("Found Start Request reference db for reference %p on channel %s",
                     reqRef, channelDb->channelName);
            *reqRefDb = refDb;
            return channelDb;
        }
    }

    LE_DEBUG("Found no channel with Start Request reference %p", reqRef);
    return NULL;
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
    le_dcs_Technology_t tech,
    void *techRef,
    uint16_t *refCount
)
{
    le_ref_IterRef_t iterRef = le_ref_GetIterator(ChannelRefMap);
    le_dcs_channelDb_t *channelDb;

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        channelDb = (le_dcs_channelDb_t *)le_ref_GetValue(iterRef);
        if ((channelDb->technology == tech) && (channelDb->techRef == techRef))
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
    uint16_t channelCount;
    le_dcs_channelDb_t *channelDb = le_dcs_GetChannelDbFromName(channelName, tech);
    if (channelDb)
    {
        LE_DEBUG("ChannelDb reference %p present for channel %s", channelDb->channelRef,
                 channelName);
        return channelDb->channelRef;
    }

    if (le_dcs_GetChannelCount(tech) >= LE_DCS_CHANNEL_LIST_QUERY_MAX)
    {
        LE_WARN("No new channel Db created for channel %s of technology %d as max # (%d) of "
                "channel Dbs supported is reached", channelName, tech, LE_DCS_CHANNELDBS_MAX);
        return NULL;
    }

    channelDb = le_mem_ForceAlloc(ChannelDbPool);
    if (!channelDb)
    {
        LE_ERROR("Unable to alloc channelDb for channel %s", channelName);
        return NULL;
    }
    memset(channelDb, 0, sizeof(le_dcs_channelDb_t));
    channelDb->technology = tech;
    le_utf8_Copy(channelDb->channelName, channelName, sizeof(channelDb->channelName), NULL);
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
    channelDb->startRequestRefList = LE_DLS_LIST_INIT;
    channelCount = le_dcs_IncrementChannelCount(tech);

    LE_DEBUG("ChannelRef %p techRef %p created for channel %s; channel count of tech %d is %d",
             channelDb->channelRef, channelDb->techRef, channelName, tech, channelCount);
    return channelDb->channelRef;
}



//--------------------------------------------------------------------------------------------------
/**
 * Delete a channelDb with the given tech db reference in the argument
 *
 */
//--------------------------------------------------------------------------------------------------
bool le_dcs_DeleteChannelDb
(
    le_dcs_Technology_t tech,
    void *techRef
)
{
    le_dcs_channelDb_t *channelDb;
    le_dcs_ChannelRef_t channelRef = le_dcs_GetChannelRefFromTechRef(tech, techRef);
    if (!channelRef)
    {
        LE_ERROR("Found no channel db reference for tech db reference %p to delete", techRef);
        return false;
    }

    channelDb = le_dcs_GetChannelDbFromRef(channelRef);
    if (!channelDb)
    {
        LE_ERROR("Found no channel db for tech db reference %p to delete", techRef);
        return false;
    }

    LE_INFO("Delete channel db for channel %s with reference %p", channelDb->channelName,
            channelDb->channelRef);
    le_mem_Release(channelDb);
    return true;
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

    DcsChannelQueryHandlerDbList = LE_DLS_LIST_INIT;
    ChannelQueryHandlerDbPool = le_mem_CreatePool("ChannelQueryHandlerDbPool",
                                                  sizeof(DcsChannelQueryHandlerDb_t));
    le_mem_ExpandPool(ChannelQueryHandlerDbPool, LE_DCS_CHANNEL_QUERY_HDLRS_MAX);
    le_mem_SetDestructor(ChannelQueryHandlerDbPool, DcsChannelQueryHandlerDbDestructor);

    // Allocate the Start Request reference db pool, and set the max number of objects
    StartRequestRefDbPool = le_mem_CreatePool("ChannelStartRequestRefDbPool",
                                              sizeof(le_dcs_startRequestRefDb_t));
    le_mem_ExpandPool(StartRequestRefDbPool, LE_DCF_START_REQ_REF_MAP_SIZE);
    le_mem_SetDestructor(StartRequestRefDbPool, DcsStartRequestRefDbDestructor);

    // Init the channel query query time enforcer timer
    ChannelQueryTimeEnforcerTimer = le_timer_Create("ChannelQueryTimeEnforcerTimer");
    le_clk_Time_t ChannelQueryTimeEnforcerMax = {GETCHANNELS_TIME_ENFORCER_LIMIT, 0};
    if ((LE_OK != le_timer_SetHandler(ChannelQueryTimeEnforcerTimer, ChannelQueryTimeEnforcerTimerHandler))
        || (LE_OK != le_timer_SetRepeat(ChannelQueryTimeEnforcerTimer, 1))   // Set as a one shot timer
        || (LE_OK != le_timer_SetInterval(ChannelQueryTimeEnforcerTimer, ChannelQueryTimeEnforcerMax))
        )
    {
        LE_ERROR("Failed to configure the channel query time limit enforcer timer");
    }
    else
    {
        EnforceChannelQueryTimeLimit = true;
    }
}
