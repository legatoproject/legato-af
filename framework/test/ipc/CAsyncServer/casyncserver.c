/**
 * Implement echo test API in C
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"

#include "utils.h"

#include <string.h>

#define MAX_VALUE_SIZE  257
#define VALUE_ENTRIES   6

typedef struct OutArrayInfo
{
    void* arrayPtr;
    size_t outArraySize;
} OutArrayInfo_t;

LE_MEM_DEFINE_STATIC_POOL(ValuePool, VALUE_ENTRIES, MAX_VALUE_SIZE);
LE_MEM_DEFINE_STATIC_POOL(OutArrayInfoPool, VALUE_ENTRIES, sizeof(OutArrayInfo_t));
LE_MEM_DEFINE_STATIC_POOL(TheStructPool, VALUE_ENTRIES, sizeof(ipcTest_TheStruct_t));
le_mem_PoolRef_t ValuePool;
le_mem_PoolRef_t OutArrayInfoPool;
le_mem_PoolRef_t TheStructPool;


void AsyncServer_AddOneSimpleRespond
(
    void* serverCmdPtr,
    void *valuePtr
)
{
    // Weird cast avoids warnings on Ubuntu 14.04
    ipcTest_AddOneSimpleRespond(serverCmdPtr,
                                (uint32_t)((size_t)valuePtr + 1));
}

void ipcTest_AddOneSimple
(
    ipcTest_ServerCmdRef_t serverCmdPtr,
    int32_t InValue
)
{
    // Weird cast avoids warnings on Ubuntu 14.04
    le_event_QueueFunction(AsyncServer_AddOneSimpleRespond,
                           serverCmdPtr,
                           (void*)((size_t)InValue));
}

void AsyncServer_AddOneSmallEnumRespond
(
    void* serverCmdPtr,
    void* valuePtr
)
{
    ipcTest_AddOneSmallEnumRespond(serverCmdPtr,
                                   util_IncSmallEnum((ipcTest_SmallEnum_t)((uintptr_t)valuePtr)));
}

void ipcTest_AddOneSmallEnum
(
    ipcTest_ServerCmdRef_t serverCmdPtr,
    ipcTest_SmallEnum_t InValue
)
{
    le_event_QueueFunction(AsyncServer_AddOneSmallEnumRespond,
                           serverCmdPtr,
                           (void*)InValue);
}

void AsyncServer_AddOneLargeEnumRespond
(
    void* serverCmdPtr,
    void* valuePtr
)
{
    ipcTest_AddOneLargeEnumRespond(serverCmdPtr,
                                   util_IncLargeEnum(*(ipcTest_LargeEnum_t*)valuePtr));
    le_mem_Release(valuePtr);
}


void ipcTest_AddOneLargeEnum
(
    ipcTest_ServerCmdRef_t serverCmdPtr,
    ipcTest_LargeEnum_t InValue
)
{
    ipcTest_LargeEnum_t* valuePtr = le_mem_AssertAlloc(ValuePool);
    *valuePtr = InValue;

    le_event_QueueFunction(AsyncServer_AddOneLargeEnumRespond,
                           serverCmdPtr,
                           valuePtr);
}


void AsyncServer_NotSmallBitMaskRespond
(
    void* serverCmdPtr,
    void* valuePtr
)
{
    ipcTest_NotSmallBitMaskRespond(serverCmdPtr,
                                   (ipcTest_SmallBitMask_t)(~(uintptr_t)valuePtr));
}


void ipcTest_NotSmallBitMask
(
    ipcTest_ServerCmdRef_t serverCmdPtr,
    ipcTest_SmallBitMask_t InValue
)
{
    le_event_QueueFunction(AsyncServer_NotSmallBitMaskRespond,
                           serverCmdPtr,
                           (void*)((uintptr_t)InValue));
}

void AsyncServer_NotLargeBitMaskRespond
(
    void* serverCmdPtr,
    void* valuePtr
)
{
    ipcTest_NotLargeBitMaskRespond(serverCmdPtr,
                                   ~*(ipcTest_LargeBitMask_t*)valuePtr);
    le_mem_Release(valuePtr);
}


void ipcTest_NotLargeBitMask
(
    ipcTest_ServerCmdRef_t serverCmdPtr,
    ipcTest_LargeBitMask_t InValue
)
{
    ipcTest_LargeBitMask_t* valuePtr = le_mem_AssertAlloc(ValuePool);
    *valuePtr = InValue;

    le_event_QueueFunction(AsyncServer_NotLargeBitMaskRespond,
                           serverCmdPtr,
                           valuePtr);
}

void AsyncServer_NotBooleanRespond
(
    void* serverCmdPtr,
    void* valuePtr
)
{
    ipcTest_NotBooleanRespond(serverCmdPtr, !(bool)valuePtr);
}

void ipcTest_NotBoolean
(
    ipcTest_ServerCmdRef_t serverCmdPtr,
    bool InValue
)
{
    le_event_QueueFunction(AsyncServer_NotBooleanRespond,
                           serverCmdPtr,
                           (void*) InValue);
}


void AsyncServer_NextResultRespond
(
    void* serverCmdPtr,
    void* valuePtr
)
{
    ipcTest_NextResultRespond(serverCmdPtr, util_IncResult((le_result_t)((intptr_t) valuePtr)));
}

void ipcTest_NextResult
(
    ipcTest_ServerCmdRef_t serverCmdPtr,
    le_result_t InValue
)
{
    le_event_QueueFunction(AsyncServer_NextResultRespond,
                           serverCmdPtr,
                           (void*) InValue);
}

void AsyncServer_ReturnNextResultRespond
(
    void* serverCmdPtr,
    void* valuePtr
)
{
    ipcTest_ReturnNextResultRespond(serverCmdPtr,
                                    util_IncResult((le_result_t)((intptr_t) valuePtr)));
}


void ipcTest_ReturnNextResult
(
    ipcTest_ServerCmdRef_t serverCmdPtr,
    le_result_t InValue
)
{
    le_event_QueueFunction(AsyncServer_ReturnNextResultRespond,
                           serverCmdPtr,
                           (void*) InValue);
}


void AsyncServer_NotOnOffRespond
(
    void* serverCmdPtr,
    void* valuePtr
)
{
    ipcTest_NotOnOffRespond(serverCmdPtr, util_NotOnOff((le_onoff_t)((uintptr_t)valuePtr)));
}

void ipcTest_NotOnOff
(
    ipcTest_ServerCmdRef_t serverCmdPtr,
    le_onoff_t InValue
)
{
    le_event_QueueFunction(AsyncServer_NotOnOffRespond,
                           serverCmdPtr,
                           (void*) InValue);
}


void AsyncServer_AddOneDoubleRespond
(
    void* serverCmdPtr,
    void* valuePtr
)
{
    ipcTest_AddOneDoubleRespond(serverCmdPtr, *(double*)valuePtr + 1);
    le_mem_Release(valuePtr);
}

void ipcTest_AddOneDouble
(
    ipcTest_ServerCmdRef_t serverCmdPtr,
    double InValue
)
{
    double* valuePtr = le_mem_AssertAlloc(ValuePool);
    *valuePtr = InValue;
    le_event_QueueFunction(AsyncServer_AddOneDoubleRespond,
                           serverCmdPtr,
                           (void*) valuePtr);
}

void AsyncServer_AddFourReferenceRespond
(
    void* serverCmdPtr,
    void* valuePtr
)
{
    if (valuePtr)
    {
        ipcTest_AddFourReferenceRespond(serverCmdPtr,
                                        (void *)((uint8_t *)valuePtr + 4));
    }
    else
    {
        ipcTest_AddFourReferenceRespond(serverCmdPtr,
                                        NULL);
    }
}


void ipcTest_AddFourReference
(
    ipcTest_ServerCmdRef_t serverCmdPtr,
    ipcTest_SimpleRef_t InRef
)
{
    le_event_QueueFunction(AsyncServer_AddFourReferenceRespond,
                           serverCmdPtr,
                           InRef);
}

void AsyncServer_ROT13StringRespond
(
    void* serverCmdPtr,
    void* valuePtr
)
{
    ipcTest_ROT13StringRespond(serverCmdPtr,
                               valuePtr);

    if (valuePtr)
    {
        le_mem_Release(valuePtr);
    }
}


void ipcTest_ROT13String
(
    ipcTest_ServerCmdRef_t serverCmdPtr,
    const char* InString,
    size_t OutStringSize
)
{
    // Cap output string size at maximum size of buffer
    if (OutStringSize > MAX_VALUE_SIZE) { OutStringSize = MAX_VALUE_SIZE; }

    char* OutString = NULL;

    OutString = le_mem_AssertAlloc(ValuePool);
    if (InString)
    {
        util_ROT13String(InString, OutString, MAX_VALUE_SIZE);
    }
    else
    {
        OutString[0] = '\0';
    }

    le_event_QueueFunction(AsyncServer_ROT13StringRespond,
                           serverCmdPtr,
                           OutString);
}

void AsyncServer_AddOneArrayRespond
(
    void* serverCmdPtr,
    void* valuePtr
)
{
    OutArrayInfo_t* arrayInfoPtr = (OutArrayInfo_t*) valuePtr;
    ipcTest_AddOneArrayRespond(serverCmdPtr, (int64_t*) arrayInfoPtr->arrayPtr,
                               arrayInfoPtr->outArraySize);
    le_mem_Release(arrayInfoPtr->arrayPtr);
    le_mem_Release(arrayInfoPtr);
}

void ipcTest_AddOneArray
(
    ipcTest_ServerCmdRef_t serverCmdPtr,
    const int64_t* InArrayPtr,
    size_t InArraySize,
    size_t OutArraySize
)
{
    uint64_t* OutArrayPtr = le_mem_AssertAlloc(ValuePool);
    size_t i;

    for (i = 0; i < InArraySize && i < OutArraySize; ++i)
    {
        OutArrayPtr[i] = InArrayPtr[i] + 1;
    }

    OutArrayInfo_t* arrayInfoPtr = le_mem_AssertAlloc(OutArrayInfoPool);
    arrayInfoPtr->arrayPtr = OutArrayPtr;
    arrayInfoPtr->outArraySize = i;

    le_event_QueueFunction(AsyncServer_AddOneArrayRespond,
                           serverCmdPtr,
                           arrayInfoPtr);
}

void AsyncServer_NotByteStringRespond
(
    void* serverCmdPtr,
    void* valuePtr
)
{
    OutArrayInfo_t* arrayInfoPtr = (OutArrayInfo_t*) valuePtr;
    ipcTest_NotByteStringRespond(serverCmdPtr, (uint8_t*) arrayInfoPtr->arrayPtr,
                                 arrayInfoPtr->outArraySize);
    le_mem_Release(arrayInfoPtr->arrayPtr);
    le_mem_Release(arrayInfoPtr);
}

void ipcTest_NotByteString
(
    ipcTest_ServerCmdRef_t serverCmdPtr,
    const uint8_t* InArrayPtr,
    size_t InArraySize,
    size_t OutArraySize
)
{
    char* OutArrayPtr = le_mem_AssertAlloc(ValuePool);
    size_t i;

    for (i = 0; i < InArraySize && i < OutArraySize; ++i)
    {
        OutArrayPtr[i] = ~InArrayPtr[i];
    }

    OutArrayInfo_t* arrayInfoPtr = le_mem_AssertAlloc(OutArrayInfoPool);
    arrayInfoPtr->arrayPtr = OutArrayPtr;
    arrayInfoPtr->outArraySize = i;

    le_event_QueueFunction(AsyncServer_NotByteStringRespond,
                           serverCmdPtr,
                           arrayInfoPtr);
}

void AsyncServer_AddOneROT13StructRespond
(
    void* serverCmdPtr,
    void* valuePtr
)
{
    ipcTest_TheStruct_t* outStructPtr = (ipcTest_TheStruct_t*) valuePtr;
    ipcTest_AddOneROT13StructRespond(serverCmdPtr, outStructPtr);
    le_mem_Release(outStructPtr);
}

void ipcTest_AddOneROT13Struct
(
    ipcTest_ServerCmdRef_t serverCmdPtr,
    const ipcTest_TheStruct_t * LE_NONNULL InStructPtr
)
{
    ipcTest_TheStruct_t* OutStructPtr = le_mem_AssertAlloc(TheStructPool);

    OutStructPtr->index = InStructPtr->index + 1;
    util_ROT13String(InStructPtr->name, OutStructPtr->name, sizeof(OutStructPtr->name));

    le_event_QueueFunction(AsyncServer_AddOneROT13StructRespond,
                           serverCmdPtr,
                           OutStructPtr);

}


void AsyncServer_AddOneROT13StructArrayRespond
(
    void* serverCmdPtr,
    void* valuePtr
)
{
    OutArrayInfo_t* arrayInfoPtr = (OutArrayInfo_t*) valuePtr;
    ipcTest_AddOneROT13StructArrayRespond(serverCmdPtr, arrayInfoPtr->arrayPtr,
                                          arrayInfoPtr->outArraySize);
    le_mem_Release(arrayInfoPtr->arrayPtr);
    le_mem_Release(arrayInfoPtr);
}

void ipcTest_AddOneROT13StructArray
(
    ipcTest_ServerCmdRef_t serverCmdPtr,
    const ipcTest_TheStruct_t* InStructArrayPtr,
    size_t InStructArraySize,
    size_t OutStructArraySize
)
{
    ipcTest_TheStruct_t* OutArrayPtr = le_mem_AssertAlloc(ValuePool);
    size_t i;

    for (i = 0; i < InStructArraySize && i < OutStructArraySize; ++i)
    {
        OutArrayPtr[i].index = InStructArrayPtr[i].index + 1;
        util_ROT13String(InStructArrayPtr[i].name,
                         OutArrayPtr[i].name, sizeof(OutArrayPtr[i].name));
    }

    OutArrayInfo_t* arrayInfoPtr = le_mem_AssertAlloc(OutArrayInfoPool);
    arrayInfoPtr->arrayPtr = OutArrayPtr;
    arrayInfoPtr->outArraySize = i;

    le_event_QueueFunction(AsyncServer_AddOneROT13StructArrayRespond,
                           serverCmdPtr,
                           arrayInfoPtr);

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
 * Storage for pointer to Complex event handlers
 */
