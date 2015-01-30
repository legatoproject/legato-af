/**
 * @file le_ecall.c
 *
 * This file contains the source code of the eCall API.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 *
 */

#include "legato.h"
#include "interfaces.h"
#include "mdmCfgEntries.h"
#include "asn1Msd.h"
#include "pa_ecall.h"


//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Define the maximum number of eCall sessions.
 */
//--------------------------------------------------------------------------------------------------
#define ECALL_MAX  1

//--------------------------------------------------------------------------------------------------
/**
 * Propulsion type string length. One extra byte is added for the null character.
 */
//--------------------------------------------------------------------------------------------------
#define PROPULSION_MAX_LEN      (16+1)

//--------------------------------------------------------------------------------------------------
/**
 * Vehicle type string length. One extra byte is added for the null character.
 */
//--------------------------------------------------------------------------------------------------
#define VEHICLE_TYPE_MAX_LEN      (16+1)

//--------------------------------------------------------------------------------------------------
/**
 * Vehicle Identification Number (VIN) string length. One extra byte is added for the null
 * character.
 */
//--------------------------------------------------------------------------------------------------
#define VIN_MAX_LEN      (17+1)

//--------------------------------------------------------------------------------------------------
/**
 * Default MSD version.
 */
//--------------------------------------------------------------------------------------------------
#define DEFAULT_MSD_VERSION     1

//--------------------------------------------------------------------------------------------------
/**
 * Macro: Get bit mask value.
 */
//--------------------------------------------------------------------------------------------------
#define GET_BIT_MASK_VALUE(_value_, _bitmask_) ((_value_ & _bitmask_) ? true : false)

//--------------------------------------------------------------------------------------------------
/**
 * Macro: Set bit mask value.
 */
//--------------------------------------------------------------------------------------------------
#define SET_BIT_MASK_VALUE(_value_, _bitmask_) ((_value_ == true) ? _bitmask_ : 0)

//--------------------------------------------------------------------------------------------------
// Data structures.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * eCall object structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char             psapNumber[LE_MDMDEFS_PHONE_NUM_MAX_BYTES]; ///< PSAP telephone number
    le_ecall_State_t state;                                      ///< eCall state
    bool             isPushed;                                   ///< True if the MSD is pushed by
                                                                 ///  the IVS, false if it  is sent
                                                                 ///  when requested by the PSAP
                                                                 ///  (pull)
    uint32_t         maxRedialAttempts;                          ///< Maximum redial attempts
    msd_t            msd;                                        ///< MSD
    uint8_t          builtMsd[LE_ECALL_MSD_MAX_LEN];             ///< built MSD
    size_t           builtMsdSize;                               ///< Size of the built MSD
}
ECall_t;


//--------------------------------------------------------------------------------------------------
// Static declarations.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Choosen system standard (PAN-EUROPEAN or ERA-GLONASS).
 *
 */
//--------------------------------------------------------------------------------------------------
static pa_ecall_SysStd_t SystemStandard;

//--------------------------------------------------------------------------------------------------
/**
 * Event ID for eCall State notification.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t ECallStateId;

//--------------------------------------------------------------------------------------------------
/**
 * eCall object. Only one eCall object is created.
 *
 */
//--------------------------------------------------------------------------------------------------
static ECall_t ECallObj;

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for eCall objects.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t ECallRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Flag indicating that the previous session was manually stopped.
 *
 */
//--------------------------------------------------------------------------------------------------
static bool IsSessionStopped;

//--------------------------------------------------------------------------------------------------
/**
 * ERA-GLONASS Data object.
 *
 */
//--------------------------------------------------------------------------------------------------
static msd_EraGlonassData_t EraGlonassDataObj;

//--------------------------------------------------------------------------------------------------
/**
 * Convert Decimal Degrees in Degrees/Minutes/Seconds and return the corresponding value in
 * milliarcseconds.
 *
 * ex: 34,53 degrees = 34 degrees + 0,55 degree
 * 0,53 degree = 0,53*60 = 31,8 minutes = 31 minutes + 0,8 minute
 * 0,8 minute = 48 secondes
 *
 * 34.530000 = 34 degrees 31' 48" = (34*60*60.000+31*60.000+48)*1000 milliarcseconds
 *
 * @return value in milliarcseconds.
 */
//--------------------------------------------------------------------------------------------------
static int32_t ConvertDdToDms
(
    int32_t    ddVal
)
{
    int32_t  deg;
    float    degMod;
    float    minAbs=0.0;
    float    secDec=0.0;
    float    sec;

    // compute degrees
    deg = ddVal/1000000;
    LE_INFO("ddVal.%d, degrees.%d", ddVal, deg);

    // compute minutes
    degMod = ddVal%1000000;
    minAbs = degMod*60;
    int32_t min = minAbs/1000000;
    LE_INFO("minute.%d", min);

    // compute seconds
    secDec = minAbs - min*1000000;
    sec = secDec*60 / 1000000;
    LE_INFO("secondes.%e", sec);

    LE_INFO("final result: %d", (int32_t)((deg*60*60.000 + min*60.000 + sec)*1000));

    return ((int32_t)((deg*60*60.000 + min*60.000 + sec)*1000));
}

