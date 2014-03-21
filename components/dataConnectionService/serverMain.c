// -------------------------------------------------------------------------------------------------
/**
 *  Data Connection Server
 *
 *  Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 *
 * todo:
 *  - assumes that DHCP client will always succeed; this is not always the case
 *  - assumes that there is a valid SIM and the modem is registered on the network
 *  - only handles the default data connection on mobile network
 *  - uses a hard-coded APN value; this value should be read from the config tree
 *  - has a very simple recovery mechanism after data connection is lost; this needs improvement.
 */
// -------------------------------------------------------------------------------------------------

#include "legato.h"

// Modem Services includes
#include "le_mrc.h"
#include "le_sms.h"
#include "le_sim.h"
#include "le_mdc.h"

// IPC for data interface
#include "le_data_server.h"

#include "le_print.h"



//--------------------------------------------------------------------------------------------------
/**
 * The Default APN, in case one isn't available by other means.
 */
//--------------------------------------------------------------------------------------------------
#define DEFAULT_APN "internet.com"


//--------------------------------------------------------------------------------------------------
/**
 * The file to read for the APN, in case it is not available in the config tree.
 */
//--------------------------------------------------------------------------------------------------
#define APN_FILE "/usr/local/lib/apn.txt"


//--------------------------------------------------------------------------------------------------
/**
 * Definitions for sending request/release commands to data thread
 */
//--------------------------------------------------------------------------------------------------
#define REQUEST_COMMAND 1
#define RELEASE_COMMAND 2
static le_event_Id_t CommandEvent;


//--------------------------------------------------------------------------------------------------
/**
 * Event for sending connection to applications
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t ConnStateEvent;


//--------------------------------------------------------------------------------------------------
/**
 * Data associated with the above ConnStateEvent.
 *
 * interfaceName is only valid if isConnected is true.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    bool isConnected;
    char interfaceName[100+1];
}
ConnStateData_t;


//--------------------------------------------------------------------------------------------------
/**
 * Is the data session connected
 */
//--------------------------------------------------------------------------------------------------
static bool IsConnected = false;


//--------------------------------------------------------------------------------------------------
/**
 * Count the number of requests
 */
//--------------------------------------------------------------------------------------------------
static uint32_t RequestCount = 0;


//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for the request reference
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t RequestRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Start data session
 */
//--------------------------------------------------------------------------------------------------
static void StartDataSession
(
    void
)
{
    le_mdc_Profile_Ref_t profileRef = le_mdc_LoadProfile("internet");

    char interfaceName[100];
    char system_cmd[200];

    le_result_t result;


    if (profileRef == NULL)
    {
        LE_ERROR("Failed to open profile.");
        return;
    }

    // Keep trying to start the data session until it succeeds.  If it fails, then wait a bit and
    // try again.  No need to do anything fancy with timers, since this thread has nothing else
    // to do while waiting for the data connection.
    while (true)
    {
        result = le_mdc_StartSession(profileRef);
        if ( result == LE_OK )
            break;

        LE_ERROR("Failed to start session.");
        sleep(15);
    }

    if (le_mdc_GetInterfaceName(profileRef, interfaceName, sizeof(interfaceName)) != LE_OK)
    {
        LE_ERROR("Failed to get interface name.");
        return;
    }

    // The system may not be configured to run the correct DHCP client script, so re-run the DHCP
    // client with the correct script.

    // First wait a few seconds so as not to conflict with the default DHCP client.
    sleep(3);

    //The -q option is used to exit after obtaining a lease.
    snprintf(system_cmd, sizeof(system_cmd), "udhcpc -q -i %s -s /etc/udhcpc.d/50default", interfaceName);

    if (system(system_cmd) != 0)
    {
        LE_ERROR("Running udhcpc failed");
        return;
    }

    // Wait a few seconds to prevent rapid toggling of data connection
    sleep(5);
}


//--------------------------------------------------------------------------------------------------
/**
 * Stop data session
 */
//--------------------------------------------------------------------------------------------------
static void StopDataSession
(
    void
)
{
    le_mdc_Profile_Ref_t profileRef = le_mdc_LoadProfile("internet");

    if (profileRef == NULL)
    {
        LE_ERROR("Failed to open profile.");
        return;
    }

    if (le_mdc_StopSession(profileRef) != LE_OK)
    {
        LE_ERROR("Failed to stop session.");
        return;
    }

    // Wait a few seconds to prevent rapid toggling of data connection
    sleep(5);
}

