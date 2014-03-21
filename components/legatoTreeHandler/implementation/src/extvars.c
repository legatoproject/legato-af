/** @file extvars.c
 *
 * This file contains the source code of the Tree Variable Handler.
 *
 * The Agent must call ExtVars_initialize() function before using the ExtVars tree handler.
 * The ExtVars_initialize() functions spawns two threads:
 * - ExtVarsThread
 * - MsClientThread
 *
 * ExtVarsThread creates two event IDs:
 * - VarValueChangeId in order to receive notifications from MsClientThread when a registered
 *   variable's value changes.
 * - SetNotifierRequestId in order to handle 'SetNotifier' request from the agent when
 *   ExtVars_set_notifier() function is called by the agent. Why this? Because the
 *   le_event_AddLayeredHandler() function must be called in the same thread in which the
 *   VarValueChangeId event id was created.
 *
 * MsClientThread registers itself as a client to the ModemDaemon. This thread is able to use the
 * Modem Services functions and is able to receive notifications from the ModemDaemon.
 * This thread will generate a notification to ExtVarsThread (using VarValueChangeId event) when
 * a registered variable's value changes.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */


#include "legato.h"
#include "returncodes.h"
#include "extvars.h"

#include "le_info.h"
#include "le_mrc.h"
#include "le_sim.h"
#include "le_pos.h"


//--------------------------------------------------------------------------------------------------
// New type definitions.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Variable Identifier.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    EXTVARS_VAR_ID_APN                  = 0,  ///< APN
    EXTVARS_VAR_ID_CDMA_ECIO            = 1,  ///< CDMA EC/IO
    EXTVARS_VAR_ID_CDMA_OPERATOR        = 2,  ///< CDMA Operator
    EXTVARS_VAR_ID_CDMA_PN_OFFSET       = 3,  ///< CDMA PN Offset
    EXTVARS_VAR_ID_CDMA_SID             = 4,  ///< CDMA SID
    EXTVARS_VAR_ID_CDMA_NID             = 5,  ///< CDMA NID
    EXTVARS_VAR_ID_GSM_CELL_ID          = 6,  ///< GSM Cell ID
    EXTVARS_VAR_ID_GSM_ECIO             = 7,  ///< GSM EC/IO
    EXTVARS_VAR_ID_GSM_OPERATOR         = 8,  ///< GSM Operator
    EXTVARS_VAR_ID_LTE_RSRP             = 9,  ///< LTE RSRP
    EXTVARS_VAR_ID_LTE_RSRQ             = 10, ///< LTE RSRQ.
    EXTVARS_VAR_ID_BYTES_RCVD           = 11, ///< Received bytes
    EXTVARS_VAR_ID_BYTES_SENT           = 12, ///< Sent bytes
    EXTVARS_VAR_ID_ROAM_STATUS          = 13, ///< Roaming Status
    EXTVARS_VAR_ID_IP                   = 14, ///< IP
    EXTVARS_VAR_ID_PKTS_RCVD            = 15, ///< Received packets
    EXTVARS_VAR_ID_PKTS_SENT            = 16, ///< Sent packets
    EXTVARS_VAR_ID_RSSI                 = 17, ///< RSSI
    EXTVARS_VAR_ID_SERVICE              = 18, ///< Service
    EXTVARS_VAR_ID_IMEI                 = 19, ///< IMEI.
    EXTVARS_VAR_ID_ICCID                = 20, ///< SIM ICCID
    EXTVARS_VAR_ID_IMSI                 = 21, ///< SIM IMSI
    EXTVARS_VAR_ID_SUBSCRIBER_PHONE_NUM = 22, ///< Sbscriber phone number
    EXTVARS_VAR_ID_SIGNAL_BARS          = 23, ///< Signal bars
    EXTVARS_VAR_ID_PRODUCT_STR          = 24, ///< Product string
    EXTVARS_VAR_ID_FW_VER               = 25, ///< FW version
    EXTVARS_VAR_ID_FW_NAME              = 26, ///< FW name
    EXTVARS_VAR_ID_POWER_IN             = 27, ///< Power in
    EXTVARS_VAR_ID_BOARD_TEMP           = 28, ///< Board temperature
    EXTVARS_VAR_ID_RADIO_TEMP           = 29, ///< Radio temperature
    EXTVARS_VAR_ID_RESET_NB             = 30, ///< Number of reset
    EXTVARS_VAR_ID_LATITUDE             = 31, ///< Latitude
    EXTVARS_VAR_ID_LONGITUDE            = 32, ///< Longitude

    EXTVARS_VAR_ID_END                  = 33
}
IdVar_t;

