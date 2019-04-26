/**
 * @page c_le_httpClient HTTP client
 *
 * @ref le_httpClientLib.h "API Reference"
 *
 * <HR>
 *
 * @section http_overview Overview
 *
 * HTTP client library allows user application to communicate with a remote HTTP server with or
 * without SSL encryption. HTTP client library features are:
 *
 * - Multi-app safe APIs
 * - Supports HTTP(s) version 1.1
 * - Supports mostly-used HTTP commands. Check @ref le_httpCommand_t for the complete list.
 * - Synchronous and asynchronous HTTP requests
 * - Credentials management
 *
 * Interactions between user application and HTTP client library rely on a set of callbacks to
 * build a request. The main advantage of this technique is to reduce memory usage and allocated
 * buffers. In fact, it allows to send chunks of the request instead of storing it entirely in
 * memory and send it at once.
 *
 * @section http_client_include HTTP client interface include
 *
 * Code sample for including HTTP client library in a user application:
 * @code
 *  requires:
 *  {
 *      component:
 *      {
 *          $LEGATO_ROOT/components/httpClientLibrary
 *      }
 *  }
 *
 *  cflags:
 *  {
 *      -I$LEGATO_ROOT/components/httpClientLibrary
 *  }
 * @endcode
 *
 * @section http_client_reference HTTP client reference
 *
 * Since HTTP client library is multi-app safe, a reference needs to be created in order to use the
 * APIs. First call @ref le_httpClient_Create and specify the host address and port. The returned
 * context reference must be used later to send HTTP requests. Call @ref le_httpClient_Delete to
 * free the previously allocated context when finished.
 *
 * Example code:
 * @snippet "apps/test/httpServices/httpIntegrationTest/httpTestComponent/httpTest.c" HttpConnect
 *
 * @section http_client_callbacks HTTP client callbacks
 *
 * As previously mentioned, HTTP library relies on callbacks to achieve exchanges with the user
 * application. There are three types of callbacks:
 *
 * - Input callbacks: used to inject data in the HTTP request:
 *  - @ref le_httpClient_SetResourceUpdateCallback
 *  - @ref le_httpClient_SetBodyConstructCallback
 *
 * - Output callbacks: used to retrieve data from HTTP server response:
 *  - @ref le_httpClient_SetBodyResponseCallback
 *  - @ref le_httpClient_SetHeaderResponseCallback
 *
 * - Events callbacks: used to report a specific status or event to the user application:
 *  - @ref le_httpClient_SetStatusCodeCallback
 *  - @ref le_httpClient_SetEventCallback
 *
 * These callbacks are not mandatory. Also, it is possible to remove a previously registered
 * callback by putting @c NULL in the callback argument.
 *
 * Example code:
 * @snippet "apps/test/httpServices/httpIntegrationTest/httpTestComponent/httpTest.c" HttpStatusCb
 * @snippet "apps/test/httpServices/httpIntegrationTest/httpTestComponent/httpTest.c" HttpSetCb
 *
 * @section http_client_synchronous Synchronous API
 * Once a reference is created and callbacks subscribed, user application can send HTTP requests
 * in a synchronous way using @ref le_httpClient_SendRequest.
 * This API blocks until request is sent to remote server and response is parsed and delivered
 * through callbacks.
 *
 * A default timeout of 10 sec is implemented to prevent infinite wait. This duration can be
 * modified by calling @ref le_httpClient_SetTimeout API.
 *
 * Workflow example when all callbacks are subscribed:
 * @code
 * +-----------------+                                                         +-------------------+
 * |User application |                                                         |HTTP client service|
 * +-------+---------+                                                         +---------+---------+
 *         |                                                                             |
 *         |  Connection initialization, callbacks subscriptions.                        |
 *         | +------------------------------------------------------------------------>  |
 *         |                                                                             |
 *         |  Send HTTP command request line by calling le_httpClient_SendRequest        |
 *         | +------------------------------------------------------------------------>  |
 *         |                                                                             |
 *         |  le_httpClient_ResourceUpdateCb_t callback:                                 |
 *         |  User fills a key/value pair and returns a status code.                     |
 *         |  If LE_OK is returned, callback is re-called to gather another key/value    |
 *         |  If LE_TERMINATED is returned, then no callback is called after.            |
 *         |  <-----------------------------------------------------------------------+  |
 *         |                                                                             |
 *         |  le_httpClient_BodyConstructCb_t callback:                                  | POST/PUT
 *         |  User fills a body data chunk and returns a status code.                    | requests
 *         |  If LE_OK is returned, callback is called again to gather the next chunk.   | only
 *         |  If LE_TERMINATED is returned, then no callback is called after.            |
 *         |  <-----------------------------------------------------------------------+  |
 *         |                                                                             |
 *         |           (Client service sends the request and parses the response)        |
 *         |                                                                             |
 *         |  le_httpClient_StatusCodeCb_t callback:                                     |
 *         |  This callback is called to report the HTTP status code to user             |
 *         |  <-----------------------------------------------------------------------+  |
 *         |                                                                             |
 *         |  le_httpClient_HeaderResponseCb_t callback:                                 |
 *         |  For each decoded key/value pair, a callback is called to report their      |
 *         |  content to user.                                                           |
 *         |  <-----------------------------------------------------------------------+  |
 *         |                                                                             |
 *         |  le_httpClient_BodyResponseCb_t callback:                                   | GET
 *         |  If the HTML body is requested, multiple callbacks may be called to report  | requests
 *         |  the body content to user.                                                  | only
 *         | <-----------------------------------------------------------------------+   |
 *         |                                                                             |
 *         | (le_httpClient_SendRequest() returns, denoting the end of HTTP transaction) |
 *         |                                                                             |
 *         +                                                                             +
 * @endcode
 *
 * @section http_client_asynchronous Asynchronous API
 * le_httpClient_SendRequestAsync API allows user to send HTTP request without blocking the current
 * thread. Calling thread must have a running  @ref le_event_runLoop.
 *
 * Workflow example:
 * @code
 *+-----------------+                                                         +-------------------+
 *|User application |                                                         |HTTP client service|
 *+-------+---------+                                                         +---------+---------+
 *        |                                                                             |
 *        |  Connection initializations, callbacks subscriptions.                       |
 *        | +------------------------------------------------------------------------>  |
 *        |                                                                             |
 *        |  Enable asynchronous mode by calling le_httpClient_SetAsyncMode             |
 *        | +------------------------------------------------------------------------>  |
 *        |                                                                             |
 *        |  Send HTTP command request line by calling le_httpClient_SendRequestAsync   |
 *        | +------------------------------------------------------------------------>  |
 *        |                                                                             |
 *        |  le_httpClient_SendRequestAsync immediately returns.                        |
 *        |  <-----------------------------------------------------------------------+  |
 *        |                                                                             |
 *        | (Similarly to synchronous mode, callbacks are called but this time from user|
 *        |                        application run loop)                                |
 *        |                                                                             |
 *        |  SendRequestRsp callback:                                                   |
 *        |  Denotes the end of HTTP transaction with final execution status            |
 *        |  <-----------------------------------------------------------------------+  |
 *        |                                                                             |
 *        +                                                                             +
 * @endcode
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * @file le_httpClientLib.h
 *
 * HTTP client library @ref c_le_httpClient include file
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LE_HTTP_CLIENT_LIB_H
#define LE_HTTP_CLIENT_LIB_H

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Enum for HTTP command
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    HTTP_HEAD,      ///< Enum for HTTP HEAD command
    HTTP_GET,       ///< Enum for HTTP GET command
    HTTP_POST,      ///< Enum for HTTP POST command
    HTTP_PUT,       ///< Enum for HTTP PUT command
    HTTP_DELETE,    ///< Enum for HTTP DELETE command
    HTTP_MAX        ///< Maximum value for HTTP commands
}
le_httpCommand_t;

//--------------------------------------------------------------------------------------------------
/**
 * Enum for HTTP asynchronous events
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_HTTP_CLIENT_EVENT_NONE,   ///< No event
    LE_HTTP_CLIENT_EVENT_CLOSED  ///< HTTP connection closed by remote server
}
le_httpClient_Event_t;

//--------------------------------------------------------------------------------------------------
/**
 *  Reference to the HTTP client context
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_httpClient_Ref*  le_httpClient_Ref_t;

//--------------------------------------------------------------------------------------------------
/**
 *  Callback definition for HTTP body response.
 *  Arguments are filled by the parser and reported to user, one chunk at a time.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*le_httpClient_BodyResponseCb_t)
(
    le_httpClient_Ref_t ref,        ///< [IN] HTTP session context reference
    const char*         dataPtr,    ///< [IN] Received data pointer
    int                 size        ///< [IN] Received data size
);

//--------------------------------------------------------------------------------------------------
/**
 *  Callback definition for HTTP header response.
 *  Arguments are filled by the parser and reported to user, one key at a time.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*le_httpClient_HeaderResponseCb_t)
(
    le_httpClient_Ref_t ref,        ///< [IN] HTTP session context reference
    const char*         keyPtr,     ///< [IN] Key field in the HTTP header response
    int                 keyLen,     ///< [IN] Key field length
    const char*         valuePtr,   ///< [IN] Key value in the HTTP header response
    int                 valueLen    ///< [IN] Key value length
);

//--------------------------------------------------------------------------------------------------
/**
 *  Callback definition for HTTP status code
 *  Status code is filled by the parser and reported to user, denoting the end of a HTTP transaction
 */
