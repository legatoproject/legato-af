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

void ipcServer_EchoBoolean
(
    bool InValue,
    bool* OutValuePtr
)
{
    ipcClient_EchoBoolean(InValue, OutValuePtr);
}

void ipcServer_EchoResult
(
    le_result_t InValue,
    le_result_t* OutValuePtr
)
{
    ipcClient_EchoResult(InValue, OutValuePtr);
}

le_result_t ipcServer_ReturnResult
(
    le_result_t InValue
)
{
    return ipcClient_ReturnResult(InValue);
}

void ipcServer_EchoOnOff
(
    le_onoff_t InValue,
    le_onoff_t* OutValuePtr
)
{
    ipcClient_EchoOnOff(InValue, OutValuePtr);
}

void ipcServer_EchoDouble
(
    double InValue,
    double* OutValuePtr
)
{
    ipcClient_EchoDouble(InValue, OutValuePtr);
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

void ipcServer_EchoArray
(
    const int64_t* InArrayPtr,
    size_t InArraySize,
    int64_t* OutArrayPtr,
    size_t* OutArraySizePtr
)
{
    ipcClient_EchoArray(InArrayPtr, InArraySize, OutArrayPtr, OutArraySizePtr);
}

void ipcServer_EchoByteString
(
    const uint8_t* InArrayPtr,
    size_t InArraySize,
    uint8_t* OutArrayPtr,
    size_t* OutArraySizePtr
)
{
    ipcClient_EchoByteString(InArrayPtr, InArraySize, OutArrayPtr, OutArraySizePtr);
}

void ipcServer_EchoStruct
(
    const ipcServer_TheStruct_t * LE_NONNULL InStructPtr,
    ipcServer_TheStruct_t * OutStructPtr
)
{
    ipcClient_EchoStruct(InStructPtr, OutStructPtr);
}

void ipcServer_EchoStructArray
(
    const ipcServer_TheStruct_t* InStructArrayPtr,
    size_t InStructArraySize,
    ipcServer_TheStruct_t* OutStructArrayPtr,
    size_t* OutStructArraySizePtr
)
{
    ipcClient_EchoStructArray(InStructArrayPtr, InStructArraySize, OutStructArrayPtr,
                              OutStructArraySizePtr);
}

void ipcServer_ExitServer
(
    void
)
{
    ipcClient_ExitServer();
}

ipcServer_EchoComplexEventHandlerRef_t ipcServer_AddEchoComplexEventHandler
(
    ipcServer_EchoComplexHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    return (ipcServer_EchoComplexEventHandlerRef_t)ipcClient_AddEchoComplexEventHandler(handlerPtr, contextPtr);
}

void ipcServer_RemoveEchoComplexEventHandler
(
    ipcServer_EchoComplexEventHandlerRef_t handlerRef
)
{
    ipcClient_RemoveEchoComplexEventHandler((ipcClient_EchoComplexEventHandlerRef_t)handlerRef);
}

void ipcServer_EchoTriggerComplexEvent
(
    int32_t cookie,
    const char* LE_NONNULL cookieString,
    const int16_t* cookieArrayPtr,
    size_t cookieArraySize
)
{
    ipcClient_EchoTriggerComplexEvent(cookie, cookieString, cookieArrayPtr, cookieArraySize);
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
)
{
    ipcClient_EchoTriggerEvent(cookie);
}

COMPONENT_INIT
{
}
