/** @file messagingInterface.h
 *
 * Inter-module definitions exported by the Interface module of the @ref c_messaging implementation.
 *
 * See @ref messaging.c for an overview of the @ref c_messaging implementation.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
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
    le_msg_InterfaceRef_t interfaceRef
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
    le_msg_InterfaceRef_t interfaceRef,
    le_msg_SessionRef_t sessionRef
);


//--------------------------------------------------------------------------------------------------
/**
 * Call a Service's registered session close handler function, if there is one registered.
 */
//--------------------------------------------------------------------------------------------------
void msgInterface_CallCloseHandler
(
    le_msg_ServiceRef_t serviceRef,
    le_msg_SessionRef_t sessionRef
);


//--------------------------------------------------------------------------------------------------
/**
 * Dispatches a message received from a client to a service's server.
 */
//--------------------------------------------------------------------------------------------------
void msgInterface_ProcessMessageFromClient
(
    le_msg_ServiceRef_t serviceRef, ///< [IN] Reference to the Service object.
    le_msg_MessageRef_t msgRef      ///< [IN] Message reference for the received message.
);


#endif // LE_MESSAGING_INTERFACE_H_INCLUDE_GUARD