//--------------------------------------------------------------------------------------------------
/**
 * SetNotifier's Parameters structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    void*              ctxPtr;      ///< An opaque pointer, to be passed to ExtVars's notification
                                    ///  callback at each call
    ExtVars_notify_t*  notifierPtr; ///< Address of the notification function
}
SetNotifierParams_t;

//--------------------------------------------------------------------------------------------------
/**
 * Variable's value structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef  union {
    int    i;
    double d;
    char   s[64];  // todo: find a better mem allocation solution
    bool   b;
}
ValueVar_t;

//--------------------------------------------------------------------------------------------------
/**
 * Tree variable structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    IdVar_t          id;
    ValueVar_t       value;
    ExtVars_type_t   type;
    bool             notified;
    bool             registered;
    bool             isReadOnly;
    bool             isAutoUpdated;
}
TreeHdlVar_t;


//--------------------------------------------------------------------------------------------------
// Symbols.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Number of variables in the tree.
 *
 */
//--------------------------------------------------------------------------------------------------
#define NVARS EXTVARS_VAR_ID_END


//--------------------------------------------------------------------------------------------------
// Static variables.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * ExtVars and MsClient Thread references.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_thread_Ref_t  ExtVarsThreadRef = NULL;
static le_thread_Ref_t  MsClientThreadRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Event ID for variable's value changes.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t VarValueChangeId;

//--------------------------------------------------------------------------------------------------
/**
 * Event ID for SetNotifier request.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t SetNotifierRequestId;

//--------------------------------------------------------------------------------------------------
/**
 * Current Roaming State.
 *
 */
//--------------------------------------------------------------------------------------------------
static bool RoamingState;

//--------------------------------------------------------------------------------------------------
/**
 * All Variables are registered.
 *
 */
//--------------------------------------------------------------------------------------------------
static bool AllVarsRegistered;

//--------------------------------------------------------------------------------------------------
/**
 * Array of the Tree variables.
 *
 */
//--------------------------------------------------------------------------------------------------
static TreeHdlVar_t TreeHdlVars[NVARS] = {
  { .id = EXTVARS_VAR_ID_ROAM_STATUS, .type = EXTVARS_TYPE_BOOL, .notified = true},
  { .id = EXTVARS_VAR_ID_IMEI, .type = EXTVARS_TYPE_STR, .notified = true},
  { .id = EXTVARS_VAR_ID_ICCID, .type = EXTVARS_TYPE_STR, .notified = true},
  { .id = EXTVARS_VAR_ID_IMSI, .type = EXTVARS_TYPE_STR, .notified = true},
  { .id = EXTVARS_VAR_ID_LATITUDE, .type = EXTVARS_TYPE_DOUBLE, .notified = true},
  { .id = EXTVARS_VAR_ID_LONGITUDE, .type = EXTVARS_TYPE_DOUBLE, .notified = true},
};

//--------------------------------------------------------------------------------------------------
/**
 * Event ID for  variable's value changes.
 *
 */
//--------------------------------------------------------------------------------------------------
static IdVar_t CurrentVarIds[NVARS];


//--------------------------------------------------------------------------------------------------
// Static functions of MSClient thread.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Helper: retrieve a variable record from its id.
 */
