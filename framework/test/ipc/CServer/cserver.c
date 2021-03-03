/**
 * Implement echo test API in C
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"

#include <string.h>

#include "utils.h"

void ipcTest_AddOneSimple
(
    int32_t InValue,
    int32_t *OutValuePtr
)
{
    if (OutValuePtr)
    {
        *OutValuePtr = InValue + 1;
    }
}

void ipcTest_AddOneSmallEnum
(
    ipcTest_SmallEnum_t InValue,
    ipcTest_SmallEnum_t* OutValuePtr
)
{
    if (OutValuePtr)
    {
        *OutValuePtr = util_IncSmallEnum(InValue);
    }
}

void ipcTest_AddOneLargeEnum
(
    ipcTest_LargeEnum_t InValue,
    ipcTest_LargeEnum_t* OutValuePtr
)
{
    if (OutValuePtr)
    {
        *OutValuePtr = util_IncLargeEnum(InValue);
    }
}

void ipcTest_NotSmallBitMask
(
    ipcTest_SmallBitMask_t InValue,
    ipcTest_SmallBitMask_t* OutValuePtr
)
{
    if (OutValuePtr)
    {
        *OutValuePtr = ~InValue;
    }
}

void ipcTest_NotLargeBitMask
(
    ipcTest_LargeBitMask_t InValue,
    ipcTest_LargeBitMask_t* OutValuePtr
)
{
    if (OutValuePtr)
    {
        *OutValuePtr = ~InValue;
    }
}

void ipcTest_NotBoolean
(
    bool InValue,
    bool* OutValuePtr
)
{
    if (OutValuePtr)
    {
        *OutValuePtr = !InValue;
    }
}

void ipcTest_NextResult
(
    le_result_t InValue,
    le_result_t* OutValuePtr
)
{
    if (OutValuePtr)
    {
        *OutValuePtr = util_IncResult(InValue);
    }
}

le_result_t ipcTest_ReturnNextResult
(
    le_result_t InValue
)
{
    return util_IncResult(InValue);
}

void ipcTest_NotOnOff
(
    le_onoff_t InValue,
    le_onoff_t* OutValuePtr
)
{
    if (OutValuePtr)
    {
        *OutValuePtr = util_NotOnOff(InValue);
    }
}

void ipcTest_AddOneDouble
(
    double InValue,
    double* OutValuePtr
)
{
    if (OutValuePtr)
    {
        *OutValuePtr = InValue + 1;
    }
}

void ipcTest_AddFourReference
(
    ipcTest_SimpleRef_t InRef,
    ipcTest_SimpleRef_t *OutRef
)
{
    if (OutRef)
    {
        if (InRef)
        {
            *OutRef = (ipcTest_SimpleRef_t)((uintptr_t)InRef + 4);
        }
        else
        {
            *OutRef = NULL;
        }
    }
}

void ipcTest_ROT13String
(
    const char *InString,
    char *OutString,
    size_t OutStringSize
)
{
    if (OutString)
    {
        util_ROT13String(InString, OutString, OutStringSize);
    }
}

void ipcTest_AddOneArray
(
    const int64_t* InArrayPtr,
    size_t InArraySize,
    int64_t* OutArrayPtr,
    size_t* OutArraySizePtr
)
{
    if (OutArrayPtr)
    {
        size_t i;

        for (i = 0; i < InArraySize && i < *OutArraySizePtr; ++i)
        {
            OutArrayPtr[i] = InArrayPtr[i] + 1;
        }

        *OutArraySizePtr = i;
    }
}


void ipcTest_NotByteString
(
    const uint8_t* InArrayPtr,
    size_t InArraySize,
    uint8_t* OutArrayPtr,
    size_t* OutArraySizePtr
)
{
    if (OutArrayPtr)
    {
        size_t i;

        for (i = 0; i < InArraySize && i < *OutArraySizePtr; ++i)
        {
            OutArrayPtr[i] = ~(InArrayPtr[i]);
        }

        *OutArraySizePtr = i;
    }
}

void ipcTest_AddOneROT13Struct
(
    const ipcTest_TheStruct_t * LE_NONNULL InStructPtr,
    ipcTest_TheStruct_t * OutStructPtr
)
{
    if (OutStructPtr)
    {
        OutStructPtr->index = InStructPtr->index + 1;
        util_ROT13String(InStructPtr->name, OutStructPtr->name, sizeof(OutStructPtr->name));
    }
}

void ipcTest_AddOneROT13StructArray
(
    const ipcTest_TheStruct_t* InStructArrayPtr,
    size_t InStructArraySize,
    ipcTest_TheStruct_t* OutStructArrayPtr,
    size_t* OutStructArraySizePtr
)
{
    if (OutStructArrayPtr)
    {
        size_t i;

        for (i = 0; i < InStructArraySize && i < *OutStructArraySizePtr; ++i)
        {
            OutStructArrayPtr[i].index = InStructArrayPtr[i].index + 1;
            util_ROT13String(InStructArrayPtr[i].name,
                             OutStructArrayPtr[i].name,
                             sizeof(OutStructArrayPtr[i].name));
        }

        *OutStructArraySizePtr = i;
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
static ipcTest_AddOneHandlerFunc_t EventHandlerPtr = NULL;
static void* EventContextPtr = NULL;
static size_t EventRef=1;

ipcTest_AddOneEventHandlerRef_t ipcTest_AddAddOneEventHandler
(
    ipcTest_AddOneHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    LE_INFO("Adding Event Handler");
    // For simplicity, only allow a single event handler
    if (EventHandlerPtr)
    {
        return NULL;
    }

    EventHandlerPtr = handlerPtr;
    EventContextPtr = contextPtr;

    return (ipcTest_AddOneEventHandlerRef_t)EventRef;
}

void ipcTest_RemoveAddOneEventHandler
(
    ipcTest_AddOneEventHandlerRef_t handlerRef
)
{
    LE_INFO("Removing event handler");
    // Remove if this is the current handler.
    if ((size_t)handlerRef == EventRef)
    {
        EventRef += 2;
        EventHandlerPtr = NULL;
        EventContextPtr = NULL;
    }
}

void ipcTest_TriggerAddOneEvent
(
    int32_t cookie ///< [IN]
)
{
    LE_INFO("Triggering an Event");
    if (EventHandlerPtr)
    {
        EventHandlerPtr(cookie + 1, EventContextPtr);
    }
}


/**
 * Storage for pointer to Complex event handlers
 */
