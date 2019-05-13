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
#include "le_gnss_local.h"


//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------

#define GNSS_POSITION_SAMPLE_MAX         1

/// Maximum expected position handlers
#define GNSS_POSITION_HANDLER_HIGH       1

/// Some platforms don't define O_CLOEXEC.  If not defined, define it as 0 (no effect)
#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#endif

//--------------------------------------------------------------------------------------------------
/**
 * NMEA node path definition
 *
 */
//--------------------------------------------------------------------------------------------------
#ifndef LE_GNSS_NMEA_NODE_PATH
#define LE_GNSS_NMEA_NODE_PATH                  "/dev/nmea"
#endif

//--------------------------------------------------------------------------------------------------
/**
 * SV ID definitions corresponding to SBAS constellation categories
 *
 */
//--------------------------------------------------------------------------------------------------
// EGNOS SBAS category
#define SBAS_EGNOS_SV_ID_33   33
#define SBAS_EGNOS_SV_ID_36   36
#define SBAS_EGNOS_SV_ID_37   37
#define SBAS_EGNOS_SV_ID_39   39
#define SBAS_EGNOS_SV_ID_44   44
#define SBAS_EGNOS_SV_ID_49   49

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

// SDCM SBAS category
#define SBAS_SDCM_SV_ID_38    38
#define SBAS_SDCM_SV_ID_53    53
#define SBAS_SDCM_SV_ID_54    54

//--------------------------------------------------------------------------------------------------
// Data structures.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Satellite Vehicle information.
 */
//--------------------------------------------------------------------------------------------------
typedef struct {
    uint16_t                satId;       ///< Satellite in View ID number [PRN].
    le_gnss_Constellation_t satConst;    ///< GNSS constellation type.
    bool                    satUsed;     ///< TRUE if satellite in View is used for fix Navigation.
    bool                    satTracked;  ///< TRUE if satellite in View is tracked for Navigation.
    uint8_t                 satSnr;      ///< Satellite in View Signal To Noise Ratio (C/No) [dBHz].
    uint16_t                satAzim;     ///< Satellite in View Azimuth [degrees].
                                         ///< Range: 0 to 360
    uint8_t                 satElev;     ///< Satellite in View Elevation [degrees].
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
    le_gnss_FixState_t fixState;              ///< Position Fix state
    bool              latitudeValid;          ///< if true, latitude is set
    int32_t           latitude;               ///< altitude
    bool              longitudeValid;         ///< if true, longitude is set
    int32_t           longitude;              ///< longitude
    bool              hAccuracyValid;         ///< if true, horizontal accuracy is set
    int32_t           hAccuracy;              ///< horizontal accuracy
    bool              altitudeValid;          ///< if true, altitude is set
    int32_t           altitude;               ///< altitude
    bool              altitudeOnWgs84Valid;   ///< if true, altitude with respect to the WGS-84 is
                                              ///< set
    int32_t           altitudeOnWgs84;        ///< altitude with respect to the WGS-84 ellipsoid
    bool              vAccuracyValid;         ///< if true, vertical accuracy is set
    int32_t           vAccuracy;              ///< vertical accuracy
    bool              hSpeedValid;            ///< if true, horizontal speed is set
    uint32_t          hSpeed;                 ///< horizontal speed
    bool              hSpeedAccuracyValid;    ///< if true, horizontal speed accuracy is set
    int32_t           hSpeedAccuracy;         ///< horizontal speed accuracy
    bool              vSpeedValid;            ///< if true, vertical speed is set
    int32_t           vSpeed;                 ///< vertical speed
    bool              vSpeedAccuracyValid;    ///< if true, vertical speed accuracy is set
    int32_t           vSpeedAccuracy;         ///< vertical speed accuracy
    bool              directionValid;         ///< if true, direction is set
    uint32_t          direction;              ///< direction
    bool              directionAccuracyValid; ///< if true, direction accuracy is set
    uint32_t          directionAccuracy;      ///< direction accuracy
    bool              dateValid;              ///< if true, date is set
    uint16_t          year;                   ///< UTC Year A.D. [e.g. 2014].
    uint16_t          month;                  ///< UTC Month into the year [range 1...12].
    uint16_t          day;                    ///< UTC Days into the month [range 1...31].
    bool              timeValid;              ///< if true, time is set
    uint16_t          hours;                  ///< UTC Hours into the day [range 0..23].
    uint16_t          minutes;                ///< UTC Minutes into the hour [range 0..59].
    uint16_t          seconds;                ///< UTC Seconds into the minute [range 0..59].
    uint16_t          milliseconds;           ///< UTC Milliseconds into the second [range 0..999].
    uint64_t          epochTime;              ///< Epoch time in milliseconds since Jan. 1, 1970
    bool              gpsTimeValid;           ///< if true, GPS time is set
    uint32_t          gpsWeek;                ///< GPS week number from midnight, Jan. 6, 1980.
    uint32_t          gpsTimeOfWeek;          ///< Amount of time in milliseconds into the GPS week.
    bool              timeAccuracyValid;      ///< if true, timeAccuracy is set
    uint32_t          timeAccuracy;           ///< Estimated Accuracy for time in nenoseconds
    bool              leapSecondsValid;       ///< if true, leapSeconds is set
    uint8_t           leapSeconds;            ///< UTC leap seconds in advance in seconds
    bool              positionLatencyValid;   ///< if true, positionLatency is set
    uint32_t          positionLatency;        ///< Position measurement latency in milliseconds
    bool              hdopValid;              ///< if true, horizontal dilution is set
    uint32_t          hdop;                   ///< The horizontal dilution of precision (DOP)
    bool              vdopValid;              ///< if true, vertical dilution is set
    uint32_t          vdop;                   ///< The vertical dilution of precision (DOP)
    bool              pdopValid;              ///< if true, position dilution is set
    uint32_t          pdop;                   ///< The position dilution of precision (DOP)
    bool              gdopValid;              ///< if true, geometric dilution is set
    uint32_t          gdop;                   ///< The geometric dilution of precision (DOP)
    bool              tdopValid;              ///< if true, time dilution is set
    uint32_t          tdop;                   ///< The time dilution of precision (DOP)
    bool              magneticDeviationValid; ///< if true, magnetic deviation is set
    int32_t           magneticDeviation;      ///< The magnetic deviation
    // Satellite Vehicles information
    bool              satsInViewCountValid;   ///< if true, satsInViewCount is set
    uint8_t           satsInViewCount;        ///< Satellites in View count.
    bool              satsTrackingCountValid; ///< if true, satsTrackingCount is set
    uint8_t           satsTrackingCount;      ///< Tracking satellites in View count.
    bool              satsUsedCountValid;     ///< if true, satsUsedCount is set
    uint8_t           satsUsedCount;          ///< Satellites in View used for Navigation.
    bool              satInfoValid;           ///< if true, satInfo is set
    le_gnss_SvInfo_t  satInfo[LE_GNSS_SV_INFO_MAX_LEN];
    bool              satMeasValid;           ///< if true, satMeas is set
    le_gnss_SvMeas_t  satMeas[LE_GNSS_SV_INFO_MAX_LEN];
                                              ///< Satellite Vehicle measurement information.
    le_dls_Link_t   link;                     ///< Object node link
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
    le_msg_SessionRef_t           sessionRef;          ///< Store message session reference.
    le_dls_Link_t                 link;                ///< Object node link
}
le_gnss_PositionHandler_t;

//--------------------------------------------------------------------------------------------------
/**
 * Position sample request objet structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_gnss_SampleRef_t             positionSampleRef;  ///< Store position sample reference.
    le_gnss_PositionSample_t*       positionSampleNodePtr; ///< Position sample node pointer.
    le_msg_SessionRef_t             sessionRef;         ///< Client session identifier.
    le_dls_Link_t                   link;               ///< Object node link.
}
le_gnss_PositionSampleRequest_t;


//--------------------------------------------------------------------------------------------------
/**
 * Client session object structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    void*                       clientRefPtr;               ///< Store Client object reference.
    le_msg_SessionRef_t         sessionRef;                 ///< Client session identifier.
    le_gnss_Resolution_t        dopResolution;              ///< Client resolution.
    le_gnss_Resolution_t        vAccuracyResolution;        ///< Vertical accuracy resolution.
    le_gnss_Resolution_t        vSpeedAccuracyResolution;   ///< Vertical speed accuracy resolution.
    le_gnss_Resolution_t        hSpeedAccuracyResolution;   ///< Horizontal speed accuracy
                                                            ///< resolution.
    le_dls_Link_t               link;                       ///< Object node link.
}
le_gnss_Client_t;

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
 * Static memory pool for position handlers
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(PositionHandler,
                          GNSS_POSITION_HANDLER_HIGH,
                          sizeof(le_gnss_PositionHandler_t));


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
 * Static pool for position samples.
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(PositionSample,
                          GNSS_POSITION_SAMPLE_MAX,
                          sizeof(le_gnss_PositionSample_t));

//--------------------------------------------------------------------------------------------------
/**
 * Memory Pool for position samples.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t   PositionSamplePoolRef;

//--------------------------------------------------------------------------------------------------
/**
 * Static memory pool for position sample request objects.
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(PositionSampleRequest,
                          GNSS_POSITION_SAMPLE_MAX,
                          sizeof(le_gnss_PositionSampleRequest_t));

//--------------------------------------------------------------------------------------------------
/**
 * Memory Pool for Client Handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t   ClientPoolRef;

//--------------------------------------------------------------------------------------------------
/**
 * Static Memory Pool for Client Handler.
 *
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(Client,
                          LE_CONFIG_POSITIONING_ACTIVATION_MAX,
                          sizeof(le_gnss_Client_t));

//--------------------------------------------------------------------------------------------------
/**
 * Memory Pool for position samples request object.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t   PositionSampleRequestPoolRef;

//--------------------------------------------------------------------------------------------------
/**
 * Create and initialize the position samples list.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_dls_List_t PositionSampleList = LE_DLS_LIST_INIT;

//--------------------------------------------------------------------------------------------------
/**
 * Static safe Reference Map for Positioning Sample objects.
 *
 */
//--------------------------------------------------------------------------------------------------
LE_REF_DEFINE_STATIC_MAP(PositionSampleMap, GNSS_POSITION_SAMPLE_MAX);

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for Positioning Sample objects.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t PositionSampleMap;

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for client Sample objects.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t ClientRequestRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * NMEA pipe file descriptor
 */
//--------------------------------------------------------------------------------------------------
static int NmeaPipeFd = -1;

//--------------------------------------------------------------------------------------------------
/**
 * Reference to the FD Monitor for the NMEA stream
 */
//--------------------------------------------------------------------------------------------------
static le_fdMonitor_Ref_t NmeaFdMonitor = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Current FD monitor enable/disable state
 */
//--------------------------------------------------------------------------------------------------
static bool FdMonitorDisabled = true;

//--------------------------------------------------------------------------------------------------
/**
 * NMEA sentence to write to NMEA fifo
 */
//--------------------------------------------------------------------------------------------------
static char* CurrentNmeaStringPtr = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Position in NMEA sentence
 */
