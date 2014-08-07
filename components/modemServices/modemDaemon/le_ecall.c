/**
 * @file le_ecall.c
 *
 * This file contains the source code of the eCall API.
 *
 * Copyright (C) Sierra Wireless, Inc. 2014. Use of this work is subject to license.
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
 * MSD message length in bytes.
 */
//--------------------------------------------------------------------------------------------------
#define MSD_MAX_LEN      (282)

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
    char               psapNumber[LE_MDMDEFS_PHONE_NUM_MAX_LEN]; ///< PSAP telephone number
    le_ecall_State_t   state;                           ///< eCall state
    bool               isPushed;                        ///< True if the MSD is pushed by the IVS,
                                                        ///  false if the MSD is sent when requested
                                                        ///  by the PSAP (pull)
    uint32_t           maxRedialAttempts;               ///< Maximum redial attempts
    msd_t              msd;                             ///< MSD
    uint8_t            builtMsd[MSD_MAX_LEN];           ///< built MSD
    size_t             builtMsdSize;                    ///< Size of the built MSD
}
ECall_t;


//--------------------------------------------------------------------------------------------------
// Static declarations.
//--------------------------------------------------------------------------------------------------

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
 * @return LE_OK on success.
 * @return LE_FAULT no vehicle type found.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ParseAndSetVehicleType
