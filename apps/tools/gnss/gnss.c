//-------------------------------------------------------------------------------------------------
/**
 * @file gnss.c
 *
 * Tool to debug/monitor GNSS device.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
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
 * Different type of constellation.
 * {@
 */
//-------------------------------------------------------------------------------------------------
#define CONSTELLATION_GPS           1
#define CONSTELLATION_GLONASS       2
#define CONSTELLATION_BEIDOU        4
#define CONSTELLATION_GALILEO       8
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
    puts("\
            NAME:\n\
                gnss - Used to access different functionality of gnss\n\
            \n\
            SYNOPSIS:\n\
                gnss help\n\
                gnss <enable/disable>\n\
                gnss <start/stop>\n\
                gnss restart <RestartType>\n\
                gnss fix [FixTime in seconds]\n\
                gnss get <parameter>\n\
                gnss get posInfo\n\
                gnss set constellation <ConstellationType>\n\
                gnss set agpsMode <ModeType>\n\
                gnss set acqRate <acqRate in milliseconds>\n\
                gnss set nmeaSentences <nmeaMask>\n\
                gnss watch [WatchPeriod in seconds]\n\
            \n\
            DESCRIPTION:\n\
                gnss help\n\
                    - Print this help message and exit\n\
                \n\
                gnss <enable/disable>\n\
                    - Enable/disable gnss device\n\
                \n\
                gnss <start/stop>\n\
                    - Start/stop gnss device\n\
                \n\
                gnss restart <RestartType>\n\
                    - Restart gnss device. Allowed when device in 'active' state. Restart type can\n\
                      be as follows:\n\
                         - hot\n\
                         - warm\n\
                         - cold\n\
                         - factory\n\
                    See GNSS topics in the Legato docs for more info on these restart types.\n\
                \n\
                gnss fix [FixTime in seconds]\n\
                    - Loop for certain time for first position fix. Here, FixTime is optional.\n\
                      Default time(60s) will be used if not specified\n\
                \n\
                gnss get <parameter>\n\
                    - Used to get different gnss parameter. Parameters and their descriptions as follow:\n\
                         - ttff --> Time to First Fix (milliseconds)\n\
                         - acqRate       --> Acquisition Rate (unit milliseconds)\n\
                         - agpsMode      --> Agps Mode\n\
                         - nmeaSentences --> Enabled NMEA sentences (bit mask)\n\
                         - constellation --> GNSS constellation\n\
                         - posState      --> Position fix state(no fix, 2D, 3D etc)\n\
                         - loc2d         --> 2D location (latitude, longitude, horizontal accuracy)\n\
                         - alt           --> Altitude (Altitude, Vertical accuracy)\n\
                         - loc3d         --> 3D location (latitude, longitude, altitude, horizontal accuracy,\n\
                                             vertical accuracy)\n\
                         - gpsTime       --> Get last updated gps time\n\
                         - time          --> Time of the last updated location\n\
                         - timeAcc       --> Time accuracy in milliseconds\n\
                         - date          --> Date of the last updated location\n\
                         - hSpeed        --> Horizontal speed(Horizontal Speed, Horizontal Speed accuracy)\n\
                         - vSpeed        --> Vertical speed(Vertical Speed, Vertical Speed accuracy)\n\
                         - motion        --> Motion data (Horizontal Speed, Horizontal Speed accuracy,\n\
                                             Vertical Speed, Vertical Speed accuracy)\n\
                         - direction     --> Direction indication\n\
                         - satInfo       --> Satellites Vehicle information\n\
                         - satStat       --> Satellites Vehicle status\n\
                         - dop           --> Dilution Of Precision for the fixed position\n\
                         - posInfo       --> Get all current position info of the device\n\
                         - status       --> Get gnss device's current status\n\
                \n\
                gnss set constellation <ConstellationType>\n\
                    - Used to set constellation. Allowed when device in 'ready' state. May require\n\
                      platform reboot, refer to platform documentation for details. ConstellationType\n\
                      can be as follows:\n\
                         - 1 --> GPS\n\
                         - 2 --> GLONASS\n\
                         - 4 --> BEIDOU\n\
                         - 8 --> GALILEO\n\
                      Please use sum of the values to set multiple constellation, e.g., 3 for GPS+GLONASS\n\
                      15 for GPS+GLONASS+BEIDOU+GALILEO\n\
                \n\
                gnss set agpsMode <ModeType>\n\
                    - Used to set agps mode. ModeType can be as follows:\n\
                         - alone --> Standalone agps mode\n\
                         - msBase --> MS-based agps mode\n\
                         - msAssist --> MS-assisted agps mode\n\
                \n\
                gnss set acqRate <acqRate in milliseconds>\n\
                    - Used to set acquisition rate. Available when device is in 'ready' state.\n\
                \n\
                gnss set nmeaSentences <nmeaMask>\n\
                    - Used to set the enabled NMEA sentences. \n\
                      Bit mask should be set with hexadecimal values, e.g., 7FFF\n\
                \n\
                gnss watch [WatchPeriod in seconds]\n\
                    - Used to monitor all gnss information (position, speed, satellites used, etc.).\n\
                      Here, WatchPeriod is optional. Default time(600s) will be used if not specified.\n\
                \n\
            Please note, some commands require gnss device to be in specific state (and platform reboot)\n\
            to produce valid result. See GNSS topics in the Legato docs for more info.\n\
         ");
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

    int status = (result == LE_OK) ? EXIT_SUCCESS: EXIT_FAILURE;
    return status;
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

    int status = (result == LE_OK) ? EXIT_SUCCESS: EXIT_FAILURE;
    return status;
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

    int status = (result == LE_OK) ? EXIT_SUCCESS: EXIT_FAILURE;
    return status;
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

    int status = (result == LE_OK) ? EXIT_SUCCESS: EXIT_FAILURE;
    return status;
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

    int status = (result == LE_OK) ? EXIT_SUCCESS: EXIT_FAILURE;
    return status;
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
    const char* acqRateStr          ///< [IN] Acquisition rate in milliseconds.
)
{
    char *end;
    errno = 0;
    uint32_t acqRate = strtoul(acqRateStr, &end, 10);

    if (end[0] != '\0' || errno != 0)
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

    int status = (result == LE_OK) ? EXIT_SUCCESS: EXIT_FAILURE;
    return status;
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
    const char* constellationPtr
)
{
    int32_t constellationMask = 0;

    char *endPtr;
    errno = 0;
    int constellationSum = strtoul(constellationPtr, &endPtr, 10);

    if (endPtr[0] != '\0' || errno != 0 || constellationSum == 0)
    {
        fprintf(stderr, "Bad constellation parameter: %s\n", constellationPtr);
        exit(EXIT_FAILURE);
    }

    char constellationStr[256] = "[";
    if (constellationSum & CONSTELLATION_GPS)
    {
        constellationMask |= LE_GNSS_CONSTELLATION_GPS;
        constellationSum -= CONSTELLATION_GPS;
        strcat(constellationStr, "GPS ");
    }
    if (constellationSum & CONSTELLATION_GLONASS)
    {
        constellationMask |= LE_GNSS_CONSTELLATION_GLONASS;
        constellationSum -= CONSTELLATION_GLONASS;
        strcat(constellationStr, "GLONASS ");
    }
    if (constellationSum & CONSTELLATION_BEIDOU)
    {
        constellationMask = LE_GNSS_CONSTELLATION_BEIDOU;
        constellationSum -= CONSTELLATION_BEIDOU;
        strcat(constellationStr, "BEIDOU ");
    }
    if (constellationSum & CONSTELLATION_GALILEO)
    {
        constellationMask = LE_GNSS_CONSTELLATION_GALILEO;
        constellationSum -= CONSTELLATION_GALILEO;
        strcat(constellationStr, "GALILEO");
    }
    strcat(constellationStr, "]");

    LE_INFO("Setting constellation %s",constellationStr);

    // Right now all constellation sum should be zero
    if (constellationSum != 0)
    {
        fprintf(stderr, "Bad constellation parameter: %s\n", constellationPtr);
        exit(EXIT_FAILURE);
    }

    le_result_t result = le_gnss_SetConstellation(
                                                  (le_gnss_ConstellationBitMask_t)constellationMask
                                                 );

    switch(result)
    {
        case LE_OK:
            printf("Success!\n");
            break;
        case LE_UNSUPPORTED:
            printf("Setting constellation %s is not supported\n", constellationStr);
            break;
        case LE_NOT_PERMITTED:
            printf("The GNSS device is not initialized, disabled or active. See logs for details\n");
            break;
        case LE_FAULT:
            printf("Failed!\n");
            break;
        default:
            printf("Bad return value: %d\n", result);
            break;
    }

    int status = (result == LE_OK) ? EXIT_SUCCESS: EXIT_FAILURE;
    return status;
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
    const char* agpsModePtr
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

    int status = (result == LE_OK) ? EXIT_SUCCESS: EXIT_FAILURE;
    return status;
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
    const char* nmeaMaskStr     ///< [IN] Enabled NMEA sentences bit mask.
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

    int status = (result == LE_OK) ? EXIT_SUCCESS: EXIT_FAILURE;
    return status;
}


