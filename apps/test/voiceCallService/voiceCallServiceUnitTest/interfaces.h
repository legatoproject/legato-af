/**
 * interfaces.h
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#ifndef _INTERFACES_H
#define _INTERFACES_H

#include "le_voicecall_interface.h"

#undef LE_KILL_CLIENT
#define LE_KILL_CLIENT LE_ERROR


//--------------------------------------------------------------------------------------------------
/**
 *  Reference type for managing active calls.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_mcc_Call* le_mcc_CallRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * Reference type used by Add/Remove functions for EVENT 'le_mcc_CallEvent'
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_mcc_CallEventHandler* le_mcc_CallEventHandlerRef_t;

//--------------------------------------------------------------------------------------------------
/**
 *  Enumeration of the possible reasons for call termination.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_MCC_TERM_LOCAL_ENDED = 0,
        ///< Local party ended the call (Normal Call Clearing).
    LE_MCC_TERM_REMOTE_ENDED = 1,
        ///< Remote party ended the call (Normal Call Clearing).
    LE_MCC_TERM_NETWORK_FAIL = 2,
        ///< Network could not complete the call.
    LE_MCC_TERM_UNASSIGNED_NUMBER = 3,
        ///< cf. 3GPP 24.008 Annex H
    LE_MCC_TERM_NO_ROUTE_TO_DESTINATION = 4,
        ///< cf. 3GPP 24.008 Annex H
    LE_MCC_TERM_CHANNEL_UNACCEPTABLE = 5,
        ///< cf. 3GPP 24.008 Annex H
    LE_MCC_TERM_OPERATOR_DETERMINED_BARRING = 6,
        ///< cf. 3GPP 24.008 Annex H
    LE_MCC_TERM_USER_BUSY = 7,
        ///< cf. 3GPP 24.008 Annex H
    LE_MCC_TERM_NO_USER_RESPONDING = 8,
        ///< cf. 3GPP 24.008 Annex H
    LE_MCC_TERM_USER_ALERTING_NO_ANSWER = 9,
        ///< cf. 3GPP 24.008 Annex H
    LE_MCC_TERM_CALL_REJECTED = 10,
        ///< cf. 3GPP 24.008 Annex H
    LE_MCC_TERM_NUMBER_CHANGED = 11,
        ///< cf. 3GPP 24.008 Annex H
    LE_MCC_TERM_PREEMPTION = 12,
        ///< cf. 3GPP 24.008 Annex H
    LE_MCC_TERM_DESTINATION_OUT_OF_ORDER = 13,
        ///< cf. 3GPP 24.008 Annex H
    LE_MCC_TERM_INVALID_NUMBER_FORMAT = 14,
        ///< cf. 3GPP 24.008 Annex H
    LE_MCC_TERM_FACILITY_REJECTED = 15,
        ///< cf. 3GPP 24.008 Annex H
    LE_MCC_TERM_RESP_TO_STATUS_ENQUIRY = 16,
        ///< cf. 3GPP 24.008 Annex H
    LE_MCC_TERM_NORMAL_UNSPECIFIED = 17,
        ///< cf. 3GPP 24.008 Annex H
    LE_MCC_TERM_NO_CIRCUIT_OR_CHANNEL_AVAILABLE = 18,
        ///< cf. 3GPP 24.008 Annex H
    LE_MCC_TERM_NETWORK_OUT_OF_ORDER = 19,
        ///< cf. 3GPP 24.008 Annex H
    LE_MCC_TERM_TEMPORARY_FAILURE = 20,
        ///< cf. 3GPP 24.008 Annex H
    LE_MCC_TERM_SWITCHING_EQUIPMENT_CONGESTION = 21,
        ///< cf. 3GPP 24.008 Annex H
    LE_MCC_TERM_ACCESS_INFORMATION_DISCARDED = 22,
        ///< cf. 3GPP 24.008 Annex H
    LE_MCC_TERM_REQUESTED_CIRCUIT_OR_CHANNEL_NOT_AVAILABLE = 23,
        ///< cf. 3GPP 24.008 Annex H
    LE_MCC_TERM_RESOURCES_UNAVAILABLE_OR_UNSPECIFIED = 24,
        ///< cf. 3GPP 24.008 Annex H
    LE_MCC_TERM_QOS_UNAVAILABLE = 25,
        ///< cf. 3GPP 24.008 Annex H
    LE_MCC_TERM_REQUESTED_FACILITY_NOT_SUBSCRIBED = 26,
        ///< cf. 3GPP 24.008 Annex H
    LE_MCC_TERM_INCOMING_CALLS_BARRED_WITHIN_CUG = 27,
        ///< cf. 3GPP 24.008 Annex H
    LE_MCC_TERM_BEARER_CAPABILITY_NOT_AUTH = 28,
        ///< cf. 3GPP 24.008 Annex H
    LE_MCC_TERM_BEARER_CAPABILITY_UNAVAILABLE = 29,
        ///< cf. 3GPP 24.008 Annex H
    LE_MCC_TERM_SERVICE_OPTION_NOT_AVAILABLE = 30,
        ///< cf. 3GPP 24.008 Annex H
    LE_MCC_TERM_ACM_LIMIT_EXCEEDED = 31,
        ///< cf. 3GPP 24.008 Annex H
    LE_MCC_TERM_BEARER_SERVICE_NOT_IMPLEMENTED = 32,
        ///< cf. 3GPP 24.008 Annex H
    LE_MCC_TERM_REQUESTED_FACILITY_NOT_IMPLEMENTED = 33,
        ///< cf. 3GPP 24.008 Annex H
    LE_MCC_TERM_ONLY_DIGITAL_INFORMATION_BEARER_AVAILABLE = 34,
        ///< cf. 3GPP 24.008 Annex H
    LE_MCC_TERM_SERVICE_OR_OPTION_NOT_IMPLEMENTED = 35,
        ///< cf. 3GPP 24.008 Annex H
    LE_MCC_TERM_INVALID_TRANSACTION_IDENTIFIER = 36,
        ///< cf. 3GPP 24.008 Annex H
    LE_MCC_TERM_USER_NOT_MEMBER_OF_CUG = 37,
        ///< cf. 3GPP 24.008 Annex H
    LE_MCC_TERM_INCOMPATIBLE_DESTINATION = 38,
        ///< cf. 3GPP 24.008 Annex H
    LE_MCC_TERM_INVALID_TRANSIT_NW_SELECTION = 39,
        ///< cf. 3GPP 24.008 Annex H
    LE_MCC_TERM_SEMANTICALLY_INCORRECT_MESSAGE = 40,
        ///< cf. 3GPP 24.008 Annex H
    LE_MCC_TERM_INVALID_MANDATORY_INFORMATION = 41,
        ///< cf. 3GPP 24.008 Annex H
    LE_MCC_TERM_MESSAGE_TYPE_NON_IMPLEMENTED = 42,
        ///< cf. 3GPP 24.008 Annex H
    LE_MCC_TERM_MESSAGE_TYPE_NOT_COMPATIBLE_WITH_PROTOCOL_STATE = 43,
        ///< cf. 3GPP 24.008 Annex H
    LE_MCC_TERM_INFORMATION_ELEMENT_NON_EXISTENT = 44,
        ///< cf. 3GPP 24.008 Annex H
    LE_MCC_TERM_CONDITONAL_IE_ERROR = 45,
        ///< cf. 3GPP 24.008 Annex H
    LE_MCC_TERM_MESSAGE_NOT_COMPATIBLE_WITH_PROTOCOL_STATE = 46,
        ///< cf. 3GPP 24.008 Annex H
    LE_MCC_TERM_RECOVERY_ON_TIMER_EXPIRY = 47,
        ///< cf. 3GPP 24.008 Annex H
    LE_MCC_TERM_PROTOCOL_ERROR_UNSPECIFIED = 48,
        ///< cf. 3GPP 24.008 Annex H
    LE_MCC_TERM_INTERWORKING_UNSPECIFIED = 49,
        ///< cf. 3GPP 24.008 Annex H
    LE_MCC_TERM_SERVICE_TEMPORARILY_OUT_OF_ORDER = 50,
        ///< cf. 3GPP 24.008 10.5.3.6
    LE_MCC_TERM_NOT_ALLOWED = 51,
        ///< Call operations not allowed
        ///<  (i.e. Radio off).
    LE_MCC_TERM_FDN_ACTIVE = 52,
        ///< FDN is active and number is not
        ///< in the FDN.
    LE_MCC_TERM_NO_SERVICE = 53,
        ///< No service or bad signal quality
    LE_MCC_TERM_PLATFORM_SPECIFIC = 54,
        ///< Platform specific code.
    LE_MCC_TERM_UNDEFINED = 55
        ///< Undefined reason.
}
le_mcc_TerminationReason_t;

//--------------------------------------------------------------------------------------------------
/**
 *  Enumeration of the possible events that may be reported to a call event handler.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_MCC_EVENT_SETUP = 0,
        ///< Call is being set up.
    LE_MCC_EVENT_INCOMING = 1,
        ///< Incoming call attempt (new call).
    LE_MCC_EVENT_ORIGINATING = 2,
        ///< Outgoing call attempt.
    LE_MCC_EVENT_ALERTING = 3,
        ///< Far end is now alerting its user (outgoing call).
    LE_MCC_EVENT_CONNECTED = 4,
        ///< Call has been established, and is media is active.
    LE_MCC_EVENT_TERMINATED = 5,
        ///< Call has terminated.
    LE_MCC_EVENT_WAITING = 6,
        ///< Call is waiting
    LE_MCC_EVENT_ON_HOLD = 7,
        ///< Remote party has put the call on hold.
    LE_MCC_EVENT_MAX = 8
        ///< Enumerate max value.
}
le_mcc_Event_t;

//--------------------------------------------------------------------------------------------------
/**
 * Handler for call state changes.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef void (*le_mcc_CallEventHandlerFunc_t)
(
    le_mcc_CallRef_t callRef,
        ///< The call reference.
    le_mcc_Event_t event,
        ///< Call event.
    void* contextPtr
        ///<
);

//--------------------------------------------------------------------------------------------------
/**
 * Simulate a new MCC call event
 */
