/** @file messagingInterface.h
 *
 * Inter-module definitions exported by the Interface module of the @ref c_messaging implementation.
 *
 * See @ref messaging.c for an overview of the @ref c_messaging implementation.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LE_MESSAGING_INTERFACE_H_INCLUDE_GUARD
#define LE_MESSAGING_INTERFACE_H_INCLUDE_GUARD

#include "limit.h"
#include "serviceDirectory/serviceDirectoryProtocol.h"


//--------------------------------------------------------------------------------------------------
/**
 * The interface type that an generic Interface object represents.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_MSG_INTERFACE_UNDEFINED,
    LE_MSG_INTERFACE_SERVER,
    LE_MSG_INTERFACE_CLIENT
}
msgInterface_Type_t;


//--------------------------------------------------------------------------------------------------
/**
 * Interface identifier.  Contains everything needed to uniquely identify an interface instance.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_msg_ProtocolRef_t    protocolRef;          ///< The protocol that this interface supports.
    char    name[LIMIT_MAX_IPC_INTERFACE_NAME_BYTES]; ///< The interface instance name.
}
msgInterface_Id_t;


//--------------------------------------------------------------------------------------------------
/**
 * Generic Interface object. This is the abstraction of interface objects such as client and server.
 * This generic Interface object should NOT be created directly.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_msg_Interface
{
    msgInterface_Id_t id;              ///< The unique identifier for the interface.
    le_dls_List_t sessionList;         ///< List of Session objects for open sessions with other
                                       ///  interfaces.
    msgInterface_Type_t interfaceType; ///< The type of the more specific interface object.
}
msgInterface_Interface_t;


//--------------------------------------------------------------------------------------------------
/**
 * Service object.  Represents a single, unique service instance offered by a server.
 */
//--------------------------------------------------------------------------------------------------
typedef struct msgInterface_UnixService
{
    msgInterface_Interface_t interface; ///< The interface part of a service object.
                                        ///< Must be the first member in this structure.
    struct le_msg_Service service;      ///< Service member (includes type of service)
    // Stuff used only on the Server side:
    void*           contextPtr;         ///< Opaque value set using le_msg_SetServiceContextPtr().

    enum
    {
        LE_MSG_INTERFACE_SERVICE_CONNECTING,  ///< Connecting to the Service Directory.
        LE_MSG_INTERFACE_SERVICE_ADVERTISED,  ///< Connected to the Service Directory (advertised).
        LE_MSG_INTERFACE_SERVICE_HIDDEN       ///< Disconnected from the Service Directory (hidden).
    }
    state;

    int             directorySocketFd;  ///< File descriptor of socket connected to the
                                        ///  Service Directory (or -1 if not connected).

    le_fdMonitor_Ref_t fdMonitorRef;///< File descriptor monitor for the directory socket.

    le_thread_Ref_t serverThread;       ///< Thread that is acting as server in this process,
                                        ///  or NULL if no server exists in this process.

    le_msg_ReceiveHandler_t         recvHandler;    ///< Handler for when messages are received.
    void*                           recvContextPtr; ///< contextPtr parameter for recvHandler.

    le_dls_List_t                   openListPtr; ///< open List: list of open session handlers
                                                 ///  called when a session is opened

    le_dls_List_t                   closeListPtr; ///< open List: list of close session handlers
                                                  ///  called when a session is opened
}
msgInterface_UnixService_t;


//--------------------------------------------------------------------------------------------------
/**
 * Client interface object.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_msg_ClientInterface
{
    msgInterface_Interface_t interface; ///< The interface part of a client interface object.

    // Stuff used only on the Client side:
}
msgInterface_ClientInterface_t;


//--------------------------------------------------------------------------------------------------
/**
 * Exposing the service object map; mainly for the Inspect tool.
 */
//--------------------------------------------------------------------------------------------------
le_hashmap_Ref_t* msgInterface_GetServiceObjMap
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Exposing the service object map change counter; mainly for the Inspect tool.
 */
//--------------------------------------------------------------------------------------------------
size_t** msgInterface_GetServiceObjMapChgCntRef
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Exposing the client interface map; mainly for the Inspect tool.
 */
//--------------------------------------------------------------------------------------------------
le_hashmap_Ref_t* msgInterface_GetClientInterfaceMap
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Exposing the client interface map change counter; mainly for the Inspect tool.
 */
//--------------------------------------------------------------------------------------------------
size_t** msgInterface_GetClientInterfaceMapChgCntRef
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Initializes the module.  This must be called only once at start-up, before any other functions in
 * that module are called.
 */
//--------------------------------------------------------------------------------------------------
void msgInterface_Init
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets a reference to a Client Interface object.  Must be released using msgInterface_Release()
 * when you are done with it.
 *
 * @return  Reference to the client interface object.
 */
//--------------------------------------------------------------------------------------------------
le_msg_ClientInterfaceRef_t msgInterface_GetClient
(
    le_msg_ProtocolRef_t    protocolRef,
    const char*             interfaceName
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the interface details for a given interface object.
 */
//--------------------------------------------------------------------------------------------------
void msgInterface_GetInterfaceDetails
(
    le_msg_InterfaceRef_t interfaceRef,     ///< [in] Pointer to the interface.
    svcdir_InterfaceDetails_t* detailsPtr   ///< [out] Ptr to where the details will be stored.
);


//--------------------------------------------------------------------------------------------------
/**
 * Get a reference to the Protocol that an Interface is running.
 *
 * @return The protocol reference.
 */
//--------------------------------------------------------------------------------------------------
static inline le_msg_ProtocolRef_t msgInterface_GetProtocolRef
(
    le_msg_InterfaceRef_t interfaceRef
)
{
    return interfaceRef->id.protocolRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Release a reference to an Interface.
 */
//--------------------------------------------------------------------------------------------------
void msgInterface_Release
(
    le_msg_InterfaceRef_t interfaceRef,  ///< Interface Reference
    const bool mutexLocked  ///< Indicates whether Mutex is already locked
);


//--------------------------------------------------------------------------------------------------
/**
 * Add a Session to an Interface's list of open sessions.
 */
//--------------------------------------------------------------------------------------------------
void msgInterface_AddSession
(
    le_msg_InterfaceRef_t interfaceRef,
    le_msg_SessionRef_t sessionRef
);


//--------------------------------------------------------------------------------------------------
/**
 * Remove a Session from an Interface's list of open sessions.
 */
//--------------------------------------------------------------------------------------------------
void msgInterface_RemoveSession
(
    le_msg_InterfaceRef_t interfaceRef,  ///< Interface Reference
    le_msg_SessionRef_t sessionRef,   ///< Session Reference
    const bool mutexLocked   ///< Indicates whether Mutex is already locked
);


//--------------------------------------------------------------------------------------------------
/**
 * Call a Service's registered session close handler function, if there is one registered.
 */
//--------------------------------------------------------------------------------------------------
void msgInterface_CallCloseHandler
(
    msgInterface_UnixService_t* serviceRef,
    le_msg_SessionRef_t sessionRef
);


//--------------------------------------------------------------------------------------------------
/**
 * Dispatches a message received from a client to a service's server.
 */
//--------------------------------------------------------------------------------------------------
void msgInterface_ProcessMessageFromClient
(
    msgInterface_UnixService_t* servicePtr, ///< [IN] Reference to the Service object.
    le_msg_MessageRef_t msgRef      ///< [IN] Message reference for the received message.
);


#endif // LE_MESSAGING_INTERFACE_H_INCLUDE_GUARD