//-------------------------------------------------------------------------------------------------
/**
 * This function gets ttff(Time to First Fix) value.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetTtff
(
    void
)
{
    uint32_t ttff;
    le_result_t result = le_gnss_GetTtff(&ttff);

    switch (result)
    {
        case LE_OK:
            printf("TTFF(Time to First Fix) = %ums\n", ttff);
            break;
        case LE_BUSY:
            printf("The position is not fixed and TTFF can't be measured. See logs for details\n");
            break;
        case LE_NOT_PERMITTED:
            printf("The GNSS device is not started or disabled. See logs for details\n");
            break;
        default:
            printf("Invalid status\n");
            break;
    }

    int status = (result == LE_OK) ? EXIT_SUCCESS: EXIT_FAILURE;
    return status;
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

    int status = (result == LE_OK) ? EXIT_SUCCESS: EXIT_FAILURE;
    return status;
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
        if (constellationMask & LE_GNSS_CONSTELLATION_GPS)
        {
            printf("GPS activated\n");
        }
        if (constellationMask & LE_GNSS_CONSTELLATION_GLONASS)
        {
            printf("GLONASS activated\n");
        }
        if (constellationMask & LE_GNSS_CONSTELLATION_BEIDOU)
        {
            printf("BEIDOU activated\n");
        }
        if (constellationMask & LE_GNSS_CONSTELLATION_GALILEO)
        {
            printf("BEIDOU activated\n");
        }
    }
    else
    {
        printf("Failed! See log for details!\n");
    }

    int status = (result == LE_OK) ? EXIT_SUCCESS: EXIT_FAILURE;
    return status;
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

    int status = (result == LE_OK) ? EXIT_SUCCESS: EXIT_FAILURE;
    return status;
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

    int status = (LE_OK == result) ? EXIT_SUCCESS: EXIT_FAILURE;
    return status;
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

    int status = (result == LE_OK) ? EXIT_SUCCESS: EXIT_FAILURE;
    return status;
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
               "hAccuracy                 : %.1fm\n",
                (float)latitude/1e6,
                (float)longitude/1e6,
                (float)hAccuracy/10.0);
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

    int status = (result == LE_OK) ? EXIT_SUCCESS: EXIT_FAILURE;
    return status;
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

    int status = (result == LE_OK) ? EXIT_SUCCESS: EXIT_FAILURE;
    return status;
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

    int status = (result == LE_OK) ? EXIT_SUCCESS: EXIT_FAILURE;
    return status;
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
        printf("GPS time accuracy %dms\n", timeAccuracy);
    }
    else if (result == LE_OUT_OF_RANGE)
    {
        printf("GPS time accuracy invalid [%d]\n", timeAccuracy);
    }
    else
    {
        printf("Failed! See log for details!\n");
    }

    int status = (result == LE_OK) ? EXIT_SUCCESS: EXIT_FAILURE;
    return status;
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

    int status = (result == LE_OK) ? EXIT_SUCCESS: EXIT_FAILURE;
    return status;
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

    int status = (result == LE_OK) ? EXIT_SUCCESS: EXIT_FAILURE;
    return status;
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

    int status = (result == LE_OK) ? EXIT_SUCCESS: EXIT_FAILURE;
    return status;
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

    int status = (result == LE_OK) ? EXIT_SUCCESS: EXIT_FAILURE;
    return status;
}


//-------------------------------------------------------------------------------------------------
/**
 * This function gets DOP(Dilution of Precision).
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
    uint16_t hdop;
    uint16_t vdop;
    uint16_t pdop;

    // Get DOP parameter
    le_result_t result = le_gnss_GetDop( positionSampleRef,
                                         &hdop,
                                         &vdop,
                                         &pdop);

    if (result == LE_OK)
    {
        printf("DOP [H%.1f,V%.1f,P%.1f]\n",
               (float)(hdop)/100,
               (float)(vdop)/100,
               (float)(pdop)/100);
    }
    else if (result == LE_OUT_OF_RANGE)
    {
        printf("DOP invalid [%d, %d, %d]\n",
                hdop,
                vdop,
                pdop);
    }
    else
    {
        printf("Failed! See log for details!\n");
    }

    int status = (result == LE_OK) ? EXIT_SUCCESS: EXIT_FAILURE;
    return status;
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
    size_t satIdNumElements = sizeof(satIdPtr);
    le_gnss_Constellation_t satConstPtr[LE_GNSS_SV_INFO_MAX_LEN];
    size_t satConstNumElements = sizeof(satConstPtr);
    bool satUsedPtr[LE_GNSS_SV_INFO_MAX_LEN];
    size_t satUsedNumElements = sizeof(satUsedPtr);
    uint8_t satSnrPtr[LE_GNSS_SV_INFO_MAX_LEN];
    size_t satSnrNumElements = sizeof(satSnrPtr);
    uint16_t satAzimPtr[LE_GNSS_SV_INFO_MAX_LEN];
    size_t satAzimNumElements = sizeof(satAzimPtr);
    uint8_t satElevPtr[LE_GNSS_SV_INFO_MAX_LEN];
    size_t satElevNumElements = sizeof(satElevPtr);
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
        if (result == LE_OUT_OF_RANGE)
        {
            printf("Satellite information invalid\n");
        }
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
            }
        }
    }
    else
    {
        printf("Failed! See log for details!\n");
    }

    int status = (result == LE_OK) ? EXIT_SUCCESS: EXIT_FAILURE;
    return status;
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

    if (result == LE_OK)
    {
        printf("satsInView %d - satsTracking %d - satsUsed %d\n",
                satsInViewCount,
                satsTrackingCount,
                satsUsedCount);
    }
    else if (result == LE_OUT_OF_RANGE)
    {
        printf("Satellite status invalid [%d, %d, %d]\n",
                satsInViewCount,
                satsTrackingCount,
                satsUsedCount);
    }
    else
    {
        printf("Failed! See log for details!\n");
    }

    int status = (result == LE_OK) ? EXIT_SUCCESS: EXIT_FAILURE;
    return status;
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
    status = (GetTtff() == EXIT_FAILURE) ? EXIT_FAILURE : status;
    status = (GetPosState(positionSampleRef) == EXIT_FAILURE) ? EXIT_FAILURE : status;
    status = (Get2Dlocation(positionSampleRef) == EXIT_FAILURE) ? EXIT_FAILURE : status;
    status = (GetAltitude(positionSampleRef) == EXIT_FAILURE) ? EXIT_FAILURE : status;
    status = (GetGpsTime(positionSampleRef) == EXIT_FAILURE) ? EXIT_FAILURE : status;
    status = (GetTime(positionSampleRef) == EXIT_FAILURE) ? EXIT_FAILURE : status;
    status = (GetTimeAccuracy(positionSampleRef) == EXIT_FAILURE) ? EXIT_FAILURE : status;
    status = (GetDate(positionSampleRef) == EXIT_FAILURE) ? EXIT_FAILURE : status;
    status = (GetDop(positionSampleRef) == EXIT_FAILURE) ? EXIT_FAILURE : status;
    status = (GetHorizontalSpeed(positionSampleRef) == EXIT_FAILURE) ? EXIT_FAILURE : status;
    status = (GetVerticalSpeed(positionSampleRef) == EXIT_FAILURE) ? EXIT_FAILURE : status;
    status = (GetDirection(positionSampleRef) == EXIT_FAILURE) ? EXIT_FAILURE : status;
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
    uint32_t fixVal          ///< [IN] Position fix time in seconds.
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
            printf("TTFF not calculated (Position not fixed) BUSY\n");
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
    le_gnss_SampleRef_t positionSampleRef,
    void* contextPtr
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
            status = (GetHorizontalSpeed(positionSampleRef) == EXIT_FAILURE) ? EXIT_FAILURE : status;
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
    void* context
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
    uint32_t watchPeriod          ///< [IN] Watch period in seconds.
)
{
    le_thread_Ref_t positionThreadRef;

    uint32_t ttff;
    le_result_t result = le_gnss_GetTtff(&ttff);

    if (result != LE_OK)
    {
        printf("Position not fixed. Try 'gnss fix' to fix position\n");
        return EXIT_FAILURE;
    }
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
    const char *params
)
{

    if (strcmp(params, "ttff") == 0)
    {
        exit(GetTtff());
    }
    else if (strcmp(params, "acqRate") == 0)
    {
        exit(GetAcquisitionRate());
    }
    else if (strcmp(params, "agpsMode") == 0)
    {
        exit(GetAgpsMode());
    }
    else if (strcmp(params, "constellation") == 0)
    {
        exit(GetConstellation());
    }
    else if (strcmp(params, "nmeaSentences") == 0)
    {
        exit(GetNmeaSentences());
    }
    else if ((strcmp(params, "posState") == 0)  ||
             (strcmp(params, "loc2d") == 0)     ||
             (strcmp(params, "alt") == 0)       ||
             (strcmp(params, "loc3d") == 0)     ||
             (strcmp(params, "gpsTime") == 0)   ||
             (strcmp(params, "time") == 0)      ||
             (strcmp(params, "timeAcc") == 0)   ||
             (strcmp(params, "date") == 0)      ||
             (strcmp(params, "hSpeed") == 0)    ||
             (strcmp(params, "vSpeed") == 0)    ||
             (strcmp(params, "motion") == 0)    ||
             (strcmp(params, "direction") == 0) ||
             (strcmp(params, "satInfo") == 0)   ||
             (strcmp(params, "satStat") == 0)   ||
             (strcmp(params, "dop") == 0)       ||
             (strcmp(params, "posInfo") == 0)
            )
    {

        // TODO: Check whether device is in active state, then check ttff value.
        uint32_t ttff;
        le_result_t result = le_gnss_GetTtff(&ttff);

        if (result != LE_OK)
        {
            printf("Position not fixed. Try 'gnss fix' to fix position\n");
            exit(EXIT_FAILURE);
        }

        // Copy the param
        strcpy(ParamsName, params);

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
    const char * argNamePtr,
    const char * argValPtr
)
{
    int status = EXIT_FAILURE;

    if (strcmp(argNamePtr, "constellation") == 0)
    {
        status = SetConstellation(argValPtr);
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
        // Following function exit on failure, so no need to check return code.
        CheckEnoughParams( 1,
                           numArgs,
                          "Restart type missing");
        exit(Restart(le_arg_GetArg(1)));

    }
    else if (strcmp(commandPtr, "fix") == 0)
    {
        uint32_t fixPeriod = DEFAULT_3D_FIX_TIME;
        //Check whether any watch period value is specified.
        if (le_arg_GetArg(1) != NULL)
        {
            char *endPtr;
            errno = 0;
            fixPeriod = strtoul(le_arg_GetArg(1), &endPtr, 10);

            if (endPtr[0] != '\0' || errno != 0)
            {
                fprintf(stderr, "Bad fix period value: %s\n", le_arg_GetArg(1));
                exit(EXIT_FAILURE);
            }
        }
        exit(DoPosFix(fixPeriod));
    }
    else if (strcmp(commandPtr, "get") == 0)
    {
        CheckEnoughParams( 1,
                           numArgs,
                           "Missing arguments");
        GetGnssParams(le_arg_GetArg(1));
    }
    else if (strcmp(commandPtr, "set") == 0)
    {
        CheckEnoughParams( 2,
                           numArgs,
                           "Missing arguments");
        exit(SetGnssParams(le_arg_GetArg(1), le_arg_GetArg(2)));
    }
    else if (strcmp(commandPtr, "watch") == 0)
    {

        uint32_t watchPeriod = DEFAULT_WATCH_PERIOD;
        //Check whether any watch period value is specified.
        if (le_arg_GetArg(1) != NULL)
        {
            char *endPtr;
            errno = 0;
            watchPeriod = strtoul(le_arg_GetArg(1), &endPtr, 10);

            if (endPtr[0] != '\0' || errno != 0)
            {
                fprintf(stderr, "Bad watch period value: %s\n", le_arg_GetArg(1));
                exit(EXIT_FAILURE);
            }
        }
        // Copy the command
        strcpy(ParamsName, commandPtr);
        exit(WatchGnssInfo(watchPeriod));
    }
    else
    {
        printf("Invalid command for GNSS service\n");
        exit(EXIT_FAILURE);
    }

}

