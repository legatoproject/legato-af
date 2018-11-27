/**
 * @file httpTest.c
 *
 * This module implements a minimal integration test for the HTTP client component.
 * It allows to initiate an HTTP session and test APIs.
 *
 * Build:
 * Use the following command to compile this integration test for Linux targets when using mkapp:
 *   - CONFIG_SOCKET_LIB_USE_OPENSSL=y CONFIG_LINUX=y mkapp -t <target> httpTest.adef
 *
 * Usage:
 *   - app runProc httpTest httpTest -- security_flag host port uri
 *
 * Examples:
 *   - HTTP:    app runProc httpTest httpTest -- 0 www.google.fr 80 /
 *   - HTTPS:   app runProc httpTest httpTest -- 1 m2mop.net 443 /s
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"
#include "le_httpClientLib.h"
#include "defaultDerKey.h"

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Reception timeout in milliseconds
 */
//--------------------------------------------------------------------------------------------------
#define RX_TIMEOUT_MS         5000

//--------------------------------------------------------------------------------------------------
/**
 * Suspend duration of an asynchronous HTTP request in milliseconds
 */
//--------------------------------------------------------------------------------------------------
#define SUSPEND_DURATION_MS   500

//--------------------------------------------------------------------------------------------------
/**
 * Requests loop
 */
//--------------------------------------------------------------------------------------------------
#define REQUESTS_LOOP         5

//--------------------------------------------------------------------------------------------------
/**
 * Number of HTTP header request fields
 */
//--------------------------------------------------------------------------------------------------
#define SAMPLE_FIELDS_NB      4

//--------------------------------------------------------------------------------------------------
/**
 * URI max size
 */
//--------------------------------------------------------------------------------------------------
#define URI_SIZE              512

//--------------------------------------------------------------------------------------------------
/**
 * Asynchronous request data structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    int              index;          ///< Request index
    le_httpCommand_t cmd;            ///< HTTP command
    bool             isSuspended;    ///< True if current request has been suspended by user
    char             uri[URI_SIZE];  ///< URI buffer
}
AsyncRequest_t;

//--------------------------------------------------------------------------------------------------
// Internal variables
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Asynchronous HTTP request data
 */
//--------------------------------------------------------------------------------------------------
static AsyncRequest_t AsyncRequest;

//--------------------------------------------------------------------------------------------------
/**
 * Timer used to suspend and resume later an HTTP request
 */
//--------------------------------------------------------------------------------------------------
static le_timer_Ref_t DelayTimerRef;

//--------------------------------------------------------------------------------------------------
/**
 * Sample of HTTP header request fields (key/value pair)
 */
//--------------------------------------------------------------------------------------------------
const char* SampleHttpHeaderFields[2*SAMPLE_FIELDS_NB] =
{
    "accept",          "*/*",
    "cache-control",   "no-cache",
    "user-agent",      "Legato app",
    "accept-encoding", "gzip, deflate"
};

//--------------------------------------------------------------------------------------------------
// Functions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 *  Callback to handle HTTP body response.
 */
//--------------------------------------------------------------------------------------------------
static void BodyResponseCb
(
    le_httpClient_Ref_t ref,        ///< [IN] HTTP session context reference
    const char*         dataPtr,    ///< [IN] Received data pointer
    int                 size        ///< [IN] Received data size
)
{
    LE_INFO("Data size: %d", size);
}

//--------------------------------------------------------------------------------------------------
/**
 *  Callback to handle HTTP header response.
 */
