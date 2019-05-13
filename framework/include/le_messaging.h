/**
 * @page c_messaging Low-level Messaging API
 *
 * @subpage le_messaging.h "API Reference"
 *
 * <HR>
 *
 * Message-based interfaces in Legato are implemented in layers.
 * This low-level messaging API is at the bottom layer.
 * It's designed to support higher layers of the messaging system.  But it's also
 * intended to be easy to hand-code low-level messaging in C, when necessary.
 *
 * Two messaging types are supported, Local and Socket.  "Local" allows logically distinct apps to
 * be combined in one executable, and essentially mediates the direct function calls between them.
 * "Socket" uses UNIX domain sockets to communicate between distinct processes.
 *
 * This low-level messaging API supports:
 *  - (Socket only) remote party identification (addressing)
 *  - (Socket only) very late (runtime) discovery and binding of parties
 *  - (Socket only) in-process and inter-process message delivery
 *  - location transparency
 *  - sessions
 *  - (Socket only) access control
 *  - request/reply transactions
 *  - message buffer memory management
 *  - support for single-threaded and multi-threaded programs
 *  - some level of protection from protocol mismatches between parties in a session.
 *
 * This API is integrated with the Legato Event Loop API so components can interact with each
 * other using messaging without having to create threads or file descriptor
 * sets that block other software from handling other events.  Support for
 * integration with legacy POSIX-based programs is also provided.
 *
 * @section c_messagingInteractionModel Interaction Model
 *
 * The Legato low-level messaging system follows a service-oriented pattern:
 *  - Service providers advertise their service.
 *  - Clients open sessions with those services
 *  - Both sides can send and receive messages through the session.
 *
 * Clients and servers can both send one-way messages within a session.
 * Clients can start a request-response transaction by sending a request to the server,
 * and the server can send a response. Request-response transactions can be blocking or
 * non-blocking (with a completion callback). If the server dies or terminates the session
 * before sending the response, Legato will automatically terminate the transaction.
 *
 * Servers are prevented from sending blocking requests to clients as a safety measure.
 * If a server were to block waiting for one of its clients, it would open up the server
 * to being blocked indefinitely by one of its clients, which would allow one client to cause
 * a server to deny service to other clients.  Also, if a client started a blocking request-response
 * transaction at the same time that the server started a blocking request-response transaction
 * in the other direction, a deadlock would occur.
 *
 * @section c_messagingAddressing Addressing
 *
 * Servers and clients have interfaces that can been connected to each other via bindings.  Both
 * client-side and server-side interfaces are identified by name, but the names don't have to match
 * for them to be bound to each other.  The binding determines which server-side interface will
 * be connected to when a client opens a session.
 *
 * Server-side interfaces are also known as "services".
 *
 * When a session is opened by a client, a session reference is provided to both the client
 * and the server.  Messages are then sent within the session using the session reference.
 * This session reference becomes invalid when the session is closed.
 *
 * @section c_messagingProtocols Protocols
 *
 * Communication between client and server is done using a message-based protocol. This protocol
 * is defined at a higher layer than this API, so this API doesn't know the structure of the
 * message payloads or the correct message sequences. That means this API can't check
 * for errors in the traffic it carries. However, it does provide a basic mechanism for detecting
 * protocol mismatches by forcing both client and server to provide the protocol identifier
 * of the protocol to be used.  The client and server must also provide
 * the maximum message size, as an extra sanity check.
 *
 * To make this possible, the client and server must independently call
 * @c le_msg_GetProtocolRef(), to get a reference to a "Protocol" object that encapsulates these
 * protocol details:
 *
 * @code
 * protocolRef = le_msg_GetProtocolRef(MY_PROTOCOL_ID, sizeof(myproto_Msg_t));
 * @endcode
 *
 * @note In this example, the protocol identifier (which is a string uniquely identifying a
 *       specific version of a specific protocol) and the message structure
 *       would be defined elsewhere, in a header file shared between the client and the
 *       server.  The structure @c myproto_Msg_t contains a
 *       @c union of all of the different messages included in the protocol, thereby
 *       making @c myproto_Msg_t as big as the biggest message in the protocol.
 *
 * When a server creates a service (by calling le_msg_CreateService()) and when a client creates
 * a session (by calling le_msg_CreateSession()), they are required to provide a reference to a
 * Protocol object that they obtained from le_msg_GetProtocolRef().
 *
 * @section c_messagingClientUsage Client Usage Model
 *
 * @ref c_messagingClientSending <br>
 * @ref c_messagingClientReceiving <br>
 * @ref c_messagingClientClosing <br>
 * @ref c_messagingClientMultithreading <br>
 * @ref c_messagingClientExample
 *
 * Clients that want to use a service do the following:
 *  -# Get a reference to the protocol they want to use by calling @c le_msg_GetProtocolRef().
 *  -# Create a session using @c le_msg_CreateSession(), passing in the protocol reference and
 *     the client's interface name.
 *  -# Optionally register a message receive callback using @c le_msg_SetSessionRecvHandler().
 *  -# Open the session using le_msg_OpenSession(), le_msg_OpenSessionSync(), or
 *     le_msg_TryOpenSessionSync().
 *
 * @code
 *     protocolRef = le_msg_GetProtocolRef(PROTOCOL_ID, sizeof(myproto_Msg_t));
 *     sessionRef = le_msg_CreateSession(protocolRef, MY_INTERFACE_NAME);
 *     le_msg_SetSessionRecvHandler(sessionRef, NotifyMsgHandlerFunc, NULL);
 *     le_msg_OpenSession(sessionRef, SessionOpenHandlerFunc, NULL);
 * @endcode
 *
 * The Legato framework takes care of setting up any IPC connections, as needed (or not, if the
 * client and server happen to be in the same process).
 *
 * When the session opens, the Event Loop will call the "session open handler" call-back function
 * that was passed into le_msg_OpenSession().
 *
 * le_msg_OpenSessionSync() is a synchronous alternative to le_msg_OpenSession(). The difference
 * is that le_msg_OpenSessionSync() will not return until the session has opened or failed to
 * open (most likely due to permissions settings).
 *
 * le_msg_TryOpenSessionSync() is like le_msg_OpenSessionSync() except that it will not wait
 * for a server session to become available if it's not already available at the time of the
 * call.  That is, if the client's interface is not bound to any service, or if the service that
 * it's bound to is not currently advertised by the server, then le_msg_TryOpenSessionSync()
 * will return an error code.
 *
 * @subsection c_messagingClientSending Sending a Message
 *
 * Before sending a message, the client must first allocate the message from the session's message
 * pool using le_msg_CreateMsg(). It can then get a pointer to the payload part of the
 * message using le_msg_GetPayloadPtr(). Once the message payload is populated, the client sends
 * the message.
 *
 * @code
 *     msgRef = le_msg_CreateMsg(sessionRef);
 *     msgPayloadPtr = le_msg_GetPayloadPtr(msgRef);
 *     msgPayloadPtr->... = ...; // <-- Populate message payload...
 * @endcode
 *
 * If no response is required from the server, the client sends the message using le_msg_Send().
 * At this point, the client has handed off the message to the messaging system, and the messaging
 * system will delete the message automatically once it has finished sending it.
 *
 * @code
 *     le_msg_Send(msgRef);
 * @endcode
 *
 * If the client expects a response from the server, the client can use
 * le_msg_RequestResponse() to send their message and specify a callback function to be called
 * when the response arrives.  This callback will be called by the event loop of the thread that
 * created the session (i.e., the thread that called le_msg_CreateSession()).
 *
 * @code
 *     le_msg_RequestResponse(msgRef, ResponseHandlerFunc, NULL);
 * @endcode
 *
 * If the client expects an immediate response from the server, and the client wants to block until
 * that response is received, it can use le_msg_RequestSyncResponse() instead of
 * le_msg_RequestResponse().  However, keep in mind that blocking the client thread will
 * block all event handlers that share that thread.  That's why le_msg_RequestSyncResponse() should
 * only be used when the server is expected to respond immediately, or when the client thread
 * is not shared by other event handlers.
 *
 * @code
 *     responseMsgRef = le_msg_RequestSyncResponse(msgRef);
 * @endcode
 *
 * @warning If the client and server are running in the same thread, and the
 * client calls le_msg_RequestSyncResponse(), it will return an error immediately, instead of
 * blocking the thread.  If the thread were blocked in this scenario, the server would also be
 * blocked and would therefore be unable to receive the request and respond to it, resulting in
 * a deadlock.
 *
 * When the client is finished with it, the <b> client must release its reference
 * to the response message </b> by calling le_msg_ReleaseMsg().
 *
 * @code
 *     le_msg_ReleaseMsg(responseMsgRef);
 * @endcode
 *
 * @subsection c_messagingClientReceiving Receiving a Non-Response Message
 *
 * When a server sends a message to the client that is not a response to a request from the client,
 * that non-response message will be passed to the receive handler that the client
 * registered using le_msg_SetSessionRecvHandler().  In fact, this is the only kind of message
 * that will result in that receive handler being called.
 *
 * @note Some protocols don't include any messages that are not responses to client requests,
 * which is why it's optional to register a receive handler on the client side.
 *
 * The payload of a received message can be accessed using le_msg_GetPayloadPtr(), and the client
 * can check what session the message arrived through by calling le_msg_GetSession().
 *
 * When the client is finished with the message, the <b> client must release its reference
 * to the message </b> by calling le_msg_ReleaseMsg().
 *
 * @code
 * // Function will be called whenever the server sends us a notification message.
 * static void NotifyHandlerFunc
 * (
 *     le_msg_MessageRef_t  msgRef,    // Reference to the received message.
 *     void*                contextPtr // contextPtr passed into le_msg_SetSessionRecvHandler().
 * )
 * {
 *     // Process notification message from the server.
 *     myproto_Msg_t* msgPayloadPtr = le_msg_GetPayloadPtr(msgRef);
 *     ...
 *
 *     // Release the message, now that we are finished with it.
 *     le_msg_ReleaseMsg(msgRef);
 * }
 *
 * COMPONENT_INIT
 * {
 *     le_msg_ProtocolRef_t protocolRef;
 *     le_msg_SessionRef_t sessionRef;
 *
 *     protocolRef = le_msg_GetProtocolRef(PROTOCOL_ID, sizeof(myproto_Msg_t));
 *     sessionRef = le_msg_CreateSession(protocolRef, MY_INTERFACE_NAME);
 *     le_msg_SetSessionRecvHandler(sessionRef, NotifyHandlerFunc, NULL);
 *     le_msg_OpenSession(sessionRef, SessionOpenHandlerFunc, NULL);
 * }
 * @endcode
 *
 * @subsection c_messagingClientClosing Closing Sessions
 *
 * When the client is done using a service, it can close the session using le_msg_CloseSession().
 * This will leave the session object in existence, though, so that it can be opened again
 * using le_msg_OpenSession().
 *
 * @code
 *     le_msg_CloseSession(sessionRef);
 * @endcode
 *
 * To delete a session object, call le_msg_DeleteSession().  This will automatically close the
 * session, if it's still open (but won't automatically delete any messages).
 *
 * @code
 *     le_msg_DeleteSession(sessionRef);
 * @endcode
 *
 * @note If a client process dies while it has a session open, that session will be
 * automatically closed and deleted by the Legato framework, so there's no need to register process
 * clean-up handlers or anything like that for this purpose.
 *
 * Additionally, clients can choose to call le_msg_SetSessionCloseHandler() to register to be
 * notified when a session gets closed by the server.  Servers often keep state on behalf of their
 * clients, and if the server closes the session (or if the system closes the session because the
 * server died), the client most likely will still be operating under the assumption (now false)
 * that the server is maintaining state on its behalf.  If a client is designed to recover from
 * the server losing its state, the client can register a close handler and handle the close.
 *
 * @code
 *     le_msg_SetSessionCloseHandler(sessionRef, SessionCloseHandler, NULL);
 * @endcode
 *
 * However, most clients are not designed to recover from their session being closed by someone
 * else, so if a close handler is not registered by a client and the session closes for some
 * reason other than the client calling le_msg_CloseSession(), then the client process
 * will be terminated.
 *
 * @note If the client closes the session, the client-side session close handler will not be called,
 * even if one is registered.
 *
 * @subsection c_messagingClientMultithreading Multithreading
 *
 * The Low-Level Messaging API is thread safe, but not async safe.
 *
 * When a client creates a session, that session gets "attached" to the thread that created it
 * (i.e., the thread that called le_msg_CreateSession()).  That thread will then call any callbacks
 * registered for that session.
 *
 * Note that this implies that if the client thread that creates the session does not
 * run the Legato event loop then no callbacks will ever be called for that session.
 * To work around this, move the session creation to another thread that that uses the Legato event
 * loop.
 *
 * Furthermore, to prevent race conditions, only the thread that is attached to a given session
 * is allowed to call le_msg_RequestSyncResponse() for that session.
 *
 * @subsection c_messagingClientExample Sample Code
 *
 * @code
 * // Function will be called whenever the server sends us a notification message.
 * static void NotifyHandlerFunc
 * (
 *     le_msg_MessageRef_t  msgRef,    // Reference to the received message.
 *     void*                contextPtr // contextPtr passed into le_msg_SetSessionRecvHandler().
 * )
 * {
 *     // Process notification message from the server.
 *     myproto_Msg_t* msgPayloadPtr = le_msg_GetPayloadPtr(msgRef);
 *     ...
 *
 *     // Release the message, now that we are finished with it.
 *     le_msg_ReleaseMsg(msgRef);
 * }
 *
 * // Function will be called whenever the server sends us a response message or our
 * // request-response transaction fails.
 * static void ResponseHandlerFunc
 * (
 *     le_msg_MessageRef_t  msgRef,    // Reference to response message (NULL if transaction failed).
 *     void*                contextPtr // contextPtr passed into le_msg_RequestResponse().
 * )
 * {
 *     // Check if we got a response.
 *     if (msgRef == NULL)
 *     {
 *         // Transaction failed.  No response received.
 *         // This might happen if the server deleted the request without sending a response,
 *         // or if we had registered a "Session End Handler" and the session terminated before
 *         // the response was sent.
 *         LE_ERROR("Transaction failed!");
 *     }
 *     else
 *     {
 *         // Process response message from the server.
 *         myproto_Msg_t* msgPayloadPtr = le_msg_GetPayloadPtr(msgRef);
 *         ...
 *
 *         // Release the response message, now that we are finished with it.
 *         le_msg_ReleaseMsg(msgRef);
 *     }
 * }
 *
 * // Function will be called when the client-server session opens.
 * static void SessionOpenHandlerFunc
 * (
 *     le_msg_SessionRef_t  sessionRef, // Reference tp the session that opened.
 *     void*                contextPtr  // contextPtr passed into le_msg_OpenSession().
 * )
 * {
 *     le_msg_MessageRef_t msgRef;
 *     myproto_Msg_t* msgPayloadPtr;
 *
 *     // Send a request to the server.
 *     msgRef = le_msg_CreateMsg(sessionRef);
 *     msgPayloadPtr = le_msg_GetPayloadPtr(msgRef);
 *     msgPayloadPtr->... = ...; // <-- Populate message payload...
 *     le_msg_RequestResponse(msgRef, ResponseHandlerFunc, NULL);
 * }
 *
 * COMPONENT_INIT
 * {
 *     le_msg_ProtocolRef_t protocolRef;
 *     le_msg_SessionRef_t sessionRef;
 *
 *     // Open a session.
 *     protocolRef = le_msg_GetProtocolRef(PROTOCOL_ID, sizeof(myproto_Msg_t));
 *     sessionRef = le_msg_CreateSession(protocolRef, MY_INTERFACE_NAME);
 *     le_msg_SetSessionRecvHandler(sessionRef, NotifyHandlerFunc, NULL);
 *     le_msg_OpenSession(sessionRef, SessionOpenHandlerFunc, NULL);
 * }
 * @endcode
 *
 * @section c_messagingServerUsage Server Usage Model
 *
 * @ref c_messagingServerProcessingMessages <br>
 * @ref c_messagingServerSendingNonResponse <br>
 * @ref c_messagingServerCleanUp <br>
 * @ref c_messagingRemovingService <br>
 * @ref c_messagingServerMultithreading <br>
 * @ref c_messagingServerExample
 *
 * Servers that wish to offer a service do the following:
 *  -# Get a reference to the protocol they want to use by calling le_msg_GetProtocolRef().
 *  -# Create a Service object using le_msg_CreateService(), passing in the protocol reference and
 *     the service name.
 *  -# Call le_msg_SetServiceRecvHandler() to register a function to handle messages received from
 *     clients.
 *  -# Advertise the service using le_msg_AdvertiseService().
 *
 * @code
 *     protocolRef = le_msg_GetProtocolRef(PROTOCOL_ID, sizeof(myproto_Msg_t));
 *     serviceRef = le_msg_CreateService(protocolRef, SERVER_INTERFACE_NAME);
 *     le_msg_SetServiceRecvHandler(serviceRef, RequestMsgHandlerFunc, NULL);
 *     le_msg_AdvertiseService(serviceRef);
 * @endcode
 *
 * Once the service is advertised, clients can open it and start sending it messages.  The server
 * will receive messages via callbacks to the function it registered using
 * le_msg_SetServiceRecvHandler().
 *
 * Servers also have the option of being notified when sessions are opened by clients.  They
 * get this notification by registering a handler function using le_msg_AddServiceOpenHandler().
 *
 * @code
 * // Function will be called whenever a client opens a session with our service.
 * static void SessionOpenHandlerFunc
 * (
 *     le_msg_SessionRef_t  sessionRef, ///< [in] Reference to the session object.
 *     void*                contextPtr  ///< [in] contextPtr passed to le_msg_SetSessionOpenHandler().
 * )
 * {
 *     // Handle new session opening...
 *     ...
 * }
 *
 * COMPONENT_INIT
 * {
 *     le_msg_ProtocolRef_t protocolRef;
 *     le_msg_ServiceRef_t serviceRef;
 *
 *     // Create my service and advertise it.
 *     protocolRef = le_msg_GetProtocolRef(PROTOCOL_ID, sizeof(myproto_Msg_t));
 *     serviceRef = le_msg_CreateService(protocolRef, SERVER_INTERFACE_NAME);
 *     le_msg_AddServiceOpenHandler(serviceRef, SessionOpenHandlerFunc, NULL);
 *     le_msg_AdvertiseService(serviceRef);
 * }
 * @endcode
 *
 * Both the "Open Handler" and the "Receive Handler" will be called by the Legato event loop in
 * the thread that registered those handlers (which must also be the same thread that created the
 * service).
 *
 * @subsection c_messagingServerProcessingMessages Processing Messages from Clients
 *
 * The payload of any received message can be accessed using le_msg_GetPayloadPtr().
 *
 * If a received message does not require a response (i.e., if the client sent it using
 * le_msg_Send()), then when the server is finished with the message, the server must release
 * the message by calling le_msg_ReleaseMsg().
 *
 * @code
 * void RequestMsgHandlerFunc
 * (
 *     le_msg_MessageRef_t msgRef  // Reference to the received message.
 * )
 * {
 *     myproto_Msg_t* msgPtr = le_msg_GetPayloadPtr(msgRef);
 *     LE_INFO("Received request '%s'", msgPtr->request.string);
 *
 *     // No response required and I'm done with this message, so release it.
 *     le_msg_ReleaseMsg(msgRef);
 * }
 * @endcode
 *
 * If a received message requires a response (i.e., if the client sent it using
 * le_msg_RequestResponse() or le_msg_RequestSyncResponse()), the server must eventually respond
 * to that message by calling le_msg_Respond() on that message.  le_msg_Respond() sends the message
 * back to the client that sent the request.  The response payload is stored inside the same
 * payload buffer that contained the request payload.
 *
 * To do this, the request payload pointer can be cast to a pointer to the response
 * payload structure, and then the response payload can be written into it.
 *
 * @code
 * void RequestMsgHandlerFunc
 * (
 *     le_msg_MessageRef_t msgRef  // Reference to the received message.
 * )
 * {
 *     myproto_RequestMsg_t* requestPtr = le_msg_GetPayloadPtr(msgRef);
 *     myproto_ResponseMsg_t* responsePtr;
 *
 *     LE_INFO("Received request '%s'", requestPtr->string);
 *
 *     responsePtr = (myproto_ResponseMsg_t*)requestPtr;
 *     responsePtr->value = Value;
 *     le_msg_Respond(msgRef);
 * }
 * @endcode
 *
 * Alternatively, the request payload structure and the response payload structure could be placed
 * into a union together.
 *
 * @code
 * typedef union
 * {
 *     myproto_Request_t request;
 *     myproto_Response_t response;
 * }
 * myproto_Msg_t;
 *
 *  ...
 *
 * void RequestMsgHandlerFunc
 * (
 *     le_msg_MessageRef_t msgRef  // Reference to the received message.
 * )
 * {
 *     myproto_Msg_t* msgPtr = le_msg_GetPayloadPtr(msgRef);
 *     LE_INFO("Received request '%s'", msgPtr->request.string);
 *     msgPtr->response.value = Value;
 *     le_msg_Respond(msgRef);
 * }
 * @endcode
 *
 * @warning Of course, once you've started writing the response payload into the buffer, the request payload
 * is no longer available, so if you still need it, copy it somewhere else first.
 *
 * @note The server doesn't have to send the response back to the client right away.  It could
 *       hold onto the request for an indefinite amount of time, for whatever reason.
 *
 * Whenever any message is received from a client, the message is associated with the session
 * through which the client sent it.  A reference to the session can be retrieved from the message,
 * if needed, by calling le_msg_GetSession().  This can be handy for tagging things in the
 * server's internal data structures that need to be cleaned up when the client closes the session
 * (see @ref c_messagingServerCleanUp for more on this).
 *
 * The function le_msg_NeedsResponse() can be used to check if a received message requires a
 * response or not.
 *
 * @subsection c_messagingServerSendingNonResponse Sending Non-Response Messages to Clients
 *
 * If a server wants to send a non-response message to a client, it first needs a reference
 * to the session that client opened.  It could have got the session reference from a previous
 * message received from the client (by calling le_msg_GetSession() on that message).
 * Or, it could have got the session reference from a Session Open Handler callback (see
 * le_msg_AddServiceOpenHandler()).  Either way, once it has the session reference, it can call
 * le_msg_CreateMsg() to create a message from that session's server-side message pool.  The
 * message can then be populated and sent in the same way that a client would send a message
 * to the server using le_msg_GetPayloadPtr() and le_msg_Send().
 *
 * @todo Explain how to configure the sizes of the server-side message pools.
 *
 * @code
 * // Function will be called whenever a client opens a session with our service.
 * static void SessionOpenHandlerFunc
 * (
 *     le_msg_SessionRef_t  sessionRef, ///< [in] Reference to the session object.
 *     void*                contextPtr  ///< [in] contextPtr passed to le_msg_SetSessionOpenHandler().
 * )
 * {
 *     le_msg_MessageRef_t msgRef;
 *     myproto_Msg_t* msgPayloadPtr;
 *
 *     // Send a "Welcome" message to the client.
 *     msgRef = le_msg_CreateMsg(sessionRef);
 *     msgPayloadPtr = le_msg_GetPayloadPtr(msgRef);
 *     msgPayloadPtr->... = ...; // <-- Populate message payload...
 *     le_msg_Send(msgRef);
 * }
 *
 * COMPONENT_INIT
 * {
 *     le_msg_ProtocolRef_t protocolRef;
 *     le_msg_ServiceRef_t serviceRef;
 *
 *     // Create my service and advertise it.
 *     protocolRef = le_msg_GetProtocolRef(PROTOCOL_ID, sizeof(myproto_Msg_t));
 *     serviceRef = le_msg_CreateService(protocolRef, SERVER_INTERFACE_NAME);
 *     le_msg_AddServiceOpenHandler(serviceRef, SessionOpenHandlerFunc, NULL);
 *     le_msg_AdvertiseService(serviceRef);
 * }
 * @endcode
 *
 * @subsection c_messagingServerCleanUp Cleaning up when Sessions Close
 *
 * If a server keeps state on behalf of its clients, it can call le_msg_AddServiceCloseHandler()
 * to ask to be notified when clients close sessions with a given service.  This allows the server
 * to clean up any state associated with a given session when the client closes that session
 * (or when the system closes the session because the client died).  The close handler is passed a
 * session reference, so the server can check its internal data structures and clean up anything
 * that it has previously tagged with that same session reference.
 *
 * @note    Servers don't delete sessions.  On the server side, sessions are automatically deleted
 *          when they close.
 *
 * @subsection c_messagingRemovingService Removing Service
 *
 * If a server wants to stop offering a service, it can hide the service by calling
 * le_msg_HideService().  This will not terminate any sessions that are already open, but it
 * will prevent clients from opening new sessions until it's advertised again.
 *
 * @warning Watch out for race conditions here. It's possible that a client is in the process
 * of opening a session when you decide to hide your service.  In this case, a new session may
 * open after you hid the service.  Be prepared to handle that gracefully.
 *
 * The server also has the option to delete the service. This hides the service and closes
 * all open sessions.
 *
 * If a server process dies, the Legato framework will automatically delete all of its services.
 *
 * @subsection c_messagingServerMultithreading Multithreading
 *
 * The Low-Level Messaging API is thread safe, but not async safe.
 *
 * When a server creates a service, that service gets attached to the thread that created it
 * (i.e., the thread that called le_msg_CreateService()).  That thread will call any handler
 * functions registered for that service.
 *
 * This implies that if the thread that creates the service doesn't
 * run the Legato event loop, then no callbacks will ever be called for that service.
 * To work around this, you could move the service to another thread that that runs the Legato event
 * loop.
 *
 * @subsection c_messagingServerExample Sample Code
 *
 * @code
 * void RequestMsgHandlerFunc
 * (
 *     le_msg_MessageRef_t msgRef,     // Reference to the received message.
 *     void*               contextPtr  ///< [in] contextPtr passed to le_msg_SetServiceRecvHandler().
 * )
 * {
 *     // Check the message type to decide what to do.
 *     myproto_Msg_t* msgPtr = le_msg_GetPayloadPtr(msgRef);
 *     switch (msgPtr->type)
 *     {
 *         case MYPROTO_MSG_TYPE_SET_VALUE:
 *              // Message doesn't require a response.
 *              Value = msgPtr->...;
 *              le_msg_ReleaseMsg(msgRef);
 *              break;
 *
 *         case MYPROTO_MSG_TYPE_GET_VALUE:
 *              // Message is a request that requires a response.
 *              // Notice that we just reuse the request message buffer for the response.
 *              msgPtr->... = Value;
 *              le_msg_Respond(msgRef);
 *              break;
 *
 *         default:
 *              // Unexpected message type!
 *              LE_ERROR("Received unexpected message type %d from session %s.",
 *                       msgPtr->type,
 *                       le_msg_GetInterfaceName(le_msg_GetSessionInterface(le_msg_GetSession(msgRef))));
 *              le_msg_ReleaseMsg(msgRef);
 *     }
 * }
 *
 * COMPONENT_INIT
 * {
 *     le_msg_ProtocolRef_t protocolRef;
 *     le_msg_ServiceRef_t serviceRef;
 *
 *     // Create my service and advertise it.
 *     protocolRef = le_msg_GetProtocolRef(PROTOCOL_ID, sizeof(myproto_Msg_t));
 *     serviceRef = le_msg_CreateService(protocolRef, SERVER_INTERFACE_NAME);
 *     le_msg_SetServiceRecvHandler(serviceRef, RequestMsgHandlerFunc, NULL);
 *     le_msg_AdvertiseService(serviceRef);
 * }
 * @endcode
 *
 * @section c_messagingStartUp Start Up Sequencing
 *
 * Worthy of special mention is the fact that the low-level messaging system can be used to
 * solve the age-old problem of coordinating the start-up sequence of processes that interact with
 * each other. Far too often, the start-up sequence of multiple interacting processes is addressed
 * using hacks like polling or sleeping for arbitrary lengths of time. These solutions can
 * waste a lot of CPU cycles and battery power, slow down start-up, and (in the case of arbitrary
 * sleeps) introduce race conditions that can cause failures in the field.
 *
 * In Legato, a messaging client can attempt to open a session before the server process has even
 * started.  The client will be notified asynchronously (via callback) when the server advertises
 * its service.
 *
 * In this way, clients are guaranteed to wait for the servers they use, without the inefficiency
 * of polling, and without having to add code elsewhere to coordinate the start-up sequence.
 * If there's work that needs to be done by the client at start-up before it opens
 * a session with the server, the client is allowed to do that work in parallel with the start-up
 * of the server, so the CPU can be more fully utilized to shorten the overall duration of the
 * start-up sequence.
 *
 * @section c_messagingMemoryManagement Memory Management
 *
 * Message buffer memory is allocated and controlled behind the scenes, inside the Messaging API.
 * This allows the Messaging API to
 *  - take some steps to remove programmer pitfalls,
 *  - provide some built-in remote troubleshooting features
 *  - encapsulate the IPC implementation, allowing for future optimization and porting.
 *
 * Each message object is allocated from a session. The sessions' message pool sizes can
 * be tuned through component and application configuration files and device configuration
 * settings.
 *
 * @todo    Explain how to configure message pools.
 *
 * Generally speaking, message payload sizes are determined by the protocol that is being used.
 * Application protocols and the packing of messages into message buffers are the domain of
 * higher-layers of the software stack. But, at this low layer, servers and clients just declare
 * the name and version of the protocol, and the size of the largest message in the protocol.
 * From this, they obtain a protocol reference that they provide to sessions when they create
 * them.
 *
 * @section c_messagingSecurity Security
 *
 * Security is provided in the form of authentication and access control.
 *
 * Clients cannot open sessions with servers until their client-side interface is "bound" to a
 * server-side interface (service).  The binding thereby provides configuration of both routing and
 * access control.
 *
 * Neither the client-side nor the server-side IPC sockets are named.  Therefore, no process other
 * than the Service Directory has access to these sockets.  The Service Directory passes client
 * connections to the appropriate server based on the binding configuration of the client's interface.
 *
 * The binding configuration is kept in the "system" configuration tree, so clients that do not
 * have write access to the "system" configuration tree have no control over their own binding
 * configuration.  By default, sandboxed apps do not have any access (read or write) to the
 * "system" configuration tree.
 *
 * @section c_messagingGetClientInfo Get Client Info
 *
 * In rare cases, a server may wish to check the user ID of the remote client.  Generally,
 * this is not necessary because the IPC system enforces user-based access control restrictions
 * automatically before allowing an IPC connection to be established.  However, sometimes it
 * may be useful when the service wishes to change the way it behaves, based on what user is
 * connected to it.
 *
 * le_msg_GetClientUserId() can be used to fetch the user ID of the client at the far end of a
 * given IPC session.
 *
 * @code
 * uid_t clientUserId;
 * if (le_msg_GetClientUserId(sessionRef, &clientUserId) != LE_OK)
 * {
 *     // The session must have closed.
 *     ...
 * }
 * else
 * {
 *     LE_INFO("My client has user ID %ud.", clientUserId);
 * }
 * @endcode
 *
 * le_msg_GetClientProcessId() can be used to fetch the process ID from the client.
 *
 * @code
 * pid_t clientProcessId;
 * if (le_msg_GetClientProcessId (sessionRef, &clientProcessId) != LE_OK)
 * {
 * {
 *     // The session must have closed.
 *     ...
 * }
 * else
 * {
 *    LE_INFO("My client has process ID %ud.", clientProcessId);
 * }
 * @endcode
 *
 * le_msg_GetClientUserCreds() can be used to fetch the user credentials from the client.
 *
 * @code
 *    le_msg_SessionRef_t sessionRef,
 *    uid_t*              userIdPtr,
 *    pid_t*              processIdPtr
 * @endcode
 *
 * @section c_messagingSendingFileDescriptors Sending File Descriptors
 *
 * It's possible to send an open file descriptor through an IPC session by adding an fd to a
 * message before sending it.  On the sender's side, le_msg_SetFd() is used to set the
 * file descriptor to be sent.  On the receiver's side, le_msg_GetFd() is used to get the fd
 * from the message.
 *
 * The IPC API will close the original fd in the sender's address space once it has
 * been sent, so if the sender still needs the fd open on its side, it should duplicate the fd
 * (e.g., using dup() ) before sending it.
 *
 * On the receiving side, if the fd is not extracted from the message, it will be closed when
 * the message is released.  The fd can only be extracted from the message once.  Subsequent
 * calls to le_msg_GetFd() will return -1.
 *
 * @warning DO NOT SEND DIRECTORY FILE DESCRIPTORS.  They can be exploited and used to break out of
 * chroot() jails.
 *
 * @section c_messagingFutureEnhancements Future Enhancements
 *
 * As an optimization to reduce the number of copies in cases where the sender of a message
 * already has the message payload of their message assembled somewhere (perhaps as static data
 * or in another message buffer received earlier from somewhere), a pointer to the payload could
 * be passed to the message, instead of having to copy the payload into the message.
 *
 * @code
 *     msgRef = le_msg_CreateMsg(sessionRef);
 *     le_msg_SetPayloadBuff(msgRef, &msgPayload, sizeof(msgPayload));
 *     msgRef = le_msg_RequestResponse(msgRef, ResponseHandlerFunc, contextPtr);
 * @endcode
 *
 * Perhaps an "iovec" version could be added to do scatter-gather too?
 *
 * @section c_messagingDesignNotes Design Notes
 *
 * We explored the option of having asynchronous messages automatically released when their
 * handler function returns, unless the handler calls an "AddRef" function before returning.
 * That would reduce the amount of code required in the common case.  However, we chose to require
 * that the client release the message explicitly in all cases, because the consequences of using
 * an invalid reference can be catastrophic and much more difficult to debug than forgetting to
 * release a message (which will generate pool growth warning messages in the log).
 *
 * @section c_messagingTroubleshooting Troubleshooting
 *
 * If you're running as the super-user (root), you can trace messaging traffic using @b TBD.
 * You can also inspect message queues and view lists of outstanding message objects within
 * processes using the Process Inspector tool.
 *
 * If you're leaking messages by forgetting to release them when you're finished with them,
 * you'll see warning messages in the log indicating your message pool is growing.
 * You should be able to tell the related messaging service by the name of the expanding pool.
 *
 * @todo Finish this section later, when the diagnostic tools become available.
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc.
 */


