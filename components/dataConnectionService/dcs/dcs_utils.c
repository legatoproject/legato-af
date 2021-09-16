//--------------------------------------------------------------------------------------------------
/**
 *  Data Channel Server's C code implementation of the utilties for the le_dcs APIs
 *
 *  Copyright (C) Sierra Wireless Inc.
 *
 */
//--------------------------------------------------------------------------------------------------

#include <stdlib.h>
#include <string.h>
#include "legato.h"
#include "interfaces.h"
#include "dcs.h"
#include "dcs_utils.h"

//--------------------------------------------------------------------------------------------------
/**
 * Config tree path statically used in this file
 */
//--------------------------------------------------------------------------------------------------
#if LE_CONFIG_ENABLE_CONFIG_TREE
static char configPath[LE_CFG_STR_LEN_BYTES] = {};
#endif

//--------------------------------------------------------------------------------------------------
/**
 * DCS technology name
 */
//--------------------------------------------------------------------------------------------------
static char DcsTechnologyName[LE_DCS_TECH_MAX_NAME_LEN];

//--------------------------------------------------------------------------------------------------
/**
 * Utility for converting a technology type enum into its name
 *
 * @return
 *     - The string name of the given technology in the function's return value
 */
//--------------------------------------------------------------------------------------------------
const char *dcs_ConvertTechEnumToName
(
    le_dcs_Technology_t tech
)
{
    switch(tech)
    {
#ifdef LE_CONFIG_ENABLE_WIFI
        case LE_DCS_TECH_WIFI:
            le_utf8_Copy(DcsTechnologyName, "wifi", sizeof(DcsTechnologyName), NULL);
            break;
#endif
        case LE_DCS_TECH_CELLULAR:
            le_utf8_Copy(DcsTechnologyName, "cellular", sizeof(DcsTechnologyName), NULL);
            break;
#ifdef LE_CONFIG_ENABLE_ETHERNET
        case LE_DCS_TECH_ETHERNET:
            le_utf8_Copy(DcsTechnologyName, "ethernet", sizeof(DcsTechnologyName), NULL);
            break;
#endif
        default:
            le_utf8_Copy(DcsTechnologyName, "unknown", sizeof(DcsTechnologyName), NULL);
            break;
    }
    return DcsTechnologyName;
}


//--------------------------------------------------------------------------------------------------
/**
 * Utility for retrieving the current channel state of the given channel in the 1st argument
 *
 * @return
 *     - The retrieved channel state will be returned in the 2nd argument
 *     - The function returns LE_OK upon a successful retrieval; otherwise, some other le_result_t
 *       failure cause
 */