//--------------------------------------------------------------------------------------------------
static ssize_t  CurrentNmeaWritePosition = 0;

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
    int result;

    // Check NMEA pipe file descriptor
    if(NmeaPipeFd == -1)
    {
        LE_WARN("Pipe already closed");
        return LE_DUPLICATE;
    }

    // Close NMEA pipe
    do
    {
        result = le_fd_Close(NmeaPipeFd);
    }
    while ((result != 0) && (errno == EINTR));

    if (0 != result)
    {
        LE_ERROR("Could not close %s. errno.%d (%s)", LE_GNSS_NMEA_NODE_PATH,
                 errno, strerror(errno));
        return LE_FAULT;
    }

    // Release the NMEA pipe file descriptor
    NmeaPipeFd = -1;
    if (NmeaFdMonitor)
    {
        le_fdMonitor_Delete(NmeaFdMonitor);
        NmeaFdMonitor = NULL;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Write the NMEA sentence to NMEA pipe
 */
//--------------------------------------------------------------------------------------------------
static void WriteNmeaPipe
(
   char* nmeaString        // [IN] The NMEA string
)
{
    // Increment by one to add the terminating null byte
    size_t stringSize = strlen(nmeaString)+1;
    size_t sizeToWrite = stringSize - CurrentNmeaWritePosition;

    size_t writeSize = le_fd_Write(NmeaPipeFd,
                                   nmeaString + CurrentNmeaWritePosition,
                                   sizeToWrite);

    LE_DEBUG("resultWrite %zd / %zd, error, errno.%d (%s)",writeSize, sizeToWrite, errno,
             strerror(errno));

    if (-1 != writeSize)
    {
        if (!writeSize)
        {
            LE_ERROR("EOF error");
            goto error;
        }

        if (writeSize == sizeToWrite)
        {
            // NMEA frame was completely writen
            LE_DEBUG("Frame completely writen");

            le_mem_Release(nmeaString);
            CurrentNmeaStringPtr = NULL;
            CurrentNmeaWritePosition = 0;
            if (!FdMonitorDisabled)
            {
                le_fdMonitor_Disable(NmeaFdMonitor, POLLOUT);
                FdMonitorDisabled = true;
            }
            return;
        }

        if (writeSize < sizeToWrite)
        {
            // Wait for the next POLLOUT to complete the frame
            LE_DEBUG("Wait for the next POLLOUT to complete the frame");

            CurrentNmeaStringPtr     = nmeaString;
            CurrentNmeaWritePosition += writeSize;

            if (FdMonitorDisabled)
            {
                le_fdMonitor_Enable(NmeaFdMonitor, POLLOUT);
                FdMonitorDisabled = false;
            }
            return;
        }

        LE_ERROR("Fifo write error");
    }
    else
    {
        if ((errno == EINTR) || (errno == EAGAIN))
        {
            // Retry writing the frame
            LE_DEBUG("Retry writing the frame");

            CurrentNmeaStringPtr = nmeaString;

            if (FdMonitorDisabled)
            {
                le_fdMonitor_Enable(NmeaFdMonitor, POLLOUT);
                FdMonitorDisabled = false;
            }
            return;

        }

        LE_ERROR("Could not write to %s (write error, errno.%d (%s))",
                     LE_GNSS_NMEA_NODE_PATH,
                     errno, strerror(errno));
    }

error:
    le_mem_Release(nmeaString);
    CurrentNmeaWritePosition = 0;
    CurrentNmeaStringPtr = NULL;
    CloseNmeaPipe();

}

//--------------------------------------------------------------------------------------------------
/**
 * Event handler for NMEA frames writing to FIFO
 */
//--------------------------------------------------------------------------------------------------
static void NmeaEventsHandler
(
    int fd,                 ///< [IN] Read file descriptor
    short events            ///< [IN] FD events
)
{
    LE_DEBUG("Received FIFO event: %x", events);

    if (events & POLLOUT)
    {
        if (CurrentNmeaStringPtr)
        {
            WriteNmeaPipe(CurrentNmeaStringPtr);
        }
    }

    if (events & POLLERR)
    {
        LE_DEBUG("Event POLLERR");
    }
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
    if (NmeaPipeFd != -1)
    {
        LE_DEBUG("Nmea Pipe is already open");
        return LE_DUPLICATE;
    }

    // Open NMEA pipe
    LE_DEBUG("Open Nmea Pipe");
    do
    {
        NmeaPipeFd = le_fd_Open(LE_GNSS_NMEA_NODE_PATH, O_WRONLY|O_APPEND|O_CLOEXEC|O_NONBLOCK);

        if ((NmeaPipeFd == -1) && (errno != EINTR))
        {
            LE_WARN_IF(errno != ENXIO, "Open %s failure: errno.%d (%s)", LE_GNSS_NMEA_NODE_PATH,
                                                                         errno, strerror(errno));
            return LE_FAULT;
        }

    }while (NmeaPipeFd == -1);

    if (!NmeaFdMonitor)
    {
        NmeaFdMonitor = le_fdMonitor_Create("NmeaPipe", NmeaPipeFd, NmeaEventsHandler, POLLOUT);

        le_fdMonitor_Disable(NmeaFdMonitor, POLLOUT);
        FdMonitorDisabled = true;
    }
    return LE_OK;
}

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
            if (positionHandlerNodePtr == (le_gnss_PositionHandler_t*)obj)
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

    LE_FATAL_IF((NULL == obj), "Position Sample Object does not exist!");

    linkPtr = le_dls_Peek(&PositionSampleList);
    if (linkPtr != NULL)
    {
        do
        {
            // Get the node from the list
            positionSampleNodePtr = (le_gnss_PositionSample_t*)
                                    CONTAINER_OF(linkPtr, le_gnss_PositionSample_t, link);
            // Check the node.
            if (positionSampleNodePtr == (le_gnss_PositionSample_t*)obj)
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
    int result = -1;

    LE_DEBUG("Create %s", LE_GNSS_NMEA_NODE_PATH);

#ifdef LE_CONFIG_LINUX
    // Create the node for /dev/nmea device folder
    umask(0);
#endif

    // Create the node for /dev/nmea fifo buffer
    result = le_fd_MkFifo(LE_GNSS_NMEA_NODE_PATH, S_IFIFO|0666);

    LE_ERROR_IF((result != 0)&&(result != EEXIST),
           "Could not create %s. errno.%d (%s)", LE_GNSS_NMEA_NODE_PATH, errno, strerror(errno));
}

//--------------------------------------------------------------------------------------------------
/**
 * The PA NMEA Handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static void PaNmeaHandler
(
    char* nmeaPtr        // [IN] The NMEA string
)
{
    LE_DEBUG("NMEA Handler %s", nmeaPtr);

    // Open the NMEA FIFO pipe
    le_result_t resultNmeaPipe = OpenNmeaPipe();
    if ((resultNmeaPipe != LE_OK) && (resultNmeaPipe != LE_DUPLICATE))
    {
        LE_WARN("Could not open Nmea Pipe");
        le_mem_Release(nmeaPtr);
        return;
    }

    if (NULL != CurrentNmeaStringPtr)
    {
        // The last frame (CurrentNmeaStringPtr) was not sent completely, discard the new one
        LE_WARN("Lost Frame");
        le_mem_Release(nmeaPtr);
        return;
    }

    CurrentNmeaWritePosition = 0;
    WriteNmeaPipe(nmeaPtr);
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
    // Leap Seconds
    posSampleDataPtr->leapSecondsValid = paPosDataPtr->leapSecondsValid;
    posSampleDataPtr->leapSeconds = paPosDataPtr->leapSeconds;
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
    posSampleDataPtr->gdopValid = paPosDataPtr->gdopValid;
    posSampleDataPtr->gdop = paPosDataPtr->gdop;
    posSampleDataPtr->tdopValid = paPosDataPtr->tdopValid;
    posSampleDataPtr->tdop = paPosDataPtr->tdop;

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


#ifdef LE_CONFIG_LINUX
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
#endif

//--------------------------------------------------------------------------------------------------
/**
 * The function sets the client request data to default value.
 */
//--------------------------------------------------------------------------------------------------
static void InitClientRequest
(
    le_gnss_Client_t* clientRequestPtr
)
{
    // Init default resolution
    clientRequestPtr->dopResolution = LE_GNSS_RES_THREE_DECIMAL;
    clientRequestPtr->vAccuracyResolution = LE_GNSS_RES_ONE_DECIMAL;
    clientRequestPtr->vSpeedAccuracyResolution = LE_GNSS_RES_ONE_DECIMAL;
    clientRequestPtr->hSpeedAccuracyResolution = LE_GNSS_RES_ONE_DECIMAL;
}

//--------------------------------------------------------------------------------------------------
/**
 * The function returns the Client session reference if exists else it returns NULL.
 */
//--------------------------------------------------------------------------------------------------
static le_gnss_Client_t* FindClientSessionReference
(
    le_msg_SessionRef_t sessionRef   ///< [IN] Session reference.
)
{
    le_ref_IterRef_t iterRef = le_ref_GetIterator(ClientRequestRefMap);
    le_result_t result = le_ref_NextNode(iterRef);

    while (LE_OK == result)
    {
        le_gnss_Client_t* gnssCtrlPtr = (le_gnss_Client_t*) le_ref_GetValue(iterRef);

        LE_DEBUG("gnssCtrlPtr %p, gnssCtrlPtr->sessionRef %p, sessionRef %p",
                 gnssCtrlPtr, gnssCtrlPtr->sessionRef, sessionRef);

        // Check if the session reference saved matchs with the current session reference.
        if (sessionRef == gnssCtrlPtr->sessionRef)
        {
             LE_DEBUG("SessionRef %p found in Client session", sessionRef);
             return gnssCtrlPtr;
        }
        // Get the next value in the reference map
        result = le_ref_NextNode(iterRef);
    }
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * The function returns a client session reference. It checks if it already exist, else it creates
 * one.
 */
//--------------------------------------------------------------------------------------------------
static le_gnss_Client_t* GetClientSessionReference
(
    void
)
{
    le_gnss_Client_t* clientRequestPtr = NULL;
    le_msg_SessionRef_t sessionRef = le_gnss_GetClientSessionRef();

    clientRequestPtr = FindClientSessionReference(sessionRef);

    if (NULL == clientRequestPtr)
    {
        clientRequestPtr = le_mem_ForceAlloc(ClientPoolRef);

        InitClientRequest(clientRequestPtr);

        // On le_mem_ForceAlloc failure, the process exits, so function doesn't need to checking the
        // returned pointer for validity.

        // Need to return a unique reference that will be used by Release.
        void* reqRefPtr = le_ref_CreateRef(ClientRequestRefMap, clientRequestPtr);

        LE_DEBUG("SessionRef %p was not found, Create Client session", sessionRef);
        LE_DEBUG("reqRefPtr %p, clientRequestPtr %p", reqRefPtr, clientRequestPtr);

        // Save the client session "sessionRef" associated with the request reference "reqRef"
        clientRequestPtr->sessionRef = sessionRef;
        clientRequestPtr->clientRefPtr = reqRefPtr;
    }

    return clientRequestPtr;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert the DOP value in the selected resolution.
 */
//--------------------------------------------------------------------------------------------------
static uint32_t ConvertDop
(
    uint32_t dopValue    ///< [IN] Dilution of Precision value to convert.
)
{
    uint16_t resValue = 0;

    le_gnss_Client_t* clientRequestPtr = NULL;
    le_msg_SessionRef_t sessionRef = le_gnss_GetClientSessionRef();
    le_gnss_Resolution_t resolution = LE_GNSS_RES_UNKNOWN;

    clientRequestPtr = FindClientSessionReference(sessionRef);

    if (NULL != clientRequestPtr)
    {
        resolution = clientRequestPtr->dopResolution;
    }

    switch(resolution)
    {
        case LE_GNSS_RES_ZERO_DECIMAL:
             resValue = dopValue/1000;
             break;
        case LE_GNSS_RES_ONE_DECIMAL:
             resValue = dopValue/100;
             break;
        case LE_GNSS_RES_TWO_DECIMAL:
             resValue = dopValue/10;
             break;
        case LE_GNSS_RES_THREE_DECIMAL:
        default:
             // treat unknown resolution as a resolution of 3 decimal places (default).
             resValue = dopValue;
             break;
    }
    LE_DEBUG("resolution %d, dopValue %"PRIu32", new dopValue %"PRIu16, (int)resolution, dopValue,
             resValue);
    return resValue;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert the position data in the selected resolution.
 *
 * @return
 *  - LE_OK     The function succeed.
 *  - LE_FAULT  The function failed.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ConvertPositionData
(
    int32_t value,                 ///< [IN] Data value to convert.
    le_gnss_DataType_t dataType,   ///< [IN] Data type.
    int32_t* valuePtr              ///< [OUT] The converted data value.
)
{
    le_gnss_Client_t* clientRequestPtr = NULL;
    le_msg_SessionRef_t sessionRef = le_gnss_GetClientSessionRef();
    le_gnss_Resolution_t resolution = LE_GNSS_RES_UNKNOWN;

    if (NULL == valuePtr)
    {
        LE_ERROR("valuePtr is NULL");
        return LE_FAULT;
    }

    clientRequestPtr = FindClientSessionReference(sessionRef);

    if (NULL != clientRequestPtr)
    {
        switch(dataType)
        {
            case LE_GNSS_DATA_VACCURACY:
                resolution = clientRequestPtr->vAccuracyResolution;
                break;
            case LE_GNSS_DATA_VSPEEDACCURACY:
                resolution = clientRequestPtr->vSpeedAccuracyResolution;
                break;
            case LE_GNSS_DATA_HSPEEDACCURACY:
                resolution = clientRequestPtr->hSpeedAccuracyResolution;
                break;
            default:
                LE_ERROR("Unsupported data type.");
                return LE_FAULT;
        }
    }
    else
    {
        // Set resolution to default value.
        switch(dataType)
        {
            case LE_GNSS_DATA_VACCURACY:
                resolution = LE_GNSS_RES_ONE_DECIMAL;
                break;
            case LE_GNSS_DATA_VSPEEDACCURACY:
                resolution = LE_GNSS_RES_ONE_DECIMAL;
                break;
            case LE_GNSS_DATA_HSPEEDACCURACY:
                resolution = LE_GNSS_RES_ONE_DECIMAL;
                break;
            default:
                LE_ERROR("Unsupported data type.");
                return LE_FAULT;
        }
    }

    switch(resolution)
    {
        case LE_GNSS_RES_ZERO_DECIMAL:
             *valuePtr = value / 1000;
             break;
        case LE_GNSS_RES_ONE_DECIMAL:
             *valuePtr = value / 100;
             break;
        case LE_GNSS_RES_TWO_DECIMAL:
             *valuePtr = value / 10;
             break;
        case LE_GNSS_RES_THREE_DECIMAL:
             *valuePtr = value;
             break;
        default:
             LE_ERROR("Unsupported resolution.");
             return LE_FAULT;
    }
    LE_DEBUG("resolution %d, value %"PRIi32", new value %"PRIi32, (int)resolution, value, *valuePtr);
    return LE_OK;
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
    le_gnss_PositionSampleRequest_t*    positionSampleRequestNodePtr=NULL;
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
    if (NULL != linkPtr)
    {
        // Create the position request sample node.
        positionSampleRequestNodePtr =
                 (le_gnss_PositionSampleRequest_t*)le_mem_ForceAlloc(PositionSampleRequestPoolRef);

        // Create the position sample node.
        positionSampleRequestNodePtr->positionSampleNodePtr =
                 (le_gnss_PositionSample_t*)le_mem_ForceAlloc(PositionSamplePoolRef);

        // Add reference for each subscribe handler.
        for(i=0 ; i<NumOfPositionHandlers-1 ; i++)
        {
            le_mem_AddRef((void *)positionSampleRequestNodePtr);
            le_mem_AddRef((void *)positionSampleRequestNodePtr->positionSampleNodePtr);
        }

        // Copy the position sample to the position sample node
        memcpy(positionSampleRequestNodePtr->positionSampleNodePtr, &LastPositionSample,
                     sizeof(le_gnss_PositionSample_t));

        // Add the node to the queue of the list by passing in the node's link.
        positionSampleRequestNodePtr->positionSampleNodePtr->link = LE_DLS_LINK_INIT;
        le_dls_Queue(&PositionSampleList, &(positionSampleRequestNodePtr->
                                                positionSampleNodePtr->link));

        // Call Handler(s)
        do
        {
            // Get the node from the list
            positionHandlerNodePtr =
                (le_gnss_PositionHandler_t*)CONTAINER_OF(linkPtr, le_gnss_PositionHandler_t, link);

            LE_DEBUG("Report sample %p to the corresponding handler (handler %p)",
                     positionSampleRequestNodePtr->positionSampleNodePtr,
                     positionHandlerNodePtr->handlerFuncPtr);

            // Create a safe reference and call the client's handler
            le_gnss_SampleRef_t safePositionSampleRef = le_ref_CreateRef(PositionSampleMap,
                                                           positionSampleRequestNodePtr);

            // Store message session reference which will be useful for close session handler
            positionSampleRequestNodePtr->sessionRef = positionHandlerNodePtr->sessionRef;

            // Store safereference which will be useful for close session handler
            positionSampleRequestNodePtr->positionSampleRef = safePositionSampleRef;

            if(safePositionSampleRef != NULL)
            {
                positionHandlerNodePtr->handlerFuncPtr(safePositionSampleRef,
                                                   positionHandlerNodePtr->handlerContextPtr);
            }
            // Move to the next node.
            linkPtr = le_dls_PeekNext(&PositionHandlerList, linkPtr);
        } while (NULL != linkPtr);
    }

    le_mem_Release(positionPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to release the client reference
 *
 * @note If the caller is passing an invalid Position reference into this function,
 *       it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
static void le_gnss_ReleaseClientRef
(
    void* RefPtr    ///< [IN] The client reference.
)
{
    void* clientPosPtr = le_ref_Lookup(ClientRequestRefMap, RefPtr);
    if (NULL == clientPosPtr)
    {
        LE_KILL_CLIENT("Invalid positioning service activation reference %p", RefPtr);
    }
    else
    {
        le_ref_DeleteRef(ClientRequestRefMap, RefPtr);
        LE_DEBUG("Remove Client Position Ctrl (%p)",RefPtr);
        le_mem_Release(clientPosPtr);
    }
}

//--------------------------------------------------------------------------------------------------
/**
* Handler function to close session service for le_gnss APIs
*
*/
//--------------------------------------------------------------------------------------------------
static void CloseSessionEventHandler
(
    le_msg_SessionRef_t sessionRef,
    void* contextPtr
)
{
    LE_ERROR("SessionRef (%p) has been closed", sessionRef);

    if (!sessionRef)
    {
        LE_ERROR("ERROR sessionRef is NULL");
        return;
    }

    // Search for all position sample request references used by the current client session
    // that has been closed.
    le_ref_IterRef_t iterRef = le_ref_GetIterator(PositionSampleMap);
    le_result_t result = le_ref_NextNode(iterRef);
    while (LE_OK == result)
    {
        le_gnss_PositionSampleRequest_t *positionSampleRequestPtr =
                                (le_gnss_PositionSampleRequest_t*)le_ref_GetValue(iterRef);

        // Check if the session reference saved matchs with the current session reference.
        if (positionSampleRequestPtr->sessionRef == sessionRef)
        {
            le_gnss_SampleRef_t safeRef = (le_gnss_SampleRef_t)le_ref_GetSafeRef(iterRef);
            LE_DEBUG("Release le_gnss_ReleaseSampleRef 0x%p, Session 0x%p", safeRef, sessionRef);

            // Release positioning client control request reference found.
            le_gnss_ReleaseSampleRef(safeRef);
        }

        // Get the next value in the reference mpa.
        result = le_ref_NextNode(iterRef);
    }

    iterRef = le_ref_GetIterator(ClientRequestRefMap);
    result = le_ref_NextNode(iterRef);
    while (LE_OK == result)
    {
        le_gnss_Client_t* gnssCtrlPtr = (le_gnss_Client_t*) le_ref_GetValue(iterRef);

        // Check if the session reference saved matchs with the current session reference.
        if (sessionRef == gnssCtrlPtr->sessionRef)
        {
            void* safeRefPtr = (void*)le_ref_GetSafeRef(iterRef);
            LE_DEBUG("Release le_gnss_ReleaseClientRef 0x%p, Session 0x%p",
                     safeRefPtr, gnssCtrlPtr->sessionRef);

            // Release positioning client control request reference found.
            le_gnss_ReleaseClientRef(safeRefPtr);
        }

        // Get the next value in the reference map
        result = le_ref_NextNode(iterRef);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Validate position sample reference pointer
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if the pointer is NULL.
 *
 * @note If the caller is passing a null pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ValidatePositionSamplePtr
(
    le_gnss_PositionSampleRequest_t* positionSampleRequestNodePtr
)
{
    if (NULL == positionSampleRequestNodePtr)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", positionSampleRequestNodePtr);
        return LE_FAULT;
    }
    else if (NULL == positionSampleRequestNodePtr->positionSampleNodePtr)
    {
        LE_ERROR("Invalid reference (%p) provided!",
                       positionSampleRequestNodePtr->positionSampleNodePtr);
        return LE_FAULT;
    }
    else
    {
        return LE_OK;
    }
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

    // If PA initialization failed, or in a wrong state, fail now so data structures below
    // are not re-initialized.
    if (result != LE_OK)
    {
        LE_ERROR("Bad PA gnss initialisation");
        return result;
    }

#ifdef LE_CONFIG_LINUX
    // Block signals.  All signals that are to be used in signal events must be blocked.
    le_sig_Block(SIGPIPE);

    // Register a signal event handler for SIGPIPE signal.
    le_sig_SetEventHandler(SIGPIPE, SigPipeHandler);
#endif

    // Create a pool for Position  Handler objects
    PositionHandlerPoolRef = le_mem_InitStaticPool(PositionHandler,
                                                   GNSS_POSITION_HANDLER_HIGH,
                                                   sizeof(le_gnss_PositionHandler_t));
    le_mem_SetDestructor(PositionHandlerPoolRef, PositionHandlerDestructor);

    // Create a pool for Position Sample objects
    PositionSamplePoolRef = le_mem_InitStaticPool(PositionSample,
                                                  GNSS_POSITION_SAMPLE_MAX,
                                                  sizeof(le_gnss_PositionSample_t));
    le_mem_SetDestructor(PositionSamplePoolRef, PositionSampleDestructor);

    // Create a pool for Position Sample request objects
    PositionSampleRequestPoolRef = le_mem_InitStaticPool(PositionSampleRequest,
                                                         GNSS_POSITION_SAMPLE_MAX,
                                                         sizeof(le_gnss_PositionSampleRequest_t));

    // Create the reference HashMap for positioning sample
    PositionSampleMap = le_ref_InitStaticMap(PositionSampleMap, GNSS_POSITION_SAMPLE_MAX);

    // Create safe reference map for request references.
    ClientRequestRefMap = le_ref_CreateMap("ClientRequestRefMap",
            LE_CONFIG_POSITIONING_ACTIVATION_MAX);

    // Create a pool for client session object structure.
    ClientPoolRef = le_mem_InitStaticPool(Client,
            LE_CONFIG_POSITIONING_ACTIVATION_MAX, sizeof(le_gnss_Client_t));

    // Initialize the event client close function handler.
    le_msg_ServiceRef_t msgService = le_gnss_GetServiceRef();
    le_msg_AddServiceCloseHandler(msgService, CloseSessionEventHandler, NULL);

    // Initialize Handler context
    NumOfPositionHandlers = 0;
    PaHandlerRef = NULL;

    // Initialize last Position sample
    memset(&LastPositionSample, 0, sizeof(LastPositionSample));
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
    else if ((resultStat == 0) && (S_ISCHR(nmeaFileStat.st_mode))) // Character device file
    {
        LE_INFO("%s is a character device file", LE_GNSS_NMEA_NODE_PATH);
    }
#ifdef LE_CONFIG_LINUX
    else if((resultStat == -1)&&(errno == ENOENT)) // No such file or directory
#else
    else if(resultStat == -1) // No such file or directory
#endif
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
        LE_ERROR("Could not get file info for '%s'. errno.%d (%s). resultstat:%d",
               LE_GNSS_NMEA_NODE_PATH, errno, strerror(errno),resultStat);
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

    LE_FATAL_IF((NULL == handlerPtr), "handlerPtr pointer is NULL !");

    // Create the position sample handler node.
    positionHandlerPtr = (le_gnss_PositionHandler_t*)le_mem_ForceAlloc(PositionHandlerPoolRef);
    positionHandlerPtr->link = LE_DLS_LINK_INIT;
    positionHandlerPtr->handlerFuncPtr = handlerPtr;
    positionHandlerPtr->handlerContextPtr = contextPtr;
    positionHandlerPtr->sessionRef = le_gnss_GetClientSessionRef();

    LE_DEBUG("handler %p", handlerPtr);

    // Subscribe to PA position Data handler
    if (NULL == PaHandlerRef)
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
            if ((le_gnss_PositionHandlerRef_t)positionHandlerNodePtr == handlerRef)
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
 * @note If the caller is passing an invalid Position sample reference or a null pointer into this
 *       function, it is a fatal error, the function will not return.
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
    le_gnss_PositionSampleRequest_t* positionSampleRequestNodePtr =
                                                le_ref_Lookup(PositionSampleMap,positionSampleRef);

    // Check input pointer
    if (NULL == statePtr)
    {
        LE_KILL_CLIENT("Invalid pointer provided!");
        return LE_FAULT;
    }

    // Check position sample's reference
    result = ValidatePositionSamplePtr(positionSampleRequestNodePtr);
    if (LE_OK != result)
    {
        return result;
    }

    // Get the position Fix state
    *statePtr = positionSampleRequestNodePtr->positionSampleNodePtr->fixState;

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the location's data (Latitude, Longitude, Horizontal accuracy).
 *
 * @return
 *  - LE_FAULT         Function failed to get the location's data
 *  - LE_OUT_OF_RANGE  At least one of the retrieved parameters is invalid (set to INT32_MAX).
 *  - LE_OK            Function succeeded.
 *
 * @note latitudePtr, longitudePtr and hAccuracyPtr can be set to NULL if not needed.
 *
 * @note The latitude and longitude values are based on the WGS84 standard coordinate system.
 *
 * @note The latitude and longitude values are given in degrees with 6 decimal places like:
 *       Latitude +48858300 = 48.858300 degrees North
 *       Longitude +2294400 = 2.294400 degrees East
 *       (The latitude and longitude values are given in degrees, minutes, seconds in NMEA frame)
 *
 * @note In case the function returns LE_OUT_OF_RANGE, some of the retrieved parameters may be
 *       valid. Please compare them with INT32_MAX.
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
    le_gnss_PositionSampleRequest_t* positionSampleRequestNodePtr
                                            = le_ref_Lookup(PositionSampleMap,positionSampleRef);

    // Check position sample's reference
    result = ValidatePositionSamplePtr(positionSampleRequestNodePtr);
    if (result != LE_OK)
    {
        return result;
    }

    if (latitudePtr)
    {
        if (positionSampleRequestNodePtr->positionSampleNodePtr->latitudeValid)
        {
            *latitudePtr = positionSampleRequestNodePtr->positionSampleNodePtr->latitude;
        }
        else
        {
            *latitudePtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (longitudePtr)
    {
        if (positionSampleRequestNodePtr->positionSampleNodePtr->longitudeValid)
        {
            *longitudePtr = positionSampleRequestNodePtr->positionSampleNodePtr->longitude;
        }
        else
        {
            *longitudePtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (hAccuracyPtr)
    {
        if (positionSampleRequestNodePtr->positionSampleNodePtr->hAccuracyValid)
        {
            *hAccuracyPtr = positionSampleRequestNodePtr->positionSampleNodePtr->hAccuracy;
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
 *  - LE_OUT_OF_RANGE  At least one of the retrieved parameters is invalid (set to INT32_MAX).
 *  - LE_OK            Function succeeded.
 *
 * @note Altitude is in meters, above Mean Sea Level, with 3 decimal places (3047 = 3.047 meters).
 *
 * @note For a 2D position fix, the altitude will be indicated as invalid and set to INT32_MAX
 *
 * @note Vertical position accuracy is default set to meters with 1 decimal place (3047 = 3.0
 *       meters). To change its accuracy, call the @c le_gnss_SetDataResolution() function. Vertical
 *       position accuracy is set as data type and accuracy from 0 to 3 decimal place is set as
 *       resolution.
 *
 * @note In case the function returns LE_OUT_OF_RANGE, some of the retrieved parameters may be
 *       valid. Please compare them with INT32_MAX.
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
        ///< [IN] Position sample's reference.
    int32_t* altitudePtr,
        ///< [OUT] Altitude in meters, above Mean Sea Level [resolution 1e-3].
    int32_t* vAccuracyPtr
        ///< [OUT] Vertical position's accuracy in meters.
)
{
    le_result_t result = LE_OK;
    le_gnss_PositionSampleRequest_t * positionSampleRequestNodePtr
                                            = le_ref_Lookup(PositionSampleMap,positionSampleRef);

    // Check position sample's reference
    result = ValidatePositionSamplePtr(positionSampleRequestNodePtr);
    if (result != LE_OK)
    {
        return result;
    }

    if (altitudePtr)
    {
        if (positionSampleRequestNodePtr->positionSampleNodePtr->altitudeValid)
        {
            *altitudePtr = positionSampleRequestNodePtr->positionSampleNodePtr->altitude;
        }
        else
         {
            *altitudePtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (vAccuracyPtr)
    {
        if ((false == positionSampleRequestNodePtr->positionSampleNodePtr->vAccuracyValid) ||
            (LE_OK != ConvertPositionData(
                                     positionSampleRequestNodePtr->positionSampleNodePtr->vAccuracy,
                                     LE_GNSS_DATA_VACCURACY,
                                     vAccuracyPtr))
           )
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
 * @note If the caller is passing an invalid Position sample reference or null pointers into this
 *       function, it is a fatal error, the function will not return.
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
    le_gnss_PositionSampleRequest_t* positionSampleRequestNodePtr
                                            = le_ref_Lookup(PositionSampleMap,positionSampleRef);

    // Check input pointers
    if ((NULL == hoursPtr)
        || (NULL == minutesPtr)
        || (NULL == secondsPtr)
        || (NULL == millisecondsPtr))
    {
        LE_KILL_CLIENT("Invalid pointers provided!");
        return LE_FAULT;
    }

    // Check position sample's reference
    result = ValidatePositionSamplePtr(positionSampleRequestNodePtr);
    if (LE_OK != result)
    {
        return result;
    }

    // Get the position sample's time
    if (positionSampleRequestNodePtr->positionSampleNodePtr->timeValid)
    {
        result = LE_OK;
        if (hoursPtr)
        {
            *hoursPtr = positionSampleRequestNodePtr->positionSampleNodePtr->hours;
        }
        if (minutesPtr)
        {
            *minutesPtr = positionSampleRequestNodePtr->positionSampleNodePtr->minutes;
        }
        if (secondsPtr)
        {
            *secondsPtr = positionSampleRequestNodePtr->positionSampleNodePtr->seconds;
        }
        if (millisecondsPtr)
        {
            *millisecondsPtr = positionSampleRequestNodePtr->positionSampleNodePtr->milliseconds;
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
 * @note If the caller is passing an invalid Position sample reference or null pointers into this
 *       function, it is a fatal error, the function will not return.
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
    le_gnss_PositionSampleRequest_t* positionSampleRequestNodePtr
                                            = le_ref_Lookup(PositionSampleMap,positionSampleRef);

    // Check input pointers
    if ((NULL == gpsWeekPtr)
        || (NULL == gpsTimeOfWeekPtr))
    {
        LE_KILL_CLIENT("Invalid pointers provided!");
        return LE_FAULT;
    }

    // Check position sample's reference
    result = ValidatePositionSamplePtr(positionSampleRequestNodePtr);
    if (LE_OK != result)
    {
        return result;
    }

    // Get the position sample's GPS time
    if (positionSampleRequestNodePtr->positionSampleNodePtr->gpsTimeValid)
    {
        result = LE_OK;
        if (gpsWeekPtr)
        {
            *gpsWeekPtr = positionSampleRequestNodePtr->positionSampleNodePtr->gpsWeek;
        }
        if (gpsTimeOfWeekPtr)
        {
            *gpsTimeOfWeekPtr = positionSampleRequestNodePtr->positionSampleNodePtr->gpsTimeOfWeek;
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
 * @note If the caller is passing an invalid position sample reference or a null pointer into this
 *       function, it is a fatal error, the function will not return.
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
    le_gnss_PositionSampleRequest_t* positionSampleRequestNodePtr
                                            = le_ref_Lookup(PositionSampleMap,positionSampleRef);

    // Check input pointer
    if (NULL == millisecondsPtr)
    {
        LE_KILL_CLIENT("Invalid pointer provided!");
        return LE_FAULT;
    }

    // Check position sample's reference
    result = ValidatePositionSamplePtr(positionSampleRequestNodePtr);
    if (result != LE_OK)
    {
        return result;
    }

    // Get the position sample's time
    if (positionSampleRequestNodePtr->positionSampleNodePtr->timeValid)
    {
        // Get the epoch time
        result = LE_OK;
        *millisecondsPtr = positionSampleRequestNodePtr->positionSampleNodePtr->epochTime;
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
 * @note If the caller is passing an invalid position sample reference or a null pointer into this
 *       function, it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetTimeAccuracy
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN] Position sample's reference.

    uint32_t* timeAccuracyPtr
        ///< [OUT] Estimated time accuracy in nanoseconds
)
{
    le_result_t result = LE_OK;
    le_gnss_PositionSampleRequest_t* positionSampleRequestNodePtr
                                            = le_ref_Lookup(PositionSampleMap,positionSampleRef);

    // Check input pointer
    if (NULL == timeAccuracyPtr)
    {
        LE_KILL_CLIENT("Invalid pointer provided!");
        return LE_FAULT;
    }

    // Check position sample's reference
    result = ValidatePositionSamplePtr(positionSampleRequestNodePtr);
    if (LE_OK != result)
    {
        return result;
    }

    // Get the position sample's time accuracy
    if (positionSampleRequestNodePtr->positionSampleNodePtr->timeAccuracyValid)
    {
        result = LE_OK;
        if (timeAccuracyPtr)
        {
            *timeAccuracyPtr = positionSampleRequestNodePtr->positionSampleNodePtr->timeAccuracy;
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
 * Get the position sample's UTC leap seconds in advance
 *
 * @return
 *  - LE_FAULT         Function failed to get the leap seconds.
 *  - LE_OUT_OF_RANGE  The retrieved time accuracy is invalid (set to UINT8_MAX).
 *  - LE_OK            Function succeeded.
 *
 * @note The leap seconds in advance is the accumulated time in seconds since the start of GPS Epoch
 * time (Jan 6, 1980). This value has to be added to the UTC time (since Jan. 1, 1970)
 *
 * @note Insertion of each UTC leap second is usually decided about six months in advance by the
 * International Earth Rotation and Reference Systems Service (IERS).
 *
 * @note If the caller is passing an invalid position sample reference or a null pointer into this
 *       function, it is a fatal error, the function will not return.
 *
 * @deprecated This function is deprecated, le_gnss_GetLeapSeconds should be used instead.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetGpsLeapSeconds
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN] Position sample's reference.

    uint8_t* leapSecondsPtr
        ///< [OUT] UTC leap seconds in advance in seconds
)
{
    le_result_t result;
    le_gnss_PositionSampleRequest_t* positionSampleRequestNodePtr
                                            = le_ref_Lookup(PositionSampleMap,positionSampleRef);

    // Check input pointer
    if (NULL == leapSecondsPtr)
    {
        LE_KILL_CLIENT("Invalid pointer provided!");
        return LE_FAULT;
    }

    // Check position sample's reference
    result = ValidatePositionSamplePtr(positionSampleRequestNodePtr);
    if (result != LE_OK)
    {
        return result;
    }

    // Get the position sample's leap seconds
    if (positionSampleRequestNodePtr->positionSampleNodePtr->leapSecondsValid)
    {
        result = LE_OK;
        *leapSecondsPtr = positionSampleRequestNodePtr->positionSampleNodePtr->leapSeconds;
    }
    else
    {
        result = LE_OUT_OF_RANGE;
        *leapSecondsPtr = UINT8_MAX;
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
 * @note If the caller is passing an invalid Position sample reference or null pointers into this
 *       function, it is a fatal error, the function will not return.
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
    le_gnss_PositionSampleRequest_t* positionSampleRequestNodePtr =
                                                le_ref_Lookup(PositionSampleMap,positionSampleRef);

    // Check input pointers
    if ((NULL == yearPtr)
        || (NULL == monthPtr)
        || (NULL == dayPtr))
    {
        LE_KILL_CLIENT("Invalid pointers provided!");
        return LE_FAULT;
    }

    // Check position sample's reference
    result = ValidatePositionSamplePtr(positionSampleRequestNodePtr);
    if (LE_OK != result)
    {
        return result;
    }

    // Get the position sample's date
    if (positionSampleRequestNodePtr->positionSampleNodePtr->dateValid)
    {
        result = LE_OK;
        if (yearPtr)
        {
            *yearPtr = positionSampleRequestNodePtr->positionSampleNodePtr->year;
        }
        if (monthPtr)
        {
            *monthPtr = positionSampleRequestNodePtr->positionSampleNodePtr->month;
        }
        if (dayPtr)
        {
            *dayPtr = positionSampleRequestNodePtr->positionSampleNodePtr->day;
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
 *  - LE_FAULT         Function failed to find the positionSample.
 *  - LE_OUT_OF_RANGE  At least one of the retrieved parameters is invalid (set to UINT32_MAX).
 *  - LE_OK            Function succeeded.
 *
 * @note hSpeedPtr, hSpeedAccuracyPtr can be set to NULL if not needed.
 *
 * @note Horizontal speed is in meters/second with 2 decimal places (3047 = 30.47 meters/second).
 *
 * @note Horizontal speed accuracy estimate is default set to meters/second with 1 decimal place
 *       (304 = 30.4 meters/second). To change its accuracy, call the @c le_gnss_SetDataResolution()
 *       function. Horizontal speed accuracy estimate is set as data type and accuracy from 0 to 3
 *       decimal place is set as resolution.
 *
 * @note In case the function returns LE_OUT_OF_RANGE, some of the retrieved parameters may be
 *       valid. Please compare them with UINT32_MAX.
 *
 * @note If the caller is passing an invalid Position sample reference into this function,
 *       it is a fatal error, the function will not return.
 *
 * @warning The Horizontal speed accuracy is platform dependent. Please refer to
 *          @ref platformConstraintsGnss_speedAccuracies section for full details.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetHorizontalSpeed
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN] Position sample's reference.
    uint32_t* hspeedPtr,
        ///< [OUT] Horizontal speed in meters/second [resolution 1e-2].
    uint32_t* hspeedAccuracyPtr
        ///< [OUT] Horizontal speed's accuracy estimate in meters/second.
)
{
    le_result_t result = LE_OK;
    le_gnss_PositionSampleRequest_t* positionSampleRequestNodePtr
                                            = le_ref_Lookup(PositionSampleMap,positionSampleRef);

    // Check position sample's reference
    result = ValidatePositionSamplePtr(positionSampleRequestNodePtr);
    if (result != LE_OK)
    {
        return result;
    }

    if (hspeedPtr)
    {
        if (positionSampleRequestNodePtr->positionSampleNodePtr->hSpeedValid)
        {
            *hspeedPtr = positionSampleRequestNodePtr->positionSampleNodePtr->hSpeed;
        }
        else
        {
            *hspeedPtr = UINT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (hspeedAccuracyPtr)
    {
        if ((false == positionSampleRequestNodePtr->positionSampleNodePtr->hSpeedAccuracyValid) ||
            (LE_OK != ConvertPositionData(
                                positionSampleRequestNodePtr->positionSampleNodePtr->hSpeedAccuracy,
                                LE_GNSS_DATA_HSPEEDACCURACY,
                                (int32_t*)hspeedAccuracyPtr))
           )
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
 *  - LE_OUT_OF_RANGE  At least one of the retrieved parameters is not valid (set to INT32_MAX).
 *  - LE_OK            The function succeeded.
 *
 * @note vSpeedPtr, vSpeedAccuracyPtr can be set to NULL if not needed.
 *
 * @note For a 2D position Fix, the vertical speed will be indicated as invalid
 * and set to INT32_MAX.
 *
 * @note Vertical speed accuracy estimate is default set to meters/second with 1 decimal place
 *       (304 = 30.4 meters/second). To change its accuracy, call the @c le_gnss_SetDataResolution()
 *       function. Vertical speed accuracy estimate is set as data type and accuracy from 0 to 3
 *       decimal place is set as resolution.
 *
 * @note In case the function returns LE_OUT_OF_RANGE, some of the retrieved parameters may be
 *       valid. Please compare them with INT32_MAX.
 *
 * @note If the caller is passing an invalid Position sample reference into this function,
 *       it is a fatal error, the function will not return.
 *
 * @warning The Vertical speed accuracy is platform dependent. Please refer to
 *          @ref platformConstraintsGnss_speedAccuracies section for full details.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetVerticalSpeed
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN] Position sample's reference.
    int32_t* vspeedPtr,
        ///< [OUT] Vertical speed in meters/second [resolution 1e-2].
    int32_t* vspeedAccuracyPtr
        ///< [OUT] Vertical speed's accuracy estimate
        ///<       in meters/second.
)
{
    le_result_t result = LE_OK;
    le_gnss_PositionSampleRequest_t* positionSampleRequestNodePtr
                                            = le_ref_Lookup(PositionSampleMap,positionSampleRef);

    // Check position sample's reference
    result = ValidatePositionSamplePtr(positionSampleRequestNodePtr);
    if (result != LE_OK)
    {
        return result;
    }

    if (vspeedPtr)
    {
        if (positionSampleRequestNodePtr->positionSampleNodePtr->vSpeedValid)
        {
            *vspeedPtr = positionSampleRequestNodePtr->positionSampleNodePtr->vSpeed;
        }
        else
        {
            *vspeedPtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (vspeedAccuracyPtr)
    {
        if ((false == positionSampleRequestNodePtr->positionSampleNodePtr->vSpeedAccuracyValid) ||
            (LE_OK != ConvertPositionData(
                                positionSampleRequestNodePtr->positionSampleNodePtr->vSpeedAccuracy,
                                LE_GNSS_DATA_VSPEEDACCURACY,
                                vspeedAccuracyPtr))
           )
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
 *  - LE_OUT_OF_RANGE  At least one of the retrieved parameters is invalid (set to UINT32_MAX).
 *  - LE_OK            Function succeeded.
 *
 * @note Direction and direction accuracy are given in degrees with 1 decimal place: 1755 = 175.5
 *       degrees.
 *       Direction ranges from 0 to 359.9 degrees, where 0 is True North.
 *
 * @note In case the function returns LE_OUT_OF_RANGE, some of the retrieved parameters may be
 *       valid. Please compare them with UINT32_MAX.
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
    le_gnss_PositionSampleRequest_t* positionSampleRequestNodePtr
                                            = le_ref_Lookup(PositionSampleMap,positionSampleRef);

    // Check position sample's reference
    result = ValidatePositionSamplePtr(positionSampleRequestNodePtr);
    if (result != LE_OK)
    {
        return result;
    }

    if (directionPtr)
    {
        if (positionSampleRequestNodePtr->positionSampleNodePtr->directionValid)
        {
            *directionPtr = positionSampleRequestNodePtr->positionSampleNodePtr->direction;
        }
        else
        {
            *directionPtr = UINT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (directionAccuracyPtr)
    {
        if (positionSampleRequestNodePtr->positionSampleNodePtr->directionAccuracyValid)
        {
            *directionAccuracyPtr = positionSampleRequestNodePtr->positionSampleNodePtr->
                                    directionAccuracy;
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
 *  - LE_OUT_OF_RANGE  At least one of the retrieved parameters is invalid.
 *  - LE_OK            Function succeeded.
 *
 * @note satId[] can be set to 0 if that information list index is not configured, so
 * all satellite parameters (satConst[], satSnr[],satAzim[], satElev[]) are fixed to 0.
 *
 * @note For LE_OUT_OF_RANGE returned code, invalid value depends on field type:
 * UINT16_MAX for satId, LE_GNSS_SV_CONSTELLATION_UNDEFINED for satConst, false for satUsed,
 * UINT8_MAX for satSnr, UINT16_MAX for satAzim, UINT8_MAX for satElev.
 *
 * @note In case the function returns LE_OUT_OF_RANGE, some of the retrieved parameters may be
 *       valid.
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
        ///< Satellites in View Signal To Noise Ratio (C/No)
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
    le_gnss_PositionSampleRequest_t* positionSampleRequestNodePtr
                                            = le_ref_Lookup(PositionSampleMap,positionSampleRef);
    int i;

    // Check position sample's reference
    result = ValidatePositionSamplePtr(positionSampleRequestNodePtr);
    if (result != LE_OK)
    {
        return result;
    }

    if (satIdPtr)
    {
        if (positionSampleRequestNodePtr->positionSampleNodePtr->satInfoValid)
        {
            for(i=0; i<*satIdNumElementsPtr; i++)
            {
                satIdPtr[i] = positionSampleRequestNodePtr->positionSampleNodePtr->satInfo[i].satId;
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
        if (positionSampleRequestNodePtr->positionSampleNodePtr->satInfoValid)
        {
            for(i=0; i<*satConstNumElementsPtr; i++)
            {
                satConstPtr[i] = positionSampleRequestNodePtr->positionSampleNodePtr->
                                                               satInfo[i].satConst;
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
        if (positionSampleRequestNodePtr->positionSampleNodePtr->satsUsedCountValid)
        {
            for(i=0; i<*satUsedNumElementsPtr; i++)
            {
                satUsedPtr[i] = positionSampleRequestNodePtr->positionSampleNodePtr->
                                                              satInfo[i].satUsed;
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
        if (positionSampleRequestNodePtr->positionSampleNodePtr->satInfoValid)
        {
            for(i=0; i<*satSnrNumElementsPtr; i++)
            {
                satSnrPtr[i] = positionSampleRequestNodePtr->positionSampleNodePtr->
                                                             satInfo[i].satSnr;
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
        if (positionSampleRequestNodePtr->positionSampleNodePtr->satInfoValid)
        {
            for(i=0; i<*satAzimNumElementsPtr; i++)
            {
                satAzimPtr[i] = positionSampleRequestNodePtr->positionSampleNodePtr->
                                                              satInfo[i].satAzim;
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
        if (positionSampleRequestNodePtr->positionSampleNodePtr->satInfoValid)
        {
            for(i=0; i<*satElevNumElementsPtr; i++)
            {
                satElevPtr[i] = positionSampleRequestNodePtr->positionSampleNodePtr->
                                                              satInfo[i].satElev;
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
 * Get the SBAS constellation given the SBAS satellite number ID.
 *
 */
//--------------------------------------------------------------------------------------------------
le_gnss_SbasConstellationCategory_t le_gnss_GetSbasConstellationCategory
(
    uint16_t  satId      ///< [IN] SBAS satellite number ID, referring to NMEA standard.
)
{
    le_gnss_SbasConstellationCategory_t sbasCategory;
    switch (satId)
    {
        case SBAS_EGNOS_SV_ID_33:
        case SBAS_EGNOS_SV_ID_36:
        case SBAS_EGNOS_SV_ID_37:
        case SBAS_EGNOS_SV_ID_39:
        case SBAS_EGNOS_SV_ID_44:
        case SBAS_EGNOS_SV_ID_49:
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
        case SBAS_SDCM_SV_ID_38:
        case SBAS_SDCM_SV_ID_53:
        case SBAS_SDCM_SV_ID_54:
            sbasCategory = LE_GNSS_SBAS_SDCM;
            break;
        default:
            sbasCategory = LE_GNSS_SBAS_UNKNOWN;
            LE_WARN("SBAS unknown category, satId %d", satId);
            break;
    }
    LE_DEBUG("satellite id, SBAS category (%d, %d)", satId, sbasCategory);

    return sbasCategory;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the Satellites Vehicle status.
 *
 * @return
 *  - LE_FAULT         Function failed to find the positionSample.
 *  - LE_OUT_OF_RANGE  At least one of the retrieved parameters is invalid (set to UINT8_MAX).
 *  - LE_OK            Function succeeded.
 *
 * @note In case the function returns LE_OUT_OF_RANGE, some of the retrieved parameters may be
 *       valid. Please compare them with UINT8_MAX.
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
    le_gnss_PositionSampleRequest_t* positionSampleRequestNodePtr
                                            = le_ref_Lookup(PositionSampleMap,positionSampleRef);

    // Check position sample's reference
    result = ValidatePositionSamplePtr(positionSampleRequestNodePtr);
    if (result != LE_OK)
    {
        return result;
    }

    // Satellites in View count
    if (satsInViewCountPtr)
    {
        if (positionSampleRequestNodePtr->positionSampleNodePtr->satsInViewCountValid)
        {
            *satsInViewCountPtr = positionSampleRequestNodePtr->positionSampleNodePtr->
                                                                satsInViewCount;
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
        if (positionSampleRequestNodePtr->positionSampleNodePtr->satsTrackingCountValid)
        {
            *satsTrackingCountPtr = positionSampleRequestNodePtr->positionSampleNodePtr->
                                                                  satsTrackingCount;
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
        if (positionSampleRequestNodePtr->positionSampleNodePtr->satsUsedCountValid)
        {
            *satsUsedCountPtr = positionSampleRequestNodePtr->positionSampleNodePtr->
                                                              satsUsedCount;
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
 * Get the DOP parameter (Dilution Of Precision) for the fixed position.
 *
 * @return
 *  - LE_FAULT         Function failed to find the DOP value.
 *  - LE_OUT_OF_RANGE  The retrieved parameter is invalid (set to UINT16_MAX).
 *  - LE_OK            Function succeeded.
 *
 * @note This function replaces the deprecated function le_gnss_GetDop().
 *
 * @note The DOP value is given with 3 decimal places by default like: DOP value 2200 = 2.200
 *       The resolution can be modified by calling @c le_gnss_SetDopResolution() function.
 *
 * @note If the caller is passing an invalid Position sample reference into this function,
 *       it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetDilutionOfPrecision
(
    le_gnss_SampleRef_t positionSampleRef,      ///< [IN] Position sample's reference.
    le_gnss_DopType_t dopType,                  ///< [IN] Dilution of Precision type.
    uint16_t* dopPtr                            ///< [OUT] Dilution of Precision corresponding to
                                                ///< the dopType in the specified resolution.
)
{
    uint32_t dop;
    bool dopValid = false;
    le_gnss_PositionSampleRequest_t* positionSampleRequestNodePtr
                                            = le_ref_Lookup(PositionSampleMap,positionSampleRef);
    // Check position sample's reference
    le_result_t result = ValidatePositionSamplePtr(positionSampleRequestNodePtr);
    if (LE_OK != result)
    {
        return result;
    }

    if (dopPtr)
    {
        *dopPtr = UINT16_MAX;

        switch(dopType)
        {
            case LE_GNSS_PDOP:
            {
                if (positionSampleRequestNodePtr->positionSampleNodePtr->pdopValid)
                {
                    // update resolution
                    dop = ConvertDop(positionSampleRequestNodePtr->positionSampleNodePtr->pdop);
                    dopValid = true;
                }
            }
            break;
            case LE_GNSS_HDOP:
            {
                if (positionSampleRequestNodePtr->positionSampleNodePtr->hdopValid)
                {
                    // update resolution
                    dop = ConvertDop(positionSampleRequestNodePtr->positionSampleNodePtr->hdop);
                    dopValid = true;
                }
            }
            break;
            case LE_GNSS_VDOP:
            {
                if (positionSampleRequestNodePtr->positionSampleNodePtr->vdopValid)
                {
                    // update resolution
                    dop = ConvertDop(positionSampleRequestNodePtr->positionSampleNodePtr->vdop);
                    dopValid = true;
                }
            }
            break;
            case LE_GNSS_GDOP:
            {
                if (positionSampleRequestNodePtr->positionSampleNodePtr->gdopValid)
                {
                    // update resolution
                    dop = ConvertDop(positionSampleRequestNodePtr->positionSampleNodePtr->gdop);
                    dopValid = true;
                }
            }
            break;
            case LE_GNSS_TDOP:
            {
                if (positionSampleRequestNodePtr->positionSampleNodePtr->tdopValid)
                {
                    // update resolution
                    dop = ConvertDop(positionSampleRequestNodePtr->positionSampleNodePtr->tdop);
                    dopValid = true;
                }
            }
            break;
            default:
            {
                LE_ERROR("Unknown dilution of precision type %d", dopType);
            }
            break;
         };

         // Test if the dop value exceeds a uint16_t after the conversion
         if ((true == dopValid) && (!(dop >> 16)))
         {
             *dopPtr = (uint16_t)dop;
             return LE_OK;
         }
    }

    return LE_OUT_OF_RANGE;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the DOP parameters (Dilution Of Precision) for the fixed position
 *
 * @return
 *  - LE_FAULT         Function failed to find the positionSample.
 *  - LE_OUT_OF_RANGE  At least one of the retrieved parameters is invalid (set to UINT16_MAX).
 *  - LE_OK            Function succeeded.
 *
 * @deprecated This function is deprecated, le_gnss_GetDilutionOfPrecision() should be used for
 *             new code.
 *
 * @note The DOP values are given with 3 decimal places like: DOP value 2200 = 2.200
 *
 * @note In case the function returns LE_OUT_OF_RANGE, some of the retrieved parameters may be
 *       valid. Please compare them with UINT16_MAX.
 *
 * @note If the caller is passing an invalid Position sample reference into this function,
 *       it is a fatal error, the function will not return.
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
    le_gnss_PositionSampleRequest_t* positionSampleRequestNodePtr
                                            = le_ref_Lookup(PositionSampleMap,positionSampleRef);

    // Check position sample's reference
    result = ValidatePositionSamplePtr(positionSampleRequestNodePtr);
    if (LE_OK != result)
    {
        return result;
    }

    if (hdopPtr)
    {
        if ((positionSampleRequestNodePtr->positionSampleNodePtr->hdopValid) &&
            (!(positionSampleRequestNodePtr->positionSampleNodePtr->hdop >> 16)))
        {
            *hdopPtr = (uint16_t)positionSampleRequestNodePtr->positionSampleNodePtr->hdop;
        }
        else
        {
            *hdopPtr = UINT16_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (vdopPtr)
    {
        if ((positionSampleRequestNodePtr->positionSampleNodePtr->vdopValid) &&
            (!(positionSampleRequestNodePtr->positionSampleNodePtr->vdop >> 16)))
        {
            *vdopPtr = (uint16_t)positionSampleRequestNodePtr->positionSampleNodePtr->vdop;
        }
        else
        {
            *vdopPtr = UINT16_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (pdopPtr)
    {
        if ((positionSampleRequestNodePtr->positionSampleNodePtr->pdopValid) &&
            (!(positionSampleRequestNodePtr->positionSampleNodePtr->pdop >> 16)))
        {
            *pdopPtr = (uint16_t)positionSampleRequestNodePtr->positionSampleNodePtr->pdop;
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
 *  - LE_OUT_OF_RANGE  The altitudeOnWgs84 is invalid (set to INT32_MAX).
 *  - LE_OK            Function succeeded.
 *
 * @note altitudeOnWgs84 is in meters, with respect to the WGS-84 ellipsoid with 3 decimal
 *       places (3047 = 3.047 meters).
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
    le_gnss_PositionSampleRequest_t * positionSampleRequestNodePtr
                                            = le_ref_Lookup(PositionSampleMap,positionSampleRef);

    // Check input pointer
    if (NULL == altitudeOnWgs84Ptr)
    {
        LE_KILL_CLIENT("Invalid pointer provided!");
        return LE_FAULT;
    }

    // Check position sample's reference
    result = ValidatePositionSamplePtr(positionSampleRequestNodePtr);
    if (result != LE_OK)
    {
        return result;
    }

    if (positionSampleRequestNodePtr->positionSampleNodePtr->altitudeOnWgs84Valid)
    {
        *altitudeOnWgs84Ptr = positionSampleRequestNodePtr->positionSampleNodePtr->altitudeOnWgs84;
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
 *  - LE_OUT_OF_RANGE  The magneticDeviation is invalid (set to INT32_MAX).
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
    le_gnss_PositionSampleRequest_t* positionSampleRequestNodePtr
                                            = le_ref_Lookup(PositionSampleMap,positionSampleRef);

    // Check position sample's reference
    result = ValidatePositionSamplePtr(positionSampleRequestNodePtr);
    if (result != LE_OK)
    {
        return result;
    }

    if (magneticDeviationPtr)
    {
        if (positionSampleRequestNodePtr->positionSampleNodePtr->magneticDeviationValid)
        {
            *magneticDeviationPtr =
                positionSampleRequestNodePtr->positionSampleNodePtr->magneticDeviation;
        }
        else
        {
            *magneticDeviationPtr = INT32_MAX;
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
    le_gnss_PositionSampleRequest_t*   positionSampleRequestNodePtr=NULL;

    // Create the position sample node.
    positionSampleRequestNodePtr =
               (le_gnss_PositionSampleRequest_t*)le_mem_ForceAlloc(PositionSampleRequestPoolRef);
    positionSampleRequestNodePtr->positionSampleNodePtr =
               (le_gnss_PositionSample_t*)le_mem_ForceAlloc(PositionSamplePoolRef);

    // Copy the position sample to the position sample node
    memcpy(positionSampleRequestNodePtr->positionSampleNodePtr, &LastPositionSample,
           sizeof(le_gnss_PositionSample_t));

    // Add the node to the queue of the list by passing in the node's link.
    positionSampleRequestNodePtr->positionSampleNodePtr->link = LE_DLS_LINK_INIT;
    le_dls_Queue(&PositionSampleList, &(positionSampleRequestNodePtr->positionSampleNodePtr->link));

    LE_DEBUG("Get sample %p", positionSampleRequestNodePtr->positionSampleNodePtr);

    positionSampleRequestNodePtr->sessionRef = le_gnss_GetClientSessionRef();

    le_gnss_SampleRef_t reqRef = le_ref_CreateRef(PositionSampleMap, positionSampleRequestNodePtr);
    positionSampleRequestNodePtr->positionSampleRef = reqRef;

    // Create a safe reference and call the client's handler
    return reqRef;
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
    le_gnss_PositionSampleRequest_t* positionSampleRequestNodePtr = le_ref_Lookup(PositionSampleMap,
                                                                positionSampleRef);

    // Check position sample's reference
    le_result_t result = ValidatePositionSamplePtr(positionSampleRequestNodePtr);
    if (result != LE_OK)
    {
        return;
    }

    le_ref_DeleteRef(PositionSampleMap,positionSampleRef);
    le_mem_Release(positionSampleRequestNodePtr->positionSampleNodePtr);
    le_mem_Release(positionSampleRequestNodePtr);
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
 *
 * @warning Some constellation types are unsupported depending on the plateform. Please refer to
 *          @ref platformConstraintsGnss_ConstellationType section for full details.
 *
 * @warning The settings are platform dependent. Please refer to
 *          @ref platformConstraintsGnss_SettingConfiguration section for full details.
 *
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
 *
 * @note If the caller is passing a null pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetConstellation
(
    le_gnss_ConstellationBitMask_t *constellationMaskPtr ///< [OUT] GNSS constellation used in
                                                         ///< solution
)
{
    le_result_t result = LE_FAULT;

    if (NULL == constellationMaskPtr)
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
 * Set the area for the GNSS constellation
 *
 * @return
 *  - LE_OK            The function succeeded.
 *  - LE_FAULT         The function failed.
 *  - LE_UNSUPPORTED   If the request is not supported.
 *  - LE_NOT_PERMITTED If the GNSS device is not initialized, disabled or active.
 *  - LE_BAD_PARAMETER Invalid constellation area.
 *
 * @warning The settings are platform dependent. Please refer to
 *          @ref platformConstraintsGnss_SettingConfiguration section for full details.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_SetConstellationArea
(
    le_gnss_Constellation_t satConstellation,       ///< [IN] GNSS constellation used in solution.
    le_gnss_ConstellationArea_t constellationArea   ///< [IN] GNSS constellation area.
)
{
    le_result_t result = LE_FAULT;

    // Check the GNSS device state
    switch (GnssState)
    {
        case LE_GNSS_STATE_READY:
        {
            // Set GNSS constellation area
            result = pa_gnss_SetConstellationArea(satConstellation, constellationArea);
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
 * Get the area for the GNSS constellation
 *
 * @return
 *  - LE_OK            On success
 *  - LE_FAULT         On failure
 *  - LE_UNSUPPORTED   Request not supported
 *  - LE_NOT_PERMITTED If the GNSS device is not initialized, disabled or active.
 *
 * @note If the caller is passing a null pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetConstellationArea
(
    le_gnss_Constellation_t satConstellation,         ///< [IN] GNSS constellation used in solution.
    le_gnss_ConstellationArea_t* constellationAreaPtr ///< [OUT] GNSS constellation area.
)
{
    le_result_t result = LE_FAULT;

    if (NULL == constellationAreaPtr)
    {
        LE_KILL_CLIENT("Pointer is NULL !");
        return LE_FAULT;
    }

    // Check the GNSS device state
    switch (GnssState)
    {
        case LE_GNSS_STATE_READY:
        {
            // Get GNSS constellation area
            result = pa_gnss_GetConstellationArea(satConstellation, constellationAreaPtr);
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
 *  - LE_FAULT         The function failed.
 *  - LE_OK            The function succeeded.
 *
 * @warning The settings are platform dependent. Please refer to
 *          @ref platformConstraintsGnss_SettingConfiguration section for full details.
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
 *  - LE_FAULT         The function failed.
 *  - LE_OK            The function succeeded.
 *
 * @warning The settings are platform dependent. Please refer to
 *          @ref platformConstraintsGnss_SettingConfiguration section for full details.
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
    int       fd      ///< [IN] extended ephemeris file descriptor
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
 * @note If the caller is passing null pointers into this function, it is a fatal error, the
 *       function will not return.
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
 * This function must be called to inject the UTC time into the GNSS device.
 *
 * @return
 *  - LE_OK            The function succeeded.
 *  - LE_FAULT         The function failed to inject the UTC time.
 *  - LE_TIMEOUT       A time-out occurred.
 *
 * @note It is mandatory to enable the 'Extended Ephemeris' file injection into the GNSS device with
 * le_gnss_EnableExtendedEphemerisFile() before injecting time with this API.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_InjectUtcTime
(
    uint64_t timeUtc,   ///< [IN] UTC time since Jan. 1, 1970 in milliseconds
    uint32_t timeUnc    ///< [IN] Time uncertainty in milliseconds
)
{
    return pa_gnss_InjectUtcTime(timeUtc, timeUnc);
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
 * This function starts the GNSS device in the specified start mode.
 *
 * @return
 *  - LE_OK              The function succeeded.
 *  - LE_BAD_PARAMETER   Invalid start mode
 *  - LE_FAULT           The function failed.
 *  - LE_DUPLICATE       If the GNSS device is already started.
 *  - LE_NOT_PERMITTED   If the GNSS device is not initialized or disabled.
 *
 * @warning This function may be subject to limitations depending on the platform. Please refer to
 *          the @ref platformConstraintsGnss page.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_StartMode
(
    le_gnss_StartMode_t mode    ///< [IN] Start mode
)
{
    if (mode >= LE_GNSS_UNKNOWN_START)
    {
        LE_ERROR("Invalid start mode %d", mode);
        return LE_BAD_PARAMETER;
    }

    if (LE_GNSS_STATE_READY == GnssState)
    {
        if (LE_FAULT == pa_gnss_DeleteAssistData(mode))
        {
            LE_ERROR("le_gnss_StartMode fail");
            return LE_FAULT;
        }
    }

    return le_gnss_Start();
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
            if (LE_OK == result)
            {
                // Initialize last Position sample
                memset(&LastPositionSample, 0, sizeof(LastPositionSample));
                LastPositionSample.fixState = LE_GNSS_STATE_FIX_NO_POS;

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
    le_result_t result = LE_OK;

    // Check the GNSS device state
    switch (GnssState)
    {
        case LE_GNSS_STATE_ACTIVE:
        {
            // Stop and start GNSS
            if ((LE_FAULT == pa_gnss_ForceEngineStop()) ||
                (LE_FAULT == pa_gnss_DeleteAssistData(LE_GNSS_HOT_START)) ||
                (LE_FAULT == pa_gnss_Start()))
            {
                // GNSS device state is updated ONLY if the restart failed
                LE_ERROR("le_gnss_ForceHotRestart fails");
                GnssState = LE_GNSS_STATE_READY;
                result = LE_FAULT;
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
    le_result_t result = LE_OK;

    // Check the GNSS device state
    switch (GnssState)
    {
        case LE_GNSS_STATE_ACTIVE:
        {
            // Stop, clean data and restart GNSS
            if ((LE_FAULT == pa_gnss_ForceEngineStop()) ||
                (LE_FAULT == pa_gnss_DeleteAssistData(LE_GNSS_WARM_START)) ||
                (LE_FAULT == pa_gnss_Start()))
            {
                // GNSS device state is updated ONLY if the restart failed
                LE_ERROR("le_gnss_ForceWarmRestart fails");
                GnssState = LE_GNSS_STATE_READY;
                result = LE_FAULT;
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
    le_result_t result = LE_OK;

    // Check the GNSS device state
    switch (GnssState)
    {
        case LE_GNSS_STATE_ACTIVE:
        {
            // Stop, clean data and restart GNSS
            if ((LE_FAULT == pa_gnss_ForceEngineStop()) ||
                (LE_FAULT == pa_gnss_DeleteAssistData(LE_GNSS_COLD_START)) ||
                (LE_FAULT == pa_gnss_Start()))
            {
                // GNSS device state is updated ONLY if the restart failed
                LE_ERROR("le_gnss_ForceColdRestart fails");
                GnssState = LE_GNSS_STATE_READY;
                result = LE_FAULT;
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
    le_result_t result = LE_OK;

    // Check the GNSS device state
    switch (GnssState)
    {
        case LE_GNSS_STATE_ACTIVE:
        {
            // Stop, clean data and restart GNSS
            if ((LE_FAULT == pa_gnss_ForceEngineStop()) ||
                (LE_FAULT == pa_gnss_DeleteAssistData(LE_GNSS_FACTORY_START)) ||
                (LE_FAULT == pa_gnss_Start()))
            {
                // GNSS device state is updated ONLY if the restart failed
                LE_ERROR("le_gnss_ForceFactoryRestart fails");
                GnssState = LE_GNSS_STATE_READY;
                result = LE_FAULT;
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
 *  - LE_FAULT         If there are some other errors.
 *
 * @note If the caller is passing a null pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetTtff
(
    uint32_t* ttffPtr     ///< [OUT] TTFF in milliseconds
)
{
    if (ttffPtr == NULL)
    {
        LE_KILL_CLIENT("ttffPtr is NULL.");
        return LE_FAULT;
    }

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
 * @warning The settings are platform dependent. Please refer to
 *          @ref platformConstraintsGnss_SettingConfiguration section for full details.
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
 * @warning The settings are platform dependent. Please refer to
 *          @ref platformConstraintsGnss_SettingConfiguration section for full details.
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
 *
 * @warning The settings are platform dependent. Please refer to
 *          @ref platformConstraintsGnss_SettingConfiguration section for full details.
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
 *
 * @note If the caller is passing a null pointer into this function, it is a fatal error, the
 *       function will not return.
*/
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetAcquisitionRate
(
    uint32_t* ratePtr      ///< Acquisition rate in milliseconds.
)
{
    le_result_t result = LE_FAULT;

    if (NULL == ratePtr)
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

// -------------------------------------------------------------------------------------------------
/**
 * Get the minimum NMEA rate supported on this platform
 *
 * @return LE_OK               Function succeeded.
 * @return LE_FAULT            Function failed.
 */
// -------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetMinNmeaRate
(
    uint32_t* minNmeaRatePtr    ///< [OUT] Minimum NMEA rate in milliseconds.
)
{
    return pa_gnss_GetMinNmeaRate(minNmeaRatePtr);
}

// -------------------------------------------------------------------------------------------------
/**
 * Get the maximum NMEA rate supported on this platform
 *
 * @return LE_OK               Function succeeded.
 * @return LE_FAULT            Function failed.
 */
// -------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetMaxNmeaRate
(
    uint32_t* maxNmeaRatePtr    ///< [OUT] Maximum NMEA rate in milliseconds.
)
{
    return pa_gnss_GetMaxNmeaRate(maxNmeaRatePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Returns a bitmask containing all NMEA sentences supported on this platform
 *
 * @return LE_OK               Function succeeded.
 * @return LE_FAULT            Function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetSupportedNmeaSentences
(
    le_gnss_NmeaBitMask_t* nmeaMaskPtr    ///< [OUT] Supported NMEA sentences
)
{
    return pa_gnss_GetSupportedNmeaSentences(nmeaMaskPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Returns a bitmask containing all satellite constellations supported on this platform
 *
 * @return LE_OK               Function succeeded.
 * @return LE_FAULT            Function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetSupportedConstellations
(
    le_gnss_ConstellationBitMask_t* constellationMaskPtr    ///< [OUT] Supported GNSS constellations
)
{
    return pa_gnss_GetSupportedConstellations(constellationMaskPtr);
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
 *
 * @warning The settings are platform dependent. Please refer to
 *          @ref platformConstraintsGnss_SettingConfiguration section for full details.
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
 *
 * @note If the caller is passing a null pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetSuplAssistedMode
(
    le_gnss_AssistedMode_t * assistedModePtr      ///< [OUT] Assisted-GNSS mode.
)
{
    if (NULL == assistedModePtr)
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
 *       terminator) size, it is a fatal error, the function will not return.
 *
 * @warning The settings are platform dependent. Please refer to
 *          @ref platformConstraintsGnss_SettingConfiguration section for full details.
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
 * This function gets leap seconds information
 *
 * @return
 *  - LE_OK            Function succeeded.
 *  - LE_FAULT         Function failed to get the data.
 *  - LE_TIMEOUT       Timeout occured.
 *  - LE_UNSUPPORTED   Not supported on this platform.
 *
 * @note Insertion of each UTC leap second is usually decided about six months in advance by the
 * International Earth Rotation and Reference Systems Service (IERS).
 *
 * @note If the caller is passing a null pointer into this function, it is considered a fatal
 * error and the function will not return.
 *
 * @note If the return value of a parameter is INT32_MAX/UINT64_MAX, the parameter is not valid.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetLeapSeconds
(
    uint64_t* gpsTimePtr,              ///< [OUT] The number of milliseconds of GPS time since
                                       ///<       Jan. 6, 1980
    int32_t* currentLeapSecondsPtr,    ///< [OUT] Current UTC leap seconds value in milliseconds
    uint64_t* changeEventTimePtr,      ///< [OUT] The number of milliseconds since Jan. 6, 1980
                                       ///<       to the next leap seconds change event
    int32_t* nextLeapSecondsPtr        ///< [OUT] UTC leap seconds value to be applied at the
                                       ///<       change event time in milliseconds
)
{
    if ((!gpsTimePtr) || (!currentLeapSecondsPtr) || (!changeEventTimePtr) || (!nextLeapSecondsPtr))
    {
        LE_ERROR("Null pointer provided: gpsTimePtr: %p, currentLeapSecondsPtr: %p, "
                 "changeEventTimePtr: %p, nextLeapSecondsPtr: %p",
                  gpsTimePtr, currentLeapSecondsPtr, changeEventTimePtr, nextLeapSecondsPtr);

        return LE_FAULT;
    }

    return pa_gnss_GetLeapSeconds(gpsTimePtr, currentLeapSecondsPtr,
                                  changeEventTimePtr, nextLeapSecondsPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function injects the SUPL certificate to be used in A-GNSS sessions.
 *
 * @return
 *  - LE_OK on success
 *  - LE_BAD_PARAMETER on invalid parameter
 *  - LE_FAULT on failure
 *  - LE_BUSY service is busy
 *  - LE_TIMEOUT a time-out occurred
 *
 * @note If the SUPL certificate size is bigger than the Maximum SUPL certificate size,
 *       it is a fatal error, the function will not return.
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
    return pa_gnss_InjectSuplCertificate(suplCertificateId,
                                         suplCertificateLen,
                                         suplCertificatePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function deletes the SUPL certificate.
 *
 * @return
 *  - LE_OK on success
 *  - LE_BAD_PARAMETER on invalid parameter
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
 *
 * @note Some NMEA sentences are unsupported depending on the plateform. Please refer to
 *       @ref platformConstraintsGnss_nmeaMask section for full details. Setting an unsuported NMEA
 *       sentence won't report an error.
 *
 * @warning The settings are platform dependent. Please refer to
 *          @ref platformConstraintsGnss_SettingConfiguration section for full details.
 *
 * @deprecated LE_GNSS_NMEA_MASK_PQXFI is deprecated. LE_GNSS_NMEA_MASK_PTYPE should be used
 *             instead. Setting LE_GNSS_NMEA_MASK_PTYPE will also set LE_GNSS_NMEA_MASK_PQXFI.
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
 * @note If the caller is passing a null pointer to this function, it is a fatal error, the
 *       function will not return.
 *
 * @note Some NMEA sentences are unsupported depending on the plateform. Please refer to
 *       @ref platformConstraintsGnss_nmeaMask section for full details. The bit mask for an unset
 *       or unsupported NMEA sentence is zero.
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
 *
 * @warning The settings are platform dependent. Please refer to
 *          @ref platformConstraintsGnss_SettingConfiguration section for full details.
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
 * @note If the caller is passing a null pointer to this function, it is a fatal error, the
 *       function will not return.
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

//--------------------------------------------------------------------------------------------------
/**
 * Set the resolution for the DOP parameters
 *
 * @return LE_OK               Function succeeded.
 * @return LE_BAD_PARAMETER    Invalid parameter provided.
 * @return LE_FAULT            Function failed.
 *
 * @note The function sets the same resolution to all DOP values returned by
 *       le_gnss_GetDilutionOfPrecision() API. The resolution setting takes effect immediately.
 *
 * @note The resolution setting is done per client session.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_SetDopResolution
(
    le_gnss_Resolution_t resolution    ///< [IN] Resolution.
)
{
    le_gnss_Client_t* clientRequestPtr = NULL;

    if (resolution >= LE_GNSS_RES_UNKNOWN)
    {
        LE_ERROR("Invalid resolution (%d)", resolution);
        return LE_BAD_PARAMETER;
    }

    clientRequestPtr = GetClientSessionReference();

    if (NULL == clientRequestPtr)
    {
        LE_ERROR("clientRequestPtr is NULL");
        return LE_FAULT;
    }

    clientRequestPtr->dopResolution = resolution;
    LE_DEBUG("clientRequest %p, resolution %d saved", clientRequestPtr, (int)resolution);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the resolution for the specific type of data
 *
 * @return LE_OK               Function succeeded.
 * @return LE_BAD_PARAMETER    Invalid parameter provided.
 * @return LE_FAULT            Function failed.
 *
 * @note The resolution setting takes effect immediately.
 *
 * @note The resolution setting is done per client session.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_SetDataResolution
(
    le_gnss_DataType_t dataType,
        ///< [IN] Data type.
    le_gnss_Resolution_t resolution
        ///< [IN] Resolution.
)
{
    le_gnss_Client_t* clientRequestPtr = NULL;

    if (resolution >= LE_GNSS_RES_UNKNOWN)
    {
        LE_ERROR("Invalid resolution (%d)", resolution);
        return LE_BAD_PARAMETER;
    }

    clientRequestPtr = GetClientSessionReference();

    if (NULL == clientRequestPtr)
    {
        LE_ERROR("clientRequestPtr is NULL");
        return LE_FAULT;
    }

    switch(dataType)
    {
        case LE_GNSS_DATA_VACCURACY:
            clientRequestPtr->vAccuracyResolution = resolution;
            break;
        case LE_GNSS_DATA_VSPEEDACCURACY:
            clientRequestPtr->vSpeedAccuracyResolution = resolution;
            break;
        case LE_GNSS_DATA_HSPEEDACCURACY:
            clientRequestPtr->hSpeedAccuracyResolution = resolution;
            break;
        default:
            LE_ERROR("Unknown data type");
            return LE_BAD_PARAMETER;
    }

    LE_DEBUG("clientRequest %p, data type %d, resolution %d saved",
             clientRequestPtr, (int)dataType, (int)resolution);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Enables the EXT_GPS_LNA_EN signal
 *
 * @return LE_OK               Function succeeded.
 * @return LE_UNSUPPORTED      Function not supported on this platform
 * @return LE_NOT_PERMITTED    GNSS is not in the ready state
 *
 * @note The EXT_GPS_LNA_EN signal will be set high when the GNSS state is active
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_EnableExternalLna
(
    void
)
{
    if(LE_GNSS_STATE_READY != GnssState)
    {
        return LE_NOT_PERMITTED;
    }

    return pa_gnss_EnableExternalLna();
}

//--------------------------------------------------------------------------------------------------
/**
 * Disables the EXT_GPS_LNA_EN signal
 *
 * @return LE_OK               Function succeeded.
 * @return LE_UNSUPPORTED      Function not supported on this platform
 * @return LE_NOT_PERMITTED    GNSS is not in the ready state
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_DisableExternalLna
(
    void
)
{
    if(LE_GNSS_STATE_READY != GnssState)
    {
        return LE_NOT_PERMITTED;
    }

    return pa_gnss_DisableExternalLna();
}

//--------------------------------------------------------------------------------------------------
/**
 * This function converts a location data parameter from/to multi-coordinate system
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 *  - LE_BAD_PARAMETER Invalid parameter provided.
 *  - LE_UNSUPPORTED request not supported
 *
 * @note The resolution of location data parameter remains unchanged after the conversion.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_ConvertDataCoordinateSystem
(
    le_gnss_CoordinateSystem_t coordinateSrc,    ///< [IN] Coordinate system to convert from.
    le_gnss_CoordinateSystem_t coordinateDst,    ///< [IN] Coordinate system to convert to.
    le_gnss_LocationDataType_t locationDataType, ///< [IN] Type of location data to convert.
    int64_t locationDataSrc,                     ///< [IN] Data to convert.
    int64_t* locationDataDstPtr                  ///< [OUT] Data after the conversion.
)
{
    if (NULL == locationDataDstPtr)
    {
        LE_ERROR("locationDataDstPtr is NULL");
        return LE_FAULT;
    }

    if (coordinateSrc == coordinateDst)
    {
        LE_ERROR("Same coordinate system !");
        return LE_BAD_PARAMETER;
    }

    if ((coordinateSrc >= LE_GNSS_COORDINATE_SYSTEM_MAX) ||
        (coordinateDst >= LE_GNSS_COORDINATE_SYSTEM_MAX))
    {
        LE_ERROR("Bad coordinate system !");
        return LE_BAD_PARAMETER;
    }

    if (locationDataType >= LE_GNSS_POS_MAX)
    {
        LE_ERROR("Bad location data type !");
        return LE_BAD_PARAMETER;
    }

    return pa_gnss_ConvertDataCoordinateSystem(coordinateSrc,
                                               coordinateDst,
                                               locationDataType,
                                               locationDataSrc,
                                               locationDataDstPtr);
}
