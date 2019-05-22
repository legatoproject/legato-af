/**
 * Data Connection Server Unit-test app/component: client.c
 *
 * This module implements the unit-test code for DCS's le_dcs APIs, which is built into a test
 * component to run on the target to start, stop, etc., data channels in different sequence &
 * timing.
 * This could/would be used as one of the few test apps, alongside the others, to run in
 * parallel to simulate scenarios having multiple apps using DCS simultaneously. One such
 * counterpart is apps/test/dataConnectionService/dcsAPICrossTests/dcsCrossClient/crossClient.c.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include <string.h>
#include "legato.h"
#include "interfaces.h"

static le_thread_Ref_t TestThreadRef;
static le_dcs_EventHandlerRef_t EventHandlerRef = NULL;
static le_dcs_ChannelRef_t MyChannel = 0;
static le_wifiClient_SecurityProtocol_t secProtocol = LE_WIFICLIENT_SECURITY_WPA_PSK_PERSONAL;
static const uint8_t secret[] = "mySecret";
static const uint8_t ssid[] = "MY-MOBILE";
static char channelName[LE_DCS_CHANNEL_NAME_MAX_LEN] = "MY-MOBILE";
static le_dcs_Technology_t MyTech = LE_DCS_TECH_WIFI;
static le_dcs_ReqObjRef_t ReqObj;
static le_data_RequestObjRef_t MyReqRef;
static le_data_ConnectionStateHandlerRef_t ConnStateHandlerRef = NULL;


//--------------------------------------------------------------------------------------------------
/**
 *  This is the event handler used & to be added in the test of DCS's NB API
 *  le_dcs_AddEventHandler() for a channel
 */
//--------------------------------------------------------------------------------------------------
static void ClientEventHandler
(
    le_dcs_ChannelRef_t channelRef, ///< [IN] The channel for which the event is
    le_dcs_Event_t event,           ///< [IN] Event up or down
    int32_t code,                   ///< [IN] Additional event code, like error code
    void *contextPtr                ///< [IN] Associated user context pointer
)
{
    char *eventString;
    switch (event)
    {
        case LE_DCS_EVENT_UP:
            eventString = "Up";
            break;
        case LE_DCS_EVENT_DOWN:
            eventString = "Down";
            break;
        case LE_DCS_EVENT_TEMP_DOWN:
            eventString = "Temporary Down";
            break;
        default:
            eventString = "Unknown";
            break;
    }
    LE_INFO("DCS-client: received for channel reference %p event %s", channelRef, eventString);
}


//--------------------------------------------------------------------------------------------------
/**
 *  This function tests DCS's NB API le_dcs_GetReference() for querying the channel name of
 *  the given channel
 */
//--------------------------------------------------------------------------------------------------
void Dcs_test_api_GetReference
(
    void *param1,
    void *param2
)
{
    le_dcs_ChannelRef_t ret_ref;
    LE_INFO("DCS-client: asking for channel reference for channel %s of tech %d", channelName,
            MyTech);
    ret_ref = le_dcs_GetReference(channelName, MyTech);
    LE_INFO("DCS-client: returned channel reference: %p", ret_ref);
}


//--------------------------------------------------------------------------------------------------
/**
 *  This function tests DCS's NB API le_dcs_GetTechnology() for querying the technology type of
 *  the given channel
 */
//--------------------------------------------------------------------------------------------------
void Dcs_test_api_GetTechnology
(
    void *param1,
    void *param2
)
{
    le_dcs_Technology_t retTech = -1;

    LE_INFO("DCS-client: asking for tech type");
    retTech = le_dcs_GetTechnology(MyChannel);
    LE_INFO("DCS-client: returned tech type: %d", (int)retTech);
}


//--------------------------------------------------------------------------------------------------
/**
 *  This function tests DCS's NB API le_dcs_GetState() for querying the channel status of
 *  the given channel
 */
//--------------------------------------------------------------------------------------------------
void Dcs_test_api_GetState
(
    void *param1,
    void *param2
)
{
    le_dcs_State_t state;
    char name[LE_DCS_INTERFACE_NAME_MAX_LEN+1];
    le_result_t ret;

    LE_INFO("DCS-client: asking for channel status");
    ret = le_dcs_GetState(MyChannel, &state, name, LE_DCS_INTERFACE_NAME_MAX_LEN);
    LE_INFO("DCS-client: returned for channel %s netIntf %s status %d (rc %d)", channelName,
            name, state, (int)ret);

}


//--------------------------------------------------------------------------------------------------
/**
 *  This function tests DCS's NB APIs le_dcs_Start() for starting the given data channel
 */
