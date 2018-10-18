/**
 * Data Connection Server Unit-test app/component: crossClient.c
 *
 * This module implements the unit-test code for DCS's le_dcs APIs, which is built into a test
 * component to run on the target to start, stop, etc., data channels in different sequence &
 * timing.
 * This could/would be used as one of the few test apps, alongside the others, to run in
 * parallel to simulate scenarios having multiple apps using DCS simultaneously. One such
 * counterpart is apps/test/dataConnectionService/dcsAPITests/dcsClient/client.c.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include <string.h>
#include "legato.h"
#include "interfaces.h"

static le_dcs_EventHandlerRef_t eventHandlerRef = NULL;
static le_dcs_ChannelRef_t myChannel;
static char channelName[LE_DCS_CHANNEL_NAME_MAX_LEN];
static le_dcs_ReqObjRef_t reqObj;
static le_data_RequestObjRef_t myReqRef;
static le_data_ConnectionStateHandlerRef_t connStateHandlerRef = NULL;


//--------------------------------------------------------------------------------------------------
/**
 *  This is the event handler used & to be added in the test of DCS's NB API
 *  le_dcs_AddEventHandler() for a channel
 */
//--------------------------------------------------------------------------------------------------
void clientEventHandler
(
    le_dcs_ChannelRef_t channelRef, ///< [IN] The channel for which the event is
    le_dcs_Event_t event,           ///< [IN] Event up or down
    int32_t code,                   ///< [IN] Additional event code, like error code
    void *contextPtr                ///< [IN] Associated user context pointer
)
{
    LE_INFO("DCS-client: received for channel reference %p event %s", channelRef,
            (event == LE_DCS_EVENT_UP) ? "Up" : "Down");
}


//--------------------------------------------------------------------------------------------------
/**
 *  This function tests DCS's NB API le_dcs_GetReference() for querying the channel name of
 *  the given channel
 */
//--------------------------------------------------------------------------------------------------
void dcs_test_api_GetReference
(
    void *param1,
    void *param2
)
{
    le_dcs_ChannelRef_t ret_ref;
    LE_INFO("DCS-client: asking for channel reference for channel %s", channelName);
    ret_ref = le_dcs_GetReference(channelName, LE_DCS_TECH_CELLULAR);
    LE_INFO("DCS-client: returned channel reference: %p", ret_ref);
}


//--------------------------------------------------------------------------------------------------
/**
 *  This function tests DCS's NB API le_dcs_GetTechnology() for querying the technology type of
 *  the given channel
 */
//--------------------------------------------------------------------------------------------------
void dcs_test_api_GetTechnology
(
    void *param1,
    void *param2
)
{
    le_dcs_Technology_t retTech = -1;

    LE_INFO("DCS-client: asking for tech type");
    retTech = le_dcs_GetTechnology(myChannel);
    LE_INFO("DCS-client: returned tech type: %d", (int)retTech);
}


//--------------------------------------------------------------------------------------------------
/**
 *  This function tests DCS's NB API le_dcs_GetState() for querying the channel status of
 *  the given channel
 */
//--------------------------------------------------------------------------------------------------
void dcs_test_api_GetState
(
    void *param1,
    void *param2
)
{
    le_dcs_State_t state;
    char name[LE_DCS_INTERFACE_NAME_MAX_LEN+1];
    le_result_t ret;

    LE_INFO("DCS-client: asking for channel status");
    ret = le_dcs_GetState(myChannel, &state, name, LE_DCS_INTERFACE_NAME_MAX_LEN);
    LE_INFO("DCS-client: returned for channel %s netIntf %s status %d (rc %d)", channelName,
            name, state, (int)ret);

}


//--------------------------------------------------------------------------------------------------
/**
 *  This function tests DCS's NB APIs le_dcs_Start() for starting the given data channel
 */
