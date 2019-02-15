#ifndef GEOCODING_GPSAPP_H_
#define GEOCODING_GPSAPP_H_

#include "gnss.h"
#include <curl/curl.h>

//-------------------------------------------------------------------------------------------------
/**
* Name of event for updated search results
*/
//-------------------------------------------------------------------------------------------------
#define EVENT_NAME "Search Result Updated"

//-------------------------------------------------------------------------------------------------
/**
* Maximum number of results returned by MapBox
*/
//-------------------------------------------------------------------------------------------------
#define MAX_NUM_RESULTS 5

//--------------------------------------------------------------------------------------------------
/**
* MapBox Request options and access token (application will not work without a mapbox api token)
*
*/
//--------------------------------------------------------------------------------------------------
#define MAPBOX_ACCESS_TOKEN             ""
#define MAPBOX_REVERSE_GEOCODE_REQUEST  "https://api.mapbox.com/geocoding/v5/mapbox.places/LONGITUDE%2C%20LATITUDE.json?access_token="
#define MAPBOX_POI_PROXIMITY_REQUEST    "https://api.mapbox.com/geocoding/v5/mapbox.places/POINAME.json?proximity=COORDINATES&access_token="
#define MAPBOX_POI_BBOX_REQUEST         "https://api.mapbox.com/geocoding/v5/mapbox.places/POINAME.json?bbox=COORDINATES&access_token="

//--------------------------------------------------------------------------------------------------
/**
* Constants to calculate distance between two coordinates
*
*/
//--------------------------------------------------------------------------------------------------
#define PI     3.141592653589793
#define RADIUS 6371

//--------------------------------------------------------------------------------------------------
/**
* Updated search results event ID
*
*/
//--------------------------------------------------------------------------------------------------
le_event_Id_t DoneEvent;

//-------------------------------------------------------------------------------------------------
/**
* Structure to hold the updated search results
*/
//-------------------------------------------------------------------------------------------------
typedef struct
{
    bool searchDone;
    bool error;
    char* result;
    double distance;
} SearchDone_t;
SearchDone_t SearchResultUpdated;

//--------------------------------------------------------------------------------------------------
/**
* Data structure for storing the incoming response from MapBox
*
*/
//--------------------------------------------------------------------------------------------------
struct JsonString {
    char *ptr;
    size_t len;
};

//--------------------------------------------------------------------------------------------------
/**
* Initiates Data connection and cURLs coordinates to mapbox to get geocoding information
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

#endif // GEOCODING_GPSAPP_H_