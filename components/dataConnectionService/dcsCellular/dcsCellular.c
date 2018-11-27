//--------------------------------------------------------------------------------------------------
/**
 *  Data Channel Server's C code implementation of its southbound interfaces with the cellular
 *  component.
 *
 *  Copyright (C) Sierra Wireless Inc.
 *
 */
//--------------------------------------------------------------------------------------------------

#include <stdlib.h>
#include <string.h>
#include "legato.h"
#include "interfaces.h"
#include "le_print.h"
#include "le_cfg_interface.h"
#include "dcs.h"
#include "dcsServer.h"
#include "dcsCellular.h"

#define DCS_CONFIG_TREE_ROOT_DIR "dataConnectionService:"
#define CFG_PATH_CELLULAR        "cellular"
#define CFG_NODE_PROFILEINDEX    "profileIndex"
#define LE_DCS_CELLCONNDBS_MAX 20      // max # of cellular conns supported


//--------------------------------------------------------------------------------------------------
/**
 * The following are the defines for backoff time parameters, including the init duration &
 * the max # of retries. After each failure to connect, the next backoff duration will be doubled
 * until the max # of retries is reached.
 */
//--------------------------------------------------------------------------------------------------
#define CELLULAR_RETRY_MAX 4
#define CELLULAR_RETRY_BACKOFF_INIT 1

static le_mem_PoolRef_t CellularConnDbPool;
static le_mrc_NetRegState_t CellPacketSwitchState = LE_MRC_REG_NONE;
static le_mrc_PacketSwitchedChangeHandlerRef_t CellPacketSwitchStateHdlrRef;


//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for cellular connection database objects
 */
//--------------------------------------------------------------------------------------------------
le_ref_MapRef_t CellularConnectionRefMap;


//--------------------------------------------------------------------------------------------------
/**
 * Search for the given cellular connection reference's connDb from its reference map
 *
 * @return
 *     - The found connDb will be returned in the function's return value; otherwise, NULL
 */
