/**
 * @file socketTest.c
 *
 * This module implements an integration test for the socket component. It acts like a small
 * utility for reading from and writing to connections using TCP or UDP.
 *
 * Build:
 * Use the following command to compile this integration test for Linux targets when using mkapp:
 *   - CONFIG_SOCKET_LIB_USE_OPENSSL=y CONFIG_LINUX=y mkapp -t <target> socketTest.adef
 *
 * Usage:
 *   - app runProc socketTest socketTest -- security_flag host port type data
 *
 * Example:
 *   - Unsecure: app runProc socketTest socketTest -- 0 google.fr 80  TCP DATA
 *   - Secure:   app runProc socketTest socketTest -- 1 m2mop.net 443 TCP DATA
 *
 * Note:
 *   - Security flag only uses a default certificate to connect to m2mop.net remote server.
 *   - If data field is not specified, a sample HTTP HEAD request is sent to remote server.
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"
#include "le_socketLib.h"
#include "defaultDerKey.h"

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Size of transmission buffer
 */
//--------------------------------------------------------------------------------------------------
#define BUFFER_SIZE           512

//--------------------------------------------------------------------------------------------------
/**
 * Reception timeout in milliseconds
 */
//--------------------------------------------------------------------------------------------------
#define RX_TIMEOUT_MS         1000

//--------------------------------------------------------------------------------------------------
/**
 * Requests loop
 */
//--------------------------------------------------------------------------------------------------
#define REQUESTS_LOOP         3

//--------------------------------------------------------------------------------------------------
/**
 * Asynchronous socket data structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    int  index;                ///< Data index
    char data[BUFFER_SIZE];    ///< Data buffer
}
AsyncData_t;

//--------------------------------------------------------------------------------------------------
// Internal variables
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Asynchronous socket data
 */
//--------------------------------------------------------------------------------------------------
static AsyncData_t AsyncData;

//--------------------------------------------------------------------------------------------------
// Functions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 *  Event handler definition to monitor input and output data availability for sockets.
 */