//--------------------------------------------------------------------------------------------------
typedef void (*le_httpClient_StatusCodeCb_t)
(
    le_httpClient_Ref_t ref,        ///< [IN] HTTP session context reference
    int                 code        ///< [IN] HTTP status code
);

//--------------------------------------------------------------------------------------------------
/**
 *  Callback definition for HTTP body construction.
 *  User fills the provided buffer and size with data, then returns a status code.
 *
 * @return
 *  - LE_OK            Callback should be called again to gather another chunk of data
 *  - LE_TERMINATED    All data have been transmitted, do not recall callback
 *  - LE_WOULD_BLOCK   Suspend current request and resume when @ref le_httpClient_Resume is called
 *  - LE_FAULT         Internal error
 *
 * @note Suspend mechanism is only relevant for asynchronous HTTP requests.
 */
//--------------------------------------------------------------------------------------------------
typedef le_result_t (*le_httpClient_BodyConstructCb_t)
(
    le_httpClient_Ref_t ref,        ///< [IN] HTTP session context reference
    char*               dataPtr,    ///< [OUT] Data pointer
    int*                sizePtr     ///< [INOUT] Data pointer size
);

//--------------------------------------------------------------------------------------------------
/**
 *  Callback definition for resources (key/value pairs) insertion.
 *  User fills the provided buffers and sizes with data, then returns a status code.
 *
 * @return
 *  - LE_OK            Callback should be called again to gather another key/value pair
 *  - LE_TERMINATED    All keys have been transmitted, do not recall callback
 *  - LE_WOULD_BLOCK   Suspend current request and resume when @ref le_httpClient_Resume is called
 *  - LE_FAULT         Internal error
 *
 * @note Suspend mechanism is only relevant for asynchronous HTTP requests.
 */
