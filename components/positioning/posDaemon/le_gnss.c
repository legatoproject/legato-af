//--------------------------------------------------------------------------------------------------
/**
 * @file le_gnss.c
 *
 * This file contains the source code of the GNSS API.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------


#include "legato.h"
#include "interfaces.h"
#include "pa_gnss.h"


//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------

#define GNSS_POSITION_SAMPLE_MAX         1

/// Typically, we don't expect more than this number of concurrent activation requests.
#define GNSS_POSITION_ACTIVATION_MAX      13      // Ideally should be a prime number.

//--------------------------------------------------------------------------------------------------
/**
 * NMEA node path definition
 *
 */
//--------------------------------------------------------------------------------------------------
#define LE_GNSS_NMEA_NODE_PATH                  "/dev/nmea"

//--------------------------------------------------------------------------------------------------
/**
 * SV ID definitions corresponding to SBAS constellation categories
 *
 */
//--------------------------------------------------------------------------------------------------
// EGNOS SBAS category
#define SBAS_EGNOS_SV_ID_33   33
#define SBAS_EGNOS_SV_ID_37   37
#define SBAS_EGNOS_SV_ID_39   39
#define SBAS_EGNOS_SV_ID_44   44

// WAAS SBAS category
#define SBAS_WAAS_SV_ID_35    35
#define SBAS_WAAS_SV_ID_46    46
#define SBAS_WAAS_SV_ID_47    47
#define SBAS_WAAS_SV_ID_48    48
#define SBAS_WAAS_SV_ID_51    51

// GAGAN SBAS category
#define SBAS_GAGAN_SV_ID_40   40
#define SBAS_GAGAN_SV_ID_41   41

// MSAS SBAS category
#define SBAS_MSAS_SV_ID_42    42
#define SBAS_MSAS_SV_ID_50    50

//--------------------------------------------------------------------------------------------------
// Data structures.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Satellite Vehicle information.
 */
//--------------------------------------------------------------------------------------------------
typedef struct {
    uint16_t                satId;        ///< Satellite in View ID number [PRN].
    le_gnss_Constellation_t satConst;     ///< GNSS constellation type.
    bool                    satUsed;      ///< TRUE if satellite in View is used for fix Navigation.
    bool                    satTracked;   ///< TRUE if satellite in View is tracked for Navigation.
    uint8_t                 satSnr;       ///< Satellite in View Signal To Noise Ratio [dBHz].
    uint16_t                satAzim;      ///< Satellite in View Azimuth [degrees].
                                          ///< Range: 0 to 360
    uint8_t                 satElev;      ///< Satellite in View Elevation [degrees].
                                          ///< Range: 0 to 90
}
le_gnss_SvInfo_t;

//--------------------------------------------------------------------------------------------------
/**
 * Satellite measurement information.
 */
//--------------------------------------------------------------------------------------------------
typedef struct {
    uint16_t satId;          ///< Satellite in View ID number.
    int32_t  satLatency;     ///< Satellite latency measurement (age of measurement)
                             ///< Units: Milliseconds.
}
le_gnss_SvMeas_t;

//--------------------------------------------------------------------------------------------------
/**
 * Position Sample structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_gnss_PositionSample
{
    le_gnss_FixState_t fixState;     ///< Position Fix state
    bool            latitudeValid;   ///< if true, latitude is set
    int32_t         latitude;        ///< altitude
    bool            longitudeValid;  ///< if true, longitude is set
    int32_t         longitude;       ///< longitude
    bool            hAccuracyValid;  ///< if true, horizontal accuracy is set
    int32_t         hAccuracy;       ///< horizontal accuracy
    bool            altitudeValid;   ///< if true, altitude is set
    int32_t         altitude;        ///< altitude
    bool            altitudeOnWgs84Valid; ///< if true, altitude with respect to the WGS-84 is set
    int32_t         altitudeOnWgs84;      ///< altitude with respect to the WGS-84 ellipsoid
    bool            vAccuracyValid;  ///< if true, vertical accuracy is set
    int32_t         vAccuracy;       ///< vertical accuracy
    bool            hSpeedValid;     ///< if true, horizontal speed is set
    uint32_t        hSpeed;          ///< horizontal speed
    bool            hSpeedAccuracyValid; ///< if true, horizontal speed accuracy is set
    int32_t         hSpeedAccuracy;  ///< horizontal speed accuracy
    bool            vSpeedValid;     ///< if true, vertical speed is set
    int32_t         vSpeed;          ///< vertical speed
    bool            vSpeedAccuracyValid; ///< if true, vertical speed accuracy is set
    int32_t         vSpeedAccuracy;  ///< vertical speed accuracy
    bool            directionValid;  ///< if true, direction is set
    uint32_t        direction;       ///< direction
    bool            directionAccuracyValid; ///< if true, direction accuracy is set
    uint32_t        directionAccuracy; ///< direction accuracy
    bool            dateValid;       ///< if true, date is set
    uint16_t        year;            ///< UTC Year A.D. [e.g. 2014].
    uint16_t        month;           ///< UTC Month into the year [range 1...12].
    uint16_t        day;             ///< UTC Days into the month [range 1...31].
    bool            timeValid;       ///< if true, time is set
    uint16_t        hours;           ///< UTC Hours into the day [range 0..23].
    uint16_t        minutes;         ///< UTC Minutes into the hour [range 0..59].
    uint16_t        seconds;         ///< UTC Seconds into the minute [range 0..59].
    uint16_t        milliseconds;    ///< UTC Milliseconds into the second [range 0..999].
    uint64_t        epochTime;       ///< Epoch time in milliseconds since Jan. 1, 1970
    bool            gpsTimeValid;    ///< if true, GPS time is set
    uint32_t        gpsWeek;         ///< GPS week number from midnight, Jan. 6, 1980.
    uint32_t        gpsTimeOfWeek;   ///< Amount of time in milliseconds into the GPS week.
    bool            timeAccuracyValid; ///< if true, timeAccuracy is set
    uint32_t        timeAccuracy;      ///< Estimated Accuracy for time in milliseconds
    bool            positionLatencyValid; ///< if true, positionLatency is set
    uint32_t        positionLatency;      ///< Position measurement latency in milliseconds
    bool            hdopValid;       ///< if true, horizontal dilution is set
    uint16_t        hdop;            ///< The horizontal Dilution of Precision (DOP)
    bool            vdopValid;       ///< if true, vertical dilition is set
    uint16_t        vdop;            ///< The vertical Dilution of Precision (DOP)
    bool            pdopValid;       ///< if true, position dilution is set
    uint16_t        pdop;            ///< The Position dilution of precision (DOP)
    bool            magneticDeviationValid; ///< if true, magnetic deviation is set
    int32_t         magneticDeviation;  ///< The magnetic deviation
    // Satellite Vehicles information
    bool             satsInViewCountValid;   ///< if true, satsInViewCount is set
    uint8_t          satsInViewCount;        ///< Satellites in View count.
    bool             satsTrackingCountValid; ///< if true, satsTrackingCount is set
    uint8_t          satsTrackingCount;      ///< Tracking satellites in View count.
    bool             satsUsedCountValid;     ///< if true, satsUsedCount is set
    uint8_t          satsUsedCount;          ///< Satellites in View used for Navigation.
    bool             satInfoValid;           ///< if true, satInfo is set
    le_gnss_SvInfo_t satInfo[LE_GNSS_SV_INFO_MAX_LEN];
    bool             satMeasValid;           ///< if true, satMeas is set
    le_gnss_SvMeas_t satMeas[LE_GNSS_SV_INFO_MAX_LEN];
                                             ///< Satellite Vehicle measurement information.
    le_dls_Link_t   link;                    ///< Object node link
}
le_gnss_PositionSample_t;

//--------------------------------------------------------------------------------------------------
/**
 * Position Sample's Handler structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_gnss_PositionHandler
{
    le_gnss_PositionHandlerFunc_t handlerFuncPtr;      ///< The handler function address.
    void*                         handlerContextPtr;   ///< The handler function context.
    le_dls_Link_t                 link;                ///< Object node link
}
le_gnss_PositionHandler_t;

//--------------------------------------------------------------------------------------------------
// Static declarations.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Maintains the GNSS device state.
 */
//--------------------------------------------------------------------------------------------------
static le_gnss_State_t GnssState = LE_GNSS_STATE_UNINITIALIZED;


//--------------------------------------------------------------------------------------------------
/**
 * Memory Pool for position sample's handlers.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t   PositionHandlerPoolRef;

//--------------------------------------------------------------------------------------------------
/**
 * PA handler's reference.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_event_HandlerRef_t PaHandlerRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * PA NMEA handler's reference.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_event_HandlerRef_t PaNmeaHandlerRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Number of position Handler functions that own position samples.
 *
 */
//--------------------------------------------------------------------------------------------------
static int32_t NumOfPositionHandlers = 0;

//--------------------------------------------------------------------------------------------------
/**
 * Create and initialize the position sample's handlers list.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_dls_List_t PositionHandlerList = LE_DLS_LIST_INIT;

//--------------------------------------------------------------------------------------------------
/**
 * Memory Pool for position samples.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_gnss_PositionSample_t   LastPositionSample;

//--------------------------------------------------------------------------------------------------
/**
 * Memory Pool for position samples.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t   PositionSamplePoolRef;

//--------------------------------------------------------------------------------------------------
/**
 * Create and initialize the position samples list.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_dls_List_t PositionSampleList = LE_DLS_LIST_INIT;

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for Positioning Sample objects.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t PositionSampleMap;

//--------------------------------------------------------------------------------------------------
/**
 * NMEA pipe file descriptor
 */
//--------------------------------------------------------------------------------------------------
static int NmeaPipeFd = -1;

//--------------------------------------------------------------------------------------------------
/**
 * Position Handler destructor.
 *
 */
