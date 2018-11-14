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
 * Global data structures defined here
 */
//--------------------------------------------------------------------------------------------------
DcsInfo_t DcsInfo;


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
    le_result_t ret;
    le_dcs_Technology_t tech;
    le_dcs_channelDb_t *channelDb = dcsGetChannelDbFromName(name, technology);
    if (!channelDb || !name)
    {
        LE_ERROR("Failed to find channel with name %s", name);
        return 0;
    }
    if (channelDb->channelRef == 0)
    {
        LE_ERROR("Channel with name %s found without reference", name);
        return 0;
    }

    ret = dcsGetTechnology(channelDb->channelRef, &tech);
    if (LE_OK != ret)
    {
        LE_ERROR("Failed to get technology type of channel %s; error %d", name, ret);
        return 0;
    }

    if (tech != technology)
    {
        LE_ERROR("Technology type mismatch for channel %s", name);
        return 0;

    }

    LE_DEBUG("Channel %s of technology type %d & reference %p found", name, tech,
             channelDb->channelRef);
    return channelDb->channelRef;
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
    le_dcs_Technology_t tech;
    char *channelName;
    le_dcs_channelDb_t *channelDb = le_dcs_GetChannelDbFromRef(channelRef);
    if (!channelDb)
    {
        LE_ERROR("Invalid channel reference %p for getting technology type", channelRef);
        return LE_DCS_TECH_UNKNOWN;
    }
    channelName = channelDb->channelName;
    tech = channelDb->technology;
    LE_DEBUG("Channel %s is of technology %s", channelName, le_dcs_ConvertTechEnumToName(tech));
    return tech;
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
    char *name,                      //// [OUT] channel's network interface name
    size_t nameSize                  //// [IN] string size of the name above
)
{
    bool netstate;
    le_result_t ret;
    char *channelName;
    le_dcs_channelDb_t *channelDb;

    if (!state)
    {
        LE_ERROR("Failed to get state as the given output string being null");
        return LE_FAULT;
    }

    channelDb = le_dcs_GetChannelDbFromRef(channelRef);
    if (!channelDb)
    {
        LE_ERROR("Invalid channel reference %p for getting channel status", channelRef);
        return LE_FAULT;
    }
    channelName = channelDb->channelName;

    if (!name || (nameSize == 0))
    {
        LE_DEBUG("Skipped getting network interface name as the given output string being null");
    }
    else
    {
        (void)le_dcsTech_GetNetInterface(channelDb->technology, channelRef, name, nameSize);
        if (LE_OK == le_net_GetNetIntfState(name, &netstate))
        {
            LE_DEBUG("Network interface %s has state %s", name, netstate ? "up" : "down");
        }
    }

    ret = dcsGetAdminState(channelRef, state);
    if (LE_OK != ret)
    {
        LE_ERROR("Failed to get admin state of channel %s of technology %s", channelName,
                 le_dcs_ConvertTechEnumToName(channelDb->technology));
    }
    else
    {
        LE_DEBUG("Channel %s of technology %s has network interface %s & state %d", channelName,
                 le_dcs_ConvertTechEnumToName(channelDb->technology), name, *state);
    }
    return ret;
}


//--------------------------------------------------------------------------------------------------
/**
 * Adjust the request count of both the given channel and the global count up or down according
 * to the boolean input argument
 */