//--------------------------------------------------------------------------------------------------
/**
 * Parse the vehicle type from the config DB entry and update the corresponding MSD element.
 *
 * @return
 *      - LE_OK on success.
 *      - LE_FAULT no vehicle type found.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ParseAndSetVehicleType
(
    char* vehStr
)
{
    LE_FATAL_IF((vehStr == NULL), "vehStr is NULL !");

    if (!strcmp(vehStr, "Passenger-M1"))
    {
        ECallObj.msd.msdMsg.msdStruct.control.vehType = MSD_VEHICLE_PASSENGER_M1;
        return LE_OK;
    }
    else if (!strcmp(vehStr, "Bus-M2"))
    {
        ECallObj.msd.msdMsg.msdStruct.control.vehType = MSD_VEHICLE_BUS_M2;
        return LE_OK;
    }
    else if (!strcmp(vehStr, "Bus-M3"))
    {
        ECallObj.msd.msdMsg.msdStruct.control.vehType = MSD_VEHICLE_BUS_M3;
        return LE_OK;
    }
    else if (!strcmp(vehStr, "Commercial-N1"))
    {
        ECallObj.msd.msdMsg.msdStruct.control.vehType = MSD_VEHICLE_COMMERCIAL_N1;
        return LE_OK;
    }
    else if (!strcmp(vehStr, "Heavy-N2"))
    {
        ECallObj.msd.msdMsg.msdStruct.control.vehType = MSD_VEHICLE_HEAVY_N2;
        return LE_OK;
    }
    else if (!strcmp(vehStr, "Heavy-N3"))
    {
        ECallObj.msd.msdMsg.msdStruct.control.vehType = MSD_VEHICLE_HEAVY_N3;
        return LE_OK;
    }
    else if (!strcmp(vehStr, "Motorcycle-L1e"))
    {
        ECallObj.msd.msdMsg.msdStruct.control.vehType = MSD_VEHICLE_MOTORCYCLE_L1E;
        return LE_OK;
    }
    else if (!strcmp(vehStr, "Motorcycle-L2e"))
    {
        ECallObj.msd.msdMsg.msdStruct.control.vehType = MSD_VEHICLE_MOTORCYCLE_L2E;
        return LE_OK;
    }
    else if (!strcmp(vehStr, "Motorcycle-L3e"))
    {
        ECallObj.msd.msdMsg.msdStruct.control.vehType = MSD_VEHICLE_MOTORCYCLE_L3E;
        return LE_OK;
    }
    else if (!strcmp(vehStr, "Motorcycle-L4e"))
    {
        ECallObj.msd.msdMsg.msdStruct.control.vehType = MSD_VEHICLE_MOTORCYCLE_L4E;
        return LE_OK;
    }
    else if (!strcmp(vehStr, "Motorcycle-L5e"))
    {
        ECallObj.msd.msdMsg.msdStruct.control.vehType = MSD_VEHICLE_MOTORCYCLE_L5E;
        return LE_OK;
    }
    else if (!strcmp(vehStr, "Motorcycle-L6e"))
    {
        ECallObj.msd.msdMsg.msdStruct.control.vehType = MSD_VEHICLE_MOTORCYCLE_L6E;
        return LE_OK;
    }
    else if (!strcmp(vehStr, "Motorcycle-L7e"))
    {
        ECallObj.msd.msdMsg.msdStruct.control.vehType = MSD_VEHICLE_MOTORCYCLE_L7E;
        return LE_OK;
    }
    else
    {
        return LE_FAULT;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Parse the propulsion type from the config DB entry and update the corresponding MSD element.
 *
 * @return
 *      - LE_OK on success.
 *      - LE_FAULT no propulsion type found.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ParseAndSetPropulsionType
(
    char* propStr
)
{
    LE_FATAL_IF((propStr == NULL), "propStr is NULL !");

    if (!strcmp(propStr, "Gasoline"))
    {
        ECallObj.msd.msdMsg.msdStruct.vehPropulsionStorageType.gasolineTankPresent = true;
        return LE_OK;
    }
    else if (!strcmp(propStr, "Diesel"))
    {
        ECallObj.msd.msdMsg.msdStruct.vehPropulsionStorageType.dieselTankPresent = true;
        return LE_OK;
    }
    else if (!strcmp(propStr, "NaturalGas"))
    {
        ECallObj.msd.msdMsg.msdStruct.vehPropulsionStorageType.compressedNaturalGas = true;
        return LE_OK;
    }
    else if (!strcmp(propStr, "Propane"))
    {
        ECallObj.msd.msdMsg.msdStruct.vehPropulsionStorageType.liquidPropaneGas = true;
        return LE_OK;
    }
    else if (!strcmp(propStr, "Electric"))
    {
        ECallObj.msd.msdMsg.msdStruct.vehPropulsionStorageType.electricEnergyStorage = true;
        return LE_OK;
    }
    else if (!strcmp(propStr, "Hydrogen"))
    {
        ECallObj.msd.msdMsg.msdStruct.vehPropulsionStorageType.hydrogenStorage = true;
        return LE_OK;
    }
    else
    {
        return LE_FAULT;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the propulsions type
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT no valid propulsion type found
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetPropulsionType
(
    void
)
{
    uint8_t i=0;
    char cfgNodeLoc[8] = {0};
    char configPath[LIMIT_MAX_PATH_BYTES];
    char propStr[PROPULSION_MAX_LEN] = {0};
    le_result_t res = LE_OK;

    snprintf(configPath, sizeof(configPath), "%s/%s", CFG_MODEMSERVICE_ECALL_PATH, CFG_NODE_PROP);
    le_cfg_IteratorRef_t propCfg = le_cfg_CreateReadTxn(configPath);

    sprintf (cfgNodeLoc, "%d", i);
    while (!le_cfg_IsEmpty(propCfg, cfgNodeLoc))
    {
        if ( le_cfg_GetString(propCfg, cfgNodeLoc, propStr, sizeof(propStr), "") != LE_OK )
        {
            LE_ERROR("No node value set for '%s'", CFG_NODE_PROP);
            res = LE_FAULT;
            break;
        }
        if ( strncmp(propStr, "", sizeof(propStr)) == 0 )
        {
            LE_ERROR("No node value set for '%s'", propStr);
            res = LE_FAULT;
            break;
        }
        LE_DEBUG("eCall settings, Propulsion is %s", propStr);
        if (ParseAndSetPropulsionType(propStr) != LE_OK)
        {
            LE_ERROR("Bad propulsion type!");
            res = LE_FAULT;
            break;
        }

        i++;
        sprintf (cfgNodeLoc, "%d", i);
    }
    le_cfg_CancelTxn(propCfg);

    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * Load the eCall settings
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT there are missing settings
 */
