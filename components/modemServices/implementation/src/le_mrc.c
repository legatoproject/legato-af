/** @file le_mrc.c
 *
 * This file contains the the prototypes definitions of the high level MRC (Modem Radio Control)
 * APIs.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */


#include "legato.h"
#include "le_mrc.h"
#include "pa_mrc.h"
#include "le_cfg_interface.h"
#include "mdmCfgEntries.h"


#define PATTERN_NETWORK     "network-"
#define PATTERN_RAT         "rat-"

//--------------------------------------------------------------------------------------------------
/**
 * List Scan Information structure safe Reference.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    void*         safeRef;
    le_dls_Link_t link;
} le_mrc_ScanInformationSafeRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * List Scan Information structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_mrc_ScanInformationList
{
    le_dls_List_t  paScanInformationList;      // list of pa_mrc_ScanInformation_t
    le_dls_List_t  safeRefScanInformationList; // list of le_mrc_ScanInformationSafeRef_t
    le_dls_Link_t *currentLink;                // link for iterator
} le_mrc_ScanInformationList_t;


//--------------------------------------------------------------------------------------------------
// Static declarations.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Event ID for New Network Registration State notification.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t NewNetRegStateId;

//--------------------------------------------------------------------------------------------------
/**
 * Memory Pool for Listed ScanInformation.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t  ScanInformationListPool;

//--------------------------------------------------------------------------------------------------
/**
 * Memory Pool for Listed Information structure safe reference.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t  ScanInformationSafeRefPool;

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for Scan Information List.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t ScanInformationListRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for one Scan Information.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t ScanInformationRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of Scan Information List objects we expect to have at one time.
 */
//--------------------------------------------------------------------------------------------------
#define MRC_MAX_SCANLIST    5

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of Scan Information objects we expect to have at one time.
 */
//--------------------------------------------------------------------------------------------------
#define MRC_MAX_SCAN    10

//--------------------------------------------------------------------------------------------------
/**
 * The first-layer Network Registration State Change Handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static void FirstLayerNetRegStateChangeHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    le_mrc_NetRegState_t*           statePtr = reportPtr;
    le_mrc_NetRegStateHandlerFunc_t clientHandlerFunc = secondLayerHandlerFunc;

    clientHandlerFunc(*statePtr, le_event_GetContextPtr());

    // The reportPtr is a reference counted object, so need to release it
    le_mem_Release(reportPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * New Network Registration State handler function.
 *
 */
//--------------------------------------------------------------------------------------------------
static void NewRegStateHandler
(
    le_mrc_NetRegState_t* regStatePtr
)
{
    LE_DEBUG("Handler Function called with regStat %d", *regStatePtr);

    // Notify all the registered client's handlers
    le_event_ReportWithRefCounting(NewNetRegStateId, regStatePtr);
}


/**
 * Function to destroy all safeRef elements in the list.
 *
 */
