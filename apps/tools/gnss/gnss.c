//-------------------------------------------------------------------------------------------------
/**
 * @file gnss.c
 *
 * Tool to debug/monitor GNSS device.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//-------------------------------------------------------------------------------------------------


#include "legato.h"
#include "interfaces.h"

//-------------------------------------------------------------------------------------------------
/**
 * Default time(in second) for 3D fixing after starting gnss device.
 */
//-------------------------------------------------------------------------------------------------
#define DEFAULT_3D_FIX_TIME   60


//-------------------------------------------------------------------------------------------------
/**
 * Default watch period(in second) to get positioning information.
 */
//-------------------------------------------------------------------------------------------------
#define DEFAULT_WATCH_PERIOD   10*60


//-------------------------------------------------------------------------------------------------
/**
 * Max characters for constellations name.
 */
//-------------------------------------------------------------------------------------------------
#define CONSTELLATIONS_NAME_LEN     256


//-------------------------------------------------------------------------------------------------
/**
 * Base 10
 */
//-------------------------------------------------------------------------------------------------
#define BASE10 10

//-------------------------------------------------------------------------------------------------
/**
 * Different type of constellation.
 * {@
 */
//-------------------------------------------------------------------------------------------------
#define CONSTELLATION_GPS           0x1
#define CONSTELLATION_GLONASS       0x2
#define CONSTELLATION_BEIDOU        0x4
#define CONSTELLATION_GALILEO       0x8
#define CONSTELLATION_UNUSED        0x10  // not supported : this constellation cannot be set.
#define CONSTELLATION_QZSS          0x20
// @}

//-------------------------------------------------------------------------------------------------
/**
 * Position handler reference.
 */
//-------------------------------------------------------------------------------------------------
static le_gnss_PositionHandlerRef_t PositionHandlerRef;


//-------------------------------------------------------------------------------------------------
/**
 * Array to store get parameter name.
 */
//-------------------------------------------------------------------------------------------------
static char ParamsName[128] = "";

//-------------------------------------------------------------------------------------------------
/**
 * Print the help text to stdout.
 */
//-------------------------------------------------------------------------------------------------
void PrintGnssHelp
(
    void
)
{
    puts("\n\t\tNAME:\n"
         "\t\t\tgnss - Used to access different functionality of gnss\n\n"
         "\t\tSYNOPSIS:\n"
         "\t\t\tgnss help\n"
         "\t\t\tgnss <enable/disable>\n"
         "\t\t\tgnss <start/stop>\n"
         "\t\t\tgnss restart <RestartType>\n"
         "\t\t\tgnss fix [FixTime in seconds]\n"
         "\t\t\tgnss get <parameter>\n"
         "\t\t\tgnss get posInfo\n"
         "\t\t\tgnss set constellation <ConstellationType>\n"
         "\t\t\tgnss set constArea <Constellation> <ConstellationArea>\n"
         "\t\t\tgnss set agpsMode <ModeType>\n"
         "\t\t\tgnss set acqRate <acqRate in milliseconds>\n"
         "\t\t\tgnss set nmeaSentences <nmeaMask>\n"
         "\t\t\tgnss set minElevation <minElevation in degrees>\n"
         "\t\t\tgnss watch [WatchPeriod in seconds]\n\n"
         "\t\tDESCRIPTION:\n"
         "\t\t\tgnss help\n"
         "\t\t\t\t- Print this help message and exit\n\n"
         "\t\t\tgnss <enable/disable>\n"
         "\t\t\t\t- Enable/disable gnss device\n\n"
         "\t\t\tgnss <start/stop>\n"
         "\t\t\t\t- Start/stop gnss device\n\n"
         "\t\t\tgnss restart <RestartType>\n"
         "\t\t\t\t- Restart gnss device. Allowed when device in 'active' state. Restart type can\n"
         "\t\t\t\t  be as follows:\n"
         "\t\t\t\t\t- hot\n"
         "\t\t\t\t\t- warm\n"
         "\t\t\t\t\t- cold\n"
         "\t\t\t\t\t- factory\n"
         "\t\t\t\tTo know more about these restart types, please look at: \n"
         "\t\t\t\t           https://docs.legato.io/latest/c_gnss.html\n\n"
         "\t\t\tgnss fix [FixTime in seconds]\n"
         "\t\t\t\t- Loop for certain time for first position fix. Here, FixTime is optional.\n"
         "\t\t\t\t  Default time(60s) will be used if not specified\n\n"
         "\t\t\tgnss get <parameter>\n"
         "\t\t\t\t- Used to get different gnss parameter.\n"
         "\t\t\t\t  Follows parameters and their descriptions :\n"
         "\t\t\t\t\t- ttff          --> Time to First Fix (milliseconds)\n"
         "\t\t\t\t\t- acqRate       --> Acquisition Rate (unit milliseconds)\n"
         "\t\t\t\t\t- agpsMode      --> Agps Mode\n"
         "\t\t\t\t\t- nmeaSentences --> Enabled NMEA sentences (bit mask)\n"
         "\t\t\t\t\t- minElevation  --> Minimum elevation in degrees\n"
         "\t\t\t\t\t- constellation --> GNSS constellation\n"
         "\t\t\t\t\t- constArea     --> Area for each constellation\n"
         "\t\t\t\t\t- posState      --> Position fix state(no fix, 2D, 3D etc)\n"
         "\t\t\t\t\t- loc2d         --> 2D location (latitude, longitude, horizontal accuracy)\n"
         "\t\t\t\t\t- alt           --> Altitude (Altitude, Vertical accuracy)\n"
         "\t\t\t\t\t- altOnWgs84    --> Altitude with respect to the WGS-84 ellipsoid\n"
         "\t\t\t\t\t- loc3d         --> 3D location (latitude, longitude, altitude,\n"
         "\t\t\t\t\t                horizontal accuracy, vertical accuracy)\n"
         "\t\t\t\t\t- gpsTime       --> Get last updated gps time\n"
         "\t\t\t\t\t- time          --> Time of the last updated location\n"
         "\t\t\t\t\t- epochTime     --> Epoch time of the last updated location\n"
         "\t\t\t\t\t- timeAcc       --> Time accuracy in milliseconds\n"
         "\t\t\t\t\t- LeapSeconds   --> Current and next leap seconds\n"
         "\t\t\t\t\t- date          --> Date of the last updated location\n"
         "\t\t\t\t\t- hSpeed        --> Horizontal speed(Horizontal Speed, Horizontal\n"
         "\t\t\t\t\t                    Speed accuracy)\n"
         "\t\t\t\t\t- vSpeed        --> Vertical speed(Vertical Speed, Vertical Speed accuracy)\n"
         "\t\t\t\t\t- motion        --> Motion data (Horizontal Speed, Horizontal Speed accuracy,\n"
         "\t\t\t\t\t                    Vertical Speed, Vertical Speed accuracy)\n"
         "\t\t\t\t\t- direction     --> Direction indication\n"
         "\t\t\t\t\t- satInfo       --> Satellites Vehicle information\n"
         "\t\t\t\t\t- satStat       --> Satellites Vehicle status\n"
         "\t\t\t\t\t- dop           --> Dilution of Precision for the fixed position. Displayed\n"
         "\t\t\t\t\t-               in all resolutions: (0 to 3 digits after the decimal point) \n"
         "\t\t\t\t\t- posInfo       --> Get all current position info of the device\n"
         "\t\t\t\t\t- status        --> Get gnss device's current status\n\n"
         "\t\t\tgnss set constellation <ConstellationType>\n"
         "\t\t\t\t- Used to set constellation. Allowed when device in 'ready' state. May require\n"
         "\t\t\t\t  platform reboot, please look platform documentation for details.\n"
         "\t\t\t\t  ConstellationType can be as follows:\n"
         "\t\t\t\t\t- 1 ---> GPS\n"
         "\t\t\t\t\t- 2 ---> GLONASS\n"
         "\t\t\t\t\t- 4 ---> BEIDOU\n"
         "\t\t\t\t\t- 8 ---> GALILEO\n"
         "\t\t\t\t\t- 16 --> Unused\n"
         "\t\t\t\t\t- 32 --> QZSS\n"
         "\t\t\t\tPlease use sum of the values to set multiple constellation, e.g.\n"
         "\t\t\t\t3 for GPS+GLONASS, 47 for GPS+GLONASS+BEIDOU+GALILEO+QZSS\n\n"
         "\t\t\tgnss set constArea <Constellation> <ConstellationArea>\n"
         "\t\t\t\t- Used to set constellation area. Allowed when device in 'ready' state. May\n"
         "\t\t\t\t  require platform reboot, please look platform documentation for details.\n"
         "\t\t\t\t  Constellation can be as follows:\n"
         "\t\t\t\t\t- 1 ---> GPS\n"
         "\t\t\t\t\t- 2 ---> Unused\n"
         "\t\t\t\t\t- 3 ---> GLONASS\n"
         "\t\t\t\t\t- 4 ---> GALILEO\n"
         "\t\t\t\t\t- 5 ---> BEIDOU\n"
         "\t\t\t\t\t- 6 ---> QZSS\n"
         "\t\t\t\t  ConstellationArea can be as follows:\n"
         "\t\t\t\t\t- 0 ---> UNSET_AREA\n"
         "\t\t\t\t\t- 1 ---> WORLDWIDE_AREA\n"
         "\t\t\t\t\t- 2 ---> OUTSIDE_US_AREA\n"
         "\t\t\tgnss set agpsMode <ModeType>\n"
         "\t\t\t\t- Used to set agps mode. ModeType can be as follows:\n"
         "\t\t\t\t\t- alone -----> Standalone agps mode\n"
         "\t\t\t\t\t- msBase ----> MS-based agps mode\n"
         "\t\t\t\t\t- msAssist --> MS-assisted agps mode\n\n"
         "\t\t\tgnss set acqRate <acqRate in milliseconds>\n"
         "\t\t\t\t- Used to set acquisition rate.\n"
         "\t\t\t\t  Please note that it is available when the device is 'ready' state.\n\n"
         "\t\t\tgnss set nmeaSentences <nmeaMask>\n"
         "\t\t\t\t- Used to set the enabled NMEA sentences. \n"
         "\t\t\t\t  Bit mask should be set with hexadecimal values, e.g. 7FFF\n\n"
         "\t\t\t\t- Used to set nmea sentences. Allowed when device in 'ready' state. May require\n"
         "\t\t\t\t  platform reboot, please look platform documentation for details.\n"
         "\t\t\t\t  nmeaMask can be as follows (the values are in hexadecimal):\n"
         "\t\t\t\t\t- 1 ------> GPGGA\n"
         "\t\t\t\t\t- 2 ------> GPGSA\n"
         "\t\t\t\t\t- 4 ------> GPGSV\n"
         "\t\t\t\t\t- 8 ------> GPRMC\n"
         "\t\t\t\t\t- 10 -----> GPVTG\n"
         "\t\t\t\t\t- 20 -----> GLGSV\n"
         "\t\t\t\t\t- 40 -----> GNGNS\n"
         "\t\t\t\t\t- 80 -----> GNGSA\n"
         "\t\t\t\t\t- 100 ----> GAGGA\n"
         "\t\t\t\t\t- 200 ----> GAGSA\n"
         "\t\t\t\t\t- 400 ----> GAGSV\n"
         "\t\t\t\t\t- 800 ----> GARMC\n"
         "\t\t\t\t\t- 1000 ---> GAVTG\n"
         "\t\t\t\t\t- 2000 ---> PSTIS\n"
         "\t\t\t\t\t- 4000 ---> PQXFI\n"
         "\t\t\t\t\t- 8000 ---> PTYPE\n"
         "\t\t\t\t\t- 10000 --> GPGRS\n"
         "\t\t\t\t\t- 20000 --> GPGLL\n"
         "\t\t\t\t\t- 40000 --> DEBUG\n"
         "\t\t\t\t\t- 80000 --> GPDTM\n"
         "\t\t\t\t\t- 100000 -> GAGNS\n"
         "\t\t\tgnss set minElevation <minElevation in degrees>\n"
         "\t\t\t\t- Used to set the minimum elevation in degrees [range 0..90].\n\n"
         "\t\t\tgnss watch [WatchPeriod in seconds]\n"
         "\t\t\t\t- Used to monitor all gnss information(position, speed, satellites used etc).\n"
         "\t\t\t\t  Here, WatchPeriod is optional. Default time(600s) will be used if not\n"
         "\t\t\t\t  specified\n\n"
         "\tPlease note, some commands require gnss device to be in specific state\n"
         "\t(and platform reboot) to produce valid result. Please look :\n"
         "\thttps://docs.legato.io/latest/howToGNSS.html,\n"
         "\thttps://docs.legato.io/latest/c_gnss.html and platform documentation for more\n"
         "\tdetails.\n"
         );
}


