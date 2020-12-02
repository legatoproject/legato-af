//--------------------------------------------------------------------------------------------------
/**
 *  Data Channel Server's C code implementation of the le_dcs APIs
 *
 *  Copyright (C) Sierra Wireless Inc.
 *
 * The Data Channel Server supports two technologies in this version:
 * - the 'Mobile' technology, with a data channel based on the Modem Data Control service (MDC)
 * - the 'Wi-Fi' technology, with a data channel based on the Wifi Client.
 *
 */
//--------------------------------------------------------------------------------------------------

#include <stdlib.h>

#include "legato.h"
#include "interfaces.h"
#include "le_print.h"
#include "dcs.h"
#include "dcsServer.h"
#include "dcsNet.h"

//--------------------------------------------------------------------------------------------------
/**
 * Internal client session reference key's value when invalid or unset
 */
//--------------------------------------------------------------------------------------------------
#define DCS_INTERNAL_CLIENT_SESSION_REF_KEY_INVALID 0

//--------------------------------------------------------------------------------------------------
/**
 * Internal client session name for le_data when it uses le_dcs
 */
//--------------------------------------------------------------------------------------------------
#define DCS_INTERNAL_SESSION_NAME "dataConnectionService"

//--------------------------------------------------------------------------------------------------
/**
 * Global data structures defined here
 */
//--------------------------------------------------------------------------------------------------
DcsInfo_t DcsInfo;

//--------------------------------------------------------------------------------------------------
/**
 * DCS command event type & ID
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t DcsCommandEventId;

typedef enum
{
    DCS_COMMAND_CHANNEL_QUERY = 0        ///< Channel list query request
}
DcsCommandType_t;


//--------------------------------------------------------------------------------------------------
/**
 * DCS command event structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    DcsCommandType_t commandType;                             ///< Command
    void *context;                                            ///< Context
    le_dcs_GetChannelsHandlerFunc_t channelQueryHandlerFunc;  ///< Channel query handler function
}
DcsCommand_t;


//--------------------------------------------------------------------------------------------------
/**
 * Search thru DCS's master list of available technologies for the given technology specified in the
 * input argument by its enum & return its index # on this list
 *
 * @return
 *     - In the function's return value, return the index # found for the given technology, or -1
 *       if not found
 */