//--------------------------------------------------------------------------------------------------
static void DeleteSafeRefList
(
    le_dls_List_t* listPtr
)
{
    le_mrc_ScanInformationSafeRef_t* nodePtr;
    le_dls_Link_t *linkPtr;

    while ((linkPtr=le_dls_Pop(listPtr)) != NULL)
    {
        nodePtr = CONTAINER_OF(linkPtr, le_mrc_ScanInformationSafeRef_t, link);
        le_ref_DeleteRef(ScanInformationRefMap, nodePtr->safeRef);
        le_mem_Release(nodePtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Function to convert configDB string into bitMask value for rat
 *
 */
//--------------------------------------------------------------------------------------------------
static uint32_t ConvertRatValue
(
    const char* ratValue
)
{
    if      ( strcmp(ratValue, "GSM") == 0 )
    {
        return PA_MRC_METWORK_RATMASK_GSM;
    }
    else if ( strcmp(ratValue, "UTMS") == 0 )
    {
        return PA_MRC_METWORK_RATMASK_UTMS;
    }
    else if ( strcmp(ratValue, "LTE") == 0 )
    {
        return PA_MRC_METWORK_RATMASK_LTE;
    }
    else if ( strcmp(ratValue, "GSM compact") == 0 )
    {
        return PA_MRC_METWORK_RATMASK_GSMCOMPACT;
    }
    LE_WARN("This rat value '%s' is not supported",ratValue);
    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function to load all rat preference for a given ratPath
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t LoadRatList
(
    const char  *ratPath,
    uint32_t    *ratMaskPtr
)
{
    uint32_t idx=0;
    LE_DEBUG("Load Rat Preference <%s>",ratPath);

    le_cfg_IteratorRef_t ratCfg = le_cfg_CreateReadTxn(ratPath);

    *ratMaskPtr = 0;
    do {
        // Get the node name.
        char ratNodeName[LIMIT_MAX_PATH_BYTES] = {0};
        char ratNodeValue[LIMIT_MAX_PATH_BYTES] = {0};

        sprintf(ratNodeName,PATTERN_RAT"%d",idx);

        // This is the exist state for the loop
        if (le_cfg_IsEmpty(ratCfg, ratNodeName))
        {
            LE_DEBUG("'%s' does not exist. stop reading configuration", ratNodeName);
            break;
        }

        if ( le_cfg_GetString(ratCfg,ratNodeName,ratNodeValue,sizeof(ratNodeValue), "") != LE_OK )
        {
            LE_WARN("Node value string for '%s' too large.",ratNodeName);
            le_cfg_CancelTxn(ratCfg);
            return LE_NOT_POSSIBLE;
        }

        if ( strncmp(ratNodeName,"",sizeof(ratNodeName)) == 0 )
        {
            LE_WARN("No node value set for '%s'",ratNodeName);
            le_cfg_CancelTxn(ratCfg);
            return LE_NOT_POSSIBLE;
        }

        *ratMaskPtr |= ConvertRatValue(ratNodeValue);

        ++idx;
    } while (true);

    le_cfg_CancelTxn(ratCfg);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Load the scanMode configuration
 *
 */
//--------------------------------------------------------------------------------------------------
static void LoadPreferredList
(
)
{
    uint32_t idx = 0;
    le_dls_List_t preferredNetworkList = LE_DLS_LIST_INIT;

    // Check that the modemRadioControl has a configuration value for preferred list.
    le_cfg_IteratorRef_t mrcCfg = le_cfg_CreateReadTxn(CFG_MODEMSERVICE_MRC_PATH"/"CFG_NODE_PREFERREDLIST);

    if (le_cfg_NodeExists(mrcCfg,"") == false)
    {
        LE_DEBUG("'%s' does not exist. Stop reading configuration",
                    CFG_MODEMSERVICE_MRC_PATH"/"CFG_NODE_PREFERREDLIST);
        le_cfg_CancelTxn(mrcCfg);
        return;
    }

    // Read all network from configDB
    do
    {
        uint32_t ratMask;
        char mccNodePath[LIMIT_MAX_PATH_BYTES] = {0};
        char mncNodePath[LIMIT_MAX_PATH_BYTES] = {0};
        char ratNodePath[LIMIT_MAX_PATH_BYTES] = {0};
        char mccStr[LIMIT_MAX_PATH_BYTES] = {0};
        char mncStr[LIMIT_MAX_PATH_BYTES] = {0};

        // Get the node name.
        char nodeName[LIMIT_MAX_PATH_BYTES] = {0};

        sprintf(nodeName,PATTERN_NETWORK"%d",idx);

        if (le_cfg_IsEmpty(mrcCfg, nodeName))
        {
            LE_DEBUG("'%s' does not exist. stop reading configuration", nodeName);
            break;
        }

        snprintf(mccNodePath, sizeof(mccNodePath), "%s/%s",nodeName,CFG_NODE_MCC);
        snprintf(mncNodePath, sizeof(mncNodePath), "%s/%s",nodeName,CFG_NODE_MNC);
        snprintf(ratNodePath, sizeof(ratNodePath),
                 CFG_MODEMSERVICE_MRC_PATH"/"CFG_NODE_PREFERREDLIST"/%s/%s",nodeName,CFG_NODE_RAT);

        if ( le_cfg_GetString(mrcCfg,mccNodePath,mccStr,sizeof(mccStr),"") != LE_OK )
        {
            LE_WARN("String value for '%s' too large.",mccNodePath);
            break;
        }

        if ( strcmp(mccStr,"") == 0 )
        {
            LE_WARN("No node value set for '%s'",mccNodePath);
            break;
        }

        if ( le_cfg_GetString(mrcCfg,mncNodePath,mncStr,sizeof(mncStr),"") != LE_OK )
        {
            LE_WARN("String value for '%s' too large.",mncNodePath);
            break;
        }

        if ( strcmp(mncStr,"") == 0 )
        {
            LE_WARN("No node value set for '%s'",mncNodePath);
            break;
        }

        if ( LoadRatList(ratNodePath,&ratMask) != LE_OK )
        {
            LE_WARN("Could not read rat information in '%s'",ratNodePath);
            break;
        }

        if ( pa_mrc_AddPreferredNetwork(&preferredNetworkList,mccStr,mncStr,ratMask) != LE_OK )
        {
            LE_WARN("Could not add [%s,%s] into the preferred list",mccStr,mncStr);
        }

        ++idx;
    }
    while (true);

    le_cfg_CancelTxn(mrcCfg);

    if ( pa_mrc_SavePreferredList(&preferredNetworkList) != LE_OK )
    {
        LE_WARN("Could not save the preferred list");
    }
    pa_mrc_ClearPreferedList(&preferredNetworkList);
}

//--------------------------------------------------------------------------------------------------
/**
 * Load the scanMode configuration
 *
 */
//--------------------------------------------------------------------------------------------------
static void LoadScanMode
(
)
{
    char configPath[LIMIT_MAX_PATH_BYTES];
    snprintf(configPath, sizeof(configPath), "%s/%s",CFG_MODEMSERVICE_MRC_PATH,CFG_NODE_SCANMODE);

    LE_DEBUG("Start reading MRC scanMode information in ConfigDB");

    le_cfg_IteratorRef_t mrcCfg = le_cfg_CreateReadTxn(configPath);

    do
    {
        if ( le_cfg_GetBool(mrcCfg,CFG_NODE_MANUAL,false) )
        {

            char mccStr[LIMIT_MAX_PATH_BYTES] = {0};
            char mncStr[LIMIT_MAX_PATH_BYTES] = {0};

            if ( le_cfg_GetString(mrcCfg,CFG_NODE_MCC,mccStr,sizeof(mccStr),"") != LE_OK )
            {
                LE_WARN("String value for '%s' too large.",CFG_NODE_MCC);
                break;
            }

            if ( strcmp(mccStr,"") == 0 )
            {
                LE_WARN("No node value set for '%s'",CFG_NODE_MCC);
                break;
            }

            if ( le_cfg_GetString(mrcCfg,CFG_NODE_MNC,mncStr,sizeof(mncStr),"") != LE_OK )
            {
                LE_WARN("String value for '%s' too large.",CFG_NODE_MNC);
                break;
            }

            if ( strcmp(mncStr,"") == 0 )
            {
                LE_WARN("No node value set for '%s'",CFG_NODE_MNC);
                break;
            }

            if ( le_mrc_ConnectCellularNetwork(mccStr,mncStr) != LE_OK )
            {
                LE_WARN("Could not connect to Network [%s,%s]",mccStr ,mncStr);
                break;
            }
        }
        else
        {
            if ( pa_mrc_SetAutomaticNetworkRegistration() != LE_OK )
            {
                LE_WARN("Could not set the Automatic Network Registration");
                break;
            }
        }

    } while (false);

    le_cfg_CancelTxn(mrcCfg);
}

//--------------------------------------------------------------------------------------------------
/**
 * Load the configuration tree
 *
 */
//--------------------------------------------------------------------------------------------------
static void LoadMrcConfigurationFromConfigDB
(
    void
)
{
    LE_DEBUG("Start reading MRC information in ConfigDB");

    LoadPreferredList();

    LoadScanMode();
}

//--------------------------------------------------------------------------------------------------
// APIs.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the MRC component.
 */
//--------------------------------------------------------------------------------------------------
void le_mrc_Init
(
    void
)
{
    le_result_t                 result=LE_OK;
    pa_mrc_NetworkRegSetting_t  setting;

    // Create an event Id for new Network Registration State notification
    NewNetRegStateId = le_event_CreateIdWithRefCounting("NewNetRegState");

    ScanInformationListPool = le_mem_CreatePool("ScanInformationListPool",
                                                sizeof(le_mrc_ScanInformationList_t));

    ScanInformationSafeRefPool = le_mem_CreatePool("ScanInformationSafeRefPool",
                                                sizeof(le_mrc_ScanInformationSafeRef_t));

    // Create the Safe Reference Map to use for Scan Information List object Safe References.
    ScanInformationListRefMap = le_ref_CreateMap("ScanInformationListMap", MRC_MAX_SCANLIST);

    // Create the Safe Reference Map to use for Scan Information List object Safe References.
    ScanInformationRefMap = le_ref_CreateMap("ScanInformationMap", MRC_MAX_SCAN);

    // Register a handler function for new Registration State indication
    LE_DEBUG("Add pa_mrc_SetNetworkRegHandler");
    LE_FATAL_IF((pa_mrc_AddNetworkRegHandler(NewRegStateHandler) == NULL),
                "Add pa_mrc_AddNetworkRegHandler failed");

    // Get & Set the Network registration state notification
    LE_DEBUG("Get the Network registration state notification configuration");
    result=pa_mrc_GetNetworkRegConfig(&setting);
    if ((result != LE_OK) || (setting == PA_MRC_DISABLE_REG_NOTIFICATION))
    {
        LE_ERROR_IF((result != LE_OK),
                    "Fails to get the Network registration state notification configuration");

        LE_INFO("Enable the Network registration state notification");
        LE_FATAL_IF((pa_mrc_ConfigureNetworkReg(PA_MRC_ENABLE_REG_NOTIFICATION)  != LE_OK),
                    "Enable the Network registration state notification failure");
    }

    LoadMrcConfigurationFromConfigDB();
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register an handler for Network registration state change.
 *
 * @return A handler reference, which is only needed for later removal of the handler.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_mrc_NetRegStateHandlerRef_t le_mrc_AddNetRegStateHandler
(
    le_mrc_NetRegStateHandlerFunc_t handlerFuncPtr, ///< [IN] The handler function.
    void*                           contextPtr      ///< [IN] The handler's context.
)
{
    le_event_HandlerRef_t        handlerRef;

    if (handlerFuncPtr == NULL)
    {
        LE_KILL_CLIENT("Handler function is NULL !");
        return NULL;
    }

    handlerRef = le_event_AddLayeredHandler("NewNetRegStateHandler",
                                            NewNetRegStateId,
                                            FirstLayerNetRegStateChangeHandler,
                                            (le_event_HandlerFunc_t)handlerFuncPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (le_mrc_NetRegStateHandlerRef_t)(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to remove an handler for Network registration state changes.
 */
//--------------------------------------------------------------------------------------------------
void le_mrc_RemoveNetRegStateHandler
(
    le_mrc_NetRegStateHandlerRef_t    handlerRef ///< [IN] The handler reference.
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the power of the Radio Module.
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_SetRadioPower
(
    le_onoff_t    power   ///< [IN] The power state.
)
{
    le_result_t res;

    res=pa_mrc_SetRadioPower(power);

    if (res != LE_OK)
    {
        return LE_FAULT;
    }
    else
    {
        return LE_OK;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the Radio Module power state.
 *
 * @return LE_NOT_POSSIBLE  The function failed to get the Radio Module power state.
 * @return LE_OK            The function succeed.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_GetRadioPower
(
    le_onoff_t*    powerPtr   ///< [OUT] The power state.
)
{
    le_result_t res;
    if (powerPtr == NULL)
    {
        LE_KILL_CLIENT("powerPtr is NULL !");
        return LE_FAULT;
    }

    res=pa_mrc_GetRadioPower(powerPtr);

    if (res != LE_OK)
    {
        return LE_NOT_POSSIBLE;
    }
    else
    {
        return LE_OK;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the Network registration state.
 *
 * @return LE_NOT_POSSIBLE  The function failed to get the Network registration state.
 * @return LE_OK            The function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_GetNetRegState
(
    le_mrc_NetRegState_t*   statePtr  ///< [OUT] The Network Registration state.
)
{
    if (statePtr == NULL)
    {
        LE_KILL_CLIENT("statePtr is NULL !");
        return LE_FAULT;
    }

    if (pa_mrc_GetNetworkRegState(statePtr) == LE_OK)
    {
        return LE_OK;
    }
    else
    {
        return LE_NOT_POSSIBLE;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the Signal Quality information.
 *
 * @return LE_NOT_POSSIBLE  The function failed to get the Signal Quality information.
 * @return LE_OK            The function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_GetSignalQual
(
    uint32_t*   qualityPtr  ///< [OUT] The received signal strength quality (0 = no signal strength,
                            ///        5 = very good signal strength).
)
{
    le_result_t   res;
    int32_t       rssi;   // The received signal strength (in dBm).
    int32_t       thresholds[] = {-113, -100, -90, -80, -65}; // TODO: Verify thresholds !
    uint32_t      i=0;
    size_t        thresholdsCount = NUM_ARRAY_MEMBERS(thresholds);

    if (qualityPtr == NULL)
    {
        LE_KILL_CLIENT("qualityPtr is NULL !");
        return LE_FAULT;
    }

    if ((res=pa_mrc_GetSignalQuality(&rssi)) == LE_OK)
    {
        for (i=0; i<thresholdsCount; i++)
        {
            if (rssi <= thresholds[i])
            {
                *qualityPtr = i;
                break;
            }
        }
        if (i == thresholdsCount)
        {
            *qualityPtr = i;
        }

        LE_DEBUG("pa_mrc_GetSignalQuality has returned rssi=%ddBm", rssi);
        return LE_OK;
    }
    else if (res == LE_OUT_OF_RANGE)
    {
        LE_DEBUG("pa_mrc_GetSignalQuality has returned LE_OUT_OF_RANGE");
        *qualityPtr = 0;
        return LE_OK;
    }
    else
    {
        LE_ERROR("pa_mrc_GetSignalQuality has returned %d", res);
        *qualityPtr = 0;
        return LE_NOT_POSSIBLE;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the Home Network Name information.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the Home Network Name can't fit in nameStr
 *      - LE_NOT_POSSIBLE on any other failure
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_GetHomeNetworkName
(
    char       *nameStr,               ///< [OUT] the home network Name
    size_t      nameStrSize            ///< [IN] the nameStr size
)
{
    if (nameStr == NULL)
    {
        LE_KILL_CLIENT("nameStr is NULL !");
        return LE_FAULT;
    }

    return pa_mrc_GetHomeNetworkName(nameStr,nameStrSize);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to connect to a cellular network [mcc;mnc]
 *
 * @return LE_NOT_POSSIBLE  The function failed to connect the network.
 * @return LE_OVERFLOW      One code is too long.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_ConnectCellularNetwork
(
    const char *mccPtr,   ///< [IN] Mobile Country Code
    const char *mncPtr    ///< [IN] Mobile Network Code
)
{
    if(strlen(mccPtr) > LE_MRC_MCC_LEN)
    {
        return LE_OVERFLOW;
    }

    if(strlen(mncPtr) > LE_MRC_MNC_LEN)
    {
        return LE_OVERFLOW;
    }

    return pa_mrc_ConnectNetwork(mccPtr,mncPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to perform a cellular network scan.
 *
 * @return
 *      Reference to the List object. Null pointer if the scan failed.
 */
//--------------------------------------------------------------------------------------------------
le_mrc_ScanInformation_ListRef_t le_mrc_PerformCellularNetworkScan
(
    le_mrc_Rat_t ratMask ///< [IN] Technology mask
)
{
    le_result_t result;
    le_mrc_ScanInformationList_t* newScanInformationListPtr = NULL;
    uint32_t networkScan = 0;

    newScanInformationListPtr = le_mem_ForceAlloc(ScanInformationListPool);
    newScanInformationListPtr->paScanInformationList = LE_DLS_LIST_INIT;
    newScanInformationListPtr->safeRefScanInformationList = LE_DLS_LIST_INIT;
    newScanInformationListPtr->currentLink = NULL;

    if (ratMask == LE_MRC_RAT_ALL) {
        networkScan |= PA_MRC_METWORK_MASK_GSM;
        networkScan |= PA_MRC_METWORK_MASK_UTMS;
        networkScan |= PA_MRC_METWORK_MASK_LTE;
        networkScan |= PA_MRC_METWORK_MASK_TD_SCDMA;
    }
    else
    {
        if (ratMask&LE_MRC_RAT_GSM)
        {
            networkScan |= PA_MRC_METWORK_MASK_GSM;
        }
        if (ratMask&LE_MRC_RAT_UTMS)
        {
            networkScan |= PA_MRC_METWORK_MASK_UTMS;
        }
        if (ratMask&LE_MRC_RAT_LTE)
        {
            networkScan |= PA_MRC_METWORK_MASK_LTE;
        }
        if (ratMask&LE_MRC_RAT_TC_SCDMA)
        {
            networkScan |= PA_MRC_METWORK_MASK_LTE;
        }
    }

    result = pa_mrc_PerformNetworkScan(networkScan,PA_MRC_SCAN_PLMN,
                                       &(newScanInformationListPtr->paScanInformationList));

    if (result != LE_OK)
    {
        le_mem_Release(newScanInformationListPtr);

        return NULL;
    }

    return le_ref_CreateRef(ScanInformationListRefMap, newScanInformationListPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the first Scan Information object reference in the list of
 * scan Information retrieved with le_mrc_PerformNetworkScan().
 *
 * @return NULL                         No scan information found.
 * @return le_mrc_ScanInformation_Ref_t The Scan Information object reference.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_mrc_ScanInformation_Ref_t le_mrc_GetFirstCellularNetworkScan
(
    le_mrc_ScanInformation_ListRef_t  scanInformationListRef ///< [IN] The list of scan information.
)
{
    pa_mrc_ScanInformation_t* nodePtr;
    le_dls_Link_t*          linkPtr;

    le_mrc_ScanInformationList_t* scanInformationListPtr = le_ref_Lookup(ScanInformationListRefMap,
                                                                         scanInformationListRef);

    if (scanInformationListPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", scanInformationListRef);
        return NULL;
    }

    linkPtr = le_dls_Peek(&(scanInformationListPtr->paScanInformationList));
    if (linkPtr != NULL)
    {
        nodePtr = CONTAINER_OF(linkPtr, pa_mrc_ScanInformation_t, link);
        scanInformationListPtr->currentLink = linkPtr;

        le_mrc_ScanInformationSafeRef_t* newScanInformationPtr = le_mem_ForceAlloc(ScanInformationSafeRefPool);
        newScanInformationPtr->safeRef = le_ref_CreateRef(ScanInformationRefMap,nodePtr);
        newScanInformationPtr->link = LE_DLS_LINK_INIT;
        le_dls_Queue(&(scanInformationListPtr->safeRefScanInformationList),&(newScanInformationPtr->link));

        return (le_mrc_ScanInformation_Ref_t)newScanInformationPtr->safeRef;
    }
    else
    {
        return NULL;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the next Scan Information object reference in the list of
 * scan Information retrieved with le_mrc_PerformNetworkScan().
 *
 * @return NULL                         No scan information found.
 * @return le_mrc_ScanInformation_Ref_t The Scan Information object reference.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_mrc_ScanInformation_Ref_t le_mrc_GetNextCellularNetworkScan
(
    le_mrc_ScanInformation_ListRef_t  scanInformationListRef ///< [IN] The list of scan information.
)
{
    pa_mrc_ScanInformation_t* nodePtr;
    le_dls_Link_t*          linkPtr;

    le_mrc_ScanInformationList_t* scanInformationListPtr = le_ref_Lookup(ScanInformationListRefMap,
                                                                         scanInformationListRef);


    if (scanInformationListPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", scanInformationListRef);
        return NULL;
    }

    linkPtr = le_dls_PeekNext(&(scanInformationListPtr->paScanInformationList),
                                scanInformationListPtr->currentLink);
    if (linkPtr != NULL)
    {
        nodePtr = CONTAINER_OF(linkPtr, pa_mrc_ScanInformation_t, link);
        scanInformationListPtr->currentLink = linkPtr;

        le_mrc_ScanInformationSafeRef_t* newScanInformationPtr = le_mem_ForceAlloc(ScanInformationSafeRefPool);
        newScanInformationPtr->safeRef = le_ref_CreateRef(ScanInformationRefMap,nodePtr);
        newScanInformationPtr->link = LE_DLS_LINK_INIT;
        le_dls_Queue(&(scanInformationListPtr->safeRefScanInformationList),&(newScanInformationPtr->link));

        return (le_mrc_ScanInformation_Ref_t)newScanInformationPtr->safeRef;
    }
    else
    {
        return NULL;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to delete the list of the Scan Information retrieved with
 * le_mrc_PerformNetworkScan().
 *
 * @note
 *      On failure, the process exits, so you don't have to worry about checking the returned
 *      reference for validity.
 */
//--------------------------------------------------------------------------------------------------
void le_mrc_DeleteCellularNetworkScan
(
    le_mrc_ScanInformation_ListRef_t  scanInformationListRef ///< [IN] The list of scan information.
)
{
    le_mrc_ScanInformationList_t* scanInformationListPtr = le_ref_Lookup(ScanInformationListRefMap,
                                                                         scanInformationListRef);

    if (scanInformationListPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", scanInformationListRef);
        return;
    }

    scanInformationListPtr->currentLink = NULL;
    pa_mrc_DeleteScanInformation(&(scanInformationListPtr->paScanInformationList));

    // Delete the safe Reference list.
    DeleteSafeRefList(&(scanInformationListPtr->safeRefScanInformationList));

    // Invalidate the Safe Reference.
    le_ref_DeleteRef(ScanInformationListRefMap, scanInformationListRef);

    le_mem_Release(scanInformationListPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the Cellular Network Code [mcc:mnc]
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the mcc or mnc would not fit in buffer
 *      - LE_NOT_POSSIBLE for all other errors
 *
 * @note
 *      On failure, the process exits, so you don't have to worry about checking the returned
 *      reference for validity.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_GetCellularNetworkMccMnc
(
    le_mrc_ScanInformation_Ref_t scanInformationRef,    ///< [IN] Scan information reference
    char                        *mccPtr,                ///< [OUT] Mobile Country Code
    size_t                       mccPtrSize,            ///< [IN] mccPtr buffer size
    char                        *mncPtr,                ///< [OUT] Mobile Network Code
    size_t                       mncPtrSize             ///< [IN] mncPtr buffer size
)
{
    pa_mrc_MobileCode_t mobileCode = { {0} };
    pa_mrc_ScanInformation_t* scanInformationPtr = le_ref_Lookup(ScanInformationRefMap,
                                                                 scanInformationRef);

    if (scanInformationPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", scanInformationRef);
        return LE_FAULT;
    }

    if (mccPtr == NULL)
    {
        LE_KILL_CLIENT("mccPtr is NULL");
        return LE_FAULT;
    }

    if (mncPtr == NULL)
    {
        LE_KILL_CLIENT("mncPtr is NULL");
        return LE_FAULT;
    }

    if ( pa_mrc_GetScanInformationCode(scanInformationPtr,&mobileCode) != LE_OK)
    {
        LE_WARN("Could not get scan information mobile code");
        return LE_NOT_POSSIBLE;
    }

    if ( le_utf8_Copy(mccPtr,mobileCode.mcc,mccPtrSize,NULL) != LE_OK )
    {
        LE_WARN("Could not copy all mcc");
        return LE_OVERFLOW;
    }

    if ( le_utf8_Copy(mncPtr,mobileCode.mnc,mncPtrSize,NULL) != LE_OK )
    {
        LE_WARN("Could not copy all mnc");
        return LE_OVERFLOW;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to perform a network scan.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the operator name would not fit in buffer
 *      - LE_NOT_POSSIBLE for all other errors
 *
 * @note
 *      On failure, the process exits, so you don't have to worry about checking the returned
 *      reference for validity.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_GetCellularNetworkName
(
    le_mrc_ScanInformation_Ref_t scanInformationRef,    ///< [IN] Scan information reference
    char *namePtr, ///< [OUT] Name of operator
    size_t nameSize ///< [IN] The size in bytes of the namePtr buffer
)
{
    pa_mrc_ScanInformation_t* scanInformationPtr = le_ref_Lookup(ScanInformationRefMap,
                                                                 scanInformationRef);

    if (scanInformationPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", scanInformationRef);
        return LE_FAULT;
    }

    return pa_mrc_GetScanInformationName(scanInformationPtr,namePtr,nameSize);
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to know if the radio control access is in scanInformationRef.
 *
 * @return
 *      - true the radio access technology is available
 *      - false otherwise
 *
 * @note
 *      On failure, the process exits.
 */
//--------------------------------------------------------------------------------------------------
bool le_mrc_IsCellularNetworkRatAvailable
(
    le_mrc_ScanInformation_Ref_t scanInformationRef,    ///< [IN] Scan information reference
    le_mrc_Rat_t                 rat                    ///< [IN] The Radio Access Technology
)
{
    pa_mrc_ScanInformation_t* scanInformationPtr = le_ref_Lookup(ScanInformationRefMap,
                                                                 scanInformationRef);

    if (scanInformationPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", scanInformationRef);
        return LE_FAULT;
    }

    uint32_t paRat = 0;
    if ( pa_mrc_GetScanInformationRat(scanInformationPtr,&paRat) != LE_OK )
    {
        LE_WARN("Could not get rat scan information");
        return false;
    }

    return (rat == paRat);
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to check if a cellular network is currently in use.
 *
 * @return true     The network is in use
 * @return false    The network is not in use
 *
 * @note
 *      On failure, the process exits, so you don't have to worry about checking the returned
 *      reference for validity.
 */
//--------------------------------------------------------------------------------------------------
bool le_mrc_IsCellularNetworkInUse
(
    le_mrc_ScanInformation_Ref_t scanInformationRef    ///< [IN] Scan information reference
)
{
    pa_mrc_ScanInformation_t* scanInformationPtr = le_ref_Lookup(ScanInformationRefMap,
                                                                 scanInformationRef);

    if (scanInformationPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", scanInformationRef);
        return false;
    }

    bool used;
    if ( pa_mrc_GetScanInformationInUse(scanInformationPtr,&used) != LE_OK)
    {
        LE_WARN("Could not retrieved Network in use status for %p!", scanInformationRef);
        return false;
    }

    return used;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to check if a cellular network is available.
 *
 * @return true     The network is available
 * @return false    The network is not available
 *
 * @note
 *      On failure, the process exits, so you don't have to worry about checking the returned
 *      reference for validity.
 */
//--------------------------------------------------------------------------------------------------
bool le_mrc_IsCellularNetworkAvailable
(
    le_mrc_ScanInformation_Ref_t scanInformationRef    ///< [IN] Scan information reference
)
{
    pa_mrc_ScanInformation_t* scanInformationPtr = le_ref_Lookup(ScanInformationRefMap,
                                                                 scanInformationRef);

    if (scanInformationPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", scanInformationRef);
        return false;
    }

    bool available;
    if ( pa_mrc_GetScanInformationAvailable(scanInformationPtr,&available) != LE_OK)
    {
        LE_WARN("Could not retrieved Network in use status for %p!", scanInformationRef);
        return false;
    }

    return available;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to check if a cellular network is currently in home mode.
 *
 * @return true     The network is home
 * @return false    The network is roaming
 *
 * @note
 *      On failure, the process exits, so you don't have to worry about checking the returned
 *      reference for validity.
 */
//--------------------------------------------------------------------------------------------------
bool le_mrc_IsCellularNetworkHome
(
    le_mrc_ScanInformation_Ref_t scanInformationRef    ///< [IN] Scan information reference
)
{
    pa_mrc_ScanInformation_t* scanInformationPtr = le_ref_Lookup(ScanInformationRefMap,
                                                                 scanInformationRef);

    if (scanInformationPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", scanInformationRef);
        return false;
    }

    bool home;
    if ( pa_mrc_GetScanInformationHome(scanInformationPtr,&home) != LE_OK)
    {
        LE_WARN("Could not retrieved Network home status for %p!",scanInformationRef);
        return false;
    }

    return home;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to check if a cellular network is forbidden by the operator.
 *
 * @return true     The network is forbidden
 * @return false    The network is allowed
 *
 * @note
 *      On failure, the process exits, so you don't have to worry about checking the returned
 *      reference for validity.
 */
//--------------------------------------------------------------------------------------------------
bool le_mrc_IsCellularNetworkForbidden
(
    le_mrc_ScanInformation_Ref_t scanInformationRef    ///< [IN] Scan information reference
)
{
    pa_mrc_ScanInformation_t* scanInformationPtr = le_ref_Lookup(ScanInformationRefMap,
                                                                 scanInformationRef);

    if (scanInformationPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", scanInformationRef);
        return false;
    }

    bool forbidden;
    if ( pa_mrc_GetScanInformationForbidden(scanInformationPtr,&forbidden) != LE_OK)
    {
        LE_WARN("Could not retrieved Network forbidden status for %p!",scanInformationRef);
        return false;
    }

    return forbidden;
}