static ipcTest_AddOneROT13HandlerFunc_t EventHandlerComplexPtr = NULL;
static void* ComplexEventContextPtr = NULL;
size_t ComplexEventRef=1;

ipcTest_AddOneROT13EventHandlerRef_t ipcTest_AddAddOneROT13EventHandler
(
    ipcTest_AddOneROT13HandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    // For simplicity, only allow a single event handler
    if (EventHandlerComplexPtr)
    {
        return NULL;
    }

    EventHandlerComplexPtr = handlerPtr;
    ComplexEventContextPtr = contextPtr;

    return (ipcTest_AddOneROT13EventHandlerRef_t)ComplexEventRef;
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
        EventHandlerComplexPtr = NULL;
        ComplexEventContextPtr = NULL;
    }
}

void AsyncServer_TriggerAddOneROT13EventRespond
(
    void* serverCmdPtr,
    void* context
)
{
    LE_UNUSED(context);
    ipcTest_TriggerAddOneROT13EventRespond(serverCmdPtr);
}


void ipcTest_TriggerAddOneROT13Event
(
    ipcTest_ServerCmdRef_t serverCmdPtr,
    int32_t cookie,
    const char* LE_NONNULL cookieString,
    const int16_t* cookieArrayPtr,
    size_t cookieArraySize
)
{
    if (EventHandlerComplexPtr)
    {
        char outString[16];
        int16_t outArray[10];
        size_t i;

        util_ROT13String(cookieString, outString, sizeof(outString));
        for (i = 0; i < cookieArraySize && i < NUM_ARRAY_MEMBERS(outArray); ++i)
        {
            outArray[i] = cookieArrayPtr[i] + 1;
        }

        EventHandlerComplexPtr(cookie + 1, outString, outArray, i,
                               ComplexEventContextPtr);
    }

    le_event_QueueFunction(AsyncServer_TriggerAddOneROT13EventRespond,
                           serverCmdPtr,
                           NULL);
}