//--------------------------------------------------------------------------------------------------
static TreeHdlVar_t* GetTreeVariable
(
    IdVar_t id
)
{
    int i;
    for(i = 0; i < NVARS; i++)
    {
        if(TreeHdlVars[i].id == id)
        {
            return (TreeHdlVars + i);
        }
    }
    LE_ERROR("Variable.%d not found\n", id);
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function forces the update of a variable value (only for some variables).
 *
 */
//--------------------------------------------------------------------------------------------------
void ForceValueUpdating
(
    TreeHdlVar_t* varPtr
)
{
    switch(varPtr->id)
    {
        case EXTVARS_VAR_ID_LATITUDE:
        {
            int32_t         latitude;
            int32_t         longitude;
            int32_t         hAccuracy;

            if(le_pos_Get2DLocation(&latitude, &longitude, &hAccuracy) != LE_OK)
            {
                LE_ERROR("Failed to get the 2D position fix!");
            }
            else
            {
                if (latitude != varPtr->value.d)
                {
                    varPtr->value.d = latitude;
                }
            }
        }
        break;

        case EXTVARS_VAR_ID_LONGITUDE:
        {
            int32_t         latitude;
            int32_t         longitude;
            int32_t         hAccuracy;

            if(le_pos_Get2DLocation(&latitude, &longitude, &hAccuracy) != LE_OK)
            {
                LE_ERROR("Failed to get the 2D position fix!");
            }
            else
            {
                if (longitude != varPtr->value.d)
                {
                    varPtr->value.d = longitude;
                }
            }
        }
        break;

        default:
        break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for SIM State Notifications.
 *
 */
//--------------------------------------------------------------------------------------------------
static void SimStateHandler
(
    le_sim_Ref_t simRef,
    void*        contextPtr
)
{
    TreeHdlVar_t*   varPtr;
    char            iccid[LE_SIM_ICCID_LEN];
    char            imsi[LE_SIM_IMSI_LEN];
    bool            notify=false;
    le_sim_States_t state;

    state = le_sim_GetState(simRef);

    LE_DEBUG("New SIM state notified (%d)", state);

    switch(state)
    {
        case LE_SIM_ABSENT:
            varPtr = GetTreeVariable(EXTVARS_VAR_ID_ICCID);
            varPtr->id = EXTVARS_VAR_ID_ICCID;
            varPtr->type = EXTVARS_TYPE_STR;
            varPtr->notified = false;
            varPtr->isReadOnly = true;
            varPtr->value.s[0] = '\0';

            varPtr = GetTreeVariable(EXTVARS_VAR_ID_IMSI);
            varPtr->id = EXTVARS_VAR_ID_IMSI;
            varPtr->type = EXTVARS_TYPE_STR;
            varPtr->notified = false;
            varPtr->isReadOnly = true;
            varPtr->value.s[0] = '\0';
            notify=true;
            break;

        case LE_SIM_INSERTED:
            if((le_sim_GetICCID(simRef, iccid, sizeof(iccid))) != LE_OK)
            {
                LE_ERROR("Failed to get the ICCID!");
            }
            else
            {
                varPtr = GetTreeVariable(EXTVARS_VAR_ID_ICCID);
                if (strcmp((const char *)varPtr->value.s, iccid))
                {
                    varPtr->id = EXTVARS_VAR_ID_ICCID;
                    varPtr->type = EXTVARS_TYPE_STR;
                    varPtr->notified = false;
                    varPtr->isReadOnly = true;
                    le_utf8_Copy((char*) varPtr->value.s, iccid, sizeof(varPtr->value.s), NULL);
                    notify=true;
                    LE_DEBUG("ICCID is updated with %s (get.%s)", (char*) varPtr->value.s, iccid);
                }
            }
            break;

        case LE_SIM_READY:
            if((le_sim_GetICCID(simRef, iccid, sizeof(iccid))) != LE_OK)
            {
                LE_ERROR("Failed to get the ICCID!");
            }
            else
            {
                varPtr = GetTreeVariable(EXTVARS_VAR_ID_ICCID);
                if (strcmp((const char *)varPtr->value.s, iccid))
                {
                    varPtr->id = EXTVARS_VAR_ID_ICCID;
                    varPtr->type = EXTVARS_TYPE_STR;
                    varPtr->notified = false;
                    varPtr->isReadOnly = true;
                    le_utf8_Copy((char*) varPtr->value.s, iccid, sizeof(varPtr->value.s), NULL);
                    notify=true;
                    LE_DEBUG("ICCID is updated with %s (get.%s)", (char*) varPtr->value.s, iccid);
                }
            }

            if((le_sim_GetIMSI(simRef, imsi, sizeof(imsi))) != LE_OK)
            {
                LE_ERROR("Failed to get the IMSI!");
            }
            else
            {
                varPtr = GetTreeVariable(EXTVARS_VAR_ID_IMSI);
                if (strcmp((const char *)varPtr->value.s, imsi))
                {
                    varPtr->id = EXTVARS_VAR_ID_IMSI;
                    varPtr->type = EXTVARS_TYPE_STR;
                    varPtr->notified = false;
                    varPtr->isReadOnly = true;
                    le_utf8_Copy((char*) varPtr->value.s, imsi, sizeof(varPtr->value.s), NULL);
                    notify=true;
                    LE_DEBUG("IMSI is updated with %s (get.%s)", (char*) varPtr->value.s, imsi);
                }
            }
            break;

        case LE_SIM_BLOCKED:
        case LE_SIM_BUSY:
        case LE_SIM_STATE_UNKNOWN:
            break;
    }

    if (notify)
    {
        LE_DEBUG("Notify on VarValueChangeId.%p", VarValueChangeId);
        // Notify the change
        le_event_Report(VarValueChangeId, NULL, 0);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for Network Registration Notifications.
 *
 */
//--------------------------------------------------------------------------------------------------
static void NetRegHandler
(
    le_mrc_NetRegState_t state,
    void*                contextPtr
)
{
    TreeHdlVar_t*          varPtr;

    if (state == LE_MRC_REG_ROAMING)
    {
        if (!RoamingState)
        {
            RoamingState = true;
            varPtr = GetTreeVariable(EXTVARS_VAR_ID_IMEI);
            varPtr->value.b = true;
            varPtr->id = EXTVARS_VAR_ID_ROAM_STATUS;
            varPtr->type = EXTVARS_TYPE_BOOL;
            varPtr->notified = false;
            // Notify the change
            le_event_Report(VarValueChangeId, NULL, 0);
        }
    }
    else
    {
        if (RoamingState)
        {
            RoamingState = false;
            varPtr = GetTreeVariable(EXTVARS_VAR_ID_IMEI);
            varPtr->value.b = false;
            varPtr->id = EXTVARS_VAR_ID_ROAM_STATUS;
            varPtr->type = EXTVARS_TYPE_BOOL;
            varPtr->notified = false;
            // Notify the change
            le_event_Report(VarValueChangeId, NULL, 0);
        }

    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialization function for Information variables.
 *
 */
//--------------------------------------------------------------------------------------------------
static rc_ReturnCode_t InitializeInfoVariables
(
    void
)
{
    rc_ReturnCode_t rc=RC_OK;
    TreeHdlVar_t*   varPtr;
    char            imei[LE_INFO_IMEI_MAX_LEN];

    // IMEI
    if(le_info_GetImei(imei, sizeof(imei)) != LE_OK)
    {
        LE_ERROR("Failed to get the IMEI");
        rc = RC_UNSPECIFIED_ERROR;
    }
    else
    {
        varPtr = GetTreeVariable(EXTVARS_VAR_ID_IMEI);

        varPtr->id = EXTVARS_VAR_ID_IMEI;
        varPtr->type = EXTVARS_TYPE_STR;
        varPtr->notified = true;
        varPtr->registered = false;
        varPtr->isReadOnly = true;
        le_utf8_Copy((char*) varPtr->value.s, imei, sizeof(varPtr->value.s), NULL);
    }

    return rc;
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialization function for MRC variables.
 *
 */
//--------------------------------------------------------------------------------------------------
static rc_ReturnCode_t InitializeMrcVariables
(
    void
)
{
    rc_ReturnCode_t         rc=RC_OK;
    TreeHdlVar_t*           varPtr;
    le_mrc_NetRegState_t    netState;

    // Roaming State
    if (le_mrc_GetNetRegState(&netState) != LE_OK)
    {
        LE_ERROR("Failed to get the Roaming State");
        rc = RC_UNSPECIFIED_ERROR;
    }
    else
    {
        varPtr = GetTreeVariable(EXTVARS_VAR_ID_ROAM_STATUS);
        if (netState == LE_MRC_REG_ROAMING)
        {
            varPtr->value.b = true;
            RoamingState = true;
        }
        else
        {
            varPtr->value.b = false;
            RoamingState = false;
        }
        varPtr->id = EXTVARS_VAR_ID_ROAM_STATUS;
        varPtr->type = EXTVARS_TYPE_BOOL;
        varPtr->notified = true;
        varPtr->registered = false;
        varPtr->isReadOnly = true;
    }

    if (le_mrc_AddNetRegStateHandler(NetRegHandler, NULL) == NULL)
    {
        LE_ERROR("Failed to install the Roaming State handler function!");
        rc = RC_UNSPECIFIED_ERROR;
    }

    return rc;
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialization function for SIM variables.
 *
 */
//--------------------------------------------------------------------------------------------------
static rc_ReturnCode_t InitializeSimVariables
(
    void
)
{
    rc_ReturnCode_t rc=RC_OK;
    TreeHdlVar_t*   varPtr;
    le_sim_Ref_t    simRef;
    char            iccid[LE_SIM_ICCID_LEN];
    char            imsi[LE_SIM_IMSI_LEN];

    if((simRef = le_sim_Create(1)) == NULL)
    {
        LE_ERROR("Failed to get the SIM reference!");
        rc = RC_UNSPECIFIED_ERROR;
    }
    else
    {
        if((le_sim_GetICCID(simRef, iccid, sizeof(iccid))) != LE_OK)
        {
            LE_ERROR("Failed to get the ICCID!");
            rc = RC_UNSPECIFIED_ERROR;
        }
        else
        {
            varPtr = GetTreeVariable(EXTVARS_VAR_ID_ICCID);

            varPtr->id = EXTVARS_VAR_ID_ICCID;
            varPtr->type = EXTVARS_TYPE_STR;
            varPtr->notified = true;
            varPtr->registered = false;
            varPtr->isReadOnly = true;
            le_utf8_Copy((char*) varPtr->value.s, iccid, sizeof(varPtr->value.s), NULL);
        }

        if((le_sim_GetIMSI(simRef, imsi, sizeof(imsi))) != LE_OK)
        {
            LE_ERROR("Failed to get the IMSI!");
            rc = RC_UNSPECIFIED_ERROR;
        }
        else
        {
            varPtr = GetTreeVariable(EXTVARS_VAR_ID_IMSI);

            varPtr->id = EXTVARS_VAR_ID_IMSI;
            varPtr->type = EXTVARS_TYPE_STR;
            varPtr->notified = true;
            varPtr->registered = false;
            varPtr->isReadOnly = true;
            le_utf8_Copy((char*) varPtr->value.s, imsi, sizeof(varPtr->value.s), NULL);
        }
    }

    if(le_sim_AddNewStateHandler(SimStateHandler, NULL) == NULL)
    {
        LE_ERROR("Failed to install the SIM state handler function!");
        rc = RC_UNSPECIFIED_ERROR;
    }

    return rc;
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialization function for Position variables.
 *
 */
//--------------------------------------------------------------------------------------------------
static rc_ReturnCode_t InitializePosVariables
(
    void
)
{
    rc_ReturnCode_t rc=RC_OK;
    TreeHdlVar_t*   varPtr;
    int32_t         latitude;
    int32_t         longitude;
    int32_t         hAccuracy;

    if(le_pos_Get2DLocation(&latitude, &longitude, &hAccuracy) != LE_OK)
    {
        LE_ERROR("Failed to get the 2D position fix!");
        rc = RC_UNSPECIFIED_ERROR;
    }
    else
    {
        varPtr = GetTreeVariable(EXTVARS_VAR_ID_LATITUDE);

        varPtr->id = EXTVARS_VAR_ID_LATITUDE;
        varPtr->type = EXTVARS_TYPE_DOUBLE;
        varPtr->notified = false;
        varPtr->registered = false;
        varPtr->isReadOnly = true;
        varPtr->value.d = latitude;

        varPtr = GetTreeVariable(EXTVARS_VAR_ID_LONGITUDE);

        varPtr->id = EXTVARS_VAR_ID_LONGITUDE;
        varPtr->type = EXTVARS_TYPE_DOUBLE;
        varPtr->notified = false;
        varPtr->registered = false;
        varPtr->isReadOnly = true;
        varPtr->value.d = longitude;
    }

    return rc;
}

//--------------------------------------------------------------------------------------------------
/**
 * MSClient Main Thread.
 *
 */
//--------------------------------------------------------------------------------------------------
static void* MsClientThread
(
    void* contextPtr
)
{
    le_sem_Ref_t semPtr = contextPtr;

    LE_DEBUG("Start MsClient thread.");

    // Populate my tree
    InitializeInfoVariables();
    InitializeMrcVariables();
    InitializeSimVariables();

    InitializePosVariables();

    le_sem_Post(semPtr);

    le_event_RunLoop();
    return NULL;
}


//--------------------------------------------------------------------------------------------------
// Static functions of ExtVars thread.
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
/**
 * The first-layer Variable Notifier Handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static void FirstLayerVariableNotifierHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    ExtVars_notify_t*   clientHandlerFunc = (ExtVars_notify_t*)secondLayerHandlerFunc;
    ExtVars_id_t        notifiedVarIds[NVARS];
    void*               notifiedVarValues[NVARS];
    ExtVars_type_t      notifiedVarTypes[NVARS];
    int                 notifiedVarCount = 0;
    int                 i;

    for(i = 0; i < NVARS; i++)
    {
        if ((TreeHdlVars[i].registered) && (!TreeHdlVars[i].notified))
        {
            notifiedVarIds[notifiedVarCount] = (ExtVars_id_t)TreeHdlVars[i].id;
            if (TreeHdlVars[i].type == EXTVARS_TYPE_STR)
            {
                notifiedVarValues[notifiedVarCount]  = (void *)TreeHdlVars[i].value.s;
            }
            else
            {
                notifiedVarValues[notifiedVarCount] = &TreeHdlVars[i].value.d;
            }
            notifiedVarTypes[notifiedVarCount] = TreeHdlVars[i].type;
            TreeHdlVars[i].notified = true;
            notifiedVarCount++;
        }
    }

    if (notifiedVarCount > 0)
    {
        LE_DEBUG("Variables must be notified, call ExtVars handlers.");
        clientHandlerFunc(le_event_GetContextPtr(),
                          notifiedVarCount,
                          notifiedVarIds,
                          notifiedVarValues,
                          notifiedVarTypes);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * The Handler that handles 'SetNotifier' requests from the Agent.
 *
 */
//--------------------------------------------------------------------------------------------------
static void SetNotifierRequestHandler
(
    void*  ctxPtr
)
{
    le_event_HandlerRef_t        handlerRef;

    SetNotifierParams_t* setNotifierPtr = (SetNotifierParams_t*)ctxPtr;

    handlerRef = le_event_AddLayeredHandler("VariableNotifierHandler",
                                            VarValueChangeId,
                                            FirstLayerVariableNotifierHandler,
                                            (le_event_HandlerFunc_t)setNotifierPtr->notifierPtr);

    le_event_SetContextPtr(handlerRef,  setNotifierPtr->ctxPtr);

    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * ExtVars Main Thread.
 *
 */
//--------------------------------------------------------------------------------------------------
static void* ExtVarsThread
(
    void* contextPtr
)
{
    le_sem_Ref_t semPtr = contextPtr;

    LE_DEBUG("Start ExtVars thread.");

    // Create an event Id and add a handler for SetNotifier requests.
    SetNotifierRequestId = le_event_CreateId("SetNotifierReq", sizeof(SetNotifierParams_t));
    le_event_AddHandler("SetNotifierRequestHandler",
                        SetNotifierRequestId,
                        (le_event_HandlerFunc_t)SetNotifierRequestHandler);


    // Create an event Id for variable's value changes.
    VarValueChangeId = le_event_CreateId("VarValueChange", 0);

    le_sem_Post(semPtr);

    le_event_RunLoop();
    return NULL;
}


//--------------------------------------------------------------------------------------------------
// Public.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the handler
 *
 * @return RC_OK, RC_UNSPECIFIED_ERROR
 */
//--------------------------------------------------------------------------------------------------
rc_ReturnCode_t ExtVars_initialize
(
    void
)
{
    LE_INFO("ExtVars_initialize called.");

    AllVarsRegistered = false;

    // I must thread ExtVars in order to receive VarValueChangeId report notifications.
    if (ExtVarsThreadRef == NULL)
    {
        le_sem_Ref_t semPtr = le_sem_Create("ExtVarsStartSem", 0);

        ExtVarsThreadRef = le_thread_Create("ExtVars", ExtVarsThread, semPtr);
        le_thread_Start(ExtVarsThreadRef);

        le_sem_Wait(semPtr);
        LE_INFO("ExtVars is correctly started.");

        // I must thread MsClient in order to send VarValueChangeId report notifications to ExtVars
        // thread.
        if (MsClientThreadRef == NULL)
        {
            MsClientThreadRef = le_thread_Create("MsClient", MsClientThread, semPtr);
            le_thread_Start(MsClientThreadRef);
            le_sem_Wait(semPtr);
            LE_INFO("MsClient is correctly started.");
        }
        le_sem_Delete(semPtr);
    }
    else
    {
        return RC_UNSPECIFIED_ERROR;
    }

    return RC_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Pass the notification function to the handler.
 *
 * The handler must call a notification function every time a registered variable's value changes.
 * But if handlers had a direct dependency to a public ExtVars_notify() function, building them as
 * DLL would become difficult and/or non-portable.
 *
 * To side-step this issue, handlers must provide a `set_notifier` function, whose purpose is to
 * receive a pointer to the notifier when the handler is Initialized. Up to the handler to keep this
 * function pointer and call it when appropriate.
 *
 */
//--------------------------------------------------------------------------------------------------
void ExtVars_set_notifier
(
    void*               ctxPtr,      ///< [IN] an opaque pointer, to be passed to ExtVars's
                                     ///       notification callback at each call
    ExtVars_notify_t*   notifierPtr  ///< [IN] address of the notification function
)
{
    SetNotifierParams_t setNotifierParams;

    setNotifierParams.ctxPtr = ctxPtr;
    setNotifierParams.notifierPtr = notifierPtr;
    le_event_Report(SetNotifierRequestId, &setNotifierParams, sizeof(setNotifierParams));
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Register or unregister for notification on one variable
 *
 * @return RC_OK, RC_BAD_PARAMETER, RC_NOT_FOUND
 */
//--------------------------------------------------------------------------------------------------
rc_ReturnCode_t ExtVars_register_variable
(
    ExtVars_id_t    var,    ///< [IN] the variable id
    int             enable  ///< [IN] register for notification if true, unregister if false
)
{
    LE_DEBUG("Variable.%d, enable=%d", var, enable);

    TreeHdlVar_t *varPtr = GetTreeVariable((IdVar_t)var);

    if(!varPtr)
    {
        return RC_NOT_FOUND;
    }
    else
    {
        varPtr->registered = enable;
        return RC_OK;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Register or unregister for notification on all variables
 *
 * @return RC_OK, RC_BAD_PARAMETER, RC_NOT_FOUND
 */
//--------------------------------------------------------------------------------------------------
rc_ReturnCode_t ExtVars_register_all
(
    int enable ///< [IN] register for notification if true, unregister if false
)
{
    int i;
    for(i = 0; i < NVARS; i++)
    {
        TreeHdlVars[i].registered = enable;
    }
    AllVarsRegistered = true;
    return RC_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Retrieve the content of a variable.
 *
 * The resources necessary to store the value must be allocated, if necessary, by the callback.
 * They must remain available at least until the `get_variable_release` callback is called.
 *
 * It is guaranteed that when a second call to `get_variable` is performed, any resource
 * returned by the previous calls can be safely freed. It is therefore acceptable to clean up
 * resources at the beginning of a `get_variable` rather than in the `get_variable_release`
 * callback.
 *
 * @return RC_OK, RC_BAD_PARAMETER, RC_NOT_FOUND
 */
//--------------------------------------------------------------------------------------------------
rc_ReturnCode_t ExtVars_get_variable
(
    ExtVars_id_t    var,       ///< [IN] The identifier of the variable to retrieve.
    void**          valuePtr,  ///< [OUT] The value of the retrieved variable.
    ExtVars_type_t* typePtr    ///< [OUT] The type of the retrieved variable.
)
{
    LE_DEBUG("Get Variable.%d", var);

    TreeHdlVar_t *varPtr = GetTreeVariable((IdVar_t)var);

    if(varPtr == NULL)
    {
        return RC_NOT_FOUND;
    }

    if(valuePtr)
    {
        ForceValueUpdating(varPtr);

        if (varPtr->type == EXTVARS_TYPE_STR)
        {
            *valuePtr = (void *)varPtr->value.s;
        }
        else
        {
            *valuePtr = &varPtr->value.d;
        }
    }
    if(typePtr)
    {
        *typePtr  = varPtr->type;
    }

    return RC_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Called by ExtVars after it has stopped needing the results of a `get_variable` callback;
 * Allows to clean up resources needed to maintain those results valid.
 */
//--------------------------------------------------------------------------------------------------
rc_ReturnCode_t ExtVars_get_variable_release
(
    ExtVars_id_t    var,    ///< [IN] The identifier of the variable.
    void*           valuePtr,  ///< [IN] The value of the retrieved variable.
    ExtVars_type_t  type    ///< [IN] The type of the retrieved variable.
)
{
    // nothing to do
    return RC_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * List all the variables identifiers handled by the handler.
 *
 * The resources necessary to store the `vars` table must be allocated by the callback,
 * and must remain available at least until the `list_release` callback is called.
 *
 * It is guaranteed that when a second call to `list` is performed, any resource
 * returned by the previous calls can be safely freed. It is therefore acceptable to clean up
 * resources at the beginning of a `list` rather than in the `list_release` callback.
 *
 * @return RC_OK, RC_BAD_PARAMETER
 */
//--------------------------------------------------------------------------------------------------
rc_ReturnCode_t ExtVars_list
(
    int*            nvarsPtr, ///< [OUT] where the number of variables must be written
    ExtVars_id_t**  varsPtr   ///< [OUT] must be made to point to an array of `nvars` variables.
)
{
    int i;
    for(i = 0; i < NVARS; i++)
    {
        CurrentVarIds[i] = TreeHdlVars[i].id;
    }

    if (varsPtr)
    {
        *varsPtr = (ExtVars_id_t*)CurrentVarIds;
    }
    if (nvarsPtr)
    {
        *nvarsPtr = NVARS;
    }

    return RC_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * If provided, called when the handler stopped needing a list of variables
 * passed to ExtVars_list(). Allows to clean up any dynamically allocated resource.
 */
//--------------------------------------------------------------------------------------------------
void ExtVars_list_release
(
    int             nvars,    ///< [IN] where the number of variables must be written
    ExtVars_id_t*   varsPtr   ///< [IN] must be made to point to an array of `nvars` variables.
)
{
    // nothing to do
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the value of several variables.
 *
 * The content of tables will remain available until the set_variables callback returns;
 * the callback is not responsible for freeing any resource it didn't create.
 *
 * @return RC_OK, RC_BAD_PARAMETER, RC_NOT_PERMITTED, RC_NOT_FOUND
 */
//--------------------------------------------------------------------------------------------------
rc_ReturnCode_t ExtVars_set_variables
(
    int             nvars,      ///< [IN] number of written variables
    ExtVars_id_t*   varsPtr,    ///< [IN] array of the written  variables names
    void**          valuesPtr,  ///< [IN] values to write in the variables
    ExtVars_type_t* typesPtr    ///< [IN] types of the written variables
)
{
    int inotified = 0, i = 0;

    for(i = 0; i < nvars; i++)
    {
        int notifiable = 0;
        TreeHdlVar_t *varPtr = GetTreeVariable((IdVar_t)varsPtr[i]);

        if(!varPtr)
        {
            return RC_NOT_FOUND;
        }

        if (varPtr->isReadOnly)
        {
            return RC_NOT_PERMITTED;
        }

        varPtr->id = (IdVar_t)varsPtr[i];

        if(varPtr->type == EXTVARS_TYPE_STR)
        {
            memset(varPtr->value.s, '\0', sizeof(varPtr->value.s));
        }

        switch(typesPtr[i])
        {
            case EXTVARS_TYPE_STR:
            {
                const char *newval = (const char *) valuesPtr[i];
                LE_DEBUG("Pushing string value \"%s\" for var %d\n", newval, varPtr->id);
                // The type has changed or this is the same type and the value has changed (either
                // this is the first initialization or another value)
                if( (typesPtr[i] != varPtr->type) ||
                    (varPtr->type == EXTVARS_TYPE_STR && (varPtr->value.s == 0 || strcmp((const char *)varPtr->value.s, newval) ) ) )
                {
                    le_utf8_Copy((char*) varPtr->value.s, newval, sizeof(varPtr->value.s), NULL);
                    notifiable = 1;
                }
                break;
            }
            case EXTVARS_TYPE_INT:
            {
                int newval = *(int *) valuesPtr[i];
                LE_DEBUG("Pushing int value %d for var %d\n", newval, varPtr->id);
                // The type has changed or this is the same type and the value has changed
                if(typesPtr[i] != varPtr->type ||
                   (varPtr->type == EXTVARS_TYPE_INT && (int)varPtr->value.i != newval))
                {
                    varPtr->value.i = newval;
                    notifiable = 1;
                }
                break;
            }
            case EXTVARS_TYPE_BOOL:
            {
                int newval = *(int *) valuesPtr[i];
                LE_DEBUG("Pushing boolean value %d for var %d\n", newval, varPtr->id);
                // The type has changed or this is the same type and the value has changed
                if((typesPtr[i] != varPtr->type) ||
                   (varPtr->type == EXTVARS_TYPE_BOOL && ((int)varPtr->value.i != newval)))
                {
                    varPtr->value.i = newval & 0x1;
                    notifiable = 1;
                }
                break;
            }
            case EXTVARS_TYPE_DOUBLE: {
                double newval = *(double *) valuesPtr[i];
                LE_DEBUG("Pushing double value %lf for var %d\n", newval, varPtr->id);
                // The type has changed or this is the same type and the value has changed
                if(typesPtr[i] != varPtr->type ||
                   (varPtr->type == EXTVARS_TYPE_DOUBLE && (double)varPtr->value.d != newval))
                {
                    varPtr->value.d = newval;
                    notifiable = 1;
                }
                break;
            }
           case EXTVARS_TYPE_NIL:
                LE_DEBUG("Deleting var %d", varPtr->id);
                notifiable = 1;
                break;
           default:
                break;
        }

        varPtr->type = typesPtr[i];
        if(notifiable && (varPtr->registered || AllVarsRegistered))
        {
            inotified++;
        }
    }

    if (inotified > 0)
    {
        le_event_Report(VarValueChangeId, NULL, 0);
    }
    return RC_OK;
}