//--------------------------------------------------------------------------------------------------
static void DcsAdjustReqCount (le_dcs_channelDb_t *channelDb, bool up)
{
    int16_t indx = le_dcsTech_GetListIndx(channelDb->technology);

    if (indx < 0)
    {
        LE_ERROR("Failed to retrieve info of technology %s to adjust reqCount",
                 le_dcs_ConvertTechEnumToName(channelDb->technology));
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
             le_dcs_ConvertTechEnumToName(channelDb->technology), channelDb->refCount);
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
    CommandData_t cmdData;
    le_dcs_ReqObjRef_t reqRef;
    le_msg_SessionRef_t sessionRef = le_dcs_GetClientSessionRef();
    le_dcs_channelDb_t *channelDb;
    char *channelName, appName[LE_DCS_APPNAME_MAX_LEN] = {0};
    bool inUse;
    pid_t pid;
    uid_t uid;

    channelDb = le_dcs_GetChannelDbFromRef(channelRef);
    if (!channelDb)
    {
        LE_ERROR("Invalid channel reference %p for starting", channelRef);
        return NULL;
    }
    channelName = channelDb->channelName;

    LE_INFO("Starting channel %s of technology %s", channelName,
            le_dcs_ConvertTechEnumToName(channelDb->technology));

    if (((LE_OK == le_msg_GetClientUserCreds(sessionRef, &uid, &pid))) &&
        (LE_OK == le_appInfo_GetName(pid, appName, sizeof(appName)-1)))
    {
        LE_DEBUG("Client app's name %s", appName);
    }

    inUse = ((channelDb->refCount > 0) || channelDb->managed_by_le_data);
    if (inUse && le_dcsTech_GetOpState(channelDb))
    {
        // channel already started; no need to send the request down to the technology again
        LE_INFO("Channel %s already started; refCount %d", channelName, channelDb->refCount);
        DcsAdjustReqCount(channelDb, true);
        dcsChannelEvtHdlrSendNotice(channelDb, sessionRef, LE_DCS_EVENT_UP);
        reqRef = le_ref_CreateRef(RequestRefMap, (void*)sessionRef);
        LE_DEBUG("Channel's session %p, reference %p", sessionRef, reqRef);
        return reqRef;
    }

    // initiate a connect
    DcsAdjustReqCount(channelDb, true);
    cmdData.command = START_COMMAND;
    cmdData.technology = channelDb->technology;
    strncpy(cmdData.channelName, channelName, LE_DCS_CHANNEL_NAME_MAX_LEN);
    le_event_Report(CommandEvent, &cmdData, sizeof(cmdData));
    reqRef = le_ref_CreateRef(RequestRefMap, (void*)sessionRef);
    LE_DEBUG("Channel's session %p, reference %p", sessionRef, reqRef);
    return reqRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Stop for an app its previously started data channel
 *
 * @return
 *     - LE_OK is returned upon a successful release request; otherwise, some other le_result_t
 *       failure cause
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_dcs_Stop
(
    le_dcs_ChannelRef_t channelRef,    ///< [IN] channel started earlier
    le_dcs_ReqObjRef_t reqRef          ///< [IN] Reference to a previously requested channel
)
{
    CommandData_t cmdData;
    le_msg_SessionRef_t sessionRef = le_dcs_GetClientSessionRef();
    le_dcs_channelDb_t *channelDb;
    char *channelName, appName[LE_DCS_APPNAME_MAX_LEN] = {0};
    bool inUse;
    pid_t pid;
    uid_t uid;

    channelDb = le_dcs_GetChannelDbFromRef(channelRef);
    if (!channelDb)
    {
        LE_ERROR("Invalid channel reference %p for stopping", channelRef);
        return LE_FAULT;
    }
    channelName = channelDb->channelName;

    if (!le_ref_Lookup(RequestRefMap, reqRef))
    {
        LE_ERROR("Invalid request reference %p for stopping channel %s of technology %s",
                 reqRef, channelName, le_dcs_ConvertTechEnumToName(channelDb->technology));
        return LE_FAULT;
    }

    le_ref_DeleteRef(RequestRefMap, reqRef);

    LE_INFO("Stopping channel %s of technology %s", channelName,
            le_dcs_ConvertTechEnumToName(channelDb->technology));

    if (((LE_OK == le_msg_GetClientUserCreds(sessionRef, &uid, &pid))) &&
        (LE_OK == le_appInfo_GetName(pid, appName, sizeof(appName)-1)))
    {
        LE_DEBUG("Client app's name %s", appName);
    }

    inUse = ((channelDb->refCount > 1) || channelDb->shared_with_le_data);
    if (inUse)
    {
        // channel still used by other apps; no need to initiate a disconnect
        LE_INFO("Channel %s still used by others; refCount %d", channelName, channelDb->refCount);
        DcsAdjustReqCount(channelDb, false);
        if (channelDb->refCount == 0)
        {
            // transfer ownership to le_data
            channelDb->managed_by_le_data = true;
        }
        dcsChannelEvtHdlrSendNotice(channelDb, sessionRef, LE_DCS_EVENT_DOWN);
        return LE_OK;
    }

    // initiate a disconnect
    DcsAdjustReqCount(channelDb, false);
    cmdData.command = STOP_COMMAND;
    cmdData.technology = channelDb->technology;
    strncpy(cmdData.channelName, channelName, LE_DCS_CHANNEL_NAME_MAX_LEN);
    le_event_Report(CommandEvent, &cmdData, sizeof(cmdData));
    LE_INFO("Channel %s requested to be stopped", channelName);
    return LE_OK;
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
    le_dcs_EventHandlerFunc_t clientHandlerFunc = secondLayerHandlerFunc;

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
le_dcs_EventHandlerRef_t le_dcs_AddEventHandler
(
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
    le_msg_SessionRef_t sessionRef = le_dcs_GetClientSessionRef();
    pid_t pid;
    uid_t uid;

    channelDb = le_dcs_GetChannelDbFromRef(channelRef);
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
                 le_dcs_ConvertTechEnumToName(channelDb->technology));
        return NULL;
    }

    LE_INFO("Adding channel handler for channel %s of technology %s",
            channelName, le_dcs_ConvertTechEnumToName(channelDb->technology));

    channelEvtHdlr = le_dcs_GetChannelAppEvtHdlr(channelDb, sessionRef);
    if (channelEvtHdlr)
    {
        LE_DEBUG("Remove old event handler of channel %s before adding new", channelName);
        le_dls_Remove(&channelDb->evtHdlrs, &(channelEvtHdlr->hdlrLink));
        le_mem_Release(channelEvtHdlr);
    }

    channelEvtHdlr = dcsChannelDbEvtHdlrInit();
    if (!channelEvtHdlr)
    {
        LE_ERROR("Unable to alloc event handler list for channel %s", channelName);
        return NULL;
    }

    if (((LE_OK == le_msg_GetClientUserCreds(sessionRef, &uid, &pid))) &&
        (LE_OK == le_appInfo_GetName(pid, appName, sizeof(appName)-1)))
    {
        LE_DEBUG("Client app's name %s", appName);
    }

    // Each channelDb has its own event for reporting state changes
    snprintf(eventName, sizeof(eventName)-1, "%s:channel:%s", appName, channelName);
    channelEvtHdlr->appSessionRef = sessionRef;
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

    LE_INFO("Event handler with reference %p and event ID %p added", handlerRef,
            channelEvtHdlr->channelEventId);

    return channelEvtHdlr->hdlrRef;
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
    le_dcs_channelDb_t *channelDb = DcsDelChannelEvtHdlr(channelHandlerRef);
    if (channelDb)
    {
        LE_INFO("Channel event handler for channel %s of technology %s removed",
                channelDb->channelName, le_dcs_ConvertTechEnumToName(channelDb->technology));
    }
    else
    {
        LE_ERROR("Channel event handler %p not found for any channel Db",
                 channelHandlerRef);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Query for the list of all available data channels of all supported technology types
 *
 * @return
 *      - LE_OK upon a successful query; otherwise some other le_result_t failure cause
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_dcs_GetList
(
    le_dcs_ChannelInfo_t *channelList, ///< [OUT] list of available channels
    size_t *channelListSize            ///< [INOUT] size of the list
)
{
    le_result_t ret;
    le_dcs_Technology_t tech = LE_DCS_TECH_CELLULAR;

    if (!channelList || !channelListSize || (*channelListSize == 0))
    {
        LE_ERROR("Failed to get list with the given output channel list being null");
        return LE_FAULT;
    }

    ret = le_dcsTech_GetChannelList(tech, channelList, channelListSize);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to get channel list for technology %d; error: %d",
                 tech, ret);
    }
    return ret;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the initial list of all available data channels of all supported technology types for le_data
 *
 * @return
 *      - LE_OK upon a successful query; otherwise some other le_result_t failure cause
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_dcs_InitChannelList
(
    le_dcs_ChannelInfo_t *channelList,
    size_t *listSize
)
{
    return le_dcs_GetList(channelList, listSize);
}


//--------------------------------------------------------------------------------------------------
/**
 *  Server initialization
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    uint16_t i;

    LE_INFO("Data Channel Server is ready");

    memset(&DcsInfo, 0, sizeof(DcsInfo));
    for (i=0; i< LE_DCS_TECH_MAX; i++)
    {
        DcsInfo.techListDb[i].techEnum = i;
        strncpy(DcsInfo.techListDb[i].techName, le_dcs_ConvertTechEnumToName(i),
                LE_DCS_TECH_MAX_NAME_LEN);
    }

    dcsCreateDbPool();
}
