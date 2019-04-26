/**
 * @file le_httpClientLib.c
 *
 * This file contains HTTP client specific interfaces.
 * le_httpClientLib is a thread-safe library that may be used in any Legato applications or
 * components.
 *
 * This library relies on callbacks to build a request and parse server response. The main advantage
 * of this implementation is to reduce memory usage: it allows to send chunks of the request instead
 * of storing and sending all at once.
 *
 * Check le_httpClientLib.h header for further details about usage and workflow.
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 */
#include "legato.h"
#include "interfaces.h"

#include "le_httpClientLib.h"
#include "le_socketLib.h"
#include "http.h"

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Maximum length for credential.
 * Credential format is the following: "user:password"
 */
//--------------------------------------------------------------------------------------------------
#define CRED_MAX_LEN                256

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of HTTP sessions. Note that increasing this value increases memory consumption
 */
//--------------------------------------------------------------------------------------------------
#define HTTP_SESSIONS_NB            2

//--------------------------------------------------------------------------------------------------
/**
 * Memory pool parameters for tinyHTTP library.
 * For each HTTP session, a MEM_MAX buffer is allocated to store key/values in tinyHTTP library.
 */
//--------------------------------------------------------------------------------------------------
#define MEM_MAX                     512               // Block of memory that can be allocated
#define MEM_MAX_COUNT               HTTP_SESSIONS_NB  // Number of blocks to allocate

//--------------------------------------------------------------------------------------------------
/**
 * Define value for CRLF
 */
//--------------------------------------------------------------------------------------------------
#define CRLF                        "\r\n"

//--------------------------------------------------------------------------------------------------
/**
 * HTTP request buffer size. This buffer is used internally when constructing HTTP requests.
 */
//--------------------------------------------------------------------------------------------------
#define REQUEST_BUFFER_SIZE         1024

//--------------------------------------------------------------------------------------------------
/**
 * HTTP response buffer size. This buffer is used internally when reading HTTP response.
 */
//--------------------------------------------------------------------------------------------------
#define RESPONSE_BUFFER_SIZE        1024


//--------------------------------------------------------------------------------------------------
/**
 * Specific error code introduced in tinyHttp's http_data() function to detect HTTP_HEAD command.
 */
//--------------------------------------------------------------------------------------------------
#define HEAD_CMD_ENDED              2

//--------------------------------------------------------------------------------------------------
/**
 * Enum for HTTP client state machine
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    STATE_IDLE,            ///< State machine is in idle state
    STATE_REQ_LINE,        ///< Build and send HTTP request line
    STATE_REQ_CREDENTIAL,  ///< Append optional HTTP connection credential
    STATE_REQ_RESOURCE,    ///< Append optional user-defined resources (key/value pairs)
    STATE_REQ_BODY,        ///< Append optional user-defined body to HTTP request
    STATE_RESP_PARSE,      ///< Parse remote server response
    STATE_END              ///< Notify end of HTTP request transaction
}
HttpSessionState_t;

//--------------------------------------------------------------------------------------------------
/**
 * Structure that defines TinyHTTP context
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    bool                     isInit;    ///< True if current context has been initialized.
    struct http_roundtripper handler;   ///< TinyHTTP handler
}
TinyHttpCtx_t;

//--------------------------------------------------------------------------------------------------
/**
 * Structure that defines HTTP session context
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_httpClient_Ref_t reference;                 ///< Safe reference to this object
    le_socket_Ref_t     socketRef;                 ///< Safe reference to the socket
    char                host[HOST_ADDR_LEN];       ///< Host address: dot-separated numeric (0-255)
                                                   ///< or explicit name of the remote server
    uint16_t            port;                      ///< HTTP server port numeric number (0-65535)
    bool                isSecure;                  ///< True if the session is secure
    char                credential[CRED_MAX_LEN];  ///< "Login:Password to be used during connection
    le_httpCommand_t    command;                   ///< Command of current HTTP request
    le_result_t         result;                    ///< Result of current HTTP request
    HttpSessionState_t  state;                     ///< HTTP client current state
    TinyHttpCtx_t       tinyHttpCtx;               ///< TinyHTTP handler
    le_timer_Ref_t      timerRef;                  ///< Timer reference used as a timeout when
                                                   ///< receiving HTTP data from remote server
    le_httpClient_SendRequestRspCb_t responseCb;        ///< Asynchronous request result callback
    le_httpClient_BodyResponseCb_t   bodyResponseCb;    ///< User-defined callback: Body response
    le_httpClient_HeaderResponseCb_t headerResponseCb;  ///< User-defined callback: Header response
    le_httpClient_StatusCodeCb_t     statusCodeCb;      ///< User-defined callback: Status code
    le_httpClient_ResourceUpdateCb_t resourceUpdateCb;  ///< User-defined callback: Resources update
    le_httpClient_BodyConstructCb_t  bodyConstructCb;   ///< User-defined callback: Body construct
    le_httpClient_EventCb_t          eventCb;           ///< User-defined callback: Session events
}
HttpSessionCtx_t;

//--------------------------------------------------------------------------------------------------
/**
 * Enum for HTTP command
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    PREFIX_HTTP,      ///< Enum for HTTP prefix
    PREFIX_HTTPS,     ///< Enum for HTTPS prefix
    PREFIX_MAX        ///< Enum for prefix max value
}
HttpPrefix_t;

//--------------------------------------------------------------------------------------------------
/**
 * Syntax of HTTP URL prefix.
 * If host address contains one of these prefixes, it needs to be removed to obtain the raw domain
 * name.
 */