//--------------------------------------------------------------------------------------------------
static le_result_t LoadECallSettings
(
    void
)
{
    le_result_t res = LE_OK;
    char configPath[LIMIT_MAX_PATH_BYTES];
    snprintf(configPath, sizeof(configPath), "%s", CFG_MODEMSERVICE_ECALL_PATH);

    LE_DEBUG("Start reading eCall information in ConfigDB");

    le_cfg_IteratorRef_t eCallCfg = le_cfg_CreateReadTxn(configPath);

    do
    {
        // Get VIN
        if (le_cfg_NodeExists(eCallCfg, CFG_NODE_VIN))
        {
            char vinStr[VIN_MAX_LEN] = {0};
            if ( le_cfg_GetString(eCallCfg, CFG_NODE_VIN, vinStr, sizeof(vinStr), "") != LE_OK )
            {
                LE_WARN("No node value set for '%s'", CFG_NODE_VIN);
                res = LE_FAULT;
                break;
            }
            else if (strncmp(vinStr, "", sizeof(vinStr)) == 0)
            {
                res = LE_FAULT;
                break;
            }
            memcpy((void *)&ECallObj.msd.msdMsg.msdStruct.vehIdentificationNumber,
                   (const void *)vinStr,
                   sizeof(ECallObj.msd.msdMsg.msdStruct.vehIdentificationNumber));
            LE_DEBUG("eCall settings, VIN is %s", vinStr);
        }
        else
        {
            LE_ERROR("No value set for '%s' !", CFG_NODE_VIN);
            res = LE_FAULT;
            break;
        }

        // Get vehicle type
        if (le_cfg_NodeExists(eCallCfg, CFG_NODE_VEH))
        {
            char  vehStr[VEHICLE_TYPE_MAX_LEN] = {0};
            if ( le_cfg_GetString(eCallCfg, CFG_NODE_VEH, vehStr, sizeof(vehStr), "") != LE_OK )
            {
                LE_WARN("No node value set for '%s'", CFG_NODE_VEH);
                res = LE_FAULT;
                break;
            }
            else if (strncmp(vehStr, "", sizeof(vehStr)) == 0)
            {
                res = LE_FAULT;
                break;
            }
            LE_DEBUG("eCall settings, vehicle is %s", vehStr);
            if ((res = ParseAndSetVehicleType(vehStr)) != LE_OK)
            {
                LE_ERROR("Bad vehicle type!");
                break;
            }
        }
        else
        {
            LE_ERROR("No value set for '%s' !", CFG_NODE_VEH);
            res = LE_FAULT;
            break;
        }

        // Get MSD version
        if (le_cfg_NodeExists(eCallCfg, CFG_NODE_MSDVERSION))
        {
            ECallObj.msd.version = le_cfg_GetInt(eCallCfg, CFG_NODE_MSDVERSION, 0);
            LE_DEBUG("eCall settings, MSD version is %d", ECallObj.msd.version);
            if (ECallObj.msd.version == 0)
            {
                LE_WARN("No correct value set for '%s' ! Use the default one (%d)",
                        CFG_NODE_MSDVERSION,
                        DEFAULT_MSD_VERSION);
                ECallObj.msd.version = 1;
            }
        }
        else
        {
            LE_WARN("No value set for '%s' ! Use the default one (%d)",
                    CFG_NODE_MSDVERSION,
                    DEFAULT_MSD_VERSION);
            ECallObj.msd.version = 1;
        }

        // Get system standard
        {
            char  sysStr[] = "PAN-EUROPEAN";
            bool isEraGlonass = false;
            if (le_cfg_NodeExists(eCallCfg, CFG_NODE_SYSTEM_STD))
            {
                if ( le_cfg_GetString(eCallCfg,
                                      CFG_NODE_SYSTEM_STD,
                                      sysStr,
                                      sizeof(sysStr),
                                      "PAN-EUROPEAN") != LE_OK )
                {
                    LE_WARN("No node value set for '%s' ! Use the default one (%s)",
                            CFG_NODE_SYSTEM_STD,
                            sysStr);
                }
                else if ((strncmp(sysStr, "PAN-EUROPEAN", strlen("PAN-EUROPEAN")) != 0) &&
                        (strncmp(sysStr, "ERA-GLONASS", strlen("ERA-GLONASS")) != 0))
                {
                    LE_WARN("Bad value set for '%s' ! Use the default one (%s)",
                            CFG_NODE_SYSTEM_STD,
                            sysStr);
                }
                LE_DEBUG("eCall settings, system standard is %s", sysStr);
                if (strncmp(sysStr, "ERA-GLONASS", strlen("ERA-GLONASS")) == 0)
                {
                    isEraGlonass = true;
                }
            }
            else
            {
                LE_WARN("No node value set for '%s' ! Use the default one (%s)",
                        CFG_NODE_SYSTEM_STD,
                        sysStr);
            }
            if (isEraGlonass)
            {
                SystemStandard = PA_ECALL_ERA_GLONASS;
            }
            else
            {
                SystemStandard = PA_ECALL_PAN_EUROPEAN;
            }
            LE_INFO("Selected standard is %s (%d)", sysStr, SystemStandard);
        }

        break;
    } while (false);

    le_cfg_CancelTxn(eCallCfg);

    if (res != LE_FAULT)
    {
        return GetPropulsionType();
    }
    else
    {
        return res;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function when the eCall settings are modified in the configuration tree.
 */
//--------------------------------------------------------------------------------------------------
static void SettingsUpdate
(
    void* contextPtr
)
{
    LE_INFO("eCall settings have changed!");
    LoadECallSettings();
}

//--------------------------------------------------------------------------------------------------
/**
 * Build an MSD from the eCall data object if needed, and load it into the Modem.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t LoadMsd
(
    ECall_t*   ecallPtr
)
{
    uint8_t outOptionalDataForEraGlonass[160]={0}; ///< 160 Bytes is guaranteed enough
                                                   ///< for the data part
    uint8_t oid[] = {1,4,1}; ///< OID version supported

    LE_FATAL_IF((ecallPtr == NULL), "ecallPtr is NULL !");

    if (ecallPtr->builtMsdSize <= 1)
    {
        LE_DEBUG("eCall MSD: VIN.%17s, version.%d, veh.%d",
                (char*)&ecallPtr->msd.msdMsg.msdStruct.vehIdentificationNumber.isowmi[0],
                ecallPtr->msd.version,
                (int)ecallPtr->msd.msdMsg.msdStruct.control.vehType);

        LE_DEBUG("eCall MSD: gasoline.%d, diesel.%d, gas.%d, propane.%d, electric.%d, hydrogen.%d",
                ecallPtr->msd.msdMsg.msdStruct.vehPropulsionStorageType.gasolineTankPresent,
                ecallPtr->msd.msdMsg.msdStruct.vehPropulsionStorageType.dieselTankPresent,
                ecallPtr->msd.msdMsg.msdStruct.vehPropulsionStorageType.compressedNaturalGas,
                ecallPtr->msd.msdMsg.msdStruct.vehPropulsionStorageType.liquidPropaneGas,
                ecallPtr->msd.msdMsg.msdStruct.vehPropulsionStorageType.electricEnergyStorage,
                ecallPtr->msd.msdMsg.msdStruct.vehPropulsionStorageType.hydrogenStorage);

        LE_DEBUG("eCall MSD: isPosTrusted.%d, lat.%d, long.%d, dir.%d",
                ecallPtr->msd.msdMsg.msdStruct.control.positionCanBeTrusted,
                ecallPtr->msd.msdMsg.msdStruct.vehLocation.latitude,
                ecallPtr->msd.msdMsg.msdStruct.vehLocation.longitude,
                ecallPtr->msd.msdMsg.msdStruct.vehDirection);

        LE_DEBUG("eCall MSD: isPax.%d, paxCount.%d",
                ecallPtr->msd.msdMsg.msdStruct.numberOfPassengersPres,
                ecallPtr->msd.msdMsg.msdStruct.numberOfPassengers);

        // Encode optional Data for ERA GLONASS if any
        if(SystemStandard == PA_ECALL_ERA_GLONASS)
        {
            ecallPtr->msd.msdMsg.optionalData.oid = oid;
            ecallPtr->msd.msdMsg.optionalData.oidlen = sizeof( oid );
            if ((ecallPtr->msd.msdMsg.optionalData.dataLen =
                msd_EncodeOptionalDataForEraGlonass(
                    &EraGlonassDataObj, outOptionalDataForEraGlonass))
                < 0)
            {
                LE_ERROR("Unable to encode optional Data for ERA GLONASS!");
                return LE_FAULT;
            }
            ecallPtr->msd.msdMsg.optionalDataPres = true;
            ecallPtr->msd.msdMsg.optionalData.data = outOptionalDataForEraGlonass;

            LE_DEBUG("eCall optional Data: Length %d",
                    ecallPtr->msd.msdMsg.optionalData.dataLen);
        }

        // Encode MSD message
        if ((ecallPtr->builtMsdSize = msd_EncodeMsdMessage(&ecallPtr->msd, ecallPtr->builtMsd))
            == LE_FAULT)
        {
            LE_ERROR("Unable to encode the MSD!");
            return LE_FAULT;
        }
    }
    else
    {
        LE_DEBUG("an MSD has been imported, no need to encode it.");
    }

    if (pa_ecall_LoadMsd(ecallPtr->builtMsd, ecallPtr->builtMsdSize) != LE_OK)
    {
        LE_ERROR("Unable to load the MSD!");
        return LE_FAULT;
    }
    else
    {
        return LE_OK;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * The first-layer eCall State Change Handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static void FirstLayerECallStateChangeHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    le_ecall_State_t*                 statePtr = reportPtr;
    le_ecall_StateChangeHandlerFunc_t clientHandlerFunc = secondLayerHandlerFunc;

    LE_DEBUG("First Layer Handler Function called with state %d", *statePtr);

    clientHandlerFunc(*statePtr, le_event_GetContextPtr());
}

//--------------------------------------------------------------------------------------------------
/**
 * Internal eCall State handler function.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ECallStateHandler
(
    le_ecall_State_t* statePtr
)
{
    LE_DEBUG("Handler Function called with state %d", *statePtr);

    if (*statePtr == LE_ECALL_STATE_COMPLETED)
    {
        IsSessionStopped = true;
        // Invalidate MSD
        memset(ECallObj.builtMsd, 0, sizeof(ECallObj.builtMsd));
        ECallObj.builtMsdSize = 0;
    }

    // Notify all the registered client's handlers
    le_event_Report(ECallStateId, statePtr, sizeof(le_ecall_State_t));
}



//--------------------------------------------------------------------------------------------------
//                                       Public declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the eCall service
 *
 * @note If the initialization failed, it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_ecall_Init
(
    void
)
{
    le_ecall_MsdTxMode_t msdTxMode;

    LE_INFO("le_ecall_Init called.");

    // Create an event Id for eCall State notification
    ECallStateId = le_event_CreateId("ECallState", sizeof(le_ecall_State_t));

    // Create the Safe Reference Map to use for eCall object Safe References.
    ECallRefMap = le_ref_CreateMap("ECallMap", ECALL_MAX);

    IsSessionStopped = true;

    // Initialize MSD structure
    ECallObj.psapNumber[0] = '\0';
    ECallObj.msd.msdMsg.msdStruct.vehIdentificationNumber.isowmi[0] = '\0';
    ECallObj.msd.msdMsg.msdStruct.vehIdentificationNumber.isovds[0] = '\0';
    ECallObj.msd.msdMsg.msdStruct.vehIdentificationNumber.isovisModelyear[0] ='\0';
    ECallObj.msd.msdMsg.msdStruct.vehIdentificationNumber.isovisSeqPlant[0] = '\0';
    ECallObj.msd.version = 1;
    ECallObj.isPushed = true;
    ECallObj.maxRedialAttempts = 10,
    ECallObj.msd.msdMsg.msdStruct.control.vehType = MSD_VEHICLE_PASSENGER_M1;
    ECallObj.msd.msdMsg.msdStruct.vehPropulsionStorageType.gasolineTankPresent = false;
    ECallObj.msd.msdMsg.msdStruct.vehPropulsionStorageType.dieselTankPresent = false;
    ECallObj.msd.msdMsg.msdStruct.vehPropulsionStorageType.compressedNaturalGas = false;
    ECallObj.msd.msdMsg.msdStruct.vehPropulsionStorageType.liquidPropaneGas = false;
    ECallObj.msd.msdMsg.msdStruct.vehPropulsionStorageType.electricEnergyStorage = false;
    ECallObj.msd.msdMsg.msdStruct.vehPropulsionStorageType.hydrogenStorage = false;

    // Retrieve the eCall settings from the configuration tree, including the static values of MSD
    if (LoadECallSettings() != LE_OK)
    {
        LE_ERROR("There are missing eCall settings, cannot perform eCall!");
    }

    if (pa_ecall_Init(SystemStandard) != LE_OK)
    {
        LE_ERROR("Cannot initialize Platform Adaptor for eCall, cannot perform eCall!");
        return;
    }

    // Add a config tree handler for eCall settings update.
    le_cfg_AddChangeHandler(CFG_MODEMSERVICE_ECALL_PATH, SettingsUpdate, NULL);

    LE_DEBUG("eCall settings: gasoline.%d, diesel.%d, gas.%d, propane.%d, electric.%d, hydrogen.%d",
             ECallObj.msd.msdMsg.msdStruct.vehPropulsionStorageType.gasolineTankPresent,
             ECallObj.msd.msdMsg.msdStruct.vehPropulsionStorageType.dieselTankPresent,
             ECallObj.msd.msdMsg.msdStruct.vehPropulsionStorageType.compressedNaturalGas,
             ECallObj.msd.msdMsg.msdStruct.vehPropulsionStorageType.liquidPropaneGas,
             ECallObj.msd.msdMsg.msdStruct.vehPropulsionStorageType.electricEnergyStorage,
             ECallObj.msd.msdMsg.msdStruct.vehPropulsionStorageType.hydrogenStorage);

    // Initialize the other members of the eCall object
    ECallObj.msd.msdMsg.msdStruct.messageIdentifier = 0;
    ECallObj.msd.msdMsg.msdStruct.timestamp = 0;
    ECallObj.msd.msdMsg.msdStruct.control.automaticActivation = true;
    ECallObj.msd.msdMsg.msdStruct.control.testCall = false;
    ECallObj.msd.msdMsg.msdStruct.control.positionCanBeTrusted = false;
    ECallObj.msd.msdMsg.msdStruct.numberOfPassengersPres = false;
    ECallObj.msd.msdMsg.msdStruct.numberOfPassengers = 0;
    ECallObj.state = LE_ECALL_STATE_COMPLETED;
    memset(ECallObj.builtMsd, 0, sizeof(ECallObj.builtMsd));
    ECallObj.builtMsdSize = 0;

    // Initialize the eCall ERA-GLONASS Data object
    memset(&EraGlonassDataObj, 0, sizeof(EraGlonassDataObj));
    EraGlonassDataObj.presentCrashSeverity = false;
    EraGlonassDataObj.presentDiagnosticResult = false;
    EraGlonassDataObj.presentCrashInfo = false;

    if (pa_ecall_GetMsdTxMode(&msdTxMode) != LE_OK)
    {
        LE_WARN("Unable to retrieve the configured Push/Pull mode!");
    }
    if (msdTxMode == LE_ECALL_TX_MODE_PULL)
    {
        ECallObj.isPushed = false;
    }
    else
    {
        ECallObj.isPushed = true;
    }

    LE_DEBUG("eCall settings: VIN.%17s, version.%d, isPushed.%d, maxRedialAttempts.%d, veh.%d",
             (char*)&ECallObj.msd.msdMsg.msdStruct.vehIdentificationNumber.isowmi[0],
             ECallObj.msd.version,
             ECallObj.isPushed,
             ECallObj.maxRedialAttempts,
             (int)ECallObj.msd.msdMsg.msdStruct.control.vehType);

    // Register a handler function for eCall state indications
    LE_FATAL_IF((pa_ecall_AddEventHandler(ECallStateHandler) == NULL),
                "Add pa_ecall_AddEventHandler failed");
}

//--------------------------------------------------------------------------------------------------
/**
 * This function configures the eCall operation mode to eCall only, only emergency number can be
 * used to start an eCall session. The modem does not try to register on the Cellular network.
 * This function forces the modem to behave as eCall only mode whatever U/SIM operation mode.
 * The change does not persist over power cycles.
 * This function can be called before making an eCall.
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT for other failures
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_ForceOnlyMode
(
    void
)
{
    return (pa_ecall_SetOperationMode(LE_ECALL_FORCED_ONLY_MODE));
}

//--------------------------------------------------------------------------------------------------
/**
 * Same as le_ecall_ForceOnlyMode(), but the change persists over power cycles.
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT for other failures
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_ForcePersistentOnlyMode
(
    void
)
{
    return (pa_ecall_SetOperationMode(LE_ECALL_FORCED_PERSISTENT_ONLY_MODE));
}

//--------------------------------------------------------------------------------------------------
/**
 * This function exits from eCall Only mode. It configures the eCall operation mode to Normal mode,
 * the modem uses the default operation mode at power up (or after U/SIM hotswap). The modem behaves
 * following the U/SIM eCall operation mode; for example the U/SIM can be configured only for eCall,
 * or a combination of eCall and commercial service provision.
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT for other failures
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_ExitOnlyMode
(
    void
)
{
    return (pa_ecall_SetOperationMode(LE_ECALL_NORMAL_MODE));
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the configured Operation mode.
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT for other failures
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_GetConfiguredOperationMode
(
    le_ecall_OpMode_t *opModePtr    ///< [OUT] Operation mode
)
{
    return (pa_ecall_GetOperationMode(opModePtr));
}

//--------------------------------------------------------------------------------------------------
/**
 * Create a new eCall object
 *
 * The eCall is not actually established at this point. It is still up to the caller to call
 * le_ecall_Start() when ready.
 *
 * @return A reference to the new Call object.
 *
 * @note On failure, the process exits, so you don't have to worry about checking the returned
 *       reference for validity.
 */
//--------------------------------------------------------------------------------------------------
le_ecall_CallRef_t le_ecall_Create
(
    void
)
{
    // Create a new Safe Reference for the Call object.
    return ((le_ecall_CallRef_t)le_ref_CreateRef(ECallRefMap, &ECallObj));
}

//--------------------------------------------------------------------------------------------------
/**
 * Call to free up a call reference.
 *
 * @note This will free the reference, but not necessarily stop an active eCall. If there are
 *       other holders of this reference then the eCall will remain active.
 */
//--------------------------------------------------------------------------------------------------
void le_ecall_Delete
(
    le_ecall_CallRef_t ecallRef     ///< [IN] eCall reference
)
{
    ECall_t*   ecallPtr = le_ref_Lookup(ECallRefMap, ecallRef);
    if (ecallPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ecallRef);
        return;
    }

    // Invalidate the Safe Reference.
    le_ref_DeleteRef(ECallRefMap, ecallRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the position transmitted by the MSD.
 *
 * The MSD is not actually transferred at this point. It is still up to the caller to call
 * le_ecall_LoadMsd() when the MSD is fully built with the le_ecall_SetMsdXxx() functions.
 *
 * @return
 *      - LE_OK on success
 *      - LE_DUPLICATE an MSD has been already imported
 *      - LE_BAD_PARAMETER bad eCall reference
 *
 * @note The process exits, if an invalid eCall reference is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_SetMsdPosition
(
    le_ecall_CallRef_t  ecallRef,   ///< [IN] eCall reference
    bool                isTrusted,  ///< [IN] True if the position can be trusted, false otherwise
    int32_t             latitude,   ///< [IN] The latitude in degrees with 6 decimal places
    int32_t             longitude,  ///< [IN] The longitude in degrees with 6 decimal places
    int32_t             direction   ///< [IN] The direction of the vehicle in degrees (where 0 is
                                    ///       True North).
)
{
    ECall_t*   ecallPtr = le_ref_Lookup(ECallRefMap, ecallRef);
    if (ecallPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ecallRef);
        return LE_BAD_PARAMETER;
    }

    if (ecallPtr->builtMsdSize > 1)
    {
        LE_ERROR("An MSD has been already imported!");
        return LE_DUPLICATE;
    }

    ecallPtr->msd.msdMsg.msdStruct.control.positionCanBeTrusted = isTrusted;
    ecallPtr->msd.msdMsg.msdStruct.vehLocation.latitude = ConvertDdToDms(latitude);
    ecallPtr->msd.msdMsg.msdStruct.vehLocation.longitude = ConvertDdToDms(longitude);
    ecallPtr->msd.msdMsg.msdStruct.vehDirection = direction;

    // Set to 1 to avoid MSD overwriting with 'ImportMSD'
    ecallPtr->builtMsdSize = 1;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the number of passengers transmitted by the MSD.
 *
 * The MSD is not actually transferred at this point. It is still up to the caller to call
 * le_ecall_LoadMsd() when the MSD is fully built with the le_ecall_SetMsdXxx() functions.
 *
 * @return
 *      - LE_OK on success
 *      - LE_DUPLICATE an MSD has been already imported
 *      - LE_BAD_PARAMETER bad eCall reference
 *
 * @note The process exits, if an invalid eCall reference is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_SetMsdPassengersCount
(
    le_ecall_CallRef_t  ecallRef,   ///< [IN] eCall reference
    uint32_t            paxCount    ///< [IN] number of passengers
)
{
    ECall_t*   ecallPtr = le_ref_Lookup(ECallRefMap, ecallRef);
    if (ecallPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ecallRef);
        return LE_BAD_PARAMETER;
    }

    if (ecallPtr->builtMsdSize > 1)
    {
        LE_ERROR("An MSD has been already imported!");
        return LE_DUPLICATE;
    }

    ecallPtr->msd.msdMsg.msdStruct.numberOfPassengersPres = true;
    ecallPtr->msd.msdMsg.msdStruct.numberOfPassengers = paxCount;

    // Set to 1 to avoid MSD overwriting with 'ImportMSD'
    ecallPtr->builtMsdSize = 1;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Import an already prepared MSD.
 *
 * The MSD is not actually transferred at this point, this functions only creates a new MSD object.
 * It is still up to the caller to call le_ecall_LoadMsd().
 *
 * @return
 *      - LE_OK on success
 *      - LE_DUPLICATE an MSD has been already imported
 *      - LE_BAD_PARAMETER bad eCall reference
 *      - LE_FAULT for other failures
 *
 * @note If MSD code is too long (max MSD_MAX_LEN bytes) fatal errors is logged by
 * killing the calling
 *       process after logging the message at EMERGENCY.
 *
* @note On failure, the process exits, so you don't have to worry about checking the returned
 *       reference for validity.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_ImportMsd
(
    le_ecall_CallRef_t  ecallRef,      ///< [IN] eCall reference
    const uint8_t*      msdPtr,        ///< [IN] the prepared MSD
    size_t              msdNumElements ///< [IN] the prepared MSD size in bytes
)
{
    ECall_t*   ecallPtr = le_ref_Lookup(ECallRefMap, ecallRef);
    if (ecallPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ecallRef);
        return LE_BAD_PARAMETER;
    }

    LE_FATAL_IF((msdNumElements > sizeof(ecallPtr->builtMsd)),
                "Imported MSD is too long (%zd > %zd)",
                msdNumElements,
                sizeof(ecallPtr->builtMsd));

    if (ecallPtr->builtMsdSize > 0)
    {
        LE_ERROR("An MSD has been already imported!");
        return LE_DUPLICATE;
    }

    memcpy(ecallPtr->builtMsd, msdPtr, msdNumElements);
    ecallPtr->builtMsdSize = msdNumElements;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Export the encoded MSD.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW  The encoded MSD length exceeds the user's buffer length.
 *      - LE_NOT_FOUND  No encoded MSD is available.
 *      - LE_BAD_PARAMETER bad eCall reference
 *      - LE_FAULT for other failures
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_ExportMsd
(
    le_ecall_CallRef_t  ecallRef,           ///< [IN] eCall reference
    uint8_t*            msdPtr,             ///< [OUT] The encoded MSD message.
    size_t*             msdNumElementsPtr   ///< [IN,OUT] The encoded MSD size in bytes
)
{
    ECall_t*   ecallPtr = le_ref_Lookup(ECallRefMap, ecallRef);
    if (ecallPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ecallRef);
        return LE_BAD_PARAMETER;
    }

    LE_FATAL_IF((sizeof(ecallPtr->builtMsd) > *msdNumElementsPtr),
                "Encoded MSD is too long for your buffer (%zd > %zd)!",
                sizeof(ecallPtr->builtMsd),
                *msdNumElementsPtr);

    if (ecallPtr->builtMsdSize == 0)
    {
        LE_WARN("MSD is not yet encoded, try to encode it.");
        if ((ecallPtr->builtMsdSize = msd_EncodeMsdMessage(&ecallPtr->msd,
                                                           ecallPtr->builtMsd)) == LE_FAULT)
        {
            LE_ERROR("Unable to encode the MSD!");
            return LE_NOT_FOUND;
        }
    }

    memcpy(msdPtr, ecallPtr->builtMsd , *msdNumElementsPtr);
    *msdNumElementsPtr = ecallPtr->builtMsdSize;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Start an automatic eCall session
 *
 * @return
 *      - LE_OK on success
 *      - LE_BUSY an eCall session is already in progress
 *      - LE_BAD_PARAMETER bad eCall reference
 *      - LE_FAULT for other failures
 *
 * @note The process exits, if an invalid eCall reference is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_StartAutomatic
(
    le_ecall_CallRef_t    ecallRef   ///< [IN] eCall reference
)
{
    ECall_t*   ecallPtr = le_ref_Lookup(ECallRefMap, ecallRef);
    if (ecallPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ecallRef);
        return LE_BAD_PARAMETER;
    }

    if (!IsSessionStopped)
    {
        LE_ERROR("An eCall session is already in progress");
        return LE_BUSY;
    }

    // Hang up all the ongoing calls using the communication channel required for eCall
    if (le_mcc_call_HangUpAll() != LE_OK)
    {
        LE_ERROR("Hang up ongoing call(s) failed");
        return LE_FAULT;
    }

    ecallPtr->msd.msdMsg.msdStruct.messageIdentifier++;
    ecallPtr->msd.msdMsg.msdStruct.timestamp =
    ecallPtr->msd.msdMsg.msdStruct.control.automaticActivation = true;
    ecallPtr->msd.msdMsg.msdStruct.control.testCall = false;

    if (LoadMsd(ecallPtr) != LE_OK)
    {
        LE_ERROR("The MSD is not valid!");
        return LE_FAULT;
    }
    else
    {
        IsSessionStopped = false;
        return pa_ecall_StartAutomatic();
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Start a manual eCall session
 *
 * @return
 *      - LE_OK on success
 *      - LE_BUSY an eCall session is already in progress
 *      - LE_BAD_PARAMETER bad eCall reference
 *      - LE_FAULT for other failures
 *
 * @note The process exits, if an invalid eCall reference is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_StartManual
(
    le_ecall_CallRef_t    ecallRef   ///< [IN] eCall reference
)
{
    ECall_t*   ecallPtr = le_ref_Lookup(ECallRefMap, ecallRef);
    if (ecallPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ecallRef);
        return LE_BAD_PARAMETER;
    }

    if (!IsSessionStopped)
    {
        LE_ERROR("An eCall session is already in progress");
        return LE_BUSY;
    }

    // Hang up all the ongoing calls using the communication channel required for eCall
    if (le_mcc_call_HangUpAll() != LE_OK)
    {
        LE_ERROR("Hang up ongoing call(s) failed");
        return LE_FAULT;
    }

    ecallPtr->msd.msdMsg.msdStruct.messageIdentifier++;
    ecallPtr->msd.msdMsg.msdStruct.timestamp =
    ecallPtr->msd.msdMsg.msdStruct.control.automaticActivation = false;
    ecallPtr->msd.msdMsg.msdStruct.control.testCall = false;

    if (LoadMsd(ecallPtr) != LE_OK)
    {
        LE_ERROR("The MSD is not valid!");
        return LE_FAULT;
    }
    else
    {
        IsSessionStopped = false;
        return pa_ecall_StartManual();
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Start a test eCall session
 *
 * @return
 *       - LE_OK on success
 *       - LE_BUSY an eCall session is already in progress
 *       - LE_BAD_PARAMETER bad eCall reference
 *       - LE_FAULT for other failures
 *
 * @note The process exits, if an invalid eCall reference is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_StartTest
(
    le_ecall_CallRef_t    ecallRef   ///< [IN] eCall reference
)
{
    ECall_t*   ecallPtr = le_ref_Lookup(ECallRefMap, ecallRef);
    if (ecallPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ecallRef);
        return LE_BAD_PARAMETER;
    }

    if (!IsSessionStopped)
    {
        LE_ERROR("An eCall session is already in progress");
        return LE_BUSY;
    }

    // Hang up all the ongoing calls using the communication channel required for eCall
    if (le_mcc_call_HangUpAll() != LE_OK)
    {
        LE_ERROR("Hang up ongoing call(s) failed");
        return LE_FAULT;
    }

    ecallPtr->msd.msdMsg.msdStruct.messageIdentifier++;
    ecallPtr->msd.msdMsg.msdStruct.timestamp =
    ecallPtr->msd.msdMsg.msdStruct.control.automaticActivation = false;
    ecallPtr->msd.msdMsg.msdStruct.control.testCall = true;

    if (LoadMsd(ecallPtr) != LE_OK)
    {
        LE_ERROR("The MSD is not valid!");
        return LE_FAULT;
    }
    else
    {
        IsSessionStopped = false;
        return pa_ecall_StartTest();
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * End the current eCall session
 *
 * @return
 *      - LE_OK on success
 *      - LE_BUSY an eCall session is still in progress
 *      - LE_BAD_PARAMETER bad eCall reference
 *      - LE_FAULT for other failures
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_End
(
    le_ecall_CallRef_t    ecallRef   ///< [IN] eCall reference
)
{
    ECall_t*   ecallPtr = le_ref_Lookup(ECallRefMap, ecallRef);
    if (ecallPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ecallRef);
        return LE_BAD_PARAMETER;
    }

    if (ecallPtr->state != LE_ECALL_STATE_COMPLETED)
    {
        LE_ERROR("An eCall session is still in progress");
        return LE_BUSY;
    }

    if (!(ecallPtr->msd.msdMsg.msdStruct.control.testCall) &&
         (ecallPtr->msd.msdMsg.msdStruct.control.automaticActivation))
    {
        LE_ERROR("An automatic eCall session cannot be ended!");
        return LE_FAULT;
    }

    // Invalidate MSD
    memset(ecallPtr->builtMsd, 0, sizeof(ecallPtr->builtMsd));
    ecallPtr->builtMsdSize = 0;

    IsSessionStopped = true;
    return (pa_ecall_End());
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the current state for the given eCall
 *
 * @return the current state for the given eCall
 *
 * @note The process exits, if an invalid eCall reference is given
 */
//--------------------------------------------------------------------------------------------------
le_ecall_State_t le_ecall_GetState
(
    le_ecall_CallRef_t      ecallRef
)
{
    ECall_t*   ecallPtr = le_ref_Lookup(ECallRefMap, ecallRef);
    if (ecallPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ecallRef);
        return LE_ECALL_STATE_UNKNOWN;
    }

    return (ecallPtr->state);
}

//--------------------------------------------------------------------------------------------------
/**
 * le_ecall_StateChangeHandler handler ADD function
 */
//--------------------------------------------------------------------------------------------------
le_ecall_StateChangeHandlerRef_t le_ecall_AddStateChangeHandler
(
    le_ecall_StateChangeHandlerFunc_t handlerPtr, ///< [IN] Handler function
    void*                             contextPtr      ///< [IN] Context pointer
)
{
    le_event_HandlerRef_t        handlerRef;

    if (handlerPtr == NULL)
    {
        LE_KILL_CLIENT("Handler function is NULL !");
        return NULL;
    }

    handlerRef = le_event_AddLayeredHandler("ECallStateHandler",
                                            ECallStateId,
                                            FirstLayerECallStateChangeHandler,
                                            (le_event_HandlerFunc_t)handlerPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (le_ecall_StateChangeHandlerRef_t)(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * le_ecall_StateChangeHandler handler REMOVE function
 */
//--------------------------------------------------------------------------------------------------
void le_ecall_RemoveStateChangeHandler
(
    le_ecall_StateChangeHandlerRef_t addHandlerRef ///< [IN] The handler reference
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)addHandlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the Public Safely Answering Point telephone number.
 *
 * @note Important! This function doesn't modified the U/SIM content.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT for other failures
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_SetPsapNumber
(
    const char* psapPtr    ///< [IN] Public Safely Answering Point number
)
{
    le_result_t res;

    if (psapPtr == NULL)
    {
        LE_KILL_CLIENT("psapPtr is NULL !");
        return LE_FAULT;
    }

    if(strlen(psapPtr) > LE_MDMDEFS_PHONE_NUM_MAX_LEN)
    {
        LE_KILL_CLIENT( "strlen(psapPtr) > %d", LE_MDMDEFS_PHONE_NUM_MAX_LEN);
        return LE_FAULT;
    }

    if(!strlen(psapPtr))
    {
        return LE_BAD_PARAMETER;
    }

    if((res = le_utf8_Copy(ECallObj.psapNumber,
                           psapPtr,
                           sizeof(ECallObj.psapNumber),
                           NULL)) != LE_OK)
    {
        return res;
    }

    if (pa_ecall_SetPsapNumber(ECallObj.psapNumber) != LE_OK)
    {
        LE_ERROR("Unable to set the desired PSAP number!");
        return LE_FAULT;
    }
    else
    {
        return LE_OK;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the Public Safely Answering Point telephone number set with le_ecall_SetPsapNumber()
 * function.
 *
 * @note Important! This function doesn't read the U/SIM content.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT for other failures
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_GetPsapNumber
(
    char   *psapPtr,           ///< [OUT] Public Safely Answering Point number
    size_t  len  ///< [IN] The maximum length of PSAP number.
)
{
    if (psapPtr == NULL)
    {
        LE_KILL_CLIENT("psapPtr is NULL !");
        return LE_FAULT;
    }

    return (pa_ecall_GetPsapNumber(psapPtr, len));
}


//--------------------------------------------------------------------------------------------------
/**
 * This function can be recalled to indicate the modem to read the number to dial from the FDN/SDN
 * of the U/SIM, depending upon the eCall operating mode.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT for other failures
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_UseUSimNumbers
(
    void
)
{
    return (pa_ecall_UseUSimNumbers());
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the NAD_DEREGISTRATION_TIME value. After termination of an emergency call the in-vehicle
 * system remains registered on the network for the period of time, defined by the installation
 * parameter NAD_DEREGISTRATION_TIME.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_SetNadDeregistrationTime
(
    uint16_t    deregTime  ///< [IN] the NAD_DEREGISTRATION_TIME value (in minutes).
)
{
    return pa_ecall_SetNadDeregistrationTime(deregTime);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the NAD_DEREGISTRATION_TIME value.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_GetNadDeregistrationTime
(
    uint16_t*    deregTimePtr  ///< [OUT] the NAD_DEREGISTRATION_TIME value (in minutes).
)
{
    return pa_ecall_GetNadDeregistrationTime(deregTimePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the push/pull transmission mode.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT for other failures
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_SetMsdTxMode
(
    le_ecall_MsdTxMode_t mode   ///< [IN] Transmission mode
)
{
    return (pa_ecall_SetMsdTxMode(mode));
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the push/pull transmission mode.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT for other failures
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_GetMsdTxMode
(
    le_ecall_MsdTxMode_t* modePtr   ///< [OUT] Transmission mode
)
{
    return (pa_ecall_GetMsdTxMode(modePtr));
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the minimum interval value between dial attempts.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT for other failures
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_SetIntervalBetweenDialAttempts
(
    uint16_t    pause   ///< [IN] the minimum interval value in seconds
)
{
    return pa_ecall_SetIntervalBetweenDialAttempts(pause);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the minimum interval value between dial attempts.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT for other failures
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_GetIntervalBetweenDialAttempts
(
     uint16_t*    pausePtr   ///< [OUT] the minimum interval value in seconds
)
{
    return pa_ecall_GetIntervalBetweenDialAttempts(pausePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the MANUAL_DIAL_ATTEMPTS value. If a dial attempt under manual emergency call initiation
 * failed, it should be repeated maximally ECALL_MANUAL_DIAL_ATTEMPTS-1 times within the maximal
 * time limit of ECALL_DIAL_DURATION. The default value is 10.
 * Redial attempts stop once the call has been cleared down correctly, or if counter/timer reached
 * their limits.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_SetEraGlonassManualDialAttempts
(
    uint16_t    attempts  ///< [IN] the MANUAL_DIAL_ATTEMPTS value
)
{
    return pa_ecall_SetEraGlonassManualDialAttempts(attempts);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set tthe AUTO_DIAL_ATTEMPTS value. If a dial attempt under automatic emergency call initiation
 * failed, it should be repeated maximally ECALL_AUTO_DIAL_ATTEMPTS-1 times within the maximal time
 * limit of ECALL_DIAL_DURATION. The default value is 10.
 * Redial attempts stop once the call has been cleared down correctly, or if counter/timer reached
 * their limits.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_SetEraGlonassAutoDialAttempts
(
    uint16_t    attempts  ///< [IN] the AUTO_DIAL_ATTEMPTS value
)
{
    return pa_ecall_SetEraGlonassAutoDialAttempts(attempts);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the ECALL_DIAL_DURATION time. It is the maximum time the IVS have to connect the emergency
 * call to the PSAP, including all redial attempts.
 * If the call is not connected within this time (or ManualDialAttempts/AutoDialAttempts dial
 * attempts), it will stop.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_SetEraGlonassDialDuration
(
    uint16_t    duration   ///< [IN] the ECALL_DIAL_DURATION time value (in seconds)
)
{
    return pa_ecall_SetEraGlonassDialDuration(duration);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the MANUAL_DIAL_ATTEMPTS value.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_GetEraGlonassManualDialAttempts
(
    uint16_t*    attemptsPtr  ///< [OUT] the MANUAL_DIAL_ATTEMPTS value
)
{
    if (attemptsPtr == NULL)
    {
        LE_KILL_CLIENT("attemptsPtr is NULL !");
        return LE_FAULT;
    }

    return pa_ecall_GetEraGlonassManualDialAttempts(attemptsPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the AUTO_DIAL_ATTEMPTS value.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_GetEraGlonassAutoDialAttempts
(
    uint16_t*    attemptsPtr  ///< [OUT] the AUTO_DIAL_ATTEMPTS value
)
{
    if (attemptsPtr == NULL)
    {
        LE_KILL_CLIENT("attemptsPtr is NULL !");
        return LE_FAULT;
    }

    return pa_ecall_GetEraGlonassAutoDialAttempts(attemptsPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the ECALL_DIAL_DURATION time.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_GetEraGlonassDialDuration
(
    uint16_t*    durationPtr  ///< [OUT] the ECALL_DIAL_DURATION time value (in seconds)
)
{
    if (durationPtr == NULL)
    {
        LE_KILL_CLIENT("durationPtr is NULL !");
        return LE_FAULT;
    }

    return pa_ecall_GetEraGlonassDialDuration(durationPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the ERA-GLONASS crash severity parameter
 *
 * @return
 *  - LE_OK on success
 *  - LE_DUPLICATE an MSD has been already imported
 *  - LE_BAD_PARAMETER bad eCall reference
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_SetMsdEraGlonassCrashSeverity
(
    le_ecall_CallRef_t  ecallRef,       ///< [IN] eCall reference
    uint32_t            crashSeverity   ///< [IN] the ERA-GLONASS crash severity parameter
)
{
    ECall_t*   ecallPtr = le_ref_Lookup(ECallRefMap, ecallRef);
    if (ecallPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ecallRef);
        return LE_BAD_PARAMETER;
    }

    if (ecallPtr->builtMsdSize > 1)
    {
        LE_ERROR("An MSD has been already imported!");
        return LE_DUPLICATE;
    }

    EraGlonassDataObj.presentCrashSeverity = true;
    EraGlonassDataObj.crashSeverity = crashSeverity;

    // Set to 1 to avoid MSD overwriting with 'ImportMSD'
    ecallPtr->builtMsdSize = 1;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Reset the ERA-GLONASS crash severity parameter. Therefore that optional parameter is not included
 * in the MSD message.
 *
 * @return
 *  - LE_OK on success
 *  - LE_DUPLICATE an MSD has been already imported
 *  - LE_BAD_PARAMETER bad eCall reference
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_ResetMsdEraGlonassCrashSeverity
(
    le_ecall_CallRef_t   ecallRef       ///< [IN] eCall reference

)
{
    ECall_t*   ecallPtr = le_ref_Lookup(ECallRefMap, ecallRef);
    if (ecallPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ecallRef);
        return LE_BAD_PARAMETER;
    }

    if (ecallPtr->builtMsdSize > 1)
    {
        LE_ERROR("An MSD has been already imported!");
        return LE_DUPLICATE;
    }

    EraGlonassDataObj.presentCrashSeverity = false;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the ERA-GLONASS diagnostic result using a bit mask.
 *
 * @return
 *  - LE_OK on success
 *  - LE_DUPLICATE an MSD has been already imported
 *  - LE_BAD_PARAMETER bad eCall reference
 */
//--------------------------------------------------------------------------------------------------

le_result_t le_ecall_SetMsdEraGlonassDiagnosticResult
(
    le_ecall_CallRef_t                 ecallRef,             ///< [IN] eCall reference
    le_ecall_DiagnosticResultBitMask_t diagnosticResultMask  ///< ERA-GLONASS diagnostic
                                                             ///< result bit mask.
)
{
    ECall_t*   ecallPtr = le_ref_Lookup(ECallRefMap, ecallRef);
    if (ecallPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ecallRef);
        return LE_BAD_PARAMETER;
    }

    if (ecallPtr->builtMsdSize > 1)
    {
        LE_ERROR("An MSD has been already imported!");
        return LE_DUPLICATE;
    }

    LE_DEBUG("DiagnosticResult mask 0x%016"PRIX64, (uint64_t)diagnosticResultMask);
    EraGlonassDataObj.presentDiagnosticResult = true;

    EraGlonassDataObj.diagnosticResult.presentMicConnectionFailure =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_PRESENT_MIC_CONNECTION_FAILURE);
    EraGlonassDataObj.diagnosticResult.micConnectionFailure =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_MIC_CONNECTION_FAILURE);
    EraGlonassDataObj.diagnosticResult.presentMicFailure =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_PRESENT_MIC_FAILURE);
    EraGlonassDataObj.diagnosticResult.micFailure =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_MIC_FAILURE);
    EraGlonassDataObj.diagnosticResult.presentRightSpeakerFailure =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_PRESENT_RIGHT_SPEAKER_FAILURE);
    EraGlonassDataObj.diagnosticResult.rightSpeakerFailure =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_RIGHT_SPEAKER_FAILURE);
    EraGlonassDataObj.diagnosticResult.presentLeftSpeakerFailure =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_PRESENT_LEFT_SPEAKER_FAILURE);
    EraGlonassDataObj.diagnosticResult.leftSpeakerFailure =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_LEFT_SPEAKER_FAILURE);
    EraGlonassDataObj.diagnosticResult.presentSpeakersFailure =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_PRESENT_SPEAKERS_FAILURE);
    EraGlonassDataObj.diagnosticResult.speakersFailure =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_SPEAKERS_FAILURE);
    EraGlonassDataObj.diagnosticResult.presentIgnitionLineFailure =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_PRESENT_IGNITION_LINE_FAILURE);
    EraGlonassDataObj.diagnosticResult.ignitionLineFailure =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_IGNITION_LINE_FAILURE);
    EraGlonassDataObj.diagnosticResult.presentUimFailure =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_PRESENT_UIM_FAILURE);
    EraGlonassDataObj.diagnosticResult.uimFailure =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_UIM_FAILURE);
    EraGlonassDataObj.diagnosticResult.presentStatusIndicatorFailure =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_PRESENT_STATUS_INDICATOR_FAILURE);
    EraGlonassDataObj.diagnosticResult.statusIndicatorFailure =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_STATUS_INDICATOR_FAILURE);
    EraGlonassDataObj.diagnosticResult.presentBatteryFailure =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_PRESENT_BATTERY_FAILURE);
    EraGlonassDataObj.diagnosticResult.batteryFailure =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_BATTERY_FAILURE);
    EraGlonassDataObj.diagnosticResult.presentBatteryVoltageLow =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_PRESENT_BATTERY_VOLTAGE_LOW);
    EraGlonassDataObj.diagnosticResult.batteryVoltageLow =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_BATTERY_VOLTAGE_LOW);
    EraGlonassDataObj.diagnosticResult.presentCrashSensorFailure =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_PRESENT_CRASH_SENSOR_FAILURE);
    EraGlonassDataObj.diagnosticResult.crashSensorFailure =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_CRASH_SENSOR_FAILURE);
    EraGlonassDataObj.diagnosticResult.presentFirmwareImageCorruption =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_PRESENT_FIRMWARE_IMAGE_CORRUPTION);
    EraGlonassDataObj.diagnosticResult.firmwareImageCorruption =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_FIRMWARE_IMAGE_CORRUPTION);
    EraGlonassDataObj.diagnosticResult.presentCommModuleInterfaceFailure =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_PRESENT_COMM_MODULE_INTERFACE_FAILURE);
    EraGlonassDataObj.diagnosticResult.commModuleInterfaceFailure =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_COMM_MODULE_INTERFACE_FAILURE);
    EraGlonassDataObj.diagnosticResult.presentGnssReceiverFailure =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_PRESENT_GNSS_RECEIVER_FAILURE);
    EraGlonassDataObj.diagnosticResult.gnssReceiverFailure =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_GNSS_RECEIVER_FAILURE);
    EraGlonassDataObj.diagnosticResult.presentRaimProblem =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_PRESENT_RAIM_PROBLEM);
    EraGlonassDataObj.diagnosticResult.raimProblem =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_RAIM_PROBLEM);
    EraGlonassDataObj.diagnosticResult.presentGnssAntennaFailure =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_PRESENT_GNSS_ANTENNA_FAILURE);
    EraGlonassDataObj.diagnosticResult.gnssAntennaFailure =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_GNSS_ANTENNA_FAILURE);
    EraGlonassDataObj.diagnosticResult.presentCommModuleFailure =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_PRESENT_COMM_MODULE_FAILURE);
    EraGlonassDataObj.diagnosticResult.commModuleFailure =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_COMM_MODULE_FAILURE);
    EraGlonassDataObj.diagnosticResult.presentEventsMemoryOverflow =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_PRESENT_EVENTS_MEMORY_OVERFLOW);
    EraGlonassDataObj.diagnosticResult.eventsMemoryOverflow =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_EVENTS_MEMORY_OVERFLOW);
    EraGlonassDataObj.diagnosticResult.presentCrashProfileMemoryOverflow =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_PRESENT_CRASH_PROFILE_MEMORY_OVERFLOW);
    EraGlonassDataObj.diagnosticResult.crashProfileMemoryOverflow =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_CRASH_PROFILE_MEMORY_OVERFLOW);
    EraGlonassDataObj.diagnosticResult.presentOtherCriticalFailures =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_PRESENT_OTHER_CRITICAL_FAILURES);
    EraGlonassDataObj.diagnosticResult.otherCriticalFailures =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_OTHER_CRITICAL_FAILURES);
    EraGlonassDataObj.diagnosticResult.presentOtherNotCriticalFailures =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_PRESENT_OTHER_NOT_CRITICAL_FAILURES);
    EraGlonassDataObj.diagnosticResult.otherNotCriticalFailures =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_OTHER_NOT_CRITICAL_FAILURES);

    // Set to 1 to avoid MSD overwriting with 'ImportMSD'
    ecallPtr->builtMsdSize = 1;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Reset the ERA-GLONASS diagnostic result bit mask. Therefore that optional parameter is
 * not included in the MSD message.
 *
 * @return
 *  - LE_OK on success
 *  - LE_DUPLICATE an MSD has been already imported
 *  - LE_BAD_PARAMETER bad eCall reference
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_ResetMsdEraGlonassDiagnosticResult
(
    le_ecall_CallRef_t   ecallRef       ///< [IN] eCall reference
)
{
    ECall_t*   ecallPtr = le_ref_Lookup(ECallRefMap, ecallRef);
    if (ecallPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ecallRef);
        return LE_BAD_PARAMETER;
    }

    if (ecallPtr->builtMsdSize > 1)
    {
        LE_ERROR("An MSD has been already imported!");
        return LE_DUPLICATE;
    }

    LE_DEBUG("DiagnosticResult mask reseted");
    EraGlonassDataObj.presentDiagnosticResult = false;

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the ERA-GLONASS crash type bit mask
 *
 * @return
 *  - LE_OK on success
 *  - LE_DUPLICATE an MSD has been already imported
 *  - LE_BAD_PARAMETER bad eCall reference
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_SetMsdEraGlonassCrashInfo
(
    le_ecall_CallRef_t          ecallRef,       ///< [IN] eCall reference
    le_ecall_CrashInfoBitMask_t crashInfoMask   ///< ERA-GLONASS crash type bit mask.
)
{
    ECall_t*   ecallPtr = le_ref_Lookup(ECallRefMap, ecallRef);
    if (ecallPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ecallRef);
        return LE_BAD_PARAMETER;
    }

    if (ecallPtr->builtMsdSize > 1)
    {
        LE_ERROR("An MSD has been already imported!");
        return LE_DUPLICATE;
    }

    LE_DEBUG("CrashInfo mask %08X", (uint16_t)crashInfoMask);

    EraGlonassDataObj.presentCrashInfo = true;

    EraGlonassDataObj.crashType.presentCrashFront =
        GET_BIT_MASK_VALUE(crashInfoMask, LE_ECALL_CRASH_INFO_PRESENT_CRASH_FRONT);
    EraGlonassDataObj.crashType.crashFront =
        GET_BIT_MASK_VALUE(crashInfoMask, LE_ECALL_CRASH_INFO_CRASH_FRONT);
    EraGlonassDataObj.crashType.presentCrashLeft =
        GET_BIT_MASK_VALUE(crashInfoMask, LE_ECALL_CRASH_INFO_PRESENT_CRASH_LEFT);
    EraGlonassDataObj.crashType.crashLeft =
        GET_BIT_MASK_VALUE(crashInfoMask, LE_ECALL_CRASH_INFO_CRASH_LEFT);
    EraGlonassDataObj.crashType.presentCrashRight =
        GET_BIT_MASK_VALUE(crashInfoMask, LE_ECALL_CRASH_INFO_PRESENT_CRASH_RIGHT);
    EraGlonassDataObj.crashType.crashRight =
        GET_BIT_MASK_VALUE(crashInfoMask, LE_ECALL_CRASH_INFO_CRASH_RIGHT);
    EraGlonassDataObj.crashType.presentCrashRear =
        GET_BIT_MASK_VALUE(crashInfoMask, LE_ECALL_CRASH_INFO_PRESENT_CRASH_REAR);
    EraGlonassDataObj.crashType.crashRear =
        GET_BIT_MASK_VALUE(crashInfoMask, LE_ECALL_CRASH_INFO_CRASH_REAR);
    EraGlonassDataObj.crashType.presentCrashRollover =
        GET_BIT_MASK_VALUE(crashInfoMask, LE_ECALL_CRASH_INFO_PRESENT_CRASH_ROLLOVER);
    EraGlonassDataObj.crashType.crashRollover =
        GET_BIT_MASK_VALUE(crashInfoMask, LE_ECALL_CRASH_INFO_CRASH_ROLLOVER);
    EraGlonassDataObj.crashType.presentCrashSide =
        GET_BIT_MASK_VALUE(crashInfoMask, LE_ECALL_CRASH_INFO_PRESENT_CRASH_SIDE);
    EraGlonassDataObj.crashType.crashSide =
        GET_BIT_MASK_VALUE(crashInfoMask, LE_ECALL_CRASH_INFO_CRASH_SIDE);
    EraGlonassDataObj.crashType.presentCrashFrontOrSide =
        GET_BIT_MASK_VALUE(crashInfoMask, LE_ECALL_CRASH_INFO_PRESENT_CRASH_FRONT_OR_SIDE);
    EraGlonassDataObj.crashType.crashFrontOrSide =
        GET_BIT_MASK_VALUE(crashInfoMask, LE_ECALL_CRASH_INFO_CRASH_FRONT_OR_SIDE);
    EraGlonassDataObj.crashType.presentCrashAnotherType =
        GET_BIT_MASK_VALUE(crashInfoMask, LE_ECALL_CRASH_INFO_PRESENT_CRASH_ANOTHER_TYPE);
    EraGlonassDataObj.crashType.crashAnotherType =
        GET_BIT_MASK_VALUE(crashInfoMask, LE_ECALL_CRASH_INFO_CRASH_ANOTHER_TYPE);

    // Set to 1 to avoid MSD overwriting with 'ImportMSD'
    ecallPtr->builtMsdSize = 1;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Reset the ERA-GLONASS crash type bit mask. Therefore that optional parameter is not included
 * in the MSD message.
 *
 * @return
 *  - LE_OK on success
 *  - LE_DUPLICATE an MSD has been already imported
 *  - LE_BAD_PARAMETER bad eCall reference
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_ResetMsdEraGlonassCrashInfo
(
    le_ecall_CallRef_t   ecallRef       ///< [IN] eCall reference
)
{
    ECall_t*   ecallPtr = le_ref_Lookup(ECallRefMap, ecallRef);
    if (ecallPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ecallRef);
        return LE_BAD_PARAMETER;
    }

    if (ecallPtr->builtMsdSize > 1)
    {
        LE_ERROR("An MSD has been already imported!");
        return LE_DUPLICATE;
    }

    LE_DEBUG("CrashInfo mask reseted");

    EraGlonassDataObj.presentCrashInfo = false;

    return LE_OK;
}

