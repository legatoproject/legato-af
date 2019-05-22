#include "geocoding.h"

//--------------------------------------------------------------------------------------------------
/**
* Data connection channel
*
*/
//--------------------------------------------------------------------------------------------------
static le_dcs_ChannelRef_t myChannel;
static char channelName[LE_DCS_CHANNEL_NAME_MAX_LEN];

//--------------------------------------------------------------------------------------------------
/**
* References
*
*/
//--------------------------------------------------------------------------------------------------
static le_json_ParsingSessionRef_t JsonParsingSessionRef;
static le_dcs_EventHandlerRef_t connectionHandlerRef = NULL;
static le_dcs_ReqObjRef_t reqObj;

//--------------------------------------------------------------------------------------------------
/**
* JSON file to parse
*
*/
//--------------------------------------------------------------------------------------------------
static int JsonFileDescriptor;
static char *Path = "MBquery";

//--------------------------------------------------------------------------------------------------
/**
* Status Flags
*
*/
//--------------------------------------------------------------------------------------------------
static bool LocateMe;
static bool BBoxSpecified;

//--------------------------------------------------------------------------------------------------
/**
* Name of POI to find
*
*/
//--------------------------------------------------------------------------------------------------
static char* SearchName;

//--------------------------------------------------------------------------------------------------
/**
* Function Prototypes
*
*/
//--------------------------------------------------------------------------------------------------
static le_result_t  StoreJson                 (const char *filepath, const char *data);
static le_result_t  GetUrl                    (void);
static size_t       WriteFunc                 (void *ptr, size_t size, size_t nmemb, struct JsonString *s);
static char*        FindReplace               (char* curlRequest, char* find, char* replace);
static void         ConnectionStateHandler    (le_dcs_ChannelRef_t channelRef, le_dcs_Event_t event, int32_t code, void *contextPtr);
static void         JsonEventHandler          (le_json_Event_t event);
static void         JsonErrorHandler          (le_json_Error_t error, const char* msg);
static void         CloseDelete               (int f);
void ctrlGPS_RemoveConnectionStateHandler        (le_dcs_EventHandlerRef_t addHandlerRef);

//--------------------------------------------------------------------------------------------------
/**
* Converts degrees to radians
*
*/
//--------------------------------------------------------------------------------------------------
static double Radians
(
    double x
)
{
    return x * PI / 180;
}

//--------------------------------------------------------------------------------------------------
/**
* Calculates distance between two coordinates using the Haversine formula
*
*/
//--------------------------------------------------------------------------------------------------
static double DistanceBetween
(
    double lon1,      ///< [IN] Longitude 1
    double lat1,      ///< [IN] Latitude 1
    double lon2,      ///< [IN] Longitude 2
    double lat2       ///< [IN] Latitude 2
)
{
    double dlon = Radians(lon2 - lon1);
    double dlat = Radians(lat2 - lat1);

    double a = (sin(dlat / 2) * sin(dlat / 2)) + cos(Radians(lat1)) * cos(Radians(lat2)) * (sin(dlon / 2) * sin(dlon / 2));
    double angle = 2 * atan2(sqrt(a), sqrt(1 - a));

    return angle * RADIUS;
}

//--------------------------------------------------------------------------------------------------
/**
* Initialize the string data structure for storing mapbox response
*
*/
//--------------------------------------------------------------------------------------------------
static void InitString
(
    struct JsonString *s      ///< [IN] String struct
)
{
    s->len = 0;
    s->ptr = malloc(s->len+1);
    if (s->ptr == NULL) {
        LE_ERROR("malloc() failed\n");
        exit(EXIT_FAILURE);
    }
    s->ptr[0] = '\0';
}

//--------------------------------------------------------------------------------------------------
/**
* Stores JSON string to a file
*
*/
//--------------------------------------------------------------------------------------------------
static le_result_t StoreJson
(
    const char *filepath,     ///< [IN] Path to store file
    const char *data          ///< [IN] String containing information in JSON format
)
{
    FILE *fp;
    fp = fopen(filepath,"w+");

    if ( fp )
    {
        fputs(data,fp);
        fclose(fp);
        return LE_OK;
    }
    else
    {
        LE_ERROR("Failed to open the file\n");
        return LE_FAULT;
    }
}