//--------------------------------------------------------------------------------------------------
static int16_t DcsGetListIndex
(
    le_dcs_Technology_t technology
)
{
    int i;
    for (i=0; i<LE_DCS_TECHNOLOGY_MAX_COUNT; i++)
    {
        if (DcsInfo.techListDb[i].techEnum == technology)
        {
            return i;
        }
    }
    return -1;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the client session reference key from the client session reference which le_dcs recieved
 * via le_dcs_GetClientSessionRef() or as DCS_INTERNAL_CLIENT_SESSION_REF which is 0.
 *
 * This dcs_GetSessionRefKey() works with dcs_GetSessionRef() together, converting in both
 * directions between a client session reference and its client session reference key.
 *
 * The need for such client session reference key is for le_data's being an internal client
 * session of le_dcs and le_net, for which case such API call's sessionRef is 0. But 0 cannot be
 * used as a reference ID for further DB and reference creations, e.g. creating a channelDb for
 * a client session and then creating a reference for this channelDb, i.e. calling
 * le_ref_CreateRef(map, 0) with 0 as the 2nd argument. Thus, the need for this
 * dcs_GetSessionRefKey() to generate a session reference key from a client sessionRef.
 *
 * The requirements for dcs_GetSessionRefKey() are (1) uniqueness for its generated keys; (2)
 * bidirectionally mappable & recoverable; (3) non-zero keys generated. A conventional way is to
 * hash(sessionRef) to get sessionRefKey, but that is more complex than necessary. Thus, the
 * algorithm used here is just +1 to sessionRef to get sessionRefKey, as no sessionRef is
 * anticipated to have the value (uint)(0-1) to fail requirement #3 above.
 *
 * @return
 *     - The session reference key for the given session reference, which should never be used as
 *       a pointer address although it's of the void* type.
 */
//--------------------------------------------------------------------------------------------------
void* dcs_GetSessionRefKey
(
    le_msg_SessionRef_t sessionRef      ///< [IN] an le_dcs client's session reference
)
{
    void* sessionRefKey = (void*)((uint8_t*)sessionRef + 1);
    LE_ASSERT(sessionRefKey != DCS_INTERNAL_CLIENT_SESSION_REF_KEY_INVALID);
    return sessionRefKey;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the client session reference back from the client session reference key that was resulted
 * from le_dcs's earlier call dcs_GetSessionRefKey(le_dcs_GetClientSessionRef()).
 *
 * This dcs_GetSessionRef() works with dcs_GetSessionRefKey() together, converting in both
 * directions between a client session reference key and its client session reference.
 *
 * Regarding the algorithm used in this function-pair, please refer to dcs_GetSessionRefKey()'s
 * header comment above for more details.
 *
 * @return
 *     - The session reference for the given session reference key from which it was generated via
 *       dcs_GetSessionRefKey().
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionRef_t dcs_GetSessionRef
(
    void* sessionRefKey                 ///< [IN] the session reference key of an le_dcs client
)
{
    LE_ASSERT(sessionRefKey != DCS_INTERNAL_CLIENT_SESSION_REF_KEY_INVALID);
    le_msg_SessionRef_t sessionRef = (le_msg_SessionRef_t)((uint8_t*)sessionRefKey - 1);
    return sessionRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the number of available channels of the given technology
 *
 * @return
 *     - channel count of the given technology
 */
//--------------------------------------------------------------------------------------------------
uint16_t dcs_GetChannelCount
(
    le_dcs_Technology_t tech
)
{
    return DcsInfo.techListDb[tech].channelCount;
}


//--------------------------------------------------------------------------------------------------
/**
 * Increment the channel count of the given technology and return its up-to-date value
 *
 * @return
 *     - updated channel count of the given technology
 */
//--------------------------------------------------------------------------------------------------
uint16_t dcs_IncrementChannelCount
(
    le_dcs_Technology_t tech
)
{
    return ++DcsInfo.techListDb[tech].channelCount;
}


//--------------------------------------------------------------------------------------------------
/**
 * Decrement the channel count of the given technology and return LE_OK if successful. Otherwise,
 * LE_OUT_OF_RANGE to indicate an already zero count.
 *
 * @return
 *     - LE_OK if the decrement is successful
 *     - LE_OUT_OF_RANGE if the channel count is already 0 and can't be further decremented
 */
//--------------------------------------------------------------------------------------------------
le_result_t dcs_DecrementChannelCount
(
    le_dcs_Technology_t tech,
    uint16_t *newCount
)
{
    if (DcsInfo.techListDb[tech].channelCount == 0)
    {
        // thought out of bound, the up-to-date count continues to be 0 in case the caller checks
        *newCount = 0;
        return LE_OUT_OF_RANGE;
    }
    *newCount = --DcsInfo.techListDb[tech].channelCount;
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Query for the channel reference of a channel given by its name
 *
 * @return
 *      - The function returns the given channel's technology type found in its return value
 */
//--------------------------------------------------------------------------------------------------
le_dcs_ChannelRef_t dcs_GetReference
(
    const char *name,                 ///< [IN] name of channel which reference is to be retrieved
    le_dcs_Technology_t technology    ///< [IN] technology of the channel given by its name above
)
{
    le_dcs_channelDb_t *channelDb = dcs_GetChannelDbFromName(name, technology);
    if (!channelDb || !name)
    {
        LE_WARN("Channel with name %s of technology %d not created yet", name, technology);
        return 0;
    }
    if (!channelDb->channelRef)
    {
        LE_ERROR("Channel with name %s found without reference", name);
        return NULL;
    }

    LE_DEBUG("Channel %s of technology type %d & reference %p found", name, technology,
             channelDb->channelRef);
    return channelDb->channelRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Query for the channel reference of a channel given by its name
 *
 * @return
 *      - The function returns the given channel's technology type found in its return value
 */
//--------------------------------------------------------------------------------------------------
le_dcs_ChannelRef_t le_dcs_GetReference
(
    const char *name,                 ///< [IN] name of channel which reference is to be retrieved
    le_dcs_Technology_t technology    ///< [IN] technology of the channel given by its name above
)
{
    return dcs_GetReference(name, technology);
}


//--------------------------------------------------------------------------------------------------
/**
 * Query for a given channel's technology type
 *
 * @return
 *      - The function returns the given channel's technology type found in its return value
 */
//--------------------------------------------------------------------------------------------------
le_dcs_Technology_t le_dcs_GetTechnology
(
    le_dcs_ChannelRef_t channelRef  ///< [IN] channel which technology type is to be queried
)
{
    le_dcs_channelDb_t *channelDb = dcs_GetChannelDbFromRef(channelRef);
    if (!channelDb)
    {
        LE_ERROR("Invalid channel reference %p for getting technology type", channelRef);
        return LE_DCS_TECH_UNKNOWN;
    }

    LE_DEBUG("Channel %s is of technology %s",
             channelDb->channelName, dcs_ConvertTechEnumToName(channelDb->technology));
    return channelDb->technology;
}


//--------------------------------------------------------------------------------------------------
/**
 * Query for the state of the given channel in the 1st input argument, which is the admin state
 * of the channel meaning whether 1 or more apps are using it
 *
 * @return
 *      - The function returns the given channel's admin state and associated network interface's
 *        name in the function's 2nd and 3rd arguments while the 4th argument specifies how long
 *        the buffer is for the interface name in the 3rd
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_dcs_GetState
(
    le_dcs_ChannelRef_t channelRef,  ///< [IN] channel which status is to be queried
    le_dcs_State_t *state,           //// [OUT] channel state returned as output
    char *interfaceName,             //// [OUT] channel's network interface name
    size_t interfaceNameSize         //// [IN] string size of the name above
)
{
    bool netstate;
    le_result_t ret;
    le_dcs_channelDb_t *channelDb;

    if (!state)
    {
        LE_ERROR("Failed to get state as the given output string being null");
        return LE_FAULT;
    }

    channelDb = dcs_GetChannelDbFromRef(channelRef);
    if (!channelDb)
    {
        LE_ERROR("Invalid channel reference %p for getting channel status", channelRef);
        return LE_FAULT;
    }

    if (!interfaceName || (interfaceNameSize == 0))
    {
        LE_DEBUG("Skipped getting network interface name as the given output string being null");
    }
    else
    {
        (void)dcsTech_GetNetInterface(channelDb->technology, channelRef, interfaceName,
                                         interfaceNameSize);
        if (LE_OK == net_GetNetIntfState(interfaceName, &netstate))
        {
            LE_DEBUG("Network interface %s has state %s", interfaceName, netstate ? "up" : "down");
        }
    }

    ret = dcs_GetAdminState(channelRef, state);
    if (LE_OK != ret)
    {
        LE_ERROR("Failed to get admin state of channel %s of technology %s", channelDb->channelName,
                 dcs_ConvertTechEnumToName(channelDb->technology));
    }
    else
    {
        LE_DEBUG("Channel %s of technology %s has network interface %s & state %d",
                 channelDb->channelName,
                 dcs_ConvertTechEnumToName(channelDb->technology), interfaceName, *state);
    }
    return ret;
}


//--------------------------------------------------------------------------------------------------
/**
 * Return le_dcs's request count back to the caller
 *
 * @return
 *     - the count of channel requests made thru le_dcs API
 */
//--------------------------------------------------------------------------------------------------
uint16_t dcs_GetReqCount
(
    void
)
{
    return DcsInfo.reqCount;
}


//--------------------------------------------------------------------------------------------------
/**
 * Adjust the request count of both the given channel and the global count up or down according
 * to the boolean input argument
 */
//--------------------------------------------------------------------------------------------------
void dcs_AdjustReqCount
(
    le_dcs_channelDb_t *channelDb,
    bool up
)
{
    int16_t indx = DcsGetListIndex(channelDb->technology);

    if (indx < 0)
    {
        LE_ERROR("Failed to retrieve info of technology %s to adjust reqCount",
                 dcs_ConvertTechEnumToName(channelDb->technology));
        return;
    }

    if (up)
    {
        channelDb->refCount++;
        DcsInfo.techListDb[indx].reqCount++;
        DcsInfo.reqCount++;
    }
    else
    {
        channelDb->refCount = (channelDb->refCount > 0) ? (channelDb->refCount-1) : 0;
        DcsInfo.techListDb[indx].reqCount = (DcsInfo.techListDb[indx].reqCount > 0) ?
            (DcsInfo.techListDb[indx].reqCount-1) : 0;
        DcsInfo.reqCount = (DcsInfo.reqCount > 0) ? (DcsInfo.reqCount-1) : 0;
    }
    LE_DEBUG("System request count %d; channel %s of technology %s with refcount %d",
             DcsInfo.reqCount, channelDb->channelName,
             dcs_ConvertTechEnumToName(channelDb->technology), channelDb->refCount);
}


//--------------------------------------------------------------------------------------------------
/**
 * Request by an app to start a data channel
 *
 * @return
 *      - Object reference to the request (to be used later for releasing the channel)
 *      - NULL if it has failed to process the request
 */
//--------------------------------------------------------------------------------------------------
le_dcs_ReqObjRef_t dcs_Start
(
    le_msg_SessionRef_t sessionRef,        ///< [IN] messaging session making request
    le_dcs_ChannelRef_t channelRef         ///< [IN] channel requested to be started
)
{
    CommandData_t cmdData;
    le_dcs_ReqObjRef_t reqRef;
    le_dcs_channelDb_t *channelDb;
    le_result_t ret;
    char *channelName, appName[LE_DCS_APPNAME_MAX_LEN] = {0};
    pid_t pid = 0;
    uid_t uid = 0;
    void *sessionRefKey = dcs_GetSessionRefKey(sessionRef);

    channelDb = dcs_GetChannelDbFromRef(channelRef);
    if (!channelDb)
    {
        LE_ERROR("Invalid channel reference %p for starting", channelRef);
        return NULL;
    }
    channelName = channelDb->channelName;

    LE_INFO("Starting channel %s of technology %s by app session with reference %p", channelName,
            dcs_ConvertTechEnumToName(channelDb->technology), sessionRef);

    if (sessionRef &&
        (LE_OK == le_msg_GetClientUserCreds(sessionRef, &uid, &pid)) &&
        (LE_OK == le_appInfo_GetName(pid, appName, sizeof(appName)-1)))
    {
        LE_DEBUG("Client app's name %s", appName);
    }

    if ((channelDb->refCount > 0) && dcsTech_GetOpState(channelDb))
    {
        // channel already started; no need to send the request down to the technology again
        reqRef = le_ref_CreateRef(dcs_GetRequestRefMap(), sessionRefKey);
        if (!dcs_AddStartRequestRef(reqRef, channelDb))
        {
            LE_ERROR("Failed to record Start Request reference");
            le_ref_DeleteRef(dcs_GetRequestRefMap(), reqRef);
            return NULL;
        }

        LE_INFO("Channel %s already started; refCount %d", channelName, channelDb->refCount);
        dcs_AdjustReqCount(channelDb, true);
        if (dcsTech_GetOpState(channelDb))
        {
            // Only send apps the Up notification when the state is up. Otherwise, the channel is
            // in the process of upcoming that this notification will be sent when it's up
            dcs_ChannelEvtHdlrSendNotice(channelDb, sessionRef, LE_DCS_EVENT_UP);
        }
        LE_DEBUG("Channel's session %p, reference %p", sessionRef, reqRef);
        dcs_SessionCleanupSaveReqRef(appName, sessionRef, channelDb, reqRef);
        return reqRef;
    }

    // Do an early check with the technology in the present running thread & context to see if it
    // allows this channel start prior to calling le_event_Report() below so that rejection can be
    // known as early as possible
    ret = dcsTech_AllowChannelStart(channelDb->technology, channelName);
    if ((ret != LE_OK) && (ret != LE_DUPLICATE))
    {
        LE_ERROR("Technology %s rejected the new Start Request on channel %s; error %d",
                 dcs_ConvertTechEnumToName(channelDb->technology), channelName, ret);
        return NULL;
    }

    // initiate a connect
    reqRef = le_ref_CreateRef(dcs_GetRequestRefMap(), sessionRefKey);
    if (!dcs_AddStartRequestRef(reqRef, channelDb))
    {
        LE_ERROR("Failed to record Start Request reference");
        le_ref_DeleteRef(dcs_GetRequestRefMap(), reqRef);
        return NULL;
    }
    dcs_AdjustReqCount(channelDb, true);
    cmdData.command = START_COMMAND;
    cmdData.technology = channelDb->technology;
    strncpy(cmdData.channelName, channelName, LE_DCS_CHANNEL_NAME_MAX_LEN);
    le_event_Report(dcs_GetCommandEventId(), &cmdData, sizeof(cmdData));
    LE_INFO("Initiating technology to start channel %s for app session %p, request reference %p",
            channelName, sessionRef, reqRef);
    dcs_SessionCleanupSaveReqRef(appName, sessionRef, channelDb, reqRef);
    return reqRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Request by an app to start a data channel
 *
 * @return
 *      - Object reference to the request (to be used later for releasing the channel)
 *      - NULL if it has failed to process the request
 */
//--------------------------------------------------------------------------------------------------
le_dcs_ReqObjRef_t le_dcs_Start
(
    le_dcs_ChannelRef_t channelRef         ///< [IN] channel requested to be started
)
{
    return dcs_Start(le_dcs_GetClientSessionRef(), channelRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Stop for an app its previously started data channel
 *
 * @return
 *     - LE_OK is returned upon a successful release request; otherwise, LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
le_result_t dcs_Stop
(
    le_msg_SessionRef_t sessionRef,    ///< [IN] Messaging session making stop request
    le_dcs_ReqObjRef_t reqRef          ///< [IN] Reference to a previously requested channel
)
{
    CommandData_t cmdData;
    le_dcs_channelDb_t *channelDb;
    char *channelName, appName[LE_DCS_APPNAME_MAX_LEN] = {0};
    bool inUse;
    pid_t pid;
    uid_t uid;

    le_dcs_startRequestRefDb_t *reqRefDb;
    channelDb = dcs_GetChannelDbFromStartRequestRef(reqRef, &reqRefDb);
    if (!channelDb)
    {
        LE_ERROR("Invalid request reference %p for stopping", reqRef);
        return LE_FAULT;
    }
    channelName = channelDb->channelName;

    if (!le_ref_Lookup(dcs_GetRequestRefMap(), reqRef))
    {
        LE_ERROR("Invalid request reference %p for stopping channel %s of technology %s",
                 reqRef, channelName, dcs_ConvertTechEnumToName(channelDb->technology));
        return LE_FAULT;
    }

    le_ref_DeleteRef(dcs_GetRequestRefMap(), reqRef);
    if (!dcs_DeleteStartRequestRef(reqRefDb, channelDb))
    {
        LE_ERROR("Failed to delete Start Request reference %p from channel %s", reqRef,
                 channelName);
    }
    else
    {
        reqRefDb = NULL;
    }

    LE_INFO("Stopping channel %s of technology %s", channelName,
            dcs_ConvertTechEnumToName(channelDb->technology));

    if (sessionRef &&
        (LE_OK == le_msg_GetClientUserCreds(sessionRef, &uid, &pid)) &&
        (LE_OK == le_appInfo_GetName(pid, appName, sizeof(appName)-1)))
    {
        LE_DEBUG("Client app's name %s", appName);
    }

    inUse = (channelDb->refCount > 1);
    if (inUse)
    {
        // channel still used by other apps; no need to initiate a disconnect
        LE_INFO("Channel %s still used by others; refCount %d", channelName, channelDb->refCount);
        dcs_AdjustReqCount(channelDb, false);
        dcs_ChannelEvtHdlrSendNotice(channelDb, sessionRef, LE_DCS_EVENT_DOWN);
        return LE_OK;
    }

    // initiate a disconnect
    dcs_AdjustReqCount(channelDb, false);
    cmdData.command = STOP_COMMAND;
    cmdData.technology = channelDb->technology;
    strncpy(cmdData.channelName, channelName, LE_DCS_CHANNEL_NAME_MAX_LEN);
    le_event_Report(dcs_GetCommandEventId(), &cmdData, sizeof(cmdData));
    LE_INFO("Channel %s requested to be stopped", channelName);
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Stop for an app its previously started data channel
 *
 * @return
 *     - LE_OK is returned upon a successful release request; otherwise, LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_dcs_Stop
(
    le_dcs_ReqObjRef_t reqRef          ///< [IN] Reference to a previously requested channel
)
{
    return dcs_Stop(le_dcs_GetClientSessionRef(), reqRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * The first-layer channel event handler
 */
//--------------------------------------------------------------------------------------------------
static void DcsFirstLayerEventHandler (void *reportPtr, void *secondLayerHandlerFunc)
{
    le_dcs_channelDbEventReport_t *evtReport = reportPtr;
    le_dcs_channelDb_t *channelDb;
    le_dcs_EventHandlerFunc_t clientHandlerFunc = (le_dcs_EventHandlerFunc_t)secondLayerHandlerFunc;

    channelDb = evtReport->channelDb;
    if (!channelDb)
    {
        return;
    }
    clientHandlerFunc(channelDb->channelRef, evtReport->event, 0, le_event_GetContextPtr());
}


//--------------------------------------------------------------------------------------------------
/**
 * This function adds a channel event handler
 *
 * @return
 *     - It returns a reference pointer to the added handler upon success; otherwise, NULL
 */
//--------------------------------------------------------------------------------------------------
le_dcs_EventHandlerRef_t dcs_AddEventHandler
(
    le_msg_SessionRef_t sessionRef,    ///< [IN] Messaging session making stop request
    le_dcs_ChannelRef_t channelRef,     ///< [IN] the channel for which the event is
    le_dcs_EventHandlerFunc_t channelHandlerPtr, ///< [IN] Handler pointer
    void *contextPtr                    ///< [IN] Associated user context pointer
)
{
    char eventName[LE_DCS_APPNAME_MAX_LEN + LE_DCS_CHANNEL_NAME_MAX_LEN + 10] = {0};
    le_dcs_channelDb_t *channelDb;
    char *channelName, appName[LE_DCS_APPNAME_MAX_LEN] = {0};
    le_event_HandlerRef_t handlerRef;
    le_dcs_channelDbEventHdlr_t *channelEvtHdlr;
    pid_t pid;
    uid_t uid;
    void *sessionRefKey = dcs_GetSessionRefKey(sessionRef);

    channelDb = dcs_GetChannelDbFromRef(channelRef);
    if (!channelDb)
    {
        LE_ERROR("Failed to find the Db for channel reference %p to add a handler",
                 channelRef);
        return NULL;
    }
    channelName = channelDb->channelName;

    if (!channelHandlerPtr)
    {
        LE_ERROR("Event handler can't be null for channel %s of technology %s", channelName,
                 dcs_ConvertTechEnumToName(channelDb->technology));
        return NULL;
    }

    LE_INFO("Adding channel handler for channel %s of technology %s",
            channelName, dcs_ConvertTechEnumToName(channelDb->technology));

    channelEvtHdlr = dcs_GetChannelAppEvtHdlr(channelDb, sessionRefKey);
    if (channelEvtHdlr)
    {
        LE_DEBUG("Remove old event handler ref %p of channel %s before adding new",
                 channelEvtHdlr->hdlrRef, channelName);
        le_dls_Remove(&channelDb->evtHdlrs, &(channelEvtHdlr->hdlrLink));
        le_mem_Release(channelEvtHdlr);
    }

    channelEvtHdlr = dcs_ChannelDbEvtHdlrInit();
    if (!channelEvtHdlr)
    {
        LE_ERROR("Unable to alloc event handler list for channel %s", channelName);
        return NULL;
    }

    if (sessionRef &&
        (LE_OK == le_msg_GetClientUserCreds(sessionRef, &uid, &pid)) &&
        (LE_OK == le_appInfo_GetName(pid, appName, sizeof(appName)-1)))
    {
        LE_DEBUG("Client app's name %s", appName);
    }

    // Each channelDb has its own event for reporting state changes
    snprintf(eventName, sizeof(eventName)-1, "%s:channel:%s", appName, channelName);
    channelEvtHdlr->appSessionRefKey = sessionRefKey;
    channelEvtHdlr->channelEventId = le_event_CreateId(eventName,
                                                       sizeof(le_dcs_channelDbEventReport_t));
    channelEvtHdlr->channelEventHdlr = channelHandlerPtr;
    channelEvtHdlr->hdlrLink = LE_DLS_LINK_INIT;
    handlerRef = le_event_AddLayeredHandler("le_dcs_EventHandler",
                                            channelEvtHdlr->channelEventId,
                                            DcsFirstLayerEventHandler,
                                            (le_event_HandlerFunc_t)channelHandlerPtr);
    channelEvtHdlr->hdlrRef = (le_dcs_EventHandlerRef_t)handlerRef;
    le_dls_Queue(&channelDb->evtHdlrs, &(channelEvtHdlr->hdlrLink));
    le_event_SetContextPtr(handlerRef, contextPtr);

    dcs_SessionCleanupSaveEventHandler(appName, sessionRef, channelDb, handlerRef);

    LE_INFO("Event handler with reference %p and event ID %p added", handlerRef,
            channelEvtHdlr->channelEventId);

    return channelEvtHdlr->hdlrRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function adds a channel event handler
 *
 * @return
 *     - It returns a reference pointer to the added handler upon success; otherwise, NULL
 */
//--------------------------------------------------------------------------------------------------
le_dcs_EventHandlerRef_t le_dcs_AddEventHandler
(
    le_dcs_ChannelRef_t channelRef,     ///< [IN] the channel for which the event is
    le_dcs_EventHandlerFunc_t channelHandlerPtr, ///< [IN] Handler pointer
    void *contextPtr                    ///< [IN] Associated user context pointer
)
{
    return dcs_AddEventHandler(le_dcs_GetClientSessionRef(),
                               channelRef, channelHandlerPtr,
                               contextPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * This function removes the channel event handler given via a reference object in the argument
 */
//--------------------------------------------------------------------------------------------------
void dcs_RemoveEventHandler
(
    le_msg_SessionRef_t sessionRef,              ///< [IN] Messaging session making stop request
    le_dcs_EventHandlerRef_t channelHandlerRef   ///< [IN] Channel event handler reference
)
{
    le_dcs_channelDb_t *channelDb = dcs_GetChannelEvtHdlr(channelHandlerRef, false);

    if (!channelDb)
    {
        LE_ERROR("Channel event handler %p not found for any channel Db", channelHandlerRef);
        return;
    }

    if (dcs_GetChannelEvtHdlr(channelHandlerRef, true))
    {
        LE_DEBUG("Client's event handler ref %p for channel %s of technology %s removed",
                 channelHandlerRef, channelDb->channelName,
                 dcs_ConvertTechEnumToName(channelDb->technology));
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * This function removes the channel event handler given via a reference object in the argument
 */
//--------------------------------------------------------------------------------------------------
void le_dcs_RemoveEventHandler
(
    le_dcs_EventHandlerRef_t channelHandlerRef   ///< [IN] Channel event handler reference
)
{
    dcs_RemoveEventHandler(le_dcs_GetClientSessionRef(),
                           channelHandlerRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Initiate a channel list query by posting a query request command to DCS to get it query all the
 * supported technologies underneath for their list of available channels
 */
//--------------------------------------------------------------------------------------------------
void le_dcs_GetChannels
(
    le_dcs_GetChannelsHandlerFunc_t handlerPtr,  ///< [IN] requester's handler for receiving results
    void *contextPtr                             ///< [IN] requester's context
)
{
    DcsCommand_t cmd = {
        .commandType = DCS_COMMAND_CHANNEL_QUERY,
        .channelQueryHandlerFunc = handlerPtr,
        .context = contextPtr
    };

    LE_DEBUG("Send channel list query command of type %d to DCS", DCS_COMMAND_CHANNEL_QUERY);
    le_event_Report(DcsCommandEventId, &cmd, sizeof(cmd));
}


//--------------------------------------------------------------------------------------------------
/**
 * Trigger a query for the list of available data channels of all supported technology types
 *
 * @return
 *      - LE_OK upon a successful query; otherwise some other le_result_t failure cause
 */
//--------------------------------------------------------------------------------------------------
void GetChannelList
(
    void
)
{
    le_result_t ret;
    le_dcs_Technology_t i;

    if (dcs_ChannelQueryIsRunning()) {
        // GetChannels is already in action; thus, do not retrigger another round & just return
        return;
    }

    for (i = LE_DCS_TECH_UNKNOWN; i < LE_DCS_TECH_MAX; i++)
    {
        ret = dcsTech_GetChannelList(i);
        if (ret != LE_OK)
        {
            LE_WARN("Failed to trigger a query for available channels of technology %d, "
                    "error: %d", i, ret);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Handler to process a DCS command event
 */
//--------------------------------------------------------------------------------------------------
static void DcsCommandHandler
(
    void *command
)
{
    DcsCommand_t *commandPtr = command;
    switch (commandPtr->commandType)
    {
        case DCS_COMMAND_CHANNEL_QUERY:
            if (!commandPtr->channelQueryHandlerFunc)
            {
                LE_DEBUG("No handler for returning channel query results");
                return;
            }
            LE_DEBUG("Process a channel list query");
            dcs_AddChannelQueryHandlerDb(commandPtr->channelQueryHandlerFunc,
                                         commandPtr->context);
            GetChannelList();
            break;
        default:
            LE_ERROR("Invalid command type %d", commandPtr->commandType);
            break;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Handler function for closing the given service session
 */
//--------------------------------------------------------------------------------------------------
static void CloseServiceSessionHandler
(
    le_msg_SessionRef_t closingSessionRef,
    void*               contextPtr
)
{
    void *closingSessionRefKey = dcs_GetSessionRefKey(closingSessionRef);

    LE_INFO("Client %p killed, remove allocated resources", closingSessionRef);

    if (!closingSessionRef)
    {
        LE_ERROR("Failed resource clean up for a null sessionRef upon session closure");
        return;
    }

    // Search the data reference used by the killed client
    le_ref_IterRef_t iterRef = le_ref_GetIterator(dcs_GetRequestRefMap());
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        void *sessionRefKey = le_ref_GetValue(iterRef);
        le_dcs_ReqObjRef_t reqRef = (le_dcs_ReqObjRef_t)le_ref_GetSafeRef(iterRef);

        // Check if the session reference saved matches with the current session reference
        if (sessionRefKey == closingSessionRefKey)
        {
            le_dcs_startRequestRefDb_t *reqRefDb;
            le_dcs_channelDb_t *channelDb = dcs_GetChannelDbFromStartRequestRef(reqRef, &reqRefDb);
            if (channelDb)
            {
                // Check session cleanup filtering config to see if this channel should be closed
                if (!dcs_IsSessionExitChannelClosable(channelDb, reqRef))
                {
                    LE_DEBUG("Do not stop for app client with session ref %p & request ref %p "
                             "for its channel %s", closingSessionRef, reqRef,
                             channelDb->channelName);
                    continue;
                }
            }

            LE_INFO("Stopping for app client with session ref %p & request ref %p for "
                    "its channel %s", closingSessionRef, reqRef, channelDb->channelName);
            le_dcs_Stop(reqRef);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 *  Component initialization called only once.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT_ONCE
{
    dcs_InitDbPools();
}

//--------------------------------------------------------------------------------------------------
/**
 *  Server initialization
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    le_dcs_Technology_t i;

    memset(&DcsInfo, 0, sizeof(DcsInfo));
    for (i = LE_DCS_TECH_UNKNOWN; i< LE_DCS_TECH_MAX; i++)
    {
        DcsInfo.techListDb[i].techEnum = i;
        strncpy(DcsInfo.techListDb[i].techName, dcs_ConvertTechEnumToName(i),
                LE_DCS_TECH_MAX_NAME_LEN);
    }

    dcs_InitDbTimers();

    dcs_cleanConfigTree();

    // Add a handler to the close session service
    le_msg_AddServiceCloseHandler(le_dcs_GetServiceRef(), CloseServiceSessionHandler, NULL);

    DcsCommandEventId = le_event_CreateId("DcsCommandEventId", sizeof(DcsCommand_t));
    le_event_AddHandler("DcsCommand", DcsCommandEventId, DcsCommandHandler);

    LE_INFO("Data Channel Service le_dcs is ready");
}
