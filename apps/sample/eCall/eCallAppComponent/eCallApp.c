/**
 * @file eCallApp.c
 *
 * This module implements an eCall application
 *
 * This app provides an @c ecallApp_StartSession() API to start a test eCall session.
 * You can call @c ecallApp_StartSession() with the number of passengers as a parameter to start the
 * session.
 *
 * This app is automatically restarted in case of errors until the eCall session is completed. You
 * don't have to retrigger the session by calling @c ecallApp_StartSession(), the app will use
 * context variables containing the number of passengers and the session status.
 *
 *
 * This App uses the configuration tree to retrieve the following data:
 *
 * @verbatim
  config get eCall:/

  /
      settings/
           hMinAccuracy<int> = <minimum horizontal accuracy value>
           dirMinAccuracy<int> = <minimum direction accuracy value>
   @endverbatim
 *
 * - 'hAccuracy' is the minimum expected horizontal accuracy to trust the position (in meters).
 * - 'dirAccuracy' is the minimum expected direction accuracy to trust the position (in degrees).
 *
 * You can set them by issuing the commands:
 * @verbatim
   config set eCall:/settings/hMinAccuracy <minimum horizontal accuracy value> int
   config set eCall:/settings/dirMinAccuracy <minimum direction accuracy value> int
   @endverbatim
 *
 * Moreover, this app use two context variables:
 *
 * @verbatim
  config get eCall:/

  /
      context/
           isCleared<bool> == <cleared session flag>
           paxCount<int> == <number of passengers>
   @endverbatim
 *
 * - 'isCleared' is a flag indicating that a previous eCall session was or was not yet completed.
 * - 'paxCount' is the number of passengers passed to the app when it was triggered.
 *
 *
 * Copyright (C) Sierra Wireless, Inc. 2014. Use of this work is subject to license.
 *
 */


#include "legato.h"
#include "interfaces.h"


//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Nodes and path definitions to acces the configuration tree entries
 *
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_H_MIN_ACCURACY     "hMinAccuracy"
#define CFG_NODE_DIR_MIN_ACCURACY   "dirMinAccuracy"
#define CFG_ECALL_APP_PATH          "/settings"

#define CFG_NODE_IS_CLEARED         "isCleared"
#define CFG_NODE_PAX_COUNT          "paxCount"
#define CFG_ECALL_PROC_PATH         "/context"

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
 * Flag indicating that a session is already in progress.
 *
 */
//--------------------------------------------------------------------------------------------------
static  bool IsSessionStarted = false;

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
    LE_DEBUG("Start reading eCall app settings in ConfigDB");

    le_cfg_IteratorRef_t eCallCfgRef = le_cfg_CreateReadTxn(CFG_ECALL_APP_PATH);

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
 * This function verify if a previous session was or was not completed. It is called at app startup.
 * It passes the number of passengers as an output parameter if a previous session was not yet
 * completed.
 *
 * @return true if there was no previous session in progress.
 * @return false if the previous session was not yet completed.
 */
