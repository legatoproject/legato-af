/** @file messagingService.h
 *
 * Inter-module definitions exported by the Service module of the @ref c_messaging implementation.
 *
 * See @ref messaging.c for an overview of the @ref c_messaging implementation.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#ifndef LE_MESSAGING_SERVICE_H_INCLUDE_GUARD
#define LE_MESSAGING_SERVICE_H_INCLUDE_GUARD

#include "serviceDirectory/serviceDirectoryProtocol.h"


//--------------------------------------------------------------------------------------------------
/**
 * Service identifier.  Contains everything needed to uniquely identify a service instance.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_msg_ProtocolRef_t    protocolRef;        ///< The protocol that this service supports.
    char    name[LIMIT_MAX_SERVICE_NAME_BYTES]; ///< The service instance name.
}
ServiceId_t;


//--------------------------------------------------------------------------------------------------
/**
 * Service object.  Represents a single, unique service instance offered by a server.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_msg_Service
{
    ServiceId_t     id;                 ///< The unique identifier for the service.

    // Stuff only used on the Server side:

    void*           contextPtr;         ///< Opaque value set using le_msg_SetServiceContextPtr().

    enum
    {
        LE_MSG_SERVICE_CONNECTING,  ///< Connecting to the Service Directory.
        LE_MSG_SERVICE_ADVERTISED,  ///< Connected to the Service Directory (advertised).
        LE_MSG_SERVICE_HIDDEN       ///< Disconnected from the Service Directory (hidden).
    }
    state;

    int             directorySocketFd;  ///< File descriptor of socket connected to the
                                        ///  Service Directory (or -1 if not connected).

    le_event_FdMonitorRef_t fdMonitorRef;///< File descriptor monitor for the directory socket.

    le_thread_Ref_t serverThread;       ///< Thread that is acting as server in this process,
                                        ///  or NULL if no server exists in this process.

    le_dls_List_t   sessionList;        ///< List of Session objects for open sessions with
                                        ///  clients (only used by the service's server thread).

    le_msg_SessionEventHandler_t    openHandler;    ///< Handler function called when sessions open.
    void*                           openContextPtr; ///< contextPtr parameter for openHandler.

    le_msg_SessionEventHandler_t    closeHandler;   ///< Handler function for when sessions close.
    void*                           closeContextPtr;///< contextPtr parameter for closeHandler.

    le_msg_ReceiveHandler_t         recvHandler;    ///< Handler for when messages are received.
    void*                           recvContextPtr; ///< contextPtr parameter for recvHandler.
}
Service_t;


//--------------------------------------------------------------------------------------------------
/**
 * Initializes the module.  This must be called only once at start-up, before
 * any other functions in that module are called.
 */
//--------------------------------------------------------------------------------------------------
void msgService_Init
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets a reference to a Service object.  Must be released using msgService_Release() when
 * you are done with it.
 *
 * @return  Reference to the service object.
 */
//--------------------------------------------------------------------------------------------------
le_msg_ServiceRef_t msgService_GetService
(
    le_msg_ProtocolRef_t    protocolRef,
    const char*             serviceName
);


//--------------------------------------------------------------------------------------------------
/**
 * Send service identification information via a connected socket.
 */
//--------------------------------------------------------------------------------------------------
void msgService_SendServiceId
(
    le_msg_ServiceRef_t serviceRef, ///< [in] Pointer to the service whose ID is to be sent.
    int                 socketFd    ///< [in] File descriptor of the connected socket to send on.
);


//--------------------------------------------------------------------------------------------------
/**
 * Get a reference to the Protocol that a Service is running.
 *
 * @return The protocol reference.
 */
//--------------------------------------------------------------------------------------------------
static inline le_msg_ProtocolRef_t msgService_GetProtocolRef
(
    le_msg_ServiceRef_t serviceRef
)
{
    return serviceRef->id.protocolRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Release a reference to a Service.
 */
//--------------------------------------------------------------------------------------------------
void msgService_Release
(
    le_msg_ServiceRef_t serviceRef
);


//--------------------------------------------------------------------------------------------------
/**
 * Add a Session to a Service's list of open sessions.
 */
//--------------------------------------------------------------------------------------------------
void msgService_AddSession
(
    le_msg_ServiceRef_t serviceRef,
    le_msg_SessionRef_t sessionRef
);


//--------------------------------------------------------------------------------------------------
/**
 * Remove a Session from a Service's list of open sessions.
 */
//--------------------------------------------------------------------------------------------------
void msgService_RemoveSession
(
    le_msg_ServiceRef_t serviceRef,
    le_msg_SessionRef_t sessionRef
);


//--------------------------------------------------------------------------------------------------
/**
 * Call a Service's registered session close handler function, if there is one registered.
 */
//--------------------------------------------------------------------------------------------------
void msgService_CallCloseHandler
(
    le_msg_ServiceRef_t serviceRef,
    le_msg_SessionRef_t sessionRef
);


//--------------------------------------------------------------------------------------------------
/**
 * Dispatches a message received from a client to a service's server.
 */
//--------------------------------------------------------------------------------------------------
void msgService_ProcessMessageFromClient
(
    le_msg_ServiceRef_t serviceRef, ///< [IN] Reference to the Service object.
    le_msg_MessageRef_t msgRef      ///< [IN] Message reference for the received message.
);


#endif // LE_MESSAGING_SERVICE_H_INCLUDE_GUARD
