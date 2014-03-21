/*
 * ====================== WARNING ======================
 *
 * THE CONTENTS OF THIS FILE HAVE BEEN AUTO-GENERATED.
 * DO NOT MODIFY IN ANY WAY.
 *
 * ====================== WARNING ======================
 */


#ifndef POS_INTERFACE_H_INCLUDE_GUARD
#define POS_INTERFACE_H_INCLUDE_GUARD


#include "legato.h"

// User customizable include file
#include "posUserInclude.h"


//--------------------------------------------------------------------------------------------------
/**
 * Start the client main thread
 */
//--------------------------------------------------------------------------------------------------
void le_pos_StartClient
(
    const char* serviceInstanceName
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * Stop the service for the current client thread
 */
//--------------------------------------------------------------------------------------------------
void le_pos_StopClient
(
    void
);


typedef void (*le_pos_MovementHandlerFunc_t)
(
    le_pos_SampleRef_t positionSampleRef,
    void* contextPtr
);

typedef struct le_pos_MovementHandler* le_pos_MovementHandlerRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * This function adds a handler ...
 */
//--------------------------------------------------------------------------------------------------
le_pos_MovementHandlerRef_t le_pos_AddMovementHandler
(
    uint32_t horizontalMagnitude,
        ///< [IN]

    uint32_t verticalMagnitude,
        ///< [IN]

    le_pos_MovementHandlerFunc_t handlerPtr,
        ///< [IN]

    void* contextPtr
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * This function removes a handler ...
 */
//--------------------------------------------------------------------------------------------------
void le_pos_RemoveMovementHandler
(
    le_pos_MovementHandlerRef_t addHandlerRef
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
le_result_t le_pos_Get2DLocation
(
    int32_t* latitudePtr,
        ///< [OUT]

    int32_t* longitudePtr,
        ///< [OUT]

    int32_t* hAccuracyPtr
        ///< [OUT]
);

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
le_result_t le_pos_Get3DLocation
(
    int32_t* latitudePtr,
        ///< [OUT]

    int32_t* longitudePtr,
        ///< [OUT]

    int32_t* hAccuracyPtr,
        ///< [OUT]

    int32_t* altitudePtr,
        ///< [OUT]

    int32_t* vAccuracyPtr
        ///< [OUT]
);

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
le_result_t le_pos_GetMotion
(
    uint32_t* hSpeedPtr,
        ///< [OUT]

    int32_t* hSpeedAccuracyPtr,
        ///< [OUT]

    int32_t* vSpeedPtr,
        ///< [OUT]

    int32_t* vSpeedAccuracyPtr
        ///< [OUT]
);

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
le_result_t le_pos_GetHeading
(
    int32_t* headingPtr,
        ///< [OUT]

    int32_t* headingAccuracyPtr
        ///< [OUT]
);

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
le_result_t le_pos_GetDirection
(
    int32_t* directionPtr,
        ///< [OUT]

    int32_t* directionAccuracyPtr
        ///< [OUT]
);


#endif // POS_INTERFACE_H_INCLUDE_GUARD