//-------------------------------------------------------------------------------------------------
/**
 * This function enables gnss device.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int Enable
(
    void
)
{
    le_result_t result = LE_FAULT;

    result = le_gnss_Enable();

    switch (result)
    {
        case LE_OK:
            printf("Success!\n");
            break;
        case LE_DUPLICATE:
            printf("The GNSS device is already enabled\n");
            break;
        case LE_NOT_PERMITTED:
            printf("The GNSS device is not initialized\n");
            break;
        case LE_FAULT:
            printf("Failed to enable GNSS device\n");
            break;
        default:
            printf("Invalid status\n");
            break;
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}



//-------------------------------------------------------------------------------------------------
/**
 * This function disables gnss device.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int Disable
(
    void
)
{
    le_result_t result = LE_FAULT;

    result = le_gnss_Disable();

    switch (result)
    {
        case LE_OK:
            printf("Success!\n");
            break;
        case LE_DUPLICATE:
            printf("The GNSS device is already disabled\n");
            break;
        case LE_NOT_PERMITTED:
            printf("The GNSS device is not initialized or started. Please see log for details\n");
            break;
        case LE_FAULT:
            printf("Failed to disable GNSS device\n");
            break;
        default:
            printf("Invalid status\n");
            break;
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}


//-------------------------------------------------------------------------------------------------
/**
 * This function starts gnss device.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int Start
(
    void
)
{
    le_result_t result = LE_FAULT;

    result = le_gnss_Start();

    switch (result)
    {
        case LE_OK:
            printf("Success!\n");
            break;
        case LE_DUPLICATE:
            printf("The GNSS device is already started\n");
            break;
        case LE_NOT_PERMITTED:
            printf("The GNSS device is disabled or not initialized. See logs for details\n");
            break;
        case LE_FAULT:
            printf("Failed to start GNSS device. See logs for details\n");
            break;
        default:
            printf("Invalid status\n");
            break;
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}


//-------------------------------------------------------------------------------------------------
/**
 * This function stops gnss device.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int Stop
(
    void
)
{
    le_result_t result = LE_FAULT;

    result = le_gnss_Stop();

    switch (result)
    {
        case LE_OK:
            printf("Success!\n");
            break;
        case LE_DUPLICATE:
            printf("The GNSS device is already stopped\n");
            break;
        case LE_NOT_PERMITTED:
            printf("The GNSS device is not initialized or disabled. See logs for details\n");
            break;
        case LE_FAULT:
            printf("Failed to stop GNSS device. See logs for details\n");
            break;
        default:
            printf("Invalid status\n");
            break;
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}


//-------------------------------------------------------------------------------------------------
/**
 * This function restarts gnss device.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int Restart
(
    const char* restartTypePtr      ///< [IN] Type of restart, i.e. hot/warm/cold/factory etc
)
{
    le_result_t result = LE_FAULT;


    if (strcmp(restartTypePtr, "cold") == 0)
    {
        printf("Doing cold restart...\n");
        result = le_gnss_ForceColdRestart();
    }
    else if (strcmp(restartTypePtr, "warm") == 0)
    {
        printf("Doing warm restart...\n");
        result = le_gnss_ForceWarmRestart();
    }
    else if (strcmp(restartTypePtr, "hot") == 0)
    {
        printf("Doing hot restart...\n");
        result = le_gnss_ForceHotRestart();
    }
    else if (strcmp(restartTypePtr, "factory") == 0)
    {
        printf("Doing factory restart...\n");
        result = le_gnss_ForceFactoryRestart();
    }
    else
    {
        printf("Invalid parameter: %s\n", restartTypePtr);
        return EXIT_FAILURE;
    }

    switch (result)
    {
        case LE_OK:
            printf("Success!\n");
            break;
        case LE_NOT_PERMITTED:
            printf("The GNSS device is not enabled or not started. See logs for details\n");
            break;
        case LE_FAULT:
            printf("Failed to do '%s' restart. See logs for details\n", restartTypePtr);
            break;
        default:
            printf("Invalid status\n");
            break;
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}


//-------------------------------------------------------------------------------------------------
/**
 * This function sets gnss device acquisition rate.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int SetAcquisitionRate
(
    const char* acqRateStr          ///< [IN] Acquisition rate in milliseconds
)
{
    char *end;
    uint32_t acqRate = strtoul(acqRateStr, &end, BASE10);

    if ('\0' != end[0])
    {
        printf("Bad acquisition rate: %s\n", acqRateStr);
        return EXIT_FAILURE;
    }

    le_result_t result = le_gnss_SetAcquisitionRate(acqRate);

    switch (result)
    {
        case LE_OK:
            printf("Success!\n");
            break;
        case LE_FAULT:
            printf("Failed to Set acquisition rate\n");
            break;
        case LE_UNSUPPORTED:
            printf("Request is not supported\n");
            break;
        case LE_NOT_PERMITTED:
            printf("GNSS device is not in \"ready\" state\n");
            break;
        case LE_TIMEOUT:
            printf("Timeout error\n");
            break;
        default:
            printf("Invalid status\n");
            break;
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function sets the GNSS minimum elevation.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int SetMinElevation
(
    const char* minElevationPtr           ///< [IN] Minimum elevation in degrees [range 0..90]
)
{
    char *end;
    uint32_t minElevation = strtoul(minElevationPtr, &end, BASE10);

    if ('\0' != end[0])
    {
        printf("Bad minimum elevation: %s\n", minElevationPtr);
        return EXIT_FAILURE;
    }

    le_result_t result = le_gnss_SetMinElevation(minElevation);

    switch (result)
    {
        case LE_OK:
            printf("Success!\n");
            break;
        case LE_FAULT:
            printf("Failed to set the minimum elevation\n");
            break;
        case LE_UNSUPPORTED:
            printf("Setting the minimum elevation is not supported\n");
            break;
        case LE_OUT_OF_RANGE:
            printf("The minimum elevation is above range\n");
            break;
        default:
            printf("Invalid status\n");
            break;
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function sets constellation of gnss device.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int SetConstellation
(
    const char* constellationPtr   ///< [IN] GNSS constellation used in solution
)
{
    uint32_t constellationMask = 0;

    char *endPtr;
    errno = 0;
    int constellationSum = strtoul(constellationPtr, &endPtr, 10);

    if (endPtr[0] != '\0' || errno != 0 || constellationSum == 0)
    {
        fprintf(stderr, "Bad constellation parameter: %s\n", constellationPtr);
        exit(EXIT_FAILURE);
    }

    char constellationStr[CONSTELLATIONS_NAME_LEN] = "[";

    if (constellationSum & CONSTELLATION_GPS)
    {
        constellationMask |= (uint32_t)LE_GNSS_CONSTELLATION_GPS;
        constellationSum -= CONSTELLATION_GPS;
        strncat(constellationStr, "GPS ", sizeof(constellationStr)-strlen(constellationStr)-1);
    }
    if (constellationSum & CONSTELLATION_GLONASS)
    {
        constellationMask |= (uint32_t)LE_GNSS_CONSTELLATION_GLONASS;
        constellationSum -= CONSTELLATION_GLONASS;
        strncat(constellationStr, "GLONASS ", sizeof(constellationStr)-strlen(constellationStr)-1);
    }
    if (constellationSum & CONSTELLATION_BEIDOU)
    {
        constellationMask |= (uint32_t)LE_GNSS_CONSTELLATION_BEIDOU;
        constellationSum -= CONSTELLATION_BEIDOU;
        strncat(constellationStr, "BEIDOU ", sizeof(constellationStr)-strlen(constellationStr)-1);
    }
    if (constellationSum & CONSTELLATION_GALILEO)
    {
        constellationMask |= (uint32_t)LE_GNSS_CONSTELLATION_GALILEO;
        constellationSum -= CONSTELLATION_GALILEO;
        strncat(constellationStr, "GALILEO ", sizeof(constellationStr)-strlen(constellationStr)-1);
    }
    if (constellationSum & CONSTELLATION_QZSS)
    {
        constellationMask |= (uint32_t)LE_GNSS_CONSTELLATION_QZSS;
        constellationSum -= CONSTELLATION_QZSS;
        strncat(constellationStr, "QZSS ", sizeof(constellationStr)-strlen(constellationStr)-1);
    }
    strncat(constellationStr, "]", sizeof(constellationStr)-strlen(constellationStr)-1);

    LE_INFO("Setting constellation %s",constellationStr);

    // Right now all constellation sum should be zero
    if (constellationSum != 0)
    {
        fprintf(stderr, "Bad constellation parameter: %s\n", constellationPtr);
        exit(EXIT_FAILURE);
    }

    le_result_t result =
       le_gnss_SetConstellation((le_gnss_ConstellationBitMask_t)constellationMask);

    switch(result)
    {
        case LE_OK:
            printf("Success!\n");
            break;
        case LE_UNSUPPORTED:
            printf("Setting constellation %s is not supported\n", constellationStr);
            break;
        case LE_NOT_PERMITTED:
            printf("The GNSS device is not initialized, disabled or active. See logs for  \
                    details\n");
            break;
        case LE_FAULT:
            printf("Failed!\n");
            break;
        default:
            printf("Bad return value: %d\n", result);
            break;
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function sets the area for a given constellation
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int SetConstellationArea
(
    const char* constellationPtr,      ///< [IN] GNSS constellation used in solution
    const char* constellationAreaPtr   ///< [IN] GNSS constellation area
)
{
    char *endPtr;
    errno = 0;
    int constellation = strtoul(constellationPtr, &endPtr, BASE10);
    int constArea = strtoul(constellationAreaPtr, &endPtr, BASE10);

    if (('\0' != endPtr[0]) || (0 != errno) || (0 == constellation) || (0 == constArea))
    {
        fprintf(stderr, "Bad constellation or area parameter: %s %s\n", constellationPtr,
                                                                        constellationAreaPtr);
        exit(EXIT_FAILURE);
    }

    le_result_t result = le_gnss_SetConstellationArea((le_gnss_Constellation_t)constellation,
                                                      (le_gnss_ConstellationArea_t) constArea);
    switch(result)
    {
        case LE_OK:
            printf("Success!\n");
            break;
        case LE_UNSUPPORTED:
            printf("Setting area %d for constellation %d is not supported\n",
                   constArea, constellation);
            break;
        case LE_NOT_PERMITTED:
            printf("The GNSS device is not initialized, disabled or active. See logs for  \
                    details\n");
            break;
        case LE_FAULT:
            printf("Failed!\n");
            break;
        case LE_BAD_PARAMETER:
            printf("Invalid area\n");
            break;
        default:
            printf("Bad return value: %d\n", result);
            break;
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function sets agps mode of gnss device.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int SetAgpsMode
(
    const char* agpsModePtr    ///< [IN] agps to set
)
{
    le_gnss_AssistedMode_t suplAgpsMode;

    if (strcmp(agpsModePtr, "alone") == 0)
    {
        suplAgpsMode = LE_GNSS_STANDALONE_MODE;
    }
    else if (strcmp(agpsModePtr,"msBase") == 0)
    {
        suplAgpsMode = LE_GNSS_MS_BASED_MODE;
    }
    else if (strcmp(agpsModePtr,"msAssist") == 0)
    {
        suplAgpsMode = LE_GNSS_MS_ASSISTED_MODE;
    }
    else
    {
        printf("Bad agps mode: %s\n", agpsModePtr);
        return EXIT_FAILURE;
    }

    le_result_t result = le_gnss_SetSuplAssistedMode(suplAgpsMode);

    switch(result)
    {
        case LE_OK:
            printf("Success!\n");
            break;
        case LE_UNSUPPORTED:
            printf("The request is not supported\n");
            break;
        case LE_TIMEOUT:
            printf("Timeout error\n");
            break;
        case LE_FAULT:
            printf("Failed!\n");
            break;
        default:
            printf("Bad return value: %d\n", result);
            break;
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}


//-------------------------------------------------------------------------------------------------
/**
 * This function sets the enabled NMEA sentences.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int SetNmeaSentences
(
    const char* nmeaMaskStr     ///< [IN] Enabled NMEA sentences bit mask
)
{
    int nmeaMask = le_hex_HexaToInteger(nmeaMaskStr);

    if (nmeaMask < 0)
    {
        printf("Bad NMEA sentences mask: %s\n", nmeaMaskStr);
        return EXIT_FAILURE;
    }

    le_result_t result = le_gnss_SetNmeaSentences(nmeaMask);

    switch (result)
    {
        case LE_OK:
            printf("Successfully set enabled NMEA sentences!\n");
            break;
        case LE_FAULT:
            printf("Failed to set enabled NMEA sentences. See logs for details\n");
            break;
        case LE_BAD_PARAMETER:
            printf("Failed to set enabled NMEA sentences, incompatible bit mask\n");
            break;
        case LE_BUSY:
            printf("Failed to set enabled NMEA sentences, service is busy\n");
            break;
        case LE_TIMEOUT:
            printf("Failed to set enabled NMEA sentences, timeout error\n");
            break;
        default:
            printf("Failed to set enabled NMEA sentences, error %d (%s)\n",
                    result, LE_RESULT_TXT(result));
            break;
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}


//-------------------------------------------------------------------------------------------------
/**
 * This function gets TTFF (Time to First Fix) value.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetTtff
(
    le_gnss_State_t state    ///< [IN] GNSS state
)
{
    uint32_t ttff;
    le_result_t result;

    if (LE_GNSS_STATE_ACTIVE != state)
    {
        printf("GNSS is not in active state!\n");
        return EXIT_FAILURE;
    }

    result = le_gnss_GetTtff(&ttff);
    switch (result)
    {
        case LE_OK:
            printf("TTFF(Time to First Fix) = %ums\n", ttff);
            break;
        case LE_BUSY:
            printf("TTFF not calculated (Position not fixed)\n");
            break;
        case LE_NOT_PERMITTED:
            printf("The GNSS device is not started or disabled. See logs for details\n");
            break;
        default:
            printf("Invalid status\n");
            break;
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}


//-------------------------------------------------------------------------------------------------
/**
 * This function gets the agps mode.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetAgpsMode
(
    void
)
{
    le_gnss_AssistedMode_t assistedMode;
    le_result_t result = le_gnss_GetSuplAssistedMode(&assistedMode);

    if (result == LE_OK)
    {
        switch (assistedMode)
        {
            case LE_GNSS_STANDALONE_MODE:
                printf("AGPS mode: Standalone\n");
                break;
            case LE_GNSS_MS_BASED_MODE:
                printf("AGPS mode: MS-based\n");
                break;
            case LE_GNSS_MS_ASSISTED_MODE:
                printf("AGPS mode: MS-assisted\n");
                break;
        }
    }
    else
    {
        printf("Failed! See log for details\n");
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}


//-------------------------------------------------------------------------------------------------
/**
 * This function gets constellation of gnss device.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetConstellation
(
    void
)
{
    le_gnss_ConstellationBitMask_t constellationMask;
    le_result_t result = le_gnss_GetConstellation(&constellationMask);

    if (result == LE_OK)
    {
        printf("ConstellationType %d\n", constellationMask);

        (constellationMask & LE_GNSS_CONSTELLATION_GPS)     ? printf("GPS activated\n") :
                                                              printf("GPS not activated\n");
        (constellationMask & LE_GNSS_CONSTELLATION_GLONASS) ? printf("GLONASS activated\n") :
                                                              printf("GLONASS not activated\n");
        (constellationMask & LE_GNSS_CONSTELLATION_BEIDOU)  ? printf("BEIDOU activated\n") :
                                                              printf("BEIDOU not activated\n");
        (constellationMask & LE_GNSS_CONSTELLATION_GALILEO) ? printf("GALILEO activated\n") :
                                                              printf("GALILEO not activated\n");
        (constellationMask & LE_GNSS_CONSTELLATION_QZSS)    ? printf("QZSS activated\n") :
                                                              printf("QZSS not activated\n");
    }
    else
    {
        printf("Failed! See log for details!\n");
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}


//-------------------------------------------------------------------------------------------------
/**
 * This function gets the area of each constellation of gnss device.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetConstellationArea
(
    void
)
{
    le_gnss_ConstellationArea_t constellationArea;
    le_gnss_Constellation_t constType = LE_GNSS_SV_CONSTELLATION_GPS;
    le_result_t result;
    static const char *tabConstellation[] =
    {
        "UNDEFINED CONSTELLATION",
        "GPS CONSTELLATION",
        "SBAS CONSTELLATION",
        "GLONASS CONSTELLATION ",
        "GALILEO CONSTELLATION",
        "BEIDOU CONSTELLATION",
        "QZSS CONSTELLATION",
    };

    do
    {
        result = le_gnss_GetConstellationArea(constType, &constellationArea);
        if (LE_OK == result)
        {
            printf("%s area %d\n", tabConstellation[constType], constellationArea);
        }
        else if (LE_UNSUPPORTED == result)
        {
            printf("%s unsupported area\n", tabConstellation[constType]);
        }
        else
        {
            printf("Failed! See log for details!\n");
            return EXIT_FAILURE;
        }
        constType++;
    }
    while (LE_GNSS_SV_CONSTELLATION_MAX != constType);

    return EXIT_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function gets gnss device acquisition rate.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetAcquisitionRate
(
    void
)
{
    uint32_t acqRate;
    le_result_t result = le_gnss_GetAcquisitionRate(&acqRate);

    switch (result)
    {
        case LE_OK:
            printf("Acquisition Rate: %ums\n", acqRate);
            break;
        case LE_FAULT:
            printf("Failed to get acquisition rate. See logs for details\n");
            break;
        case LE_NOT_PERMITTED:
            printf("GNSS device is not in \"ready\" state\n");
            break;
        default:
            printf("Invalid status\n");
            break;
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}


//-------------------------------------------------------------------------------------------------
/**
 * This function gets the GNSS minimum elevation.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetMinElevation
(
    void
)
{
    uint8_t  minElevation;
    le_result_t result = le_gnss_GetMinElevation(&minElevation);

    switch (result)
    {
        case LE_OK:
            printf("Minimum elevation: %d\n", minElevation);
            break;
        case LE_FAULT:
            printf("Failed to get the minimum elevation. See logs for details\n");
            break;
        case LE_UNSUPPORTED:
            printf("Request not supported\n");
            break;
        default:
            printf("Invalid status\n");
            break;
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function gets the enabled NMEA sentences.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetNmeaSentences
(
    void
)
{
    le_gnss_NmeaBitMask_t nmeaMask;
    le_result_t result = le_gnss_GetNmeaSentences(&nmeaMask);

    switch (result)
    {
        case LE_OK:
            printf("Enabled NMEA sentences bit mask = 0x%08X\n", nmeaMask);
            if (nmeaMask & LE_GNSS_NMEA_MASK_GPGGA)
            {
                printf("\tGPGGA (GPS fix data) enabled\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_GPGSA)
            {
                printf("\tGPGSA (GPS DOP and active satellites) enabled\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_GPGSV)
            {
                printf("\tGPGSV (GPS satellites in view) enabled\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_GPRMC)
            {
                printf("\tGPRMC (GPS recommended minimum data) enabled\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_GPVTG)
            {
                printf("\tGPVTG (GPS vector track and speed over the ground) enabled\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_GLGSV)
            {
                printf("\tGLGSV (GLONASS satellites in view) enabled\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_GNGNS)
            {
                printf("\tGNGNS (GNSS fix data) enabled\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_GNGSA)
            {
                printf("\tGNGSA (GNSS DOP and active satellites) enabled\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_GAGGA)
            {
                printf("\tGAGGA (Galileo fix data) enabled\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_GAGSA)
            {
                printf("\tGAGSA (Galileo DOP and active satellites) enabled\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_GAGSV)
            {
                printf("\tGAGSV (Galileo satellites in view) enabled\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_GARMC)
            {
                printf("\tGARMC (Galileo recommended minimum data) enabled\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_GAVTG)
            {
                printf("\tGAVTG (Galileo vector track and speed over the ground) enabled\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_PSTIS)
            {
                printf("\tPSTIS (GPS session start indication) enabled\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_PQXFI)
            {
                printf("\tPQXFI (Proprietary Qualcomm eXtended Fix Information) enabled\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_PTYPE)
            {
                printf("\tPTYPE (Proprietary Type mask) enabled\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_GPGRS)
            {
                printf("\tGPGRS (GPS Range residuals) enabled\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_GPGLL)
            {
                printf("\tGPGLL (GPS Geographic position, latitude / longitude) enabled\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_DEBUG)
            {
               printf("\tDEBUG (Debug NMEA indication) enabled\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_GPDTM)
            {
               printf("\tGPDTM (Local geodetic datum and datum offset from a reference) enabled\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_GAGNS)
            {
               printf("\tGAGNS (Fix data for Galileo) enabled\n");
            }
            break;
        case LE_FAULT:
            printf("Failed to get enabled NMEA sentences. See logs for details\n");
            break;
        case LE_BUSY:
            printf("Failed to get enabled NMEA sentences, service is busy\n");
            break;
        case LE_TIMEOUT:
            printf("Failed to get enabled NMEA sentences, timeout error\n");
            break;
        default:
            printf("Failed to get enabled NMEA sentences, error %d (%s)\n",
                    result, LE_RESULT_TXT(result));
            break;
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}


//-------------------------------------------------------------------------------------------------
/**
 * This function gets position fix for last updated sample.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetPosState
(
    le_gnss_SampleRef_t positionSampleRef    ///< [IN] Position sample reference
)
{
    le_gnss_FixState_t state;
    le_result_t result = le_gnss_GetPositionState( positionSampleRef,
                                                   &state);
    if (result == LE_OK)
    {
        printf("Position state: %s\n", (state == LE_GNSS_STATE_FIX_NO_POS)?"No Fix"
                                     : (state == LE_GNSS_STATE_FIX_2D)?"2D Fix"
                                     : (state == LE_GNSS_STATE_FIX_3D)?"3D Fix"
                                     : (state == LE_GNSS_STATE_FIX_ESTIMATED)?"Estimated Fix"
                                     : "Invalid");
    }
    else
    {
        printf("Failed! See log for details\n");
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}


//-------------------------------------------------------------------------------------------------
/**
 * This function gets latitude, longitude and horizontal accuracy of last updated location.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int Get2Dlocation
(
    le_gnss_SampleRef_t positionSampleRef    ///< [IN] Position sample reference
)
{
    int32_t     latitude;
    int32_t     longitude;
    int32_t     hAccuracy;

    le_result_t result = le_gnss_GetLocation( positionSampleRef,
                                              &latitude,
                                              &longitude,
                                              &hAccuracy);

    if (result == LE_OK)
    {
        printf("Latitude(positive->north) : %.6f\n"
               "Longitude(positive->east) : %.6f\n"
               "hAccuracy                 : %.2fm\n",
                (float)latitude/1e6,
                (float)longitude/1e6,
                (float)hAccuracy/1e2);
    }
    else if(result == LE_OUT_OF_RANGE)
    {
        printf("Location invalid [%d, %d, %d]\n",
               latitude,
               longitude,
               hAccuracy);
    }
    else
    {
        printf("Failed! See log for details\n");
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}


//-------------------------------------------------------------------------------------------------
/**
 * This function gets latitude and vertical accuracy of last updated location.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetAltitude
(
    le_gnss_SampleRef_t positionSampleRef    ///< [IN] Position sample reference
)
{
    int32_t altitude;
    int32_t vAccuracy;

    le_result_t result = le_gnss_GetAltitude( positionSampleRef,
                                              &altitude,
                                              &vAccuracy);

    if(result == LE_OK)
    {
        printf("Altitude  : %.3fm\n"
               "vAccuracy : %.1fm\n",
               (float)altitude/1e3,
               (float)vAccuracy/10.0);
    }
    else if (result == LE_OUT_OF_RANGE)
    {
        printf("Altitude invalid [%d, %d]\n",
               altitude,
               vAccuracy);
    }
    else
    {
        printf("Failed! See log for details\n");
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function gets the altitude with respect to the WGS-84 ellipsoid of last updated location.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetAltitudeOnWgs84
(
    le_gnss_SampleRef_t positionSampleRef    ///< [IN] Position sample reference
)
{
    int32_t altitudeOnWgs84;

    le_result_t result = le_gnss_GetAltitudeOnWgs84(positionSampleRef, &altitudeOnWgs84);

    if (LE_OK == result)
    {
        printf("AltitudeOnWgs84  : %.3fm\n", (float)altitudeOnWgs84/1e3);
    }
    else if (LE_OUT_OF_RANGE == result)
    {
        printf("AltitudeOnWgs84 invalid [%d]\n", altitudeOnWgs84);
    }
    else
    {
        printf("Failed! See log for details\n");
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}


//-------------------------------------------------------------------------------------------------
/**
 * This function gets gps time of last updated sample.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetGpsTime
(
    le_gnss_SampleRef_t positionSampleRef    ///< [IN] Position sample reference
)
{
    uint32_t gpsWeek;
    uint32_t gpsTimeOfWeek;

    le_result_t result = le_gnss_GetGpsTime( positionSampleRef,
                                             &gpsWeek,
                                             &gpsTimeOfWeek);

    if (result == LE_OK)
    {
        printf("GPS time, Week %02d:TimeOfWeek %d ms\n",
                gpsWeek,
                gpsTimeOfWeek);
    }
    else if (result == LE_OUT_OF_RANGE)
    {
        printf("GPS time invalid [%d, %d]\n",
                gpsWeek,
                gpsTimeOfWeek);
    }
    else
    {
        printf("Failed! See log for details\n");
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}


//-------------------------------------------------------------------------------------------------
/**
 * This function gets time of last updated location.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetTime
(
    le_gnss_SampleRef_t positionSampleRef    ///< [IN] Position sample reference
)
{
    uint16_t    hours = 0;
    uint16_t    minutes = 0;
    uint16_t    seconds = 0;
    uint16_t    milliseconds = 0;

    le_result_t result = le_gnss_GetTime( positionSampleRef,
                                          &hours,
                                          &minutes,
                                          &seconds,
                                          &milliseconds);

    if (result == LE_OK)
    {
        printf("Time(HH:MM:SS:MS) %02d:%02d:%02d:%03d\n",
                hours,
                minutes,
                seconds,
                milliseconds);
    }
    else if(result == LE_OUT_OF_RANGE)
    {
        printf("Time invalid %02d:%02d:%02d.%03d\n",
                hours,
                minutes,
                seconds,
                milliseconds);
    }
    else
    {
        printf("Failed! See log for details\n");
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function gets Epoch time of last updated location.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetEpochTime
(
    le_gnss_SampleRef_t positionSampleRef    ///< [IN] Position sample reference
)
{
    uint64_t epochTime;          ///< Epoch time in milliseconds since Jan. 1, 1970

    le_result_t result = le_gnss_GetEpochTime( positionSampleRef, &epochTime);

    if (LE_OK == result)
    {
        printf("Epoch Time %llu ms\n", (unsigned long long int) epochTime);
    }
    else if (LE_OUT_OF_RANGE == result)
    {
        printf("Time invalid %llu ms\n", (unsigned long long int) epochTime);
    }
    else
    {
        printf("Failed! See log for details\n");
    }

    int status = (result == LE_OK) ? EXIT_SUCCESS: EXIT_FAILURE;
    return status;
}


//-------------------------------------------------------------------------------------------------
/**
 * This function gets time accuracy of last updated sample.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetTimeAccuracy
(
    le_gnss_SampleRef_t positionSampleRef    ///< [IN] Position sample reference
)
{
    uint32_t timeAccuracy;
    le_result_t result = le_gnss_GetTimeAccuracy( positionSampleRef,
                                                  &timeAccuracy);

    if (result == LE_OK)
    {
        printf("GPS time accuracy %dns\n", timeAccuracy);
    }
    else if (result == LE_OUT_OF_RANGE)
    {
        printf("GPS time accuracy invalid [%d]\n", timeAccuracy);
    }
    else
    {
        printf("Failed! See log for details!\n");
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function gets current GPS time, leap seconds, next leap seconds event time and next leap
 * seconds value.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetLeapSeconds
(
    void
)
{
    int32_t currentLeapSec, nextLeapSec;
    uint64_t gpsTimeMs, nextEventMs;
    le_result_t result;

    result = le_gnss_GetLeapSeconds(&gpsTimeMs, &currentLeapSec, &nextEventMs,&nextLeapSec);

    if (LE_OK == result)
    {
        printf("Leap seconds report:\n");

        printf("\tCurrent GPS time: ");
        if (gpsTimeMs != UINT64_MAX)
        {
            printf("%"PRIu64" ms\n", gpsTimeMs);
        }
        else
        {
            printf("\n");
        }

        printf("\tLeap seconds: ");
        if (currentLeapSec != INT32_MAX)
        {
            printf("%"PRIi32" ms\n",currentLeapSec);
        }
        else
        {
            printf("\n");
        }

        printf("\tNext event in: ");
        if (nextEventMs != UINT64_MAX)
        {
            printf("%"PRIu64" ms\n", nextEventMs);
        }
        else
        {
            printf("\n");
        }

        printf("\tNext leap seconds in: ");
        if (nextLeapSec != INT32_MAX)
        {
            printf("%"PRIi32" ms\n",nextLeapSec);
        }
        else
        {
            printf("\n");
        }
    }
    else if (LE_TIMEOUT == result)
    {
        printf("Timeout for getting next leap second event.\n");
    }
    else
    {
        printf("Failed! See log for details!\n");
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function gets the date of updated location.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetDate
(
    le_gnss_SampleRef_t positionSampleRef    ///< [IN] Position sample reference
)
{
    uint16_t    year = 0;
    uint16_t    month = 0;
    uint16_t    day = 0;

    le_result_t result = le_gnss_GetDate( positionSampleRef,
                                         &year,
                                         &month,
                                         &day);

    if (result == LE_OK)
    {
        printf("Date(YYYY-MM-DD) %04d-%02d-%02d\n",
                year,
                month,
                day);

    }
    else if(result == LE_OUT_OF_RANGE)
    {
        printf("Date invalid %04d-%02d-%02d\n",
                year,
                month,
                day);
    }
    else
    {
        printf("Failed! See log for details!\n");
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}


//-------------------------------------------------------------------------------------------------
/**
 * This function gets horizontal speed and its accuracy of last updated sample.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetHorizontalSpeed
(
    le_gnss_SampleRef_t positionSampleRef    ///< [IN] Position sample reference
)
{
    uint32_t hSpeed;
    uint32_t hSpeedAccuracy;

    le_result_t result = le_gnss_GetHorizontalSpeed( positionSampleRef,
                                                     &hSpeed,
                                                     &hSpeedAccuracy);
    if (result == LE_OK)
    {
        printf("hSpeed %.2fm/s\n"
               "Accuracy %.1fm/s\n",
                hSpeed/100.0,
                hSpeedAccuracy/10.0);
    }
    else if (result == LE_OUT_OF_RANGE)
    {
        printf("hSpeed invalid [%u, %u]\n",
                hSpeed,
                hSpeedAccuracy);
    }
    else
    {
        printf("Failed! See log for details!\n");
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}


//-------------------------------------------------------------------------------------------------
/**
 * This function gets vertical speed and its accuracy of last updated sample.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetVerticalSpeed
(
    le_gnss_SampleRef_t positionSampleRef    ///< [IN] Position sample reference
)
{
    int32_t vSpeed;
    int32_t vSpeedAccuracy;

    le_result_t result = le_gnss_GetVerticalSpeed( positionSampleRef,
                                                   &vSpeed,
                                                   &vSpeedAccuracy);
    if (result == LE_OK)
    {
        printf( "vSpeed %.2fm/s\n"
                "Accuracy %.1fm/s\n",
                vSpeed/100.0,
                vSpeedAccuracy/10.0);
    }
    else if (result == LE_OUT_OF_RANGE)
    {
        printf("vSpeed invalid [%d, %d]\n",
                vSpeed,
                vSpeedAccuracy);
    }
    else
    {
        printf("Failed! See log for details!\n");
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}


//-------------------------------------------------------------------------------------------------
/**
 * This function gets direction of gnss device.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetDirection
(
    le_gnss_SampleRef_t positionSampleRef    ///< [IN] Position sample reference
)
{
    uint32_t direction         = 0;
    uint32_t directionAccuracy = 0;

    le_result_t result = le_gnss_GetDirection(positionSampleRef,
                                              &direction,
                                              &directionAccuracy);

    if (result == LE_OK)
    {
        printf("Direction(0 degree is True North) : %.1f degrees\n"
               "Accuracy                          : %.1f degrees\n",
                (float)direction/10.0,
                (float)directionAccuracy/10.0);
    }
    else if(result == LE_OUT_OF_RANGE)
    {
        printf("Direction invalid [%u, %u]\n",
               direction,
               directionAccuracy);
    }
    else
    {
        printf("Failed! See log for details\n");
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function gets the DOP (Dilution of Precision).
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetDop
(
    le_gnss_SampleRef_t positionSampleRef    ///< [IN] Position sample reference
)
{
    uint16_t dop[LE_GNSS_RES_UNKNOWN];
    bool err = false;
    le_result_t result;
    le_gnss_DopType_t dopType = LE_GNSS_PDOP;
    le_gnss_Resolution_t DopRes;

    static const char *tabDop[] =
    {
        "Position dilution of precision (PDOP)",
        "Horizontal dilution of precision (HDOP)",
        "Vertical dilution of precision (VDOP)",
        "Geometric dilution of precision (GDOP)",
        "Time dilution of precision (TDOP)"
    };

    do
    {
        // Get DOP parameter in all resolutions
        for (DopRes=LE_GNSS_RES_ZERO_DECIMAL; DopRes<LE_GNSS_RES_UNKNOWN; DopRes++)
        {
            if (LE_OK != le_gnss_SetDopResolution(DopRes))
            {
                printf("Failed! See log for details!\n");
                return EXIT_FAILURE;
            }

            result = le_gnss_GetDilutionOfPrecision(positionSampleRef,
                                                    dopType,
                                                    &dop[DopRes]);
            if (LE_OUT_OF_RANGE == result)
            {
                printf("%s invalid %d\n", tabDop[dopType], dop[0]);
                err = true;
                break;
            }
            else if (LE_OK != result)
            {
                printf("Failed! See log for details!\n");
                return EXIT_FAILURE;
            }
        }
        if (LE_OK == result)
        {
            printf("%s [%.1f %.1f %.2f %.3f]\n", tabDop[dopType],
                   (float)dop[LE_GNSS_RES_ZERO_DECIMAL],
                   (float)dop[LE_GNSS_RES_ONE_DECIMAL]/10,
                   (float)dop[LE_GNSS_RES_TWO_DECIMAL]/100,
                   (float)dop[LE_GNSS_RES_THREE_DECIMAL]/1000);
        }
        dopType++;
    }
    while (dopType != LE_GNSS_DOP_LAST);

    return err ? EXIT_FAILURE : EXIT_SUCCESS;
}


//-------------------------------------------------------------------------------------------------
/**
 * This function gets the Satellites Vehicle information.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetSatelliteInfo
(
    le_gnss_SampleRef_t positionSampleRef    ///< [IN] Position sample reference
)
{
    // Satellites information
    uint16_t satIdPtr[LE_GNSS_SV_INFO_MAX_LEN];
    size_t satIdNumElements = NUM_ARRAY_MEMBERS(satIdPtr);
    le_gnss_Constellation_t satConstPtr[LE_GNSS_SV_INFO_MAX_LEN];
    size_t satConstNumElements = NUM_ARRAY_MEMBERS(satConstPtr);
    bool satUsedPtr[LE_GNSS_SV_INFO_MAX_LEN];
    size_t satUsedNumElements = NUM_ARRAY_MEMBERS(satUsedPtr);
    uint8_t satSnrPtr[LE_GNSS_SV_INFO_MAX_LEN];
    size_t satSnrNumElements = NUM_ARRAY_MEMBERS(satSnrPtr);
    uint16_t satAzimPtr[LE_GNSS_SV_INFO_MAX_LEN];
    size_t satAzimNumElements = NUM_ARRAY_MEMBERS(satAzimPtr);
    uint8_t satElevPtr[LE_GNSS_SV_INFO_MAX_LEN];
    size_t satElevNumElements = NUM_ARRAY_MEMBERS(satElevPtr);
    int i;

    le_result_t result =  le_gnss_GetSatellitesInfo( positionSampleRef,
                                                     satIdPtr,
                                                     &satIdNumElements,
                                                     satConstPtr,
                                                     &satConstNumElements,
                                                     satUsedPtr,
                                                     &satUsedNumElements,
                                                     satSnrPtr,
                                                     &satSnrNumElements,
                                                     satAzimPtr,
                                                     &satAzimNumElements,
                                                     satElevPtr,
                                                     &satElevNumElements);

    if((result == LE_OK)||(result == LE_OUT_OF_RANGE))
    {
        // Satellite Vehicle information
        for(i=0; i<satIdNumElements; i++)
        {
            if((satIdPtr[i] != 0)&&(satIdPtr[i] != UINT8_MAX))
            {
                printf("[%02d] SVid %03d - C%01d - U%d - SNR%02d - Azim%03d - Elev%02d\n"
                        , i
                        , satIdPtr[i]
                        , satConstPtr[i]
                        , satUsedPtr[i]
                        , satSnrPtr[i]
                        , satAzimPtr[i]
                        , satElevPtr[i]);

                if (LE_GNSS_SV_CONSTELLATION_SBAS == satConstPtr[i])
                {
                    printf("SBAS category : %d\n",
                           le_gnss_GetSbasConstellationCategory(satIdPtr[i]));
                }
            }
        }
    }
    else
    {
        printf("Failed! See log for details!\n");
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}


//-------------------------------------------------------------------------------------------------
/**
 * This function gets the Satellites Vehicle status.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetSatelliteStatus
(
    le_gnss_SampleRef_t positionSampleRef    ///< [IN] Position sample reference
)
{
    uint8_t satsInViewCount;
    uint8_t satsTrackingCount;
    uint8_t satsUsedCount;
    le_result_t result =  le_gnss_GetSatellitesStatus( positionSampleRef,
                                                       &satsInViewCount,
                                                       &satsTrackingCount,
                                                       &satsUsedCount);

    LE_ASSERT((result == LE_OK)||(result == LE_OUT_OF_RANGE));

    if ((result == LE_OK) || (result == LE_OUT_OF_RANGE))
    {
        printf("satsInView %d - satsTracking %d - satsUsed %d\n",
               (satsInViewCount == UINT8_MAX) ? 0: satsInViewCount,
               (satsTrackingCount == UINT8_MAX) ? 0: satsTrackingCount,
               (satsUsedCount == UINT8_MAX) ? 0: satsUsedCount);
    }
    else
    {
        printf("Failed! See log for details!\n");
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}


//-------------------------------------------------------------------------------------------------
/**
 * Function to get all positional information of last updated sample.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetPosInfo
(
    le_gnss_SampleRef_t positionSampleRef    ///< [IN] Position sample reference
)
{
    int status = EXIT_SUCCESS;
    status = (EXIT_FAILURE == GetTtff(le_gnss_GetState())) ? EXIT_FAILURE : status;
    status = (EXIT_FAILURE == GetPosState(positionSampleRef)) ? EXIT_FAILURE : status;
    status = (EXIT_FAILURE == Get2Dlocation(positionSampleRef)) ? EXIT_FAILURE : status;
    status = (EXIT_FAILURE == GetAltitude(positionSampleRef)) ? EXIT_FAILURE : status;
    status = (EXIT_FAILURE == GetAltitudeOnWgs84(positionSampleRef)) ? EXIT_FAILURE : status;
    status = (EXIT_FAILURE == GetGpsTime(positionSampleRef)) ? EXIT_FAILURE : status;
    status = (EXIT_FAILURE == GetTime(positionSampleRef)) ? EXIT_FAILURE : status;
    status = (EXIT_FAILURE == GetEpochTime(positionSampleRef)) ? EXIT_FAILURE : status;
    status = (EXIT_FAILURE == GetTimeAccuracy(positionSampleRef)) ? EXIT_FAILURE : status;
    status = (EXIT_FAILURE == GetDate(positionSampleRef)) ? EXIT_FAILURE : status;
    status = (EXIT_FAILURE == GetDop(positionSampleRef)) ? EXIT_FAILURE : status;
    status = (EXIT_FAILURE == GetHorizontalSpeed(positionSampleRef)) ? EXIT_FAILURE : status;
    status = (EXIT_FAILURE == GetVerticalSpeed(positionSampleRef)) ? EXIT_FAILURE : status;
    status = (EXIT_FAILURE == GetDirection(positionSampleRef)) ? EXIT_FAILURE : status;
    return status;
}


//-------------------------------------------------------------------------------------------------
/**
 * Function to do first position fix.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int DoPosFix
(
    uint32_t fixVal          ///< [IN] Position fix time in seconds
)
{
    uint32_t count = 0;
    le_result_t result = le_gnss_Start();

    if (result == LE_NOT_PERMITTED)
    {
        printf("GNSS was not enabled. Enabling it\n");
        if (le_gnss_Enable() != LE_OK)
        {
            fprintf(stderr, "Failed to enable GNSS. Try rebooting device. Exiting\n");
            return EXIT_FAILURE;
        }

        // Now start GNSS device
        if(le_gnss_Start() != LE_OK)
        {
            fprintf(stderr, "Failed to start GNSS. Try rebooting device. Exiting\n");
            return EXIT_FAILURE;
        }
    }
    else if(result == LE_FAULT)
    {
        fprintf(stderr, "Failed to start GNSS. Try rebooting device. Exiting\n");
        return EXIT_FAILURE;
    }

    result = LE_BUSY;

    while ((result == LE_BUSY) && (count < fixVal))
    {
        // Get TTFF
        uint32_t ttff;
        result = le_gnss_GetTtff(&ttff);

        if (result == LE_OK)
        {
            printf("TTFF start = %d msec\n", ttff);
            return EXIT_SUCCESS;
        }
        else if (result == LE_BUSY)
        {
            count++;
            printf("TTFF not calculated (Position not fixed)\n");
            sleep(1);
        }
        else
        {
            printf("Failed! See log for details\n");
            return EXIT_FAILURE;
        }
    }

    return EXIT_FAILURE;
}


//--------------------------------------------------------------------------------------------------
/**
 * Handler function for Position Notifications.
 *
 */