//--------------------------------------------------------------------------------------------------
void le_mccTest_SimulateState
(
    le_mcc_Event_t event
);

//--------------------------------------------------------------------------------------------------
/**
 * Simulate Termination Reason
 */
//--------------------------------------------------------------------------------------------------
void le_mccTest_SimulateTerminationReason
(
    le_mcc_TerminationReason_t Termination
);

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
);

//--------------------------------------------------------------------------------------------------
/**
 * le_mcc_Start() stub.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mcc_Start
(
    le_mcc_CallRef_t callRef   ///< [IN] Reference to the call object.
);

//--------------------------------------------------------------------------------------------------
/**
 *  le_mcc_Answer() stub.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mcc_Answer
(
    le_mcc_CallRef_t callRef   ///< [IN] The call reference.
);

//--------------------------------------------------------------------------------------------------
/**
 * le_mcc_AddCallEventHandler() stub.
 *
 */
//--------------------------------------------------------------------------------------------------
le_mcc_CallEventHandlerRef_t le_mcc_AddCallEventHandler
(
    le_mcc_CallEventHandlerFunc_t       handlerFuncPtr, ///< [IN] The event handler function.
    void*                                   contextPtr      ///< [IN] The handlers context.
);

//--------------------------------------------------------------------------------------------------
/**
 * le_mcc_GetTerminationReason() stub.
 *
 */
