/**
 * @file eCallApp.c
 *
 * This module implements an eCallDemo application
 *
 * This demo app provides an @c ecallApp_StartSession() API to start a test eCall session.
 * You can call @c ecallApp_StartSession() with the number of passengers as a parameter to start the
 * session.
 *
 * This App uses the configuration tree to retrieve the following data:
 *
 * @verbatim
  $ config get eCallDemo:/

  /
      settings/
           psap<string> == <PSAP number>
           hMinAccuracy<int> == <minimum horizontal accuracy value>
           dirMinAccuracy<int> == <minimum direction accuracy value>
   @endverbatim
 *
 * - 'psap' is the PSAP telephone number.
 * - 'hAccuracy' is the minimum expected horizontal accuracy to trust the position (in meters).
 * - 'dirAccuracy' is the minimum expected direction accuracy to trust the position (in degrees).
 *
 * You can set them by issuing the commands:
 * @verbatim
   $ config set eCallDemo:/settings/psap <PSAP number>
   $ config set eCallDemo:/settings/hMinAccuracy <minimum horizontal accuracy value> int
   $ config set eCallDemo:/settings/dirMinAccuracy <minimum direction accuracy value> int
   @endverbatim
 *
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
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
#define CFG_ECALL_APP_PATH          "/settings"


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
    int32_t*  dirMinAccuracyPtr
)
{
    char psapStr[LE_MDMDEFS_PHONE_NUM_MAX_BYTES] = {0};

    LE_DEBUG("Start reading eCall app settings in Configuration Tree");

    le_cfg_IteratorRef_t eCallCfgRef = le_cfg_CreateReadTxn(CFG_ECALL_APP_PATH);

    // Get PSAP
    if (le_cfg_NodeExists(eCallCfgRef, CFG_NODE_PSAP))
    {
        if ( le_cfg_GetString(eCallCfgRef, CFG_NODE_PSAP, psapStr, sizeof(psapStr), "") != LE_OK )
        {
            LE_FATAL("No node value set for '%s', restart the app!", CFG_NODE_PSAP);
        }
        LE_DEBUG("eCall settings, PSAP number is %s", psapStr);
        if (le_ecall_SetPsapNumber(psapStr) != LE_OK)
        {
            LE_FATAL("Cannot set PSAP number, restart the app!");
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
    le_ecall_State_t    state,
    void*               contextPtr
)
{
    le_ecall_CallRef_t*  eCallRefPtr = (le_ecall_CallRef_t*)contextPtr;

    LE_INFO("New eCall state for eCallRef.%p",  *eCallRefPtr);

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
            break;
        }
        case LE_ECALL_STATE_WAITING_PSAP_START_IND:
        {
            LE_INFO("New eCall state is LE_ECALL_STATE_WAITING_PSAP_START_IND.");
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
            le_ecall_End(*eCallRefPtr);
            le_ecall_Delete(*eCallRefPtr);
            break;
        }
        case LE_ECALL_STATE_FAILED:
        {
            LE_INFO("New eCall state is LE_ECALL_STATE_FAILED.");
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
    int32_t  dirMinAccuracy ///< [IN] minimum direction accuracy to trust the position (in degrees)
)
{
    le_ecall_StateChangeHandlerRef_t  stateChangeHandlerRef;
    bool                              isPosTrusted = false;
    int32_t                           latitude = 0;
    int32_t                           longitude = 0;
    int32_t                           hAccuracy = 0;
    int32_t                           direction = 0;
    int32_t                           dirAccuracy = 0;

    LE_DEBUG("StartSession called");

    ECallRef=le_ecall_Create();
    LE_FATAL_IF((!ECallRef), "Unable to create an eCall object, restart the app!");

    LE_DEBUG("Create eCallRef.%p",  ECallRef);
    stateChangeHandlerRef = le_ecall_AddStateChangeHandler(ECallStateHandler, (void*)&ECallRef);
    LE_ERROR_IF((!stateChangeHandlerRef), " Unable to add an eCall state change handler!");

    if ((le_pos_Get2DLocation(&latitude, &longitude, &hAccuracy) == LE_OK) &&
        (le_pos_GetDirection(&direction, &dirAccuracy) == LE_OK))
    {
        if ((hAccuracy < hMinAccuracy) && (dirAccuracy < dirMinAccuracy))
        {
            isPosTrusted = true;
        }
    }

    LE_ERROR_IF((le_ecall_SetMsdTxMode(LE_ECALL_TX_MODE_PUSH) != LE_OK),
                "Unable to set the MSD Push mode!");

    LE_ERROR_IF((le_ecall_SetMsdPosition(ECallRef, isPosTrusted, latitude, longitude, direction) != LE_OK),
                "Unable to set the position!");

    if (paxCount > 0)
    {
        LE_ERROR_IF((le_ecall_SetMsdPassengersCount(ECallRef, paxCount) != LE_OK),
                    "Unable to set the number of passengers!");
    }

    LE_FATAL_IF((le_ecall_StartTest(ECallRef) != LE_OK),
                "Unable to start an eCall, restart the app!");

    LE_INFO("Test eCall has been successfully triggered.");
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
    int32_t  dirMinAccuracy = 0;

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

    le_posCtrl_Request();

    LE_INFO("eCallDemo app is started.");
}