//--------------------------------------------------------------------------------------------------
void Dcs_test_api_Start
(
    void *param1,
    void *param2
)
{
    LE_INFO("DCS-client: asking to start channel %s", channelName);
    if (MyChannel)
    {
        ReqObj = le_dcs_Start(MyChannel);
        LE_INFO("DCS-client: returned RequestObj %p", ReqObj);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 *  This function tests DCS's NB API le_dcs_Stop() for stopping the given data channel
 */
//--------------------------------------------------------------------------------------------------
void Dcs_test_api_Stop
(
    void *param1,
    void *param2
)
{
    le_result_t ret;

    LE_INFO("DCS-client: asking to stop channel %s", channelName);

    if (!ReqObj || !MyChannel)
    {
        return;
    }

    ret = le_dcs_Stop(ReqObj);
    LE_INFO("DCS-client: got for channel %s release status %d", channelName, ret);
}


//--------------------------------------------------------------------------------------------------
/**
 *  This function tests DCS's NB API le_dcs_AddEventHandler() for adding a channel event handler
 *  for the given channel
 */
//--------------------------------------------------------------------------------------------------
void Dcs_test_api_AddEventHandler
(
    void *param1,
    void *param2
)
{
    LE_INFO("DCS-client: asking to add event handler for channel %s", channelName);
    EventHandlerRef = le_dcs_AddEventHandler(MyChannel, ClientEventHandler, NULL);
    LE_INFO("DCS-client: channel event handler added %p for channel %s", EventHandlerRef,
            channelName);
}


//--------------------------------------------------------------------------------------------------
/**
 *  This function tests DCS's NB API le_dcs_RemoveEventHandler() for removing a channel event
 *  handler for the given channel
 */
//--------------------------------------------------------------------------------------------------
void Dcs_test_api_RmEventHandler
(
    void *param1,
    void *param2
)
{
    LE_INFO("DCS-client: asking to remove event handler for channel %s", channelName);
    if (!EventHandlerRef)
    {
        LE_INFO("DCS-client: no channel event handler to remove");
        return;
    }

    le_dcs_RemoveEventHandler(EventHandlerRef);
    EventHandlerRef = NULL;
    LE_INFO("DCS-client: Done removing event handler");
}


//--------------------------------------------------------------------------------------------------
/**
 *  This is the event handler used & to be added in the test of DCS's NB API
 *  le_dcs_AddEventHandler() for a channel query
 */
//--------------------------------------------------------------------------------------------------
static void ClientChannelQueryHandler
(
    le_result_t result,                       ///< [IN] Result of the query
    const le_dcs_ChannelInfo_t *channelList,  ///< [IN] Channel list returned
    size_t channelListSize,                   ///< [IN] Channel list's size
    void *contextPtr                          ///< [IN] Associated user context pointer
)
{
    uint16_t i;

    LE_INFO("DCS-client: result received for channel query %d, channel list size %d",
            result, channelListSize);

    if (channelListSize == 0)
    {
        channelName[0] = '\0';
        MyChannel = 0;
        return;
    }

    for (i = 0; i < channelListSize; i++)
    {
        LE_INFO("DCS-client: available channel #%d from technology %d with name %s, state %d, "
                "ref %p", i + 1, channelList[i].technology, channelList[i].name,
                channelList[i].state, channelList[i].ref);
        if (strcmp(channelList[i].name, channelName) == 0)
        {
            strncpy(channelName, channelList[i].name, LE_DCS_CHANNEL_NAME_MAX_LEN);
            MyChannel = channelList[i].ref;
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 *  This function tests DCS's NB API le_dcs_GetChannels() for adding a channel
 *  query handler for the given channel
 */
//--------------------------------------------------------------------------------------------------
void Dcs_test_api_GetChannels
(
    void *param1,
    void *param2
)
{
    LE_INFO("DCS-client: asking to query channel list");
    le_dcs_GetChannels(ClientChannelQueryHandler, NULL);
}


//--------------------------------------------------------------------------------------------------
/**
 *  This function tests DCS's API le_data_Request() for requesting a data connection
 */
//--------------------------------------------------------------------------------------------------
void Dcs_test_data_api_Request
(
    void *param1,
    void *param2
)
{
    LE_INFO("DCS-client: request for a connection via le_data API");

    MyReqRef = le_data_Request();
    if (MyReqRef == 0)
    {
        LE_ERROR("DCS-client: failed to get a connection");
        return;
    }

    LE_INFO("DCS-client: succeeded to init a connection via le_data with MyReqRef %p", MyReqRef);
}


//--------------------------------------------------------------------------------------------------
/**
 *  This function tests DCS's API le_data_Release() for releasing an already started data
 *  connection
 */
//--------------------------------------------------------------------------------------------------
void Dcs_test_data_api_Release
(
    void *param1,
    void *param2
)
{
    if (MyReqRef == 0)
    {
        return;
    }

    LE_INFO("DCS-client: asking to release a connection via le_data API");
    le_data_Release(MyReqRef);
}


//--------------------------------------------------------------------------------------------------
/**
 *  This function tests DCS's API le_data_Release() for releasing an already started data
 *  connection
 */
//--------------------------------------------------------------------------------------------------
void Dcs_test_api_Networking
(
    void *param1,
    void *param2
)
{
    bool isAdd = *(bool *)param1;

    if (isAdd)
    {
        LE_INFO("DCS-client: asking to add route for channel %s", channelName);
        le_net_BackupDefaultGW();
        le_net_SetDefaultGW(MyChannel);
        le_net_SetDNS(MyChannel);
    }
    else
    {
        LE_INFO("DCS-client: asking to remove route for channel %s", channelName);
        le_net_RestoreDefaultGW();
        le_net_RestoreDNS();
    }
    le_net_ChangeRoute(MyChannel, "1.1.1.1", "", isAdd);
}


//--------------------------------------------------------------------------------------------------
/**
 *  This is the connection state handler used & to be added in the test of the le_data API
 *  le_data_AddConnectionStateHandler()
 */
//--------------------------------------------------------------------------------------------------
void DataConnectionStateHandler
(
    const char* intfName,
    bool isConnected,
    void* contextPtr
)
{
    le_data_Technology_t currentTech = le_data_GetTechnology();
    LE_INFO("DCS-client: received for interface %s of technology %d connection status %d ",
            intfName, currentTech, isConnected);
}


//--------------------------------------------------------------------------------------------------
/**
 *  This function tests le_data's API le_data_AddEventHandler() for adding a connection state
 *  handler
 */
//--------------------------------------------------------------------------------------------------
void Dcs_test_data_api_AddConnStateHandler
(
    void *param1,
    void *param2
)
{
    LE_INFO("DCS-client: asking to add an event handler");
    ConnStateHandlerRef = le_data_AddConnectionStateHandler(DataConnectionStateHandler, NULL);
    LE_INFO("DCS-client: le_data connection state handler added %p", ConnStateHandlerRef);
}


//--------------------------------------------------------------------------------------------------
/**
 *  This function tests le_wifiClient's API le_wifiClient_RemoveSsidSecurityConfigs() for removing
 *  previously configured user credentials for a given SSID and resetting the security protocol to
 *  none.
 */
//--------------------------------------------------------------------------------------------------
static void Dcs_test_api_wifiSecurityCleanup
(
    void *param1,
    void *param2
)
{
    le_result_t ret = le_wifiClient_RemoveSsidSecurityConfigs(ssid, sizeof(ssid));
    if ((ret != LE_OK) && (ret != LE_NOT_FOUND))
    {
        LE_ERROR("DCS-client: Failed to clean Wifi security configs; retcode %d", ret);
    }
    else
    {
        LE_INFO("Succeeded cleaning Wifi security configs");
    }
}


//--------------------------------------------------------------------------------------------------
/**
 *  This function tests le_wifiClient's API le_wifiClient_ConfigPsk() for configuring WPA PSK
 *  user credentials for a given SSID.
 */
//--------------------------------------------------------------------------------------------------
static void Dcs_test_api_wifiSecurityConfig
(
    void *param1,
    void *param2
)
{
    le_result_t ret = le_wifiClient_ConfigurePsk(ssid, sizeof(ssid), secProtocol, secret,
                                                 sizeof(secret), secret, sizeof(secret));
    if (ret != LE_OK)
    {
        LE_ERROR("DCS-client: Failed to configure Wifi PSK; retcode %d", ret);
    }
    else
    {
        LE_INFO("Succeeded installing Wifi PSK configs");
    }
}


//--------------------------------------------------------------------------------------------------
/**
 *  This is the thread that runs an event loop to take test functions to run
 */
//--------------------------------------------------------------------------------------------------
static void *TestThread
(
    void *context
)
{
    le_dcs_ConnectService();
    le_net_ConnectService();
    le_data_ConnectService();
    le_wifiClient_ConnectService();

    le_event_RunLoop();
}


//--------------------------------------------------------------------------------------------------
/**
 *  Main, with component init
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
#define CHOSEN_PROFILE 5
#define INIT_SLEEP 8
#define END_SLEEP 10
#define WAIT_FOR_CHANNELS_LOOP_SLEEP 2
#define LOOP_SLEEP10 10
#define LOOP_SLEEP20 20
#define LOOP_SLEEP30 30
#define LOOP_SLEEP50 50
#define LOOP_SLEEP16 16
#define LOOP_SLEEP24 24
#define LOOP_SLEEP36 36
#define WAIT_FOR_CHANNELS_LOOP 10
#define TEST_LOOP 3

    TestThreadRef = le_thread_Create("DCS client test thread", TestThread, NULL);
    le_thread_SetPriority(TestThreadRef, LE_THREAD_PRIORITY_MEDIUM);
    le_thread_Start(TestThreadRef);

    sleep(INIT_SLEEP);

    le_event_QueueFunctionToThread(TestThreadRef, Dcs_test_api_wifiSecurityCleanup, NULL, NULL);

    le_event_QueueFunctionToThread(TestThreadRef, Dcs_test_api_wifiSecurityConfig, NULL, NULL);

#if 1

    uint16_t i, j;

    for (j=0; j<TEST_LOOP && !MyChannel; j++)
    {
        le_event_QueueFunctionToThread(TestThreadRef, Dcs_test_api_GetChannels, NULL, NULL);
        for (i=0; i<WAIT_FOR_CHANNELS_LOOP; i++)
        {
            sleep(WAIT_FOR_CHANNELS_LOOP_SLEEP);
            if (MyChannel)
            {
                break;
            }
        }
    }

    le_event_QueueFunctionToThread(TestThreadRef, Dcs_test_api_AddEventHandler, NULL, NULL);

    for (i=0; i<TEST_LOOP; i++)
    {
        le_event_QueueFunctionToThread(TestThreadRef, Dcs_test_api_Start, NULL, NULL);
        sleep(LOOP_SLEEP20);
        le_event_QueueFunctionToThread(TestThreadRef, Dcs_test_api_Stop, NULL, NULL);
        sleep(LOOP_SLEEP20);
        le_event_QueueFunctionToThread(TestThreadRef, Dcs_test_api_GetChannels, NULL, NULL);
        sleep(LOOP_SLEEP20);
    }

    le_event_QueueFunctionToThread(TestThreadRef, Dcs_test_api_RmEventHandler, NULL, NULL);

#else

    bool isAdd;

    le_event_QueueFunctionToThread(TestThreadRef, Dcs_test_api_GetReference, NULL, NULL);

    le_event_QueueFunctionToThread(TestThreadRef, Dcs_test_api_AddEventHandler, NULL, NULL);
    for (i=0; i<WAIT_FOR_CHANNELS_LOOP; i++)
    {
        le_event_QueueFunctionToThread(TestThreadRef, Dcs_test_api_GetChannels, NULL, NULL);
        sleep(WAIT_FOR_CHANNELS_LOOP_SLEEP);
    }

    le_event_QueueFunctionToThread(TestThreadRef, Dcs_test_api_GetState, NULL, NULL);

    le_event_QueueFunctionToThread(TestThreadRef, Dcs_test_api_Start, NULL, NULL);

    sleep(LOOP_SLEEP10);

    isAdd = true;
    le_event_QueueFunctionToThread(TestThreadRef, Dcs_test_api_Networking, &isAdd, NULL);

    le_event_QueueFunctionToThread(TestThreadRef, Dcs_test_api_GetTechnology, NULL, NULL);

    le_event_QueueFunctionToThread(TestThreadRef, Dcs_test_api_GetState, NULL, NULL);

    sleep(LOOP_SLEEP30);

    isAdd = false;
    le_event_QueueFunctionToThread(TestThreadRef, Dcs_test_api_Networking, &isAdd, NULL);

    le_event_QueueFunctionToThread(TestThreadRef, Dcs_test_api_GetState, NULL, NULL);

    sleep(LOOP_SLEEP20);

    le_event_QueueFunctionToThread(TestThreadRef, Dcs_test_api_Stop, NULL, NULL);

    int32_t i, profileIndex = CHOSEN_PROFILE;

    if (LE_OK != le_data_SetTechnologyRank(1, LE_DATA_WIFI) ||
        LE_OK != le_data_SetCellularProfileIndex(profileIndex))
    {
        LE_ERROR("DCS-client: failed to set 1st rank to cellular, profile %d", profileIndex);
    }

    le_event_QueueFunctionToThread(TestThreadRef, Dcs_test_data_api_AddConnStateHandler,
                                   NULL, NULL);
    for (i=0; i<TEST_LOOP; i++)
    {
        le_event_QueueFunctionToThread(TestThreadRef, Dcs_test_data_api_Request, NULL, NULL);

        sleep(LOOP_SLEEP30);

        le_event_QueueFunctionToThread(TestThreadRef, Dcs_test_data_api_Release, NULL, NULL);

        sleep(LOOP_SLEEP30);
    }

#endif

    sleep(END_SLEEP);

    LE_INFO("DCS-client: Done testing");
}
