#ifndef GNSS_GPSAPP_H_
#define GNSS_GPSAPP_H_

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
* Maximum number of times to try and reach sufficient location accuracy
*
*/
//--------------------------------------------------------------------------------------------------
#define MAX_LOOP_COUNT  30

//--------------------------------------------------------------------------------------------------
/**
* Data structure for storing the coordinate information
*
*/
//--------------------------------------------------------------------------------------------------
typedef struct
{
    double minLat;
    double minLon;
    double maxLat;
    double maxLon;
    double currentLat;
    double currentLon;
} CoordinateInformation_t;
CoordinateInformation_t* CoordinateInformationPtr;

//--------------------------------------------------------------------------------------------------
/**
* Initiates Data connection and cURLs coordinates to mapbox to get get geocoding information
*
*/
//--------------------------------------------------------------------------------------------------
void gc_InitiateGeocode
(
    CoordinateInformation_t* CoordinateInformationPtr, ///< [IN] Pointer to coordinate information struct
    bool bbox,                                         ///< [IN] The boundary box in which to search POI
    char* poi,                                         ///< [IN] POI name
    bool locate                                        ///< [IN] Indicates whether we are locating device or searching for POI
);

//--------------------------------------------------------------------------------------------------
/**
* Initiates GNSS and watches coordinates until the desired accuracy is reached
*
*/
//--------------------------------------------------------------------------------------------------
void gnss_InitiateWatchGNSS
(
    int accuracy,       ///< [IN] Accuracy specified in command line
    bool bbox,          ///< [IN] The boundary box in which to search POI
    char* poiName,      ///< [IN] POI name
    bool locate         ///< [IN] Indicates whether we are locating device or searching for POI
);

#endif // GNSS_GPSAPP_H_