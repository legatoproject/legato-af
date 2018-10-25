/**
 * @file eCallApp.c
 *
 * This module implements an eCallDemo application. The following parameters need to be saved
 * in the config tree in order to run the application correctly:
 *
 * - config set eCallDemo:/settings/psap <number> string
 * - config set eCallDemo:/settings/hMinAccuracy <value> int
 * - config set eCallDemo:/settings/dirMinAccuracy <value> int
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */


#include "legato.h"
#include "interfaces.h"



//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Nodes and path definitions to access the configuration tree entries
 *
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_PSAP               "psap"
#define CFG_NODE_H_MIN_ACCURACY     "hMinAccuracy"
#define CFG_NODE_DIR_MIN_ACCURACY   "dirMinAccuracy"
#define CFG_ECALL_APP_PATH          "eCallDemo:/settings"


//--------------------------------------------------------------------------------------------------
/**
 * Default settings values
 *
 */
//--------------------------------------------------------------------------------------------------
#define DEFAULT_PAX_COUNT       1
#define DEFAULT_H_ACCURACY      100
#define DEFAULT_DIR_ACCURACY    360

//--------------------------------------------------------------------------------------------------
/**
 * Define if the GNSS service shall be used for the eCallDemo test:
 * - true GNSS service is used.
 * - false GNSS service is not used.
 */
//--------------------------------------------------------------------------------------------------
#define TEST_GNSS_SERVICE_USED  true

//--------------------------------------------------------------------------------------------------
// Static declarations.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * eCall reference.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_ecall_CallRef_t ECallRef;

//--------------------------------------------------------------------------------------------------
/**
 * Load the eCall app settings
 *
 */
