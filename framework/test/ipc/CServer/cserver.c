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

#if 0
// Not currently supported on Java.
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
#endif

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

void ipcTest_EchoTriggerEvent
(
    int32_t cookie
        ///< [IN]
)
{
    if (EchoEventHandlerPtr)
    {
        EchoEventHandlerPtr(cookie, EchoEventContextPtr);
    }
}


COMPONENT_INIT
{
}