//--------------------------------------------------------------------------------------------------
le_result_t dcs_GetAdminState
(
    le_dcs_ChannelRef_t channelRef,
    le_dcs_State_t *state
)
{
    le_dcs_channelDb_t *channelDb = dcs_GetChannelDbFromRef(channelRef);
    if (!channelDb)
    {
        LE_ERROR("Failed to find channel with reference %p to get state",
                 channelRef);
        *state = LE_DCS_STATE_DOWN;
        return LE_FAULT;
    }

    *state = (channelDb->refCount > 0) ? LE_DCS_STATE_UP : LE_DCS_STATE_DOWN;
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Utility for converting an le_dcs event into a string for printing
 *
 * @return
 *     - The string name of the given le_dcs event
 */
//--------------------------------------------------------------------------------------------------
const char *dcs_ConvertEventToString
(
    le_dcs_Event_t event
)
{
    switch (event)
    {
        case LE_DCS_EVENT_UP:
            return "Up";
        case LE_DCS_EVENT_DOWN:
            return "Down";
        case LE_DCS_EVENT_TEMP_DOWN:
            return "Temporary Down";
        default:
            return "Unknown";
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * This utility function checks the config tree path that specifies whether a channel event handler
 * is to be deleted or allowed to stay. It is called from dcs_RemoveEventHandler() either via a
 * client app's session closure or its le_dcs_RemoveEventHandler() API call.
 *
 * @return
 *     - True if the channel event handler can be deleted; false if otherwise
 */
//--------------------------------------------------------------------------------------------------
bool dcs_IsEventHandlerDeletable
(
    le_dcs_channelDb_t *channelDb,
    le_dcs_EventHandlerRef_t handlerRefToDelete
)
{
#if LE_CONFIG_ENABLE_CONFIG_TREE
    le_dcs_EventHandlerRef_t handlerRef;
    le_cfg_IteratorRef_t cfg;

    LE_DEBUG("Event handler ref %p for channel %s of tech %d", handlerRefToDelete,
             channelDb->channelName, channelDb->technology);

    snprintf(configPath, sizeof(configPath), "%s/%s/%d/%s", DCS_CONFIG_TREE_ROOT_DIR,
             CFG_PATH_SESSION_CLEANUP, channelDb->technology, channelDb->channelName);
    cfg = le_cfg_CreateReadTxn(configPath);
    if (!le_cfg_NodeExists(cfg, CFG_SESSION_CLEANUP_HANDLERREF))
    {
        LE_DEBUG("Not configured to skip; thus deletable");
        le_cfg_CancelTxn(cfg);
        return true;
    }

    handlerRef =
        (le_dcs_EventHandlerRef_t)(intptr_t)le_cfg_GetInt(cfg, CFG_SESSION_CLEANUP_HANDLERREF, 0);
    if (handlerRef == handlerRefToDelete)
    {
        LE_DEBUG("Configured to skip; thus not deletable");
        le_cfg_CancelTxn(cfg);
        return false;
    }

    LE_DEBUG("Not configured to skip; thus deletable");
    le_cfg_CancelTxn(cfg);
#endif
    return true;
}


//--------------------------------------------------------------------------------------------------
/**
 * This utility function checks the config tree path that specifies whether a data channel is to be
 * closed or allowed to stay upon its client app's exit. It is called from
 * CloseSessionEventHandler() during the closure of its client app session.
 *
 * @return
 *     - True if the given data channel is to be closed; false if otherwise
 */
//--------------------------------------------------------------------------------------------------
bool dcs_IsSessionExitChannelClosable
(
    le_dcs_channelDb_t *channelDb,
    le_dcs_ReqObjRef_t reqRefToClose
)
{
#if LE_CONFIG_ENABLE_CONFIG_TREE
    le_dcs_ReqObjRef_t reqRef;
    le_cfg_IteratorRef_t cfg;

    LE_DEBUG("Start request ref %p for channel %s of tech %d", reqRefToClose,
             channelDb->channelName, channelDb->technology);

    snprintf(configPath, sizeof(configPath), "%s/%s/%d/%s", DCS_CONFIG_TREE_ROOT_DIR,
             CFG_PATH_SESSION_CLEANUP, channelDb->technology, channelDb->channelName);
    cfg = le_cfg_CreateReadTxn(configPath);
    if (!le_cfg_NodeExists(cfg, CFG_SESSION_CLEANUP_REQREF))
    {
        LE_DEBUG("Not configured to skip; thus closable");
        le_cfg_CancelTxn(cfg);
        return true;
    }

    reqRef = (le_dcs_ReqObjRef_t)(intptr_t)le_cfg_GetInt(cfg, CFG_SESSION_CLEANUP_REQREF, 0);
    if (reqRef == reqRefToClose)
    {
        LE_DEBUG("Configured to skip; thus not closable");
        le_cfg_CancelTxn(cfg);
        return false;
    }

    LE_DEBUG("Not configured to skip; thus closure");
    le_cfg_CancelTxn(cfg);
#endif
    return true;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function checks if session cleanup filtering is configured on the config tree for use by
 * the given client app or not.
 *
 * @return
 *     - True if session cleanup filtering is enabled for the given client app; false if otherwise
 */
//--------------------------------------------------------------------------------------------------
#if LE_CONFIG_ENABLE_CONFIG_TREE
static bool dcs_IsSessionCleanupConfigured
(
    char const* appName
)
{
    if (!appName || (strlen(appName) == 0))
    {
        return false;
    }

    snprintf(configPath, sizeof(configPath), "%s/%s/%s", DCS_CONFIG_TREE_ROOT_DIR,
             CFG_PATH_SESSION_CLEANUP, appName);
    return (le_cfg_QuickGetBool(configPath, false));
}
#endif


//--------------------------------------------------------------------------------------------------
/**
 * This function seeks to save the given start channel reference for the given data channel onto
 * the config tree if the specified client app by name & session reference has session cleanup
 * filtering enabled for use.
 */
//--------------------------------------------------------------------------------------------------
void dcs_SessionCleanupSaveReqRef
(
    char const* appName,
    le_msg_SessionRef_t sessionRef,
    le_dcs_channelDb_t* channelDb,
    le_dcs_ReqObjRef_t reqRef
)
{
#if LE_CONFIG_ENABLE_CONFIG_TREE
    le_cfg_IteratorRef_t cfg;

    if (!dcs_IsSessionCleanupConfigured(appName))
    {
        // No need to proceed saving
        return;
    }
    LE_DEBUG("Saving request ref %p for app %s & session ref %p", reqRef, appName, sessionRef);

    // Save the reqRef
    snprintf(configPath, sizeof(configPath), "%s/%s/%d/%s", DCS_CONFIG_TREE_ROOT_DIR,
             CFG_PATH_SESSION_CLEANUP, channelDb->technology, channelDb->channelName);
    cfg = le_cfg_CreateWriteTxn(configPath);
    if (!cfg)
    {
        LE_ERROR("Failed to create config tree node to write info of client %s", appName);
        return;
    }

    le_cfg_SetInt(cfg, CFG_SESSION_CLEANUP_REQREF, (intptr_t)reqRef);
    le_cfg_CommitTxn(cfg);
#endif
}


//--------------------------------------------------------------------------------------------------
/**
 * This function seeks to save the given channel handler reference for the given data channel onto
 * the config tree if the specified client app by name & session reference has session cleanup
 * filtering enabled for use.
 */
//--------------------------------------------------------------------------------------------------
void dcs_SessionCleanupSaveEventHandler
(
    char const* appName,
    le_msg_SessionRef_t sessionRef,
    le_dcs_channelDb_t* channelDb,
    le_event_HandlerRef_t handlerRef
)
{
#if LE_CONFIG_ENABLE_CONFIG_TREE
    le_cfg_IteratorRef_t cfg;

    if (!dcs_IsSessionCleanupConfigured(appName))
    {
        // No need to proceed saving
        return;
    }
    LE_DEBUG("Saving event handler ref %p for app %s & session ref %p", handlerRef, appName,
             sessionRef);

    // Save the handlerRef
    snprintf(configPath, sizeof(configPath), "%s/%s/%d/%s", DCS_CONFIG_TREE_ROOT_DIR,
             CFG_PATH_SESSION_CLEANUP, channelDb->technology, channelDb->channelName);
    cfg = le_cfg_CreateWriteTxn(configPath);
    if (!cfg)
    {
        LE_ERROR("Failed to create config tree node to write info of client %s", appName);
        return;
    }

    le_cfg_SetInt(cfg, CFG_SESSION_CLEANUP_HANDLERREF, (intptr_t)handlerRef);
    le_cfg_CommitTxn(cfg);
#endif
}


//--------------------------------------------------------------------------------------------------
/**
 * This function upon le_dcs's component init cleans up the config tree params for session cleanup
 * filtering that are left behind by any earlier client app sessions that have already exited.
 */
//--------------------------------------------------------------------------------------------------
void dcs_cleanConfigTree
(
    void
)
{
#if LE_CONFIG_ENABLE_CONFIG_TREE
    snprintf(configPath, sizeof(configPath), "%s/%s", DCS_CONFIG_TREE_ROOT_DIR,
             CFG_PATH_SESSION_CLEANUP);
    le_cfg_QuickDeleteNode(configPath);
#endif
}