/** @file le_messaging.h
 *
 * Legato @ref c_messaging include file.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LE_MESSAGING_H_INCLUDE_GUARD
#define LE_MESSAGING_H_INCLUDE_GUARD

// =======================================
//  DATA TYPES
// =======================================

//--------------------------------------------------------------------------------------------------
/**
 * Reference to a protocol.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_msg_Protocol* le_msg_ProtocolRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * Reference to an interface's service instance.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_msg_Interface* le_msg_InterfaceRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * Reference to a server's service instance.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_msg_Service* le_msg_ServiceRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * Reference to a client's service instance.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_msg_ClientInterface* le_msg_ClientInterfaceRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * Reference to a client-server session.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_msg_Session* le_msg_SessionRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * Reference to a message.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_msg_Message* le_msg_MessageRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * Reference to a handler (call-back) function for events that can occur on a service (such as
 * opening and closing of sessions and receipt of messages).
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_msg_SessionEventHandler* le_msg_SessionEventHandlerRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * Handler function prototype for handlers that take session references as their arguments.
 *
 * See le_msg_SetSessionCloseHandler(), le_msg_AddServiceOpenHandler(), and
 * le_msg_AddServiceCloseHandler().
 *
 * @param sessionRef [in] Reference to the session that experienced the event.
 *
 * @param contextPtr [in] Opaque contextPtr value provided when the handler was registered.
 */
