/** @file gnss_at.c
 *
 * Legato @ref c_gnss_at include file.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#include "legato.h"

#include "pa_gnss.h"

#include "atMgr.h"
#include "atCmdSync.h"
#include "atPorts.h"

#define DEFAULT_POSITIONDATA_POOL_SIZE  1

#define     GNSS_CONVERT_KNOTS_MS   0.514444

#define         NMEA_SIZE    82   // 82 is the nmea max length.

static atmgr_Ref_t NmeaPortRef=NULL;

typedef enum {
    PA_NO_FIX,
    PA_2D_FIX,
    PA_3D_FIX
}
pa_Gnss_FixType_t;

static le_mutex_Ref_t       GnssMutex=NULL;

static le_event_Id_t        GnssEventId;
static le_event_Id_t        GnssEventUnsolId;
static le_event_Id_t        GnssEventFSMId;
static pa_Gnss_FixType_t    CurrentFixType=PA_NO_FIX;

static le_thread_Ref_t      GnssThreadRef=NULL;

static pa_Gnss_Position_t   LastPosition;

static char  NmeaGga[NMEA_SIZE+1]={0};
static char  NmeaRmc[NMEA_SIZE+1]={0};
static char  NmeaGsa[NMEA_SIZE+1]={0};
static char  NmeaVtg[NMEA_SIZE+1]={0};
static char  NmeaSwi[NMEA_SIZE+1]={0};

//--------------------------------------------------------------------------------------------------
/**
 * Memory Pool for GNSS position handlers.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t   GnssPosPoolRef;

//--------------------------------------------------------------------------------------------------
/**
 * This function Initialize the gnss layer.
 *
 * @return LE_NOT_POSSIBLE The function failed to Initialize the platform adapter layer.
 * @return LE_COMM_ERROR   The communication device has returned an error.
 * @return LE_TIMEOUT      No response was received from the Modem.
 * @return LE_OK           The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GnssInit
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * This function is called when debug
 *
 */