//--------------------------------------------------------------------------------------------------
typedef le_result_t (*le_httpClient_ResourceUpdateCb_t)
(
    le_httpClient_Ref_t ref,           ///< [IN] HTTP session context reference
    char*               keyPtr,        ///< [OUT] Key field pointer
    int*                keyLenPtr,     ///< [INOUT] Key field size
    char*               valuePtr,      ///< [OUT] Key value pointer
    int*                valueLenPtr    ///< [INOUT] Key value size
);

//--------------------------------------------------------------------------------------------------
/**
 *  Callback definition for @c le_httpClient_SendRequestAsync result value.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*le_httpClient_SendRequestRspCb_t)
(
    le_httpClient_Ref_t ref,          ///< [IN] HTTP session context reference
    le_result_t         result        ///< [IN] Asynchronous send response result. Possible values
                                      ///<      are the same as in @ref le_httpClient_SendRequest
);

//--------------------------------------------------------------------------------------------------
/**
 * Callback definition for asynchronous events.
 * The possible event types are described in @c le_httpClient_Event_t
*/
//--------------------------------------------------------------------------------------------------
typedef void (*le_httpClient_EventCb_t)
(
    le_httpClient_Ref_t   ref,          ///< [IN] HTTP session context reference
    le_httpClient_Event_t event         ///< [IN] Event that occured
);

