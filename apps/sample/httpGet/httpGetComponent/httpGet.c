//--------------------------------------------------------------------------------------------------
/** @file httpGet.c
 *
 * Demonstrates opening a data connection and libcurl usage
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "le_data_interface.h"
#include "le_timer.h"
#include <curl/curl.h>
#include <time.h>

#define TIMEOUT_SECS    30
#define SSL_ERROR_HELP    "Make sure your system date is set correctly (e.g. `date -s '2016-7-7'`)"
#define SSL_ERROR_HELP_2  "You can check the minimum date for this SSL cert to work using: `openssl s_client -connect httpbin.org:443 2>/dev/null | openssl x509 -noout -dates`"

static const char * Url = "https://httpbin.org/get";

static le_data_RequestObjRef_t ConnectionRef;
static bool WaitingForConnection;

//--------------------------------------------------------------------------------------------------
/**
 * Callback for printing the response of a succesful request
 */
//--------------------------------------------------------------------------------------------------
static size_t PrintCallback
(
    void *bufferPtr,
    size_t size,
    size_t nbMember,
    void *userData // unused, but can be set with CURLOPT_WRITEDATA
)
{
    printf("Succesfully received data:\n");
    fwrite(bufferPtr, size, nbMember, stdout);
    return size * nbMember;
}

//--------------------------------------------------------------------------------------------------
/**
 * Callback for the timeout timer
 */
//--------------------------------------------------------------------------------------------------

static void TimeoutHandler
(
    le_timer_Ref_t timerRef
)
{
    if (WaitingForConnection)
    {
        LE_ERROR("Couldn't establish connection after " STRINGIZE(TIMEOUT_SECS) " seconds");
        exit(EXIT_FAILURE);
    }
}


static void GetUrl
(
    void
)
{
    CURL *curl;
    CURLcode res;

    curl = curl_easy_init();
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, Url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, PrintCallback);
        res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            LE_ERROR("curl_easy_perform() failed: %s", curl_easy_strerror(res));
            if (res == CURLE_SSL_CACERT)
            {
                LE_ERROR(SSL_ERROR_HELP);
                LE_ERROR(SSL_ERROR_HELP_2);
            }
        }
        curl_easy_cleanup(curl);
    }
    else
    {
        LE_ERROR("Couldn't initialize cURL.");
    }

    curl_global_cleanup();
}

//--------------------------------------------------------------------------------------------------
/**
 * Callback for when the connection state changes
 */
//--------------------------------------------------------------------------------------------------

static void ConnectionStateHandler
(
    const char *intfName,
    bool isConnected,
    void *contextPtr
)
{
    if (isConnected)
    {
        WaitingForConnection = false;
        LE_INFO("Interface %s connected.", intfName);
        GetUrl();
        le_data_Release(ConnectionRef);
    }
    else
    {
        LE_INFO("Interface %s disconnected.", intfName);
    }

}

COMPONENT_INIT
{
    printf("HTTP Get!\n");

    le_timer_Ref_t timerPtr = le_timer_Create("Connection timeout timer");
    le_clk_Time_t interval = {TIMEOUT_SECS, 0};
    le_timer_SetInterval(timerPtr, interval);
    le_timer_SetHandler(timerPtr, &TimeoutHandler);
    WaitingForConnection = true;
    le_timer_Start(timerPtr);

    le_data_AddConnectionStateHandler(&ConnectionStateHandler, NULL);
    LE_INFO("Requesting connection...");
    ConnectionRef = le_data_Request();
}