/**
 * Storage for pointer to event handlers
 */
static ipcTest_AddOneHandlerFunc_t EventHandlerPtr = NULL;
static void* EventContextPtr = NULL;
size_t EventRef=1;

ipcTest_AddOneEventHandlerRef_t ipcTest_AddAddOneEventHandler
(
    ipcTest_AddOneHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
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
    // Remove if this is the current handler.
    if ((size_t)handlerRef == EventRef)
    {
        EventRef += 2;
        EventHandlerPtr = NULL;
        EventContextPtr = NULL;
    }
}

void AsyncServer_TriggerAddOneEventRespond
(
    void* serverCmdPtr,
    void* context
)
{
    LE_UNUSED(context);
    ipcTest_TriggerAddOneEventRespond(serverCmdPtr);
}

void ipcTest_TriggerAddOneEvent
(
    ipcTest_ServerCmdRef_t serverCmdPtr,
    int32_t cookie
)
{
    if (EventHandlerPtr)
    {
        EventHandlerPtr(cookie + 1, EventContextPtr);
    }
    le_event_QueueFunction(AsyncServer_TriggerAddOneEventRespond,
                           serverCmdPtr,
                           NULL);
}

COMPONENT_INIT
{
    ValuePool = le_mem_InitStaticPool(ValuePool, VALUE_ENTRIES, MAX_VALUE_SIZE);
    OutArrayInfoPool = le_mem_InitStaticPool(OutArrayInfoPool, VALUE_ENTRIES,
                                             sizeof(OutArrayInfo_t));
    TheStructPool = le_mem_InitStaticPool(TheStructPool, VALUE_ENTRIES,
                                          sizeof(ipcTest_TheStruct_t));
}