//--------------------------------------------------------------------------------------------------
void dcs_test_api_Start
(
    void *param1,
    void *param2
)
{
    LE_INFO("DCS-client: asking to start channel %s", channelName);
    reqObj = le_dcs_Start(myChannel);
    LE_INFO("DCS-client: returned RequestObj %p", reqObj);
}


//--------------------------------------------------------------------------------------------------
/**
 *  This function tests DCS's NB API le_dcs_Stop() for stopping the given data channel
 */
//--------------------------------------------------------------------------------------------------
void dcs_test_api_Stop
(
    void *param1,
    void *param2
)
{
    le_result_t ret;

    LE_INFO("DCS-client: asking to stop channel %s", channelName);

    if (!reqObj)
    {
        LE_ERROR("DCS-client: error in stopping channel %s with a null object", channelName);
        return;
    }

    ret = le_dcs_Stop(myChannel, reqObj);
    LE_INFO("DCS-client: got for channel %s release status %d", channelName, ret);
}


//--------------------------------------------------------------------------------------------------
/**
 *  This function tests DCS's NB API le_dcs_AddEventHandler() for adding a channel event handler
 *  for the given channel
 */
//--------------------------------------------------------------------------------------------------
void dcs_test_api_AddEventHandler
(
    void *param1,
    void *param2
)
{
    LE_INFO("DCS-client: asking to add event handler for channel %s", channelName);
    eventHandlerRef = le_dcs_AddEventHandler(myChannel, clientEventHandler, NULL);
    LE_INFO("DCS-client: channel event handler added %p for channel %s", eventHandlerRef,
            channelName);
}


//--------------------------------------------------------------------------------------------------
/**
 *  This function tests DCS's NB API le_dcs_RemoveEventHandler() for removing a channel event
 *  handler for the given channel
 */
//--------------------------------------------------------------------------------------------------
void dcs_test_api_RmEventHandler
(
    void *param1,
    void *param2
)
{
    LE_INFO("DCS-client: asking to remove event handler for channel %s", channelName);
    if (!eventHandlerRef)
    {
        LE_INFO("DCS-client: no channel event handler to remove");
        return;
    }

    le_dcs_RemoveEventHandler(eventHandlerRef);
    eventHandlerRef = NULL;
    LE_INFO("DCS-client: Done removing event handler");
}


//--------------------------------------------------------------------------------------------------
/**
 *  This function tests DCS's NB API le_dcs_GetList() for querying the entire list of all
 *  channels available on the device
 */
