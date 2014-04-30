/*
 * ====================== WARNING ======================
 *
 * THE CONTENTS OF THIS FILE HAVE BEEN AUTO-GENERATED.
 * DO NOT MODIFY IN ANY WAY.
 *
 * ====================== WARNING ======================
 */


#ifndef MCC_CALL_INTERFACE_H_INCLUDE_GUARD
#define MCC_CALL_INTERFACE_H_INCLUDE_GUARD


#include "legato.h"

// User customizable include file
#include "userInclude.h"


//--------------------------------------------------------------------------------------------------
/**
 * Start the service for the client main thread
 */
//--------------------------------------------------------------------------------------------------
void le_mcc_call_StartClient
(
    const char* serviceInstanceName
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * Stop the service for the current client thread
 */
//--------------------------------------------------------------------------------------------------
void le_mcc_call_StopClient
(
    void
);

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
void le_mcc_call_Delete
(
    le_mcc_call_Ref_t callRef
        ///< [IN]
        ///< The call object to free.
);

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
le_result_t le_mcc_call_Start
(
    le_mcc_call_Ref_t callRef
        ///< [IN]
        ///< Reference to the call object.
);

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
bool le_mcc_call_IsConnected
(
    le_mcc_call_Ref_t callRef
        ///< [IN]
        ///< The call reference to read.
);

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
le_result_t le_mcc_call_GetRemoteTel
(
    le_mcc_call_Ref_t callRef,
        ///< [IN]
        ///<  The call reference to read from.

    char* telPtr,
        ///< [OUT]
        ///< The telephone number string.

    size_t telPtrNumElements
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
le_mcc_call_TerminationReason_t le_mcc_call_GetTerminationReason
(
    le_mcc_call_Ref_t callRef
        ///< [IN]
        ///< The call reference to read from.
);

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
le_audio_StreamRef_t le_mcc_call_GetTxAudioStream
(
    le_mcc_call_Ref_t callRef
        ///< [IN]
        ///< The call reference to read from.
);

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
le_audio_StreamRef_t le_mcc_call_GetRxAudioStream
(
    le_mcc_call_Ref_t callRef
        ///< [IN]
        ///< The call reference to read from.
);

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
le_result_t le_mcc_call_Answer
(
    le_mcc_call_Ref_t callRef
        ///< [IN]
        ///< The call reference.
);

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
le_result_t le_mcc_call_HangUp
(
    le_mcc_call_Ref_t callRef
        ///< [IN]
        ///< The call to end.
);


#endif // MCC_CALL_INTERFACE_H_INCLUDE_GUARD

