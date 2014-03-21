/*
 * ====================== WARNING ======================
 *
 * THE CONTENTS OF THIS FILE HAVE BEEN AUTO-GENERATED.
 * DO NOT MODIFY IN ANY WAY.
 *
 * ====================== WARNING ======================
 */

/**
 * @page c_le_data Data Connection Service
 *
 * @ref le_data_interface.h "Click here for the API Reference documentation."
 *
 * <HR>
 *
 *  - @ref c_le_data_default
 *  - @ref c_le_data_options
 *
 *
 * A data connection is needed for applications that exchange data with 
 * devices where SMS messages are insufficient or not possible.  The data connection can 
 * be over a mobile network, over Wi-Fi, or over a fixed link (e.g., Ethernet).
 * 
 * The data connection service provides a basic API for requesting and releasing a data connection.
 *
 *
 * @section c_le_data_default Default Data Connection
 *
 * Default data connection is obtained using @ref le_data_Request.  Before the data 
 * connection is requested, an application registers a connection state handler using
 * @ref le_data_AddConnectionStateHandler.  Once the data connection is established, the
 * handler will be called indicating it's now connected.  If the state of the data
 * connection changes, then the handler will be called with the new state. To release a data
 * connection, an application can use @ref le_data_Release.
 *
 * If the default data connection is not currently available when @ref le_data_Request is called,
 * the data connection service first ensures all pre-conditions are satisfied (e.g., 
 * modem is registered on the network), before trying to start the data connection.  
 *
 * If the default data connection is already available when @ref le_data_Request is called, a
 * new connection will not be started.  Instead, the existing data connection will be used.  This
 * happens if another application also requested the default data connection.  This
 * is how multiple applications can share the same data connection.
 *
 * Once an application makes a data connection request, it should monitor the connection state
 * reported to the registered connection state handler.  The application
 * should only try transmitting data when the state is connected, and should stop transmitting
 * data when the state is not connected.  If the state is not connected, the data connection
 * service will try to re-establish the connection. There's no need for an application to
 * issue a new connection request.
 *
 * The default data connection will not necessarily be released when an application calls
 * @ref le_data_Release.  The data connection will be released only after @ref le_data_Release
 * is called by all applications that previously called @ref le_data_Request.
 *
 * All configuration data required for the default data connection, like the Access Point Name 
 * (APN), will be stored in the Config database.
 *
 * The default data connection will always use the mobile network.
 
 * @todo Should the default be configurable?
 * 
 *
 * @section c_le_data_options Data Connection Options
 *
 * @note Functionaliy described in this section is not currently implemented; this
 * description is provided to outline future functionality.
 * 
 * Some applications may have data connection requirements that are not met by the default data
 * connection (e.g., use a least cost data link or disable roaming on mobile networks). You can do this
 * by:
 * - creating a request object using @ref le_data_CreateRequest, 
 * - setting optional values on that request object using @ref le_data_SelectLeastCost or @ref le_data_DisableRoaming,
 * - and then submitting that object to a data connection request using @ref le_data_SubmitRequest
 *
 * @ref le_data_AddConnectionStateHandler and @ref le_data_Release APIs can continue to be
 * used, as described above.
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */
/**
 * @file le_data_interface.h
 *
 * Legato @ref c_le_data include file.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 *
 */

#ifndef LE_DATA_INTERFACE_H_INCLUDE_GUARD
#define LE_DATA_INTERFACE_H_INCLUDE_GUARD


#include "legato.h"

// User customizable include file
#include "le_data_defs.h"


//--------------------------------------------------------------------------------------------------
/**
 * Start the client main thread
 */
//--------------------------------------------------------------------------------------------------
void le_data_StartClient
(
    const char* serviceInstanceName
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * Stop the service for the current client thread
 */
//--------------------------------------------------------------------------------------------------
void le_data_StopClient
(
    void
);


typedef void (*le_data_ConnectionStateHandlerFunc_t)
(
    const char* intfName,
    bool isConnected,
    void* contextPtr
);

typedef struct le_data_ConnectionStateHandler* le_data_ConnectionStateHandlerRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * This function adds a handler ...
 */
//--------------------------------------------------------------------------------------------------
le_data_ConnectionStateHandlerRef_t le_data_AddConnectionStateHandler
(
    le_data_ConnectionStateHandlerFunc_t handlerPtr,
        ///< [IN]

    void* contextPtr
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * This function removes a handler ...
 */
//--------------------------------------------------------------------------------------------------
void le_data_RemoveConnectionStateHandler
(
    le_data_ConnectionStateHandlerRef_t addHandlerRef
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * Request the default data connection
 *
 * @return
 *      - Reference to the data connection (to be used later for releasing the connection)
 *      - NULL if the data connection requested could not be processed
 */
//--------------------------------------------------------------------------------------------------
le_data_Request_Ref_t le_data_Request
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Release a previously requested data connection
 */
//--------------------------------------------------------------------------------------------------
void le_data_Release
(
    le_data_Request_Ref_t requestRef
        ///< [IN]
        ///< Reference to a previously requested data connection
);


#endif // LE_DATA_INTERFACE_H_INCLUDE_GUARD