//--------------------------------------------------------------------------------------------------
static void HeaderResponseCb
(
    le_httpClient_Ref_t ref,        ///< [IN] HTTP session context reference
    const char*         keyPtr,     ///< [IN] Key field in the HTTP header response
    int                 keyLen,     ///< [IN] Key field length
    const char*         valuePtr,   ///< [IN] Key value in the HTTP header response
    int                 valueLen    ///< [IN] Key value length
)
{
    LE_INFO("Key: %.*s, Value: %.*s", keyLen, keyPtr, valueLen, valuePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 *  Callback to handle HTTP status code
 */
//--------------------------------------------------------------------------------------------------
//! [HttpStatusCb]
static void StatusCodeCb
(
    le_httpClient_Ref_t ref,        ///< [IN] HTTP session context reference
    int                 code        ///< [IN] HTTP status code
)
{
    LE_INFO("HTTP status code: %d", code);
}
//! [HttpStatusCb]

//--------------------------------------------------------------------------------------------------
/**
 *  Callback to handle HTTP body construction.
 *
 * @return
 *  - LE_OK            Callback should be called again to gather another chunk of data
 *  - LE_TERMINATED    All data have been transmitted, do not recall callback
 *  - LE_FAULT         Internal error
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t BodyConstructCb
(
    le_httpClient_Ref_t ref,        ///< [IN] HTTP session context reference
    char*               dataPtr,    ///< [OUT] Data pointer
    int*                sizePtr     ///< [INOUT] Data pointer size
)
{
    *sizePtr = 0;
    return LE_TERMINATED;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Callback to handle resources (key/value pairs) insertion.
 *
 * @return
 *  - LE_OK            Callback should be called again to gather another key/value pair
 *  - LE_TERMINATED    All keys have been transmitted, do not recall callback
 *  - LE_FAULT         Internal error
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ResourceUpdateCb
(
    le_httpClient_Ref_t ref,           ///< [IN] HTTP session context reference
    char*               keyPtr,        ///< [OUT] Key field pointer
    int*                keyLenPtr,     ///< [INOUT] Key field size
    char*               valuePtr,      ///< [OUT] Key value pointer
    int*                valueLenPtr    ///< [INOUT] Key value size
)
{
    static int index = 0;
    char maxIndex = 2 * SAMPLE_FIELDS_NB - 1;

    if ((index + 1) > maxIndex)
    {
        LE_ERROR("Index out of range");
        *keyLenPtr = 0;
        *valueLenPtr = 0;
        return LE_FAULT;
    }

    if (DelayTimerRef)
    {
        AsyncRequest_t* requestPtr = &AsyncRequest;
        if (!requestPtr->isSuspended)
        {
            LE_INFO("Request a HTTP request suspend");

            requestPtr->isSuspended = true;
            le_timer_Restart(DelayTimerRef);
            return LE_WOULD_BLOCK;
        }
        else
        {
            requestPtr->isSuspended = false;
        }
    }

    const char* sampleKeyPtr = SampleHttpHeaderFields[index];
    const char* sampleValuePtr = SampleHttpHeaderFields[index+1];

    strncpy(keyPtr, sampleKeyPtr, *keyLenPtr);
    strncpy(valuePtr, sampleValuePtr, *valueLenPtr);

    *keyLenPtr = strlen(sampleKeyPtr);
    *valueLenPtr = strlen(sampleValuePtr);

    index+=2;

    if (index < maxIndex)
    {
        return LE_OK;
    }
    else
    {
        index = 0;
        LE_INFO("End of keys injection");
        return LE_TERMINATED;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 *  Callback definition for le_httpClient_SendRequestAsync result value.
 */
//--------------------------------------------------------------------------------------------------
static void SendRequestRspCb
(
    le_httpClient_Ref_t ref,          ///< [IN] HTTP session context reference
    le_result_t         result        ///< [IN] Result value
)
{
    AsyncRequest_t* requestPtr = &AsyncRequest;

    LE_INFO("Request %d final status: %d", requestPtr->index, result);

    if (requestPtr->index)
    {
        requestPtr->index--;
        le_httpClient_SendRequestAsync(ref, requestPtr->cmd, requestPtr->uri, SendRequestRspCb);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Timer handler: On expiry, this function attempts to resume the suspended HTTP requests
 */
//--------------------------------------------------------------------------------------------------
static void TimerHandler
(
    le_timer_Ref_t timerRef    ///< [IN] This timer has expired
)
{
    le_httpClient_Ref_t sessionRef = le_timer_GetContextPtr(timerRef);

    le_result_t status = le_httpClient_Resume(sessionRef);
    if (status != LE_OK)
    {
        LE_INFO("Unable to resume HTTP request");
        return;
    }

    LE_INFO("Resuming HTTP request");
}

//--------------------------------------------------------------------------------------------------
/**
 * Callback definition for asynchronous events.
*/
//--------------------------------------------------------------------------------------------------
static void EventCb
(
    le_httpClient_Ref_t   ref,          ///< [IN] HTTP session context reference
    le_httpClient_Event_t event         ///< [IN] Event that occured
)
{
    switch (event)
    {
        case LE_HTTP_CLIENT_EVENT_NONE:
            LE_INFO("Event: LE_HTTP_CLIENT_EVENT_NONE");
            break;

        case LE_HTTP_CLIENT_EVENT_CLOSED:
            LE_INFO("Event: LE_HTTP_CLIENT_EVENT_CLOSED");
            le_httpClient_Stop(ref);
            le_httpClient_Delete(ref);
            exit(EXIT_SUCCESS);
            break;

        default:
            LE_INFO("Event: %d", event);
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Component main function
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    le_result_t status;

    // Check arguments number
    if (le_arg_NumArgs() < 4)
    {
        LE_INFO("Usage: app runProc httpTest httpTest -- security_flag host port");
        exit(EXIT_FAILURE);
    }

    // Get and decode arguments
    bool securityFlag = (strtol(le_arg_GetArg(0), NULL, 10) == 1) ? true : false;
    char* hostPtr     = (char*)le_arg_GetArg(1);
    long portNumber   = strtol(le_arg_GetArg(2), NULL, 10);
    char* uriPtr      = (char*)le_arg_GetArg(3);

    // Check parameters validity
    if ((!hostPtr) || (!uriPtr))
    {
        LE_ERROR("Null parameter provided");
        exit(EXIT_FAILURE);
    }

    // Port number range is [1 .. 65535]
    if ((portNumber < 1) || (portNumber > USHRT_MAX))
    {
        LE_ERROR("Invalid port number. Accepted range: [1 .. %d]", USHRT_MAX);
        exit(EXIT_FAILURE);
    }

    LE_INFO("Creating a HTTP client...");
    //! [HttpConnect]
    le_httpClient_Ref_t sessionRef = le_httpClient_Create(hostPtr, (uint16_t)portNumber);
    if (sessionRef == NULL)
    {
        LE_ERROR("Unable to create HTTP client");
        exit(EXIT_FAILURE);
    }
    //! [HttpConnect]

    if (securityFlag)
    {
        LE_INFO("Adding default security certificate...");
        status = le_httpClient_AddCertificate(sessionRef, DefaultDerKey, DEFAULT_DER_KEY_LEN);
        if (LE_OK != status)
        {
            LE_ERROR("Failed to add certificate");
            goto end;
        }
    }

    LE_INFO("Adding callbacks...");
    status = le_httpClient_SetBodyResponseCallback(sessionRef, BodyResponseCb);
    if (LE_OK != status)
    {
        LE_ERROR("Unable to set callback");
        exit(EXIT_FAILURE);
    }

    status = le_httpClient_SetHeaderResponseCallback(sessionRef, HeaderResponseCb);
    if (LE_OK != status)
    {
        LE_ERROR("Unable to set callback");
        exit(EXIT_FAILURE);
    }

    //! [HttpSetCb]
    status = le_httpClient_SetStatusCodeCallback(sessionRef, StatusCodeCb);
    if (LE_OK != status)
    {
        LE_ERROR("Unable to set callback");
        exit(EXIT_FAILURE);
    }
    //! [HttpSetCb]

    status = le_httpClient_SetBodyConstructCallback(sessionRef, BodyConstructCb);
    if (LE_OK != status)
    {
        LE_ERROR("Unable to set callback");
        exit(EXIT_FAILURE);
    }

    status = le_httpClient_SetResourceUpdateCallback(sessionRef, ResourceUpdateCb);
    if (LE_OK != status)
    {
        LE_ERROR("Unable to set callback");
        exit(EXIT_FAILURE);
    }

    status = le_httpClient_SetEventCallback(sessionRef, EventCb);
    if (LE_OK != status)
    {
        LE_ERROR("Unable to set callback");
        exit(EXIT_FAILURE);
    }

    LE_INFO("Setting timeout to %d milliseconds...", RX_TIMEOUT_MS);
    status = le_httpClient_SetTimeout(sessionRef, RX_TIMEOUT_MS);
    if (LE_OK != status)
    {
        LE_ERROR("Unable to set timeout");
        goto end;
    }

    LE_INFO("Starting the HTTP session...");
    status = le_httpClient_Start(sessionRef);
    if (LE_OK != status)
    {
        LE_ERROR("Unable to start HTTP client");
        goto end;
    }

    LE_INFO("Sending synchronous HTTP requests %d times...", REQUESTS_LOOP);
    int i = REQUESTS_LOOP;

    while (i)
    {
        LE_INFO("Sending a HTTP HEAD command on URI...");
        status = le_httpClient_SendRequest(sessionRef, HTTP_HEAD, uriPtr);
        if (LE_OK != status)
        {
            LE_ERROR("Unable to send request");
            goto end;
        }

        LE_INFO("Sending a HTTP GET command on URI...");
        status = le_httpClient_SendRequest(sessionRef, HTTP_GET, uriPtr);
        if (LE_OK != status)
        {
            LE_ERROR("Unable to send request");
            goto end;
        }

        i--;
    }

    LE_INFO("Enable asynchronous mode");
    status = le_httpClient_SetAsyncMode(sessionRef, true);
    if (LE_OK != status)
    {
        LE_ERROR("Unable to set asynchronous mode");
        goto end;
    }

    DelayTimerRef = le_timer_Create("DelayTimer");
    if (!DelayTimerRef)
    {
        LE_ERROR("Unable to create a timer");
        goto end;
    }

    if ((LE_OK != le_timer_SetHandler(DelayTimerRef, TimerHandler))
     || (LE_OK != le_timer_SetContextPtr(DelayTimerRef, (void*)sessionRef))
     || (LE_OK != le_timer_SetMsInterval(DelayTimerRef, SUSPEND_DURATION_MS))
     || (LE_OK != le_timer_SetRepeat(DelayTimerRef, 1)))
    {
        LE_ERROR("Unable to configure timer");
        status = LE_FAULT;
        goto end;
    }

    LE_INFO("Sending asynchronous HTTP requests %d times...", REQUESTS_LOOP);

    AsyncRequest.index = REQUESTS_LOOP;
    AsyncRequest.cmd = HTTP_GET;
    AsyncRequest.isSuspended = false;
    strncpy(AsyncRequest.uri, uriPtr, sizeof(AsyncRequest.uri)-1);

    le_httpClient_SendRequestAsync(sessionRef, AsyncRequest.cmd, uriPtr, SendRequestRspCb);

end:
    if (status != LE_OK)
    {
        LE_INFO("Stopping and deleting the HTTP client...");
        le_httpClient_Stop(sessionRef);
        le_httpClient_Delete(sessionRef);
        exit(EXIT_FAILURE);
    }
}