//--------------------------------------------------------------------------------------------------
static cellular_connDb_t *DcsCellularGetDbFromRef
(
    le_dcs_CellularConnectionRef_t cellConnRef
)
{
    return (cellular_connDb_t *)le_ref_Lookup(CellularConnectionRefMap, cellConnRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Utility for retrieving the cellConnDb of the given cellular profile index in the argument
 *
 * @return
 *     - The cellConnDb data structure of the given cellular profile in the function's return
 *       value upon success; otherwise NULL
 */
//--------------------------------------------------------------------------------------------------
static cellular_connDb_t *DcsCellularGetDbFromIndex
(
    uint32_t index
)
{
    le_ref_IterRef_t iterRef = le_ref_GetIterator(CellularConnectionRefMap);
    cellular_connDb_t *cellConnDb;

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        cellConnDb = (cellular_connDb_t *)le_ref_GetValue(iterRef);
        if (cellConnDb->index == index)
        {
            return cellConnDb;
        }
    }
    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function to retrieve the name of the channel at the given profile index
 *
 * @return
 *     - The retrieved name is returned in the 2nd argument of the function
 *     - This returned name is set to a null string upon failure to get the name
 */
//--------------------------------------------------------------------------------------------------
void le_dcsCellular_GetNameFromIndex
(
    uint32_t index,
    char channelName[LE_DCS_CHANNEL_NAME_MAX_LEN]
)
{
    if (index == 0)
    {
        channelName[0] = '\0';
        return;
    }
    snprintf(channelName, LE_DCS_CHANNEL_NAME_MAX_LEN-1, "%d", index);
}


//--------------------------------------------------------------------------------------------------
/**
 * Function to retrieve the profile index of a given connection which is a data profile name
 *
 * @return
 *     - The retrieved index is returned back as the function's return value
 *     - 0 upon error, although no error is supposed to happen since this never comes from user
 *       input but internal input; but this is defensive coding
 */
//--------------------------------------------------------------------------------------------------
static uint32_t DcsCellularGetProfileIndex
(
    const char *conn
)
{
    int index;

    if ((le_utf8_ParseInt(&index, conn) != LE_OK) || (index < 0))
    {
        // This is not supposed to happen since the input never comes as user input but internal
        // generated; but this is defensive coding here
        LE_ERROR("Invalid profile index %s for conversion into int", conn);
        return 0;
    }

    return index;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function to get the profile reference of a given connection which is a data profile by name
 *
 * @return
 *     - The retrieved profile reference is returned back as the function's return value. Upon
 *       unsuccessful retrieval, NULL will be.
 */
//--------------------------------------------------------------------------------------------------
static le_mdc_ProfileRef_t DcsCellularGetMdcProfileRef
(
    const char *conn
)
{
    uint32_t profIdx = DcsCellularGetProfileIndex(conn);
    return le_mdc_GetProfile(profIdx);
}


//--------------------------------------------------------------------------------------------------
/**
 * Function to check if the given MDC state is considered up or down & return a boolean to reflect
 * that
 *
 * @return
 *     - It returns true when the given state is considered up; otherwise false
 */
//--------------------------------------------------------------------------------------------------
static bool DcsCellularMdcStateIsUp
(
    le_mdc_ConState_t mdcState
)
{
    return ((mdcState == LE_MDC_CONNECTED) ? true : false);
}


//--------------------------------------------------------------------------------------------------
/**
 * Function to convert the cellular MRC state into a simple Up or Down state
 *
 * @return
 *     - It returns true when the given state is either LE_MRC_REG_HOME or LE_MRC_REG_ROAMING;
 *       otherwise false
 */
//--------------------------------------------------------------------------------------------------
static bool DcsCellularPacketSwitchStateIsUp
(
    le_mrc_NetRegState_t psState
)
{
    if ((psState == LE_MRC_REG_HOME) || (psState == LE_MRC_REG_ROAMING))
    {
        return true;
    }
    return false;
}


//--------------------------------------------------------------------------------------------------
/**
 *  DCS's event handler for connection state changes to be added to get DCS notified
 */
//--------------------------------------------------------------------------------------------------
static void DcsCellularConnEventStateHandler
(
    le_mdc_ProfileRef_t profileRef,         ///< [IN] Mobile data connection profile reference
    le_mdc_ConState_t state,                ///< [IN] Mobile data connection status
    void *contextPtr                        ///< [IN] Associated context pointer
)
{
    uint32_t profileIndex = le_mdc_GetProfileIndex(profileRef);
    char connName[LE_DCS_CHANNEL_NAME_MAX_LEN];
    cellular_connDb_t *cellConnDb;
    le_dcs_ChannelRef_t channelRef;
    le_result_t ret;
    uint16_t refcount;
    bool oldStateUp, newStateUp;

    LE_DEBUG("Connection event handler invoked with profile index: %d, state %i",
             profileIndex, state);

    le_dcsCellular_GetNameFromIndex(profileIndex, connName);
    cellConnDb = DcsCellularGetDbFromIndex(profileIndex);
    if (!cellConnDb)
    {
        LE_ERROR("No db found for connection %s for event notification", connName);
        return;
    }
    channelRef = le_dcs_GetChannelRefFromTechRef(cellConnDb->connRef);

    LE_DEBUG("Updating profile %d of cellular connection %s", profileIndex, connName);

    // retrieve the network interface for this connection & update it in the cellConndb
    ret = le_dcsCellular_GetNetInterface(cellConnDb->connRef, cellConnDb->netIntf,
                                         LE_DCS_INTERFACE_NAME_MAX_LEN);
    if (ret != LE_OK)
    {
        // See LE-11280 that a failure to get the network interface might be due to back-to-back
        // state changes and old state being received late here. Thus, when this happens,
        // override the state to be reported with the "disconnected" state instead
        LE_DEBUG("Report the disconnected state upon no network interface retrieved for "
                 "connection %s", connName);
        state = LE_MDC_DISCONNECTED;
    }

    oldStateUp = DcsCellularMdcStateIsUp(cellConnDb->opState);
    newStateUp = DcsCellularMdcStateIsUp(state);
    LE_INFO("State of connection %s transitioned from %s to %s", connName,
            oldStateUp ? "up" : "down", newStateUp ? "up" : "down");

    if (!DcsCellularPacketSwitchStateIsUp(CellPacketSwitchState))
    {
        // declare state down due to packet switch state's being down regarding of the state
        // input in the function argument
        LE_INFO("Send down notification for connection %s due to down packet switch state",
                connName);
        cellConnDb->opState = LE_MDC_DISCONNECTED;
        le_dcs_ChannelEventNotifier(channelRef, LE_DCS_EVENT_DOWN);
        return;
    }

    // update the states and send event notification upon an up-down or down-up state transition
    cellConnDb->opState = state;
    if (newStateUp)
    {
        // reset retry parameters
        cellConnDb->retries = 1;
        cellConnDb->backoff = CELLULAR_RETRY_BACKOFF_INIT;
        if (le_timer_IsRunning(cellConnDb->retryTimer))
        {
            le_timer_Stop(cellConnDb->retryTimer);
        }
        if (!oldStateUp)
        {
            le_dcs_ChannelEventNotifier(channelRef, LE_DCS_EVENT_UP);
        }
        return;
    }
    else
    {
        // new state is down
        if (le_dcs_GetChannelRefCountFromTechRef(cellConnDb->connRef, &refcount) != LE_OK)
        {
            LE_ERROR("Failed to get reference count of connection %s to handle state change",
                     connName);
            le_dcs_ChannelEventNotifier(channelRef, LE_DCS_EVENT_DOWN);
            return;
        }

        if (refcount == 0)
        {
            if (oldStateUp)
            {
                le_dcs_ChannelEventNotifier(channelRef, LE_DCS_EVENT_DOWN);
            }
            return;
        }

        // Need to retry until exhausted
        ret = le_dcsCellular_RetryConn(cellConnDb->connRef);
        switch (ret)
        {
            case LE_OK:
                LE_INFO("Wait for the next retry before failing connection %s", connName);
                break;
            case LE_DUPLICATE:
                LE_DEBUG("No need to trigger retry for connection %s", connName);
                break;
            case LE_OVERFLOW:
            case LE_FAULT:
            default:
                // report the down event anyway; even duplicated event notice is better than none
                cellConnDb->opState = LE_MDC_DISCONNECTED;
                le_dcs_ChannelEventNotifier(channelRef, LE_DCS_EVENT_DOWN);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Function to get the default profile's index
 *
 * @return
 *     - The function returns back the profile index found
 */
//--------------------------------------------------------------------------------------------------
uint32_t le_dcsCellular_GetProfileIndex
(
    int32_t mdcIndex
)
{
    int32_t index = mdcIndex;
    le_mdc_ProfileRef_t profileRef;

    char configPath[LE_CFG_STR_LEN_BYTES];
    snprintf(configPath, sizeof(configPath), "%s/%s", DCS_CONFIG_TREE_ROOT_DIR, CFG_PATH_CELLULAR);

    le_cfg_IteratorRef_t cfg = le_cfg_CreateReadTxn(configPath);

    // Get Cid Profile
    if (le_cfg_NodeExists(cfg, CFG_NODE_PROFILEINDEX))
    {
        index = le_cfg_GetInt(cfg, CFG_NODE_PROFILEINDEX, LE_MDC_DEFAULT_PROFILE);
        LE_DEBUG("Use data profile index %d", index);
    }
    le_cfg_CancelTxn(cfg);

    profileRef = le_mdc_GetProfile(index);
    if (!profileRef)
    {
        // If data profile could not be created/retrieved, we keep the index from config tree as is
        LE_ERROR("Unable to retrieve data profile");
    }
    else
    {
        index = le_mdc_GetProfileIndex(profileRef);
    }

    LE_DEBUG("Cellular profile index retrieved: %d", index);
    return index;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function for retrieving the current connection state of the given connection in the 1st argument
 *
 * @return
 *     - The retrieved connection state will be returned in the 2nd argument
 *     - The function returns LE_OK upon a successful retrieval; otherwise, some other le_result_t
 *       failure cause
 */
//--------------------------------------------------------------------------------------------------
static le_result_t DcsCellularGetConnState
(
    const char *conn,
    le_mdc_ConState_t *state
)
{
    le_result_t ret;
    le_mdc_ProfileRef_t profileRef = DcsCellularGetMdcProfileRef(conn);
    *state = LE_MDC_DISCONNECTED;

    if (!profileRef)
    {
        LE_ERROR("Failed to get cellular profile for connection %s", conn);
        return LE_FAULT;
    }

    ret = le_mdc_GetSessionState(profileRef, state);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to query cellular connection state for connection %s; error: %d",
                 conn, ret);
        return LE_FAULT;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Cellular connection's retry timer handler
 * When the timer expires after the last backoff, check if further retry can proceed. Go ahead if so
 */
//--------------------------------------------------------------------------------------------------
static void DcsCellularRetryConnTimerHandler
(
    le_timer_Ref_t timerRef
)
{
    le_result_t ret;
    uint16_t refcount;
    char cellConnName[LE_DCS_CHANNEL_NAME_MAX_LEN];
    cellular_connDb_t *cellConnDb = le_timer_GetContextPtr(timerRef);
    if (!cellConnDb)
    {
        LE_ERROR("Cellular connection db missing for processing retry timeout");
        return;
    }

    le_dcsCellular_GetNameFromIndex(cellConnDb->index, cellConnName);
    if (le_dcs_GetChannelRefCountFromTechRef(cellConnDb->connRef, &refcount) != LE_OK)
    {
        LE_ERROR("Failed to get reference count of connection %s to retry connecting",
                 cellConnName);
        return;
    }
    if (refcount == 0)
    {
        LE_DEBUG("No need to retry connection %s with no app using it", cellConnName);
        return;
    }

    LE_DEBUG("Retry timer expired; retrying to start connection %s", cellConnName);
    ret = le_dcsCellular_Start(cellConnDb->connRef);
    if ((ret == LE_OK) || (ret == LE_DUPLICATE))
    {
        LE_DEBUG("Succeeded initiating retry on connection %s", cellConnName);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Function for adding for the given connection in the argument DCS's own cellular-specific
 * connection event handler, i.e. DcsCellularConnEventStateHandler.
 *
 * @return
 *     - The function returns LE_OK upon a successful handler registration; otherwise, some other
 *       le_result_t failure cause
 */
//--------------------------------------------------------------------------------------------------
static le_result_t DcsCellularAddConnEventHandler
(
    cellular_connDb_t *cellConnDb
)
{
    char cellConnName[LE_DCS_CHANNEL_NAME_MAX_LEN];
    le_mdc_ProfileRef_t profileRef;

    if (!cellConnDb)
    {
        LE_ERROR("Invalid null cellular db to add connection event handler");
        return LE_FAULT;
    }

    le_dcsCellular_GetNameFromIndex(cellConnDb->index, cellConnName);
    profileRef = le_mdc_GetProfile(cellConnDb->index);
    if (!profileRef)
    {
        LE_ERROR("Failed to add event handler for connection %s due to invalid profile",
                 cellConnName);
        return LE_FAULT;
    }

    cellConnDb->evtHdlrRef = le_mdc_AddSessionStateHandler(profileRef,
                                                           DcsCellularConnEventStateHandler,
                                                           NULL);
    if (!cellConnDb->evtHdlrRef)
    {
        LE_ERROR("Failed to add connection event handler for connection %s", cellConnName);
        return LE_FAULT;
    }

    LE_INFO("Succeeded adding connection event handler for connection %s", cellConnName);
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function for internally creating a connection db of the cellular type for the given cellular
 * profile index
 *
 * @return
 *     - The newly created cellular connection db is returned in this function's return value upon
 *       successful creation; otherwise NULL
 */
//--------------------------------------------------------------------------------------------------
static cellular_connDb_t *DcsCellularCreateConnDb
(
    uint32_t profileIdx
)

{
    char cellConnName[LE_DCS_CHANNEL_NAME_MAX_LEN];
    cellular_connDb_t *cellConnDb;
    le_mdc_ConState_t mdcState;

    if (profileIdx == 0)
    {
        LE_ERROR("Cannot create cellular connection db with profile index 0");
        return NULL;
    }

    cellConnDb = DcsCellularGetDbFromIndex(profileIdx);
    if (cellConnDb)
    {
        return cellConnDb;
    }

    le_dcsCellular_GetNameFromIndex(profileIdx, cellConnName);
    cellConnDb = le_mem_ForceAlloc(CellularConnDbPool);
    if (!cellConnDb)
    {
        LE_ERROR("Unable to alloc cellular Db for connection %s", cellConnName);
        return NULL;
    }

    memset(cellConnDb, 0, sizeof(cellular_connDb_t));

    // Init the retry parameters including the timer & backoff
    cellConnDb->index = profileIdx;
    cellConnDb->retryTimer = le_timer_Create("cellConnRetryTimer");
    cellConnDb->backoff = CELLULAR_RETRY_BACKOFF_INIT;
    le_clk_Time_t retryInterval = {cellConnDb->backoff, 0};
    if ((LE_OK != le_timer_SetHandler(cellConnDb->retryTimer, DcsCellularRetryConnTimerHandler))
        || (LE_OK != le_timer_SetRepeat(cellConnDb->retryTimer, 1))       // One shot timer
        || (LE_OK != le_timer_SetInterval(cellConnDb->retryTimer, retryInterval)))
    {
        LE_ERROR("Failed to init retry timer for cellular connection %s", cellConnName);
        le_mem_Release(cellConnDb);
        return NULL;
    }
    if (LE_OK != le_timer_SetContextPtr(cellConnDb->retryTimer, (void*)cellConnDb))
    {
        LE_ERROR("Failed to set context pointer for the RetryTimer");
        le_timer_Delete(cellConnDb->retryTimer);
        le_mem_Release(cellConnDb);
        return NULL;
    }

    cellConnDb->connRef = le_ref_CreateRef(CellularConnectionRefMap, cellConnDb);
    if (DcsCellularAddConnEventHandler(cellConnDb) != LE_OK)
    {
        LE_ERROR("Failed to add event handler for cellular connection %s", cellConnName);
        le_timer_Delete(cellConnDb->retryTimer);
        le_mem_Release(cellConnDb);
        return NULL;
    }

    (void)le_dcsCellular_GetNetInterface(cellConnDb->connRef, cellConnDb->netIntf,
                                         LE_DCS_INTERFACE_NAME_MAX_LEN);
    (void)DcsCellularGetConnState(cellConnName, &mdcState);
    cellConnDb->opState = (uint16_t)mdcState;
    LE_DEBUG("ConnRef %p created for cellular connection %s", cellConnDb->connRef, cellConnName);
    return (cellConnDb);
}


//--------------------------------------------------------------------------------------------------
/**
 * Function to call MDC to get the list of all available profiles
 *
 * @return
 *     - The 1st argument is to be filled out with the retrieved list where the 2nd argument
 *       specifies how long this given list's size is for filling out
 *     - The function returns LE_OK upon a successful retrieval; otherwise, some other
 *       le_result_t failure cause
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_dcsCellular_GetChannelList
(
    le_dcs_ChannelInfo_t *channelList,
    int16_t *listSize
)
{
    uint16_t i;
    le_mdc_ConState_t mdcState;
    le_mdc_ProfileRef_t profileRef;
    char apn[LE_MDC_APN_NAME_MAX_LEN];
    le_mdc_ProfileInfo_t profileList[LE_DCS_CHANNEL_LIST_ENTRY_MAX];
    size_t listLen = LE_DCS_CHANNEL_LIST_ENTRY_MAX;
    le_result_t ret = le_mdc_GetProfileList(profileList, &listLen);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to get cellular's profile list; error: %d", ret);
        *listSize = 0;
        return LE_FAULT;
    }

    *listSize = (listLen < *listSize) ? listLen : *listSize;
    for (i = 0; i < *listSize; i++)
    {
        LE_DEBUG("Cellular profile retrieved index %i, type %i, name %s",
                 profileList[i].index, profileList[i].type, profileList[i].name);
        profileRef = le_mdc_GetProfile(profileList[i].index);
        if (!profileRef)
        {
            LE_WARN("Failed to get profile with index %d", profileList[i].index);
            apn[0] = '\0';
        }
        else
        {
            ret = le_mdc_GetAPN(profileRef, apn, LE_MDC_APN_NAME_MAX_LEN);
            if (LE_OK != ret)
            {
                LE_WARN("Failed to get apn name for profile index %d (rc %d)",
                        profileList[i].index, ret);
                apn[0] = '\0';
            }
        }
        DcsCellularCreateConnDb(profileList[i].index);
        le_dcsCellular_GetNameFromIndex(profileList[i].index, channelList[i].name);
        channelList[i].technology = LE_DCS_TECH_CELLULAR;
        if (LE_OK != DcsCellularGetConnState(channelList[i].name, &mdcState))
        {
            LE_WARN("Failed to get state of cellular connection %s, profile %s",
                    channelList[i].name, profileList[i].name);
        }
        else
        {
            channelList[i].state = DcsCellularMdcStateIsUp(mdcState) ?
                LE_DCS_STATE_UP : LE_DCS_STATE_DOWN;
        }
        LE_DEBUG("Cellular channel %s profile %s has state %d (apn %s)",
                 channelList[i].name, profileList[i].name, channelList[i].state, apn);
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Handler function for packet switch change notifications
 * When this is an up-down state change, an event notification needs to be generated for all active
 * connections.  When this is a down-up state change, legitimate sessions need to be retried.
 */
//--------------------------------------------------------------------------------------------------
static void DcsCellularPacketSwitchHandler
(
    le_mrc_NetRegState_t psState,
    void *contextPtr
)
{
    bool oldStateUp, newStateUp;

    LE_INFO("Packet switch state: previous %d, new %d", CellPacketSwitchState, psState);
    oldStateUp = DcsCellularPacketSwitchStateIsUp(CellPacketSwitchState);
    newStateUp = DcsCellularPacketSwitchStateIsUp(psState);
    CellPacketSwitchState = psState;
    if (oldStateUp != newStateUp)
    {
        le_dcs_EventNotifierTechStateTransition(LE_DCS_TECH_CELLULAR, newStateUp);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Function for querying the network interface of the given connection specified in the 1st argument
 *
 * @return
 *     - The retrieved network interface's name will be returned in the 2nd argument which allowed
 *       buffer length is specified in the 3rd argument that is to be observed strictly
 *     - The function returns LE_OK upon a successful retrieval; otherwise, some other le_result_t
 *       failure cause
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_dcsCellular_GetNetInterface
(
    void *techRef,
    char *intfName,
    int nameSize
)
{
    le_result_t ret;
    le_mdc_ProfileRef_t profileRef;
    char connName[LE_DCS_CHANNEL_NAME_MAX_LEN];
    cellular_connDb_t *cellConnDb =
        DcsCellularGetDbFromRef((le_dcs_CellularConnectionRef_t)techRef);
    intfName[0] = '\0';
    if (!cellConnDb)
    {
        LE_ERROR("Failed to find cellular connection db with reference %p", techRef);
        return LE_FAULT;
    }

    le_dcsCellular_GetNameFromIndex(cellConnDb->index, connName);
    profileRef = le_mdc_GetProfile(cellConnDb->index);
    if (!profileRef)
    {
        LE_ERROR("Failed to get profile reference for cellular connection %s", connName);
        return LE_FAULT;
    }

    ret = le_mdc_GetInterfaceName(profileRef, intfName, nameSize);
    LE_DEBUG("Network interface %s is for connection %s (query status: %d)",
             intfName, connName, ret);
    return ret;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function for querying the default GW address of the given connection specified in the 1st
 * argument
 *
 * @return
 *     - The retrieved default GW address will be returned in the 3rd argument which allowed
 *       buffer length is specified in the 4th argument that is to be observed strictly
 *     - The function returns LE_OK upon a successful retrieval; otherwise, some other le_result_t
 *       failure cause
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_dcsCellular_GetDefaultGWAddress
(
    void *techRef,
    bool *isIpv6,
    char *gwAddr,
    size_t *gwAddrSize
)
{
    le_mdc_ProfileRef_t profileRef;
    cellular_connDb_t *cellConnDb;
    char connName[LE_DCS_CHANNEL_NAME_MAX_LEN];

    gwAddr[0] = '\0';
    cellConnDb = DcsCellularGetDbFromRef((le_dcs_CellularConnectionRef_t)techRef);
    if (!cellConnDb)
    {
        LE_ERROR("Failed to find cellular connection db with reference %p", techRef);
        *gwAddrSize = 0;
        return LE_FAULT;
    }

    le_dcsCellular_GetNameFromIndex(cellConnDb->index, connName);
    profileRef = le_mdc_GetProfile(cellConnDb->index);
    if (!profileRef)
    {
        LE_ERROR("Failed to get mobile profile reference for cellular connection %s",
                 connName);
        *gwAddrSize = 0;
        return LE_FAULT;
    }

    if (!(*isIpv6 = le_mdc_IsIPv6(profileRef)) && !le_mdc_IsIPv4(profileRef))
    {
        LE_ERROR("Found mobile profile is of neither IPv4 nor IPv6");
        *gwAddrSize = 0;
        return LE_FAULT;
    }

    if (*isIpv6)
    {
        if (*gwAddrSize < LE_MDC_IPV6_ADDR_MAX_BYTES)
        {
            LE_ERROR("Input address string array too short for output for connection %s",
                     connName);
            *gwAddrSize = 0;
            return LE_FAULT;
        }

        *gwAddrSize = LE_MDC_IPV6_ADDR_MAX_BYTES;
        return le_mdc_GetIPv6GatewayAddress(profileRef, gwAddr, *gwAddrSize);
    }

    if (*gwAddrSize < LE_MDC_IPV4_ADDR_MAX_BYTES)
    {
        LE_ERROR("Input address string array too short for output for connection %s",
                 connName);
        *gwAddrSize = 0;
        return LE_FAULT;
    }

    *gwAddrSize = LE_MDC_IPV4_ADDR_MAX_BYTES;
    return le_mdc_GetIPv4GatewayAddress(profileRef, gwAddr, *gwAddrSize);
}


//--------------------------------------------------------------------------------------------------
/**
 * Function for querying the DNS addresses of the given connection specified in the 1st
 * argument
 *
 * @return
 *     - The retrieved 2 DNS addresses will be returned in the 3rd & 5th arguments which allowed
 *       buffer lengths are specified in the 4th & 6th arguments correspondingly
 *     - The function returns LE_OK upon a successful retrieval; otherwise, some other le_result_t
 *       failure cause
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_dcsCellular_GetDNSAddrs
(
    void *techRef,                   ///< [IN] object reference of the cellular connection
    bool *isIpv6,                    ///< [OUT] IP addr version: IPv6 (true) or IPv4 (false)
    char *dns1Addr,                  ///< [OUT] 1st DNS IP addr to be installed
    size_t *addr1Size,               ///< [INOUT] array length of the 1st DNS IP addr to be installed
    char *dns2Addr,                  ///< [OUT] 2nd DNS IP addr to be installed
    size_t *addr2Size                ///< [INOUT] array length of the 2nd DNS IP addr to be installed
)
{
    le_mdc_ProfileRef_t profileRef;
    cellular_connDb_t *cellConnDb;
    char connName[LE_DCS_CHANNEL_NAME_MAX_LEN];
    le_result_t ret;

    dns1Addr[0] = dns2Addr[0] = '\0';
    cellConnDb = DcsCellularGetDbFromRef((le_dcs_CellularConnectionRef_t)techRef);
    if (!cellConnDb)
    {
        LE_ERROR("Failed to find cellular connection db with reference %p", techRef);
        *addr1Size = *addr2Size = 0;
        return LE_FAULT;
    }

    le_dcsCellular_GetNameFromIndex(cellConnDb->index, connName);
    profileRef = le_mdc_GetProfile(cellConnDb->index);
    if (!profileRef)
    {
        LE_ERROR("Failed to get mobile profile reference for cellular connection %s", connName);
        *addr1Size = *addr2Size = 0;
        return LE_FAULT;
    }

    if (!(*isIpv6 = le_mdc_IsIPv6(profileRef)) && !le_mdc_IsIPv4(profileRef))
    {
        LE_ERROR("Found mobile profile is of neither IPv4 nor IPv6");
        *addr1Size = *addr2Size = 0;
        return LE_FAULT;
    }

    if (*isIpv6)
    {
        if ((*addr1Size < LE_MDC_IPV6_ADDR_MAX_BYTES) || (*addr1Size < LE_MDC_IPV6_ADDR_MAX_BYTES))
        {
            LE_ERROR("Input address string array too short for output for connection %s",
                     connName);
            *addr1Size = *addr2Size = 0;
            return LE_FAULT;
        }

        *addr1Size = *addr2Size = LE_MDC_IPV6_ADDR_MAX_BYTES;
        ret = le_mdc_GetIPv6DNSAddresses(profileRef, dns1Addr, *addr1Size, dns2Addr, *addr2Size);
        if (LE_OK != ret)
        {
            LE_ERROR("Failed to retrieve DNS addrs for connection %s", connName);
        }
        else
        {
            LE_DEBUG("Succeeded to retrieve DNS IPv6 addrs %s and %s", dns1Addr, dns2Addr);
        }
        return ret;
    }

    if ((*addr1Size < LE_MDC_IPV4_ADDR_MAX_BYTES) || (*addr1Size < LE_MDC_IPV4_ADDR_MAX_BYTES))
    {
        LE_ERROR("Input address string array too short for output for connection %s", connName);
        *addr1Size = *addr2Size = 0;
        return LE_FAULT;
    }

    *addr1Size = *addr2Size = LE_MDC_IPV4_ADDR_MAX_BYTES;
    ret = le_mdc_GetIPv4DNSAddresses(profileRef, dns1Addr, *addr1Size, dns2Addr, *addr2Size);
    if (LE_OK != ret)
    {
        LE_ERROR("Failed to retrieve DNS addrs for connection %s", connName);
    }
    else
    {
        LE_DEBUG("Succeeded to retrieve DNS IPv4 addrs %s and %s", dns1Addr, dns2Addr);
    }
    return ret;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function for requesting cellular to start the given data/connection in the 1st argument
 *
 * @return
 *     - The function returns LE_OK or LE_DUPLICATE upon a successful start; otherwise, some other
 *       le_result_t failure cause
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_dcsCellular_Start
(
    void *techRef                   ///< [IN] object reference of the cellular connection
)
{
    le_result_t ret, rc;
    char connName[LE_DCS_CHANNEL_NAME_MAX_LEN];
    le_mdc_ProfileRef_t profileRef;
    le_mdc_DisconnectionReason_t failure_reason;
    int32_t failure_code;
    uint32_t reqCount;
    cellular_connDb_t *cellConnDb;

    cellConnDb = DcsCellularGetDbFromRef((le_dcs_CellularConnectionRef_t)techRef);
    if (!cellConnDb)
    {
        LE_ERROR("Failed to find cellular connection db with reference %p", techRef);
        return LE_FAULT;
    }

    le_dcsCellular_GetNameFromIndex(cellConnDb->index, connName);
    profileRef = le_mdc_GetProfile(cellConnDb->index);
    if (!profileRef)
    {
        LE_ERROR("Failed to get mobile profile reference for cellular connection %s", connName);
        return LE_FAULT;
    }

    if (cellConnDb->index == le_data_ConnectedDataProfileIndex(&reqCount))
    {
        // this profile is already started up via the le_data API
        LE_INFO("Succeeded sharing cellular connection %s with apps via le_data", connName);
        cellConnDb->retries = 1;
        cellConnDb->backoff = CELLULAR_RETRY_BACKOFF_INIT;
        if (le_timer_IsRunning(cellConnDb->retryTimer))
        {
            le_timer_Stop(cellConnDb->retryTimer);
        }
        return LE_OK;
    }

    if (!DcsCellularPacketSwitchStateIsUp(CellPacketSwitchState))
    {
        LE_DEBUG("Connection %s not immediately started due to down packet switch state",
                 connName);
        return LE_UNAVAILABLE;
    }

    if (IsApnEmpty(profileRef))
    {
        LE_DEBUG("Set default APN for connection %s", connName);
        if (LE_OK != le_mdc_SetDefaultAPN(profileRef))
        {
            // Don't fail the request, as an empty APN might still get it connected
            LE_WARN("Failed to set default APN");
        }
    }

    ret = le_mdc_StartSession(profileRef);
    if ((ret == LE_DUPLICATE) || (ret == LE_OK))
    {
        LE_INFO("Succeeded starting cellular connection %s", connName);
        cellConnDb->retries = 1;
        cellConnDb->backoff = CELLULAR_RETRY_BACKOFF_INIT;
        if (le_timer_IsRunning(cellConnDb->retryTimer))
        {
            le_timer_Stop(cellConnDb->retryTimer);
        }
    }
    else
    {
        LE_ERROR("Failed to start cellular connection %s; error: %d", connName, ret);
        failure_reason = le_mdc_GetDisconnectionReason(profileRef);
        failure_code = le_mdc_GetPlatformSpecificDisconnectionCode(profileRef);
        LE_ERROR("Failure reason %d, code %d", failure_reason, failure_code);
        rc = le_dcsCellular_RetryConn(cellConnDb->connRef);
        switch (rc)
        {
            case LE_OK:
                LE_INFO("Wait for the next retry before failing connection %s", connName);
                ret = LE_OK;
                break;
            case LE_DUPLICATE:
                LE_DEBUG("No need to trigger retry for connection %s", connName);
                ret = LE_OK;
                break;
            case LE_OVERFLOW:
            case LE_FAULT:
            default:
                // report failure
                ret = LE_FAULT;
        }
    }
    return ret;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function for stopping the given data link/connection in the argument
 *
 * @return
 *     - The function returns LE_OK upon a successful stop; otherwise, some other
 *       le_result_t failure cause
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_dcsCellular_Stop
(
    void *techRef                   ///< [IN] object reference of the cellular connection
)
{
    le_result_t ret;
    le_mdc_ProfileRef_t profileRef;
    char connName[LE_DCS_CHANNEL_NAME_MAX_LEN];
    cellular_connDb_t *cellConnDb =
        DcsCellularGetDbFromRef((le_dcs_CellularConnectionRef_t)techRef);
    if (!cellConnDb)
    {
        LE_ERROR("Failed to find cellular connection db with reference %p", techRef);
        return LE_FAULT;
    }

    le_dcsCellular_GetNameFromIndex(cellConnDb->index, connName);
    profileRef = le_mdc_GetProfile(cellConnDb->index);
    if (!profileRef)
    {
        LE_ERROR("Failed to get mobile profile reference for cellular connection %s", connName);
        return LE_FAULT;
    }

    ret = le_mdc_StopSession(profileRef);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to stop cellular connection %s; error: %d", connName, ret);
    }
    else
    {
        LE_INFO("Succeeded stopping cellular connection %s", connName);
    }
    return ret;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function for creating a cellular connection db of the given connection if it's not present yet.
 * If present, it will set itself into the given connection's connDb
 *
 * @return
 *     - The object reference to the newly created cellular connection db is returned in this
 *       function's return value upon successful creation or found existence; otherwise NULL
 */
//--------------------------------------------------------------------------------------------------
void *le_dcsCellular_CreateConnDb
(
    const char *conn
)
{
    cellular_connDb_t *cellConnDb;
    uint32_t profileIdx = DcsCellularGetProfileIndex(conn);

    cellConnDb = DcsCellularGetDbFromIndex(profileIdx);
    if (!cellConnDb && !(cellConnDb = DcsCellularCreateConnDb(profileIdx)))
    {
        LE_ERROR("Failed to create cellular connection db for connection %s", conn);
        return NULL;
    }

    return ((void *)cellConnDb->connRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Function for checking if the given cellular connection db's operational state is up or not
 *
 * @return
 *     - In the function's return value, the bool is returned to indicate whether the given
 *       connection's techRef is up or not
 */
//--------------------------------------------------------------------------------------------------
bool le_dcsCellular_GetOpState
(
    void *techRef
)
{
    le_dcs_CellularConnectionRef_t cellConnRef = (le_dcs_CellularConnectionRef_t)techRef;
    cellular_connDb_t *cellConnDb;

    cellConnDb = DcsCellularGetDbFromRef(cellConnRef);
    if (!cellConnDb)
    {
        return 0;
    }

    return DcsCellularMdcStateIsUp(cellConnDb->opState);
}


//--------------------------------------------------------------------------------------------------
/**
 * Function to initiate a connection retry by starting the connection's retry timer upon which
 * expiry the retry will be carried out
 *
 * @return
 *     - It returns true if an upcoming retry will happen no matter who has initiated it; false
 *       otherwise
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_dcsCellular_RetryConn
(
    void *techRef
)
{
    char cellConnName[LE_DCS_CHANNEL_NAME_MAX_LEN];
    cellular_connDb_t *cellConnDb;
    le_dcs_ChannelRef_t channelRef;
    le_dcs_CellularConnectionRef_t cellConnRef = (le_dcs_CellularConnectionRef_t)techRef;

    cellConnDb = DcsCellularGetDbFromRef(cellConnRef);
    if (!cellConnDb)
    {
        LE_ERROR("Cellular connection db missing for processing retry timeout");
        return LE_FAULT;
    }

    channelRef = le_dcs_GetChannelRefFromTechRef(cellConnRef);
    le_clk_Time_t retryInterval = {cellConnDb->backoff, 0};
    le_dcsCellular_GetNameFromIndex(cellConnDb->index, cellConnName);

    if (DcsCellularMdcStateIsUp(cellConnDb->opState))
    {
        LE_DEBUG("Cellular connection %s already up with no need to retry", cellConnName);
        return LE_DUPLICATE;
    }

    if (cellConnDb->retries > CELLULAR_RETRY_MAX)
    {
        LE_INFO("Cellular connection %s already maxed out retry allowed (%d)", cellConnName,
                CELLULAR_RETRY_MAX);
        le_dcs_ChannelEventNotifier(channelRef, LE_DCS_EVENT_DOWN);
        return LE_OVERFLOW;
    }

    if (le_timer_IsRunning(cellConnDb->retryTimer))
    {
        LE_DEBUG("Connection retry will start after next retry timer expiry");
        return LE_DUPLICATE;
    }

    // set timer duration & then start it
    if ((LE_OK != le_timer_SetInterval(cellConnDb->retryTimer, retryInterval)) ||
        (LE_OK != le_timer_Start(cellConnDb->retryTimer)))
    {
        LE_ERROR("Failed to start retry timer for connection %s with backoff %d secs",
                 cellConnName, cellConnDb->backoff);
        return LE_FAULT;
    }

    // update retry count & backoff duration for next round
    LE_INFO("Initiated retrying connection %s; retry attempt %d, backoff %d secs", cellConnName,
            cellConnDb->retries++, cellConnDb->backoff);
    cellConnDb->backoff = cellConnDb->backoff * 2;
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Destructor function that runs when a cellular connection db is deallocated
 */
//--------------------------------------------------------------------------------------------------
static void DcsCellularConnDbDestructor
(
    void *objPtr
)
{
    cellular_connDb_t *cellConnDb = (cellular_connDb_t *)objPtr;

    if (le_timer_IsRunning(cellConnDb->retryTimer))
    {
        le_timer_Stop(cellConnDb->retryTimer);
    }
    le_timer_Delete(cellConnDb->retryTimer);
    le_ref_DeleteRef(CellularConnectionRefMap, cellConnDb->connRef);
    cellConnDb->connRef = NULL;
    le_mdc_RemoveSessionStateHandler(cellConnDb->evtHdlrRef);
    cellConnDb->evtHdlrRef = NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function for releasing a cellular_connDb_t back to free memory after it's looked up from the
 * given reference in the argument
 */
//--------------------------------------------------------------------------------------------------
void le_dcsCellular_ReleaseConnDb
(
    void *techRef
)
{
    le_dcs_CellularConnectionRef_t cellConnRef = (le_dcs_CellularConnectionRef_t)techRef;
    cellular_connDb_t *cellConnDb;

    cellConnDb = DcsCellularGetDbFromRef(cellConnRef);
    if (cellConnDb)
    {
        le_mem_Release(cellConnDb);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 *  Cellular handlers component initialization
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // Allocate the connection DB app event pool, and set the max number of objects
    CellularConnDbPool = le_mem_CreatePool("CellularConnDbPool", sizeof(cellular_connDb_t));
    le_mem_ExpandPool(CellularConnDbPool, LE_DCS_CELLCONNDBS_MAX);
    le_mem_SetDestructor(CellularConnDbPool, DcsCellularConnDbDestructor);

    // Create a safe reference map for cellular connection objects
    CellularConnectionRefMap = le_ref_CreateMap("Cellular Connection Reference Map",
                                                LE_DCS_CELLCONNDBS_MAX);

    (void)le_mrc_GetPacketSwitchedState(&CellPacketSwitchState);
    CellPacketSwitchStateHdlrRef =
        le_mrc_AddPacketSwitchedChangeHandler(DcsCellularPacketSwitchHandler, NULL);
    if (!CellPacketSwitchStateHdlrRef)
    {
        LE_WARN("Failed to add cellular packet switch state handler");
    }

    LE_INFO("Data Channel Service's Cellular Handlers component is ready");
}