//--------------------------------------------------------------------------------------------------
static void LoadECallSettings
(
    int32_t*  hMinAccuracyPtr,
    uint32_t* dirMinAccuracyPtr
)
{
    char psapStr[LE_MDMDEFS_PHONE_NUM_MAX_BYTES] = {0};

    LE_DEBUG("Start reading eCall app settings in Configuration Tree");

    le_cfg_IteratorRef_t eCallCfgRef = le_cfg_CreateReadTxn(CFG_ECALL_APP_PATH);

    // Get PSAP
    if (le_cfg_NodeExists(eCallCfgRef, CFG_NODE_PSAP))
    {
        if (le_cfg_GetString(eCallCfgRef, CFG_NODE_PSAP, psapStr, sizeof(psapStr), "") != LE_OK)
        {
            LE_FATAL("No node value set for '%s', exit the app!", CFG_NODE_PSAP);
        }
        LE_DEBUG("eCall settings, PSAP number is %s", psapStr);
        if (le_ecall_SetPsapNumber(psapStr) != LE_OK)
        {
            LE_FATAL("Cannot set PSAP number, exit the app!");
        }
    }
    else
    {
        LE_FATAL("No value set for '%s', restart the app!", CFG_NODE_PSAP);
    }

    // Get minimum horizontal accuracy
    if (le_cfg_NodeExists(eCallCfgRef, CFG_NODE_H_MIN_ACCURACY))
    {
        *hMinAccuracyPtr = le_cfg_GetInt(eCallCfgRef, CFG_NODE_H_MIN_ACCURACY, DEFAULT_H_ACCURACY);
        LE_DEBUG("eCall app settings, horizontal accuracy is %d meter(s)", *hMinAccuracyPtr);
    }
    else
    {
        *hMinAccuracyPtr = DEFAULT_H_ACCURACY;
    }

    // Get minimum direction accuracy
    if (le_cfg_NodeExists(eCallCfgRef, CFG_NODE_DIR_MIN_ACCURACY))
    {
        *dirMinAccuracyPtr = le_cfg_GetInt(eCallCfgRef, CFG_NODE_DIR_MIN_ACCURACY, DEFAULT_DIR_ACCURACY);
        LE_DEBUG("eCall app settings, direction accuracy is %d degree(s)", *dirMinAccuracyPtr);
    }
    else
    {
        *dirMinAccuracyPtr = DEFAULT_DIR_ACCURACY;
    }

    le_cfg_CancelTxn(eCallCfgRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Handler function for eCall state Notifications.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ECallStateHandler
(
    le_ecall_CallRef_t  eCallRef,
    le_ecall_State_t    state,
    void*               contextPtr
)
{
    LE_INFO("New eCall state for eCallRef.%p",  eCallRef);

    switch(state)
    {
        case LE_ECALL_STATE_STARTED:
        {
            LE_INFO("New eCall state is LE_ECALL_STATE_STARTED.");
            break;
        }
        case LE_ECALL_STATE_CONNECTED:
        {
            LE_INFO("New eCall state is LE_ECALL_STATE_CONNECTED.");
            break;
        }
        case LE_ECALL_STATE_DISCONNECTED:
        {
            LE_INFO("New eCall state is LE_ECALL_STATE_DISCONNECTED.");
            LE_INFO("Termination reason: %d", le_ecall_GetTerminationReason(eCallRef) );
            break;
        }
        case LE_ECALL_STATE_WAITING_PSAP_START_IND:
        {
            LE_INFO("New eCall state is LE_ECALL_STATE_WAITING_PSAP_START_IND.");
            break;
        }
        case LE_ECALL_STATE_PSAP_START_IND_RECEIVED:
        {
            LE_INFO("New eCall state is LE_ECALL_STATE_PSAP_START_IND_RECEIVED.");
            if (le_ecall_SendMsd(eCallRef) != LE_OK)
            {
                LE_ERROR("Could not send the MSD");
            }
            break;
        }
        case LE_ECALL_STATE_MSD_TX_STARTED:
        {
            LE_INFO("New eCall state is LE_ECALL_STATE_MSD_TX_STARTED.");
            break;
        }
        case LE_ECALL_STATE_LLNACK_RECEIVED:
        {
            LE_INFO("New eCall state is LE_ECALL_STATE_LLNACK_RECEIVED.");
            break;
        }
        case LE_ECALL_STATE_LLACK_RECEIVED:
        {
            LE_INFO("New eCall state is LE_ECALL_STATE_LLACK_RECEIVED.");
            break;
        }
        case LE_ECALL_STATE_MSD_TX_COMPLETED:
        {
            LE_INFO("New eCall state is LE_ECALL_STATE_MSD_TX_COMPLETED.");
            break;
        }
        case LE_ECALL_STATE_MSD_TX_FAILED:
        {
            LE_INFO("New eCall state is LE_ECALL_STATE_MSD_TX_FAILED.");
            break;
        }
        case LE_ECALL_STATE_ALACK_RECEIVED_POSITIVE:
        {
            LE_INFO("New eCall state is LE_ECALL_STATE_ALACK_RECEIVED_POSITIVE.");
            break;
        }
        case LE_ECALL_STATE_ALACK_RECEIVED_CLEAR_DOWN:
        {
            LE_INFO("New eCall state is LE_ECALL_STATE_ALACK_RECEIVED_CLEAR_DOWN.");
            break;
        }
        case LE_ECALL_STATE_STOPPED:
        {
            LE_INFO("New eCall state is LE_ECALL_STATE_STOPPED.");
            break;
        }
        case LE_ECALL_STATE_RESET:
        {
            LE_INFO("New eCall state is LE_ECALL_STATE_RESET.");
            break;
        }
        case LE_ECALL_STATE_COMPLETED:
        {
            LE_INFO("New eCall state is LE_ECALL_STATE_COMPLETED.");
            break;
        }
        case LE_ECALL_STATE_FAILED:
        {
            LE_INFO("New eCall state is LE_ECALL_STATE_FAILED.");
            break;
        }
        case LE_ECALL_STATE_END_OF_REDIAL_PERIOD:
        {
            LE_INFO("New eCall state is LE_ECALL_STATE_END_OF_REDIAL_PERIOD.");
            break;
        }
        case LE_ECALL_STATE_TIMEOUT_T2:
        {
            LE_INFO("New eCall state is LE_ECALL_STATE_TIMEOUT_T2.");
            break;
        }
        case LE_ECALL_STATE_TIMEOUT_T3:
        {
            LE_INFO("New eCall state is LE_ECALL_STATE_TIMEOUT_T3.");
            break;
        }
        case LE_ECALL_STATE_TIMEOUT_T5:
        {
            LE_INFO("New eCall state is LE_ECALL_STATE_TIMEOUT_T5.");
            break;
        }
        case LE_ECALL_STATE_TIMEOUT_T6:
        {
            LE_INFO("New eCall state is LE_ECALL_STATE_TIMEOUT_T6.");
            break;
        }
        case LE_ECALL_STATE_TIMEOUT_T7:
        {
            LE_INFO("New eCall state is LE_ECALL_STATE_TIMEOUT_T7.");
            break;
        }
        case LE_ECALL_STATE_TIMEOUT_T9:
        {
            LE_INFO("New eCall state is LE_ECALL_STATE_TIMEOUT_T9.");
            break;
        }
        case LE_ECALL_STATE_TIMEOUT_T10:
        {
            LE_INFO("New eCall state is LE_ECALL_STATE_TIMEOUT_T10.");
            break;
        }
        case LE_ECALL_STATE_UNKNOWN:
        default:
        {
            LE_WARN("Unknown eCall state %d!", state);
            break;
        }
    }

}

//--------------------------------------------------------------------------------------------------
/**
 * Start a test eCall Session
 *
 * @note The process exits, if an error occurs.
 */
//--------------------------------------------------------------------------------------------------
static void StartSession
(
    uint32_t paxCount,      ///< [IN] number of passengers
    int32_t  hMinAccuracy,  ///< [IN] minimum horizontal accuracy to trust the position (in meters)
    uint32_t dirMinAccuracy ///< [IN] minimum direction accuracy to trust the position (in degrees)
)
{
    bool                              isPosTrusted = false;
    int32_t                           latitude = 0x7FFFFFFF;
    int32_t                           longitude = 0x7FFFFFFF;
    int32_t                           hAccuracy = 0;
    uint32_t                          direction = 0xFFFFFFFF;
    uint32_t                          dirAccuracy = 0;

    LE_DEBUG("StartSession called");

    ECallRef=le_ecall_Create();
    LE_FATAL_IF((!ECallRef), "Unable to create an eCall object, exit the app!");
    LE_DEBUG("Create eCallRef.%p",  ECallRef);

    // Get the position data
    if (TEST_GNSS_SERVICE_USED)
    {
        if ((le_pos_Get2DLocation(&latitude, &longitude, &hAccuracy) == LE_OK) &&
                        (le_pos_GetDirection(&direction, &dirAccuracy) == LE_OK))
        {
            if ((hAccuracy < hMinAccuracy) && (dirAccuracy < dirMinAccuracy))
            {
                isPosTrusted = true;
                LE_INFO("Position can be trusted.");
            }
        }
    }

    if (isPosTrusted == false)
    {
        LE_WARN("Position can't be trusted!");
    }


    LE_ERROR_IF((le_ecall_SetMsdPosition(ECallRef,
                                         isPosTrusted,
                                         latitude,
                                         longitude,
                                         direction) != LE_OK),
                "Unable to set the position!");

    // for demo purposes N-1 is hardcoded
    LE_ERROR_IF((le_ecall_SetMsdPositionN1(ECallRef,
                                         -512,
                                         -512) != LE_OK),
                "Unable to set the position N-1!");

    // for demo purposes N-2 is hardcoded
    LE_ERROR_IF((le_ecall_SetMsdPositionN2(ECallRef,
                                         511,
                                         0) != LE_OK),
                "Unable to set the position N-2!");

    if (paxCount > 0)
    {
        LE_ERROR_IF((le_ecall_SetMsdPassengersCount(ECallRef, paxCount) != LE_OK),
                    "Unable to set the number of passengers!");
    }

    LE_ERROR_IF((le_ecall_StartTest(ECallRef) != LE_OK),
                "Unable to start an eCall, try again!");

    LE_INFO("Test eCall has been successfully triggered.");
}

//--------------------------------------------------------------------------------------------------
/**
 * The signal event handler function for SIGINT/SIGTERM when process dies.
 */
//--------------------------------------------------------------------------------------------------
static void SigHandler
(
    int sigNum
)
{
    LE_INFO("Exit eCallDemo app");
    if (ECallRef)
    {
        le_ecall_End(ECallRef);
        le_ecall_Delete(ECallRef);
        ECallRef = NULL;
    }
    exit(EXIT_SUCCESS);
}


//--------------------------------------------------------------------------------------------------
//                                       Public declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Retrieve the eCall app settings and start a test eCall Session.
 *
 * @note On failure, the process exits, so you don't have to worry about checking any returned
 *       error codes.
 */
//--------------------------------------------------------------------------------------------------
void ecallApp_StartSession
(
    uint32_t  paxCount ///< [IN] number of passengers
)
{
    int32_t  hMinAccuracy = 0;
    uint32_t dirMinAccuracy = 0;

    if (ECallRef)
    {
        LE_WARN("End and Delete previous eCall session.");
        le_ecall_End(ECallRef);
        le_ecall_Delete(ECallRef);
        ECallRef = NULL;
    }

    LoadECallSettings(&hMinAccuracy, &dirMinAccuracy);

    LE_DEBUG("Start eCall session with %d passengers, hMinAccuracy.%d, dirMinAccuracy.%d",
                paxCount,
                hMinAccuracy,
                dirMinAccuracy);

    StartSession(paxCount, hMinAccuracy, dirMinAccuracy);
}

//--------------------------------------------------------------------------------------------------
/**
 * App init.
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("start eCallDemo app");

    ECallRef = NULL;

    signal(SIGINT, SigHandler);
    signal(SIGTERM, SigHandler);

    if (TEST_GNSS_SERVICE_USED)
    {
        le_posCtrl_Request();
        LE_INFO("Positioning service Started");
    }

    LE_ERROR_IF((NULL == le_ecall_AddStateChangeHandler(ECallStateHandler, NULL)),
                " Unable to add an eCall state change handler!");

    LE_WARN_IF((LE_OK != le_ecall_SetMsdTxMode(LE_ECALL_TX_MODE_PUSH)),
                "Unable to set the MSD Push mode! Use default settings.");

    LE_ERROR_IF((LE_OK != le_ecall_SetMsdVersion(1)),
                " Unable to set Msd Version!");

    LE_ERROR_IF((LE_OK != le_ecall_SetVehicleType(LE_ECALL_MSD_VEHICLE_BUS_M2)),
                " Unable to set Vehicle Type!");

    LE_ERROR_IF((LE_OK != le_ecall_SetVIN("WP0ZZZ99ZTS392124")),
                " Unable to set VIN!");

    LE_ERROR_IF((LE_OK != le_ecall_SetPropulsionType(LE_ECALL_PROPULSION_TYPE_ELECTRIC)),
                " Unable to set VIN!");

    LE_INFO("eCallDemo app is started.");
}
