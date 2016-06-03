/**
 * @file pa_mcc_simu.c
 *
 * Simulation implementation of @ref c_pa_mcc API.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "pa_mcc.h"

#define CURRENT_CALL_ID     1

//--------------------------------------------------------------------------------------------------
/**
 * Call event ID used to report call events to the registered event handlers.
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t CallEventId;

//--------------------------------------------------------------------------------------------------
/**
 * Memory pool for call event data.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t CallEventDataPool;

//--------------------------------------------------------------------------------------------------
/**
 * The user's event handler reference.
 */
//--------------------------------------------------------------------------------------------------
static le_event_HandlerRef_t CallEventHandlerRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Voice dial result.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t VoiceDialResult = LE_OK;

//--------------------------------------------------------------------------------------------------
/**
 * Call waiting status
 */
//--------------------------------------------------------------------------------------------------
static bool CallWaitingStatus = LE_OFF;

//--------------------------------------------------------------------------------------------------
/**
 * simu init
 *
 **/
//--------------------------------------------------------------------------------------------------
le_result_t mcc_simu_Init
(
    void
)
{
    // Create the event for signaling user handlers.
    CallEventId = le_event_CreateIdWithRefCounting("CallEvent");
    CallEventDataPool = le_mem_CreatePool("CallEventDataPool", sizeof(pa_mcc_CallEventData_t));

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Report the Call event
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_mccSimu_ReportCallEvent
(
    const char*     phoneNumPtr, ///< [IN] The simulated telephone number
    le_mcc_Event_t  event        ///< [IN] The simulated call eventDataPtr
)
{
    LE_INFO("Report call event.%d", event);

    pa_mcc_CallEventData_t* eventDataPtr = le_mem_ForceAlloc(CallEventDataPool);
    eventDataPtr->callId = CURRENT_CALL_ID;
    eventDataPtr->event = event;
    le_utf8_Copy(eventDataPtr->phoneNumber, phoneNumPtr, sizeof(eventDataPtr->phoneNumber), NULL);

    le_event_ReportWithRefCounting(CallEventId, eventDataPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Report the Call termination reason
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_mccSimu_ReportCallTerminationReason
(
    const char*     phoneNumPtr, ///< [IN] The simulated telephone number
    le_mcc_TerminationReason_t  term,
    int32_t                     termCode
)
{
    LE_INFO("Report termination reason.%d, platform specific code.0x%X", term, termCode);

    pa_mcc_CallEventData_t* eventDataPtr = le_mem_ForceAlloc(CallEventDataPool);
    eventDataPtr->callId = CURRENT_CALL_ID;
    eventDataPtr->event = LE_MCC_EVENT_TERMINATED;
    le_utf8_Copy(eventDataPtr->phoneNumber, phoneNumPtr, sizeof(eventDataPtr->phoneNumber), NULL);
    eventDataPtr->terminationEvent = term;
    eventDataPtr->terminationCode = termCode;

    le_event_ReportWithRefCounting(CallEventId, eventDataPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register a handler for Call event notifications.
 *
 * @return
 *  - LE_FAULT         The function failed to register the handler.
 *  - LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mcc_SetCallEventHandler
(
    pa_mcc_CallEventHandlerFunc_t   handlerFuncPtr ///< [IN] The event handler function.
)
{

    LE_INFO("Set new Call Event handler.");

    LE_FATAL_IF(handlerFuncPtr == NULL, "The new Call Event handler is NULL.");

    CallEventHandlerRef = le_event_AddHandler("CallEventHandler",
                                              CallEventId,
                                              (le_event_HandlerFunc_t)handlerFuncPtr);

    if (CallEventHandlerRef)
    {
        return LE_OK;
    }
    else
    {
        return LE_FAULT;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to unregister the handler for incoming calls handling.
 *
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_mcc_ClearCallEventHandler
(
    void
)
{
    LE_INFO("Clear Call Event handler %p", CallEventHandlerRef);
    le_event_RemoveHandler(CallEventHandlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function sets the voice dial status.
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_mccSimu_SetVoiceDialResult
(
    le_result_t voiceDialResult
)
{
    VoiceDialResult = voiceDialResult;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set a voice call.
 *
 * @return
 *  - LE_FAULT        The function failed to set the call.
 *  - LE_OK           The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mcc_VoiceDial
(
    const char*    phoneNumberPtr,          ///< [IN] The phone number.
    pa_mcc_clir_t  clir,                    ///< [IN] The CLIR supplementary service subscription.
    pa_mcc_cug_t   cug,                     ///< [IN] The CUG supplementary service information.
    uint8_t*       callIdPtr,               ///< [OUT] The outgoing call ID.
    le_mcc_TerminationReason_t*  errorPtr   ///< [OUT] Call termination error.
)
{
    *callIdPtr = CURRENT_CALL_ID;
    return VoiceDialResult;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to answer a call.
 *
 * @return
 *  - LE_FAULT        The function failed to answer a call.
 *  - LE_COMM_ERROR   The communication device has returned an error.
 *  - LE_TIMEOUT      No response was received from the Modem.
 *  - LE_OK           The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mcc_Answer
(
    uint8_t  callId     ///< [IN] The call ID to answer
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to disconnect the remote user.
 *
 * @return
 *  - LE_FAULT        The function failed to disconnect the remote user.
 *  - LE_COMM_ERROR   The communication device has returned an error.
 *  - LE_TIMEOUT      No response was received from the Modem.
 *  - LE_OK           The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mcc_HangUp
(
    uint8_t  callId     ///< [IN] The call ID to hangup
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to end all the ongoing calls.
 *
 * @return LE_NOT_POSSIBLE  The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mcc_HangUpAll
(
    void
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function activates or deactivates the call waiting service.
 *
 * @return
 *     - LE_OK        The function succeed.
 *     - LE_FAULT     The function failed.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mcc_SetCallWaitingService
(
    bool active
        ///< [IN] The call waiting activation.
)
{
    CallWaitingStatus = active;
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function gets the call waiting service status.
 *
 * @return
 *     - LE_OK        The function succeed.
 *     - LE_FAULT     The function failed.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mcc_GetCallWaitingService
(
    bool* activePtr
        ///< [OUT] The call waiting activation.
)
{
    *activePtr = CallWaitingStatus;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function activates the specified call. Other calls are placed on hold.
 *
 * @return
 *     - LE_OK        The function succeed.
 *     - LE_FAULT     The function failed.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mcc_ActivateCall
(
    uint8_t  callId     ///< [IN] The active call ID
)
{
    return LE_OK;
}