//--------------------------------------------------------------------------------------------------
static void PositionHandlerFunction
(
    le_gnss_SampleRef_t positionSampleRef,    ///< [IN] Position sample reference
    void* contextPtr                          ///< [IN] The context pointer
)
{

    if (strcmp(ParamsName, "watch") == 0)
    {
        GetPosInfo(positionSampleRef);
        GetSatelliteStatus(positionSampleRef);
        GetSatelliteInfo(positionSampleRef);
        // Release provided Position sample reference
        le_gnss_ReleaseSampleRef(positionSampleRef);
    }
    else
    {
        int status = EXIT_FAILURE;

        if (strcmp(ParamsName, "posState") == 0)
        {
            status = GetPosState(positionSampleRef);
        }
        else if (strcmp(ParamsName, "loc2d") == 0)
        {
            status = Get2Dlocation(positionSampleRef);
        }
        else if (strcmp(ParamsName, "alt") == 0)
        {
            status = GetAltitude(positionSampleRef);
        }
        else if (0 == strcmp(ParamsName, "altOnWgs84"))
        {
            status = GetAltitudeOnWgs84(positionSampleRef);
        }
        else if (strcmp(ParamsName, "loc3d") == 0)
        {
            status = EXIT_SUCCESS;
            status = (Get2Dlocation(positionSampleRef) == EXIT_FAILURE) ? EXIT_FAILURE : status;
            status = (GetAltitude(positionSampleRef) == EXIT_FAILURE) ? EXIT_FAILURE : status;
        }
        else if (strcmp(ParamsName, "gpsTime") == 0)
        {
            status = GetGpsTime(positionSampleRef);
        }
        else if (strcmp(ParamsName, "time") == 0)
        {
            status = GetTime(positionSampleRef);
        }
        else if (strcmp(ParamsName, "epochTime") == 0)
        {
            status = GetEpochTime(positionSampleRef);
        }
        else if (strcmp(ParamsName, "timeAcc") == 0)
        {
            status = GetTimeAccuracy(positionSampleRef);
        }
        else if (strcmp(ParamsName, "date") == 0)
        {
            status = GetDate(positionSampleRef);
        }
        else if (strcmp(ParamsName, "hSpeed") == 0)
        {
            status = GetHorizontalSpeed(positionSampleRef);
        }
        else if (strcmp(ParamsName, "vSpeed") == 0)
        {
            status = GetVerticalSpeed(positionSampleRef);
        }
        else if (strcmp(ParamsName, "motion") == 0)
        {
            status = EXIT_SUCCESS;
            status = (GetHorizontalSpeed(positionSampleRef) == EXIT_FAILURE) ?
                                                                             EXIT_FAILURE : status;
            status = (GetVerticalSpeed(positionSampleRef) == EXIT_FAILURE) ? EXIT_FAILURE : status;
        }
        else if (strcmp(ParamsName, "direction") == 0)
        {
            status = GetDirection(positionSampleRef);
        }
        else if (strcmp(ParamsName, "satInfo") == 0)
        {
            status = GetSatelliteInfo(positionSampleRef);
        }
        else if (strcmp(ParamsName, "satStat") == 0)
        {
            status = GetSatelliteStatus(positionSampleRef);
        }
        else if (strcmp(ParamsName, "dop") == 0)
        {
            status = GetDop(positionSampleRef);
        }
        else if (strcmp(ParamsName, "posInfo") == 0)
        {
            status = GetPosInfo(positionSampleRef);
        }

        le_gnss_ReleaseSampleRef(positionSampleRef);
        exit(status);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Thread to monitor all gnss information.
 *
*/
//--------------------------------------------------------------------------------------------------
static void* PositionThread
(
    void* contextPtr             ///< [IN] The context pointer
)
{
    le_gnss_ConnectService();

    PositionHandlerRef = le_gnss_AddPositionHandler(PositionHandlerFunction, NULL);
    LE_ASSERT((PositionHandlerRef != NULL));

    le_event_RunLoop();
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function to enable gnss and monitor its information.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//--------------------------------------------------------------------------------------------------
static int WatchGnssInfo
(
    uint32_t watchPeriod          ///< [IN] Watch period in seconds
)
{
    le_thread_Ref_t positionThreadRef;

    // Add Position Handler
    positionThreadRef = le_thread_Create("PositionThread",PositionThread,NULL);
    le_thread_Start(positionThreadRef);

    printf("Watch positioning data for %ds\n", watchPeriod);
    sleep(watchPeriod);

    le_gnss_RemovePositionHandler(PositionHandlerRef);

    // stop thread
    le_thread_Cancel(positionThreadRef);

    return EXIT_SUCCESS;
}


//-------------------------------------------------------------------------------------------------
/**
 * This function prints the GNSS device status.
 */
//-------------------------------------------------------------------------------------------------
static int GetGnssDeviceStatus
(
void
)
{
    le_gnss_State_t state;
    const char *status;

    state = le_gnss_GetState();

    switch (state)
    {
        case LE_GNSS_STATE_UNINITIALIZED:
            status = "not initialized";
            break;
        case LE_GNSS_STATE_READY:
            status = "ready";
            break;
        case LE_GNSS_STATE_ACTIVE:
            status = "active";
            break;
        case LE_GNSS_STATE_DISABLED:
            status = "disabled";
            break;
        default:
            status = "unknown";
            break;
    }

    fprintf(stdout, "%s\n", status);

    return 0;
};

//-------------------------------------------------------------------------------------------------
/**
 * This function gets different gnss parameters.
 */
//-------------------------------------------------------------------------------------------------
static void GetGnssParams
(
    const char *params      ///< [IN] gnss parameters
)
{
    le_gnss_State_t state = le_gnss_GetState();

    if (0 == strcmp(params, "ttff"))
    {
        exit(GetTtff(state));
    }
    else if (0 == strcmp(params, "acqRate"))
    {
        exit(GetAcquisitionRate());
    }
    else if (0 == strcmp(params, "LeapSeconds"))
    {
        exit(GetLeapSeconds());
    }
    else if (0 == strcmp(params, "agpsMode"))
    {
        exit(GetAgpsMode());
    }
    else if (0 == strcmp(params, "constellation"))
    {
        exit(GetConstellation());
    }
    else if (0 == strcmp(params, "constArea"))
    {
        exit(GetConstellationArea());
    }
    else if (0 == strcmp(params, "nmeaSentences"))
    {
        exit(GetNmeaSentences());
    }
    else if (0 == strcmp(params, "minElevation"))
    {
        exit(GetMinElevation());
    }
    else if ((0 == strcmp(params, "posState"))    ||
             (0 == strcmp(params, "loc2d"))       ||
             (0 == strcmp(params, "alt"))         ||
             (0 == strcmp(params, "altOnWgs84"))  ||
             (0 == strcmp(params, "loc3d"))       ||
             (0 == strcmp(params, "gpsTime"))     ||
             (0 == strcmp(params, "time"))        ||
             (0 == strcmp(params, "epochTime"))   ||
             (0 == strcmp(params, "timeAcc"))     ||
             (0 == strcmp(params, "LeapSeconds")) ||
             (0 == strcmp(params, "date"))        ||
             (0 == strcmp(params, "hSpeed"))      ||
             (0 == strcmp(params, "vSpeed"))      ||
             (0 == strcmp(params, "motion"))      ||
             (0 == strcmp(params, "direction"))   ||
             (0 == strcmp(params, "satInfo"))     ||
             (0 == strcmp(params, "satStat"))     ||
             (0 == strcmp(params, "dop"))         ||
             (0 == strcmp(params, "posInfo")))
    {
        if (LE_GNSS_STATE_ACTIVE != state)
        {
            printf("GNSS is not in active state!\n");
            exit(EXIT_FAILURE);
        }

        // Copy the param
        le_utf8_Copy(ParamsName, params, sizeof(ParamsName), NULL);

        PositionHandlerRef = le_gnss_AddPositionHandler(PositionHandlerFunction, NULL);
        LE_ASSERT((PositionHandlerRef != NULL));
    }
    else if (strcmp(params, "status") == 0)
    {
        exit(GetGnssDeviceStatus());
    }
    else
    {
        printf("Bad parameter: %s\n", params);
        exit(EXIT_FAILURE);
    }

}


//-------------------------------------------------------------------------------------------------
/**
 * This function sets different gnss parameters .
 */
//-------------------------------------------------------------------------------------------------
static int SetGnssParams
(
    const char * argNamePtr,    ///< [IN] gnss parameters
    const char * argValPtr,     ///< [IN] gnss parameters
    const char * arg2ValPtr     ///< [IN] gnss parameters
)
{
    int status = EXIT_FAILURE;

    if (strcmp(argNamePtr, "constellation") == 0)
    {
        status = SetConstellation(argValPtr);
    }
    else if (strcmp(argNamePtr, "constArea") == 0)
    {
        if (NULL == arg2ValPtr)
        {
            LE_ERROR("arg2ValPtr is NULL");
            exit(EXIT_FAILURE);
        }
        status = SetConstellationArea(argValPtr, arg2ValPtr);
    }
    else if (strcmp(argNamePtr, "acqRate") == 0)
    {
        status = SetAcquisitionRate(argValPtr);
    }
    else if (strcmp(argNamePtr, "agpsMode") == 0)
    {
        status = SetAgpsMode(argValPtr);
    }
    else if (strcmp(argNamePtr, "nmeaSentences") == 0)
    {
        status = SetNmeaSentences(argValPtr);
    }
    else if (0 == strcmp(argNamePtr, "minElevation"))
    {
        status = SetMinElevation(argValPtr);
    }
    else
    {
        printf("Bad parameter request: %s\n", argNamePtr);
    }

    exit(status);
}

//--------------------------------------------------------------------------------------------------
/**
 * Verify if enough parameter passed into command.
 * If not, output error message and terminate the program.
 */
//--------------------------------------------------------------------------------------------------
void CheckEnoughParams
(
    size_t requiredParam,      ///< [IN] Required parameters for the command
    size_t numArgs,            ///< [IN] Number of arguments passed into the command line
    const char * errorMsgPtr   ///< [IN] Error message to output if not enough parameters
)
{
    if ( (requiredParam + 1) <= numArgs)
    {
        return;
    }
    else
    {
        printf("%s\nTry '%s help'\n", errorMsgPtr, le_arg_GetProgramName());
        exit(EXIT_FAILURE);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Program init
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // Process the command
    if (le_arg_NumArgs() < 1)
    {
        // No argument specified. Print help and exit.
        PrintGnssHelp();
        exit(EXIT_FAILURE);
    }

    const char* commandPtr = le_arg_GetArg(0);
    if(NULL == commandPtr)
    {
        LE_ERROR("commandPtr is NULL");
        exit(EXIT_FAILURE);
    }
    size_t numArgs = le_arg_NumArgs();

    if (strcmp(commandPtr, "help") == 0)
    {
        PrintGnssHelp();
        exit(EXIT_SUCCESS);
    }
    else if (strcmp(commandPtr, "start") == 0)
    {
        exit(Start());
    }
    else if (strcmp(commandPtr, "stop") == 0)
    {
        exit(Stop());
    }
    else if (strcmp(commandPtr, "enable") == 0)
    {
        exit(Enable());
    }
    else if (strcmp(commandPtr, "disable") == 0)
    {
        exit(Disable());
    }
    else if (strcmp(commandPtr, "restart") == 0)
    {
        const char* restartTypePtr = le_arg_GetArg(1);
        // Following function exit on failure, so no need to check return code.
        CheckEnoughParams( 1,
                           numArgs,
                          "Restart type missing");
        exit(Restart(restartTypePtr));

    }
    else if (strcmp(commandPtr, "fix") == 0)
    {
        const char* fixPeriodPtr = le_arg_GetArg(1);
        uint32_t fixPeriod = DEFAULT_3D_FIX_TIME;
        //Check whether any watch period value is specified.
        if (NULL != fixPeriodPtr)
        {
            char *endPtr;
            errno = 0;
            fixPeriod = strtoul(fixPeriodPtr, &endPtr, 10);

            if (endPtr[0] != '\0' || errno != 0)
            {
                fprintf(stderr, "Bad fix period value: %s\n", fixPeriodPtr);
                exit(EXIT_FAILURE);
            }
        }
        exit(DoPosFix(fixPeriod));
    }
    else if (strcmp(commandPtr, "get") == 0)
    {
        const char* paramsPtr = le_arg_GetArg(1);
        if (NULL == paramsPtr)
        {
            LE_ERROR("paramsPtr is NULL");
            exit(EXIT_FAILURE);
        }
        CheckEnoughParams( 1,
                           numArgs,
                           "Missing arguments");
        GetGnssParams(paramsPtr);
    }
    else if (strcmp(commandPtr, "set") == 0)
    {
        const char* argNamePtr = le_arg_GetArg(1);
        const char* argValPtr = le_arg_GetArg(2);
        const char* arg2ValPtr = le_arg_GetArg(3);
        if (NULL == argNamePtr)
        {
            LE_ERROR("argNamePtr is NULL");
            exit(EXIT_FAILURE);
        }
        if (NULL == argValPtr)
        {
            LE_ERROR("argValPtr is NULL");
            exit(EXIT_FAILURE);
        }
        CheckEnoughParams( 2,
                           numArgs,
                           "Missing arguments");
        exit(SetGnssParams(argNamePtr, argValPtr, arg2ValPtr));
    }
    else if (strcmp(commandPtr, "watch") == 0)
    {
        if (LE_GNSS_STATE_ACTIVE != le_gnss_GetState())
        {
            printf("GNSS is not in active state!\n");
            exit(EXIT_FAILURE);
        }

        const char* watchPeriodPtr = le_arg_GetArg(1);
        uint32_t watchPeriod = DEFAULT_WATCH_PERIOD;
        //Check whether any watch period value is specified.
        if (NULL != watchPeriodPtr)
        {
            char *endPtr;
            errno = 0;
            watchPeriod = strtoul(watchPeriodPtr, &endPtr, 10);

            if (endPtr[0] != '\0' || errno != 0)
            {
                fprintf(stderr, "Bad watch period value: %s\n", watchPeriodPtr);
                exit(EXIT_FAILURE);
            }
        }

        // Copy the command
        le_utf8_Copy(ParamsName, commandPtr, sizeof(ParamsName), NULL);
        exit(WatchGnssInfo(watchPeriod));
    }
    else
    {
        printf("Invalid command for GNSS service\n");
        exit(EXIT_FAILURE);
    }

}