//--------------------------------------------------------------------------------------------------
static bool IsSessionCleared
(
    uint32_t*  paxCountPtr ///< [OUT] number of passengers
)
{
    bool                 isCleared;
    le_cfg_IteratorRef_t eCallProcRef = le_cfg_CreateReadTxn(CFG_ECALL_PROC_PATH);

    // Get minimum horizontal accuracy
    if (le_cfg_NodeExists(eCallProcRef, CFG_NODE_IS_CLEARED))
    {
        isCleared = le_cfg_GetBool(eCallProcRef, CFG_NODE_IS_CLEARED, false);
        LE_DEBUG("An eCall session was not completed");
        if(!isCleared)
        {
            // Get minimum horizontal accuracy
            if (le_cfg_NodeExists(eCallProcRef, CFG_NODE_PAX_COUNT))
            {
                *paxCountPtr = le_cfg_GetInt(eCallProcRef, CFG_NODE_PAX_COUNT, DEFAULT_PAX_COUNT);
                LE_DEBUG("An eCall session was not completed with %d passengers", *paxCountPtr);
            }
            else
            {
                *paxCountPtr = DEFAULT_PAX_COUNT;
            }
        }
    }
    else
    {
        isCleared = true;
    }

    le_cfg_CancelTxn(eCallProcRef);
    return isCleared;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the context variables.
 *
 */
//--------------------------------------------------------------------------------------------------
static void SetContextVariables
(
    uint32_t   paxCount ///< [IN] number of passengers
)
{
    LE_DEBUG("SetContextVariables called");
    le_cfg_IteratorRef_t eCallProcRef = le_cfg_CreateWriteTxn(CFG_ECALL_PROC_PATH);

    le_cfg_SetInt(eCallProcRef, CFG_NODE_PAX_COUNT, paxCount);
    le_cfg_SetBool(eCallProcRef, CFG_NODE_IS_CLEARED, false);

    le_cfg_CommitTxn(eCallProcRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Reset the context variables.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ResetContextVariables
(
    void
)
{
    le_cfg_IteratorRef_t eCallProcRef = le_cfg_CreateWriteTxn(CFG_ECALL_PROC_PATH);

    le_cfg_DeleteNode(eCallProcRef, CFG_NODE_PAX_COUNT);
    le_cfg_DeleteNode(eCallProcRef, CFG_NODE_IS_CLEARED);

    le_cfg_CommitTxn(eCallProcRef);
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
    if (state == LE_ECALL_STATE_CONNECTED)
    {
        LE_INFO("New eCall state is LE_ECALL_STATE_CONNECTED.");
    }
    else if (state == LE_ECALL_STATE_MSD_TX_COMPLETED)
    {
        LE_INFO("New eCall state is LE_ECALL_STATE_MSD_TX_COMPLETED.");
    }
    else if (state == LE_ECALL_STATE_MSD_TX_FAILED)
    {
        LE_INFO("New eCall state is LE_ECALL_STATE_MSD_TX_FAILED.");
    }
    else if (state == LE_ECALL_STATE_STOPPED)
    {
        LE_INFO("New eCall state is LE_ECALL_STATE_STOPPED.");
    }
    else if (state == LE_ECALL_STATE_RESET)
    {
        LE_INFO("New eCall state is LE_ECALL_STATE_RESET.");
    }
    else if (state == LE_ECALL_STATE_COMPLETED)
    {
        ResetContextVariables();
        IsSessionStarted = false;
        LE_INFO("New eCall state is LE_ECALL_STATE_COMPLETED.");
    }
    else if (state == LE_ECALL_STATE_FAILED)
    {
        LE_INFO("New eCall state is LE_ECALL_STATE_FAILED.");
    }
    else
    {
        LE_WARN("Unknowm eCall state!");
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
    le_ecall_ObjRef_t    eCallRef;
    bool                 isPosTrusted = false;
    int32_t              latitude = 0;
    int32_t              longitude = 0;
    int32_t              hAccuracy = 0;
    int32_t              direction = 0;
    int32_t              dirAccuracy = 0;

    LE_DEBUG("StartSession called");

    eCallRef=le_ecall_Create();
    LE_FATAL_IF((!eCallRef), "Unable to create an eCall object, restart the app!");

    if ((le_pos_Get2DLocation(&latitude, &longitude, &hAccuracy) == LE_OK) &&
        (le_pos_GetDirection(&direction, &dirAccuracy) == LE_OK))
    {
        if ((hAccuracy < hMinAccuracy) && (dirAccuracy < dirMinAccuracy))
        {
            isPosTrusted = true;
        }
    }

    LE_FATAL_IF((le_ecall_SetMsdPosition(eCallRef, isPosTrusted, latitude, longitude, direction) != LE_OK),
                "Unable to set the position, restart the app!");

    if (paxCount > 0)
    {
        LE_ERROR_IF((le_ecall_SetMsdPassengersCount(eCallRef, paxCount) != LE_OK),
                    "Unable to set the number of passengers, restart the app!");
    }

    LE_FATAL_IF((le_ecall_StartTest(eCallRef) == LE_FAULT),
                "Unable to start an eCall, restart the app!");

    LE_INFO("Test eCall has been succesfully triggered.");
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

    if(!IsSessionStarted)
    {
        LoadECallSettings(&hMinAccuracy, &dirMinAccuracy);

        LE_DEBUG("Start eCall session with %d passengers, hMinAccuracy.%d, dirMinAccuracy.%d",
                 paxCount,
                 hMinAccuracy,
                 dirMinAccuracy);

        SetContextVariables(paxCount);

        StartSession(paxCount, hMinAccuracy, dirMinAccuracy);
        IsSessionStarted = true;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * App init.
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    uint32_t                          paxCount;
    le_ecall_StateChangeHandlerRef_t  stateChangeHandlerRef;

    LE_INFO("start eCall app");

    IsSessionStarted = false;

    if(!IsSessionCleared(&paxCount))
    {
        LE_INFO("An eCall session was not completed, restart it!");
        ecallApp_StartSession(paxCount);
    }

    stateChangeHandlerRef = le_ecall_AddStateChangeHandler(ECallStateHandler, NULL);
    LE_ERROR_IF((!stateChangeHandlerRef), " Unable to add an eCall state change handler!");

    LE_INFO("eCall app is started.");
}
