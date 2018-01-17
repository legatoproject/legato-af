//--------------------------------------------------------------------------------------------------
/**
 * @file le_antenna.c
 *
 * This file contains the source code of the Monitoring Antenna API.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "pa_antenna.h"
#include "le_antenna_local.h"

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
// Data structures.
//--------------------------------------------------------------------------------------------------

typedef struct
{
    le_antenna_Type_t               antennaType;              // Type of the monitored antenna
    le_antenna_ObjRef_t             antennaRef;               // Antenna reference
    le_event_Id_t                   statusEventId;            // Event identifier to report a status
                                                              // modification
    le_event_HandlerRef_t           statusEventHandlerRef;    // event handler reference
} AntennaCtx_t;

//--------------------------------------------------------------------------------------------------
//                                       Static declarations
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for the antenna reference
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t AntennaRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Context for antenna monitoring
 *
 */
//--------------------------------------------------------------------------------------------------
static AntennaCtx_t AntennaCtx[LE_ANTENNA_MAX];

//--------------------------------------------------------------------------------------------------
/**
 * The first-layer antenna status Handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static void FirstLayerAntennaStatusHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    le_antenna_StatusHandlerFunc_t clientHandlerFunc = secondLayerHandlerFunc;

    pa_antenna_StatusInd_t* statusPtr = (pa_antenna_StatusInd_t*) reportPtr;

    LE_DEBUG("Call application handler antennaType %d", statusPtr->antennaType);

    // Call the client handler
    clientHandlerFunc( AntennaCtx[statusPtr->antennaType].antennaRef,
                       statusPtr->status,
                       le_event_GetContextPtr() );
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler called by the PA to inform a status changed of an antenna
 *
 */
//--------------------------------------------------------------------------------------------------
static void AntennaStatus ( pa_antenna_StatusInd_t* msgRef )
{
    // Sanity check
    if (( msgRef->antennaType >= LE_ANTENNA_MAX ) ||
        ( AntennaCtx[msgRef->antennaType].antennaRef == NULL ) ||
        ( AntennaCtx[msgRef->antennaType].statusEventId == NULL ))
    {
        LE_ERROR("Invalid status indication");
        return;
    }

    LE_DEBUG("Report AntennaStatus antenna %d", msgRef->antennaType);

    // Report the status if event was created
    le_event_Report( AntennaCtx[msgRef->antennaType].statusEventId,
                    (void*)msgRef, sizeof(pa_antenna_StatusInd_t) );
}

//--------------------------------------------------------------------------------------------------
//                                       Public declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Requested the antenna monitoring.
 *
 * @return
 *      - Reference to the antenna object.
 *      - NULL on failure.
 */
