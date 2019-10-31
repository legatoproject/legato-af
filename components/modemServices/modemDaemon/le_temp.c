//--------------------------------------------------------------------------------------------------
/**
 * @file le_temp.c
 *
 * This file contains the source code of the high level temperature API.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "pa_temp.h"

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of sensors (can be extended dynamically).
 *
 */
//--------------------------------------------------------------------------------------------------
#define MAX_NUM_OF_SENSOR   10


//--------------------------------------------------------------------------------------------------
/**
 * Expected maximum number of threshold reports.
 */
//--------------------------------------------------------------------------------------------------
#define HIGH_THRESHOLD_REPORT_COUNT      1


//--------------------------------------------------------------------------------------------------
// Data structures.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Data structure of a sensor context.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    pa_temp_Handle_t        paHandle;                        ///< Platform adaptor layer handle
    le_temp_SensorRef_t     ref;                             ///< Sensor reference
    char                    thresholdEvent[LE_TEMP_THRESHOLD_NAME_MAX_BYTES];
                                                             ///< Threshold event
} SensorCtx_t;

//--------------------------------------------------------------------------------------------------
/**
 * Temperature threshold report structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_temp_SensorRef_t ref;                                         ///< Sensor reference
    char                threshold[LE_TEMP_THRESHOLD_NAME_MAX_BYTES]; ///< Threshold name
} ThresholdReport_t;

//--------------------------------------------------------------------------------------------------
/**
 * SessionRef node structure used for the SessionRefList.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_msg_SessionRef_t sessionRef;           ///< Client sessionRef.
    le_temp_SensorRef_t ref;                  ///< Sensor reference.
    le_dls_Link_t       link;                 ///< Link for SessionRefList.
}
SessionRefNode_t;

//--------------------------------------------------------------------------------------------------
//                                       Static declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Static memory pool for sensors.
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(SensorPool,
                          MAX_NUM_OF_SENSOR,
                          sizeof(SensorCtx_t));

//--------------------------------------------------------------------------------------------------
/**
 * Memory Pool for Sensors.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t   SensorPool;

//--------------------------------------------------------------------------------------------------
/**
 * List of session reference.
 */
//--------------------------------------------------------------------------------------------------
static le_dls_List_t  SessionRefList;

//--------------------------------------------------------------------------------------------------
/**
 * Static safe Reference Map for the antenna reference
 */
//--------------------------------------------------------------------------------------------------
LE_REF_DEFINE_STATIC_MAP(SensorRefMap, MAX_NUM_OF_SENSOR);

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for the antenna reference
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t SensorRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Event ID for New Temperature Threshold event notification.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t TemperatureThresholdEventId;

//--------------------------------------------------------------------------------------------------
/**
 * Static pool for Temperature threshold Event reporting.
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(ThresholdReportPool,
                          HIGH_THRESHOLD_REPORT_COUNT,
                          sizeof(ThresholdReport_t));

//--------------------------------------------------------------------------------------------------
/**
 * Pool for Temperature threshold Event reporting.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t ThresholdReportPool;

//--------------------------------------------------------------------------------------------------
/**
 * The static memory pool for the client's sessionRef objects
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(SessionRef,
                          MAX_NUM_OF_SENSOR,
                          sizeof(SessionRefNode_t));


//--------------------------------------------------------------------------------------------------
/**
 * The memory pool for the clients sessionRef objects
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t SessionRefPool;


//--------------------------------------------------------------------------------------------------
/**
 * Look for a sensor reference corresponding to a name.
 */
