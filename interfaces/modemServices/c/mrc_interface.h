/*
 * ====================== WARNING ======================
 *
 * THE CONTENTS OF THIS FILE HAVE BEEN AUTO-GENERATED.
 * DO NOT MODIFY IN ANY WAY.
 *
 * ====================== WARNING ======================
 */


#ifndef MRC_INTERFACE_H_INCLUDE_GUARD
#define MRC_INTERFACE_H_INCLUDE_GUARD


#include "legato.h"

// User customizable include file
#include "userInclude.h"


//--------------------------------------------------------------------------------------------------
/**
 * Start the client main thread
 */
//--------------------------------------------------------------------------------------------------
void le_mrc_StartClient
(
    const char* serviceInstanceName
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * Stop the service for the current client thread
 */
//--------------------------------------------------------------------------------------------------
void le_mrc_StopClient
(
    void
);


typedef void (*le_mrc_NetRegStateHandlerFunc_t)
(
    le_mrc_NetRegState_t state,
    void* contextPtr
);

typedef struct le_mrc_NetRegStateHandler* le_mrc_NetRegStateHandlerRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * This function adds a handler ...
 */
//--------------------------------------------------------------------------------------------------
le_mrc_NetRegStateHandlerRef_t le_mrc_AddNetRegStateHandler
(
    le_mrc_NetRegStateHandlerFunc_t handlerPtr,
        ///< [IN]

    void* contextPtr
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * This function removes a handler ...
 */
//--------------------------------------------------------------------------------------------------
void le_mrc_RemoveNetRegStateHandler
(
    le_mrc_NetRegStateHandlerRef_t addHandlerRef
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_GetNetRegState
(
    le_mrc_NetRegState_t* statePtr
        ///< [OUT]
);

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_GetSignalQual
(
    uint32_t* qualityPtr
        ///< [OUT]
);

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_SetRadioPower
(
    le_onoff_t power
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_GetRadioPower
(
    le_onoff_t* powerPtr
        ///< [OUT]
);


#endif // MRC_INTERFACE_H_INCLUDE_GUARD