//--------------------------------------------------------------------------------------------------
static void PositionHandlerDestructor
(
    void* obj
)
{
    le_gnss_PositionHandler_t *positionHandlerNodePtr;
    le_dls_Link_t          *linkPtr;

    linkPtr = le_dls_Peek(&PositionHandlerList);
    if (linkPtr != NULL)
    {
        do
        {
            // Get the node from the list
            positionHandlerNodePtr =
                (le_gnss_PositionHandler_t*)CONTAINER_OF(linkPtr, le_gnss_PositionHandler_t, link);
            // Check the node.
            if ( positionHandlerNodePtr == (le_gnss_PositionHandler_t*)obj)
            {
                // Remove the node.
                le_dls_Remove(&PositionHandlerList, linkPtr);
                linkPtr=NULL;
            }
            else
            {
                // Move to the next node.
                linkPtr = le_dls_PeekNext(&PositionHandlerList, linkPtr);
            }
        } while (linkPtr != NULL);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Position Sample destructor.
 *
 */
//--------------------------------------------------------------------------------------------------
static void PositionSampleDestructor
(
    void* obj
)
{
    le_gnss_PositionSample_t *positionSampleNodePtr;
    le_dls_Link_t   *linkPtr;

    LE_FATAL_IF((obj == NULL), "Position Sample Object does not exist!");

    linkPtr = le_dls_Peek(&PositionSampleList);
    if (linkPtr != NULL)
    {
        do
        {
            // Get the node from the list
            positionSampleNodePtr = (le_gnss_PositionSample_t*)
                                    CONTAINER_OF(linkPtr, le_gnss_PositionSample_t, link);
            // Check the node.
            if ( positionSampleNodePtr == (le_gnss_PositionSample_t*)obj)
            {
                // Remove the node.
                le_dls_Remove(&PositionSampleList, linkPtr);
                linkPtr=NULL;
            }
            else
            {
                // Move to the next node.
                linkPtr = le_dls_PeekNext(&PositionSampleList, linkPtr);
            }
        } while (linkPtr != NULL);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Create NMEA named pipe (FIFO)
 */
//--------------------------------------------------------------------------------------------------
static void CreateNmeaPipe
(
    void
)
{
    int result = 0;

    LE_DEBUG("Create %s", LE_GNSS_NMEA_NODE_PATH);

    // Create the node for /dev/nmea device folder
    umask(0);
    result = mknod(LE_GNSS_NMEA_NODE_PATH, S_IFIFO|0666, 0);

    LE_ERROR_IF((result != 0)&&(result != EEXIST)
            , "Could not create %s. errno.%d (%s)", LE_GNSS_NMEA_NODE_PATH, errno, strerror(errno));
}

//--------------------------------------------------------------------------------------------------
/**
 * Open the NMEA pipe
 */
//--------------------------------------------------------------------------------------------------
static le_result_t OpenNmeaPipe
(
    void
)
{

    // Check NMEA pipe file descriptor
    if(NmeaPipeFd != -1)
    {
        return LE_DUPLICATE;
    }

    // Open NMEA pipe
    do
    {
        if ( ((NmeaPipeFd=open(LE_GNSS_NMEA_NODE_PATH,
                                O_WRONLY|O_APPEND|O_CLOEXEC|O_NONBLOCK)) == -1)
            && (errno != EINTR) )
        {
            LE_WARN_IF( errno != 6, "Open %s failure: errno.%d (%s)"
                    , LE_GNSS_NMEA_NODE_PATH, errno, strerror(errno));
            return LE_FAULT;
        }
    }while (NmeaPipeFd==-1);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Close the NMEA pipe
 */
//--------------------------------------------------------------------------------------------------
static le_result_t CloseNmeaPipe
(
    void
)
{
    le_result_t retResult = LE_OK;
    int result;

    // Check NMEA pipe file descriptor
    if(NmeaPipeFd == -1)
    {
        LE_WARN("Invalid file descriptor. File already closed");
        return LE_DUPLICATE;
    }

    // Close NMEA pipe
    do
    {
        result = close(NmeaPipeFd);
    }
    while ( (result != 0) && (errno == EINTR) );

    LE_ERROR_IF(result != 0, "Could not close %s. errno.%d (%s)"
                , LE_GNSS_NMEA_NODE_PATH, errno, strerror(errno));

    if(result != 0)
    {
        retResult = LE_FAULT;
    }
    else
    {
        // Release the NMEA pipe file descriptor
        NmeaPipeFd = -1;
    }

    return retResult;
}

//--------------------------------------------------------------------------------------------------
/**
 * Write the NMEA sentence to NMEA pipe
 */
//--------------------------------------------------------------------------------------------------
static le_result_t WriteNmeaPipe
(
    const char*  nmeaStringPtr          ///< [IN] Pointer to the NMEA sentence to write.
)
{
    le_result_t resultNmeaPipe = LE_OK;
    ssize_t resultWrite = 0;
    ssize_t written = 0;
    ssize_t stringSize = 0;

    // Open the NMEA FIFO pipe
    resultNmeaPipe = OpenNmeaPipe();

    if((resultNmeaPipe == LE_OK)||(resultNmeaPipe == LE_DUPLICATE))
    {
        written = 0;
        stringSize = strlen(nmeaStringPtr)+1;
        // Write to NMEA pipe
        while (written < stringSize)
        {
            resultWrite = write(NmeaPipeFd, nmeaStringPtr + written, stringSize - written);

            if ((resultWrite < 0) && (errno != EINTR))
            {
                LE_ERROR("Could not write to %s (write error, errno.%d (%s))",
                         LE_GNSS_NMEA_NODE_PATH, errno, strerror(errno));
                CloseNmeaPipe();
                return LE_FAULT;
            }

            written += resultWrite;
        }
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * The PA NMEA Handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static void PaNmeaHandler
(
    char* nmeaPtr
)
{
    LE_DEBUG("Handler Function called with PA NMEA %p", nmeaPtr);

    // Write the NMEA sentence to the /dev/nmea device folder
    WriteNmeaPipe(nmeaPtr);

    le_mem_Release(nmeaPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Fills in the position sample data by parsing the PA position data report.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static void GetPosSampleData
(
    le_gnss_PositionSample_t* posSampleDataPtr,  // [OUT] Pointer to the position sample report.
    pa_Gnss_Position_t* paPosDataPtr             // [IN] The PA position data report.
)
{
    uint8_t i;
    // Position information
    posSampleDataPtr->fixState = paPosDataPtr->fixState;
    posSampleDataPtr->latitudeValid = paPosDataPtr->latitudeValid;
    posSampleDataPtr->latitude = paPosDataPtr->latitude;
    posSampleDataPtr->longitudeValid = paPosDataPtr->longitudeValid;
    posSampleDataPtr->longitude = paPosDataPtr->longitude;
    posSampleDataPtr->hAccuracyValid = paPosDataPtr->hUncertaintyValid;
    posSampleDataPtr->hAccuracy = paPosDataPtr->hUncertainty;
    posSampleDataPtr->altitudeValid = paPosDataPtr->altitudeValid;
    posSampleDataPtr->altitude = paPosDataPtr->altitude;
    posSampleDataPtr->altitudeOnWgs84Valid = paPosDataPtr->altitudeOnWgs84Valid;
    posSampleDataPtr->altitudeOnWgs84 = paPosDataPtr->altitudeOnWgs84;
    posSampleDataPtr->vAccuracyValid = paPosDataPtr->vUncertaintyValid;
    posSampleDataPtr->vAccuracy = paPosDataPtr->vUncertainty;
    posSampleDataPtr->hSpeedValid = paPosDataPtr->hSpeedValid;
    posSampleDataPtr->hSpeed = paPosDataPtr->hSpeed;
    posSampleDataPtr->hSpeedAccuracyValid = paPosDataPtr->hSpeedUncertaintyValid;
    posSampleDataPtr->hSpeedAccuracy = paPosDataPtr->hSpeedUncertainty;
    posSampleDataPtr->vSpeedValid = paPosDataPtr->vSpeedValid;
    posSampleDataPtr->vSpeed = paPosDataPtr->vSpeed;
    posSampleDataPtr->vSpeedAccuracyValid = paPosDataPtr->vSpeedUncertaintyValid;
    posSampleDataPtr->vSpeedAccuracy = paPosDataPtr->vSpeedUncertainty;
    posSampleDataPtr->directionValid = paPosDataPtr->directionValid;
    posSampleDataPtr->direction = paPosDataPtr->direction;
    posSampleDataPtr->directionAccuracyValid = paPosDataPtr->directionUncertaintyValid;
    posSampleDataPtr->directionAccuracy = paPosDataPtr->directionUncertainty;
    posSampleDataPtr->magneticDeviationValid = paPosDataPtr->magneticDeviationValid;
    posSampleDataPtr->magneticDeviation = paPosDataPtr->magneticDeviation;
    // Date
    posSampleDataPtr->dateValid = paPosDataPtr->dateValid;
    posSampleDataPtr->year = paPosDataPtr->date.year;
    posSampleDataPtr->month = paPosDataPtr->date.month;
    posSampleDataPtr->day = paPosDataPtr->date.day;
    // UTC time
    posSampleDataPtr->timeValid = paPosDataPtr->timeValid;
    posSampleDataPtr->hours = paPosDataPtr->time.hours;
    posSampleDataPtr->minutes = paPosDataPtr->time.minutes;
    posSampleDataPtr->seconds = paPosDataPtr->time.seconds;
    posSampleDataPtr->milliseconds = paPosDataPtr->time.milliseconds;
    // Epoch time
    posSampleDataPtr->epochTime = paPosDataPtr->epochTime;
    // GPS time
    posSampleDataPtr->gpsTimeValid = paPosDataPtr->gpsTimeValid;
    posSampleDataPtr->gpsWeek = paPosDataPtr->gpsWeek;
    posSampleDataPtr->gpsTimeOfWeek = paPosDataPtr->gpsTimeOfWeek;
    // Time accuracy
    posSampleDataPtr->timeAccuracyValid = paPosDataPtr->timeAccuracyValid;
    posSampleDataPtr->timeAccuracy = paPosDataPtr->timeAccuracy;

    // Position measurement latency
    posSampleDataPtr->positionLatencyValid = paPosDataPtr->positionLatencyValid;
    posSampleDataPtr->positionLatency = paPosDataPtr->positionLatency;

    // DOP parameters
    posSampleDataPtr->hdopValid = paPosDataPtr->hdopValid;
    posSampleDataPtr->hdop = paPosDataPtr->hdop;
    posSampleDataPtr->vdopValid = paPosDataPtr->vdopValid;
    posSampleDataPtr->vdop = paPosDataPtr->vdop;
    posSampleDataPtr->pdopValid = paPosDataPtr->pdopValid;
    posSampleDataPtr->pdop = paPosDataPtr->pdop;
    // Satellites information
    posSampleDataPtr->satsInViewCountValid = paPosDataPtr->satsInViewCountValid;
    posSampleDataPtr->satsInViewCount = paPosDataPtr->satsInViewCount;
    posSampleDataPtr->satsTrackingCountValid = paPosDataPtr->satsTrackingCountValid;
    posSampleDataPtr->satsTrackingCount = paPosDataPtr->satsTrackingCount;
    posSampleDataPtr->satsUsedCountValid = paPosDataPtr->satsUsedCountValid;
    posSampleDataPtr->satsUsedCount = paPosDataPtr->satsUsedCount;
    posSampleDataPtr->satInfoValid = paPosDataPtr->satInfoValid;
    for(i=0; i<LE_GNSS_SV_INFO_MAX_LEN; i++)
    {
        posSampleDataPtr->satInfo[i].satId = paPosDataPtr->satInfo[i].satId;
        posSampleDataPtr->satInfo[i].satConst = paPosDataPtr->satInfo[i].satConst;
        posSampleDataPtr->satInfo[i].satUsed = paPosDataPtr->satInfo[i].satUsed;
        posSampleDataPtr->satInfo[i].satTracked = paPosDataPtr->satInfo[i].satTracked;
        posSampleDataPtr->satInfo[i].satSnr = paPosDataPtr->satInfo[i].satSnr;
        posSampleDataPtr->satInfo[i].satAzim = paPosDataPtr->satInfo[i].satAzim;
        posSampleDataPtr->satInfo[i].satElev = paPosDataPtr->satInfo[i].satElev;
    }

    // Satellite latency measurement
    posSampleDataPtr->satMeasValid = paPosDataPtr->satMeasValid;
    for(i=0; i<LE_GNSS_SV_INFO_MAX_LEN; i++)
    {
        posSampleDataPtr->satMeas[i].satId = paPosDataPtr->satMeas[i].satId;
        posSampleDataPtr->satMeas[i].satLatency = paPosDataPtr->satMeas[i].satLatency;
    }

    // Node Link
    posSampleDataPtr->link = LE_DLS_LINK_INIT;

    return;
}


//--------------------------------------------------------------------------------------------------
/**
 * The signal event handler function for SIGPIPE called from the Legato event loop.
 *
 * If the read end of a pipe is closed, then a write to the pipe will cause a SIGPIPE signal for the
 * calling process and this process will be killed.
 * By catching the SIGPIPE signal, the write to the pipe will only cause a write error.
 */
//--------------------------------------------------------------------------------------------------
static void SigPipeHandler
(
    int sigNum
)
{
    LE_FATAL_IF(sigNum != SIGPIPE, "Unknown signal %s.", strsignal(sigNum));
    LE_INFO("%s received through SigPipeHandler.", strsignal(sigNum));
}



//--------------------------------------------------------------------------------------------------
// APIs.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * The PA position Handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static void PaPositionHandler
(
    pa_Gnss_Position_t* positionPtr
)
{

    le_gnss_PositionHandler_t*  positionHandlerNodePtr;
    le_dls_Link_t*              linkPtr;
    le_gnss_PositionSample_t*   positionSampleNodePtr=NULL;
    uint8_t i;

    if (NULL == positionPtr)
    {
        LE_ERROR("positionPtr is Null");
        return;
    }

    LE_DEBUG("Handler Function called with PA position %p", positionPtr);

    // Get the position sample data from the PA position data report
    GetPosSampleData(&LastPositionSample, positionPtr);

    if(!NumOfPositionHandlers)
    {
        LE_DEBUG("No positioning handlers, exit Handler Function");
        le_mem_Release(positionPtr);
        return;
    }

    linkPtr = le_dls_Peek(&PositionHandlerList);
    if (linkPtr != NULL)
    {
        // Create the position sample node.
        positionSampleNodePtr =
                    (le_gnss_PositionSample_t*)le_mem_ForceAlloc(PositionSamplePoolRef);

        // Copy the position sample to the position sample node
        memcpy(positionSampleNodePtr, &LastPositionSample, sizeof(le_gnss_PositionSample_t));

        // Add the node to the queue of the list by passing in the node's link.
        le_dls_Queue(&PositionSampleList, &(positionSampleNodePtr->link));

        // Add reference for each subscribed handler
        for(i=0 ; i<NumOfPositionHandlers-1 ; i++)
        {
            le_mem_AddRef((void *)positionSampleNodePtr);
        }

        // Call Handler(s)
        do
        {
            // Get the node from the list
            positionHandlerNodePtr =
                (le_gnss_PositionHandler_t*)CONTAINER_OF(linkPtr, le_gnss_PositionHandler_t, link);

            LE_DEBUG("Report sample %p to the corresponding handler (handler %p)",
                     positionSampleNodePtr,
                     positionHandlerNodePtr->handlerFuncPtr);

            // Create a safe reference and call the client's handler
            void* safePositionSampleRef = le_ref_CreateRef(PositionSampleMap,
                                                           positionSampleNodePtr);
            if(safePositionSampleRef != NULL)
            {
                positionHandlerNodePtr->handlerFuncPtr(safePositionSampleRef
                                                    , positionHandlerNodePtr->handlerContextPtr);
            }
            // Move to the next node.
            linkPtr = le_dls_PeekNext(&PositionHandlerList, linkPtr);
        } while (linkPtr != NULL);
    }

    le_mem_Release(positionPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the GNSS
 *
 *  - LE_FAULT  The function failed.
 *  - LE_NOT_PERMITTED If the GNSS device is already initialized.
 *  - LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t gnss_Init
(
    void
)
{
    le_result_t result = LE_FAULT;
    struct stat nmeaFileStat;
    int resultStat = 0;

    LE_DEBUG("gnss_Init");

    // Check the GNSS device state
    switch (GnssState)
    {
        case LE_GNSS_STATE_UNINITIALIZED:
        {
            result = pa_gnss_Init();
            if(result == LE_OK)
            {
                GnssState = LE_GNSS_STATE_READY;
            }
        }
        break;
        case LE_GNSS_STATE_READY:
        case LE_GNSS_STATE_ACTIVE:
        case LE_GNSS_STATE_DISABLED:
        {
            LE_ERROR("Bad state for that request [%d]", GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        default:
        {
            LE_ERROR("Unknown GNSS state [%d]", GnssState);
            result = LE_FAULT;
        }
        break;
    }

    // Block signals.  All signals that are to be used in signal events must be blocked.
    le_sig_Block(SIGPIPE);

    // Register a signal event handler for SIGPIPE signal.
    le_sig_SetEventHandler(SIGPIPE, SigPipeHandler);

    // Create a pool for Position  Handler objects
    PositionHandlerPoolRef = le_mem_CreatePool("PositionHandlerPoolRef"
                                                , sizeof(le_gnss_PositionHandler_t));
    le_mem_SetDestructor(PositionHandlerPoolRef, PositionHandlerDestructor);

    // Create a pool for Position Sample objects
    PositionSamplePoolRef = le_mem_CreatePool("PositionSamplePoolRef"
                                            , sizeof(le_gnss_PositionSample_t));
    le_mem_ExpandPool(PositionSamplePoolRef,GNSS_POSITION_SAMPLE_MAX);
    le_mem_SetDestructor(PositionSamplePoolRef, PositionSampleDestructor);

    // Create the reference HashMap for positioning sample
    PositionSampleMap = le_ref_CreateMap("PositionSampleMap", GNSS_POSITION_SAMPLE_MAX);

    // Initialize Handler context
    NumOfPositionHandlers = 0;
    PaHandlerRef = NULL;

    // Initialize last Position sample
    memset(&LastPositionSample, 0, sizeof(LastPositionSample) );
    LastPositionSample.fixState = LE_GNSS_STATE_FIX_NO_POS;

    // Subscribe to PA position Data handler
    if ((PaHandlerRef=pa_gnss_AddPositionDataHandler(PaPositionHandler)) == NULL)
    {
        LE_ERROR("Failed to add PA position Data handler!");
        return LE_FAULT;
    }

    // NMEA pipe management
    // Get information from NMEA device file
    resultStat = stat(LE_GNSS_NMEA_NODE_PATH, &nmeaFileStat);
    // That node is a character device file: it will be managed from the Firmware (Kernel space).
    // That node is a FIFO (named pipe): it will be managed from Legato (User space).
    if ((resultStat == 0) && (S_ISFIFO(nmeaFileStat.st_mode))) // FIFO (named pipe)
    {
         if ((PaNmeaHandlerRef=pa_gnss_AddNmeaHandler(PaNmeaHandler)) == NULL)
         {
             LE_ERROR("Failed to add PA NMEA handler!");
         }
    }
    else if ((resultStat == 0 )&& (S_ISCHR(nmeaFileStat.st_mode))) // Character device file
    {
        LE_INFO("%s is a character device file", LE_GNSS_NMEA_NODE_PATH);
    }
    else if((resultStat == -1)&&(errno == ENOENT)) // No such file or directory
    {
        if ((PaNmeaHandlerRef=pa_gnss_AddNmeaHandler(PaNmeaHandler)) != NULL)
        {
            // Create NMEA device folder
            CreateNmeaPipe();
        }
        else
        {
            LE_ERROR("Failed to add PA NMEA handler!");
        }
    }
    else
    {
        LE_ERROR("Could not get file info for '%s'. errno.%d (%s)"
                , LE_GNSS_NMEA_NODE_PATH, errno, strerror(errno));
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register an handler for position notifications.
 *
 *  - A handler reference, which is only needed for later removal of the handler.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_gnss_PositionHandlerRef_t le_gnss_AddPositionHandler
(
    le_gnss_PositionHandlerFunc_t handlerPtr,          ///< [IN] The handler function.
    void*                        contextPtr           ///< [IN] The context pointer
)
{
    le_gnss_PositionHandler_t*  positionHandlerPtr=NULL;

    LE_FATAL_IF((handlerPtr == NULL), "handlerPtr pointer is NULL !");

    // Create the position sample handler node.
    positionHandlerPtr = (le_gnss_PositionHandler_t*)le_mem_ForceAlloc(PositionHandlerPoolRef);
    positionHandlerPtr->handlerFuncPtr = handlerPtr;
    positionHandlerPtr->handlerContextPtr = contextPtr;

    LE_DEBUG("handler %p", handlerPtr);

    // Subscribe to PA position Data handler
    if (PaHandlerRef == NULL)
    {
        if ((PaHandlerRef=pa_gnss_AddPositionDataHandler(PaPositionHandler)) == NULL)
        {
            LE_ERROR("Failed to add PA position Data handler!");
        }
        else
        {
            LE_DEBUG("PaHandlerRef %p subscribed", PaHandlerRef);
        }
    }

    // Update the position handler list with that new handler
    le_dls_Queue(&PositionHandlerList, &(positionHandlerPtr->link));
    NumOfPositionHandlers++;

    LE_DEBUG("Position handler %p added",
             positionHandlerPtr->handlerFuncPtr);

    return (le_gnss_PositionHandlerRef_t)positionHandlerPtr;

}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to remove a handler for position notifications.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
void le_gnss_RemovePositionHandler
(
    le_gnss_PositionHandlerRef_t    handlerRef ///< [IN] The handler reference.
)
{
    le_gnss_PositionHandler_t* positionHandlerNodePtr;
    le_dls_Link_t*          linkPtr;

    linkPtr = le_dls_Peek(&PositionHandlerList);
    if (linkPtr != NULL)
    {
        do
        {
            // Get the node from MsgList
            positionHandlerNodePtr =
                (le_gnss_PositionHandler_t*)CONTAINER_OF(linkPtr, le_gnss_PositionHandler_t, link);
            // Check the node.
            if ( (le_gnss_PositionHandlerRef_t)positionHandlerNodePtr == handlerRef )
            {
                // Remove the node.
                le_mem_Release(positionHandlerNodePtr);
                NumOfPositionHandlers--;
                linkPtr=NULL;
            }
            else
            {
                // Move to the next node.
                linkPtr = le_dls_PeekNext(&PositionHandlerList, linkPtr);
            }
        } while (linkPtr != NULL);
    }

    if(NumOfPositionHandlers == 0)
    {
        pa_gnss_RemovePositionDataHandler(PaHandlerRef);
        PaHandlerRef = NULL;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the position sample's fix state
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 *
 * @note If the caller is passing an invalid Position sample reference into this function,
 *       it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetPositionState
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN]
        ///< Position sample's reference.

    le_gnss_FixState_t* statePtr
        ///< [OUT]
        ///< Position fix state.
)
{
    le_result_t result = LE_OK;
    le_gnss_PositionSample_t* positionSamplePtr =
                                                le_ref_Lookup(PositionSampleMap,positionSampleRef);

    // Check position sample's reference
    if ( positionSamplePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!",positionSampleRef);
        return LE_FAULT;
    }

    // Check input pointers
    if (statePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid pointer provided!");
        return LE_FAULT;
    }

    // Get the position Fix state
    *statePtr = positionSamplePtr->fixState;

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the location's data (Latitude, Longitude, Horizontal accuracy).
 *
 * @return
 *  - LE_FAULT         Function failed to get the location's data
 *  - LE_OUT_OF_RANGE  One of the retrieved parameter is invalid (set to INT32_MAX).
 *  - LE_OK            Function succeeded.
 *
 * @note latitudePtr, longitudePtr,hAccuracyPtr, altitudePtr, vAccuracyPtr can be set to NULL
 *       if not needed.
 *
 * @note: The latitude and longitude values are based on the WGS84 standard coordinate system.
 *
 * @note The latitude and longitude are given in degrees with 6 decimal places like:
 *    Latitude +48858300 = 48.858300 degrees North
 *    Longitude +2294400 = 2.294400 degrees East
 *
 * @note If the caller is passing an invalid Position sample reference into this function,
 *       it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetLocation
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN]
        ///< Position sample's reference.

    int32_t* latitudePtr,
        ///< [OUT]
        ///< WGS84 Latitude in degrees, positive North [resolution 1e-6].

    int32_t* longitudePtr,
        ///< [OUT]
        ///< WGS84 Longitude in degrees, positive East [resolution 1e-6].

    int32_t* hAccuracyPtr
        ///< [OUT]
        ///< Horizontal position's accuracy in meters [resolution 1e-2].
)
{
    le_result_t result = LE_OK;
    le_gnss_PositionSample_t* positionSamplePtr
                                            = le_ref_Lookup(PositionSampleMap,positionSampleRef);

    if ( positionSamplePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!",positionSampleRef);
        return LE_FAULT;
    }

    if (latitudePtr)
    {
        if (positionSamplePtr->latitudeValid)
        {
            *latitudePtr = positionSamplePtr->latitude;
        }
        else
        {
            *latitudePtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (longitudePtr)
    {
        if (positionSamplePtr->longitudeValid)
        {
            *longitudePtr = positionSamplePtr->longitude;
        }
        else
        {
            *longitudePtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (hAccuracyPtr)
    {
        if (positionSamplePtr->hAccuracyValid)
        {
            *hAccuracyPtr = positionSamplePtr->hAccuracy;
        }
        else
        {
            *hAccuracyPtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the position sample's altitude.
 *
 * @return
 *  - LE_FAULT         Function failed to get the altitude. Invalid Position reference provided.
 *  - LE_OUT_OF_RANGE  One of the retrieved parameter is invalid (set to INT32_MAX).
 *  - LE_OK            Function succeeded.
 *
 * @note Altitude is in meters, above Mean Sea Level, with 3 decimal places (3047 = 3.047 meters).
 *
 * @note For a 2D position fix, the altitude will be indicated as invalid and set to INT32_MAX
 *
 * @note If the caller is passing an invalid Position reference into this function,
 *       it is a fatal error, the function will not return.
 *
 * @note altitudePtr, altitudeAccuracyPtr can be set to NULL if not needed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetAltitude
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN]
        ///< Position sample's reference.

    int32_t* altitudePtr,
        ///< [OUT]
        ///< Altitude in meters, above Mean Sea Level [resolution 1e-3].

    int32_t* vAccuracyPtr
        ///< [OUT]
        ///< Vertical position's accuracy in meters [resolution 1e-1].
)
{
    le_result_t result = LE_OK;
    le_gnss_PositionSample_t * positionSamplePtr
                                            = le_ref_Lookup(PositionSampleMap,positionSampleRef);

    if ( positionSamplePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!",positionSampleRef);
        return LE_FAULT;
    }

    if (altitudePtr)
    {
        if (positionSamplePtr->altitudeValid)
        {
            *altitudePtr = positionSamplePtr->altitude;
        }
        else
         {
            *altitudePtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (vAccuracyPtr)
    {
        if (positionSamplePtr->vAccuracyValid)
        {
            *vAccuracyPtr = positionSamplePtr->vAccuracy;
        }
        else
        {
            *vAccuracyPtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the position sample's time.
 *
 * @return
 *  - LE_FAULT         Function failed to get the time.
 *  - LE_OUT_OF_RANGE  The retrieved time is invalid (all fields are set to 0).
 *  - LE_OK            Function succeeded.
 *
 * @note If the caller is passing an invalid Position sample reference into this function,
 *       it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetTime
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN]
        ///< Position sample's reference.

    uint16_t* hoursPtr,
        ///< [OUT]
        ///< UTC Hours into the day [range 0..23].

    uint16_t* minutesPtr,
        ///< [OUT]
        ///< UTC Minutes into the hour [range 0..59].

    uint16_t* secondsPtr,
        ///< [OUT]
        ///< UTC Seconds into the minute [range 0..59].

    uint16_t* millisecondsPtr
        ///< [OUT]
        ///< UTC Milliseconds into the second [range 0..999].
)
{
    le_result_t result = LE_OK;
    le_gnss_PositionSample_t* positionSamplePtr
                                            = le_ref_Lookup(PositionSampleMap,positionSampleRef);

    // Check position sample's reference
    if ( positionSamplePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!",positionSampleRef);
        return LE_FAULT;
    }

    // Check input pointers
    if ((hoursPtr == NULL)
        || (minutesPtr == NULL)
        || (secondsPtr == NULL)
        || (millisecondsPtr == NULL))
    {
        LE_KILL_CLIENT("Invalid pointer provided!");
        return LE_FAULT;
    }

    // Get the position sample's time
    if (positionSamplePtr->timeValid)
    {
        result = LE_OK;
        if (hoursPtr)
        {
            *hoursPtr = positionSamplePtr->hours;
        }
        if (minutesPtr)
        {
            *minutesPtr = positionSamplePtr->minutes;
        }
        if (secondsPtr)
        {
            *secondsPtr = positionSamplePtr->seconds;
        }
        if (millisecondsPtr)
        {
            *millisecondsPtr = positionSamplePtr->milliseconds;
        }
    }
    else
    {
        result = LE_OUT_OF_RANGE;
        if (hoursPtr)
        {
            *hoursPtr = 0;
        }
        if (minutesPtr)
        {
            *minutesPtr = 0;
        }
        if (secondsPtr)
        {
            *secondsPtr = 0;
        }
        if (millisecondsPtr)
        {
            *millisecondsPtr = 0;
        }
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the position sample's GPS time.
 *
 * @return
 *  - LE_FAULT         Function failed to get the time.
 *  - LE_OUT_OF_RANGE  The retrieved time is invalid (all fields are set to 0).
 *  - LE_OK            Function succeeded.
 *
 * @note If the caller is passing an invalid Position sample reference into this function,
 *       it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetGpsTime
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN] Position sample's reference.

    uint32_t* gpsWeekPtr,
        ///< [OUT] GPS week number from midnight, Jan. 6, 1980.

    uint32_t* gpsTimeOfWeekPtr
        ///< [OUT] Amount of time in milliseconds into the GPS week.
)
{
    le_result_t result = LE_OK;
    le_gnss_PositionSample_t* positionSamplePtr
                                            = le_ref_Lookup(PositionSampleMap,positionSampleRef);

    // Check position sample's reference
    if ( positionSamplePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!",positionSampleRef);
        return LE_FAULT;
    }

    // Check input pointers
    if ((gpsWeekPtr == NULL)
        || (gpsTimeOfWeekPtr == NULL))
    {
        LE_KILL_CLIENT("Invalid pointer provided!");
        return LE_FAULT;
    }

    // Get the position sample's GPS time
    if (positionSamplePtr->timeValid)
    {
        result = LE_OK;
        if (gpsWeekPtr)
        {
            *gpsWeekPtr = positionSamplePtr->gpsWeek;
        }
        if (gpsTimeOfWeekPtr)
        {
            *gpsTimeOfWeekPtr = positionSamplePtr->gpsTimeOfWeek;
        }
    }
    else
    {
        result = LE_OUT_OF_RANGE;
        if (gpsWeekPtr)
        {
            *gpsWeekPtr = 0;
        }
        if (gpsTimeOfWeekPtr)
        {
            *gpsTimeOfWeekPtr = 0;
        }
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Get the position sample's epoch time.
 *
 * @return
 *  - LE_OK            Function succeeded.
 *  - LE_FAULT         Function failed to acquire the epoch time.
 *  - LE_OUT_OF_RANGE  The retrieved time is invalid (all fields are set to 0).
 *
 * @note The epoch time is the number of seconds elapsed since January 1, 1970
 *       (midnight UTC/GMT), not counting leaps seconds.
 *
 * @note If the caller is passing an invalid position sample reference into this function,
 *       it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetEpochTime
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN] Position sample's reference.

    uint64_t* millisecondsPtr
        ///< [OUT] Milliseconds since Jan. 1, 1970.
)
{
    le_result_t result;
    le_gnss_PositionSample_t* positionSamplePtr
                                            = le_ref_Lookup(PositionSampleMap,positionSampleRef);
    // Check position sample's reference
    if ( NULL == positionSamplePtr)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!",positionSampleRef);
        return LE_FAULT;
    }

    // Check input pointers
    if (NULL == millisecondsPtr)
    {
        LE_KILL_CLIENT("Invalid pointer provided!");
        return LE_FAULT;
    }

    // Get the position sample's time
    if (positionSamplePtr->timeValid)
    {
        // Get the epoch time
        result = LE_OK;
        *millisecondsPtr = positionSamplePtr->epochTime;
    }
    else
    {
        result = LE_OUT_OF_RANGE;
        *millisecondsPtr = 0;
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the position sample's time accurary.
 *
 * @return
 *  - LE_FAULT         Function failed to get the time.
 *  - LE_OUT_OF_RANGE  The retrieved time accuracy is invalid (set to UINT16_MAX).
 *  - LE_OK            Function succeeded.
 *
 * @note If the caller is passing an invalid Position sample reference into this function,
 *       it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetTimeAccuracy
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN] Position sample's reference.

    uint32_t* timeAccuracyPtr
        ///< [OUT] Estimated time accuracy in milliseconds
)
{
    le_result_t result = LE_OK;
    le_gnss_PositionSample_t* positionSamplePtr
                                            = le_ref_Lookup(PositionSampleMap,positionSampleRef);

    // Check position sample's reference
    if ( positionSamplePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!",positionSampleRef);
        return LE_FAULT;
    }

    // Check input pointers
    if (timeAccuracyPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid pointer provided!");
        return LE_FAULT;
    }

    // Get the position sample's time accuracy
    if (positionSamplePtr->timeAccuracyValid)
    {
        result = LE_OK;
        if (timeAccuracyPtr)
        {
            *timeAccuracyPtr = positionSamplePtr->timeAccuracy;
        }
    }
    else
    {
        result = LE_OUT_OF_RANGE;
        if (timeAccuracyPtr)
        {
            *timeAccuracyPtr = UINT16_MAX;
        }
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the position sample's date.
 *
 * @return
 *  - LE_FAULT         Function failed to get the date.
 *  - LE_OUT_OF_RANGE  The retrieved date is invalid (all fields are set to 0).
 *  - LE_OK            Function succeeded.
 *
 * @note If the caller is passing an invalid Position sample reference into this function,
 *       it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetDate
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN]
        ///< Position sample's reference.

    uint16_t* yearPtr,
        ///< [OUT]
        ///< UTC Year A.D. [e.g. 2014].

    uint16_t* monthPtr,
        ///< [OUT]
        ///< UTC Month into the year [range 1...12].

    uint16_t* dayPtr
        ///< [OUT]
        ///< UTC Days into the month [range 1...31].
)
{

    le_result_t result = LE_OK;
    le_gnss_PositionSample_t* positionSamplePtr =
                                                le_ref_Lookup(PositionSampleMap,positionSampleRef);

    // Check position sample's reference
    if ( positionSamplePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!",positionSampleRef);
        return LE_FAULT;
    }

    // Check input pointers
    if ((yearPtr == NULL)
        || (monthPtr == NULL)
        || (dayPtr == NULL))
    {
        LE_KILL_CLIENT("Invalid pointer provided!");
        return LE_FAULT;
    }

    // Get the position sample's date
    if (positionSamplePtr->dateValid)
    {
        result = LE_OK;
        if (yearPtr)
        {
            *yearPtr = positionSamplePtr->year;
        }
        if (monthPtr)
        {
            *monthPtr = positionSamplePtr->month;
        }
        if (dayPtr)
        {
            *dayPtr = positionSamplePtr->day;
        }
    }
    else
    {
        result = LE_OUT_OF_RANGE;
        if (yearPtr)
        {
            *yearPtr = 0;
        }
        if (monthPtr)
        {
            *monthPtr = 0;
        }
        if (dayPtr)
        {
            *dayPtr = 0;
        }
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the position sample's horizontal speed.
 *
 * @return
 *  - LE_FAULT         Function failed to find the positionSample.
 *  - LE_OUT_OF_RANGE  One of the retrieved parameter is invalid (set to UINT32_MAX).
 *  - LE_OK            Function succeeded.
 *
 * @note hSpeedPtr, hSpeedAccuracyPtr can be set to NULL if not needed.
 *
 * @note If the caller is passing an invalid Position sample reference into this function,
 *       it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetHorizontalSpeed
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN]
        ///< Position sample's reference.

    uint32_t* hspeedPtr,
        ///< [OUT]
        ///< Horizontal speed in meters/second [resolution 1e-2].

    uint32_t* hspeedAccuracyPtr
        ///< [OUT]
        ///< Horizontal speed's accuracy estimate
        ///< in meters/second [resolution 1e-1].
)
{
    le_result_t result = LE_OK;
    le_gnss_PositionSample_t* positionSamplePtr
                                            = le_ref_Lookup(PositionSampleMap,positionSampleRef);

    // Check position sample's reference
    if ( positionSamplePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!",positionSampleRef);
        return LE_FAULT;
    }

    if (hspeedPtr)
    {
        if (positionSamplePtr->hSpeedValid)
        {
            *hspeedPtr = positionSamplePtr->hSpeed;
        }
        else
        {
            *hspeedPtr = UINT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (hspeedAccuracyPtr)
    {
        if (positionSamplePtr->hSpeedAccuracyValid)
        {
            *hspeedAccuracyPtr = positionSamplePtr->hSpeedAccuracy;
        }
        else
        {
            *hspeedAccuracyPtr = UINT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the position sample's vertical speed.
 *
 * @return
 *  - LE_FAULT         The function failed to find the positionSample.
 *  - LE_OUT_OF_RANGE  One of the retrieved parameter is not valid (set to INT32_MAX).
 *  - LE_OK            The function succeeded.
 *
 * @note vSpeedPtr, vSpeedAccuracyPtr can be set to NULL if not needed.
 *
 * @note For a 2D position Fix, the vertical speed will be indicated as invalid
 * and set to INT32_MAX.
 *
 * @note If the caller is passing an invalid Position sample reference into this function,
 *       it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetVerticalSpeed
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN]
        ///< Position sample's reference.

    int32_t* vspeedPtr,
        ///< [OUT]
        ///< Vertical speed in meters/second [resolution 1e-2].

    int32_t* vspeedAccuracyPtr
        ///< [OUT]
        ///< Vertical speed's accuracy estimate
        ///< in meters/second [resolution 1e-1].
)
{
    le_result_t result = LE_OK;
    le_gnss_PositionSample_t* positionSamplePtr
                                            = le_ref_Lookup(PositionSampleMap,positionSampleRef);

    // Check position sample's reference
    if ( positionSamplePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!",positionSampleRef);
        return LE_FAULT;
    }

    if (vspeedPtr)
    {
        if (positionSamplePtr->vSpeedValid)
        {
            *vspeedPtr = positionSamplePtr->vSpeed;
        }
        else
        {
            *vspeedPtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (vspeedAccuracyPtr)
    {
        if (positionSamplePtr->vSpeedAccuracyValid)
        {
            *vspeedAccuracyPtr = positionSamplePtr->vSpeedAccuracy;
        }
        else
        {
            *vspeedAccuracyPtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the position sample's direction. Direction of movement is the direction that the vehicle or
 * person is actually moving.
 *
 * @return
 *  - LE_FAULT         Function failed to find the positionSample.
 *  - LE_OUT_OF_RANGE  One of the retrieved parameter is invalid (set to UINT32_MAX).
 *  - LE_OK            Function succeeded.
 *
 * @note Direction is given in degrees with 1 decimal place: 1755 = 175,5 degrees.
 *       Direction ranges from 0 to 359.9 degrees, where 0 is True North.
 *
 * @note directionPtr, directionAccuracyPtr can be set to NULL if not needed.
 *
 * @note If the caller is passing an invalid Position sample reference into this function,
 *       it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetDirection
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN]
        ///< Position sample's reference.

    uint32_t* directionPtr,
        ///< [OUT]
        ///< Direction in degrees [resolution 1e-1].
        ///< Range: 0 to 359.9, where 0 is True North.

    uint32_t* directionAccuracyPtr
        ///< [OUT]
        ///< Direction's accuracy estimate in degrees [resolution 1e-1].
)
{
    le_result_t result = LE_OK;
    le_gnss_PositionSample_t* positionSamplePtr
                                            = le_ref_Lookup(PositionSampleMap,positionSampleRef);

    // Check position sample's reference
    if ( positionSamplePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!",positionSampleRef);
        return LE_FAULT;
    }

    if (directionPtr)
    {
        if (positionSamplePtr->directionValid)
        {
            *directionPtr = positionSamplePtr->direction;
        }
        else
        {
            *directionPtr = UINT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (directionAccuracyPtr)
    {
        if (positionSamplePtr->directionAccuracyValid)
        {
            *directionAccuracyPtr = positionSamplePtr->directionAccuracy;
        }
        else
        {
            *directionAccuracyPtr = UINT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the Satellites Vehicle information.
 *
 * @return
 *  - LE_FAULT         Function failed to find the positionSample.
 *  - LE_OUT_OF_RANGE  One of the retrieved parameter is invalid.
 *  - LE_OK            Function succeeded.
 *
 * @note satId[] can be set to 0 if that information list index is not configured, so
 * all satellite parameters (satConst[], satSnr[],satAzim[], satElev[]) are fixed to 0.
 *
 * @note For LE_OUT_OF_RANGE returned code, invalid value depends on field type:
 * UINT16_MAX for satId, LE_GNSS_SV_CONSTELLATION_UNDEFINED for satConst, false for satUsed,
 * UINT8_MAX for satSnr, UINT16_MAX for satAzim, UINT8_MAX for satElev.
 *
 * @note If the caller is passing an invalid Position sample reference into this function,
 *       it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetSatellitesInfo
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN]
        ///< Position sample's reference.

    uint16_t* satIdPtr,
        ///< [OUT]
        ///< Satellites in View ID number, referring
        ///< to NMEA standard.

    size_t* satIdNumElementsPtr,
        ///< [INOUT]

    le_gnss_Constellation_t* satConstPtr,
        ///< [OUT]
        ///< GNSS constellation type.

    size_t* satConstNumElementsPtr,
        ///< [INOUT]

    bool* satUsedPtr,
        ///< [OUT]
        ///< TRUE if satellite in View Used
        ///< for Navigation.

    size_t* satUsedNumElementsPtr,
        ///< [INOUT]

    uint8_t* satSnrPtr,
        ///< [OUT]
        ///< Satellites in View Signal To Noise Ratio
        ///< [dBHz].

    size_t* satSnrNumElementsPtr,
        ///< [INOUT]

    uint16_t* satAzimPtr,
        ///< [OUT]
        ///< Satellites in View Azimuth [degrees].
        ///< Range: 0 to 360
        ///< If Azimuth angle
        ///< is currently unknown, the value is
        ///< set to UINT16_MAX.

    size_t* satAzimNumElementsPtr,
        ///< [INOUT]

    uint8_t* satElevPtr,
        ///< [OUT]
        ///< Satellites in View Elevation [degrees].
        ///< Range: 0 to 90
        ///< If Elevation angle
        ///< is currently unknown, the value is
        ///< set to UINT8_MAX.

    size_t* satElevNumElementsPtr
        ///< [INOUT]
)
{
    le_result_t result = LE_OK;
    le_gnss_PositionSample_t* positionSamplePtr
                                            = le_ref_Lookup(PositionSampleMap,positionSampleRef);
    int i;

    // Check position sample's reference
    if ( positionSamplePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!",positionSampleRef);
        return LE_FAULT;
    }

    if (satIdPtr)
    {
        if (positionSamplePtr->satInfoValid)
        {
            for(i=0; i<*satIdNumElementsPtr; i++)
            {
                satIdPtr[i] = positionSamplePtr->satInfo[i].satId;
            }
        }
        else
        {
            for(i=0; i<*satIdNumElementsPtr; i++)
            {
                satIdPtr[i] = UINT16_MAX;
            }
            result = LE_OUT_OF_RANGE;
        }
    }

    if (satConstPtr)
    {
        if (positionSamplePtr->satInfoValid)
        {
            for(i=0; i<*satConstNumElementsPtr; i++)
            {
                satConstPtr[i] = positionSamplePtr->satInfo[i].satConst;
            }
        }
        else
        {
            for(i=0; i<*satConstNumElementsPtr; i++)
            {
                satConstPtr[i] = LE_GNSS_SV_CONSTELLATION_UNDEFINED;
            }
            result = LE_OUT_OF_RANGE;
        }
    }

    if (satUsedPtr)
    {
        if (positionSamplePtr->satsUsedCountValid)
        {
            for(i=0; i<*satUsedNumElementsPtr; i++)
            {
                satUsedPtr[i] = positionSamplePtr->satInfo[i].satUsed;
            }
        }
        else
        {
            for(i=0; i<*satUsedNumElementsPtr; i++)
            {
                satUsedPtr[i] = false;
            }
            result = LE_OUT_OF_RANGE;
        }
    }

    if (satSnrPtr)
    {
        if (positionSamplePtr->satInfoValid)
        {
            for(i=0; i<*satSnrNumElementsPtr; i++)
            {
                satSnrPtr[i] = positionSamplePtr->satInfo[i].satSnr;
            }
        }
        else
        {
            for(i=0; i<*satSnrNumElementsPtr; i++)
            {
                satSnrPtr[i] = UINT8_MAX;
            }
            result = LE_OUT_OF_RANGE;
        }
    }

    if (satAzimPtr)
    {
        if (positionSamplePtr->satInfoValid)
        {
            for(i=0; i<*satAzimNumElementsPtr; i++)
            {
                satAzimPtr[i] = positionSamplePtr->satInfo[i].satAzim;
            }
        }
        else
        {
            for(i=0; i<*satAzimNumElementsPtr; i++)
            {
                satAzimPtr[i] = UINT16_MAX;
            }
            result = LE_OUT_OF_RANGE;
        }
    }

    if (satElevPtr)
    {
        if (positionSamplePtr->satInfoValid)
        {
            for(i=0; i<*satElevNumElementsPtr; i++)
            {
                satElevPtr[i] = positionSamplePtr->satInfo[i].satElev;
            }
        }
        else
        {
            for(i=0; i<*satElevNumElementsPtr; i++)
            {
                satElevPtr[i] = UINT8_MAX;
            }
            result = LE_OUT_OF_RANGE;
        }
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the SBAS constellation category according to Satellites in View ID number.
 *
 */
//--------------------------------------------------------------------------------------------------
le_gnss_SbasConstellationCategory_t le_gnss_GetSbasConstellationCategory
(
    uint16_t  satId      ///< [IN] Satellites in View ID number, referring to NMEA standard.
)
{
    le_gnss_SbasConstellationCategory_t sbasCategory;
    switch (satId)
    {
        case SBAS_EGNOS_SV_ID_33:
        case SBAS_EGNOS_SV_ID_37:
        case SBAS_EGNOS_SV_ID_39:
        case SBAS_EGNOS_SV_ID_44:
            sbasCategory = LE_GNSS_SBAS_EGNOS;
            break;
        case SBAS_WAAS_SV_ID_35:
        case SBAS_WAAS_SV_ID_46:
        case SBAS_WAAS_SV_ID_47:
        case SBAS_WAAS_SV_ID_48:
        case SBAS_WAAS_SV_ID_51:
            sbasCategory = LE_GNSS_SBAS_WAAS;
            break;
        case SBAS_GAGAN_SV_ID_40:
        case SBAS_GAGAN_SV_ID_41:
            sbasCategory = LE_GNSS_SBAS_GAGAN;
            break;
        case SBAS_MSAS_SV_ID_42:
        case SBAS_MSAS_SV_ID_50:
            sbasCategory = LE_GNSS_SBAS_MSAS;
            break;
        default:
            sbasCategory = LE_GNSS_SBAS_UNKNOWN;
            LE_WARN("SBAS unknown category, satId %d", satId);
            break;
    }
    LE_DEBUG("satellite id , SBAS category (%d, %d)", satId, sbasCategory);

    return sbasCategory;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the Satellites Vehicle status.
 *
 * @return
 *  - LE_FAULT         Function failed to find the positionSample.
 *  - LE_OUT_OF_RANGE  One of the retrieved parameter is invalid.
 *  - LE_OK            Function succeeded.
 *
 * @note If the caller is passing an invalid Position sample reference into this function,
 *       it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetSatellitesStatus
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN]
        ///< Position sample's reference.

    uint8_t* satsInViewCountPtr,
        ///< [OUT]
        ///< Number of satellites expected to be in view.

    uint8_t* satsTrackingCountPtr,
        ///< [OUT]
        ///< Number of satellites in view, when tracking.

    uint8_t* satsUsedCountPtr
        ///< [OUT]
        ///< Number of satellites in view used for Navigation.
)
{
    le_result_t result = LE_OK;
    le_gnss_PositionSample_t* positionSamplePtr
                                            = le_ref_Lookup(PositionSampleMap,positionSampleRef);

    // Check position sample's reference
    if ( positionSamplePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!",positionSampleRef);
        return LE_FAULT;
    }

    // Satellites in View count
    if (satsInViewCountPtr)
    {
        if (positionSamplePtr->satsInViewCountValid)
        {
            *satsInViewCountPtr = positionSamplePtr->satsInViewCount;
        }
        else
        {
            *satsInViewCountPtr = UINT8_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }

    // Tracking satellites in View count
    if (satsTrackingCountPtr)
    {
        if (positionSamplePtr->satsTrackingCountValid)
        {
            *satsTrackingCountPtr = positionSamplePtr->satsTrackingCount;
        }
        else
        {
            *satsTrackingCountPtr = UINT8_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }

    // Satellites in View used for establishing a fix
    if (satsUsedCountPtr)
    {
        if (positionSamplePtr->satsUsedCountValid)
        {
            *satsUsedCountPtr = positionSamplePtr->satsUsedCount;
        }
        else
        {
            *satsUsedCountPtr = UINT8_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the DOP parameters (Dilution Of Precision) for the fixed position
 *
 * @return
 *  - LE_FAULT         Function failed to find the positionSample.
 *  - LE_OUT_OF_RANGE  One of the retrieved parameter is invalid (set to UINT16_MAX).
 *  - LE_OK            Function succeeded.
 *
 * @note If the caller is passing an invalid Position reference into this function,
 *       it is a fatal error, the function will not return.
 *
 * @note The DOP values are given with 3 decimal places like: DOP value 2200 = 2.20
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetDop
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN]
        ///< Position sample's reference.

    uint16_t* hdopPtr,
        ///< [OUT]
        ///< Horizontal Dilution of Precision [resolution 1e-3].

    uint16_t* vdopPtr,
        ///< [OUT]
        ///< Vertical Dilution of Precision [resolution 1e-3].

    uint16_t* pdopPtr
        ///< [OUT]
        ///< Position Dilution of Precision [resolution 1e-3].
)
{
    le_result_t result = LE_OK;
    le_gnss_PositionSample_t* positionSamplePtr
                                            = le_ref_Lookup(PositionSampleMap,positionSampleRef);

    // Check position sample's reference
    if ( positionSamplePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!",positionSampleRef);
        return LE_FAULT;
    }

    if (hdopPtr)
    {
        if (positionSamplePtr->hdopValid)
        {
            *hdopPtr = positionSamplePtr->hdop;
        }
        else
        {
            *hdopPtr = UINT16_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (vdopPtr)
    {
        if (positionSamplePtr->vdopValid)
        {
            *vdopPtr = positionSamplePtr->vdop;
        }
        else
        {
            *vdopPtr = UINT16_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (pdopPtr)
    {
        if (positionSamplePtr->pdopValid)
        {
            *pdopPtr = positionSamplePtr->pdop;
        }
        else
        {
            *pdopPtr = UINT16_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the position sample's altitude with respect to the WGS-84 ellipsoid
 *
 * @return
 *  - LE_FAULT         Function failed to get the altitude.
 *  - LE_OUT_OF_RANGE  One of the retrieved parameter is invalid (set to INT32_MAX).
 *  - LE_OK            Function succeeded.
 *
 * @note altitudeOnWgs84 is in meters, between WGS-84 earth ellipsoid and mean sea level
 *       with 3 decimal places (3047 = 3.047 meters).
 *
 * @note For a 2D position fix, the altitude with respect to the WGS-84 ellipsoid will be indicated
 *       as invalid and set to INT32_MAX.
 *
 * @note If the caller is passing an invalid Position reference or a null pointer into this
 *       function, it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetAltitudeOnWgs84
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN] Position sample's reference.

    int32_t* altitudeOnWgs84Ptr
        ///< [OUT] Altitude in meters, between WGS-84 earth ellipsoid
        ///<       and mean sea level [resolution 1e-3].
)
{
    le_result_t result;
    le_gnss_PositionSample_t * positionSamplePtr
                                            = le_ref_Lookup(PositionSampleMap,positionSampleRef);

    if (NULL == positionSamplePtr)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!",positionSampleRef);
        return LE_FAULT;
    }

    // Check input pointer
    if (NULL == altitudeOnWgs84Ptr)
    {
        LE_KILL_CLIENT("Invalid pointer provided!");
        return LE_FAULT;
    }

    if (positionSamplePtr->altitudeOnWgs84Valid)
    {
        *altitudeOnWgs84Ptr = positionSamplePtr->altitudeOnWgs84;
        result = LE_OK;
    }
    else
    {
        *altitudeOnWgs84Ptr = INT32_MAX;
        result = LE_OUT_OF_RANGE;
    }
    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the position sample's magnetic deviation. It is the difference between the bearing to
 * true north and the bearing shown on a magnetic compass. The deviation is positive when the
 * magnetic north is east of true north.
 *
 * @return
 *  - LE_FAULT         Function failed to find the positionSample.
 *  - LE_OUT_OF_RANGE  One of the retrieved parameter is invalid (set to INT32_MAX).
 *  - LE_OK            Function succeeded.
 *
 * @note magneticDeviation is in degrees, with 1 decimal places (47 = 4.7 degree).
 *
 * @note If the caller is passing an invalid Position sample reference into this function,
 *       it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetMagneticDeviation
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN] Position sample's reference.

    int32_t* magneticDeviationPtr
        ///< [OUT] MagneticDeviation in degrees [resolution 1e-1].
)
{
    le_result_t result = LE_OK;
    le_gnss_PositionSample_t* positionSamplePtr
                                            = le_ref_Lookup(PositionSampleMap,positionSampleRef);

    // Check position sample's reference
    if ( positionSamplePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!",positionSampleRef);
        return LE_FAULT;
    }

    if (magneticDeviationPtr)
    {
        if (positionSamplePtr->magneticDeviationValid)
        {
            *magneticDeviationPtr = positionSamplePtr->magneticDeviation;
        }
        else
        {
            *magneticDeviationPtr = UINT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function gets the last updated position sample object reference.
 *
 * @return A reference to last Position's sample.
 *
 * @note
 *      On failure, the process exits, so you don't have to worry about checking the returned
 *      reference for validity.
 */
//--------------------------------------------------------------------------------------------------
le_gnss_SampleRef_t le_gnss_GetLastSampleRef
(
    void
)
{
    le_gnss_PositionSample_t*   positionSampleNodePtr=NULL;

    // Create the position sample node.
    positionSampleNodePtr = (le_gnss_PositionSample_t*)le_mem_ForceAlloc(PositionSamplePoolRef);

    // Copy the position sample to the position sample node
    memcpy(positionSampleNodePtr, &LastPositionSample, sizeof(le_gnss_PositionSample_t));

    // Add the node to the queue of the list by passing in the node's link.
    le_dls_Queue(&PositionSampleList, &(positionSampleNodePtr->link));

    LE_DEBUG("Get sample %p", positionSampleNodePtr);

    // Create a safe reference and call the client's handler
    return le_ref_CreateRef(PositionSampleMap, positionSampleNodePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to release the position sample.
 *
 * @note If the caller is passing an invalid Position reference into this function,
 *       it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_gnss_ReleaseSampleRef
(
    le_gnss_SampleRef_t    positionSampleRef    ///< [IN] The position sample's reference.
)
{
    le_gnss_PositionSample_t* positionSamplePtr = le_ref_Lookup(PositionSampleMap,
                                                                positionSampleRef);

    if ( positionSamplePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!",positionSampleRef);
        return;
    }
    le_ref_DeleteRef(PositionSampleMap,positionSampleRef);
    le_mem_Release(positionSamplePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the GNSS constellation bit mask
 *
 * @return
 *  - LE_FAULT         The function failed.
 *  - LE_UNSUPPORTED   If the request is not supported.
 *  - LE_NOT_PERMITTED If the GNSS device is not initialized, disabled or active.
 *  - LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_SetConstellation
(
    le_gnss_ConstellationBitMask_t constellationMask  ///< [IN] GNSS constellation used in solution.
)
{

    le_result_t result = LE_FAULT;

    // Check the GNSS device state
    switch (GnssState)
    {
        case LE_GNSS_STATE_READY:
        {
            // Set GNSS constellation
            result = pa_gnss_SetConstellation(constellationMask);
        }
        break;

        case LE_GNSS_STATE_UNINITIALIZED:
        case LE_GNSS_STATE_DISABLED:
        case LE_GNSS_STATE_ACTIVE:
        {
            LE_ERROR("Bad state for that request [%d]", GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        default:
        {
            LE_ERROR("Unknown GNSS state %d", GnssState);
            result = LE_FAULT;
        }
        break;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the GNSS constellation bit mask
 *
* @return
*  - LE_OK on success
*  - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetConstellation
(
    le_gnss_ConstellationBitMask_t *constellationMaskPtr ///< [OUT] GNSS constellation used in
                                                         ///< solution
)
{
    le_result_t result = LE_FAULT;

    if (constellationMaskPtr == NULL)
    {
        LE_KILL_CLIENT("Pointer is NULL !");
        return LE_FAULT;
    }

    // Check the GNSS device state
    switch (GnssState)
    {
        case LE_GNSS_STATE_READY:
        {
            // Get GNSS constellation
            result = pa_gnss_GetConstellation(constellationMaskPtr);
        }
        break;

        case LE_GNSS_STATE_UNINITIALIZED:
        case LE_GNSS_STATE_DISABLED:
        case LE_GNSS_STATE_ACTIVE:
        {
            LE_ERROR("Bad state for that request [%d]", GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        default:
        {
            LE_ERROR("Unknown GNSS state %d", GnssState);
            result = LE_FAULT;
        }
        break;
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function enables the use of the 'Extended Ephemeris' file into the GNSS device.
 *
 * @return
 *  - LE_FAULT         The function failed to enable the 'Extended Ephemeris' file.
 *  - LE_OK            The function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_EnableExtendedEphemerisFile
(
    void
)
{
    return (pa_gnss_EnableExtendedEphemerisFile());
}


//--------------------------------------------------------------------------------------------------
/**
 * This function disables the use of the 'Extended Ephemeris' file into the GNSS device.
 *
 * @return
 *  - LE_FAULT         The function failed to disable the 'Extended Ephemeris' file.
 *  - LE_OK            The function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_DisableExtendedEphemerisFile
(
    void
)
{
    return (pa_gnss_DisableExtendedEphemerisFile());
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to load an 'Extended Ephemeris' file into the GNSS device.
 *
 * @return
 *  - LE_FAULT         The function failed to inject the 'Extended Ephemeris' file.
 *  - LE_TIMEOUT       A time-out occurred.
 *  - LE_FORMAT_ERROR  'Extended Ephemeris' file format error.
 *  - LE_OK            The function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_LoadExtendedEphemerisFile
(
    int32_t       fd      ///< [IN] extended ephemeris file descriptor
)
{
    return (pa_gnss_LoadExtendedEphemerisFile(fd));
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the validity of the last injected Extended Ephemeris.
 *
 * @return
 *  - LE_FAULT         The function failed to get the validity
 *  - LE_OK            The function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetExtendedEphemerisValidity
(
    uint64_t *startTimePtr,    ///< [OUT] Start time in seconds (since Jan. 1, 1970)
    uint64_t *stopTimePtr      ///< [OUT] Stop time in seconds (since Jan. 1, 1970)
)
{
    if (!startTimePtr)
    {
        LE_KILL_CLIENT("startTimePtr is NULL !");
        return LE_FAULT;
    }

    if (!stopTimePtr)
    {
        LE_KILL_CLIENT("stopTimePtr is NULL !");
        return LE_FAULT;
    }

    return pa_gnss_GetExtendedEphemerisValidity(startTimePtr,stopTimePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function starts the GNSS device.
 *
 * @return
 *  - LE_FAULT         The function failed.
 *  - LE_DUPLICATE     If the GNSS device is already started.
 *  - LE_NOT_PERMITTED If the GNSS device is not initialized or disabled.
 *  - LE_OK            The function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_Start
(
    void
)
{
    le_result_t result = LE_FAULT;

    // Check the GNSS device state
    switch (GnssState)
    {
        case LE_GNSS_STATE_READY:
        {
            // Start GNSS
            result = pa_gnss_Start();
            // Update GNSS device state
            if(result == LE_OK)
            {
                GnssState = LE_GNSS_STATE_ACTIVE;
            }
        }
        break;
        case LE_GNSS_STATE_UNINITIALIZED:
        case LE_GNSS_STATE_DISABLED:
        {
            LE_ERROR("Bad state for that request [%d]", GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        case LE_GNSS_STATE_ACTIVE:
        {
            LE_ERROR("Bad state for that request [%d]", GnssState);
            result = LE_DUPLICATE;
        }
        break;
        default:
        {
            LE_ERROR("Unknown GNSS state %d", GnssState);
            result = LE_FAULT;
        }
        break;
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function stops the GNSS device.
 *
 * @return
 *  - LE_FAULT         The function failed.
 *  - LE_DUPLICATE     If the GNSS device is already stopped.
 *  - LE_NOT_PERMITTED If the GNSS device is not initialized or disabled.
 *  - LE_OK            The function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_Stop
(
    void
)
{
    le_result_t result = LE_FAULT;

    // Check the GNSS device state
    switch (GnssState)
    {
        case LE_GNSS_STATE_ACTIVE:
        {
            // Stop GNSS
            result = pa_gnss_Stop();
            // Update GNSS device state
            if(result == LE_OK)
            {
                GnssState = LE_GNSS_STATE_READY;
            }
        }
        break;
        case LE_GNSS_STATE_UNINITIALIZED:
        case LE_GNSS_STATE_DISABLED:
        {
                        LE_ERROR("Bad state for that request [%d]", GnssState);

            result = LE_NOT_PERMITTED;
        }
        break;
        case LE_GNSS_STATE_READY:
        {
                        LE_ERROR("Bad state for that request [%d]", GnssState);

            result = LE_DUPLICATE;
        }
        break;
        default:
        {
            LE_ERROR("Unknown GNSS state %d", GnssState);
            result = LE_FAULT;
        }
        break;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function performs a "HOT" restart of the GNSS device.
 *
 * @return
 *  - LE_FAULT         The function failed.
 *  - LE_NOT_PERMITTED If the GNSS device is not enabled or not started.
 *  - LE_OK            The function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_ForceHotRestart
(
    void
)
{
    le_result_t result = LE_FAULT;

    // Check the GNSS device state
    switch (GnssState)
    {
        case LE_GNSS_STATE_ACTIVE:
        {
            // Restart GNSS
            result = pa_gnss_ForceRestart(PA_GNSS_HOT_RESTART);
            // GNSS device state is updated ONLY if the restart is failed
            if(result == LE_FAULT)
            {
                GnssState = LE_GNSS_STATE_READY;
            }
        }
        break;
        case LE_GNSS_STATE_UNINITIALIZED:
        case LE_GNSS_STATE_DISABLED:
        case LE_GNSS_STATE_READY:
        {
                        LE_ERROR("Bad state for that request [%d]", GnssState);

            result = LE_NOT_PERMITTED;
        }
        break;
        default:
        {
            LE_ERROR("Unknown GNSS state %d", GnssState);
            result = LE_FAULT;
        }
        break;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function performs a "WARM" restart of the GNSS device.
 *
 * @return
 *  - LE_FAULT         The function failed.
 *  - LE_NOT_PERMITTED If the GNSS device is not enabled or not started.
 *  - LE_OK            The function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_ForceWarmRestart
(
    void
)
{
    le_result_t result = LE_FAULT;

    // Check the GNSS device state
    switch (GnssState)
    {
        case LE_GNSS_STATE_ACTIVE:
        {
            // Restart GNSS
            result = pa_gnss_ForceRestart(PA_GNSS_WARM_RESTART);
            // GNSS device state is updated ONLY if the restart is failed
            if(result == LE_FAULT)
            {
                GnssState = LE_GNSS_STATE_READY;
            }
        }
        break;
        case LE_GNSS_STATE_UNINITIALIZED:
        case LE_GNSS_STATE_DISABLED:
        case LE_GNSS_STATE_READY:
        {
                        LE_ERROR("Bad state for that request [%d]", GnssState);

            result = LE_NOT_PERMITTED;
        }
        break;
        default:
        {
            LE_ERROR("Unknown GNSS state %d", GnssState);
            result = LE_FAULT;
        }
        break;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function performs a "COLD" restart of the GNSS device.
 *
 * @return
 *  - LE_FAULT         The function failed.
 *  - LE_NOT_PERMITTED If the GNSS device is not enabled or not started.
 *  - LE_OK            The function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_ForceColdRestart
(
    void
)
{
    le_result_t result = LE_FAULT;

    // Check the GNSS device state
    switch (GnssState)
    {
        case LE_GNSS_STATE_ACTIVE:
        {
            // Restart GNSS
            result = pa_gnss_ForceRestart(PA_GNSS_COLD_RESTART);
            // GNSS device state is updated ONLY if the restart is failed
            if(result == LE_FAULT)
            {
                GnssState = LE_GNSS_STATE_READY;
            }
        }
        break;
        case LE_GNSS_STATE_UNINITIALIZED:
        case LE_GNSS_STATE_DISABLED:
        case LE_GNSS_STATE_READY:
        {
                        LE_ERROR("Bad state for that request [%d]", GnssState);

            result = LE_NOT_PERMITTED;
        }
        break;
        default:
        {
            LE_ERROR("Unknown GNSS state %d", GnssState);
            result = LE_FAULT;
        }
        break;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function performs a "FACTORY" restart of the GNSS device.
 *
 * @return
 *  - LE_FAULT         The function failed.
 *  - LE_NOT_PERMITTED If the GNSS device is not enabled or not started.
 *  - LE_OK            The function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_ForceFactoryRestart
(
    void
)
{
    le_result_t result = LE_FAULT;

    // Check the GNSS device state
    switch (GnssState)
    {
        case LE_GNSS_STATE_ACTIVE:
        {
            // Restart GNSS
            result = pa_gnss_ForceRestart(PA_GNSS_FACTORY_RESTART);
            // GNSS device state is updated ONLY if the restart is failed
            if(result == LE_FAULT)
            {
                GnssState = LE_GNSS_STATE_READY;
            }
        }
        break;
        case LE_GNSS_STATE_UNINITIALIZED:
        case LE_GNSS_STATE_DISABLED:
        case LE_GNSS_STATE_READY:
        {
                        LE_ERROR("Bad state for that request [%d]", GnssState);

            result = LE_NOT_PERMITTED;
        }
        break;
        default:
        {
            LE_ERROR("Unknown GNSS state %d", GnssState);
            result = LE_FAULT;
        }
        break;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the TTFF in milliseconds
 *
 * @return
 *  - LE_BUSY          The position is not fixed and TTFF can't be measured.
 *  - LE_NOT_PERMITTED If the GNSS device is not enabled or not started.
 *  - LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetTtff
(
    uint32_t* ttffPtr     ///< [OUT] TTFF in milliseconds
)
{
    le_result_t result = LE_FAULT;
    *ttffPtr = 0;

    // Check the GNSS device state
    switch (GnssState)
    {
        case LE_GNSS_STATE_DISABLED:
        case LE_GNSS_STATE_UNINITIALIZED:
        {
            LE_ERROR("Bad state for that request [%d]", GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        case LE_GNSS_STATE_READY:
        case LE_GNSS_STATE_ACTIVE:
        {
            result = pa_gnss_GetTtff(ttffPtr);
        }
        break;
        default:
        {
            LE_ERROR("Unknown GNSS state %d", GnssState);
            result = LE_FAULT;
        }
        break;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function enables the GNSS device.
 *
 * @return
 *  - LE_FAULT         The function failed.
 *  - LE_DUPLICATE     If the GNSS device is already enabled.
 *  - LE_NOT_PERMITTED If the GNSS device is not initialized.
 *  - LE_OK            The function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_Enable
(
    void
)
{
    le_result_t result = LE_FAULT;

    // Check the GNSS device state
    switch (GnssState)
    {
        case LE_GNSS_STATE_DISABLED:
        {
            // Enable GNSS device
            result = pa_gnss_Enable();
            // Update GNSS device state
            if(result == LE_OK)
            {
                GnssState = LE_GNSS_STATE_READY;
            }
        }
        break;
        case LE_GNSS_STATE_UNINITIALIZED:
        {
                        LE_ERROR("Bad state for that request [%d]", GnssState);

            result = LE_NOT_PERMITTED;
        }
        break;
        case LE_GNSS_STATE_READY:
        case LE_GNSS_STATE_ACTIVE:
        {
                        LE_ERROR("Bad state for that request [%d]", GnssState);

            result = LE_DUPLICATE;
        }
        break;
        default:
        {
            LE_ERROR("Unknown GNSS state %d", GnssState);
            result = LE_FAULT;
        }
        break;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function disables the GNSS device.
 *
 * @return
 *  - LE_FAULT         The function failed.
 *  - LE_DUPLICATE     If the GNSS device is already disabled.
 *  - LE_NOT_PERMITTED If the GNSS device is not initialized or started.
 *  - LE_OK            The function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_Disable
(
    void
)
{
    le_result_t result = LE_FAULT;

    // Check the GNSS device state
    switch (GnssState)
    {
        case LE_GNSS_STATE_READY:
        {
            // Disable GNSS device
            result = pa_gnss_Disable();
            // Update GNSS device state
            if(result == LE_OK)
            {
                GnssState = LE_GNSS_STATE_DISABLED;
            }
        }
        break;
        case LE_GNSS_STATE_UNINITIALIZED:
        case LE_GNSS_STATE_ACTIVE:
        {
                        LE_ERROR("Bad state for that request [%d]", GnssState);

            result = LE_NOT_PERMITTED;
        }
        break;
        case LE_GNSS_STATE_DISABLED:
        {
                        LE_ERROR("Bad state for that request [%d]", GnssState);

            result = LE_DUPLICATE;
        }
        break;
        default:
        {
            LE_ERROR("Unknown GNSS state %d", GnssState);
            result = LE_FAULT;
        }
        break;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function sets the GNSS device acquisition rate.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 *  - LE_UNSUPPORTED request not supported
 *  - LE_TIMEOUT a time-out occurred
 *  - LE_NOT_PERMITTED If the GNSS device is not in "ready" state.
 *  - LE_OUT_OF_RANGE  if acquisition rate value is equal to zero
 *
 * @warning This function may be subject to limitations depending on the platform. Please refer to
 *          the @ref platformConstraintsGnss page.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_SetAcquisitionRate
(
    uint32_t  rate      ///< Acquisition rate in milliseconds.
)
{
    le_result_t result;

    if (0 == rate)
    {
        LE_ERROR("Acquisition rate is zero");
        return LE_OUT_OF_RANGE;
    }

    // Check the GNSS device state
    switch (GnssState)
    {
        case LE_GNSS_STATE_READY:
        {
            // Set the GNSS device acquisition rate
            result = pa_gnss_SetAcquisitionRate(rate);
        }
        break;
        case LE_GNSS_STATE_UNINITIALIZED:
        case LE_GNSS_STATE_ACTIVE:
        case LE_GNSS_STATE_DISABLED:
        {
            LE_ERROR("Bad state for that request [%d]", GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        default:
        {
            LE_ERROR("Unknown GNSS state %d", GnssState);
            result = LE_FAULT;
        }
        break;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the GNSS device acquisition rate.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 *  - LE_NOT_PERMITTED If the GNSS device is not in "ready" state.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetAcquisitionRate
(
    uint32_t* ratePtr      ///< Acquisition rate in milliseconds.
)
{
    le_result_t result = LE_FAULT;

    if (ratePtr == NULL)
    {
        LE_KILL_CLIENT("Pointer is NULL !");
        return LE_FAULT;
    }

    // Check the GNSS device state
    switch (GnssState)
    {
        case LE_GNSS_STATE_READY:
        {
            // Set the GNSS device acquisition rate
            result = pa_gnss_GetAcquisitionRate(ratePtr);
        }
        break;
        case LE_GNSS_STATE_UNINITIALIZED:
        case LE_GNSS_STATE_ACTIVE:
        case LE_GNSS_STATE_DISABLED:
        {
            LE_ERROR("Bad state for that request [%d]", GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        default:
        {
            LE_ERROR("Unknown GNSS state %d", GnssState);
            result = LE_FAULT;
        }
        break;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function sets the SUPL Assisted-GNSS mode.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 *  - LE_UNSUPPORTED request not supported
 *  - LE_TIMEOUT a time-out occurred
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_SetSuplAssistedMode
(
    le_gnss_AssistedMode_t  assistedMode      ///< [IN] Assisted-GNSS mode.
)
{
    return pa_gnss_SetSuplAssistedMode(assistedMode);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the SUPL Assisted-GNSS mode.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetSuplAssistedMode
(
    le_gnss_AssistedMode_t * assistedModePtr      ///< [OUT] Assisted-GNSS mode.
)
{
    if (assistedModePtr == NULL)
    {
        LE_KILL_CLIENT("assistedModePtr is NULL !");
        return LE_FAULT;
    }

    return pa_gnss_GetSuplAssistedMode(assistedModePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function sets the SUPL server URL.
 * That server URL is a NULL-terminated string with a maximum string length (including NULL
 * terminator) equal to 256. Optionally the port number is specified after a colon.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 *  - LE_BUSY service is busy
 *  - LE_TIMEOUT a time-out occurred
 *
 * @note If the SUPL server URL size is bigger than the maximum string length (including NULL
 * terminator) size, it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_SetSuplServerUrl
(
    const char*  suplServerUrlPtr      ///< [IN] SUPL server URL.
)
{
    return pa_gnss_SetSuplServerUrl(suplServerUrlPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function injects the SUPL certificate to be used in A-GNSS sessions.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 *  - LE_BUSY service is busy
 *  - LE_TIMEOUT a time-out occurred
 *
 * @note If the SUPL certificate size is bigger than the Maximum SUPL certificate size,
 * it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_InjectSuplCertificate
(
    uint8_t  suplCertificateId,      ///< [IN] ID of the SUPL certificate.
                                     ///< Certificate ID range is 0 to 9
    uint16_t suplCertificateLen,     ///< [IN] SUPL certificate size in Bytes.
    const char*  suplCertificatePtr  ///< [IN] SUPL certificate contents.
)
{
    return pa_gnss_InjectSuplCertificate( suplCertificateId
                                        , suplCertificateLen
                                        , suplCertificatePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function deletes the SUPL certificate.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 *  - LE_BUSY service is busy
 *  - LE_TIMEOUT a time-out occurred
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_DeleteSuplCertificate
(
    uint8_t  suplCertificateId  ///< [IN]  ID of the SUPL certificate.
                                ///< Certificate ID range is 0 to 9
)
{
    return pa_gnss_DeleteSuplCertificate(suplCertificateId);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function sets the enabled NMEA sentences using a bit mask.
 *
 * @return
 *  - LE_OK             Success
 *  - LE_BAD_PARAMETER  Bit mask exceeds the maximal value
 *  - LE_FAULT          Failure
 *  - LE_BUSY           Service is busy
 *  - LE_TIMEOUT        Timeout occurred
 *  - LE_NOT_PERMITTED  GNSS device is not in "ready" state
 *
 * @warning This function may be subject to limitations depending on the platform. Please refer to
 *          the @ref platformConstraintsGnss page.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_SetNmeaSentences
(
    le_gnss_NmeaBitMask_t nmeaMask ///< [IN] Bit mask for enabled NMEA sentences.
)
{
    le_result_t result = LE_NOT_PERMITTED;

    // Check if the bit mask is correct
    if (nmeaMask & ~LE_GNSS_NMEA_SENTENCES_MAX)
    {
        LE_ERROR("Unable to set the enabled NMEA sentences, wrong bit mask 0x%08X", nmeaMask);
        result = LE_BAD_PARAMETER;
    }
    else
    {
        // Check the GNSS device state
        switch (GnssState)
        {
            case LE_GNSS_STATE_READY:
            {
                // Set the enabled NMEA sentences
                result = pa_gnss_SetNmeaSentences(nmeaMask);

                if (LE_OK != result)
                {
                    LE_ERROR("Unable to set the enabled NMEA sentences, error = %d (%s)",
                              result, LE_RESULT_TXT(result));
                }
            }
            break;
            case LE_GNSS_STATE_UNINITIALIZED:
            case LE_GNSS_STATE_ACTIVE:
            case LE_GNSS_STATE_DISABLED:
            {
                LE_ERROR("Bad state for that request [%d]", GnssState);
                result = LE_NOT_PERMITTED;
            }
            break;
            default:
            {
                LE_ERROR("Unknown GNSS state %d", GnssState);
                result = LE_FAULT;
            }
            break;
        }
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the bit mask for the enabled NMEA sentences.
 *
 * @return
 *  - LE_OK             Success
 *  - LE_FAULT          Failure
 *  - LE_BUSY           Service is busy
 *  - LE_TIMEOUT        Timeout occurred
 *  - LE_NOT_PERMITTED  GNSS device is not in "ready" state
 *
 * @note If the caller is passing an null pointer to this function, it is a fatal error
 *       and the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetNmeaSentences
(
    le_gnss_NmeaBitMask_t* nmeaMaskPtr ///< [OUT] Bit mask for enabled NMEA sentences.
)
{
    if (NULL == nmeaMaskPtr)
    {
        LE_KILL_CLIENT("nmeaMaskPtr is NULL !");
        return LE_FAULT;
    }

    le_result_t result = LE_NOT_PERMITTED;

    // Check the GNSS device state
    switch (GnssState)
    {
        case LE_GNSS_STATE_READY:
        {
            // Get the enabled NMEA sentences
            result = pa_gnss_GetNmeaSentences(nmeaMaskPtr);

            if (LE_OK != result)
            {
                LE_ERROR("Unable to get the enabled NMEA sentences, error = %d (%s)",
                          result, LE_RESULT_TXT(result));
            }
        }
        break;
        case LE_GNSS_STATE_UNINITIALIZED:
        case LE_GNSS_STATE_ACTIVE:
        case LE_GNSS_STATE_DISABLED:
        {
            LE_ERROR("Bad state for that request [%d]", GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        default:
        {
            LE_ERROR("Unknown GNSS state %d", GnssState);
            result = LE_FAULT;
        }
        break;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function returns the state of the GNSS device.
 *
 */
//--------------------------------------------------------------------------------------------------
le_gnss_State_t le_gnss_GetState
(
    void
)
{
    return GnssState;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function sets the GNSS minimum elevation.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 *  - LE_OUT_OF_RANGE if the minimum elevation is above range
 *  - LE_UNSUPPORTED request not supported
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_SetMinElevation
(
    uint8_t  minElevation      ///< [IN] Minimum elevation in degrees [range 0..90].
)
{
    if (minElevation > LE_GNSS_MIN_ELEVATION_MAX_DEGREE)
    {
        LE_ERROR("minimum elevation %d is above maximal range %d", minElevation,
                                                                  LE_GNSS_MIN_ELEVATION_MAX_DEGREE);
        return LE_OUT_OF_RANGE;
    }

    return pa_gnss_SetMinElevation(minElevation);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the GNSS minimum elevation.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 *  - LE_UNSUPPORTED request not supported
 *
 * @note If the caller is passing an null pointer to this function, it is a fatal error
 *       and the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetMinElevation
(
   uint8_t*  minElevationPtr     ///< [OUT] Minimum elevation in degrees [range 0..90].
)
{
    if (NULL == minElevationPtr)
    {
        LE_KILL_CLIENT("minElevationPtr is NULL !");
        return LE_FAULT;
    }

    return pa_gnss_GetMinElevation(minElevationPtr);
}