//--------------------------------------------------------------------------------------------------
typedef void (* le_msg_SessionEventHandler_t)
(
    le_msg_SessionRef_t sessionRef,
    void*               contextPtr
);


//--------------------------------------------------------------------------------------------------
/**
 * Receive handler function prototype.
 *
 * See le_msg_SetSessionRecvHandler() and le_msg_SetServiceRecvHandler().
 *
 * @param msgRef       [in] Reference to the received message.  Don't forget to release this using
 *                          le_msg_ReleaseMsg() when you're finished with it.
 *
 * @param contextPtr   [in] Opaque contextPtr value provided when the handler was registered.
 */
//--------------------------------------------------------------------------------------------------
typedef void (* le_msg_ReceiveHandler_t)
(
    le_msg_MessageRef_t msgRef,
    void*               contextPtr
);


//--------------------------------------------------------------------------------------------------
/**
 * Asynchronous response callback function prototype.
 *
 * See le_msg_RequestResponse().
 *
 * @param msgRef       [in] Reference to the received response message, or NULL if the transaction
 *                          failed and no response was received.  If not NULL, don't forget to
 *                          release it by calling le_msg_ReleaseMsg() when you're finished with it.
 *
 * @param contextPtr   [in] Opaque contextPtr value passed to le_msg_RequestResponse().
 */
