#include "gnss.h"

//--------------------------------------------------------------------------------------------------
/**
* Status Flags
*
*/
//--------------------------------------------------------------------------------------------------
static bool GiveUp;
static bool LocationReady;

//--------------------------------------------------------------------------------------------------
/**
* Accuracy of GNSS location
*
*/
//--------------------------------------------------------------------------------------------------
static int LocationAccuracy;

//--------------------------------------------------------------------------------------------------
/**
* References
*
*/
//--------------------------------------------------------------------------------------------------
static le_thread_Ref_t                PositionThreadRef;
static le_gnss_PositionHandlerRef_t   PositionHandlerRef;
static le_mem_PoolRef_t               CoordinateInformationPoolRef;

//--------------------------------------------------------------------------------------------------
/**
* Function Prototypes
*
*/
//--------------------------------------------------------------------------------------------------
static void   FindCoordinates           (int32_t latitude, int32_t longitude, int32_t hAccuracy);
static void*  PositionThread            (void* context);
static void   PositionHandlerFunction   (le_gnss_SampleRef_t positionSampleRef, void* contextPtr);

//--------------------------------------------------------------------------------------------------
/**
* Get the last sampled coordinates from the GNSS service and store them in coordinate information
*  data structure. End GNSS service and cancel the thread handling it. Set LocationReady to true
*
*/
//--------------------------------------------------------------------------------------------------
static void FindCoordinates
(
    int32_t latitude,
    int32_t longitude,
    int32_t hAccuracy
)
{
    le_gnss_SampleRef_t positionSampleRef;
    le_result_t result;

    double  currentLat;
    double  currentLon;

    // Get Epoch time, get last position sample
    positionSampleRef = le_gnss_GetLastSampleRef();
    result = le_gnss_GetLocation(positionSampleRef, &latitude, &longitude, &hAccuracy);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get last registered location");
    }

    // set current lattitude and longitude to last registered coordinates
    currentLat = (double)latitude/1000000.0;
    currentLon = (double)longitude/1000000.0;

    if (currentLat != 0 && currentLon != 0)
    {
        CoordinateInformationPoolRef = le_mem_CreatePool("CoordinateInformationPoolRef", sizeof(CoordinateInformation_t));
        CoordinateInformationPtr = (CoordinateInformation_t*)le_mem_ForceAlloc(CoordinateInformationPoolRef);
        CoordinateInformationPtr->currentLat = currentLat;
        CoordinateInformationPtr->currentLon = currentLon;
        LocationReady = true;
    }

    result = le_gnss_Stop();
    if (result != LE_OK)
    {
        LE_ERROR("Failed to stop gnss");
    }

    le_gnss_ReleaseSampleRef(positionSampleRef);
    le_gnss_RemovePositionHandler(PositionHandlerRef);
    le_thread_Cancel(PositionThreadRef);
}

//--------------------------------------------------------------------------------------------------
/**
* Position handler thread
*
*/
//--------------------------------------------------------------------------------------------------
static void* PositionThread
(
    void* context     ///< [IN]
)
{
    le_gnss_ConnectService();
    PositionHandlerRef = le_gnss_AddPositionHandler(PositionHandlerFunction, NULL);
    le_event_RunLoop();
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
* Initiate GNSS and create a thread to watch the location until sufficient accuracy reached, then geocode.
*
*/
//--------------------------------------------------------------------------------------------------
void gnss_InitiateWatchGNSS
(
    int accuracy,       ///< [IN] Accuracy specified in command line
    bool bbox,          ///< [IN] The boundary box in which to search POI
    char* poiName,      ///< [IN] POI name
    bool locate         ///< [IN] Indicates whether we are locating device or searching for POI
)
{
    le_result_t result;
    LocationAccuracy = accuracy;

    result = le_gnss_Start();
    if (result != LE_OK)
    {
        LE_ERROR("Failed to start gnss");
        ctrlGPS_CleanUp(false);
    }
    sleep(5);

    // Add Position Handler
    PositionThreadRef = le_thread_Create("PositionThread", PositionThread, NULL);
    le_thread_Start(PositionThreadRef);
    while (1)
    {
        if (LocationReady)
        {
            gc_InitiateGeocode(CoordinateInformationPtr, bbox, poiName, locate);
            break;
        }
        else if (GiveUp)
        {
            ctrlGPS_CleanUp(false);
        }
        sleep(2);
    }
}

//--------------------------------------------------------------------------------------------------
/**
* Handler function for positioning
*
*/
//--------------------------------------------------------------------------------------------------
static void PositionHandlerFunction
(
    le_gnss_SampleRef_t positionSampleRef,    ///< [IN]
    void* contextPtr                          ///< [IN]
)
{
    le_result_t result;
    le_gnss_FixState_t state;

    static int locationLoopCounter;

    int32_t latitude;
    int32_t longitude;
    int32_t hAccuracy;

    uint64_t EpochTime;

    if (NULL == positionSampleRef)
    {
        LE_ERROR("New Position sample is NULL!");
    }
    else
    {
        LE_DEBUG("New Position sample %p", positionSampleRef);
    }

    // Get Epoch time
    result = le_gnss_GetEpochTime(positionSampleRef, &EpochTime);
    if (result != LE_OK)
    {
        LE_INFO("Failed to get epoch time");
    }

    // Display Epoch time
    LE_INFO("epoch time: %llu:", (unsigned long long int) EpochTime);

    result = le_gnss_InjectUtcTime(EpochTime , 0);
    if (result != LE_OK)
    {
        LE_INFO("Failed to inject utc time");
    }

    // Get position state
    result = le_gnss_GetPositionState(positionSampleRef, &state);
    if (result != LE_OK)
    {
        LE_INFO("Failed to get position state");
    }

    // Get Location
    result = le_gnss_GetLocation(positionSampleRef, &latitude, &longitude, &hAccuracy);
    if (result != LE_OK)
    {
        LE_INFO("Failed to get location");
    }
    if (LE_OK == result)
    {
        LE_INFO("Position lat.%f, long.%f, hAccuracy %f",
          (float)latitude/1000000.0,
          (float)longitude/1000000.0,
          (float)hAccuracy/100.0);
    }
    else
    {
        if (INT32_MAX != latitude)
        {
            LE_INFO("Latitude %f", (float)latitude/1000000.0);
        }
        else
        {
            LE_INFO("Latitude unknown %d", latitude);
        }

        if (INT32_MAX != longitude)
        {
            LE_INFO("Latitude %f", (float)longitude/1000000.0);
        }
        else
        {
            LE_INFO("Longitude unknown %d", longitude);
        }

        if (INT32_MAX != hAccuracy)
        {
            LE_INFO("Horizontal accuracy %f", (float)hAccuracy/100.0);
        }
        else
        {
            LE_INFO("Horizontal accuracy unknown %d", hAccuracy);
        }
    }

    // Release provided Position sample reference
    le_gnss_ReleaseSampleRef(positionSampleRef);


    // exit event loop if accuracy below specified value. If no value is specified, it is 20m by default.
    if ((float)hAccuracy/100.0 < LocationAccuracy)
    {
        FindCoordinates(latitude, longitude, hAccuracy);

    }
    // If loop MAX_LOOP_COUNT is reached before sufficient accuracy, give up and print error message.
    else if (locationLoopCounter >= MAX_LOOP_COUNT)
    {
        LE_ERROR("Failed to accurately locate your position. Please try again later.");
        GiveUp = true;
        le_gnss_Stop();
        le_thread_Cancel(PositionThreadRef);
    }

    locationLoopCounter++;
}