//--------------------------------------------------------------------------------------------------
const char* SyntaxHttpPrefixPtr[] =
{
   [PREFIX_HTTP]  = "http://",
   [PREFIX_HTTPS] = "https://"
};

//--------------------------------------------------------------------------------------------------
/**
 *  Syntax of HTTP command as defined in the HTTP standard
 */
//--------------------------------------------------------------------------------------------------
const char* SyntaxHttpCommandPtr[] =
{
  [HTTP_HEAD]     = "HEAD",
  [HTTP_GET]      = "GET",
  [HTTP_POST]     = "POST",
  [HTTP_PUT]      = "PUT",
  [HTTP_DELETE]   = "DELETE"
};

//--------------------------------------------------------------------------------------------------
// Internal variables
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Memory pool for HTTP sessions.
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(HttpContextPool, HTTP_SESSIONS_NB, sizeof(HttpSessionCtx_t));

//--------------------------------------------------------------------------------------------------
/**
 * Static memory pool for tinyHTTP component
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(MemPool, MEM_MAX_COUNT, MEM_MAX);

//--------------------------------------------------------------------------------------------------
/**
 * Memory pool reference for tinyHTTP component pool
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t MemPoolRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Memory pool reference for the HTTP sessions pool
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t HttpSessionPoolRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for the HTTP sessions pool.
 */
//--------------------------------------------------------------------------------------------------
 static le_ref_MapRef_t HttpSessionRefMap;

//--------------------------------------------------------------------------------------------------
// Internal functions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pick an unused HTTP session context from the HTTP sessions pool and return it.
 *
 * @return Socket descriptor
 */
//--------------------------------------------------------------------------------------------------
static HttpSessionCtx_t* NewHttpSessionContext
(
    void
)
{
    HttpSessionCtx_t* contextPtr = NULL;

    // Initialize the socket pool and the socket reference map if not yet done
    if (!HttpSessionPoolRef)
    {
        HttpSessionPoolRef = le_mem_InitStaticPool(HttpContextPool,
                                                   HTTP_SESSIONS_NB,
                                                   sizeof(HttpSessionCtx_t));
        HttpSessionRefMap = le_ref_CreateMap("le_httpClientMap", HTTP_SESSIONS_NB);
    }

    // Alloc memory from pool
    contextPtr = le_mem_TryAlloc(HttpSessionPoolRef);

    if (NULL == contextPtr)
    {
        LE_ERROR("Unable to allocate a HTTP session context from pool");
        return NULL;
    }

    // Zero-init HTTP session context
    memset(contextPtr, 0, sizeof(HttpSessionCtx_t));

    // Create a safe reference for this object
    contextPtr->reference = le_ref_CreateRef(HttpSessionRefMap, contextPtr);

    return contextPtr;
}

//--------------------------------------------------------------------------------------------------
/**
 * Free a HTTP session context and make it available for future use.
 *
 * @return Socket descriptor
 */