//--------------------------------------------------------------------------------------------------
// Public functions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Create a HTTP session reference and store the host address in a dedicated context.
 *
 * @return
 *  - Reference to the created context
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_httpClient_Ref_t le_httpClient_Create
(
    char*            hostPtr,     ///< [IN] HTTP server address
    uint16_t         port         ///< [IN] HTTP server port numeric number (0-65535)
);

//--------------------------------------------------------------------------------------------------
/**
 * Delete a previously created HTTP session and free allocated resources.
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_FAULT         Internal error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_httpClient_Delete
(
    le_httpClient_Ref_t    ref  ///< [IN] HTTP session context reference
);

//--------------------------------------------------------------------------------------------------
/**
 * Set the HTTP session communication timeout. This timeout is used when server takes too much time
 * before responding.
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_FAULT         Internal error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_httpClient_SetTimeout
(
    le_httpClient_Ref_t    ref,       ///< [IN] HTTP session context reference
    uint32_t               timeout    ///< [IN] Timeout in milliseconds
);

//--------------------------------------------------------------------------------------------------
/**
 * Set user credentials to the HTTP session.
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_httpClient_SetCredentials
(
    le_httpClient_Ref_t    ref,         ///< [IN] HTTP session context reference
    char*                  loginPtr,    ///< [IN] User name to be used during the HTTP connection
    char*                  passwordPtr  ///< [IN] Password to be used during the HTTP connection
);

//--------------------------------------------------------------------------------------------------
/**
 * Add a certificate to the HTTP session in order to make the connection secure
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_FORMAT_ERROR  Invalid certificate
 *  - LE_FAULT         Internal error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_httpClient_AddCertificate
(
    le_httpClient_Ref_t  ref,             ///< [IN] HTTP session context reference
    const uint8_t*       certificatePtr,  ///< [IN] Certificate Pointer
    size_t               certificateLen   ///< [IN] Certificate Length
);

//--------------------------------------------------------------------------------------------------
/**
 * Initiate a connection with the server using the defined configuration.
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_TIMEOUT       Timeout occurred during communication
 *  - LE_UNAVAILABLE   Unable to reach the server or DNS issue
 *  - LE_FAULT         Internal error
 *  - LE_NO_MEMORY     Memory allocation issue
 *  - LE_CLOSED        In case of end of file error
 *  - LE_COMM_ERROR    Connection failure
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_httpClient_Start
(
    le_httpClient_Ref_t  ref    ///< [IN] HTTP session context reference
);

//--------------------------------------------------------------------------------------------------
/**
 * Stop the HTTP connection with the server.
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_FAULT         Internal error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_httpClient_Stop
(
    le_httpClient_Ref_t  ref     ///< [IN] HTTP session context reference
);

//--------------------------------------------------------------------------------------------------
/**
 * Send a HTTP command request and block until response is received from server or timeout reached.
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_TIMEOUT       Timeout occurred during communication
 *  - LE_BUSY          Busy state machine
 *  - LE_FAULT         Internal error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_httpClient_SendRequest
(
    le_httpClient_Ref_t  ref,              ///< [IN] HTTP session context reference
    le_httpCommand_t     command,          ///< [IN] HTTP command request
    char*                requestUriPtr     ///< [IN] URI buffer pointer
);

//--------------------------------------------------------------------------------------------------
/**
 * Set callback to handle HTTP response body data
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_FAULT         Internal error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_httpClient_SetBodyResponseCallback
(
    le_httpClient_Ref_t              ref,      ///< [IN] HTTP session context reference
    le_httpClient_BodyResponseCb_t   callback  ///< [IN] Callback
);


//--------------------------------------------------------------------------------------------------
/**
 * Set callback to handle HTTP header key/value pair
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_FAULT         Internal error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_httpClient_SetHeaderResponseCallback
(
    le_httpClient_Ref_t               ref,      ///< [IN] HTTP session context reference
    le_httpClient_HeaderResponseCb_t  callback  ///< [IN] Callback
);

//--------------------------------------------------------------------------------------------------
/**
 * Set callback to handle HTTP status code
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_FAULT         Internal error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_httpClient_SetStatusCodeCallback
(
    le_httpClient_Ref_t             ref,      ///< [IN] HTTP session context reference
    le_httpClient_StatusCodeCb_t    callback  ///< [IN] Callback
);

//--------------------------------------------------------------------------------------------------
/**
 * Set callback to fill HTTP body during a POST or PUT request
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_FAULT         Internal error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_httpClient_SetBodyConstructCallback
(
    le_httpClient_Ref_t              ref,       ///< [IN] HTTP session context reference
    le_httpClient_BodyConstructCb_t  callback   ///< [IN] Callback
);

//--------------------------------------------------------------------------------------------------
/**
 * Set callback to insert/update resources (key/value pairs) during a HTTP request.
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_FAULT         Internal error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_httpClient_SetResourceUpdateCallback
(
    le_httpClient_Ref_t              ref,      ///< [IN] HTTP session context reference
    le_httpClient_ResourceUpdateCb_t callback  ///< [IN] Callback
);

//--------------------------------------------------------------------------------------------------
/**
 * Set callback to get HTTP asynchronous events.
 * The possible event types are described in @c le_httpClient_Event_t
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_FAULT         Internal error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_httpClient_SetEventCallback
(
    le_httpClient_Ref_t              ref,      ///< [IN] HTTP session context reference
    le_httpClient_EventCb_t          callback  ///< [IN] Callback
);

//--------------------------------------------------------------------------------------------------
/**
 * Enable or disable HTTP client asynchronous mode. By default, HTTP client is synchronous.
 *
 * @note If asynchronous mode is enabled, calling thread should provide an event loop to catch
 *       remote server events after using @c le_httpClient_SendRequestAsync() API.
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_DUPLICATE     Request already executed
 *  - LE_FAULT         Internal error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_httpClient_SetAsyncMode
(
    le_httpClient_Ref_t     ref,      ///< [IN] HTTP session context reference
    bool                    enable    ///< [IN] True to activate asynchronous mode, false otherwise
);

//--------------------------------------------------------------------------------------------------
/**
 * Check whether the HTTP client mode is set to asynchronous.
 *
 * @return
 *  - True if the current mode is asynchronous, false otherwise.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED bool le_httpClient_IsAsyncMode
(
    le_httpClient_Ref_t     ref       ///< [IN] HTTP session context reference
);

//--------------------------------------------------------------------------------------------------
/**
 * Send a HTTP command request to remote server. Response reception is handled in an asynchronous
 * way in the calling thread event loop.  This API is non-blocking.
 *
 * @note Function execution result can be retrieved through the provided callback
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void le_httpClient_SendRequestAsync
(
    le_httpClient_Ref_t              ref,           ///< [IN] HTTP session context reference
    le_httpCommand_t                 command,       ///< [IN] HTTP command request
    char*                            requestUriPtr, ///< [IN] URI buffer pointer
    le_httpClient_SendRequestRspCb_t callback       ///< [IN] Function execution result callback
);

//--------------------------------------------------------------------------------------------------
/**
 * Resume asynchronous HTTP request execution
 *
 * @note Resume mechanism is only relevant when user has suspended the current asynchronous HTTP
 *       request by issuing an LE_WOULD_BLOCK in @ref le_httpClient_BodyConstructCb_t or
 *       @ref le_httpClient_ResourceUpdateCb_t callbacks.
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_FAULT         Internal error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_httpClient_Resume
(
    le_httpClient_Ref_t     ref       ///< [IN] HTTP session context reference
);

#endif  // LE_HTTP_CLIENT_LIB_H