//--------------------------------------------------------------------------------------------------
typedef void (* le_msg_ResponseCallback_t)
(
    le_msg_MessageRef_t msgRef,
    void*               contextPtr
);




//--------------------------------------------------------------------------------------------------
/**
 * Generic service object.  Used internally as part of low-level messaging implementation.
 */
//--------------------------------------------------------------------------------------------------
struct le_msg_Service
{
    enum
    {
        LE_MSG_SERVICE_UNIX_SOCKET,
        LE_MSG_SERVICE_LOCAL
    } type;
};


//--------------------------------------------------------------------------------------------------
/**
 * Generic message object.  Holds a pointer to the session the message is associated with.
 */
//--------------------------------------------------------------------------------------------------
struct le_msg_Message
{
    le_msg_SessionRef_t sessionRef;   ///< Session for this message
};


//--------------------------------------------------------------------------------------------------
/**
 * Message handler receiver for local messages
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_thread_Ref_t thread;           ///< Thread on which receive should be processes.
    le_msg_ReceiveHandler_t handler;  ///< Handler function which should be called on thread
    void* contextPtr;                 ///< Context pointer to pass to the handler
} le_msg_LocalReceiver_t;


//--------------------------------------------------------------------------------------------------
/**
 * Local service object.
 *
 * Create an instance of this object for each local service used by your program.
 *
 * @note  This structure should never be accessed directly; instead access through le_msg_...()
 *        functions.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    struct le_msg_Service service;          ///< Service
#if LE_CONFIG_CUSTOM_OS && defined(LE_MSG_SERVICE_READY_FLAG)
    LE_MSG_SERVICE_READY_FLAG;       ///< Indicate if service is ready in an OS-specific manner
#else
    bool serviceReady;               ///< Indicate if service is ready
#endif
    le_msg_LocalReceiver_t receiver; ///< Server destination
    le_mem_PoolRef_t messagePool;    ///< Pool for messages on this service
} le_msg_LocalService_t;


//--------------------------------------------------------------------------------------------------
/**
 * Message that's sent over a queue transport.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_msg_LocalMessage
{
    struct le_msg_Message message;          ///< Pointer to base message
    int fd;                                 ///< File descriptor sent with message (via Get/SetFd)
    le_sem_Ref_t responseReady;             ///< Semaphore which will be set when response is ready
    bool needsResponse;                     ///< True if message needs a response
    le_msg_ResponseCallback_t completionCallback; ///< Function to be called when transaction done.
    void* contextPtr;                       ///< Opaque value to be passed to handler function.
    uint8_t data[] __attribute__((aligned(__BIGGEST_ALIGNMENT__)));   ///< Start of message data
                                                                      // Align so any type of
                                                                      // data can be stored inside.
} le_msg_LocalMessage_t;


//--------------------------------------------------------------------------------------------------
/**
 * Size of local message header (needs to be added to size of message in local message pools).
 */