//--------------------------------------------------------------------------------------------------
le_mcc_TerminationReason_t le_mcc_GetTerminationReason
(
    le_mcc_CallRef_t callRef   ///< [IN] The call reference to read from.
);

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
);

//--------------------------------------------------------------------------------------------------
/**
 * le_mcc_RemoveCallEventHandler() stub.
 *
 */
//--------------------------------------------------------------------------------------------------
void le_mcc_RemoveCallEventHandler
(
    le_mcc_CallEventHandlerRef_t handlerRef   ///< [IN] The handler object to remove.
);

//--------------------------------------------------------------------------------------------------
/**
 * le_mcc_Delete() stub.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mcc_Delete
(
    le_mcc_CallRef_t callRef   ///< [IN] The call object to free.
);

//--------------------------------------------------------------------------------------------------
/**
 * le_mcc_HangUp() stub.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mcc_HangUp
(
    le_mcc_CallRef_t callRef   ///< [IN] The call to end.
);

//--------------------------------------------------------------------------------------------------
/**
 * le_audio_OpenModemVoiceTx() stub.
 *
 */
//--------------------------------------------------------------------------------------------------
le_audio_StreamRef_t le_audio_OpenModemVoiceTx
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * le_audio_OpenModemVoiceRx() stub.
 *
 */
//--------------------------------------------------------------------------------------------------
le_audio_StreamRef_t le_audio_OpenModemVoiceRx
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the server service reference
 */
//--------------------------------------------------------------------------------------------------
le_msg_ServiceRef_t le_voicecall_GetServiceRef
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the client session reference for the current message
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionRef_t le_voicecall_GetClientSessionRef
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the server and advertise the service.
 */
//--------------------------------------------------------------------------------------------------
void le_voicecall_AdvertiseService
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * SetServiceRecvHandler stub
 *
 */
//--------------------------------------------------------------------------------------------------
void MySetServiceRecvHandler
(
    le_msg_ServiceRef_t     serviceRef, ///< [in] Reference to the service.
    le_msg_ReceiveHandler_t handlerFunc,///< [in] Handler function.
    void*                   contextPtr  ///< [in] Opaque pointer value to pass to the handler.
);

//--------------------------------------------------------------------------------------------------
/**
 * Add service open handler stub
 *
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionEventHandlerRef_t MyAddServiceOpenHandler
(
    le_msg_ServiceRef_t serviceRef,
    le_msg_SessionEventHandler_t handlerFunc,
    void *contextPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Add service close handler stub
 *
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionEventHandlerRef_t MyAddServiceCloseHandler
(
    le_msg_ServiceRef_t serviceRef,
    le_msg_SessionEventHandler_t handlerFunc,
    void *contextPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Synchronization function to wait for le_mcc_Start().
 */
//--------------------------------------------------------------------------------------------------
void le_mccTest_SimulateWaitMccStart
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * le_mcc_Init() stub.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mcc_Init
(
    void
);

#endif /* interfaces.h */
