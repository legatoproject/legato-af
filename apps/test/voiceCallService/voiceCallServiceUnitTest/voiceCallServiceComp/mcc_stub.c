/**
 * This module implements some stubs for the voice call service unit tests.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Semaphore timeout in seconds
 */
//--------------------------------------------------------------------------------------------------
#define SEMAPHORE_TIMEOUT    5

//--------------------------------------------------------------------------------------------------
/**
 * Event for new MCC call state
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t MccCallEventId = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * MCC termination reason
 */
//--------------------------------------------------------------------------------------------------
static le_mcc_TerminationReason_t TermReason = LE_MCC_TERM_UNDEFINED;

//--------------------------------------------------------------------------------------------------
/**
 * Remote Telephone number
 */
//--------------------------------------------------------------------------------------------------
static char RemotePhoneNum[LE_MDMDEFS_PHONE_NUM_MAX_BYTES] = {0};

//--------------------------------------------------------------------------------------------------
/**
 * MCC Voice call context profile.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_mcc_CallRef_t callRef;
    le_mcc_Event_t   callEvent;
} MccContext_t;

//--------------------------------------------------------------------------------------------------
/**
 * Dummy MCC profile
 */
//--------------------------------------------------------------------------------------------------
static MccContext_t MccCtx =
{
    (le_mcc_CallRef_t) 0x10000004,
    LE_MCC_EVENT_TERMINATED
};

//--------------------------------------------------------------------------------------------------
/**
 * Semaphore for event synchronization.
 */
//--------------------------------------------------------------------------------------------------
static le_sem_Ref_t MccSemaphore;


//--------------------------------------------------------------------------------------------------
/**
 * Simulate a new MCC call event
 */
//--------------------------------------------------------------------------------------------------
void le_mccTest_SimulateState
(
    le_mcc_Event_t event
)
{
    MccContext_t *mccCtxPtr = &MccCtx;
    mccCtxPtr->callEvent = event;

    // Check if event is created before using it
    if (MccCallEventId)
    {
        // Notify all the registered client handlers
        le_event_Report(MccCallEventId, mccCtxPtr, sizeof(MccContext_t));
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Synchronization function to wait for le_mcc_Start().
 */
//--------------------------------------------------------------------------------------------------
void le_mccTest_SimulateWaitMccStart
(
    void
)
{
    le_clk_Time_t timeToWait = {SEMAPHORE_TIMEOUT, 0};
    LE_ASSERT_OK(le_sem_WaitWithTimeOut(MccSemaphore, timeToWait));
}

//--------------------------------------------------------------------------------------------------
/**
 * Simulate Termination Reason
 */
//--------------------------------------------------------------------------------------------------
void le_mccTest_SimulateTerminationReason
(
    le_mcc_TerminationReason_t termination
)
{
    TermReason = termination;
}

//--------------------------------------------------------------------------------------------------
/**
 * le_mcc_GetRemoteTel() stub.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mcc_GetRemoteTel
(
    le_mcc_CallRef_t callRef,    ///< [IN]  The call reference to read from.
    char*                telPtr,     ///< [OUT] The telephone number string.
    size_t               len         ///< [IN]  The length of telephone number string.
)
{
    if (LE_MDMDEFS_PHONE_NUM_MAX_BYTES <= len)
    {
        return le_utf8_Copy(telPtr, RemotePhoneNum, LE_MDMDEFS_PHONE_NUM_MAX_BYTES, NULL);
    }
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * le_mcc_Start() stub.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mcc_Start
(
    le_mcc_CallRef_t callRef   ///< [IN] Reference to the call object.
)
{
    // Post the mcc semaphore.
    le_sem_Post(MccSemaphore);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *  le_mcc_Answer() stub.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mcc_Answer
(
    le_mcc_CallRef_t callRef   ///< [IN] The call reference.
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * The first-layer New Session State Change Handler.
 */
//--------------------------------------------------------------------------------------------------
static void FirstLayerStateHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    MccContext_t* MccCtxPtr = reportPtr;
    le_mcc_CallEventHandlerFunc_t clientHandlerFunc = secondLayerHandlerFunc;

    clientHandlerFunc(  MccCtxPtr->callRef,
                        MccCtxPtr->callEvent,
                        le_event_GetContextPtr()
                     );
}

//--------------------------------------------------------------------------------------------------
/**
 * le_mcc_AddCallEventHandler() stub.
 *
 */
//--------------------------------------------------------------------------------------------------
le_mcc_CallEventHandlerRef_t le_mcc_AddCallEventHandler
(
    le_mcc_CallEventHandlerFunc_t       handlerFuncPtr, ///< [IN] The event handler function.
    void*                               contextPtr      ///< [IN] The handlers context.
)
{
    le_event_HandlerRef_t        handlerRef;

    if (NULL == handlerFuncPtr)
    {
        LE_ERROR("Handler function is NULL !");
        return NULL;
    }

    // Create an event Id for new Network Registration State notification if not already done
    if (!MccCallEventId)
    {
        MccCallEventId = le_event_CreateId("MccCallEvent", sizeof(MccContext_t));
    }

    handlerRef = le_event_AddLayeredHandler("MccCallEventHandler",
                                         MccCallEventId,
                                         FirstLayerStateHandler,
                                         (le_event_HandlerFunc_t)handlerFuncPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (le_mcc_CallEventHandlerRef_t)(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * le_mcc_GetTerminationReason() stub.
 *
 */
//--------------------------------------------------------------------------------------------------
le_mcc_TerminationReason_t le_mcc_GetTerminationReason
(
    le_mcc_CallRef_t callRef   ///< [IN] The call reference to read from.
)
{
   return TermReason;
}

//--------------------------------------------------------------------------------------------------
/**
 * le_mcc_Create() stub.
 *
 */
//--------------------------------------------------------------------------------------------------
le_mcc_CallRef_t le_mcc_Create
(
    const char* phoneNumPtr
        ///< [IN]
        ///< The target number we are going to
        ///< call.
)
{
    le_utf8_Copy(RemotePhoneNum, phoneNumPtr, LE_MDMDEFS_PHONE_NUM_MAX_BYTES, NULL);
    return MccCtx.callRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * le_mcc_RemoveCallEventHandler() stub.
 *
 */
//--------------------------------------------------------------------------------------------------
void le_mcc_RemoveCallEventHandler
(
    le_mcc_CallEventHandlerRef_t handlerRef   ///< [IN] The handler object to remove.
)
{
   LE_INFO("Clear Call Event handler %p", handlerRef);
   le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * le_mcc_Delete() stub.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mcc_Delete
(
    le_mcc_CallRef_t callRef   ///< [IN] The call object to free.
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * le_mcc_HangUp() stub.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mcc_HangUp
(
    le_mcc_CallRef_t callRef   ///< [IN] The call to end.
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * le_mcc_Init() stub.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mcc_Init
(
    void
)
{
    // Create semaphore for le_mcc_Start synchronization.
    MccSemaphore = le_sem_Create("mccSemaphore", 0);
    return LE_OK;
}
