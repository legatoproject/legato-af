/** @file messagingSession.h
 *
 * Inter-module definitions exported by the Session module of the @ref c_messaging implementation.
 *
 * See @ref messaging.c for an overview of the @ref c_messaging implementation.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#ifndef LE_MESSAGING_SESSION_H_INCLUDE_GUARD
#define LE_MESSAGING_SESSION_H_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Initializes the messagingSession module.  This must be called only once at start-up, before
 * any other functions in that module are called.
 */
//--------------------------------------------------------------------------------------------------
void msgSession_Init
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Checks whether a given Session reference is for the client side or the server side of a session.
 *
 * @return  true if client-side, false if server-side.
 */
//--------------------------------------------------------------------------------------------------
bool msgSession_IsClient
(
    le_msg_SessionRef_t sessionRef
);


//--------------------------------------------------------------------------------------------------
/**
 * Checks whether a given Session reference is for an open session.
 *
 * @return  true if the session is still open, false if it is closed.
 */
//--------------------------------------------------------------------------------------------------
bool msgSession_IsOpen
(
    le_msg_SessionRef_t sessionRef
);


//--------------------------------------------------------------------------------------------------
/**
 * Sends a given Message object through a given Session.
 */
//--------------------------------------------------------------------------------------------------
void msgSession_SendMessage
(
    le_msg_SessionRef_t sessionRef,
    le_msg_MessageRef_t messageRef
);


//--------------------------------------------------------------------------------------------------
/**
 * Start an asynchronous request-response transaction.
 */
//--------------------------------------------------------------------------------------------------
void msgSession_RequestResponse
(
    le_msg_SessionRef_t sessionRef,
    le_msg_MessageRef_t msgRef
);


//--------------------------------------------------------------------------------------------------
/**
 * Do a synchronous request-response transaction.
 */
//--------------------------------------------------------------------------------------------------
le_msg_MessageRef_t msgSession_DoSyncRequestResponse
(
    le_msg_SessionRef_t sessionRef,
    le_msg_MessageRef_t msgRef
);


//--------------------------------------------------------------------------------------------------
/**
 * Fetches the service reference for a given Session object.
 *
 * @return  The service reference.
 */
//--------------------------------------------------------------------------------------------------
le_msg_ServiceRef_t msgSession_GetServiceRef
(
    le_msg_SessionRef_t sessionRef
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets a pointer to the list link inside of a Session object.  This is used to link the
 * Session onto a Service's list of open sessions.
 *
 * @return  Pointer to a link.
 */
//--------------------------------------------------------------------------------------------------
le_dls_Link_t* msgSession_GetListLink
(
    le_msg_SessionRef_t sessionRef
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets a reference to the Session object in which a given list link exists.
 *
 * @return The reference.
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionRef_t msgSession_GetSessionContainingLink
(
    le_dls_Link_t* linkPtr
);


//--------------------------------------------------------------------------------------------------
/**
 * Creates a server-side Session object for a given client connection to a given Service.
 *
 * @return A reference to the newly created Session object.
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionRef_t msgSession_CreateServerSideSession
(
    le_msg_ServiceRef_t serviceRef,
    int                 fd          ///< [IN] File descriptor of socket connected to client.
);


#endif // LE_MESSAGING_SESSION_H_INCLUDE_GUARD
