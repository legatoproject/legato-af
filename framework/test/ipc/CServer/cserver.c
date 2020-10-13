/**
 * Implement echo test API in C
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"

#include <string.h>

void ipcTest_EchoSimple
(
    int32_t InValue,
    int32_t *OutValuePtr
)
{
    if (OutValuePtr)
    {
        *OutValuePtr = InValue;
    }
}

void ipcTest_EchoSmallEnum
(
    ipcTest_SmallEnum_t InValue,
    ipcTest_SmallEnum_t* OutValuePtr
)
{
    if (OutValuePtr)
    {
        *OutValuePtr = InValue;
    }
}

void ipcTest_EchoLargeEnum
(
    ipcTest_LargeEnum_t InValue,
    ipcTest_LargeEnum_t* OutValuePtr
)
{
    if (OutValuePtr)
    {
        *OutValuePtr = InValue;
    }
}

void ipcTest_EchoSmallBitMask
(
    ipcTest_SmallBitMask_t InValue,
    ipcTest_SmallBitMask_t* OutValuePtr
)
{
    if (OutValuePtr)
    {
        *OutValuePtr = InValue;
    }
}

void ipcTest_EchoLargeBitMask
(
    ipcTest_LargeBitMask_t InValue,
    ipcTest_LargeBitMask_t* OutValuePtr
)
{
    if (OutValuePtr)
    {
        *OutValuePtr = InValue;
    }
}

void ipcTest_EchoBoolean
(
    bool InValue,
    bool* OutValuePtr
)
{
    if (OutValuePtr)
    {
        *OutValuePtr = InValue;
    }
}

void ipcTest_EchoResult
(
    le_result_t InValue,
    le_result_t* OutValuePtr
)
{
    if (OutValuePtr)
    {
        *OutValuePtr = InValue;
    }
}

le_result_t ipcTest_ReturnResult
(
    le_result_t InValue
)
{
    return InValue;
}

void ipcTest_EchoOnOff
(
    le_onoff_t InValue,
    le_onoff_t* OutValuePtr
)
{
    if (OutValuePtr)
    {
        *OutValuePtr = InValue;
    }
}

void ipcTest_EchoDouble
(
    double InValue,
    double* OutValuePtr
)
{
    if (OutValuePtr)
    {
        *OutValuePtr = InValue;
    }
}

void ipcTest_EchoReference
(
    ipcTest_SimpleRef_t InRef,
    ipcTest_SimpleRef_t *OutRef
)
{
    if (OutRef)
    {
        *OutRef = InRef;
    }
}

void ipcTest_EchoString
(
    const char *InString,
    char *OutString,
    size_t OutStringSize
)
{
    if (OutString)
    {
        strncpy(OutString, InString, OutStringSize);
        OutString[OutStringSize-1] = '\0';
    }
}

void ipcTest_EchoArray
(
    const int64_t* InArrayPtr,
    size_t InArraySize,
    int64_t* OutArrayPtr,
    size_t* OutArraySizePtr
)
{
    if (OutArrayPtr)
    {
        if (InArraySize <= *OutArraySizePtr)
        {
            memcpy(OutArrayPtr, InArrayPtr, InArraySize*sizeof(int64_t));
            *OutArraySizePtr = InArraySize;
        }
        else
        {
            memcpy(OutArrayPtr, InArrayPtr, (*OutArraySizePtr)*sizeof(int64_t));
        }
    }
}


void ipcTest_EchoByteString
(
    const uint8_t* InArrayPtr,
    size_t InArraySize,
    uint8_t* OutArrayPtr,
    size_t* OutArraySizePtr
)
{
    if (OutArrayPtr)
    {
        if (InArraySize <= *OutArraySizePtr)
        {
            memcpy(OutArrayPtr, InArrayPtr, InArraySize*sizeof(uint8_t));
            *OutArraySizePtr = InArraySize;
        }
        else
        {
            memcpy(OutArrayPtr, InArrayPtr, (*OutArraySizePtr)*sizeof(uint8_t));
        }
    }
}

void ipcTest_EchoStruct
(
    const ipcTest_TheStruct_t * LE_NONNULL InStructPtr,
    ipcTest_TheStruct_t * OutStructPtr
)
{
    if (OutStructPtr)
    {
        memcpy(OutStructPtr, InStructPtr, sizeof(ipcTest_TheStruct_t));
    }
}

void ipcTest_EchoStructArray
(
    const ipcTest_TheStruct_t* InStructArrayPtr,
    size_t InStructArraySize,
    ipcTest_TheStruct_t* OutStructArrayPtr,
    size_t* OutStructArraySizePtr
)
{
    if (OutStructArrayPtr)
    {
        if (InStructArraySize <= *OutStructArraySizePtr)
        {
            memcpy(OutStructArrayPtr, InStructArrayPtr,
                   InStructArraySize * sizeof(ipcTest_TheStruct_t));
            *OutStructArraySizePtr = InStructArraySize;
        }
        else
        {
            memcpy(OutStructArrayPtr, InStructArrayPtr,
                   (*OutStructArraySizePtr) * sizeof(ipcTest_TheStruct_t));
        }
    }
}

void ipcTest_ExitServer
(
    void
)
{
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
    LE_INFO("Adding Event Handler");
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
    LE_INFO("Removing event handler");
    // Remove if this is the current handler.
    if ((size_t)handlerRef == EchoEventRef)
    {
        EchoEventRef += 2;
        EchoEventHandlerPtr = NULL;
        EchoEventContextPtr = NULL;
    }
}

void ipcTest_EchoTriggerEvent
(
    int32_t cookie ///< [IN]
)
{
    LE_INFO("Triggering an Event");
    if (EchoEventHandlerPtr)
    {
        EchoEventHandlerPtr(cookie, EchoEventContextPtr);
    }
}


/**
 * Storage for pointer to Complex event handlers
 */
static ipcTest_EchoComplexHandlerFunc_t EchoComplexEventHandlerPtr = NULL;
static void* EchoComplexEventContextPtr = NULL;
size_t EchoComplexEventRef=1;

ipcTest_EchoComplexEventHandlerRef_t ipcTest_AddEchoComplexEventHandler
(
    ipcTest_EchoComplexHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    LE_INFO("Adding Complex Event Handler");
    // For simplicity, only allow a single event handler
    if (EchoComplexEventHandlerPtr)
    {
        return NULL;
    }

    EchoComplexEventHandlerPtr = handlerPtr;
    EchoComplexEventContextPtr = contextPtr;

    return (ipcTest_EchoComplexEventHandlerRef_t)EchoComplexEventRef;
}

void ipcTest_RemoveEchoComplexEventHandler
(
    ipcTest_EchoComplexEventHandlerRef_t handlerRef
)
{
    // Remove if this is the current handler.
    if ((size_t)handlerRef == EchoComplexEventRef)
    {
        EchoComplexEventRef += 2;
        EchoComplexEventHandlerPtr = NULL;
        EchoComplexEventContextPtr = NULL;
    }
}

void ipcTest_EchoTriggerComplexEvent
(
    int32_t cookie,
    const char* LE_NONNULL cookieString,
    const int16_t* cookieArrayPtr,
    size_t cookieArraySize
)
{
    LE_INFO("Triggering a complex Event");
    if (EchoComplexEventHandlerPtr)
    {
        EchoComplexEventHandlerPtr(cookie, cookieString, cookieArrayPtr, cookieArraySize,
                            EchoComplexEventContextPtr);
    }
}


COMPONENT_INIT
{
}