//--------------------------------------------------------------------------------------------------
static void FreeHttpSessionContext
(
    HttpSessionCtx_t*    contextPtr    ///< [IN] Socket context pointer
)
{
    le_ref_DeleteRef(HttpSessionRefMap, contextPtr->reference);

    memset(contextPtr, 0, sizeof(HttpSessionCtx_t));
    le_mem_Release(contextPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * tinyHTTP callback for realloc
 */
//--------------------------------------------------------------------------------------------------
static void* TinyHttpReallocCb
(
    void*   opaquePtr,      ///< [IN] User data context
    void*   dataPtr,        ///< [IN] Data pointer
    int     size            ///< [IN] New size allocation
)
{
    (void)opaquePtr;

    LE_INFO("Request to allocate: %d in %p", size, dataPtr);

    if (!MemPoolRef)
    {
        MemPoolRef = le_mem_InitStaticPool(MemPool, MEM_MAX_COUNT, MEM_MAX);
    }

    if (size > MEM_MAX)
    {
        LE_WARN("Requested size (%d) higher than pool elements size (%d)", size, MEM_MAX);
        return NULL;
    }

    if (!dataPtr)
    {
        return le_mem_TryAlloc(MemPoolRef);
    }
    else if (size == 0)
    {
        le_mem_Release(dataPtr);
        return NULL;
    }

    return dataPtr;
}

//--------------------------------------------------------------------------------------------------
/**
 * tinyHTTP callback for received data in HTTP body response
 */
//--------------------------------------------------------------------------------------------------
static void TinyHttpBodyRspCb
(
    void*       opaquePtr,  ///< [IN] User data context
    const char* dataPtr,    ///< [IN] HTTP body data
    int         size        ///< [IN] Data length
)
{
    HttpSessionCtx_t *contextPtr = (HttpSessionCtx_t *)le_ref_Lookup(HttpSessionRefMap, opaquePtr);
    if (contextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", opaquePtr);
        return;
    }

    if (contextPtr->bodyResponseCb)
    {
        contextPtr->bodyResponseCb(opaquePtr, dataPtr, size);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * tinyHTTP callback for received data in HTTP header response
 */
//--------------------------------------------------------------------------------------------------
static void TinyHttpHeaderRspCb
(
    void*       opaquePtr,  ///< [IN] User data context
    const char* keyPtr,     ///< [IN] Key field in the HTTP header response
    int         nkey,       ///< [IN] Key field length
    const char* valuePtr,   ///< [IN] Key value in the HTTP header response
    int         nvalue      ///< [IN] Key value length
)
{
    HttpSessionCtx_t *contextPtr = (HttpSessionCtx_t *)le_ref_Lookup(HttpSessionRefMap, opaquePtr);
    if (contextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", opaquePtr);
        return;
    }

    if (contextPtr->headerResponseCb)
    {
        contextPtr->headerResponseCb(opaquePtr, keyPtr, nkey, valuePtr, nvalue);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * tinyHTTP callback for received HTTP error code
 */
//--------------------------------------------------------------------------------------------------
static void TinyHttpErrorCodeCb
(
    void*   opaquePtr,      ///< [IN] User data context
    int     code            ///< [IN] HTTP error code
)
{
    HttpSessionCtx_t *contextPtr = (HttpSessionCtx_t *)le_ref_Lookup(HttpSessionRefMap, opaquePtr);
    if (contextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", opaquePtr);
        return;
    }

    if (contextPtr->statusCodeCb)
    {
        contextPtr->statusCodeCb(opaquePtr, code);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Timer handler: On expiry, this function stops the current HTTP request and returns a timeout
 */
//--------------------------------------------------------------------------------------------------
static void TimeoutHandler
(
    le_timer_Ref_t timerRef    ///< [IN] Expired timer reference
)
{
    HttpSessionCtx_t *contextPtr = le_timer_GetContextPtr(timerRef);
    if (contextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", timerRef);
        return;
    }

    // This timer is only relevant when waiting for data from remote server
    if (contextPtr->state != STATE_RESP_PARSE)
    {
        return;
    }

    LE_INFO("Timeout when waiting for data from remote server");
    le_httpClient_Stop(contextPtr->reference);

    // Since state machine is stopped ungracefully, clean tinyHTTP context correctly
    TinyHttpCtx_t* tinyCtxPtr = &(contextPtr->tinyHttpCtx);
    if ((tinyCtxPtr) && (tinyCtxPtr->isInit))
    {
       http_free(&tinyCtxPtr->handler);
       tinyCtxPtr->isInit = false;
    }

    contextPtr->state = STATE_IDLE;
    contextPtr->result = LE_TIMEOUT;

    if (contextPtr->responseCb)
    {
        contextPtr->responseCb(contextPtr->reference, contextPtr->result);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Build HTTP request-line along with mandatory HTTP header resources and send it through socket.
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_FAULT         Internal error
 */
//--------------------------------------------------------------------------------------------------
static le_result_t BuildAndSendRequest
(
    HttpSessionCtx_t*    contextPtr,   ///< [IN] HTTP session context pointer
    le_httpCommand_t     command,      ///< [IN] HTTP command request
    char*                uriPtr        ///< [IN] URI buffer pointer
)
{
    char buffer[REQUEST_BUFFER_SIZE] = {0};
    int length = 0;
    char* reqUriPtr = "";

    if (uriPtr)
    {
        // Remove extra '/' that may be present in the URI
        if (*uriPtr == '/')
        {
            reqUriPtr = uriPtr+1;
        }
        else
        {
            reqUriPtr = uriPtr;
        }
    }

    // Construct request line following user input
    length = snprintf(buffer, sizeof(buffer), "%s /%s HTTP/1.1\r\n"
                                              "host: %s\r\n",
                                               SyntaxHttpCommandPtr[command],
                                               reqUriPtr,
                                               contextPtr->host);
    if ((length < 0) || (length >= sizeof(buffer)))
    {
        LE_ERROR("Unable to construct request line");
        return LE_FAULT;
    }

    // Save HTTP command request for later use
    contextPtr->command = command;

    // Send request through socket
    if (LE_OK != le_socket_Send(contextPtr->socketRef, buffer, length))
    {
        LE_ERROR("Unable to transmit request");
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Build credential header field and send it through socket.
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_UNAVAILABLE   Credential not available
 *  - LE_FAULT         Internal error
 */
//--------------------------------------------------------------------------------------------------
static le_result_t BuildAndSendCredential
(
    HttpSessionCtx_t*    contextPtr   ///< [IN] HTTP session context pointer
)
{
    char buffer[REQUEST_BUFFER_SIZE] = "Authorization: Basic ";
    int length = strlen(buffer);

    if (!(contextPtr->credential[0]))
    {
        return LE_UNAVAILABLE;
    }

    // Convert credential to BASE64 representation and append it to buffer
    uint8_t* encodedCredPtr = (uint8_t*)&buffer[length];
    size_t encodedCredLen = sizeof(buffer) - length - sizeof(CRLF);

    if (LE_OK != le_base64_Encode((uint8_t*)contextPtr->credential,
                                   strlen(contextPtr->credential),
                                   (char*)encodedCredPtr,
                                   &encodedCredLen))
    {
        LE_ERROR("Unable to encode credential");
        return LE_FAULT;
    }
    length += encodedCredLen;

    // Add final CRLF to the request
    if ((length + sizeof(CRLF)) > sizeof(buffer))
    {
        LE_ERROR("Unable to append CRLF");
        return LE_FAULT;
    }
    le_utf8_Copy(buffer+length, CRLF, sizeof(buffer)-length, NULL);
    length += strlen(CRLF);

    // Send request through socket
    if (LE_OK != le_socket_Send(contextPtr->socketRef, buffer, length))
    {
        LE_ERROR("Unable to transmit request");
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Retrieve user-defined HTTP header field (key/Value pair) and send it through socket.
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_TERMINATED    End of user resources injection
 *  - LE_FAULT         Internal error
 */
//--------------------------------------------------------------------------------------------------
static le_result_t BuildAndSendResource
(
    HttpSessionCtx_t*    contextPtr   ///< [IN] HTTP session context pointer
)
{
    char buffer[REQUEST_BUFFER_SIZE] = {0};
    int length = 0;
    int reservedBytes = 6; // Bytes reserved for semantics (e.g: \r, \n, :, space)
    le_result_t status;

    if (!contextPtr->resourceUpdateCb)
    {
        status = LE_TERMINATED;
        goto end;
    }

    char keyBuf[REQUEST_BUFFER_SIZE/2];
    char valueBuf[REQUEST_BUFFER_SIZE/2];
    int keyLen = sizeof(keyBuf) - reservedBytes;
    int valueLen = sizeof(valueBuf);

    status = contextPtr->resourceUpdateCb(contextPtr->reference,
                                          keyBuf, &keyLen, valueBuf, &valueLen);

    // Suspend resource injection when requested by user
    if (status == LE_WOULD_BLOCK)
    {
        if (le_httpClient_IsAsyncMode(contextPtr->reference))
        {
            return status;
        }

        LE_WARN("LE_WOULD_BLOCK is irrelevant in synchronous HTTP request");
        status = LE_OK;
    }

    // If one of returned buffer lengths values is zero, then end processing
    if ((!keyLen) || (!valueLen))
    {
        status = LE_TERMINATED;
        goto end;
    }

    // Copy key/value pair in the request buffer
    length = snprintf(buffer, sizeof(buffer), "%.*s: %.*s\r\n", keyLen, keyBuf, valueLen, valueBuf);

    if ((length < 0) || (length >= sizeof(buffer)))
    {
        LE_ERROR("Unable to construct header field");
        return LE_FAULT;
    }

end:
    // Append a final CRLF if needed
    if (status == LE_TERMINATED)
    {
        if ((length + sizeof(CRLF)) > sizeof(buffer))
        {
            LE_ERROR("Unable to append CRLF");
            return LE_FAULT;
        }

        le_utf8_Copy(buffer+length, CRLF, sizeof(buffer)-length, NULL);
        length += strlen(CRLF);
    }

    // Send request through socket
    if (LE_OK != le_socket_Send(contextPtr->socketRef, buffer, length))
    {
        LE_ERROR("Unable to transmit request");
        return LE_FAULT;
    }

    return status;
}

//--------------------------------------------------------------------------------------------------
/**
 * Retrieve user-defined HTTP body chunk and send it through socket.
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_UNAVAILABLE   Nothing to send
 *  - LE_TERMINATED    End of user body injection
 *  - LE_FAULT         Internal error
 */
//--------------------------------------------------------------------------------------------------
static le_result_t BuildAndSendBody
(
    HttpSessionCtx_t*    contextPtr   ///< [IN] HTTP session context pointer
)
{
    char buffer[REQUEST_BUFFER_SIZE] = {0};
    int length = sizeof(buffer);
    le_result_t status;

    if (!contextPtr->bodyConstructCb)
    {
        return LE_UNAVAILABLE;
    }

    status = contextPtr->bodyConstructCb(contextPtr->reference, buffer, &length);

    // Suspend resource injection when requested by user
    if (status == LE_WOULD_BLOCK)
    {
        if (le_httpClient_IsAsyncMode(contextPtr->reference))
        {
            return status;
        }

        LE_WARN("LE_WOULD_BLOCK is irrelevant in synchronous HTTP request");
        status = LE_OK;
    }

    // If buffer length value is zero, end processing
    if (!length)
    {
        return LE_UNAVAILABLE;
    }

    // Send body chunk through socket
    if (LE_OK != le_socket_Send(contextPtr->socketRef, buffer, length))
    {
        LE_ERROR("Unable to transmit request");
        return LE_FAULT;
    }

    return status;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read and parse remote server response.
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_TERMINATED    End of response parsing
 *  - LE_FAULT         Internal error
 */
//--------------------------------------------------------------------------------------------------
static le_result_t HandleHttpResponse
(
    HttpSessionCtx_t*    contextPtr   ///< [IN] HTTP session context pointer
)
{
    TinyHttpCtx_t* tinyCtxPtr = &(contextPtr->tinyHttpCtx);
    char buffer[RESPONSE_BUFFER_SIZE] = {0};
    const char* data = buffer;
    size_t length = sizeof(buffer);
    le_result_t status;
    int needmore = 1;

    if (tinyCtxPtr->isInit == false)
    {
        struct http_funcs responseFuncs =
        {
            TinyHttpReallocCb,
            TinyHttpBodyRspCb,
            TinyHttpHeaderRspCb,
            TinyHttpErrorCodeCb
        };

        http_init(&tinyCtxPtr->handler, responseFuncs, contextPtr->reference);
        tinyCtxPtr->isInit = true;
    }

    status = le_socket_Read(contextPtr->socketRef, buffer, &length);
    if (status != LE_OK)
    {
        if (status == LE_WOULD_BLOCK)
        {
            LE_INFO("Socket would block");
            return LE_OK;
        }

        LE_ERROR("Error receiving data");
        goto end;
    }

    if (!length)
    {
        LE_ERROR("No data received");
        status = LE_FAULT;
        goto end;
    }

    while (needmore && length)
    {
        int read;
        needmore = http_data(&tinyCtxPtr->handler, data, (int)length, &read);
        // If data read equals HEAD size but body is expected due to
        // HTTP_GET command, then continue to read remaining data.
        if (HEAD_CMD_ENDED == needmore)
        {
            if (HTTP_GET == contextPtr->command)
            {
                LE_DEBUG("HTTP_HEAD response received, continue reading data");
                break;
            }
            else
            {
                needmore = 0;
            }
        }

        length -= read;
        data += read;
    }

    // Need to read more data from socket
    if (needmore)
    {
        return LE_OK;
    }

    // Check for HTTP parsing result
    if (http_iserror(&tinyCtxPtr->handler))
    {
        LE_ERROR("Error parsing data");
        status = LE_FAULT;
        goto end;
    }

    // At this point, HTTP response has been totally read and processed correctly
    status = LE_TERMINATED;

end:
    http_free(&tinyCtxPtr->handler);
    tinyCtxPtr->isInit = false;
    return status;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function implements HTTP client state machine.
 *
 * @note
 * - For asynchronous requests, this function is called by socket monitoring when data is available.
 * - For synchronous requests, this function is looped inside the API
 */
//--------------------------------------------------------------------------------------------------
static void HttpClientStateMachine
(
    le_socket_Ref_t  socketRef,  ///< [IN] Socket context reference
    short            events,     ///< [IN] Bitmap of events that occurred
    void*            userPtr     ///< [IN] User data pointer
)
{
    le_result_t status;
    bool restartLoop;

    HttpSessionCtx_t *contextPtr = (HttpSessionCtx_t *)le_ref_Lookup(HttpSessionRefMap, userPtr);
    if (contextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", userPtr);
        return;
    }

    // Check if remote server closed connection
    if (events & POLLRDHUP)
    {
        LE_INFO("Connection closed by remote server");

        le_socket_Disconnect(socketRef);

        if (contextPtr->eventCb)
        {
            contextPtr->eventCb(contextPtr->reference, LE_HTTP_CLIENT_EVENT_CLOSED);
        }

        if (contextPtr->state != STATE_IDLE)
        {
            contextPtr->result = LE_FAULT;
            contextPtr->state = STATE_END;
        }
    }

    // Transitions are normally based on socket events but in some cases, a transition
    // needs to be executed immediately. This is why state machine execution is encapsulated in
    // a while loop
    do
    {
        restartLoop = false;

        switch (contextPtr->state)
        {
            case STATE_REQ_CREDENTIAL:

                if (!(events & POLLOUT))
                {
                    break;
                }

                status = BuildAndSendCredential(contextPtr);
                switch (status)
                {
                    case LE_OK:
                        contextPtr->state = STATE_REQ_RESOURCE;
                        break;

                    case LE_UNAVAILABLE:
                        contextPtr->state = STATE_REQ_RESOURCE;
                        restartLoop = true;
                        break;

                    default:
                        contextPtr->state = STATE_END;
                        contextPtr->result = status;
                        restartLoop = true;
                        break;
                }
                break;

            case STATE_REQ_RESOURCE:

                if (!(events & POLLOUT))
                {
                    break;
                }

                status = BuildAndSendResource(contextPtr);
                switch (status)
                {
                    case LE_OK:
                    case LE_WOULD_BLOCK:
                        contextPtr->state = STATE_REQ_RESOURCE;
                        break;

                    case LE_TERMINATED:
                        if ((contextPtr->command == HTTP_POST) ||
                            (contextPtr->command == HTTP_PUT))
                        {
                            contextPtr->state = STATE_REQ_BODY;
                        }
                        else
                        {
                            contextPtr->state = STATE_RESP_PARSE;
                        }
                        break;

                    default:
                        contextPtr->state = STATE_END;
                        contextPtr->result = status;
                        restartLoop = true;
                        break;
                }
                break;

            case STATE_REQ_BODY:

                if (!(events & POLLOUT))
                {
                    break;
                }

                status = BuildAndSendBody(contextPtr);
                switch (status)
                {
                    case LE_OK:
                    case LE_WOULD_BLOCK:
                        contextPtr->state = STATE_REQ_BODY;
                        break;

                    case LE_UNAVAILABLE:
                        contextPtr->state = STATE_RESP_PARSE;
                        restartLoop = true;
                        break;

                    case LE_TERMINATED:
                        contextPtr->state = STATE_RESP_PARSE;
                        break;

                    default:
                        contextPtr->state = STATE_END;
                        contextPtr->result = status;
                        restartLoop = true;
                        break;
                }
                break;

            case STATE_RESP_PARSE:

                if (!(events & POLLIN))
                {
                    break;
                }

                status = HandleHttpResponse(contextPtr);
                switch (status)
                {
                    case LE_OK:
                        contextPtr->state = STATE_RESP_PARSE;
                        break;

                    case LE_TERMINATED:
                        contextPtr->state = STATE_END;
                        contextPtr->result = LE_OK;
                        restartLoop = true;
                        break;

                    default:
                        contextPtr->state = STATE_END;
                        contextPtr->result = status;
                        restartLoop = true;
                        break;
                }
                break;

            case STATE_END:

                contextPtr->state = STATE_IDLE;

                if (contextPtr->timerRef)
                {
                    le_timer_Stop(contextPtr->timerRef);
                }

                if (contextPtr->responseCb)
                {
                    contextPtr->responseCb(contextPtr->reference, contextPtr->result);
                }
                break;

            default:
                if (events & POLLIN)
                {
                    char tmp;
                    size_t len = sizeof(tmp);

                    // There are two cases where flushing data from socket is needed:
                    // - HTTPs server sends a last message before closing the socket and this data
                    //   needs to be consumed by SSL layer before receiving POLLRDHUP event
                    // - Network drop tears down the connection resulting in POLLIN event reported
                    //   continuously
                    status = le_socket_Read(contextPtr->socketRef, &tmp, &len);
                    if ((!len) ||  (status != LE_OK))
                    {
                        LE_INFO("Connection teared down");

                        le_httpClient_Stop(contextPtr->reference);
                        if (contextPtr->eventCb)
                        {
                            contextPtr->eventCb(contextPtr->reference,
                                                LE_HTTP_CLIENT_EVENT_CLOSED);
                        }
                    }
                }
                break;
        }
    }
    while (restartLoop);

    // When parse state is reached, device waits for data from remote server. In this case,
    // a timeout is enabled according to the user defined value
    if ((contextPtr->state == STATE_RESP_PARSE) && (contextPtr->timerRef))
    {
        le_timer_Restart(contextPtr->timerRef);
    }
}

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
le_httpClient_Ref_t le_httpClient_Create
(
    char*            hostPtr,     ///< [IN] HTTP server address
    uint16_t         port         ///< [IN] HTTP server port numeric number (0-65535)
)
{
    HttpSessionCtx_t* contextPtr = NULL;
    int i = 0, offset = 0;

    // Check input parameters
    if (NULL == hostPtr)
    {
        LE_ERROR("Unspecified host address");
        return NULL;
    }

    // Allocate a HTTP session context and save server parameters
    contextPtr = NewHttpSessionContext();
    if (NULL == contextPtr)
    {
        LE_ERROR("Unable to allocate a HTTP session context from pool");
        return NULL;
    }

    // Remove any prefix before storing the hostname
    for (i = 0; i < PREFIX_MAX; i++ )
    {
        if (0 == strncmp(hostPtr, SyntaxHttpPrefixPtr[i], strlen(SyntaxHttpPrefixPtr[i])))
        {
            offset = strlen(SyntaxHttpPrefixPtr[i]);
        }
    }

    strncpy(contextPtr->host, hostPtr + offset, sizeof(contextPtr->host)-1);
    contextPtr->port = port;

    // Connect the socket
    contextPtr->socketRef = le_socket_Create(contextPtr->host, contextPtr->port, TCP_TYPE);
    if (NULL == contextPtr->socketRef)
    {
        LE_ERROR("Failed to connect socket");
        FreeHttpSessionContext(contextPtr);
        return NULL;
    }

    // Create a timeout timer for the current context
    contextPtr->timerRef = le_timer_Create("Timeout");
    if (NULL == contextPtr->timerRef)
    {
        LE_ERROR("Failed to create timer");
        FreeHttpSessionContext(contextPtr);
        return NULL;
    }

    le_timer_SetRepeat(contextPtr->timerRef, 1);
    le_timer_SetContextPtr(contextPtr->timerRef, contextPtr);
    le_timer_SetHandler(contextPtr->timerRef, TimeoutHandler);
    le_timer_SetMsInterval(contextPtr->timerRef, COMM_TIMEOUT_DEFAULT_MS);

    LE_INFO("Allocated: %p, referenced by: %p", contextPtr, contextPtr->reference);
    return contextPtr->reference;
}

//--------------------------------------------------------------------------------------------------
/**
 * Delete a previously created HTTP socket and free allocated resources.
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_FAULT         Internal error
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_httpClient_Delete
(
    le_httpClient_Ref_t    ref  ///< [IN] HTTP session context reference
)
{
    HttpSessionCtx_t *contextPtr = (HttpSessionCtx_t *)le_ref_Lookup(HttpSessionRefMap, ref);
    if (contextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", ref);
        return LE_BAD_PARAMETER;
    }

    TinyHttpCtx_t* tinyCtxPtr = &(contextPtr->tinyHttpCtx);
    if ((tinyCtxPtr) && (tinyCtxPtr->isInit))
    {
       http_free(&tinyCtxPtr->handler);
       tinyCtxPtr->isInit = false;
    }

    le_socket_Delete(contextPtr->socketRef);
    le_timer_Delete(contextPtr->timerRef);

    FreeHttpSessionContext(contextPtr);
    return LE_OK;
}

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
le_result_t le_httpClient_SetTimeout
(
    le_httpClient_Ref_t    ref,       ///< [IN] HTTP session context reference
    uint32_t               timeout    ///< [IN] Timeout in milliseconds
)
{
    HttpSessionCtx_t *contextPtr = (HttpSessionCtx_t *)le_ref_Lookup(HttpSessionRefMap, ref);
    if (contextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", ref);
        return LE_BAD_PARAMETER;
    }

    if (contextPtr->timerRef)
    {
        le_timer_SetMsInterval(contextPtr->timerRef, timeout);
    }

    return le_socket_SetTimeout(contextPtr->socketRef, timeout);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set user credentials to the HTTP session.
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_httpClient_SetCredentials
(
    le_httpClient_Ref_t    ref,         ///< [IN] HTTP session context reference
    char*                  loginPtr,    ///< [IN] User name to be used during the HTTP connection
    char*                  passwordPtr  ///< [IN] Password to be used during the HTTP connection
)
{
    int offset = 0;
    HttpSessionCtx_t *contextPtr = (HttpSessionCtx_t *)le_ref_Lookup(HttpSessionRefMap, ref);
    if (contextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", ref);
        return LE_BAD_PARAMETER;
    }

    if ((!loginPtr) || (!passwordPtr))
    {
        LE_ERROR("Wrong parameter: %p, %p", loginPtr, passwordPtr);
        return LE_BAD_PARAMETER;
    }

    offset = snprintf(contextPtr->credential, sizeof(contextPtr->credential), "%s:%s",
                                                                              loginPtr,
                                                                              passwordPtr);
    if ((offset < 0) || (offset >= sizeof(contextPtr->credential)))
    {
        LE_ERROR("Credential size exceeds maximum allowed");
        return LE_FAULT;
    }

    return LE_OK;
}

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
le_result_t le_httpClient_AddCertificate
(
    le_httpClient_Ref_t  ref,             ///< [IN] HTTP session context reference
    const uint8_t*       certificatePtr,  ///< [IN] Certificate Pointer
    size_t               certificateLen   ///< [IN] Certificate Length
)
{
    le_result_t status;
    HttpSessionCtx_t *contextPtr = (HttpSessionCtx_t *)le_ref_Lookup(HttpSessionRefMap, ref);
    if (contextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", ref);
        return LE_BAD_PARAMETER;
    }

    status = le_socket_AddCertificate(contextPtr->socketRef, certificatePtr, certificateLen);
    if (status == LE_OK)
    {
        contextPtr->isSecure = true;
    }
    else
    {
        contextPtr->isSecure = false;
    }

    return status;
}

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
le_result_t le_httpClient_Start
(
    le_httpClient_Ref_t  ref    ///< [IN] HTTP session context reference
)
{
    HttpSessionCtx_t *contextPtr = (HttpSessionCtx_t *)le_ref_Lookup(HttpSessionRefMap, ref);
    if (contextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", ref);
        return LE_BAD_PARAMETER;
    }

    return le_socket_Connect(contextPtr->socketRef);
}

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
le_result_t le_httpClient_Stop
(
    le_httpClient_Ref_t  ref     ///< [IN] HTTP session context reference
)
{
    HttpSessionCtx_t *contextPtr = (HttpSessionCtx_t *)le_ref_Lookup(HttpSessionRefMap, ref);
    if (contextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", ref);
        return LE_BAD_PARAMETER;
    }

    contextPtr->state = STATE_IDLE;
    return le_socket_Disconnect(contextPtr->socketRef);
}

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
le_result_t le_httpClient_SendRequest
(
    le_httpClient_Ref_t  ref,              ///< [IN] HTTP session context reference
    le_httpCommand_t     command,          ///< [IN] HTTP command request
    char*                requestUriPtr     ///< [IN] URI buffer pointer
)
{
    le_result_t status;
    HttpSessionCtx_t *contextPtr = (HttpSessionCtx_t *)le_ref_Lookup(HttpSessionRefMap, ref);
    if (contextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", ref);
        return LE_BAD_PARAMETER;
    }

    if (command >= HTTP_MAX)
    {
        LE_ERROR("Unrecognized HTTP command: %d", command);
        return LE_BAD_PARAMETER;
    }

    if (contextPtr->state != STATE_IDLE)
    {
        LE_ERROR("Busy handling previous request. Current state: %d", contextPtr->state);
        return LE_BUSY;
    }

    status = BuildAndSendRequest(contextPtr, command, requestUriPtr);
    if (LE_OK != status)
    {
        LE_ERROR("Unable to build request line");
        return status;
    }

    // Loop HTTP client state machine until the request is executed and response parsed.
    contextPtr->state = STATE_REQ_CREDENTIAL;
    do
    {
        HttpClientStateMachine(contextPtr->socketRef, POLLIN | POLLOUT, ref);
    }
    while (contextPtr->state != STATE_IDLE);

    return contextPtr->result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set a callback to handle HTTP response body data.
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_FAULT         Internal error
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_httpClient_SetBodyResponseCallback
(
    le_httpClient_Ref_t              ref,      ///< [IN] HTTP session context reference
    le_httpClient_BodyResponseCb_t   callback  ///< [IN] Callback
)
{
    HttpSessionCtx_t *contextPtr = (HttpSessionCtx_t *)le_ref_Lookup(HttpSessionRefMap, ref);
    if (contextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", ref);
        return LE_BAD_PARAMETER;
    }

    contextPtr->bodyResponseCb = callback;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set a callback to handle HTTP header key/value pair.
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_FAULT         Internal error
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_httpClient_SetHeaderResponseCallback
(
    le_httpClient_Ref_t               ref,      ///< [IN] HTTP session context reference
    le_httpClient_HeaderResponseCb_t  callback  ///< [IN] Callback
)
{
    HttpSessionCtx_t *contextPtr = (HttpSessionCtx_t *)le_ref_Lookup(HttpSessionRefMap, ref);
    if (contextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", ref);
        return LE_BAD_PARAMETER;
    }

    contextPtr->headerResponseCb = callback;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set callback to handle HTTP status code.
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_FAULT         Internal error
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_httpClient_SetStatusCodeCallback
(
    le_httpClient_Ref_t             ref,      ///< [IN] HTTP session context reference
    le_httpClient_StatusCodeCb_t    callback  ///< [IN] Callback
)
{
    HttpSessionCtx_t *contextPtr = (HttpSessionCtx_t *)le_ref_Lookup(HttpSessionRefMap, ref);
    if (contextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", ref);
        return LE_BAD_PARAMETER;
    }

    contextPtr->statusCodeCb = callback;
    return LE_OK;
}

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
le_result_t le_httpClient_SetResourceUpdateCallback
(
    le_httpClient_Ref_t              ref,      ///< [IN] HTTP session context reference
    le_httpClient_ResourceUpdateCb_t callback  ///< [IN] Callback
)
{
    HttpSessionCtx_t *contextPtr = (HttpSessionCtx_t *)le_ref_Lookup(HttpSessionRefMap, ref);
    if (contextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", ref);
        return LE_BAD_PARAMETER;
    }

    contextPtr->resourceUpdateCb = callback;
    return LE_OK;
}

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
le_result_t le_httpClient_SetBodyConstructCallback
(
    le_httpClient_Ref_t              ref,       ///< [IN] HTTP session context reference
    le_httpClient_BodyConstructCb_t  callback   ///< [IN] Callback
)
{
    HttpSessionCtx_t *contextPtr = (HttpSessionCtx_t *)le_ref_Lookup(HttpSessionRefMap, ref);
    if (contextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", ref);
        return LE_BAD_PARAMETER;
    }

    contextPtr->bodyConstructCb = callback;
    return LE_OK;
}

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
)
{
    HttpSessionCtx_t *contextPtr = (HttpSessionCtx_t *)le_ref_Lookup(HttpSessionRefMap, ref);
    if (contextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", ref);
        return LE_BAD_PARAMETER;
    }

    contextPtr->eventCb = callback;
    return LE_OK;
}

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
le_result_t le_httpClient_SetAsyncMode
(
    le_httpClient_Ref_t     ref,      ///< [IN] HTTP session context reference
    bool                    enable    ///< [IN] True to activate asynchronous mode, false otherwise
)
{
    le_result_t status;
    HttpSessionCtx_t *contextPtr = (HttpSessionCtx_t *)le_ref_Lookup(HttpSessionRefMap, ref);
    if (contextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", ref);
        return LE_BAD_PARAMETER;
    }

    status = le_socket_AddEventHandler(contextPtr->socketRef, HttpClientStateMachine, ref);
    if (LE_OK != status)
    {
        LE_ERROR("Failed to add socket event handler");
        return status;
    }

    return le_socket_SetMonitoring(contextPtr->socketRef, enable);
}

//--------------------------------------------------------------------------------------------------
/**
 * Check whether the HTTP client mode is set to asynchronous.
 *
 * @return
 *  - True if the current mode is asynchronous, false otherwise.
 */
//--------------------------------------------------------------------------------------------------
bool le_httpClient_IsAsyncMode
(
    le_httpClient_Ref_t     ref       ///< [IN] HTTP session context reference
)
{
    HttpSessionCtx_t *contextPtr = (HttpSessionCtx_t *)le_ref_Lookup(HttpSessionRefMap, ref);
    if (contextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", ref);
        return false;
    }

    return le_socket_IsMonitoring(contextPtr->socketRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Send a HTTP command request to remote server. Response reception is handled in an asynchronous
 * way in the calling thread event loop.  This API is non-blocking.
 *
 * @note Function execution result can be retrieved through the provided callback
 */
//--------------------------------------------------------------------------------------------------
void le_httpClient_SendRequestAsync
(
    le_httpClient_Ref_t              ref,           ///< [IN] HTTP session context reference
    le_httpCommand_t                 command,       ///< [IN] HTTP command request
    char*                            requestUriPtr, ///< [IN] URI buffer pointer
    le_httpClient_SendRequestRspCb_t callback       ///< [IN] Function execution result callback
)
{
    le_result_t status = LE_OK;
    HttpSessionCtx_t *contextPtr = (HttpSessionCtx_t *)le_ref_Lookup(HttpSessionRefMap, ref);
    if (contextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", ref);
        return;
    }

    if (command >= HTTP_MAX)
    {
        LE_ERROR("Unrecognized HTTP command: %d", command);
        status = LE_BAD_PARAMETER;
        goto end;
    }

    if (contextPtr->state != STATE_IDLE)
    {
        LE_ERROR("Busy handling previous request. Current state: %d", contextPtr->state);
        status = LE_BUSY;
        goto end;
    }

    status = BuildAndSendRequest(contextPtr, command, requestUriPtr);
    if (LE_OK != status)
    {
        LE_ERROR("Unable to build request line");
        goto end;
    }

    // At this point, asynchronous state machine will continue the request handling
    contextPtr->responseCb = callback;
    contextPtr->state = STATE_REQ_CREDENTIAL;
    return;

end:
    if (callback)
    {
        callback(contextPtr->reference, status);
    }
    contextPtr->state = STATE_IDLE;
}

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
le_result_t le_httpClient_Resume
(
    le_httpClient_Ref_t     ref       ///< [IN] HTTP session context reference
)
{
    HttpSessionCtx_t *contextPtr = (HttpSessionCtx_t *)le_ref_Lookup(HttpSessionRefMap, ref);
    if (contextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", ref);
        return LE_BAD_PARAMETER;
    }

    if ((contextPtr->state != STATE_REQ_RESOURCE) && (contextPtr->state != STATE_REQ_BODY))
    {
        LE_ERROR("Wrong state. Resume not allowed: %d", contextPtr->state);
        return LE_FAULT;
    }

    return le_socket_TrigMonitoring(contextPtr->socketRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Component initialization function
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_DEBUG("httpClientLibrary initializing");
}