//--------------------------------------------------------------------------------------------------
/**
* Write function which gets passed to cURL for writing incoming data to string data structure
*
*/
//--------------------------------------------------------------------------------------------------
static size_t WriteFunc
(
    void *ptr,
    size_t size,
    size_t nmemb,
    struct JsonString *s
)
{
    size_t new_len = s->len + size*nmemb;
    s->ptr = realloc(s->ptr, new_len+1);
    if (s->ptr == NULL) {
        LE_ERROR("realloc() failed\n");
        exit(EXIT_FAILURE);
    }
    memcpy(s->ptr+s->len, ptr, size*nmemb);
    s->ptr[new_len] = '\0';
    s->len = new_len;

    return size*nmemb;
}

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
)
{
    LocateMe = locate;
    BBoxSpecified = bbox;
    SearchName = poi;
    le_result_t res;
    le_dcs_ChannelRef_t ret_ref;
    le_dcs_ChannelInfo_t channelList[LE_DCS_CHANNEL_LIST_ENTRY_MAX];
    size_t listLen = 10;

    // Get list of all available data channels
    res = le_dcs_GetList(channelList, &listLen);
    if (LE_OK != res)
    {
        LE_ERROR("Failed to get channel list!");
        ctrlGPS_CleanUp(false);
    }
    if (listLen)
    {
        strncpy(channelName, channelList[0].name, LE_DCS_CHANNEL_NAME_MAX_LEN);
        myChannel = channelList[0].ref;
    }

    // Get reference to channel
    LE_INFO("asking for channel reference for channel %s", channelName);
    ret_ref = le_dcs_GetReference(channelName, LE_DCS_TECH_CELLULAR);
    LE_INFO("returned channel reference: %p", ret_ref);

    // Add connection handler
    LE_INFO("asking to add event handler for channel %s", channelName);
    connectionHandlerRef = le_dcs_AddEventHandler(myChannel, ConnectionStateHandler, NULL);
    LE_INFO("channel event handler added %p for channel %s", ConnectionStateHandler,
            channelName);

    // Start connection
    LE_INFO("asking to start channel %s", channelName);
    reqObj = le_dcs_Start(myChannel);
    LE_INFO("returned RequestObj %p", reqObj);
    sleep(5);

    // Setting default gateway
    LE_INFO("asking to add route for channel %s", channelName);
    le_net_BackupDefaultGW();
    le_net_SetDefaultGW(myChannel);
    le_net_SetDNS(myChannel);
    le_net_ChangeRoute(myChannel, "1.1.1.1", "", true);
}