//--------------------------------------------------------------------------------------------------
static le_temp_SensorRef_t FindSensorRef
(
    const char*  sensorPtr     ///< [IN] Name of the temperature sensor.
)
{
    le_temp_Handle_t    leHandle;

    if (pa_temp_GetHandle(sensorPtr, &leHandle) == LE_OK)
    {
        if (leHandle)
        {
            SensorCtx_t* sensorCtxPtr=(SensorCtx_t*)leHandle;
            return sensorCtxPtr->ref;
        }
        else
        {
            return NULL;
        }
    }
    else
    {
        return NULL;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * The first-layer Temperature Handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static void FirstLayerTemperatureChangeHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    ThresholdReport_t*                  tempPtr = reportPtr;
    le_temp_ThresholdEventHandlerFunc_t clientHandlerFunc = secondLayerHandlerFunc;

    LE_DEBUG("Call application handler for %p sensor reference with '%s' threshold",
             tempPtr->ref,
             tempPtr->threshold);

    // Call the client handler
    clientHandlerFunc(tempPtr->ref,
                      tempPtr->threshold,
                      le_event_GetContextPtr());

    le_mem_Release(reportPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * PA Temperature change handler function.
 *
 */
//--------------------------------------------------------------------------------------------------
static void PaTemperatureThresholdHandler
(
    le_temp_Handle_t leHandle,      ///< [IN] Handle of the temperature sensor.
    const char*      thresholdPtr,  ///< [IN] Name of the threshold.
    void*            contextPtr     ///< [IN] Context pointer.
)
{
    SensorCtx_t* sensorCtxPtr = (SensorCtx_t*)leHandle;

    if (NULL == sensorCtxPtr)
    {
        LE_ERROR("Temperature sensor handle has not been set");
        return;
    }

    ThresholdReport_t* tempEventPtr = le_mem_ForceAlloc(ThresholdReportPool);

    tempEventPtr->ref = sensorCtxPtr->ref;
    le_utf8_Copy(tempEventPtr->threshold, thresholdPtr, LE_TEMP_THRESHOLD_NAME_MAX_BYTES, NULL);

    LE_INFO("Report '%s' threshold for %p sensor reference",
             tempEventPtr->threshold,
             tempEventPtr->ref);

    le_event_ReportWithRefCounting(TemperatureThresholdEventId, tempEventPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Close session handler for client session.
 *
 */
//--------------------------------------------------------------------------------------------------
static void CloseSessionEventHandler
(
    le_msg_SessionRef_t sessionRef, ///< [IN] Message session reference.
    void* contextPtr                ///< [IN] Context pointer.
)
{
    if (!sessionRef)
    {
        LE_ERROR("ERROR sessionRef is NULL");
        return;
    }

    LE_DEBUG("SessionRef (%p) has been closed", sessionRef);

    le_dls_Link_t* linkPtr = le_dls_Peek(&SessionRefList);

    while (linkPtr)
    {
        SessionRefNode_t *sessionRefNodePtr = CONTAINER_OF(linkPtr, SessionRefNode_t, link);
        linkPtr = le_dls_PeekNext(&SessionRefList, linkPtr);

        if (sessionRefNodePtr->sessionRef == sessionRef)
        {
            LE_DEBUG("Release memory for session 0x%p\n", sessionRef);

            SensorCtx_t* sensorCtxPtr = le_ref_Lookup(SensorRefMap, sessionRefNodePtr->ref);

            // Release temperature sensor handle reference.
            le_mem_Release(sensorCtxPtr);

            // Remove the link from the session reference list.
            le_dls_Remove(&SessionRefList, &(sessionRefNodePtr->link));

            // Release session reference node.
            le_mem_Release(sessionRefNodePtr);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Destructor function that runs when a memory reference count of
 * temperature sensor context pointer reaches 0.
 */
//--------------------------------------------------------------------------------------------------
static void TempSensorDestructor
(
    void *objPtr  ///< [IN] Sensor context object pointer.
)
{
    char sensorName[LE_TEMP_SENSOR_NAME_MAX_BYTES];
    SensorCtx_t* sensorCtxPtr = (SensorCtx_t*)objPtr;

    /* Delete the reference */
    le_ref_DeleteRef(SensorRefMap, sensorCtxPtr->ref);

    // Get sensor name from paHandle.
    if (LE_OK != pa_temp_GetSensorName(sensorCtxPtr->paHandle, sensorName,
                                               LE_TEMP_SENSOR_NAME_MAX_BYTES))
    {
        LE_ERROR("Not able to get temperature sensor name");
        return;
    }

    // Reset temperature handle at PA side.
    if (LE_OK != pa_temp_ResetHandle(sensorName))
    {
        LE_ERROR("Not able to reset temperature handle");
    }
}


//--------------------------------------------------------------------------------------------------
//                                       Public declarations
//--------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
/**
 * Initialization of the Legato Temperature Monitoring Service
 */
//--------------------------------------------------------------------------------------------------
void le_temp_Init
(
    void
)
{
    LE_DEBUG("call marker.");

    // Create an event Id for temperature change notification
    TemperatureThresholdEventId = le_event_CreateIdWithRefCounting("TempThresholdEvent");

    ThresholdReportPool = le_mem_InitStaticPool(ThresholdReportPool,
                                                HIGH_THRESHOLD_REPORT_COUNT,
                                                sizeof(ThresholdReport_t));

    SensorPool = le_mem_InitStaticPool(SensorPool,
                                       MAX_NUM_OF_SENSOR,
                                       sizeof(SensorCtx_t));

    // Create destructor to reset PA handle reference
    // when memory reference count of sensor context reaches 0.
    le_mem_SetDestructor(SensorPool, TempSensorDestructor);

    SensorRefMap = le_ref_InitStaticMap(SensorRefMap, MAX_NUM_OF_SENSOR);

    // Memory pool to store session reference associated to the temperature sensor.
    SessionRefPool = le_mem_InitStaticPool(SessionRef,
                                           MAX_NUM_OF_SENSOR,
                                           sizeof(SessionRefNode_t));

    // Session reference list.
    SessionRefList = LE_DLS_LIST_INIT;

    // Register close session handler.
    le_msg_AddServiceCloseHandler(le_temp_GetServiceRef(), CloseSessionEventHandler, NULL);

    // Register a handler function for new temperature Threshold Event
    pa_temp_AddTempEventHandler(PaTemperatureThresholdHandler, NULL);
}

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

    LE_DEBUG("call marker.");

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
    LE_DEBUG("call marker.");

    if (addHandlerRef == NULL)
    {
        LE_KILL_CLIENT("addHandlerRef function is NULL !");
        return;
    }

    le_event_RemoveHandler((le_event_HandlerRef_t) addHandlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Request a temperature sensor reference.
 *
 * @return
 *      - Reference to the temperature sensor.
 *      - NULL when the requested sensor is not supported.
 */
//--------------------------------------------------------------------------------------------------
le_temp_SensorRef_t le_temp_Request
(
    const char*  sensorPtr ///< [IN] Name of the temperature sensor.
)
{
    size_t              length;
    SensorCtx_t*        currentPtr=NULL;
    le_temp_SensorRef_t sensorRef;

    LE_DEBUG("call marker.");

    if (strlen(sensorPtr) > (LE_TEMP_SENSOR_NAME_MAX_BYTES-1))
    {
        LE_KILL_CLIENT("strlen(sensorPtr) > %d", (LE_TEMP_SENSOR_NAME_MAX_BYTES-1));
        return NULL;
    }

    length = strnlen(sensorPtr, LE_TEMP_SENSOR_NAME_MAX_BYTES+1);
    if (!length)
    {
        return NULL;
    }

    if (length > LE_TEMP_SENSOR_NAME_MAX_BYTES)
    {
        return NULL;
    }

    // Check if this sensor already exists
    if ((sensorRef = FindSensorRef(sensorPtr)) != NULL)
    {
        SensorCtx_t* sensorCtxPtr = le_ref_Lookup(SensorRefMap, sensorRef);
        le_mem_AddRef(sensorCtxPtr);

        // Create session reference list which associate to the sensor reference.
        SessionRefNode_t* sessionRefNodePtr = le_mem_ForceAlloc(SessionRefPool);
        sessionRefNodePtr->ref = sensorRef;
        sessionRefNodePtr->sessionRef = le_temp_GetClientSessionRef();
        sessionRefNodePtr->link = LE_DLS_LINK_INIT;
        le_dls_Queue(&SessionRefList, &(sessionRefNodePtr->link));

        return sensorRef;
    }
    else
    {
        currentPtr = le_mem_ForceAlloc(SensorPool);

        if (LE_OK == pa_temp_Request(sensorPtr,
                            (le_temp_Handle_t)currentPtr,
                            &currentPtr->paHandle))
        {
            currentPtr->ref = le_ref_CreateRef(SensorRefMap, currentPtr);

            // Create session reference list which associate to the sensor reference.
            SessionRefNode_t* sessionRefNodePtr = le_mem_ForceAlloc(SessionRefPool);
            sessionRefNodePtr->ref = currentPtr->ref;
            sessionRefNodePtr->sessionRef = le_temp_GetClientSessionRef();
            sessionRefNodePtr->link = LE_DLS_LINK_INIT;
            le_dls_Queue(&SessionRefList, &sessionRefNodePtr->link);

            LE_DEBUG("Create a new sensor reference (%p)", currentPtr->ref);
            return currentPtr->ref;
        }
        else
        {
            le_mem_Release(currentPtr);
            LE_DEBUG("This sensor (%s) doesn't exist on your platform", sensorPtr);
            return NULL;
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Retrieve the temperature sensor's name from its reference.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_OVERFLOW      The name length exceed the maximum length.
 *      - LE_FAULT         The function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_temp_GetSensorName
(
    le_temp_SensorRef_t sensorRef,      ///< [IN]  Temperature sensor reference.
    char*               sensorNamePtr,  ///< [OUT] Name of the temperature sensor.
    size_t              len             ///< [IN] The maximum length of the sensor name.
)
{
    SensorCtx_t* sensorCtxPtr = le_ref_Lookup(SensorRefMap, sensorRef);

    LE_DEBUG("call marker.");

    if (sensorCtxPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", sensorRef);
        return LE_FAULT;
    }

    if (sensorNamePtr == NULL)
    {
        LE_KILL_CLIENT("sensorNamePtr is NULL !");
        return LE_FAULT;
    }

    return pa_temp_GetSensorName(sensorCtxPtr->paHandle, sensorNamePtr, len);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the temperature in degree Celsius.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to get the temperature.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_temp_GetTemperature
(
    le_temp_SensorRef_t  sensorRef,     ///< [IN] Temperature sensor reference.
    int32_t*             temperaturePtr ///< [OUT] Temperature in degree Celsius.
)
{
    SensorCtx_t* sensorCtxPtr = le_ref_Lookup(SensorRefMap, sensorRef);

    LE_DEBUG("call marker.");

    if (sensorCtxPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", sensorRef);
        return LE_FAULT;
    }

    if (temperaturePtr == NULL)
    {
        LE_KILL_CLIENT("temperaturePtr is NULL!!");
        return LE_FAULT;
    }

    return pa_temp_GetTemperature(sensorCtxPtr->paHandle, temperaturePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the temperature threshold in degree Celsius. This function does not start the temperature
 * monitoring, call le_temp_StartMonitoring() to start it.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_temp_SetThreshold
(
    le_temp_SensorRef_t  sensorRef,     ///< [IN] Temperature sensor reference.
    const char*          thresholdPtr,  ///< [IN] Name of the threshold.
    int32_t              temperature    ///< [IN] Temperature threshold in degree Celsius.
)
{
    size_t       length;
    SensorCtx_t* sensorCtxPtr = le_ref_Lookup(SensorRefMap, sensorRef);

    LE_DEBUG("call marker.");

    if (sensorCtxPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", sensorRef);
        return LE_FAULT;
    }

    if(strlen(thresholdPtr) > (LE_TEMP_THRESHOLD_NAME_MAX_BYTES-1))
    {
        LE_KILL_CLIENT("strlen(thresholdPtr) > %d", (LE_TEMP_THRESHOLD_NAME_MAX_BYTES-1));
        return LE_FAULT;
    }

    length = strnlen(thresholdPtr, LE_TEMP_THRESHOLD_NAME_MAX_BYTES+1);
    if (!length)
    {
        return LE_FAULT;
    }

    if (length > LE_TEMP_THRESHOLD_NAME_MAX_BYTES)
    {
        return LE_FAULT;
    }

    return pa_temp_SetThreshold(sensorCtxPtr->paHandle, thresholdPtr, temperature);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the temperature threshold in degree Celsius.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_temp_GetThreshold
(
    le_temp_SensorRef_t  sensorRef,     ///< [IN] Temperature sensor reference.
    const char*          thresholdPtr,  ///< [IN] Name of the threshold.
    int32_t*             temperaturePtr ///< [OUT] Temperature threshold in degree Celsius.
)
{
    size_t       length;
    SensorCtx_t* sensorCtxPtr = le_ref_Lookup(SensorRefMap, sensorRef);

    LE_DEBUG("call marker.");

    if (sensorCtxPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", sensorRef);
        return LE_FAULT;
    }

    if(strlen(thresholdPtr) > (LE_TEMP_THRESHOLD_NAME_MAX_BYTES-1))
    {
        LE_KILL_CLIENT("strlen(thresholdPtr) > %d", (LE_TEMP_THRESHOLD_NAME_MAX_BYTES-1));
        return LE_FAULT;
    }

    length = strnlen(thresholdPtr, LE_TEMP_THRESHOLD_NAME_MAX_BYTES+1);
    if (!length)
    {
        return LE_FAULT;
    }

    if (length > LE_TEMP_THRESHOLD_NAME_MAX_BYTES)
    {
        return LE_FAULT;
    }

    if (temperaturePtr == NULL)
    {
        LE_KILL_CLIENT("temperaturePtr is NULL!!");
        return LE_FAULT;
    }

    return pa_temp_GetThreshold(sensorCtxPtr->paHandle, thresholdPtr, temperaturePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Start the temperature monitoring with the temperature thresholds configured by
 * le_temp_SetThreshold() function.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to apply the thresholds.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_temp_StartMonitoring
(
    void
)
{
    LE_DEBUG("call marker.");

    return pa_temp_StartMonitoring();
}