//--------------------------------------------------------------------------------------------------
/**
 * Send connection state event
 */
//--------------------------------------------------------------------------------------------------
void SendConnStateEvent
(
    bool isConnected
)
{
    // todo: There must be a better way to get profileRefs than always just loading the profile.
    //       Revisit this when there is more time.
    le_mdc_Profile_Ref_t profileRef = le_mdc_LoadProfile("internet");
    if (profileRef == NULL)
    {
        LE_ERROR("Failed to open profile.");
        return;
    }

    // Init the event data
    ConnStateData_t eventData;
    eventData.isConnected = isConnected;
    if ( isConnected )
    {
        le_mdc_GetInterfaceName(profileRef, eventData.interfaceName, sizeof(eventData.interfaceName));
    }
    else
    {
        // init to empty string
        eventData.interfaceName[0] = '\0';
    }

    LE_PRINT_VALUE("%s", eventData.interfaceName);
    LE_PRINT_VALUE("%i", eventData.isConnected);

    // Send the event to interested applications
    le_event_Report(ConnStateEvent, &eventData, sizeof(eventData));
}


//--------------------------------------------------------------------------------------------------
/**
 * Handler to process a command
 */
//--------------------------------------------------------------------------------------------------
void ProcessCommand
(
    void* reportPtr
)
{
    uint32_t command = *(uint32_t*)reportPtr;

    LE_PRINT_VALUE("%i", command);

    if ( command == REQUEST_COMMAND )
    {
        RequestCount++;
        if ( ! IsConnected )
        {
            StartDataSession();

            // Do this here, as well as in the callback, in case another command sneaks in
            // before the callback in invoked.
            IsConnected = true;
        }
        else
        {
            // There is already a data session, so send a fake event so that the new application
            // that just sent the command knows about the current state.  This will also cause
            // redundant info to be sent to the other registered apps, but that's okay.
            SendConnStateEvent(true);
        }
    }
    else if ( command == RELEASE_COMMAND )
    {
        // Don't decrement below zero, as it will wrap-around.
        if ( RequestCount > 0 )
        {
            RequestCount--;
        }

        if ( (RequestCount == 0) && IsConnected )
        {
            StopDataSession();

            // Do this here, as well as in the callback, in case another command sneaks in
            // before the callback in invoked.
            IsConnected = false;
        }
    }
    else
    {
        LE_ERROR("Command %i is not valid", command);
    }
}


// -------------------------------------------------------------------------------------------------
/**
 *  Event callback for data session state changes.
 */