//--------------------------------------------------------------------------------------------------
/**
* Handler function for data connection
*
*/
//--------------------------------------------------------------------------------------------------
static void ConnectionStateHandler
(
    le_dcs_ChannelRef_t channelRef, ///< [IN] The channel for which the event is
    le_dcs_Event_t event,           ///< [IN] Event up or down
    int32_t code,                   ///< [IN] Additional event code, like error code
    void *contextPtr                ///< [IN] Associated user context pointer
)
{
    char *eventString;
    switch (event)
    {
        case LE_DCS_EVENT_UP:
            LE_INFO("CONNECTED!");
            eventString = "Up";
            break;
        case LE_DCS_EVENT_DOWN:
            LE_INFO("DISCONNECTED!");
            eventString = "Down";
            break;
        case LE_DCS_EVENT_TEMP_DOWN:
            LE_INFO("TEMPORARILY DISCONNECTED!");
            eventString = "Temporary Down";
            break;
        default:
            eventString = "Unknown";
            break;
    }
    LE_INFO("received for channel reference %p event %s", channelRef, eventString);

    le_result_t res = LE_OK;
    if (event == LE_DCS_EVENT_UP)
    {
        sleep(5);
        res = GetUrl();
        if (res != LE_OK)
        {
            ctrlGPS_CleanUp(false);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
* Replace a word within a string with another word
*
*/
//--------------------------------------------------------------------------------------------------
static char* FindReplace
(
    char* curlRequest,      ///< [IN] The cURL request in which we want to insert specific info
    char* find,             ///< [IN] Predefined word to find
    char* replace           ///< [IN] The specific word we want to insert into the cURL request
)
{
    char *dest = malloc(strlen(curlRequest)-strlen(find)+strlen(replace)+1);
    char *destptr = dest;
    *dest = 0;

    while (*curlRequest)
    {
        if (!strncmp(curlRequest, find, strlen(find)))
        {
            strcat(destptr, replace);
            curlRequest += strlen(find);
            destptr += strlen(replace);
        }
        else
        {
            *destptr = *curlRequest;
            destptr++;
            curlRequest++;
        }
  }
  *destptr = 0;
  return dest;
}

//--------------------------------------------------------------------------------------------------
/**
* Send request to Mapbox using cURL and store response to file in "Path"
*
*/
//--------------------------------------------------------------------------------------------------
static le_result_t GetUrl
(
    void
)
{
    CURL *curl;
    CURLcode res;

    struct JsonString s;
    InitString(&s);
    char* completeRequest;
    char* poiName = SearchName;
    // If locate me option is specified on the command line, use the reverse geocoding Mapbox query
    if (LocateMe)
    {
        char incompleteRequest[256];
        char myLat[10];
        char myLon[10];

        if (strcmp(MAPBOX_ACCESS_TOKEN, "") == 0)
        {
            LE_ERROR("You have not entered a mapbox access toke. Please enter it in the geocoding.h file and compile and run the application again.");
            ctrlGPS_CleanUp(false);
        }

        strcpy(incompleteRequest, MAPBOX_REVERSE_GEOCODE_REQUEST);
        strcat(incompleteRequest, MAPBOX_ACCESS_TOKEN);

        gcvt((double)CoordinateInformationPtr->currentLon, 10, myLon);
        gcvt((double)CoordinateInformationPtr->currentLat, 10, myLat);

        char* partialRequest = FindReplace(incompleteRequest, "LONGITUDE", myLon);
        completeRequest = malloc(strlen(partialRequest)-(strlen("LATITUDE")+1)+(strlen(myLat)+1));
        strcpy(completeRequest, FindReplace(partialRequest, "LATITUDE", myLat));

    }
    // If no boundary is specified in command line, use proximity request mapbox query
    else if (!BBoxSpecified)
    {
        char incompleteRequest[256];
        char myLat[12];
        char myLon[12];
        char currentCoordinates[24];

        // All strcpy and strcat functions are to be replaced with le_utf8_Copy and le_utf8_Append in future releases
        strcpy(incompleteRequest, MAPBOX_POI_PROXIMITY_REQUEST);
        strcat(incompleteRequest, MAPBOX_ACCESS_TOKEN);

        gcvt((double)CoordinateInformationPtr->currentLon, 7, myLon);
        gcvt((double)CoordinateInformationPtr->currentLat, 7, myLat);

        strcpy(currentCoordinates, myLon);
        strcat(currentCoordinates, ",");
        strcat(currentCoordinates, myLat);

        char* partialRequest = FindReplace(incompleteRequest, "POINAME", poiName);
        completeRequest = malloc(strlen(partialRequest)-(strlen("COORDINATES")+1)+(strlen(currentCoordinates)+1));
        strcpy(completeRequest, FindReplace(partialRequest, "COORDINATES", currentCoordinates));
        free(poiName);
    }
    // This is for when a boundary is specified on the command line and we use the bbox option in the mapbox query
    else
    {
        char incompleteRequest[256];
        char minimumLon[12];
        char maximumLon[12];
        char minimumLat[12];
        char maximumLat[12];
        char coordinatesBoundary[48];

        strcpy(incompleteRequest, MAPBOX_POI_BBOX_REQUEST);
        strcat(incompleteRequest, MAPBOX_ACCESS_TOKEN);

        gcvt((double)CoordinateInformationPtr->minLon, 7, minimumLon);
        gcvt((double)CoordinateInformationPtr->minLat, 7, minimumLat);
        gcvt((double)CoordinateInformationPtr->maxLon, 7, maximumLon);
        gcvt((double)CoordinateInformationPtr->maxLat, 7, maximumLat);

        strcpy(coordinatesBoundary, minimumLon);
        strcat(coordinatesBoundary, ",");
        strcat(coordinatesBoundary, minimumLat);
        strcat(coordinatesBoundary, ",");
        strcat(coordinatesBoundary, maximumLon);
        strcat(coordinatesBoundary, ",");
        strcat(coordinatesBoundary, maximumLat);

        char* partialRequest = FindReplace(incompleteRequest, "POINAME", poiName);
        completeRequest = malloc(strlen(partialRequest)-(strlen("COORDINATES")+1)+(strlen(coordinatesBoundary)+1));
        strcpy(completeRequest, FindReplace(partialRequest, "COORDINATES", coordinatesBoundary));


        LE_INFO("complete request %s", completeRequest);


        free(poiName);
    }
    curl = curl_easy_init();
    // Send the request to cURL
    if(curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, completeRequest);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteFunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);

        res = curl_easy_perform(curl);
        if(res != CURLE_OK)
        {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            return LE_FAULT;
        }
        // Store the string containing the entire response to Path
        StoreJson(Path, s.ptr);
        JsonFileDescriptor = open(Path, O_RDONLY);
        // Send the JSON file to the JSON parser
        JsonParsingSessionRef = le_json_Parse(JsonFileDescriptor, JsonEventHandler, JsonErrorHandler, NULL);

        free(s.ptr);
        LE_DEBUG("request %s", completeRequest);
        free(completeRequest);
    }

    curl_global_cleanup();
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
* Update acquired address in the SearchResultUpdated struct
*
*/
//--------------------------------------------------------------------------------------------------
static void MakeAddressReadyForReport
(
    const char* address       ///< [IN] address acquired from POI search
)
{
    SearchResultUpdated.result = malloc(strlen(address)+1);
    strcpy(SearchResultUpdated.result, address);
}

//--------------------------------------------------------------------------------------------------
/**
* Report the updated results via the DoneEvent since the results are now ready and search is done.
*
*/
//--------------------------------------------------------------------------------------------------
static void ReportPoiInfo
(
    bool JsonParseComplete    ///< [IN] Flag indicating JSON parsing is complete
)
{
    SearchResultUpdated.searchDone = JsonParseComplete;
    le_event_Report(DoneEvent, &SearchResultUpdated, sizeof(SearchResultUpdated));
}

//--------------------------------------------------------------------------------------------------
/**
* Handler function for JSON parsing events
*
*/
//--------------------------------------------------------------------------------------------------
static void JsonEventHandler
(
    le_json_Event_t event    ///< [IN] JSON event.
)
{
    static double poiCoordinates[2];
    static int    coordinatesAcquired;
    static int    resCounter;
    static bool   coordinatesReady;
    static bool   getCoordinates;
    static bool   getAddress;
    static bool   JsonParseComplete;

    switch (event)
    {
        case LE_JSON_OBJECT_START:
            break;

        case LE_JSON_OBJECT_END:
            break;

        case LE_JSON_DOC_END:
            if (resCounter == 0)
            {
                LE_ERROR("No results found. Try broadening your search boundary.");
                JsonParseComplete = false;
                SearchResultUpdated.error = true;
                ReportPoiInfo(JsonParseComplete);
            }
            else
            {
                LE_DEBUG("Parsing complete!");
                JsonParseComplete = true;
                ReportPoiInfo(JsonParseComplete);
            }
            break;

        case LE_JSON_OBJECT_MEMBER:
            ;
            const char* memberName = le_json_GetString();
            if (strcmp(memberName, "place_name") == 0)
            {
                resCounter++;
                getAddress = true;
            }
            else if (strcmp(memberName, "center") == 0)
            {
                getCoordinates = true;
            }
            break;

        case LE_JSON_STRING:
            if (getAddress)
            {
            // Get the address of each result from the JSON file and print it
                getAddress = false;
                const char* address = le_json_GetString();
                fprintf(stdout, "Result %d: %s\n", resCounter, address);
                MakeAddressReadyForReport(address);
                // If locate me option was specified, we are done after printing 1 result (which is the current address)
                if (LocateMe)
                {
                    JsonParseComplete = true;
                    ReportPoiInfo(JsonParseComplete);
                }
            }
            break;

          case LE_JSON_NUMBER:
            // If both coordinates are acquired for a result, calculate distance and print it
            if (coordinatesAcquired == 2)
            {
                double distance = DistanceBetween(CoordinateInformationPtr->currentLon, CoordinateInformationPtr->currentLat, poiCoordinates[0], poiCoordinates[1]);
                SearchResultUpdated.distance = distance;
                ReportPoiInfo(JsonParseComplete);

                fprintf(stdout, "Distance: %.1f km\n", distance);
                coordinatesAcquired = 0;
                coordinatesReady = false;
            }
            if (coordinatesReady)
            {
            // Store coordinates in the poiCoordinates array and print them
                poiCoordinates[coordinatesAcquired] = le_json_GetNumber();
                fprintf(stdout, "Coordinates: %f\n", le_json_GetNumber());
                coordinatesAcquired++;
            }
            break;

        case LE_JSON_ARRAY_START:
            if (getCoordinates)
            {
                coordinatesReady = true;
                getCoordinates = false;
            }
            break;

        case LE_JSON_ARRAY_END:
        case LE_JSON_TRUE:
        case LE_JSON_FALSE:
        case LE_JSON_NULL:
            break;
        default:
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
* Error handler function for JSON parser
*
*/
//--------------------------------------------------------------------------------------------------
static void JsonErrorHandler
(
    le_json_Error_t error,    ///< [IN] Error enum from JSON.
    const char* msg           ///< [IN] Error message in human readable format.
)
{
    switch (error)
    {
        case LE_JSON_SYNTAX_ERROR:
            LE_ERROR("JSON error message: %s", msg);
            ctrlGPS_CleanUp(false);
            break;

        case LE_JSON_READ_ERROR:
            LE_ERROR("JSON error message: %s", msg);
            ctrlGPS_CleanUp(false);
            break;

        default:
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
* Release data connection, cleanup JSON parser, delete JSON file and exit
* EXIT_SUCCESS if parsing was complete
* EXIT_FAILURE if parsing was not complete
*
*/
//--------------------------------------------------------------------------------------------------
le_result_t ctrlGPS_CleanUp
(
    bool searchDone   ///< [IN] Flag indicating search is done
)
{
    // Stop JSON parsing and close the JSON file descriptor.
    if (JsonParsingSessionRef)
    {
        le_json_Cleanup(JsonParsingSessionRef);
    }

    // Close JSON file.
    if (JsonFileDescriptor)
    {
        CloseDelete(JsonFileDescriptor);
    }

    // Cleanup data connection
    if (reqObj)
    {
        LE_INFO("asking to remove route for channel %s", channelName);
        le_net_RestoreDefaultGW();
        le_net_RestoreDNS();
        le_net_ChangeRoute(myChannel, "1.1.1.1", "", false);
        le_dcs_Stop(myChannel, reqObj);
    }

    if (connectionHandlerRef)
    {
        ctrlGPS_RemoveConnectionStateHandler(connectionHandlerRef);
    }

    if(searchDone)
    {
        exit(EXIT_SUCCESS);
    }
    else
    {
        exit(EXIT_FAILURE);
    }
}

//--------------------------------------------------------------------------------------------------
/**
* Close JSON file and delete it
*
*/
//--------------------------------------------------------------------------------------------------
static void CloseDelete
(
    int f    ///< [IN] File descriptor.
)
{
    if (-1 == f)
    {
        LE_ERROR("File descriptor is not valid!");
        return;
    }
    if (-1 == close(f))
    {
        LE_WARN("failed to close file %d: %m", f);
    }
    if (remove(Path) != 0)
    {
        LE_WARN("Failed to delete file.");
    }
}

//--------------------------------------------------------------------------------------------------
/**
* First layer handler function in which the search results are stored in the results struct
*
*/
//--------------------------------------------------------------------------------------------------
static void FirstLayerStateHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    ctrlGPS_ResultInfo_t* results;
    SearchDone_t* tempValuePtr = reportPtr;
    ctrlGPS_UpdatedValueHandlerFunc_t clientHandlerFunc = secondLayerHandlerFunc;

    results = malloc(sizeof(ctrlGPS_ResultInfo_t));

    results->searchDone = tempValuePtr->searchDone;
    results->error = tempValuePtr->error;

    if (tempValuePtr->result)
    {
        strcpy(results->result, tempValuePtr->result);
    }

    if (tempValuePtr->distance)
    {
        results->distance = tempValuePtr->distance;
    }

    // Call the client handler
    clientHandlerFunc(results, le_event_GetContextPtr());
}

//--------------------------------------------------------------------------------------------------
/**
* Function to create the latered handler and send search results to controller for printing.
*
*/
//--------------------------------------------------------------------------------------------------
ctrlGPS_UpdatedValueHandlerRef_t ctrlGPS_AddUpdatedValueHandler
(
    ctrlGPS_UpdatedValueHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    DoneEvent = le_event_CreateId(EVENT_NAME, sizeof(SearchDone_t));
    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler(EVENT_NAME, DoneEvent,
        FirstLayerStateHandler, (le_event_HandlerFunc_t) handlerPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (ctrlGPS_UpdatedValueHandlerRef_t) (handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
* Remove updated search result handler
*
*/
//--------------------------------------------------------------------------------------------------
void ctrlGPS_RemoveUpdatedValueHandler
(
    ctrlGPS_UpdatedValueHandlerRef_t addHandlerRef
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t) addHandlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
* Remove connection state handler
*
*/
//--------------------------------------------------------------------------------------------------
void ctrlGPS_RemoveConnectionStateHandler
(
    le_dcs_EventHandlerRef_t addHandlerRef
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t) addHandlerRef);
}

