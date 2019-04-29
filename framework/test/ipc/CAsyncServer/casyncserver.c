/**
 * Implement echo test API in C
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"

#include <string.h>

#define MAX_VALUE_SIZE 257

le_mem_PoolRef_t ValuePool;

void AsyncServer_EchoSimpleRespond
(
    void* serverCmdPtr,
    void* valuePtr
)
{
    // Weird cast avoids warnings on Ubuntu 14.04
    ipcTest_EchoSimpleRespond(serverCmdPtr,
                              (uint32_t)((size_t)valuePtr));
}

void ipcTest_EchoSimple
(
    ipcTest_ServerCmdRef_t serverCmdPtr,
    int32_t InValue
)
{
    // Weird cast avoids warnings on Ubuntu 14.04
    le_event_QueueFunction(AsyncServer_EchoSimpleRespond,
                           serverCmdPtr,
                           (void*)((size_t)InValue));
}

void AsyncServer_EchoSmallEnumRespond
(
    void* serverCmdPtr,
    void* valuePtr
)
{
    ipcTest_EchoSmallEnumRespond(serverCmdPtr,
                                 (ipcTest_SmallEnum_t)valuePtr);
}

void ipcTest_EchoSmallEnum
(
    ipcTest_ServerCmdRef_t serverCmdPtr,
    ipcTest_SmallEnum_t InValue
)
{
    le_event_QueueFunction(AsyncServer_EchoSmallEnumRespond,
                           serverCmdPtr,
                           (void*)InValue);
}

void AsyncServer_EchoLargeEnumRespond
(
    void* serverCmdPtr,
    void* valuePtr
)
{
    ipcTest_EchoLargeEnumRespond(serverCmdPtr,
                                 *(ipcTest_LargeEnum_t*)valuePtr);
    le_mem_Release(valuePtr);
}


void ipcTest_EchoLargeEnum
(
    ipcTest_ServerCmdRef_t serverCmdPtr,
    ipcTest_LargeEnum_t InValue
)
{
    ipcTest_LargeEnum_t* valuePtr = le_mem_ForceAlloc(ValuePool);
    *valuePtr = InValue;

    le_event_QueueFunction(AsyncServer_EchoLargeEnumRespond,
                           serverCmdPtr,
                           valuePtr);
}


void AsyncServer_EchoSmallBitMaskRespond
(
    void* serverCmdPtr,
    void* valuePtr
)
{
    ipcTest_EchoSmallBitMaskRespond(serverCmdPtr,
                                    (ipcTest_SmallBitMask_t)valuePtr);
}


void ipcTest_EchoSmallBitMask
(
    ipcTest_ServerCmdRef_t serverCmdPtr,
    ipcTest_SmallBitMask_t InValue
)
{
    le_event_QueueFunction(AsyncServer_EchoSmallBitMaskRespond,
                           serverCmdPtr,
                           (void*)InValue);
}

void AsyncServer_EchoLargeBitMaskRespond
(
    void* serverCmdPtr,
    void* valuePtr
)
{
    ipcTest_EchoLargeBitMaskRespond(serverCmdPtr,
                                    *(ipcTest_LargeBitMask_t*)valuePtr);
    le_mem_Release(valuePtr);
}


void ipcTest_EchoLargeBitMask
(
    ipcTest_ServerCmdRef_t serverCmdPtr,
    ipcTest_LargeBitMask_t InValue
)
{
    ipcTest_LargeBitMask_t* valuePtr = le_mem_ForceAlloc(ValuePool);
    *valuePtr = InValue;

    le_event_QueueFunction(AsyncServer_EchoLargeBitMaskRespond,
                           serverCmdPtr,
                           valuePtr);
}

void AsyncServer_EchoReferenceRespond
(
    void* serverCmdPtr,
    void* valuePtr
)
{
    ipcTest_EchoReferenceRespond(serverCmdPtr,
                                 valuePtr);
}


void ipcTest_EchoReference
(
    ipcTest_ServerCmdRef_t serverCmdPtr,
    ipcTest_SimpleRef_t InRef
)
{
    le_event_QueueFunction(AsyncServer_EchoReferenceRespond,
                           serverCmdPtr,
                           InRef);
}

void AsyncServer_EchoStringRespond
(
    void* serverCmdPtr,
    void* valuePtr
)
{
    ipcTest_EchoStringRespond(serverCmdPtr,
                              valuePtr);
    le_mem_Release(valuePtr);
}


void ipcTest_EchoString
(
    ipcTest_ServerCmdRef_t serverCmdPtr,
    const char* InString,
    size_t OutStringSize
)
{
    // Cap output string size at maximum size of buffer
    if (OutStringSize > MAX_VALUE_SIZE) { OutStringSize = MAX_VALUE_SIZE; }

    char* OutString = le_mem_ForceAlloc(ValuePool);
    strncpy(OutString, InString, OutStringSize);
    OutString[OutStringSize-1] = '\0';
    le_event_QueueFunction(AsyncServer_EchoStringRespond,
                           serverCmdPtr,
                           OutString);
}


void ipcTest_ExitServer
(
    ipcTest_ServerCmdRef_t serverCmdRef
)
{
    LE_UNUSED(serverCmdRef);
    abort();
}

/**
 * Storage for pointer to event handlers
 */
static ipcTest_EchoHandlerFunc_t EchoEventHandlerPtr = NULL;
static void* EchoEventContextPtr = NULL;
size_t EchoEventRef=1;

ipcTest_EchoEventHandlerRef_t ipcTest_AddEchoEventHandler
(
    ipcTest_EchoHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    // For simplicity, only allow a single event handler
    if (EchoEventHandlerPtr)
    {
        return NULL;
    }

    EchoEventHandlerPtr = handlerPtr;
    EchoEventContextPtr = contextPtr;

    return (ipcTest_EchoEventHandlerRef_t)EchoEventRef;
}

void ipcTest_RemoveEchoEventHandler
(
    ipcTest_EchoEventHandlerRef_t handlerRef
)
{
    // Remove if this is the current handler.
    if ((size_t)handlerRef == EchoEventRef)
    {
        EchoEventRef += 2;
        EchoEventHandlerPtr = NULL;
        EchoEventContextPtr = NULL;
    }
}

void AsyncServer_EchoTriggerEventRespond
(
    void* serverCmdPtr,
    void* context
)
{
    LE_UNUSED(context);
    ipcTest_EchoTriggerEventRespond(serverCmdPtr);
}


void ipcTest_EchoTriggerEvent
(
    ipcTest_ServerCmdRef_t serverCmdPtr,
    int32_t cookie
        ///< [IN]
)
{
    if (EchoEventHandlerPtr)
    {
        EchoEventHandlerPtr(cookie, EchoEventContextPtr);
    }

    le_event_QueueFunction(AsyncServer_EchoTriggerEventRespond,
                           serverCmdPtr,
                           NULL);
}




COMPONENT_INIT
{
    ValuePool = le_mem_CreatePool("values", MAX_VALUE_SIZE);
}