// -------------------------------------------------------------------------------------------------
static void DataSessionStateHandler
(
    bool   isConnected,
    void*  contextPtr
)
{
    le_mdc_Profile_Ref_t profileRef = (le_mdc_Profile_Ref_t)contextPtr;
    char name[100+1];

    le_mdc_GetProfileName(profileRef, name, sizeof(name));

    LE_PRINT_VALUE("%s", name);
    LE_PRINT_VALUE("%i", isConnected);

    // Update global state variable
    IsConnected = isConnected;

    // Send the state event to applications
    SendConnStateEvent(isConnected);

    // Restart data connection, if it has gone down, and there are still valid requests
    // todo: this mechanism needs to be much better
    if ( ( RequestCount>0 ) && ( !isConnected ) )
    {
        // Give the modem some time to recover from whatever caused the loss of the data
        // connection, before trying to recover.
        sleep(30);

        // Try to restart
        StartDataSession();

        // Do this here, in case another command has snuck in while re-connecting
        IsConnected = true;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * This thread does the actual work of starting/stopping a data connection
 */
//--------------------------------------------------------------------------------------------------
void* DataThread
(
    void* contextPtr
)
{
    LE_INFO("Data Thread Started");

    // Register for command events
    le_event_AddHandler("ProcessCommand",
                        CommandEvent,
                        ProcessCommand);


    // Register for data session state changes
    le_mdc_Profile_Ref_t profileRef = le_mdc_LoadProfile("internet");
    if (profileRef == NULL)
    {
        LE_ERROR("Failed to open profile.");
    }
    else
    {
        le_mdc_AddSessionStateHandler(profileRef, DataSessionStateHandler, profileRef);
    }


    // Run the event loop
    le_event_RunLoop();
    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * The first-layer Connection State Handler
 *
 */
//--------------------------------------------------------------------------------------------------
static void FirstLayerConnectionStateHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    ConnStateData_t* eventDataPtr = reportPtr;
    le_data_ConnectionStateHandlerFunc_t clientHandlerFunc = secondLayerHandlerFunc;

    clientHandlerFunc(eventDataPtr->interfaceName,
                      eventDataPtr->isConnected,
                      le_event_GetContextPtr());
}


//--------------------------------------------------------------------------------------------------
/**
 * This function adds a handler ...
 */
//--------------------------------------------------------------------------------------------------
le_data_ConnectionStateHandlerRef_t le_data_AddConnectionStateHandler
(
    le_data_ConnectionStateHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    LE_PRINT_VALUE("%p", handlerPtr);
    LE_PRINT_VALUE("%p", contextPtr);

    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler(
                                                    "DataConnState",
                                                    ConnStateEvent,
                                                    FirstLayerConnectionStateHandler,
                                                    (le_event_HandlerFunc_t)handlerPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (le_data_ConnectionStateHandlerRef_t)(handlerRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * This function removes a handler ...
 */
//--------------------------------------------------------------------------------------------------
void le_data_RemoveConnectionStateHandler
(
    le_data_ConnectionStateHandlerRef_t addHandlerRef
)
{
    LE_PRINT_VALUE("%p", addHandlerRef);

    le_event_RemoveHandler((le_event_HandlerRef_t)addHandlerRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Request the default data connection
 *
 * @return
 *      - A reference to the data connection (to be used later for releasing the connection)
 *      - NULL if the data connection requested could not be processed
 */
//--------------------------------------------------------------------------------------------------
le_data_Request_Ref_t le_data_Request
(
    void
)
{
    uint32_t command = REQUEST_COMMAND;
    le_event_Report(CommandEvent, &command, sizeof(command));

    // Need to return a unique reference that will be used by Release.  Don't actually have
    // any data for now, but have to use some value other than NULL for the data pointer.
    le_data_Request_Ref_t reqRef = le_ref_CreateRef(RequestRefMap, (void*)1);
    return reqRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Release a previously requested data connection
 */
//--------------------------------------------------------------------------------------------------
void le_data_Release
(
    le_data_Request_Ref_t requestRef
    ///< Reference to a previously requested data connection
)
{
    // Look up the reference.  If it is NULL, then the reference is not valid.
    // Otherwise, delete the reference and send the release command to the data thread.
    void* dataPtr = le_ref_Lookup(RequestRefMap, requestRef);
    if ( dataPtr == NULL )
    {
        LE_ERROR("Invalid data request reference %p", requestRef);
    }
    else
    {
        LE_PRINT_VALUE("%p", requestRef);
        le_ref_DeleteRef(RequestRefMap, requestRef);

        uint32_t command = RELEASE_COMMAND;
        le_event_Report(CommandEvent, &command, sizeof(command));
    }
}



//--------------------------------------------------------------------------------------------------
/**
 *  Server Init
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // Init the data connection service
    le_data_StartServer("dataConnectionService");

    // Init the various events
    CommandEvent = le_event_CreateId("Data Command", sizeof(uint32_t));
    ConnStateEvent = le_event_CreateId("Conn State", sizeof(ConnStateData_t));

    // Create safe reference map for request references. The size of the map should be based on
    // the expected number of simultaneous data requests, so take a reasonable guess.
    RequestRefMap = le_ref_CreateMap("Requests", 5);

    // Start the data thread
    le_thread_Start( le_thread_Create("Data Thread", DataThread, NULL) );

    // Close the fd that we inherited from the Supervisor.  This will let the Supervisor know that
    // we are initialized.  Then re-open it to /dev/null so that it cannot be reused later.
    FILE* filePtr;
    do
    {
        filePtr = freopen("/dev/null", "r", stdin);
    }
    while ( (filePtr == NULL) && (errno == EINTR) );

    LE_FATAL_IF(filePtr == NULL, "Failed to redirect stdin to /dev/null.  %m.");

    LE_INFO("Data Connection Server is ready");
}

