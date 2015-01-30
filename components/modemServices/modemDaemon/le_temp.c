//--------------------------------------------------------------------------------------------------
/**
 * @file le_temp.c
 *
 * This file contains the source code of the high level temperature API.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "pa_temp.h"

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Data structures.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
//                                       Static declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Event ID for New Temperature Threshold event notification.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t TemperatureThresholdEventId;


//--------------------------------------------------------------------------------------------------
/**
 * The first-layer Radio Access Technology Change Handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static void FirstLayerTemperatureChangeHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    le_temp_ThresholdStatus_t*           tempPtr = reportPtr;
    le_temp_ThresholdEventHandlerFunc_t clientHandlerFunc = secondLayerHandlerFunc;

    clientHandlerFunc(*tempPtr, le_event_GetContextPtr());

    // The reportPtr is a reference counted object, so need to release it
    le_mem_Release(reportPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Temperature Change handler function.
 *
 */
//--------------------------------------------------------------------------------------------------
static void TemperatureChangeHandler
(
    le_temp_ThresholdStatus_t* thresholdEventPtr
)
{
    // Notify all the registered client's handlers
    le_event_ReportWithRefCounting(TemperatureThresholdEventId, thresholdEventPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * handler function to the close session service
 *
 */
//--------------------------------------------------------------------------------------------------
static void CloseSessionEventHandler
(
    le_msg_SessionRef_t sessionRef,
    void*               contextPtr
)
{
    LE_DEBUG("Temperature client killed");
}

//--------------------------------------------------------------------------------------------------
//                                       Public declarations
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'le_temp_ThresholdEvent'
 *
 * This event provides information on Threshold reached.
 */
//--------------------------------------------------------------------------------------------------
le_temp_ThresholdEventHandlerRef_t le_temp_AddThresholdEventHandler
(
    le_temp_ThresholdEventHandlerFunc_t handlerPtr,
        ///< [IN]

    void* contextPtr
        ///< [IN]
)
{
    le_event_HandlerRef_t        handlerRef;

    if (handlerPtr == NULL)
    {
        LE_KILL_CLIENT("Handler function is NULL !");
        return NULL;
    }

    handlerRef = le_event_AddLayeredHandler("TemperatureThresholdHandler",
                    TemperatureThresholdEventId,
                    FirstLayerTemperatureChangeHandler,
                    (le_event_HandlerFunc_t)handlerPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (le_temp_ThresholdEventHandlerRef_t)(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'le_temp_ThresholdEvent'
 */
//--------------------------------------------------------------------------------------------------
void le_temp_RemoveThresholdEventHandler
(
    le_temp_ThresholdEventHandlerRef_t addHandlerRef
        ///< [IN]
)
{
    if (addHandlerRef == NULL)
    {
        LE_KILL_CLIENT("addHandlerRef function is NULL !");
    }

    le_event_RemoveHandler((le_event_HandlerRef_t) addHandlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the Platform temperature level in degree celsius.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to get the temperature.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_temp_GetPlatformTemperature
(
    int32_t* platformTempPtr
        ///< [OUT]
        ///< [OUT] The Platform temperature level in degree celsius.
)
{
    if (platformTempPtr == NULL)
    {
        LE_KILL_CLIENT("platformTempPtr is NULL!!");
        return LE_FAULT;
    }

    return pa_temp_GetPlatformTemperature(platformTempPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the Radio temperature level in degree celsius.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to get the temperature.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_temp_GetRadioTemperature
(
    int32_t* radioTempPtr
        ///< [OUT]
        ///< [OUT] The Radio temperature level in degree celsius.
)
{
    if (radioTempPtr == NULL)
    {
        LE_KILL_CLIENT("radioTempPtr is NULL!!");
        return LE_FAULT;
    }

    return pa_temp_GetRadioTemperature(radioTempPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the Radio warning and critical temperature thresholds in degree celsius.
 *  When thresholds temperature are reached, a temperature event is triggered.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_BAD_PARAMETER The hiWarning threshold + 1 is equal to or higher than
 *                           the hiCritical threshold.
 *      - LE_FAULT         The function failed to set the thresholds.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_temp_SetRadioThresholds
(
    int32_t hiWarningTemp,
        ///< [IN]
        ///< [IN] The high warning temperature threshold in degree celsius.

    int32_t hiCriticalTemp
        ///< [IN]
        ///< [IN] The high critical temperature threshold in degree celsius.
)
{
    return pa_temp_SetRadioThresholds(hiWarningTemp, hiCriticalTemp);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the Radio warning and critical temperature thresholds in degree celsius.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to get the thresholds.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_temp_GetRadioThresholds
(
    int32_t* hiWarningTempPtr,
        ///< [OUT]
        ///< [OUT] The high warning temperature threshold in degree celsius.

    int32_t* hiCriticalTempPtr
        ///< [OUT]
        ///< [OUT] The high critical temperature threshold
        ///<  in degree celsius.
)
{
    if ( (hiWarningTempPtr == NULL) || (hiWarningTempPtr == NULL))
    {
        LE_KILL_CLIENT("hiWarningTempPtr or/and hiWarningTempPtr are NULL!!");
        return LE_FAULT;
    }
    return pa_temp_GetRadioThresholds(hiWarningTempPtr, hiCriticalTempPtr);
}



//--------------------------------------------------------------------------------------------------
/**
 * Set the Platform warning and critical temperature thresholds in degree celsius.
 *  When thresholds temperature are reached, a temperature event is triggered.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_BAD_PARAMETER The hiWarning threshold + 1 is equal to or higher than
 *                           the hiCritical threshold.
 *                         The loWwarning threshold is equal to or higher than
 *                           the hiWarning threshold.
 *                         The loWwarning threshold is equal to or lower than
 *                           the loCritical threshold.
 *      - LE_FAULT         The function failed to set the thresholds.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_temp_SetPlatformThresholds
(
    int32_t lowCriticalTemp,
        ///< [IN]
        ///< [IN] The low critical temperature threshold in degree celsius.

    int32_t lowWarningTemp,
        ///< [IN]
        ///< [IN] The low warning temperature threshold in degree celsius.

    int32_t hiWarningTemp,
        ///< [IN]
        ///< [IN] The high warning temperature threshold in degree celsius.

    int32_t hiCriticalTemp
        ///< [IN]
        ///< [IN] The high critical temperature threshold in degree celsius.
)
{
   return pa_temp_SetPlatformThresholds (lowCriticalTemp, lowWarningTemp,
                        hiWarningTemp, hiCriticalTemp);
}



//--------------------------------------------------------------------------------------------------
/**
 * Get the Platform warning and critical temperature thresholds in degree celsius.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to get the thresholds.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_temp_GetPlatformThresholds
(
    int32_t* lowCriticalTempPtr,
        ///< [OUT]
        ///< [OUT] The low critical temperature threshold in degree celsius.

    int32_t* lowWarningTempPtr,
        ///< [OUT]
        ///< [OUT] The low warning temperature threshold in degree celsius.

    int32_t* hiWarningTempPtr,
        ///< [OUT]
        ///< [OUT] The high warning temperature threshold in degree celsius.

    int32_t* hiCriticalTempPtr
        ///< [OUT]
        ///< [OUT] The high critical temperature threshold
        ///<  in degree celsius.
)
{
    if ( (lowCriticalTempPtr == NULL) || (lowWarningTempPtr == NULL)
         || (hiWarningTempPtr == NULL) || (hiCriticalTempPtr == NULL) )
    {
        LE_KILL_CLIENT("Parameters pointer are NULL!!");
        return LE_FAULT;
    }
    return pa_temp_GetPlatformThresholds(lowCriticalTempPtr, lowWarningTempPtr,
                    hiWarningTempPtr, hiCriticalTempPtr);
}



//-------------------------------------------------------------------------------------------------
/**
 * Initialization of the Legato Tempearture Monitoring Service
 */
//--------------------------------------------------------------------------------------------------
void le_temp_Init
(
    void
)
{
    // Add a handler to the close session service
    le_msg_AddServiceCloseHandler(
                    le_temp_GetServiceRef(),
                    CloseSessionEventHandler,
                    NULL
    );

    // Create an event Id for temperature change notification
    TemperatureThresholdEventId = le_event_CreateIdWithRefCounting("TemperatureThresholdEvent");

    // Register a handler function for new temparute Threshold Event
    pa_temp_AddTempEventHandler (TemperatureChangeHandler);
}