//--------------------------------------------------------------------------------------------------
le_antenna_ObjRef_t le_antenna_Request
(
    le_antenna_Type_t antennaType
        ///< [IN]
        ///< antenna to be monitored
)
{
    // Sanity checks
    if ( antennaType >= LE_ANTENNA_MAX )
    {
        return NULL;
    }

    return AntennaCtx[antennaType].antennaRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the antenna type.
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_FOUND if the antenna reference is unknown
 *      - LE_BAD_PARAMETER if an invalid reference provided.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_antenna_GetType
(
    le_antenna_ObjRef_t antennaRef,
        ///< [IN]
        ///< antenna reference

    le_antenna_Type_t* antennaTypePtr
        ///< [OUT]
        ///< allocated antenna type
)
{
    if (antennaTypePtr == NULL)
    {
        LE_KILL_CLIENT("antennaTypePtr is NULL.");
        return LE_FAULT;
    }

    if (NULL == antennaRef)
    {
        LE_ERROR("Invalid reference %p", antennaRef);
        return LE_NOT_FOUND;
    }

    // Get the context from the reference
    AntennaCtx_t*  antennaCtxPtr = le_ref_Lookup(AntennaRefMap, antennaRef);
    if (NULL == antennaCtxPtr)
    {
        LE_KILL_CLIENT("Invalid reference %p", antennaRef);
        return LE_BAD_PARAMETER;
    }

    // Get the antenna type
    *antennaTypePtr = antennaCtxPtr->antennaType;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the ADC value used to detect a short circuit.
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_FOUND if the antenna reference is unknown
 *      - LE_FAULT on other failure
 *      - LE_BAD_PARAMETER Invalid reference provided.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_antenna_SetShortLimit
(
    le_antenna_ObjRef_t antennaRef,
        ///< [IN]
        ///< antenna reference

    uint32_t shortLimit
        ///< [IN]
        ///< The ADC value used to detect a short circuit
)
{
    if (NULL == antennaRef)
    {
        LE_ERROR("Invalid reference %p", antennaRef);
        return LE_NOT_FOUND;
    }

    // Get the context from the reference
    AntennaCtx_t*  antennaCtxPtr = le_ref_Lookup(AntennaRefMap, antennaRef);
    if (NULL == antennaCtxPtr)
    {
        LE_KILL_CLIENT("Invalid reference %p", antennaRef);
        return LE_BAD_PARAMETER;
    }

    // Set the short limit in the PA
    return pa_antenna_SetShortLimit(antennaCtxPtr->antennaType, shortLimit);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the ADC value used to detect a short circuit.
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_FOUND if the antenna reference is unknown
 *      - LE_FAULT on other failure
 *      - LE_BAD_PARAMETER Invalid reference provided.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_antenna_GetShortLimit
(
    le_antenna_ObjRef_t antennaRef,
        ///< [IN]
        ///< antenna reference

    uint32_t* shortLimitPtr
        ///< [OUT]
        ///< The ADC value used to detect a short circuit
)
{
    if (NULL == antennaRef)
    {
        LE_ERROR("Invalid reference %p", antennaRef);
        return LE_NOT_FOUND;
    }

    // Get the context from the reference
    AntennaCtx_t*  antennaCtxPtr = le_ref_Lookup(AntennaRefMap, antennaRef);
    if (NULL == antennaCtxPtr)
    {
        LE_KILL_CLIENT("Invalid reference %p", antennaRef);
        return LE_BAD_PARAMETER;
    }

    // Get the short limit
    return pa_antenna_GetShortLimit(antennaCtxPtr->antennaType, shortLimitPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the ADC value used to detect an open circuit.
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_FOUND if the antenna reference is unknown
 *      - LE_FAULT on other failure
 *      - LE_BAD_PARAMETER Invalid reference provided.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_antenna_SetOpenLimit
(
    le_antenna_ObjRef_t antennaRef,
        ///< [IN]
        ///< antenna reference

    uint32_t openLimit
        ///< [IN]
        ///< The ADC value used to detect an open circuit
)
{
    if (NULL == antennaRef)
    {
        LE_ERROR("Invalid reference %p", antennaRef);
        return LE_NOT_FOUND;
    }

    // Get the context from the reference
    AntennaCtx_t*  antennaCtxPtr = le_ref_Lookup(AntennaRefMap, antennaRef);
    if (NULL == antennaCtxPtr)
    {
        LE_KILL_CLIENT("Invalid reference %p", antennaRef);
        return LE_BAD_PARAMETER;
    }

    // Set the open limit
    return pa_antenna_SetOpenLimit(antennaCtxPtr->antennaType, openLimit);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the ADC value used to detect an open circuit.
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_FOUND if the antenna reference is unknown
 *      - LE_FAULT on other failure
 *      - LE_BAD_PARAMETER Invalid reference provided.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_antenna_GetOpenLimit
(
    le_antenna_ObjRef_t antennaRef,
        ///< [IN]
        ///< antenna reference

    uint32_t* openLimitPtr
        ///< [OUT]
        ///< The ADC value used to detect an open circuit
)
{
    if (NULL == antennaRef)
    {
        LE_ERROR("Invalid reference %p", antennaRef);
        return LE_NOT_FOUND;
    }

    // Get the context from the reference
    AntennaCtx_t*  antennaCtxPtr = le_ref_Lookup(AntennaRefMap, antennaRef);
    if (NULL == antennaCtxPtr)
    {
        LE_KILL_CLIENT("Invalid reference %p", antennaRef);
        return LE_BAD_PARAMETER;
    }

    // Get the open limit
    return pa_antenna_GetOpenLimit(antennaCtxPtr->antennaType, openLimitPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * le_antenna_StatusHandler handler ADD function
 */
//--------------------------------------------------------------------------------------------------
le_antenna_StatusEventHandlerRef_t le_antenna_AddStatusEventHandler
(
    le_antenna_ObjRef_t antennaRef,
        ///< [IN]
        ///< antenna reference

    le_antenna_StatusHandlerFunc_t handlerPtr,
        ///< [IN]

    void* contextPtr
        ///< [IN]
)
{
    if ( handlerPtr == NULL )
    {
        LE_ERROR("handlerPtr is NULL !");
        return NULL;
    }

    // Get the context from the reference
    AntennaCtx_t* antennaCtxPtr = le_ref_Lookup( AntennaRefMap, antennaRef );

    if ( antennaCtxPtr == NULL )
    {
        LE_ERROR("Invalid reference (%p) provided!", antennaCtxPtr);
        return NULL;
    }

    // Set the status indication of the antenna in the PA
    if ( pa_antenna_SetStatusIndication( antennaCtxPtr->antennaType ) == LE_OK )
    {
        // Add the layered handler
        antennaCtxPtr->statusEventHandlerRef = le_event_AddLayeredHandler(
                                                "LeAntennaStatusHandler",
                                                antennaCtxPtr->statusEventId,
                                                FirstLayerAntennaStatusHandler,
                                                (le_event_HandlerFunc_t)handlerPtr);

        le_event_SetContextPtr(antennaCtxPtr->statusEventHandlerRef, contextPtr);

        LE_DEBUG("Handler set for antenna %d", antennaCtxPtr->antennaType);

        return (le_antenna_StatusEventHandlerRef_t)(antennaRef);
    }
    else
    {
        LE_ERROR("Status event hanlder not subscribed");
        return NULL;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * le_antenna_StatusHandler handler REMOVE function
 */
//--------------------------------------------------------------------------------------------------
void le_antenna_RemoveStatusEventHandler
(
    le_antenna_StatusEventHandlerRef_t addHandlerRef
        ///< [IN]
)
{
    AntennaCtx_t*  antennaCtxPtr = le_ref_Lookup(AntennaRefMap, addHandlerRef);

    if ( antennaCtxPtr == NULL )
    {
        LE_ERROR("Invalid reference (%p) provided!", antennaCtxPtr);
        return;
    }

    if ( antennaCtxPtr->statusEventHandlerRef )
    {
        // Remove the handler
        le_event_RemoveHandler( (le_event_HandlerRef_t) antennaCtxPtr->statusEventHandlerRef );
        antennaCtxPtr->statusEventHandlerRef = NULL;
    }
    else
    {
        LE_ERROR("No handler subscribed");
    }

    // Remove the status indication into the PA
    pa_antenna_RemoveStatusIndication( antennaCtxPtr->antennaType );
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the antenna status.
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_FOUND if the antenna reference is unknown
 *      - LE_UNSUPPORTED if the Antenna detection is not supported
 *      - LE_FAULT on other failure
 *      - LE_BAD_PARAMETER Invalid reference provided.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_antenna_GetStatus
(
    le_antenna_ObjRef_t antennaRef,
        ///< [IN]
        ///< antenna reference

    le_antenna_Status_t* statusPtr
        ///< [OUT]
        ///< antenna status
)
{
    if (NULL == antennaRef)
    {
        LE_ERROR("Invalid reference %p", antennaRef);
        return LE_NOT_FOUND;
    }

    // Get the context from the reference
    AntennaCtx_t*  antennaCtxPtr = le_ref_Lookup(AntennaRefMap, antennaRef);
    if (NULL == antennaCtxPtr)
    {
        LE_KILL_CLIENT("Invalid reference %p", antennaRef);
        return LE_BAD_PARAMETER;
    }

    // Get the open limit
    return pa_antenna_GetStatus( antennaCtxPtr->antennaType, statusPtr );
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the external ADC used to monitor the requested antenna.
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_FOUND if the antenna reference is unknown
 *      - LE_UNSUPPORTED request not supported
 *      - LE_FAULT on other failure
 *      - LE_BAD_PARAMETER Invalid reference provided.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_antenna_SetExternalAdc
(
    le_antenna_ObjRef_t    antennaRef,  ///< antenna reference
    int8_t                 adcId        ///< The external ADC used to monitor the requested antenna
)
{
    if (NULL == antennaRef)
    {
        LE_ERROR("Invalid reference %p", antennaRef);
        return LE_NOT_FOUND;
    }

    // Get the context from the reference
    AntennaCtx_t*  antennaCtxPtr = le_ref_Lookup(AntennaRefMap, antennaRef);
    if (NULL == antennaCtxPtr)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", antennaRef);
        return LE_BAD_PARAMETER;
    }

    return pa_antenna_SetExternalAdc(antennaCtxPtr->antennaType, adcId);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the external ADC channel used to monitor the requested antenna.
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_FOUND if the antenna reference is unknown
 *      - LE_UNSUPPORTED request not supported
 *      - LE_FAULT on other failure
 *      - LE_BAD_PARAMETER Invalid reference provided.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_antenna_GetExternalAdc
(
    le_antenna_ObjRef_t    antennaRef,  ///< antenna reference
    int8_t*                adcIdPtr     ///< The external ADC used to monitor the requested antenna
)
{
    if (NULL == antennaRef)
    {
        LE_ERROR("Invalid reference %p", antennaRef);
        return LE_NOT_FOUND;
    }

    // Get the context from the reference
    AntennaCtx_t*  antennaCtxPtr = le_ref_Lookup(AntennaRefMap, antennaRef);
    if (NULL == antennaCtxPtr)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", antennaRef);
        return LE_BAD_PARAMETER;
    }

    return pa_antenna_GetExternalAdc(antennaCtxPtr->antennaType, adcIdPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialization of the Legato Antenna Monitoring Service
 */
//--------------------------------------------------------------------------------------------------
void le_antenna_Init
(
    void
)
{
    // Create safe reference map for antenna references.
    AntennaRefMap = le_ref_CreateMap("AntennaRef", LE_ANTENNA_MAX);

    memset( AntennaCtx, 0, LE_ANTENNA_MAX*sizeof(AntennaCtx_t) );

    pa_antenna_AddStatusHandler ( AntennaStatus );

    // Init the context
    le_antenna_Type_t antenna;

    for (antenna = LE_ANTENNA_PRIMARY_CELLULAR; antenna < LE_ANTENNA_MAX; antenna++)
    {
        char eventName[32];
        snprintf(eventName, sizeof(eventName), "AntennaStatus_%d", antenna);

        AntennaCtx[antenna].antennaType = antenna;
        AntennaCtx[antenna].antennaRef = le_ref_CreateRef(AntennaRefMap, &AntennaCtx[antenna]);
        AntennaCtx[antenna].statusEventId = le_event_CreateId(
                                            eventName,
                                            sizeof(pa_antenna_StatusInd_t));
        AntennaCtx[antenna].statusEventHandlerRef = NULL;
    }
}