//--------------------------------------------------------------------------------------------------
static void PrintNmea
(
    const char* txtPtr,
    uint32_t    size
)
{
    int i;
    for (i=0;i<size;i++) {
        LE_DEBUG("L%d: >%s<",
                 i+1,
                 atcmd_GetLineParameter(txtPtr,i+1));
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to count the number of paramter between ',' and replace them by '\0'
 *
 * @todo: check the CRC...
 */
//--------------------------------------------------------------------------------------------------
static bool NmeaLineParsParam
(
    char      *linePtr,       ///< [IN/OUT] line to parse
    uint32_t  *numParamPtr   ///< [OUT] Number of parameter retreived
)
{
    uint32_t  cpt=1;
    uint32_t  LineSize = strlen(linePtr);

    while (LineSize)
    {
        if ( linePtr[LineSize] == ',' ) {
            linePtr[LineSize] = '\0';
            cpt++;
        } else if ( linePtr[LineSize] == '*' ) {
            linePtr[LineSize] = '\0';
            cpt++;
        }

        LineSize--;
    }

    *numParamPtr = cpt;

    return true;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to run the gnss thread
 *
 */
//--------------------------------------------------------------------------------------------------
static void* GnssThread
(
    void* contextPtr
)
{
    le_sem_Ref_t semPtr = contextPtr;
    LE_INFO("Start GNSS");

    GnssInit();

    le_sem_Post(semPtr);
    le_event_RunLoop();
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function parse time into structure
 *
 */
//--------------------------------------------------------------------------------------------------
static void ParseTime
(
    const char*         timeStringPtr,
    pa_Gnss_Time_t*     timePtr
)
{
    uint32_t timeSize = strlen(timeStringPtr);
    char tmp[4]={0};

    if (timeSize>1) {
        tmp[0]= timeStringPtr[0];
        tmp[1]= timeStringPtr[1];
        tmp[2]='\0';
        timePtr->hours = (uint16_t) atoi(tmp);
    } else {
        timePtr->hours = 0;
    }

    if (timeSize>3) {
        tmp[0]= timeStringPtr[2];
        tmp[1]= timeStringPtr[3];
        tmp[2]='\0';
        timePtr->minutes = (uint16_t) atoi(tmp);
    } else {
        timePtr->minutes = 0;
    }

    if (timeSize>5) {
        tmp[0]= timeStringPtr[4];
        tmp[1]= timeStringPtr[5];
        tmp[2]='\0';
        timePtr->seconds = (uint16_t) atoi(tmp);
    } else {
        timePtr->seconds = 0;
    }

    if (timeSize>6) {
        strncpy(tmp,&timeStringPtr[7],4);
        timePtr->milliseconds = (uint16_t) atoi(tmp);
    } else {
        timePtr->milliseconds = 0;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function parse date into structure
 *
 */
//--------------------------------------------------------------------------------------------------
static void ParseDate
(
    const char     *dateStringPtr,
    pa_Gnss_Date_t *datePtr
)
{
    uint32_t timeSize = strlen(dateStringPtr);
    char tmp[4]={0};

    if (timeSize>1) {
        tmp[0]= dateStringPtr[0];
        tmp[1]= dateStringPtr[1];
        tmp[2]='\0';
        datePtr->day = (uint16_t) atoi(tmp);
    } else {
        datePtr->day = 0;
    }

    if (timeSize>3) {
        tmp[0]= dateStringPtr[2];
        tmp[1]= dateStringPtr[3];
        tmp[2]='\0';
        datePtr->month = (uint16_t) atoi(tmp);
    } else {
        datePtr->month = 0;
    }

    if (timeSize>5) {
        tmp[0]= dateStringPtr[4];
        tmp[1]= dateStringPtr[5];
        tmp[2]='\0';
        datePtr->year = 2000 + (uint16_t) atoi(tmp);
    } else {
        datePtr->year = 0;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function parse latitude and direction into structure
 *
 */
//--------------------------------------------------------------------------------------------------
static void ParseLatitude
(
    const char*            latitudePtr,
    const char*            directionPtr,
    pa_Gnss_Position_Ref_t posRef
)
{
    posRef->latitude = (int32_t) (10000*atof(latitudePtr));

    if ( strcmp (directionPtr,"S" )==0 )
    {
        posRef->latitude = -posRef->latitude;
    }

    posRef->latitudeValid = true;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function parse longitude and direction into structure
 *
 */
//--------------------------------------------------------------------------------------------------
static void ParseLongitude
(
    const char*            longitudePtr,
    const char*            directionPtr,
    pa_Gnss_Position_Ref_t posRef
)
{
    posRef->longitude =  (int32_t) (10000*atof(longitudePtr));

    if ( strcmp (directionPtr,"W" )==0 )
    {
        posRef->longitude = -posRef->longitude;
    }

    posRef->longitudeValid = true;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function get dop
 *
 */
//--------------------------------------------------------------------------------------------------
static uint16_t ParseDop
(
    const char* dopPtr
)
{
    return (uint16_t) (10*atof(dopPtr));
}

//--------------------------------------------------------------------------------------------------
/**
 * This function parse altitude into structure
 *
 */
//--------------------------------------------------------------------------------------------------
static void ParseAltitude
(
    const char*            altitudePtr,
    pa_Gnss_Position_Ref_t posRef
)
{
    posRef->altitude = (int32_t) (100*10*atof(altitudePtr));

    posRef->altitudeValid = true;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function parse horizontal speed into structure
 *
 */
//--------------------------------------------------------------------------------------------------
static void ParseSpeed
(
    const char*             speedPtr,
    pa_Gnss_Position_Ref_t  posRef
)
{
    posRef->hSpeed = (uint32_t) (atof(speedPtr)*100);

    posRef->hSpeedValid = true;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function parse track into structure
 *
 */
//--------------------------------------------------------------------------------------------------
static void ParseTrack
(
    const char*             trackPtr,
    pa_Gnss_Position_Ref_t  posRef
)
{
    posRef->track = (uint32_t) (atof(trackPtr)*10);

    posRef->trackValid = true;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function parse track into structure
 *
 */
//--------------------------------------------------------------------------------------------------
static void ParseHeading
(
    const char*             headingPtr,
    pa_Gnss_Position_Ref_t  posRef
)
{
    posRef->heading = (uint32_t) (atof(headingPtr)*10);

    posRef->headingValid = true;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function parse horizontal uncertainty into structure
 *
 */
//--------------------------------------------------------------------------------------------------
static void ParseHorizontalUncertainty
(
    const char*             huncertaintyStringPtr,
    const char*             huncertaintyValidityPtr,
    pa_Gnss_Position_Ref_t  posRef
)
{
    posRef->hUncertainty = (uint32_t) (10*atof(huncertaintyStringPtr));

    switch (atoi(huncertaintyValidityPtr))
    {
        case 3:
        case 4:
        case 5:
        case 6:
        {
            posRef->hUncertaintyValid = true;
            break;
        }
        default:
        {
            posRef->hUncertaintyValid = false;
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function parse vertical uncertainty into structure
 *
 */
//--------------------------------------------------------------------------------------------------
static void ParseVerticalUncertainty
(
    const char*             vuncertaintyStringPtr,
    const char*             vuncertaintyValidityPtr,
    pa_Gnss_Position_Ref_t  posRef
)
{
    posRef->vUncertainty = (uint32_t) (10*atof(vuncertaintyStringPtr));

    switch (atoi(vuncertaintyValidityPtr))
    {
        case 4:
        case 6:
        {
            posRef->vUncertaintyValid = true;
            break;
        }
        case 3:
        case 5:
        default:
        {
            posRef->vUncertaintyValid = false;
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * This function parse GGA Frame into position
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ConvertGga
(
    char*                  linePtr,
    pa_Gnss_Position_Ref_t posRef
)
{
    uint32_t numParam=0;

    LE_DEBUG("Convert gga %s",linePtr);

    if (NmeaLineParsParam(linePtr,&numParam)==true)
    {
        PrintNmea(linePtr,numParam);

        if ( numParam >= 4 ) {
            ParseLatitude(atcmd_GetLineParameter(linePtr,3),
                          atcmd_GetLineParameter(linePtr,4),
                          posRef);
        }

        if ( numParam >= 6 ) {
            ParseLongitude(atcmd_GetLineParameter(linePtr,5),
                           atcmd_GetLineParameter(linePtr,6),
                           posRef);
        }

        if ( numParam >= 10 ) {
            ParseAltitude(atcmd_GetLineParameter(linePtr,10),
                          posRef);
        }
    }
    else
    {
        LE_DEBUG("This pattern is not expected");
        return LE_NOT_POSSIBLE;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function parse RMC Frame into position
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ConvertRmc
(
    char*                   linePtr,
    pa_Gnss_Position_Ref_t  posRef
)
{
    uint32_t numParam=0;

    LE_DEBUG("Convert rmc %s",linePtr);

    if (NmeaLineParsParam(linePtr,&numParam)==true)
    {
        PrintNmea(linePtr,numParam);

        if ( numParam >= 2) {
            ParseTime(atcmd_GetLineParameter(linePtr,2),&(posRef->time));
            posRef->timeValid = true;
        }

        if ( numParam >= 10 ) {
            ParseDate(atcmd_GetLineParameter(linePtr,10),&(posRef->date));
            posRef->dateValid = true;
        }
    }
    else
        {
        LE_DEBUG("This pattern is not expected");
            return LE_NOT_POSSIBLE;
        }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function parse GSA Frame into position
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ConvertGsa
(
    char*                   linePtr,
    pa_Gnss_Position_Ref_t  posRef
)
{
    uint32_t numParam=0;

    LE_DEBUG("Convert gsa %s",linePtr);

    if (NmeaLineParsParam(linePtr,&numParam)==true)
    {
        PrintNmea(linePtr,numParam);

        if ( numParam >= 17 ) {
            posRef->hdop = ParseDop(atcmd_GetLineParameter(linePtr,17));
            if (posRef->hdop!=0)
            {
                posRef->hdopValid = true;
        }
        }

        if ( numParam >= 18 ) {
            posRef->vdop = ParseDop(atcmd_GetLineParameter(linePtr,18));
            if (posRef->vdop!=0)
        {
                posRef->vdopValid = true;
        }
        }

        return LE_OK;
    }
    else
        {
        LE_DEBUG("This pattern is not expected");
            return LE_NOT_POSSIBLE;
        }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function parse VTG Frame into position
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ConvertVtg
(
    char*                   linePtr,
    pa_Gnss_Position_Ref_t  posRef
)
{
    uint32_t numParam=0;

    LE_DEBUG("Convert vtg %s",linePtr);

    if (NmeaLineParsParam(linePtr,&numParam)==true)
    {
        PrintNmea(linePtr,numParam);

        if ( numParam >= 2 ) {
            ParseTrack(atcmd_GetLineParameter(linePtr,2),posRef);
        }

        if ( numParam >= 4 ) {
            ParseHeading(atcmd_GetLineParameter(linePtr,4),posRef);
        }

        if ( numParam >= 8 ) {
            ParseSpeed(atcmd_GetLineParameter(linePtr,8),posRef);
        }

        return LE_OK;
    }
    else
    {
        LE_DEBUG("This pattern is not expected");
        return LE_NOT_POSSIBLE;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function parse SWI Frame into position
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ConvertSwi
(
    char*                   linePtr,
    pa_Gnss_Position_Ref_t  posRef
)
{
    uint32_t numParam=0;

    LE_DEBUG("Convert swi %s",linePtr);

    if (NmeaLineParsParam(linePtr,&numParam)==true)
    {
        PrintNmea(linePtr,numParam);

        if ( numParam >= 6 ) {
            ParseHorizontalUncertainty(atcmd_GetLineParameter(linePtr,6),
                                       atcmd_GetLineParameter(linePtr,4),
                                       posRef);
        }

        if ( numParam >= 7 ) {
            ParseVerticalUncertainty(atcmd_GetLineParameter(linePtr,7),
                                     atcmd_GetLineParameter(linePtr,4),
                                     posRef);
        }
        return LE_OK;
    }
    else
    {
        LE_DEBUG("This pattern is not expected");
        return LE_NOT_POSSIBLE;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function convert NMEA Frame into position
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ConvertPosition
(
    pa_Gnss_Position_Ref_t posRef
)
{
    if (posRef==NULL)
    {
        return LE_FAULT;
    }

    memset(posRef,0,sizeof(*posRef));

    if  (
        (ConvertGga(NmeaGga,posRef)!=LE_OK)
            ||
        (ConvertRmc(NmeaRmc,posRef)!=LE_OK)
            ||
        (ConvertGsa(NmeaGsa,posRef)!=LE_OK)
            ||
        (ConvertVtg(NmeaVtg,posRef)!=LE_OK)
            ||
        (ConvertSwi(NmeaSwi,posRef)!=LE_OK)
        )
    {
        LE_DEBUG("Cannot convert position");
        return LE_NOT_POSSIBLE;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function is the GNSS NMEA handler
 *
 */
//--------------------------------------------------------------------------------------------------
static void GNSSUnsolHandler
(
    void* reportPtr
)
{
    atmgr_UnsolResponse_t* unsolPtr = reportPtr;
    static bool GgaBool=false;
    static bool GsaBool=false;
    static bool RmcBool=false;
    static bool VtgBool=false;
    static bool SwiBool=false;

    LE_DEBUG("GNSS UNSOL HANDLER CALLED gga(%d) gsa(%d) rmc(%d) vtg(%d) swi(%d)",
             GgaBool,GsaBool,RmcBool,VtgBool,SwiBool);

    if ( CurrentFixType == PA_NO_FIX )
    {
        LE_DEBUG("No fix relevant, do not need to parse");
        return;
    }

    le_mutex_Lock(GnssMutex);

    char* pStrNmea = unsolPtr->line;
    LE_DEBUG("GNSS STR %s",pStrNmea);

    if      ( strncmp(pStrNmea,"$GPGGA",strlen("$GPGGA")) == 0 )
    {
        strncpy(NmeaGga,pStrNmea,NMEA_SIZE);
        GgaBool=true;
    }
    else if ( strncmp(pStrNmea,"$GPGSA",strlen("$GPGSA")) == 0 )
    {
        strncpy(NmeaGsa,pStrNmea,NMEA_SIZE);
        GsaBool=true;
    }
    else if ( strncmp(pStrNmea,"$GPRMC",strlen("$GPRMC")) == 0 )
    {
        strncpy(NmeaRmc,pStrNmea,NMEA_SIZE);
        RmcBool=true;
    }
    else if ( strncmp(pStrNmea,"$GPVTG",strlen("$GPVTG")) == 0 )
    {
        strncpy(NmeaVtg,pStrNmea,NMEA_SIZE);
        VtgBool=true;
    }
    else if ( strncmp(pStrNmea,"$PSWI",strlen("$PSWI")) == 0 )
    {
        strncpy(NmeaSwi,pStrNmea,NMEA_SIZE);
        SwiBool=true;
    }

    LE_DEBUG("GNSS COPY gga(%d) gsa(%d) rmc(%d) vtg(%d) swi(%d)",
             GgaBool,GsaBool,RmcBool,VtgBool,SwiBool);
    if (GgaBool && GsaBool && RmcBool && VtgBool && SwiBool)
    {
        GgaBool=GsaBool=RmcBool=VtgBool=SwiBool=false;
        LE_DEBUG("GNSS Start position convertion ");
        if ( ConvertPosition(&LastPosition) != LE_OK)
        {
            LE_WARN("cannot convert position data");
        }
        else
        {
            pa_Gnss_Position_Ref_t lastPositionPtr = le_mem_ForceAlloc(GnssPosPoolRef);
            memcpy(lastPositionPtr, &LastPosition,sizeof(LastPosition));
            le_event_ReportWithRefCounting(GnssEventId, lastPositionPtr);
        }
    }

    le_mutex_Unlock(GnssMutex);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is the GNSS FSM handler
 *
 */
//--------------------------------------------------------------------------------------------------
static void GNSSFsmHandler
(
    void* reportPtr
)
{
    atmgr_UnsolResponse_t* unsolPtr = reportPtr;

    char* linePtr = unsolPtr->line;
    uint32_t numParam = atcmd_CountLineParameter(linePtr);
    if (numParam>=2)
    {
        switch (atoi(atcmd_GetLineParameter(linePtr,2)))
        {
            case 2:
                CurrentFixType = PA_2D_FIX;
                break;
            case 3:
                CurrentFixType = PA_3D_FIX;
                break;
            default:
                CurrentFixType = PA_NO_FIX;
        }
        LE_DEBUG("New Fix Type %d",CurrentFixType);
    }
    else
    {
        LE_DEBUG("This pattern is not expected");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function set event that will make live the FSM
 *
 */
//--------------------------------------------------------------------------------------------------
static void SetFsmEvent
(
    void
)
{
    atmgr_SubscribeUnsolReq(atports_GetInterface(ATPORT_COMMAND),
                            GnssEventFSMId,
                            "+GPSEVPOS:",
                            false);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function configure the NMEA Port
 *
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SetNmeaPort
(
    void
)
{
    NmeaPortRef = atports_GetInterface(ATPORT_COMMAND);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function configure NMEA frame that will be parsed
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SetNmeaFrame
(
    void
)
{
    atmgr_SubscribeUnsolReq(NmeaPortRef,
                                   GnssEventUnsolId,
                                   "$GPGGA",
                                   false);
    atmgr_SubscribeUnsolReq(NmeaPortRef,
                                   GnssEventUnsolId,
                                   "$GPGSA",
                                   false);
    atmgr_SubscribeUnsolReq(NmeaPortRef,
                                   GnssEventUnsolId,
                                   "$GPRMC",
                                   false);
    atmgr_SubscribeUnsolReq(NmeaPortRef,
                                   GnssEventUnsolId,
                                   "$GPVTG",
                                   false);
    atmgr_SubscribeUnsolReq(NmeaPortRef,
                                   GnssEventUnsolId,
                                   "$PSWI",
                                   false);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function start the gnss module with AT command
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ExecGnssInit
(
    void
)
{
    le_result_t  result=LE_NOT_POSSIBLE;
    atcmd_Ref_t atReqRef=NULL;
    atcmdsync_ResultRef_t  respPtr = NULL;
    const char* interRespPtr[] = { "OK" , NULL };
    const char* finalOkPtr[] = {"+GPSEVINIT:",NULL};
    const char* finalKoPtr[] = { "ERROR","+GPS ERROR:","+CME ERROR:","+CMS ERROR","TIMEOUT",NULL};

    atReqRef = atcmdsync_PrepareStandardCommand("AT+GPSINIT=30",
                                                    interRespPtr,
                                                    finalOkPtr,
                                                    finalKoPtr,
                                                    30000);
    respPtr = atcmdsync_SendCommand(atports_GetInterface(ATPORT_COMMAND),atReqRef);

    // check OK on first line and check +GPSEVINIT: 1 on second line
    if  (   (strcmp("OK",atcmdsync_GetLine(respPtr,0)) == 0)
            &&
            (strcmp("+GPSEVINIT: 1",atcmdsync_GetLine(respPtr,1)) == 0)
        )
    {
        result = LE_OK;
    }

    le_mem_Release(atReqRef);
    le_mem_Release(respPtr);

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function set default configuration for gnss
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t DefaultConfig
(
    void
)
{
    SetFsmEvent();
    if ( SetNmeaPort() != LE_OK)
    {
        LE_WARN("cannot set the nmea port");
        return LE_FAULT;
    }

    if ( SetNmeaFrame() != LE_OK)
    {
        LE_WARN("cannot set the nmea frame");
        return LE_FAULT;
    }

    if ( ExecGnssInit() != LE_OK )
    {
        LE_WARN("cannot initialize the gnss");
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function Initialize the gnss layer.
 *
 * @return LE_NOT_POSSIBLE The function failed to Initialize the platform adapter layer.
 * @return LE_COMM_ERROR   The communication device has returned an error.
 * @return LE_TIMEOUT      No response was received from the Modem.
 * @return LE_OK           The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GnssInit
(
    void
)
{
    if (atports_GetInterface(ATPORT_COMMAND)==NULL) {
        LE_WARN("gnss Module is not initialize in this session");
        return LE_NOT_POSSIBLE;
    }

    GnssMutex = le_mutex_CreateNonRecursive("GnssMutex");

    GnssPosPoolRef = le_mem_CreatePool("GnssPosPoolRef", sizeof(pa_Gnss_Position_t));

    GnssEventUnsolId = le_event_CreateId("gnssEventIdUnsol",sizeof(atmgr_UnsolResponse_t));
    GnssEventId          = le_event_CreateIdWithRefCounting("gnssEventId");

    le_event_AddHandler("GNSSUnsolHandler",GnssEventUnsolId  ,GNSSUnsolHandler);

    GnssEventFSMId = le_event_CreateId("gnssEventFsmId",sizeof(atmgr_UnsolResponse_t));
    le_event_AddHandler("GNSSFsmHandler",GnssEventFSMId  ,GNSSFsmHandler);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the PA gnss Module.
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_gnss_Init
(
    void
)
{
    if ( atmgr_IsStarted() == false )
    {
        atmgr_Start();

        atcmdsync_Init();

        atports_Init();
    }

    if (NmeaPortRef) {
        LE_WARN("gnss by AT command is already initialized");
        return LE_OK;
    }

    atmgr_StartInterface(atports_GetInterface(ATPORT_COMMAND));

    le_sem_Ref_t semPtr = le_sem_Create("GNSSStartSem",0);

    GnssThreadRef = le_thread_Create("GNSS",GnssThread,semPtr);
    le_thread_Start(GnssThreadRef);

    le_sem_Wait(semPtr);
    LE_INFO("GNSS is started");
    le_sem_Delete(semPtr);

    if ( DefaultConfig() != LE_OK )
    {
        LE_WARN("gnss is not configured as expected");
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to release the PA gnss Module.
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_gnss_Release
(
    void
)
{
    atcmd_Ref_t atReqRef=NULL;
    atcmdsync_ResultRef_t  respPtr = NULL;
    const char* interRespPtr[] = { "OK" , NULL };
    const char* finalOkPtr[] = {"+GPSEVRELEASE:",NULL};
    const char* finalKoPtr[] = { "ERROR","+GPS ERROR:","+CME ERROR:","+CMS ERROR","TIMEOUT",NULL};

    atReqRef = atcmdsync_PrepareStandardCommand("AT+GPSRELEASE",
                                                    interRespPtr,
                                                    finalOkPtr,
                                                    finalKoPtr,
                                                    30000);
    respPtr = atcmdsync_SendCommand(atports_GetInterface(ATPORT_COMMAND),atReqRef);

    if  ( !
            ((strcmp("OK",atcmdsync_GetLine(respPtr,0)) == 0)
            &&
            (strcmp("+GPSEVRELEASE: 1",atcmdsync_GetLine(respPtr,1)) == 0))
        )
    {
        LE_WARN("cannot release the gnss");
        le_mem_Release(atReqRef);
        le_mem_Release(respPtr);
        return LE_FAULT;
    }

    le_mem_Release(atReqRef);
    le_mem_Release(respPtr);

    if (NmeaPortRef==NULL) {
        LE_WARN("gnss nmea was not initialized");
        return LE_FAULT;
    } else {
        // TODO: FIX when GNSS dedicated port is enable
        le_mutex_Delete(GnssMutex);
//         atmgr_StopInterface(NmeaPortRef);
        NmeaPortRef = NULL;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to start the gnss acquisition.
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_gnss_Start
(
    void
)
{
    le_result_t  result=LE_NOT_POSSIBLE;
    atcmd_Ref_t atReqRef=NULL;
    atcmdsync_ResultRef_t  respPtr = NULL;
    const char* interRespPtr[] = { "OK" , NULL };
    const char* finalOkPtr[] = {"+GPSEVSTART:",NULL};
    const char* finalKoPtr[] = { "ERROR","+GPS ERROR:","+CME ERROR:","+CMS ERROR","TIMEOUT",NULL};

    atReqRef = atcmdsync_PrepareStandardCommand("AT+GPSSTART=3",
                                                    interRespPtr,
                                                    finalOkPtr,
                                                    finalKoPtr,
                                                    30000);

    respPtr = atcmdsync_SendCommand(atports_GetInterface(ATPORT_COMMAND),atReqRef);

    if  (   (strcmp("OK",atcmdsync_GetLine(respPtr,0)) == 0)
            &&
            (strcmp("+GPSEVSTART: 1",atcmdsync_GetLine(respPtr,1)) == 0)
        )
    {
        result = LE_OK;
    }
    else
    {
        LE_WARN("cannot start gnss tracking");
        result =  LE_FAULT;
    }

    le_mem_Release(atReqRef);
    le_mem_Release(respPtr);

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to stop the gnss acquisition.
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_gnss_Stop
(
    void
)
{
    le_result_t  result=LE_NOT_POSSIBLE;
    atcmd_Ref_t atReqRef=NULL;
    atcmdsync_ResultRef_t  respPtr = NULL;
    const char* interRespPtr[] = { "OK" , NULL };
    const char* finalOkPtr[] = {"+GPSEVSTOP:",NULL};
    const char* finalKoPtr[] = { "ERROR","+GPS ERROR:","+CME ERROR:","+CMS ERROR","TIMEOUT",NULL};

    atReqRef = atcmdsync_PrepareStandardCommand("AT+GPSSTOP",
                                                    interRespPtr,
                                                    finalOkPtr,
                                                    finalKoPtr,
                                                    30000);

    respPtr = atcmdsync_SendCommand(atports_GetInterface(ATPORT_COMMAND),atReqRef);

    if  (   (strcmp("OK",atcmdsync_GetLine(respPtr,0)) == 0)
            &&
            (strcmp("+GPSEVSTOP: 1",atcmdsync_GetLine(respPtr,1)) == 0)
        )
    {
        result = LE_OK;
    }
    else
    {
        LE_WARN("cannot stop gnss acquisition");
        result =  LE_FAULT;
    }

    le_mem_Release(atReqRef);
    le_mem_Release(respPtr);
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the rate of gps fix reception
 *
 * @return LE_NOT_POSSIBLE  The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_gnss_SetAcquisitionRate
(
    uint32_t rate     ///< [IN] rate in seconds
)
{
    atcmd_Ref_t atReqRef=NULL;
    atcmdsync_ResultRef_t  respPtr = NULL;
    const char* finalOkPtr[] = {"OK",NULL};
    const char* finalKoPtr[] = { "ERROR","+GPS ERROR:","+CME ERROR:","+CMS ERROR","TIMEOUT",NULL};
    char gnss_at_cmd[ATCOMMAND_SIZE] ;

    atcmdsync_PrepareString(gnss_at_cmd,ATCOMMAND_SIZE,"AT+GPSNMEA=1,%d,FFFF",rate);
    atReqRef = atcmdsync_PrepareStandardCommand(gnss_at_cmd,
                                                    NULL,
                                                    finalOkPtr,
                                                    finalKoPtr,
                                                    30000);
    // TODO: use tree manager to get config on which uart set NMEA frame (for now USB (3))
    respPtr = atcmdsync_SendCommand(atports_GetInterface(ATPORT_COMMAND),atReqRef);
    le_result_t result = atcmdsync_CheckCommandResult(respPtr,finalOkPtr,finalKoPtr);

    le_mem_Release(atReqRef);
    le_mem_Release(respPtr);
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register an handler for gnss position data notifications.
 *
 * @return A handler reference, which is only needed for later removal of the handler.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_event_HandlerRef_t pa_gnss_AddPositionDataHandler
(
    pa_gnss_PositionDataHandlerFunc_t handler ///< [IN] The handler function.
)
{
    LE_FATAL_IF((handler==NULL),"gnss module cannot set handler");

    le_event_HandlerRef_t newHandlerPtr = le_event_AddHandler(
                                                            "gpsInformationHandler",
                                                            GnssEventId,
                                                            (le_event_HandlerFunc_t) handler);

    return newHandlerPtr;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to remove a handler for gnss position data notifications.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
void pa_gnss_RemovePositionDataHandler
(
    le_event_HandlerRef_t    handlerRef ///< [IN] The handler reference.
)
{
    le_event_RemoveHandler(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the location's data.
 *
 * @return LE_NOT_POSSIBLE  The function cannot get internal position information
 * @return LE_OK            The function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_gnss_GetLastPositionData
(
    pa_Gnss_Position_Ref_t  positionRef   ///< [OUT] Reference to a position struct
)
{
    if (positionRef==NULL)
    {
        LE_WARN("Parameter are not valid");
        return LE_FAULT;
    }

    memcpy(positionRef,&LastPosition,sizeof(LastPosition));

    return LE_OK;
}

