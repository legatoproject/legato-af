/*
 * ====================== WARNING ======================
 *
 * THE CONTENTS OF THIS FILE HAVE BEEN AUTO-GENERATED.
 * DO NOT MODIFY IN ANY WAY.
 *
 * ====================== WARNING ======================
 */


#ifndef POS_SAMPLE_INTERFACE_H_INCLUDE_GUARD
#define POS_SAMPLE_INTERFACE_H_INCLUDE_GUARD


#include "legato.h"

// User customizable include file
#include "posUserInclude.h"


//--------------------------------------------------------------------------------------------------
/**
 * Start the service for the client main thread
 */
//--------------------------------------------------------------------------------------------------
void le_pos_sample_StartClient
(
    const char* serviceInstanceName
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * Stop the service for the current client thread
 */
//--------------------------------------------------------------------------------------------------
void le_pos_sample_StopClient
(
    void
);

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
le_result_t le_pos_sample_Get2DLocation
(
    le_pos_SampleRef_t positionSampleRef,
        ///< [IN]

    int32_t* latitudePtr,
        ///< [OUT]

    int32_t* longitudePtr,
        ///< [OUT]

    int32_t* horizontalAccuracyPtr
        ///< [OUT]
);

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
le_result_t le_pos_sample_GetAltitude
(
    le_pos_SampleRef_t positionSampleRef,
        ///< [IN]

    int32_t* altitudePtr,
        ///< [OUT]

    int32_t* altitudeAccuracyPtr
        ///< [OUT]
);

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
le_result_t le_pos_sample_GetHorizontalSpeed
(
    le_pos_SampleRef_t positionSampleRef,
        ///< [IN]

    uint32_t* hspeedPtr,
        ///< [OUT]

    int32_t* hspeedAccuracyPtr
        ///< [OUT]
);

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
le_result_t le_pos_sample_GetVerticalSpeed
(
    le_pos_SampleRef_t positionSampleRef,
        ///< [IN]

    int32_t* vspeedPtr,
        ///< [OUT]

    int32_t* vspeedAccuracyPtr
        ///< [OUT]
);

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
le_result_t le_pos_sample_GetHeading
(
    le_pos_SampleRef_t positionSampleRef,
        ///< [IN]

    int32_t* headingPtr,
        ///< [OUT]

    int32_t* headingAccuracyPtr
        ///< [OUT]
);

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
le_result_t le_pos_sample_GetDirection
(
    le_pos_SampleRef_t positionSampleRef,
        ///< [IN]

    int32_t* directionPtr,
        ///< [OUT]

    int32_t* directionAccuracyPtr
        ///< [OUT]
);

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
void le_pos_sample_Release
(
    le_pos_SampleRef_t positionSampleRef
        ///< [IN]
);


#endif // POS_SAMPLE_INTERFACE_H_INCLUDE_GUARD

