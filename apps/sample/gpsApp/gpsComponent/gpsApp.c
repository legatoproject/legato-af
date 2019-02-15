
#include "gnss.h"
#include "geocoding.h"

//--------------------------------------------------------------------------------------------------
/**
* Constants for converting coordinates to distances (these are latitude specific and would change
* slightly depending on what latitude you are on, but the change is not significant and these numbers
* can be used all across the globe with reasonable error)
*
*/
//--------------------------------------------------------------------------------------------------
#define KM_PER_DEGREE_LAT    111.229
#define KM_PER_DEGREE_LON    71.696

//--------------------------------------------------------------------------------------------------
/**
* Sets the boundaries for bbox option of mapbox. Converts distance in KMs to coordinates.
*
*/
//--------------------------------------------------------------------------------------------------
static void SetBoundary
(
    CoordinateInformation_t* CoordinateInformationPtr,
    double km
)
{
    double degLat = 0;
    double degLon = 0;

    degLat = km / KM_PER_DEGREE_LAT;
    degLon = km / KM_PER_DEGREE_LON;

    CoordinateInformationPtr->minLat = CoordinateInformationPtr->currentLat - degLat;
    CoordinateInformationPtr->minLon = CoordinateInformationPtr->currentLon - degLon;
    CoordinateInformationPtr->maxLat = CoordinateInformationPtr->currentLat + degLat;
    CoordinateInformationPtr->maxLon = CoordinateInformationPtr->currentLon + degLon;
}

//--------------------------------------------------------------------------------------------------
/**
* Used for finding points of intereset
*
*/
//--------------------------------------------------------------------------------------------------
le_result_t ctrlGPS_FindPoi
(
    const char* argPtr,
    double km,
    double accuracy
)
{
    int locationAccuracy = 0;
    bool bbox = false;
    char* poiName;
    bool locateMe = false;
    if (km > 0)
    {
        bbox = true;
    }
    if (accuracy != 0)
    {
        locationAccuracy = (int)accuracy;
    }
    else
    {
        // If no accuracy is specified, set it to 20m by default.
        locationAccuracy = 20;
    }

    poiName = malloc(strlen(argPtr)+1);
    strcpy(poiName, argPtr);
    if (poiName == NULL)
    {
        LE_INFO("Search NULL");
        return LE_FAULT;
    }

    gnss_InitiateWatchGNSS(locationAccuracy, bbox, poiName, locateMe);
    if (bbox)
    {
        SetBoundary(CoordinateInformationPtr, km);
    }
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
* Used for getting the current physical address through reverse geocoding the current coordinates
*
*/
//--------------------------------------------------------------------------------------------------
le_result_t ctrlGPS_LocateMe
(
    double accuracy
)
{
    int locationAccuracy = 0;
    bool bbox = false;
    bool locateMe = true;
    if (accuracy != 0)
    {
        locationAccuracy = (int)accuracy;
    }
    else
    {
        // If no accuracy is specified, set it to 20m by default.
        locationAccuracy = 20;
    }
    gnss_InitiateWatchGNSS(locationAccuracy, bbox, "", locateMe);

return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
* Intentionally Empty
*
*/
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    //
}