static ipcTest_AddOneROT13HandlerFunc_t ComplexEventHandlerPtr = NULL;
static void* ComplexEventContextPtr = NULL;
size_t ComplexEventRef=1;

ipcTest_AddOneROT13EventHandlerRef_t ipcTest_AddAddOneROT13EventHandler
(
    ipcTest_AddOneROT13HandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    LE_INFO("Adding Complex Event Handler");
    // For simplicity, only allow a single event handler
    if (ComplexEventHandlerPtr)
    {
        return NULL;
    }

    ComplexEventHandlerPtr = handlerPtr;
    ComplexEventContextPtr = contextPtr;

    return (ipcTest_AddOneROT13EventHandlerRef_t)EventRef;
}

void ipcTest_RemoveAddOneROT13EventHandler
(
    ipcTest_AddOneROT13EventHandlerRef_t handlerRef
)
{
    // Remove if this is the current handler.
    if ((size_t)handlerRef == ComplexEventRef)
    {
        ComplexEventRef += 2;
        ComplexEventHandlerPtr = NULL;
        ComplexEventContextPtr = NULL;
    }
}

void ipcTest_TriggerAddOneROT13Event
(
    int32_t cookie,
    const char* LE_NONNULL cookieString,
    const int16_t* cookieArrayPtr,
    size_t cookieArraySize
)
{
    LE_INFO("Triggering a complex Event");
    if (ComplexEventHandlerPtr)
    {
        char outString[16];
        int16_t outArray[10];
        size_t i;

        util_ROT13String(cookieString, outString, sizeof(outString));
        for (i = 0; i < cookieArraySize && i < NUM_ARRAY_MEMBERS(outArray); ++i)
        {
            outArray[i] = cookieArrayPtr[i] + 1;
        }

        ComplexEventHandlerPtr(cookie + 1, outString, outArray, i,
                               ComplexEventContextPtr);
    }
}


COMPONENT_INIT
{
}
