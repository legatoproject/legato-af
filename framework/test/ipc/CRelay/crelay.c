/**
 * Implement echo test API in C by calling another echo test API server
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"

#include <string.h>

void ipcServer_EchoSimple
(
    int32_t InValue,
    int32_t *OutValuePtr
)
{
    ipcClient_EchoSimple(InValue, OutValuePtr);
}

void ipcServer_EchoSmallEnum
(
    ipcServer_SmallEnum_t InValue,
    ipcServer_SmallEnum_t* OutValuePtr
)
{
    ipcClient_EchoSmallEnum((ipcClient_SmallEnum_t)InValue,
                            (ipcClient_SmallEnum_t*)OutValuePtr);
}

void ipcServer_EchoLargeEnum
(
    ipcServer_LargeEnum_t InValue,
    ipcServer_LargeEnum_t* OutValuePtr
)
{
    ipcClient_EchoLargeEnum((ipcClient_LargeEnum_t)InValue,
                            (ipcClient_LargeEnum_t*)OutValuePtr);
}

void ipcServer_EchoSmallBitMask
(
    ipcServer_SmallBitMask_t InValue,
    ipcServer_SmallBitMask_t* OutValuePtr
)
{
    ipcClient_EchoSmallBitMask((ipcClient_SmallBitMask_t)InValue,
                               (ipcClient_SmallBitMask_t*)OutValuePtr);
}

void ipcServer_EchoLargeBitMask
(
    ipcServer_LargeBitMask_t InValue,
    ipcServer_LargeBitMask_t* OutValuePtr
)
{
    ipcClient_EchoLargeBitMask((ipcClient_LargeBitMask_t)InValue,
                               (ipcClient_LargeBitMask_t*)OutValuePtr);
}

void ipcServer_EchoReference
(
    ipcServer_SimpleRef_t InRef,
    ipcServer_SimpleRef_t *OutRef
)
{
    ipcClient_EchoReference((ipcClient_SimpleRef_t)InRef,
                            (ipcClient_SimpleRef_t*)OutRef);
}

void ipcServer_EchoString
(
    const char *InString,
    char *OutString,
    size_t OutStringSize
)
{
    ipcClient_EchoString(InString, OutString, OutStringSize);
}

#if 0
// Not currently supported on Java.
void ipcServer_EchoArray
(
    const int64_t* InArrayPtr,
    size_t InArraySize,
    int64_t* OutArrayPtr,
    size_t* OutArraySizePtr
)
{
    ipcClient_EchoArray(InArrayPtr, InArraySize, OutArrayPtr, OutArraySizePtr
}
#endif

void ipcServer_ExitServer
(
    void
)
{
    ipcClient_ExitServer();
}

ipcServer_EchoEventHandlerRef_t ipcServer_AddEchoEventHandler
(
    ipcServer_EchoHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    return (ipcServer_EchoEventHandlerRef_t)ipcClient_AddEchoEventHandler(handlerPtr, contextPtr);
}

void ipcServer_RemoveEchoEventHandler
(
    ipcServer_EchoEventHandlerRef_t handlerRef
)
{
    ipcClient_RemoveEchoEventHandler((ipcClient_EchoEventHandlerRef_t)handlerRef);
}

void ipcServer_EchoTriggerEvent
(
    int32_t cookie
        ///< [IN]
)
{
    ipcClient_EchoTriggerEvent(cookie);
}


COMPONENT_INIT
{
}