(
    char* vehStr
)
{
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
 * @return LE_OK on success.
 * @return LE_FAULT no propulsion type found.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ParseAndSetPropulsionType
(
    char* propStr
)
{
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
 * @return LE_OK on success
 * @return LE_FAULT no valid propulsion type found
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
 * @return LE_OK on success
 * @return LE_FAULT there are missing settings
 */
//--------------------------------------------------------------------------------------------------
static le_result_t LoadECallSettings
(
    void
)
{
    le_result_t res = LE_OK;
    char psapStr[LE_MDMDEFS_PHONE_NUM_MAX_LEN] = {0};
    char configPath[LIMIT_MAX_PATH_BYTES];
    snprintf(configPath, sizeof(configPath), "%s", CFG_MODEMSERVICE_ECALL_PATH);

    LE_DEBUG("Start reading eCall information in ConfigDB");

    le_cfg_IteratorRef_t eCallCfg = le_cfg_CreateReadTxn(configPath);

    do
    {
        // Get PSAP
        if (le_cfg_NodeExists(eCallCfg, CFG_NODE_PSAP))
        {
            if ( le_cfg_GetString(eCallCfg, CFG_NODE_PSAP, psapStr, sizeof(psapStr), "112") != LE_OK )
            {
                LE_WARN("No node value set for '%s', use default one", CFG_NODE_PSAP);
                le_utf8_Copy(psapStr, "112", sizeof(psapStr), NULL);
            }
            le_utf8_Copy(ECallObj.psapNumber, psapStr, sizeof(ECallObj.psapNumber), NULL);
            LE_DEBUG("eCall settings, PSAP number is %s", psapStr);
        }
        else
        {
            LE_WARN("No value set for '%s', use default one", CFG_NODE_PSAP);
            le_utf8_Copy(ECallObj.psapNumber, "112", sizeof(ECallObj.psapNumber), NULL);
        }

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

        // Get MSD version
        if (le_cfg_NodeExists(eCallCfg, CFG_NODE_MSDVERSION))
        {
            ECallObj.msd.version = le_cfg_GetInt(eCallCfg, CFG_NODE_MSDVERSION, 0);
            LE_DEBUG("eCall settings, MSD version is %d", ECallObj.msd.version);
            if (ECallObj.msd.version == 0)
            {
                LE_ERROR("No correct value set for '%s' !", CFG_NODE_MSDVERSION);
                res = LE_FAULT;
                break;
            }
        }
        else
        {
            LE_ERROR("No value set for '%s' !", CFG_NODE_MSDVERSION);
            res = LE_FAULT;
            break;
        }

        // Get Max Redial Attempts value
        if (le_cfg_NodeExists(eCallCfg, CFG_NODE_MAX_REDIAL_ATTEMPTS))
        {
            ECallObj.maxRedialAttempts = le_cfg_GetInt(eCallCfg, CFG_NODE_MAX_REDIAL_ATTEMPTS, 10);
            LE_DEBUG("eCall settings, maxRedialAttempts is %d", ECallObj.maxRedialAttempts);
        }
        else
        {
            LE_ERROR("No value set for '%s' !", CFG_NODE_MAX_REDIAL_ATTEMPTS);
            res = LE_FAULT;
            break;
        }

        // Get Push/Pull mode
        if (le_cfg_NodeExists(eCallCfg, CFG_NODE_PUSHPULL))
        {
            char modeStr[VIN_MAX_LEN] = {0};
            if ( le_cfg_GetString(eCallCfg, CFG_NODE_PUSHPULL, modeStr, sizeof(modeStr), "") != LE_OK )
            {
                LE_WARN("No node value set for '%s'", CFG_NODE_PUSHPULL);
                res = LE_FAULT;
                break;
            }
            else if (strncmp(modeStr, "", sizeof(modeStr)) == 0)
            {
                LE_WARN("No node value set for '%s'", CFG_NODE_PUSHPULL);
                res = LE_FAULT;
                break;
            }

            LE_DEBUG("eCall settings, mode is %s", modeStr);
            if (strncmp(modeStr, "Push", sizeof(modeStr)) == 0)
            {
                ECallObj.isPushed = true;
            }
            else if (strncmp(modeStr, "Pull", sizeof(modeStr)) == 0)
            {
                ECallObj.isPushed = false;
            }
            else
            {
                LE_WARN("No node value set for '%s'", CFG_NODE_PUSHPULL);
                res = LE_FAULT;
                break;
            }
        }
        else
        {
            LE_ERROR("No value set for '%s' !", CFG_NODE_PUSHPULL);
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
 * Build an MSD from the eCall data object if needed, and load it into the Modem.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t LoadMsd
(
    ECall_t*   ecallPtr
)
{
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

        if ((ecallPtr->builtMsdSize = msd_EncodeMsdMessage(&ecallPtr->msd, ecallPtr->builtMsd)) == LE_FAULT)
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
    LE_INFO("le_ecall_Init called.");

    // Create an event Id for eCall State notification
    ECallStateId = le_event_CreateId("ECallState", sizeof(le_ecall_State_t));

    // Create the Safe Reference Map to use for eCall object Safe References.
    ECallRefMap = le_ref_CreateMap("ECallMap", ECALL_MAX);

    IsSessionStopped = true;

    // Retrieve the eCall settings from the configuration tree, including the static values of MSD
    if (LoadECallSettings() != LE_OK)
    {
        // TODO: LE_FATAL ?
        LE_ERROR("There are missing eCall settings, cannot perform eCall!");
    }


    LE_DEBUG("eCall settings: PSAP.%s, VIN.%17s, version.%d, isPushed.%d, maxRedialAttempts.%d, veh.%d",
             ECallObj.psapNumber,
             (char*)&ECallObj.msd.msdMsg.msdStruct.vehIdentificationNumber.isowmi[0],
             ECallObj.msd.version,
             ECallObj.isPushed,
             ECallObj.maxRedialAttempts,
             (int)ECallObj.msd.msdMsg.msdStruct.control.vehType);

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

    if (pa_ecall_SetPsapNumber(ECallObj.psapNumber) != LE_OK)
    {
        // TODO: LE_FATAL ?
        LE_ERROR("Unable to set the PSAP number, cannot perform eCall!");
    }

    if (pa_ecall_SetMaxRedialAttempts(ECallObj.maxRedialAttempts) != LE_OK)
    {
        LE_WARN("Unable to set the maximum redial attempts value");
    }

    pa_ecall_MsdTxMode_t msdTxMode;
    if (ECallObj.isPushed)
    {
        msdTxMode = PA_ECALL_TX_MODE_PUSH;
    }
    else
    {
        msdTxMode = PA_ECALL_TX_MODE_PULL;
    }
    if (pa_ecall_SetMsdTxMode(msdTxMode) != LE_OK)
    {
        // TODO: LE_FATAL ?
        LE_ERROR("Unable to set the PSAP number, cannot perform eCall!");
    }

    // Register a handler function for eCall state indications
    LE_FATAL_IF((pa_ecall_AddEventHandler(ECallStateHandler) == NULL),
                "Add pa_ecall_AddEventHandler failed");
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
le_ecall_ObjRef_t le_ecall_Create
(
    void
)
{
    // Create a new Safe Reference for the Call object.
    return ((le_ecall_ObjRef_t)le_ref_CreateRef(ECallRefMap, &ECallObj));
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
    le_ecall_ObjRef_t ecallRef     ///< [IN] eCall reference
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
 * @return LE_OK on success
 * @return LE_DUPLICATE an MSD has been already imported
 * @return LE_FAULT for other failures
 *
 * @note The process exits, if an invalid eCall reference is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_SetMsdPosition
(
    le_ecall_ObjRef_t   ecallRef,   ///< [IN] eCall reference
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
        return LE_FAULT;
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
 * @return LE_OK on success
 * @return LE_DUPLICATE an MSD has been already imported
 * @return LE_FAULT for other failures
 *
 * @note The process exits, if an invalid eCall reference is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_SetMsdPassengersCount
(
    le_ecall_ObjRef_t   ecallRef,   ///< [IN] eCall reference
    uint32_t            paxCount    ///< [IN] number of passengers
)
{
    ECall_t*   ecallPtr = le_ref_Lookup(ECallRefMap, ecallRef);
    if (ecallPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ecallRef);
        return LE_FAULT;
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
 * @return LE_OK on success
 * @return LE_DUPLICATE an MSD has been already imported
 * @return LE_FAULT for other failures
 *
 * @note If MSD code is too long (max MSD_MAX_LEN bytes) fatal errors is logged by killing the calling
 *       process after logging the message at EMERGENCY.
 *
* @note On failure, the process exits, so you don't have to worry about checking the returned
 *       reference for validity.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_ImportMsd
(
    le_ecall_ObjRef_t   ecallRef,      ///< [IN] eCall reference
    const uint8_t*      msdPtr,        ///< [IN] the prepared MSD
    size_t              msdNumElements ///< [IN] the prepared MSD size in bytes
)
{
    ECall_t*   ecallPtr = le_ref_Lookup(ECallRefMap, ecallRef);
    if (ecallPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ecallRef);
        return LE_FAULT;
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
 * Start an automatic eCall session
 *
 * @return LE_OK on success
 * @return LE_DUPLICATE an eCall session is already in progress
 * @return LE_FAULT for other failures
 *
 * @note The process exits, if an invalid eCall reference is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_StartAutomatic
(
    le_ecall_ObjRef_t    ecallRef   ///< [IN] eCall reference
)
{
    ECall_t*   ecallPtr = le_ref_Lookup(ECallRefMap, ecallRef);
    if (ecallPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ecallRef);
        return LE_FAULT;
    }

    if (!IsSessionStopped)
    {
        LE_ERROR("An eCall session is already in progress");
        return LE_DUPLICATE;
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
 * @return LE_OK on success
 * @return LE_DUPLICATE an eCall session is already in progress
 * @return LE_FAULT for other failures
 *
 * @note The process exits, if an invalid eCall reference is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_StartManual
(
    le_ecall_ObjRef_t    ecallRef   ///< [IN] eCall reference
)
{
    ECall_t*   ecallPtr = le_ref_Lookup(ECallRefMap, ecallRef);
    if (ecallPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ecallRef);
        return LE_FAULT;
    }

    if (!IsSessionStopped)
    {
        LE_ERROR("An eCall session is already in progress");
        return LE_DUPLICATE;
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
 * @return LE_OK on success
 * @return LE_DUPLICATE an eCall session is already in progress
 * @return LE_FAULT for other failures
 *
 * @note The process exits, if an invalid eCall reference is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_StartTest
(
    le_ecall_ObjRef_t    ecallRef   ///< [IN] eCall reference
)
{
    ECall_t*   ecallPtr = le_ref_Lookup(ECallRefMap, ecallRef);
    if (ecallPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ecallRef);
        return LE_FAULT;
    }

    if (!IsSessionStopped)
    {
        LE_ERROR("An eCall session is already in progress");
        return LE_DUPLICATE;
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
 * @return LE_OK on success
 * @return LE_DUPLICATE the eCall session was started by another application
 * @return LE_FAULT for other failures
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_End
(
    le_ecall_ObjRef_t    ecallRef   ///< [IN] eCall reference
)
{
    ECall_t*   ecallPtr = le_ref_Lookup(ECallRefMap, ecallRef);
    if (ecallPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ecallRef);
        return LE_FAULT;
    }

    if (ecallPtr->state != LE_ECALL_STATE_COMPLETED)
    {
        LE_ERROR("An eCall session is already in progress");
        return LE_DUPLICATE;
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
    le_ecall_ObjRef_t      ecallRef
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



