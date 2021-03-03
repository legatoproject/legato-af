/**
 * Implement echo test API in C by calling another echo test API server
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"

#include <string.h>

void ipcServer_AddOneSimple
(
    int32_t InValue,
    int32_t *OutValuePtr
)
{
    ipcClient_AddOneSimple(InValue, OutValuePtr);
}

void ipcServer_AddOneSmallEnum
(
    ipcServer_SmallEnum_t InValue,
    ipcServer_SmallEnum_t* OutValuePtr
)
{
    ipcClient_AddOneSmallEnum((ipcClient_SmallEnum_t)InValue,
                              (ipcClient_SmallEnum_t*)OutValuePtr);
}

void ipcServer_AddOneLargeEnum
(
    ipcServer_LargeEnum_t InValue,
    ipcServer_LargeEnum_t* OutValuePtr
)
{
    ipcClient_AddOneLargeEnum((ipcClient_LargeEnum_t)InValue,
                              (ipcClient_LargeEnum_t*)OutValuePtr);
}

void ipcServer_NotSmallBitMask
(
    ipcServer_SmallBitMask_t InValue,
    ipcServer_SmallBitMask_t* OutValuePtr
)
{
    ipcClient_NotSmallBitMask((ipcClient_SmallBitMask_t)InValue,
                              (ipcClient_SmallBitMask_t*)OutValuePtr);
}

void ipcServer_NotLargeBitMask
(
    ipcServer_LargeBitMask_t InValue,
    ipcServer_LargeBitMask_t* OutValuePtr
)
{
    ipcClient_NotLargeBitMask((ipcClient_LargeBitMask_t)InValue,
                              (ipcClient_LargeBitMask_t*)OutValuePtr);
}

void ipcServer_NotBoolean
(
    bool InValue,
    bool* OutValuePtr
)
{
    ipcClient_NotBoolean(InValue, OutValuePtr);
}

void ipcServer_NextResult
(
    le_result_t InValue,
    le_result_t* OutValuePtr
)
{
    ipcClient_NextResult(InValue, OutValuePtr);
}

le_result_t ipcServer_ReturnNextResult
(
    le_result_t InValue
)
{
    return ipcClient_ReturnNextResult(InValue);
}

void ipcServer_NotOnOff
(
    le_onoff_t InValue,
    le_onoff_t* OutValuePtr
)
{
    ipcClient_NotOnOff(InValue, OutValuePtr);
}

void ipcServer_AddOneDouble
(
    double InValue,
    double* OutValuePtr
)
{
    ipcClient_AddOneDouble(InValue, OutValuePtr);
}

void ipcServer_AddFourReference
(
    ipcServer_SimpleRef_t InRef,
    ipcServer_SimpleRef_t *OutRef
)
{
    ipcClient_AddFourReference((ipcClient_SimpleRef_t)InRef,
                               (ipcClient_SimpleRef_t*)OutRef);
}

void ipcServer_ROT13String
(
    const char *InString,
    char *OutString,
    size_t OutStringSize
)
{
    ipcClient_ROT13String(InString, OutString, OutStringSize);
}

void ipcServer_AddOneArray
(
    const int64_t* InArrayPtr,
    size_t InArraySize,
    int64_t* OutArrayPtr,
    size_t* OutArraySizePtr
)
{
    ipcClient_AddOneArray(InArrayPtr, InArraySize, OutArrayPtr, OutArraySizePtr);
}

void ipcServer_NotByteString
(
    const uint8_t* InArrayPtr,
    size_t InArraySize,
    uint8_t* OutArrayPtr,
    size_t* OutArraySizePtr
)
{
    ipcClient_NotByteString(InArrayPtr, InArraySize, OutArrayPtr, OutArraySizePtr);
}

void ipcServer_AddOneROT13Struct
(
    const ipcServer_TheStruct_t * LE_NONNULL InStructPtr,
    ipcServer_TheStruct_t * OutStructPtr
)
{
    ipcClient_AddOneROT13Struct(InStructPtr, OutStructPtr);
}

void ipcServer_AddOneROT13StructArray
(
    const ipcServer_TheStruct_t* InStructArrayPtr,
    size_t InStructArraySize,
    ipcServer_TheStruct_t* OutStructArrayPtr,
    size_t* OutStructArraySizePtr
)
{
    ipcClient_AddOneROT13StructArray(InStructArrayPtr, InStructArraySize, OutStructArrayPtr,
                                     OutStructArraySizePtr);
}

void ipcServer_ExitServer
(
    void
)
{
    ipcClient_ExitServer();
 }

ipcServer_AddOneROT13EventHandlerRef_t ipcServer_AddAddOneROT13EventHandler
(
    ipcServer_AddOneROT13HandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    return (ipcServer_AddOneROT13EventHandlerRef_t)ipcClient_AddAddOneROT13EventHandler(handlerPtr, contextPtr);
}

void ipcServer_RemoveAddOneROT13EventHandler
(
    ipcServer_AddOneROT13EventHandlerRef_t handlerRef
)
{
    ipcClient_RemoveAddOneROT13EventHandler((ipcClient_AddOneROT13EventHandlerRef_t)handlerRef);
}

void ipcServer_TriggerAddOneROT13Event
(
    int32_t cookie,
    const char* LE_NONNULL cookieString,
    const int16_t* cookieArrayPtr,
    size_t cookieArraySize
)
{
    ipcClient_TriggerAddOneROT13Event(cookie, cookieString, cookieArrayPtr, cookieArraySize);
}

ipcServer_AddOneEventHandlerRef_t ipcServer_AddAddOneEventHandler
(
    ipcServer_AddOneHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    return (ipcServer_AddOneEventHandlerRef_t)ipcClient_AddAddOneEventHandler(handlerPtr, contextPtr);
}

void ipcServer_RemoveAddOneEventHandler
(
    ipcServer_AddOneEventHandlerRef_t handlerRef
)
{
    ipcClient_RemoveAddOneEventHandler((ipcClient_AddOneEventHandlerRef_t)handlerRef);
}

void ipcServer_TriggerAddOneEvent
(
    int32_t cookie
)
{
    ipcClient_TriggerAddOneEvent(cookie);
}

COMPONENT_INIT
{
}