//--------------------------------------------------------------------------------------------------
void dcs_test_api_GetList
(
    void *param1,
    void *param2
)
{
    uint16_t i;
    le_result_t ret;
    le_dcs_ChannelInfo_t channelList[LE_DCS_CHANNEL_LIST_ENTRY_MAX];
    size_t listLen = 10;

    LE_INFO("DCS-client: asking to return a complete channel list");
    ret = le_dcs_GetList(channelList, &listLen);
    LE_INFO("DCS-client: got channel list of size %d (rc %d)", listLen, (int)ret);
    for (i = 0; i < listLen; i++)
    {
        LE_INFO("DCS-client: available channel #%d with name %s, technology %d, state %d",
                i + 1, channelList[i].name, channelList[i].technology, channelList[i].state);
    }
    if (listLen)
    {
        strncpy(channelName, channelList[listLen - 1].name, LE_DCS_CHANNEL_NAME_MAX_LEN);
        myChannel = channelList[listLen - 1].ref;
    }
    else
    {
        channelName[0] = '\0';
        myChannel = 0;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 *  This function tests DCS's API le_data_Request() for requesting a data connection
 */
//--------------------------------------------------------------------------------------------------
void dcs_test_data_api_Request
(
    void *param1,
    void *param2
)
{
    LE_INFO("DCS-client: request for a connection via le_data API");

    myReqRef = le_data_Request();
    if (myReqRef == 0)
    {
        LE_ERROR("DCS-client: failed to get a connection");
        return;
    }

    LE_INFO("DCS-client: succeeded to init a connection via le_data with myReqRef %p", myReqRef);
}


//--------------------------------------------------------------------------------------------------
/**
 *  This function tests DCS's API le_data_Release() for releasing an already started data
 *  connection
 */
//--------------------------------------------------------------------------------------------------
void dcs_test_data_api_Release
(
    void *param1,
    void *param2
)
{
    if (myReqRef == 0)
    {
        return;
    }
    LE_INFO("DCS-client: asking to release a connection via le_data API");
    le_data_Release(myReqRef);
}


//--------------------------------------------------------------------------------------------------
/**
 *  This is the connection state handler used & to be added in the test of the le_data API
 *  le_data_AddConnectionStateHandler()
 */
//--------------------------------------------------------------------------------------------------
void dataConnectionStateHandler
(
    const char* intfName,
    bool isConnected,
    void* contextPtr
)
{
    le_data_Technology_t currentTech = le_data_GetTechnology();
    LE_INFO("DCS-client: received connection status %d for interface %s of technology %d",
            isConnected, intfName, currentTech);
}


//--------------------------------------------------------------------------------------------------
/**
 *  This function tests le_data's API le_data_AddEventHandler() for adding a connection state
 *  handler
 */
//--------------------------------------------------------------------------------------------------
void dcs_test_data_api_AddConnStateHandler
(
    void *param1,
    void *param2
)
{
    LE_INFO("DCS-client: asking to add an event handler");
    connStateHandlerRef = le_data_AddConnectionStateHandler(dataConnectionStateHandler, NULL);
    LE_INFO("DCS-client: le_data connection state handler added %p", connStateHandlerRef);
}


//--------------------------------------------------------------------------------------------------
/**
 *  This is the thread that runs an event loop to take test functions to run
 */
//--------------------------------------------------------------------------------------------------
static void *testThread
(
    void *context
)
{
    le_dcs_ConnectService();
    le_net_ConnectService();
    le_data_ConnectService();

    le_event_RunLoop();
}


//--------------------------------------------------------------------------------------------------
/**
 *  Main, with component init
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    le_thread_Ref_t testThreadRef = le_thread_Create("client test thread", testThread, NULL);
    le_thread_SetPriority(testThreadRef, LE_THREAD_PRIORITY_MEDIUM);
    le_thread_Start(testThreadRef);

    sleep(10);

#if 1

    le_event_QueueFunctionToThread(testThreadRef, dcs_test_api_GetList, NULL, NULL);

    le_event_QueueFunctionToThread(testThreadRef, dcs_test_api_GetReference, NULL, NULL);

    le_event_QueueFunctionToThread(testThreadRef, dcs_test_api_AddEventHandler, NULL, NULL);

    le_event_QueueFunctionToThread(testThreadRef, dcs_test_api_Start, NULL, NULL);

    le_event_QueueFunctionToThread(testThreadRef, dcs_test_api_GetTechnology, NULL, NULL);

    sleep(60);

    le_event_QueueFunctionToThread(testThreadRef, dcs_test_api_Stop, NULL, NULL);

#else

    int32_t profileIndex = 5;

    if (LE_OK != le_data_SetTechnologyRank(1, LE_DATA_WIFI) ||
        LE_OK != le_data_SetCellularProfileIndex(profileIndex))
    {
        LE_ERROR("DCS-client: failed to set 1st rank to cellular, profile %d", profileIndex);
    }

    le_event_QueueFunctionToThread(testThreadRef, dcs_test_data_api_AddConnStateHandler,
                                   NULL, NULL);

    le_event_QueueFunctionToThread(testThreadRef, dcs_test_data_api_Request, NULL, NULL);

    sleep(30);

    le_event_QueueFunctionToThread(testThreadRef, dcs_test_data_api_Release, NULL, NULL);

#endif
}