//--------------------------------------------------------------------------------------------------
static void SocketEventHandler
(
    le_socket_Ref_t  socketRef, ///< [IN] Socket context reference
    short            events,    ///< [IN] Bitmap of events that occurred
    void*            userPtr    ///< [IN] User data pointer
)
{
    AsyncData_t* dataPtr = (AsyncData_t*)userPtr;
    if (!dataPtr)
    {
        LE_ERROR("Unable to retrieve user data pointer");
        le_socket_Delete(socketRef);
        exit(EXIT_FAILURE);
    }

    LE_INFO("Event: %d", events);

    if (events & POLLRDHUP)
    {
        LE_INFO("Connection closed by remote server");
        le_socket_Delete(socketRef);
        exit(EXIT_SUCCESS);
    }

    if (events & POLLOUT)
    {
        if (dataPtr->index)
        {
            if (LE_OK != le_socket_Send(socketRef, (char*)dataPtr->data, strlen(dataPtr->data)))
            {
                LE_ERROR("Unable to send data");
                le_socket_Delete(socketRef);
                exit(EXIT_FAILURE);
            }
            dataPtr->index--;
        }
    }

    if (events & POLLIN)
    {
        char buffer[BUFFER_SIZE];
        size_t length = sizeof(buffer);

        if (LE_OK != le_socket_Read(socketRef, buffer, &length))
        {
            LE_ERROR("Unable to send data");
            le_socket_Delete(socketRef);
            exit(EXIT_FAILURE);
        }

        LE_INFO("Data received size: %zu", length);
        LE_DUMP((const uint8_t*)buffer, length);
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
    le_socket_Ref_t socketRef;
    SocketType_t type;
    char sampleRequest[] = "HEAD / HTTP/1.1\r\n\r\n";

    // Check arguments number
    if (le_arg_NumArgs() < 4)
    {
        LE_INFO("Usage: app runProc socketTest socketTest -- security_flag host port type data");
        exit(EXIT_FAILURE);
    }

    // Get and decode arguments
    bool securityFlag = (strtol(le_arg_GetArg(0), NULL, 10) == 1) ? true : false;
    char* hostPtr     = (char*)le_arg_GetArg(1);
    long portNumber   = strtol(le_arg_GetArg(2), NULL, 10);
    char* typePtr     = (char*)le_arg_GetArg(3);
    char* dataPtr     = (le_arg_NumArgs() == 5) ? (char*)le_arg_GetArg(4) : (char*)sampleRequest;

    // Check parameters validity
    if ((!hostPtr) || (!typePtr))
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

    if (!memcmp(typePtr, "TCP", sizeof("TCP")))
    {
        type = TCP_TYPE;
    }
    else if (!memcmp(typePtr, "UDP", sizeof("UDP")))
    {
        type = UDP_TYPE;
    }
    else
    {
        LE_ERROR("Unrecognized socket type. Use UDP or TCP sockets only");
        exit(EXIT_FAILURE);
    }

    LE_INFO("Creating the %s socket...", typePtr);
    //! [SocketCreate]
    socketRef = le_socket_Create(hostPtr, (uint16_t)portNumber, type);
    if (!socketRef)
    {
        LE_ERROR("Failed to connect socket");
        exit(EXIT_FAILURE);
    }
    //! [SocketCreate]

    if (securityFlag)
    {
        LE_INFO("Adding default security certificate...");
        //! [SocketCertificate]
        status = le_socket_AddCertificate(socketRef, DefaultDerKey, DEFAULT_DER_KEY_LEN);
        if (LE_OK != status)
        {
            LE_ERROR("Failed to add certificate");
            goto end;
        }
        //! [SocketCertificate]
    }

    LE_INFO("Setting timeout to %d milliseconds...", RX_TIMEOUT_MS);
    status = le_socket_SetTimeout(socketRef, RX_TIMEOUT_MS);
    if (LE_OK != status)
    {
        LE_ERROR("Unable to set timeout");
        goto end;
    }

    LE_INFO("Starting the socket connection...");
    //! [SocketConnect]
    status = le_socket_Connect(socketRef);
    if (LE_OK != status)
    {
        LE_ERROR("Unable to start HTTP session");
        goto end;
    }
    //! [SocketConnect]

    LE_INFO("Sending data through socket %d times...", REQUESTS_LOOP);
    int i = REQUESTS_LOOP;

    while (i)
    {
        status = le_socket_Send(socketRef, dataPtr, strlen(dataPtr));
        if (LE_OK != status)
        {
            LE_ERROR("Unable to send data");
            goto end;
        }

        char buffer[BUFFER_SIZE];
        size_t length;
        bool chunkReceived = false;

        while (true)
        {
            length = sizeof(buffer);

            status = le_socket_Read(socketRef, buffer, &length);
            if (LE_OK == status)
            {
                LE_INFO("Data received size: %zu", length);
                LE_DUMP((const uint8_t*)buffer, length);
                chunkReceived = true;

                if (length == 0)
                {
                    break;
                }
            }
            else
            {
                if (!chunkReceived)
                {
                    LE_ERROR("Nothing received on socket");
                    status = LE_FAULT;
                    goto end;
                }
                break;
            }
        }
        i--;
    }

    LE_INFO("Sending data through socket %d times in a async way...", REQUESTS_LOOP);

    AsyncData.index = REQUESTS_LOOP;
    strncpy(AsyncData.data, dataPtr, sizeof(AsyncData.data)-1);

    //! [SocketMonitoring]
    status = le_socket_AddEventHandler(socketRef, SocketEventHandler, &AsyncData);
    if (LE_OK != status)
    {
        LE_ERROR("Failed to add socket event handler");
        goto end;
    }

    status = le_socket_SetMonitoring(socketRef, true);
    if (LE_OK != status)
    {
        LE_ERROR("Failed to set monitoring mode");
        goto end;
    }
    //! [SocketMonitoring]

    status = le_socket_Send(socketRef, dataPtr, strlen(dataPtr));
    if (LE_OK != status)
    {
        LE_ERROR("Unable to send data");
        goto end;
    }

end:
    if (LE_OK != status)
    {
        le_socket_Disconnect(socketRef);
        le_socket_Delete(socketRef);
        exit(EXIT_FAILURE);
    }
}