//--------------------------------------------------------------------------------------------------
#define LE_MSG_LOCAL_HEADER_SIZE      sizeof(le_msg_LocalMessage_t)


// =======================================
//  PROTOCOL FUNCTIONS
// =======================================

//--------------------------------------------------------------------------------------------------
/**
 * Gets a reference to refer to a particular version of a particular protocol.
 *
 * @return  Protocol reference.
 */
//--------------------------------------------------------------------------------------------------
LE_FULL_API le_msg_ProtocolRef_t le_msg_GetProtocolRef
(
    const char* protocolId,     ///< [in] String uniquely identifying the the protocol and version.
    size_t largestMsgSize       ///< [in] Size (in bytes) of the largest message in the protocol.
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets the unique identifier string of the protocol.
 *
 * @return  Pointer to the protocol identifier (null-terminated, UTF-8 string).
 */
//--------------------------------------------------------------------------------------------------
LE_FULL_API const char* le_msg_GetProtocolIdStr
(
    le_msg_ProtocolRef_t protocolRef    ///< [in] Reference to the protocol.
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets the protocol's maximum message size.
 *
 * @return  The size, in bytes.
 */
//--------------------------------------------------------------------------------------------------
LE_FULL_API size_t le_msg_GetProtocolMaxMsgSize
(
    le_msg_ProtocolRef_t protocolRef    ///< [in] Reference to the protocol.
);


// =======================================
//  SESSION FUNCTIONS
// =======================================

//--------------------------------------------------------------------------------------------------
/**
 * Creates a session that will make use of a protocol to talk to a service on a given client
 * interface.
 *
 * @note    This doesn't actually attempt to open the session.  It just creates the session
 *          object, allowing the client the opportunity to register handlers for the session
 *          before attempting to open it using le_msg_OpenSession().
 *
 * @return  Session reference.
 */
//--------------------------------------------------------------------------------------------------
LE_FULL_API le_msg_SessionRef_t le_msg_CreateSession
(
    le_msg_ProtocolRef_t    protocolRef,    ///< [in] Reference to the protocol to be used.
    const char*             interfaceName   ///< [in] Name of the client-side interface.
);


//--------------------------------------------------------------------------------------------------
/**
 * Create a session that will always use message boxes to talk to a service in the same process
 * space.
 *
 * @return   Session reference.
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionRef_t le_msg_CreateLocalSession
(
    le_msg_LocalService_t*             servicePtr  ///< [in] Reference to the service.
);


//--------------------------------------------------------------------------------------------------
/**
 * Sets an opaque context value (void pointer) that can be retrieved from that session later using
 * le_msg_GetSessionContextPtr().
 */
//--------------------------------------------------------------------------------------------------
LE_FULL_API void le_msg_SetSessionContextPtr
(
    le_msg_SessionRef_t sessionRef, ///< [in] Reference to the session.

    void*               contextPtr  ///< [in] Opaque value to be returned by
                                    ///         le_msg_GetSessionContextPtr().
);

//--------------------------------------------------------------------------------------------------
/**
 * Fetches the opaque context value (void pointer) that was set earlier using
 * le_msg_SetSessionContextPtr().
 *
 * @return  Context Ptr value passed into le_msg_SetSessionContextPtr(), or NULL if
 *          le_msg_SetSessionContextPtr() has not been called for this session yet.
 */
//--------------------------------------------------------------------------------------------------
LE_FULL_API void* le_msg_GetSessionContextPtr
(
    le_msg_SessionRef_t sessionRef  ///< [in] Reference to the session.
);


//--------------------------------------------------------------------------------------------------
/**
 * Deletes a session.  This will end the session and free up any resources associated
 * with it.  Any pending request-response transactions in this session will be terminated.
 * If the far end has registered a session close handler callback, it will be called.
 *
 * @note    Function is only used by clients.  On the server side, sessions are automatically
 *          deleted when they close.
 */
//--------------------------------------------------------------------------------------------------
void le_msg_DeleteSession
(
    le_msg_SessionRef_t sessionRef  ///< [in] Reference to the session.
);


//--------------------------------------------------------------------------------------------------
/**
 * Sets the receive handler callback function to be called when a non-response message arrives
 * on this session.
 *
 * The handler function will be called by the Legato event loop of the thread that created
 * the session.
 *
 * @note    This is a client-only function.  Servers are expected to use
 *          le_msg_SetServiceRecvHandler() instead.
 */
//--------------------------------------------------------------------------------------------------
void le_msg_SetSessionRecvHandler
(
    le_msg_SessionRef_t     sessionRef, ///< [in] Reference to the session.
    le_msg_ReceiveHandler_t handlerFunc,///< [in] Handler function.
    void*                   contextPtr  ///< [in] Opaque pointer value to pass to the handler.
);


//--------------------------------------------------------------------------------------------------
/**
 * Sets the handler callback function to be called when the session is closed from the other
 * end.  A local termination of the session will not trigger this callback.
 *
 * The handler function will be called by the Legato event loop of the thread that created
 * the session.
 *
 * @note
 * - If this isn't set on the client side, the framework assumes the client is not designed
 *   to recover from the server terminating the session, and the client process will terminate
 *   if the session is terminated by the server.
 * - This is a client-only function.  Servers are expected to use le_msg_AddServiceCloseHandler()
 *   instead.
 */
//--------------------------------------------------------------------------------------------------
LE_FULL_API void le_msg_SetSessionCloseHandler
(
    le_msg_SessionRef_t             sessionRef, ///< [in] Reference to the session.
    le_msg_SessionEventHandler_t    handlerFunc,///< [in] Handler function.
    void*                           contextPtr  ///< [in] Opaque pointer value to pass to handler.
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets the handler callback function to be called when the session is closed from the other
 * end.
 */
//--------------------------------------------------------------------------------------------------
void le_msg_GetSessionCloseHandler
(
    le_msg_SessionRef_t             sessionRef, ///< [in] Reference to the session.
    le_msg_SessionEventHandler_t*   handlerFunc,///< [out] Handler function.
    void**                          contextPtr  ///< [out] Opaque pointer value to pass to
                                                ///<       the handler.
);


//--------------------------------------------------------------------------------------------------
/**
 * Opens a session with a service, providing a function to be called-back when the session is
 * open.
 *
 * Asynchronous sessions are not supported by mailbox sessions.
 *
 * @note    Only clients open sessions.  Servers must patiently wait for clients to open sessions
 *          with them.
 *
 * @warning If the client and server don't agree on the maximum message size for the protocol,
 *          a fatal error will be logged and the client process will be killed.
 */
//--------------------------------------------------------------------------------------------------
void le_msg_OpenSession
(
    le_msg_SessionRef_t             sessionRef,     ///< [in] Reference to the session.
    le_msg_SessionEventHandler_t    callbackFunc,   ///< [in] Function to be called when open.
                                                    ///       NULL if no notification is needed.
    void*                           contextPtr      ///< [in] Opaque value to pass to the callback.
);


//--------------------------------------------------------------------------------------------------
/**
 * Synchronously open a session with a service.  Blocks until the session is open.
 *
 * This function logs a fatal error and terminates the calling process if unsuccessful.
 *
 * @note    Only clients open sessions.  Servers must patiently wait for clients to open sessions
 *          with them.
 *
 * @warning If the client and server do not agree on the maximum message size for the protocol,
 *          a fatal error will be logged and the client process will be killed.
 */
//--------------------------------------------------------------------------------------------------
void le_msg_OpenSessionSync
(
    le_msg_SessionRef_t             sessionRef      ///< [in] Reference to the session.
);


//--------------------------------------------------------------------------------------------------
/**
 * Synchronously open a session with a service.  Does not wait for the session to become available
 * if not available..
 *
 * le_msg_TryOpenSessionSync() differs from le_msg_OpenSessionSync() in that
 * le_msg_TryOpenSessionSync() will not wait for a server session to become available if it's
 * not already available at the time of the call.  That is, if the client's interface is not
 * bound to any service, or if the service that it's bound to is not currently advertised
 * by the server, then le_msg_TryOpenSessionSync() will return an error code, while
 * le_msg_OpenSessionSync() will wait until the binding is created or the server advertises
 * the service (or both).
 *
 * @return
 *  - LE_OK if the session was successfully opened.
 *  - LE_NOT_FOUND if the server is not currently offering the service to which the client is bound.
 *  - LE_NOT_PERMITTED if the client interface is not bound to any service (doesn't have a binding).
 *  - LE_COMM_ERROR if the Service Directory cannot be reached.
 *
 * @note    Only clients open sessions.  Servers' must patiently wait for clients to open sessions
 *          with them.
 *
 * @warning If the client and server do not agree on the maximum message size for the protocol,
 *          a fatal error will be logged and the client process will be killed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_msg_TryOpenSessionSync
(
    le_msg_SessionRef_t             sessionRef      ///< [in] Reference to the session.
);


//--------------------------------------------------------------------------------------------------
/**
 * Terminates a session.
 */
//--------------------------------------------------------------------------------------------------
void le_msg_CloseSession
(
    le_msg_SessionRef_t sessionRef  ///< [in] Reference to the session.
);


//--------------------------------------------------------------------------------------------------
/**
 * Terminates a session, already having acquired the Mutex lock.
 */
//--------------------------------------------------------------------------------------------------
void le_msg_CloseSessionLocked
(
    le_msg_SessionRef_t sessionRef  ///< [in] Reference to the session.
);


//--------------------------------------------------------------------------------------------------
/**
 * Fetches a reference to the protocol that is being used for a given session.
 *
 * @return Reference to the protocol.
 */
//--------------------------------------------------------------------------------------------------
LE_FULL_API le_msg_ProtocolRef_t le_msg_GetSessionProtocol
(
    le_msg_SessionRef_t sessionRef  ///< [in] Reference to the session.
);


//--------------------------------------------------------------------------------------------------
/**
 * Fetches a reference to the interface that is associated with a given session.
 *
 * @return Reference to the interface.
 */
//--------------------------------------------------------------------------------------------------
LE_FULL_API le_msg_InterfaceRef_t le_msg_GetSessionInterface
(
    le_msg_SessionRef_t sessionRef  ///< [in] Reference to the session.
);

//--------------------------------------------------------------------------------------------------
/**
 * Fetches the user ID of the client at the far end of a given IPC session.
 *
 * @warning This function can only be called for the server-side of a session.
 *
 * @return LE_OK if successful.
 *         LE_CLOSED if the session has closed.
 **/
//--------------------------------------------------------------------------------------------------
le_result_t le_msg_GetClientUserId
(
    le_msg_SessionRef_t sessionRef, ///< [in] Reference to the session.
    uid_t*              userIdPtr   ///< [out] Ptr to where the result is to be stored on success.
);

//--------------------------------------------------------------------------------------------------
/**
 * Fetches the user PID of the client at the far end of a given IPC session.
 *
 * @warning This function can only be called for the server-side of a session.
 *
 * @return LE_OK if successful.
 *         LE_CLOSED if the session has closed.
 **/
//--------------------------------------------------------------------------------------------------
le_result_t le_msg_GetClientProcessId
(
    le_msg_SessionRef_t sessionRef,   ///< [in] Reference to the session.
    pid_t*              processIdPtr  ///< [out] Ptr to where the result is to be stored on success.
);

//--------------------------------------------------------------------------------------------------
/**
 * Fetches the user credentials of the client at the far end of a given IPC session.
 *
 * @warning This function can only be called for the server-side of a session.
 *
 * @return LE_OK if successful.
 *         LE_CLOSED if the session has closed.
 **/
//--------------------------------------------------------------------------------------------------
le_result_t le_msg_GetClientUserCreds
(
    le_msg_SessionRef_t sessionRef,   ///< [in] Reference to the session.
    uid_t*              userIdPtr,    ///< [out] Ptr to where the uid is to be stored on success.
    pid_t*              processIdPtr  ///< [out] Ptr to where the pid is to be stored on success.
);



// =======================================
//  MESSAGE FUNCTIONS
// =======================================

//--------------------------------------------------------------------------------------------------
/**
 * Creates a message to be sent over a given session.
 *
 * @return  Message reference.
 *
 * @note
 * - Function never returns on failure, there's no need to check the return code.
 * - If you see warnings on message pools expanding, then you may be forgetting to
 *   release the messages you have received.
 * - In full API this can be called by either client or server, otherwise it can only
 *   be called by the client.
 */
//--------------------------------------------------------------------------------------------------
le_msg_MessageRef_t le_msg_CreateMsg
(
    le_msg_SessionRef_t sessionRef  ///< [in] Reference to the session.
);


//--------------------------------------------------------------------------------------------------
/**
 * Adds to the reference count on a message object.
 */
//--------------------------------------------------------------------------------------------------
void le_msg_AddRef
(
    le_msg_MessageRef_t msgRef      ///< [in] Reference to the message.
);


//--------------------------------------------------------------------------------------------------
/**
 * Releases a message object, decrementing its reference count.  If the reference count has reached
 * zero, the message object is deleted.
 */
//--------------------------------------------------------------------------------------------------
void le_msg_ReleaseMsg
(
    le_msg_MessageRef_t msgRef      ///< [in] Reference to the message.
);


//--------------------------------------------------------------------------------------------------
/**
 * Checks whether a message requires a response or not.
 *
 * @note    This is intended for use on the server side only.
 *
 * @return
 *  - TRUE if the message needs to be responded to using le_msg_Respond().
 *  - FALSE if the message doesn't need to be responded to, and should be disposed of using
 *    le_msg_ReleaseMsg() when it's no longer needed.
 */
//--------------------------------------------------------------------------------------------------
bool le_msg_NeedsResponse
(
    le_msg_MessageRef_t msgRef      ///< [in] Reference to the message.
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets a pointer to the message payload memory buffer.
 *
 * @return Pointer to the payload buffer.
 *
 * @warning Be careful not to overflow this buffer.
 */
//--------------------------------------------------------------------------------------------------
void* le_msg_GetPayloadPtr
(
    le_msg_MessageRef_t msgRef      ///< [in] Reference to the message.
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets the size, in bytes, of the message payload memory buffer.
 *
 * @return The size, in bytes.
 */
//--------------------------------------------------------------------------------------------------
size_t le_msg_GetMaxPayloadSize
(
    le_msg_MessageRef_t msgRef      ///< [in] Reference to the message.
);


//--------------------------------------------------------------------------------------------------
/**
 * Sets the file descriptor to be sent with this message.
 *
 * This file descriptor will be closed when the message is sent (or when it's deleted without
 * being sent).
 *
 * At most one file descriptor is allowed to be sent per message.
 **/
//--------------------------------------------------------------------------------------------------
void le_msg_SetFd
(
    le_msg_MessageRef_t msgRef,     ///< [in] Reference to the message.
    int                 fd          ///< [in] File descriptor.
);


//--------------------------------------------------------------------------------------------------
/**
 * Fetches a received file descriptor from the message.
 *
 * @return The file descriptor, or -1 if no file descriptor was sent with this message or if the
 *         fd was already fetched from the message.
 **/
//--------------------------------------------------------------------------------------------------
int le_msg_GetFd
(
    le_msg_MessageRef_t msgRef      ///< [in] Reference to the message.
);


//--------------------------------------------------------------------------------------------------
/**
 * Sends a message.  No response expected.
 */
//--------------------------------------------------------------------------------------------------
void le_msg_Send
(
    le_msg_MessageRef_t msgRef      ///< [in] Reference to the message.
);

//--------------------------------------------------------------------------------------------------
/**
 * Gets a reference to the session to which a given message belongs.
 *
 * @return Session reference.
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionRef_t le_msg_GetSession
(
    le_msg_MessageRef_t msgRef      ///< [in] Reference to the message.
);


//--------------------------------------------------------------------------------------------------
/**
 * Requests a response from a server by sending it a request.  Doesn't block.  Instead,
 * provides a callback function to be called when the response arrives or the transaction
 * terminates without a response (due to the session terminating or the server deleting the
 * request without responding).
 *
 * Async response not supported with mailbox API
 *
 * @note
 *     - The thread attached to the session (i.e., thread created by the session)
 *       will trigger the callback from its main event loop.  This means if
 *       that thread doesn't run its main event loop, it won't trigger the callback.
 *     - Function can only be used on the client side of a session.
 */
//--------------------------------------------------------------------------------------------------
void le_msg_RequestResponse
(
    le_msg_MessageRef_t         msgRef,     ///< [in] Reference to the request message.
    le_msg_ResponseCallback_t   handlerFunc,///< [in] Function to be called when transaction done.
    void*                       contextPtr  ///< [in] Opaque value to be passed to handler function.
);


//--------------------------------------------------------------------------------------------------
/**
 * Requests a response from a server by sending it a request.  Blocks until the response arrives
 * or until the transaction terminates without a response (i.e., if the session terminates or
 * the server deletes the request without responding).
 *
 * @return  Reference to the response message, or NULL if the transaction terminated without a
 *          response.
 *
 * @note
 *        - To prevent deadlocks, this function can only be used on the client side of a session.
 *          Servers can't use this function.
 *        - To prevent race conditions, only the client thread attached to the session
 *          (the thread that created the session) is allowed to perform a synchronous
 *          request-response transaction.
 *
 * @warning
 *        - The calling (client) thread will be blocked until the server responds, so no other
 *          event handling will happen in that client thread until the response is received (or the
 *          server dies).  This function should only be used when the server is certain
 *          to respond quickly enough to ensure that it will not cause any event response time
 *          deadlines to be missed by the client.  Consider using le_msg_RequestResponse()
 *          instead.
 *        - If this function is used when the client and server are in the same thread, then the
 *          message will be discarded and NULL will be returned.  This is a deadlock prevention
 *          measure.
 */
//--------------------------------------------------------------------------------------------------
le_msg_MessageRef_t le_msg_RequestSyncResponse
(
    le_msg_MessageRef_t msgRef      ///< [in] Reference to the request message.
);


//--------------------------------------------------------------------------------------------------
/**
 * Sends a response back to the client that send the request message.
 *
 * Takes a reference to the request message.  Copy the response payload (if any) into the
 * same payload buffer that held the request payload, then call le_msg_Respond().
 *
 * The messaging system will delete the message automatically when it's finished sending
 * the response.
 *
 * @note    Function can only be used on the server side of a session.
 */
//--------------------------------------------------------------------------------------------------
void le_msg_Respond
(
    le_msg_MessageRef_t msgRef      ///< [in] Reference to the request message.
);


// =======================================
//  INTERFACE FUNCTIONS
// =======================================

//--------------------------------------------------------------------------------------------------
/**
 * Creates a service that is accessible using a protocol.
 *
 * Mailbox services should be created statically.
 *
 * @return  Service reference.
 */
//--------------------------------------------------------------------------------------------------
LE_FULL_API le_msg_ServiceRef_t le_msg_CreateService
(
    le_msg_ProtocolRef_t    protocolRef,    ///< [in] Reference to the protocol to be used.
    const char*             interfaceName   ///< [in] Server-side interface name.
);


//--------------------------------------------------------------------------------------------------
/**
 * Initialize a mailbox service.
 *
 * This must be called before any client can connect to the service, for example in COMPONENT_INIT
 * before any other threads are created.
 */
//--------------------------------------------------------------------------------------------------
le_msg_ServiceRef_t le_msg_InitLocalService
(
    le_msg_LocalService_t* servicePtr,
    const char* serviceNameStr,
    le_mem_PoolRef_t messagingPoolRef
);


//--------------------------------------------------------------------------------------------------
/**
 * Deletes a service. Any open sessions will be terminated.
 *
 * @note    Server-only function.
 */
//--------------------------------------------------------------------------------------------------
LE_FULL_API void le_msg_DeleteService
(
    le_msg_ServiceRef_t             serviceRef  ///< [in] Reference to the service.
);

//--------------------------------------------------------------------------------------------------
/**
 * Registers a function to be called when clients open sessions with this service.
 *
 * @note    Server-only function.
 */
//--------------------------------------------------------------------------------------------------
LE_FULL_API le_msg_SessionEventHandlerRef_t le_msg_AddServiceOpenHandler
(
    le_msg_ServiceRef_t             serviceRef, ///< [in] Reference to the service.
    le_msg_SessionEventHandler_t    handlerFunc,///< [in] Handler function.
    void*                           contextPtr  ///< [in] Opaque pointer value to pass to handler.
);

//--------------------------------------------------------------------------------------------------
/**
 * Registers a function to be called whenever one of this service's sessions is closed by
 * the client.
 *
 * @note    Server-only function.
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionEventHandlerRef_t le_msg_AddServiceCloseHandler
(
    le_msg_ServiceRef_t             serviceRef, ///< [in] Reference to the service.
    le_msg_SessionEventHandler_t    handlerFunc,///< [in] Handler function.
    void*                           contextPtr  ///< [in] Opaque pointer value to pass to handler.
);

//--------------------------------------------------------------------------------------------------
/**
 * Remove a function previously registered by le_msg_AddServiceOpenHandler or
 * le_msg_AddServiceCloseHandler.
 *
 * @note    This is a server-only function.
 */
//--------------------------------------------------------------------------------------------------
LE_FULL_API void le_msg_RemoveServiceHandler
(
    le_msg_SessionEventHandlerRef_t handlerRef   ///< [in] Reference to a previously call of
                                                 ///       le_msg_AddServiceCloseHandler()
);

//--------------------------------------------------------------------------------------------------
/**
 * Registers a function to be called when messages are received from clients via sessions
 * that they have open with this service.
 *
 * @note    Server-only function.
 */
//--------------------------------------------------------------------------------------------------
void le_msg_SetServiceRecvHandler
(
    le_msg_ServiceRef_t     serviceRef, ///< [in] Reference to the service.
    le_msg_ReceiveHandler_t handlerFunc,///< [in] Handler function.
    void*                   contextPtr  ///< [in] Opaque pointer value to pass to the handler.
);


//--------------------------------------------------------------------------------------------------
/**
 * Associates an opaque context value (void pointer) with a given service that can be retrieved
 * later using le_msg_GetServiceContextPtr().
 *
 * @note    Server-only function.
 */
//--------------------------------------------------------------------------------------------------
LE_FULL_API void le_msg_SetServiceContextPtr
(
    le_msg_ServiceRef_t serviceRef, ///< [in] Reference to the service.

    void*               contextPtr  ///< [in] Opaque value to be returned by
                                    ///         le_msg_GetServiceContextPtr().
);


//--------------------------------------------------------------------------------------------------
/**
 * Fetches the opaque context value (void pointer) associated with a specified service using
 * le_msg_SetServiceContextPtr().
 *
 * @return  Context pointer value, or NULL if le_msg_SetServiceContextPtr() was never called
 *          for this service.
 *
 * @note    Server-only function.
 */
//--------------------------------------------------------------------------------------------------
LE_FULL_API void* le_msg_GetServiceContextPtr
(
    le_msg_ServiceRef_t serviceRef  ///< [in] Reference to the service.
);


//--------------------------------------------------------------------------------------------------
/**
 * Makes a given service available for clients to find.
 *
 * @note    Server-only function.
 */
//--------------------------------------------------------------------------------------------------
void le_msg_AdvertiseService
(
    le_msg_ServiceRef_t serviceRef  ///< [in] Reference to the service.
);


//--------------------------------------------------------------------------------------------------
/**
 * Makes a specified service unavailable for clients to find without terminating any ongoing
 * sessions.
 *
 * @note    Server-only function.
 */
//--------------------------------------------------------------------------------------------------
LE_FULL_API void le_msg_HideService
(
    le_msg_ServiceRef_t serviceRef  ///< [in] Reference to the service.
);


//--------------------------------------------------------------------------------------------------
/**
 * Fetches a pointer to the name of an interface.
 *
 * @return Pointer to a null-terminated string.
 *
 * @warning Pointer returned will remain valid only until the interface is deleted.
 */
//--------------------------------------------------------------------------------------------------
LE_FULL_API const char* le_msg_GetInterfaceName
(
    le_msg_InterfaceRef_t interfaceRef  ///< [in] Reference to the interface.
);


//--------------------------------------------------------------------------------------------------
/**
 * Fetches a reference to the protocol supported by a specified interface.
 *
 * @return  Protocol reference.
 */
//--------------------------------------------------------------------------------------------------
LE_FULL_API le_msg_ProtocolRef_t le_msg_GetInterfaceProtocol
(
    le_msg_InterfaceRef_t interfaceRef  ///< [in] Reference to the interface.
);


//--------------------------------------------------------------------------------------------------
/**
 * Check if the calling thread is currently running a Service's message receive handler;
 *  if so, return a reference to the message object being handled.
 *
 * @return  Reference to the message being handled, or NULL if no Service message receive handler
 *          is currently running.
 **/
//--------------------------------------------------------------------------------------------------
le_msg_MessageRef_t le_msg_GetServiceRxMsg
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Logs an error message (at EMERGENCY level) and:
 *  - if the caller is running a server-side IPC function, kills the connection to the client
 *    and returns.
 *  - if the caller is not running a server-side IPC function, kills the caller (doesn't return).
 **/
//--------------------------------------------------------------------------------------------------
#define LE_KILL_CLIENT(formatString, ...)  \
{ \
    le_msg_MessageRef_t msgRef = le_msg_GetServiceRxMsg(); \
    LE_FATAL_IF(msgRef == NULL, formatString, ##__VA_ARGS__); \
    LE_EMERG(formatString, ##__VA_ARGS__); \
    le_msg_CloseSession(le_msg_GetSession(msgRef)); \
}

#endif // LEGATO_MESSAGING_INCLUDE_GUARD
