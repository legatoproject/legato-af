/*
 * ====================== WARNING ======================
 *
 * THE CONTENTS OF THIS FILE HAVE BEEN AUTO-GENERATED.
 * DO NOT MODIFY IN ANY WAY.
 *
 * ====================== WARNING ======================
 */


#ifndef MCC_PROFILE_INTERFACE_H_INCLUDE_GUARD
#define MCC_PROFILE_INTERFACE_H_INCLUDE_GUARD


#include "legato.h"

// User customizable include file
#include "userInclude.h"


//--------------------------------------------------------------------------------------------------
/**
 * Start the client main thread
 */
//--------------------------------------------------------------------------------------------------
void le_mcc_profile_StartClient
(
    const char* serviceInstanceName
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * Stop the service for the current client thread
 */
//--------------------------------------------------------------------------------------------------
void le_mcc_profile_StopClient
(
    void
);


typedef void (*le_mcc_profile_StateChangeHandlerFunc_t)
(
    le_mcc_profile_State_t newState,
    void* contextPtr
);


typedef void (*le_mcc_profile_CallEventHandlerFunc_t)
(
    le_mcc_call_Ref_t callRef,
    le_mcc_call_Event_t event,
    void* contextPtr
);

typedef struct le_mcc_profile_StateChangeHandler* le_mcc_profile_StateChangeHandlerRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * This function adds a handler ...
 */
//--------------------------------------------------------------------------------------------------
le_mcc_profile_StateChangeHandlerRef_t le_mcc_profile_AddStateChangeHandler
(
    le_mcc_profile_Ref_t profileRef,
        ///< [IN]
        ///< The profile reference.

    le_mcc_profile_StateChangeHandlerFunc_t handlerPtr,
        ///< [IN]

    void* contextPtr
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * This function removes a handler ...
 */
//--------------------------------------------------------------------------------------------------
void le_mcc_profile_RemoveStateChangeHandler
(
    le_mcc_profile_StateChangeHandlerRef_t addHandlerRef
        ///< [IN]
);

typedef struct le_mcc_profile_CallEventHandler* le_mcc_profile_CallEventHandlerRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * This function adds a handler ...
 */
//--------------------------------------------------------------------------------------------------
le_mcc_profile_CallEventHandlerRef_t le_mcc_profile_AddCallEventHandler
(
    le_mcc_profile_Ref_t profileRef,
        ///< [IN]
        ///< The profile to update.

    le_mcc_profile_CallEventHandlerFunc_t handlerPtr,
        ///< [IN]

    void* contextPtr
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * This function removes a handler ...
 */
//--------------------------------------------------------------------------------------------------
void le_mcc_profile_RemoveCallEventHandler
(
    le_mcc_profile_CallEventHandlerRef_t addHandlerRef
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
le_mcc_profile_Ref_t le_mcc_profile_GetByName
(
    const char* profileNamePtr
        ///< [IN]
        ///< The name of the profile to search for.
);

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
void le_mcc_profile_Release
(
    le_mcc_profile_Ref_t profileRef
        ///< [IN]
        ///< The Call profile reference.
);

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
le_mcc_profile_State_t le_mcc_profile_GetState
(
    le_mcc_profile_Ref_t profileRef
        ///< [IN]
        ///< The profile reference to read.
);

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
le_mcc_call_Ref_t le_mcc_profile_CreateCall
(
    le_mcc_profile_Ref_t profileRef,
        ///< [IN]
        ///< The profile to create a new call on.

    const char* destinationPtr
        ///< [IN]
        ///< The target number we are going to call.
);


#endif // MCC_PROFILE_INTERFACE_H_INCLUDE_GUARD

