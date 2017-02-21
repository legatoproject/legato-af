/** @file le_mrc.c
 *
 * This file contains the data structures and the source code of the MRC (Modem Radio Control) APIs.
 *
 *
 * The implementation of @c le_mrc_PerformCellularNetworkScan() and @c le_mrc_GetNeighborCellsInfo()
 * functions requires the use of lists of safe reference mappings.
 * For instance, @c le_mrc_GetNeighborCellsInfo() returns a safe reference for a @c CellList_t
 * object, this object contains a list of cells information and a list of @c CellSafeRef_t objects.
 * One node of @c CellSafeRef_t is a safe reference for an object that gathers the information of
 * one cell. This allows to have several safe references that point to the the same cell information
 * object. The @c le_mrc_GetFirstNeighborCellInfo() and @c le_mrc_GetNextNeighborCellInfo() return
 * a node of a @c CellSafeRef_t object.
 *
 * We need the extra  @c CellSafeRef_t objects so that we can free up all those safe references
 * when the @c CellList_t object is released without having to multi-pass search the @c CellRefMap.
 *
 * This rationale is also used to implement the @c le_mrc_PerformCellularNetworkScan(),
 * @c le_mrc_GetFirstCellularNetworkScan() and @c le_mrc_GetNextCellularNetworkScan() functions.
 *
 * This rationale is also used to implement the 'PreferredOperator' functions:
 * - @c le_mrc_AddPreferredOperator()
 * - @c le_mrc_RemovePreferredOperator()
 * - @c le_mrc_GetPreferredOperatorsList()
 * - @c le_mrc_GetFirstPreferredOperator()
 * - @c le_mrc_GetNextPreferredOperator()
 * - @c le_mrc_DeletePreferredOperatorsList()
 * - @c le_mrc_GetPreferredOperatorDetails()
 *
 * Copyright (C) Sierra Wireless Inc.
 */


#include "legato.h"
#include "interfaces.h"
#include "pa_mrc.h"
#include "mdmCfgEntries.h"


//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of neighboring cells information we expect to have at one time.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_NUM_NEIGHBORS        6

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of neighboring cells information lists we expect to have at one time.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_NUM_NEIGHBOR_LISTS    5

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of preferred operator lists we expect to have at one time.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_NUM_PREFERRED_OPERATORS_LISTS    2

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of preferred operator information we expect to have at one time.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_NUM_PREFERRED_OPERATORS     100

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
 * Maximum number of Signal Metrics objects we expect to have at one time.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_NUM_METRICS 1

//--------------------------------------------------------------------------------------------------
/**
 * Mutex to prevent race condition with asynchronous functions.
 */
//--------------------------------------------------------------------------------------------------
static pthread_mutex_t RegisteringNetworkMutex = PTHREAD_MUTEX_INITIALIZER;

//--------------------------------------------------------------------------------------------------
/**
 * Macro used to prevent race condition with asynchronous functions.
 */
//--------------------------------------------------------------------------------------------------
#define LOCK()    LE_FATAL_IF((pthread_mutex_lock(&RegisteringNetworkMutex)!=0), \
                               "Could not lock the mutex")
#define UNLOCK()  LE_FATAL_IF((pthread_mutex_unlock(&RegisteringNetworkMutex)!=0), \
                               "Could not unlock the mutex")

//--------------------------------------------------------------------------------------------------
/**
 * MRC command Type.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_MRC_CMD_TYPE_ASYNC_REGISTRATION = 0, ///< Pool and perform a network registration.
    LE_MRC_CMD_TYPE_ASYNC_SCAN         = 1  ///< Pool and perform a cellular network scan.
}
CmdType_t;


//--------------------------------------------------------------------------------------------------
// Data structures.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Neighboring Cells Information safe Reference list structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    void*         safeRef;
    le_dls_Link_t link;
} CellSafeRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * Neighboring Cells Information list structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    int32_t        cellsCount;          // number of detected cells
    le_dls_List_t  paNgbrCellInfoList;  // list of pa_mrc_CellInfo_t
    le_dls_List_t  safeRefCellInfoList; // list of CellSafeRef_t
    le_dls_Link_t *currentLinkPtr;      // link for current CellSafeRef_t reference
} CellList_t;


//--------------------------------------------------------------------------------------------------
/**
 * Preferred Operator safe Reference list structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    void*         safeRef;
    le_dls_Link_t link;
} PreferredOperatorsSafeRef_t;


//--------------------------------------------------------------------------------------------------
/**
 * Preferred Operator list structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    int32_t        opsCount;            // number of Preferred operators
    le_dls_List_t  paPrefOpList;        // list of pa_mrc_PreferredNetworkOperator_t
    le_dls_List_t  safeRefPrefOpList;   // list of safe reference of PreferredOperatorsSafeRef_t
    le_dls_Link_t *currentLinkPtr;      // link for current PreferredOperatorsSafeRef_t reference
} PreferredOperatorsList_t;


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
} ScanInfoSafeRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * List Scan Information structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_dls_List_t  paScanInfoList;      // list of pa_mrc_ScanInformation_t
    le_dls_List_t  safeRefScanInfoList; // list of ScanInfoSafeRef_t
    le_dls_Link_t *currentLink;         // link for iterator
} ScanInfoList_t;

//--------------------------------------------------------------------------------------------------
/**
 * Signal Strength Indication Handler context.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_mrc_SignalStrengthChangeHandlerFunc_t handlerFuncPtr; ///< Handler function.
    void*                                    handlerCtxPtr;  ///< Handler's context.
    le_mrc_Rat_t                             rat;            ///< Radio Access Technology.
    int32_t                                  ssPrevious;     ///< last recorded signal strength.
    int32_t                                  ssThreshold;    ///< Signal strength threshold in dBm.
    le_dls_Link_t                            link;           ///< Object node link.
} SignalStrengthHandlerCtx_t;



//--------------------------------------------------------------------------------------------------
/**
 * MRC selection command structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    CmdType_t       command;          ///< Command Request.
    void           *callBackPtr;      ///< The Callback Response.
    void           *context;          ///< Context.
    union
    {
        struct
        {
            le_mrc_RatBitMask_t ratMask;      ///< ratMask.
        } scan;

        struct
        {
            char  mccStr[LE_MRC_MCC_BYTES];  ///< Mobile Country Code
            char  mncStr[LE_MRC_MNC_BYTES];  ///< Mobile Network Code
        } selection;
    };
} CmdRequest_t;


//--------------------------------------------------------------------------------------------------
// Static declarations.
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
/**
 * Pool for neighboring cells information list.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t CellListPool;

//--------------------------------------------------------------------------------------------------
/**
 * Pool for cell information safe reference.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t  CellInfoSafeRefPool;

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for all neighboring cells information list objects
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t CellListRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for one neighboring cell information objects
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t CellRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Pool for preferred PLMN operators list.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t PrefOpsListPool;

//--------------------------------------------------------------------------------------------------
/**
 * Pool for preferred PLMN operators safe reference.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t  PreferredOperatorsSafeRefPool;

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for Preferred operators list.
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t PreferredOperatorsListRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for Preferred operators.
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t PreferredOperatorsRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Event ID for New Network Registration State notification.
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t NewNetRegStateId;

//--------------------------------------------------------------------------------------------------
/**
 * Memory Pool for Listed ScanInformation.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t  ScanInformationListPool;

//--------------------------------------------------------------------------------------------------
/**
 * Memory Pool for Listed Information structure safe reference.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t  ScanInformationSafeRefPool;

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for Scan Information List.
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t ScanInformationListRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for one Scan Information.
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t ScanInformationRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Event ID for New RAT change notification.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t RatChangeId;

//--------------------------------------------------------------------------------------------------
/**
 * Event ID for New Packet Switched change notification.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t PSChangeId;

//--------------------------------------------------------------------------------------------------
/**
 * Event ID for New Circuit Switched change notification.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t CSChangeId;

//--------------------------------------------------------------------------------------------------
/**
 * Memory Pool for Signal Metrics data.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t  MetricsPool;

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for ignal Metrics.
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t MetricsRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Event IDs for Signal Strength notification.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t GsmSsChangeId;
static le_event_Id_t UmtsSsChangeId;
static le_event_Id_t LteSsChangeId;
static le_event_Id_t CdmaSsChangeId;

//--------------------------------------------------------------------------------------------------
/**
 * Event ID for message sending commands.
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t MrcCommandEventId;

//--------------------------------------------------------------------------------------------------
/**
 * Pool for Preferred network Operator.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t PreferredNetworkOperatorPool;


//--------------------------------------------------------------------------------------------------
/**
 * Test mcc and mnc strings
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on error
 */
//--------------------------------------------------------------------------------------------------
static le_result_t TestMccMnc
(
    const char*   mccPtr,      ///< [IN] Mobile Country Code
    const char*   mncPtr       ///< [IN] Mobile Network Code
)
{
    uint16_t mncLength;

    if (mccPtr == NULL)
    {
        LE_ERROR("mccPtr is NULL !");
        return LE_FAULT;
    }
    if (mncPtr == NULL)
    {
        LE_ERROR("mncPtr is NULL !");
        return LE_FAULT;
    }

    // Check mcc length, mcc is encoded on LE_MRC_MCC_LEN digits
    if (strlen(mccPtr) != LE_MRC_MCC_LEN)
    {
        LE_ERROR(" mcc length error != %d", LE_MRC_MCC_LEN);
        return LE_FAULT;
    }

    // Check mnc length, mnc can be encoded on LE_MRC_MCC_LEN or LE_MRC_MCC_LEN-1 digits
    mncLength = strlen(mncPtr);
    if ((mncLength < (LE_MRC_MNC_LEN-1)) || (mncLength > (LE_MRC_MNC_LEN)))
    {
        LE_ERROR(" mnc length error %d", mncLength);
        return LE_FAULT;
    }

    // Check mcc and mnc digits
    const char* mccMncTab[]= {mccPtr, mncPtr, NULL};
    const char** tmpPtr = mccMncTab;

    while (*tmpPtr)
    {
        uint8_t idx=0;
        do
        {
            if (((*tmpPtr)[idx]<'0') || ((*tmpPtr)[idx]>'9'))
            {
                LE_ERROR("format error %s", *tmpPtr);
                return LE_FAULT;
            }
            idx++;
        } while (idx < strlen((*tmpPtr)));

        tmpPtr++;
    }
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function to destroy all safeRef elements in the CellInfoSafeRef list.
 *
 */
//--------------------------------------------------------------------------------------------------
static void DeleteCellInfoSafeRefList
(
    le_dls_List_t* listPtr
)
{
    CellSafeRef_t* nodePtr;
    le_dls_Link_t *linkPtr;

    while ((linkPtr=le_dls_Pop(listPtr)) != NULL)
    {
        nodePtr = CONTAINER_OF(linkPtr, CellSafeRef_t, link);
        le_ref_DeleteRef(CellRefMap, nodePtr->safeRef);
        le_mem_Release(nodePtr);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Function to destroy all safeRef elements in the PreferredOperatorsRefMap list.
 *
 */
//--------------------------------------------------------------------------------------------------
static void DeletePreferredOperatorsSafeRefList
(
    le_dls_List_t* listPtr
)
{
    PreferredOperatorsSafeRef_t* nodePtr;
    le_dls_Link_t* linkPtr;

    while ((linkPtr = le_dls_Pop(listPtr)) != NULL)
    {
        nodePtr = CONTAINER_OF(linkPtr, PreferredOperatorsSafeRef_t, link);
        le_ref_DeleteRef(PreferredOperatorsRefMap, nodePtr->safeRef);
        le_mem_Release(nodePtr);
    }
}

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

//--------------------------------------------------------------------------------------------------
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
    ScanInfoSafeRef_t* nodePtr;
    le_dls_Link_t *linkPtr;

    while ((linkPtr=le_dls_Pop(listPtr)) != NULL)
    {
        nodePtr = CONTAINER_OF(linkPtr, ScanInfoSafeRef_t, link);
        le_ref_DeleteRef(ScanInformationRefMap, nodePtr->safeRef);
        le_mem_Release(nodePtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * The first-layer Radio Access Technology Change Handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static void FirstLayerRatChangeHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    le_mrc_Rat_t*           ratPtr = reportPtr;
    le_mrc_RatChangeHandlerFunc_t clientHandlerFunc = secondLayerHandlerFunc;

    clientHandlerFunc(*ratPtr, le_event_GetContextPtr());

    // The reportPtr is a reference counted object, so need to release it
    le_mem_Release(reportPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * The first-layer Packet Switched Change Handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static void FirstLayerPSChangeHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    le_mrc_ServiceState_t*         serviceStatePtr = (le_mrc_ServiceState_t*) reportPtr;
    le_mrc_PacketSwitchedChangeHandlerFunc_t clientHandlerFunc =
                    (le_mrc_PacketSwitchedChangeHandlerFunc_t) secondLayerHandlerFunc;

    clientHandlerFunc(*serviceStatePtr, le_event_GetContextPtr());

    // The reportPtr is a reference counted object, so need to release it
    le_mem_Release(reportPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * The first-layer Circuit Switched Change Handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static void FirstLayerCSChangeHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    le_mrc_ServiceState_t*         serviceStatePtr = (le_mrc_ServiceState_t*) reportPtr;
    le_mrc_CircuitSwitchedChangeHandlerFunc_t clientHandlerFunc =
                    (le_mrc_CircuitSwitchedChangeHandlerFunc_t) secondLayerHandlerFunc;

    clientHandlerFunc(*serviceStatePtr, le_event_GetContextPtr());

    // The reportPtr is a reference counted object, so need to release it
    le_mem_Release(reportPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Radio Access Technology Change handler function.
 *
 */
//--------------------------------------------------------------------------------------------------
static void RatChangeHandler
(
    le_mrc_Rat_t* ratPtr
)
{
    LE_DEBUG("Handler Function called with RAT %d", *ratPtr);

    // Notify all the registered client's handlers
    le_event_ReportWithRefCounting(RatChangeId, ratPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Circuit Switched Change handler function.
 *
 */
//--------------------------------------------------------------------------------------------------
static void CSChangeHandler
(
    le_mrc_ServiceState_t* serviceStatePtr
)
{
    LE_DEBUG("Handler Function called with CS %d", *serviceStatePtr);

    // Notify all the registered client's handlers
    le_event_ReportWithRefCounting(CSChangeId, serviceStatePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Packet Switched Change handler function.
 *
 */
//--------------------------------------------------------------------------------------------------
static void PSChangeHandler
(
    le_mrc_ServiceState_t* serviceStatePtr
)
{
    LE_DEBUG("Handler Function called with PS %d", *serviceStatePtr);

    // Notify all the registered client's handlers
    le_event_ReportWithRefCounting(PSChangeId, serviceStatePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * The first-layer Signal Strength Change Handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static void FirstLayerSsChangeHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    pa_mrc_SignalStrengthIndication_t*    ssIndPtr = (pa_mrc_SignalStrengthIndication_t*)reportPtr;
    le_mrc_SignalStrengthChangeHandlerFunc_t clientHandlerFunc = secondLayerHandlerFunc;

    clientHandlerFunc(ssIndPtr->ss, le_event_GetContextPtr());

    // The reportPtr is a reference counted object, so need to release it
    le_mem_Release(reportPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * The first-layer Signal Strength Change Handler for GSM.
 *
 */
//--------------------------------------------------------------------------------------------------
static void FirstLayerGsmSsChangeHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    FirstLayerSsChangeHandler(reportPtr, secondLayerHandlerFunc);
}

//--------------------------------------------------------------------------------------------------
/**
 * The first-layer Signal Strength Change Handler for UMTS.
 *
 */
//--------------------------------------------------------------------------------------------------
static void FirstLayerUmtsSsChangeHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    FirstLayerSsChangeHandler(reportPtr, secondLayerHandlerFunc);
}

//--------------------------------------------------------------------------------------------------
/**
 * The first-layer Signal Strength Change Handler for LTE.
 *
 */
//--------------------------------------------------------------------------------------------------
static void FirstLayerLteSsChangeHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    FirstLayerSsChangeHandler(reportPtr, secondLayerHandlerFunc);
}

//--------------------------------------------------------------------------------------------------
/**
 * The first-layer Signal Strength Change Handler for CDMA.
 *
 */
//--------------------------------------------------------------------------------------------------
static void FirstLayerCdmaSsChangeHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    FirstLayerSsChangeHandler(reportPtr, secondLayerHandlerFunc);
}

//--------------------------------------------------------------------------------------------------
/**
 * The Signal Strength Indication Handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static void SignalStrengthIndHandlerFunc
(
    pa_mrc_SignalStrengthIndication_t* ssIndPtr
)
{
    LE_INFO("Signal Strength Ind Handler called with RAT.%d and ss.%d",
             ssIndPtr->rat, ssIndPtr->ss);

    switch(ssIndPtr->rat)
    {
        case LE_MRC_RAT_GSM:
            le_event_ReportWithRefCounting(GsmSsChangeId, ssIndPtr);
            break;

        case LE_MRC_RAT_UMTS:
            le_event_ReportWithRefCounting(UmtsSsChangeId, ssIndPtr);
            break;

        case LE_MRC_RAT_LTE:
            le_event_ReportWithRefCounting(LteSsChangeId, ssIndPtr);
            break;

        case LE_MRC_RAT_CDMA:
            le_event_ReportWithRefCounting(CdmaSsChangeId, ssIndPtr);
            break;

        case LE_MRC_RAT_UNKNOWN:
        default:
            break ;
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Handler to process an asynchronous command
 */
//--------------------------------------------------------------------------------------------------
static void ProcessMrcCommandEventHandler
(
    void* msgCommand
)
{
    le_result_t res;

    void * ctxPtr = ((CmdRequest_t*) msgCommand)->context;
    CmdType_t command = ((CmdRequest_t*) msgCommand)->command;

    if( command == LE_MRC_CMD_TYPE_ASYNC_REGISTRATION)
    {
        char * mccStr = ((CmdRequest_t*) msgCommand)->selection.mccStr;
        char * mncStr = ((CmdRequest_t*) msgCommand)->selection.mncStr;
        le_mrc_ManualSelectionHandlerFunc_t handlerFunc =
                           ((CmdRequest_t*) msgCommand)->callBackPtr;

        LE_DEBUG("Register plmn (%s,%s)", mccStr, mncStr);

        if (LE_OK != TestMccMnc(mccStr, mncStr))
        {
            LE_WARN("Invalid mcc or mnc");
            return;
        }

        LOCK();
        res = pa_mrc_RegisterNetwork(mccStr, mncStr);
        UNLOCK();

        // Check if a handler function is available.
        if (handlerFunc)
        {
            LE_DEBUG("Calling handler (%p), Status %d", handlerFunc, res);
            handlerFunc (res, ctxPtr);
        }
        else
        {
            LE_WARN("No handler function, status %d!!", res);
        }
    }
    else if(command == LE_MRC_CMD_TYPE_ASYNC_SCAN)
    {
        le_mrc_CellularNetworkScanHandlerFunc_t handlerFunc =
                        ((CmdRequest_t*) msgCommand)->callBackPtr;
        le_mrc_RatBitMask_t ratMask =
                        ((CmdRequest_t*) msgCommand)->scan.ratMask;

        ScanInfoList_t* newScanInformationListPtr = NULL;
        le_mrc_ScanInformationListRef_t scanInformationListRef = NULL;

        newScanInformationListPtr = le_mem_ForceAlloc(ScanInformationListPool);
        newScanInformationListPtr->paScanInfoList = LE_DLS_LIST_INIT;
        newScanInformationListPtr->safeRefScanInfoList = LE_DLS_LIST_INIT;
        newScanInformationListPtr->currentLink = NULL;

        LOCK();
        res = pa_mrc_PerformNetworkScan(ratMask, PA_MRC_SCAN_PLMN,
            &(newScanInformationListPtr->paScanInfoList));
        UNLOCK();

        if (res != LE_OK)
        {
            le_mem_Release(newScanInformationListPtr);
        }
        else
        {
            scanInformationListRef =
                     le_ref_CreateRef(ScanInformationListRefMap, newScanInformationListPtr);
        }

        // Check if a handler function is available.
        if (handlerFunc)
        {
            LE_DEBUG("Sending Scan information list ref (%p)", scanInformationListRef);
            handlerFunc (scanInformationListRef, ctxPtr);
        }
        else
        {
            LE_WARN("No handler function, status %d!!", res);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * This thread does the actual work of Pool and perform a registration or a cellular network scan.
 */
//--------------------------------------------------------------------------------------------------
static void* MrcCommandThread
(
    void* contextPtr
)
{
    le_sem_Ref_t initSemaphore = (le_sem_Ref_t)contextPtr;

    // Register for MRC command events
    le_event_AddHandler("ProcessMrcCommandHandler",
                        MrcCommandEventId,
                        ProcessMrcCommandEventHandler);

    le_sem_Post(initSemaphore);

    // Run the event loop
    le_event_RunLoop();
    return NULL;
}



//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to remove a mobile country/network code in the list
 *
 * @return
 *      - LE_OK             On success
 *      - LE_NOT_FOUND      Not present in preferred PLMN list
 *      - LE_FAULT          For all other errors
 */
//--------------------------------------------------------------------------------------------------
static le_result_t RemovePreferredOperators
(
    le_dls_List_t      *preferredOperatorsListPtr,   ///< [IN] List of preferred network operator
    const char*         mccPtr,                      ///< [IN] Mobile Country Code
    const char*         mncPtr                       ///< [IN] Mobile Network Code
)
{
    pa_mrc_PreferredNetworkOperator_t* nodePtr;
    le_dls_Link_t *linkPtr;

    LE_DEBUG("Removing [%s,%s]", mccPtr, mncPtr);

    linkPtr = le_dls_Peek(preferredOperatorsListPtr);

    if (linkPtr == NULL)
    {
        return LE_FAULT;
    }

    while (linkPtr != NULL)
    {
        // Get the node from MsgList
        nodePtr = CONTAINER_OF(linkPtr, pa_mrc_PreferredNetworkOperator_t, link);

        if ( (strncmp(nodePtr->mobileCode.mcc, mccPtr, LE_MRC_MCC_BYTES) == 0)
                        && (strncmp(nodePtr->mobileCode.mnc, mncPtr, LE_MRC_MNC_BYTES) == 0))
        {
            LE_DEBUG("Found [%s,%s] in the list, Removed it", mccPtr, mncPtr);
            le_dls_Remove(preferredOperatorsListPtr,linkPtr);
            le_mem_Release(nodePtr);
            return LE_OK;
        }

        // Get Next element
        linkPtr = le_dls_PeekNext(preferredOperatorsListPtr, linkPtr);
    }

    return LE_NOT_FOUND;
}



//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to add a new mobile country/network code in the list
 *
 * @return
 *  - LE_FAULT         Function failed.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t AddPreferredOperators
(
    le_dls_List_t*      preferredOperatorsListPtr,   ///< [OUT] List of preferred network operator
    const char*         mccPtr,                      ///< [IN] Mobile Country Code
    const char*         mncPtr,                      ///< [IN] Mobile Network Code
    le_mrc_RatBitMask_t ratMask                      ///< [IN] Radio Access Technology mask
)
{
    pa_mrc_PreferredNetworkOperator_t* nodePtr;
    le_dls_Link_t *linkPtr;

    LE_DEBUG("Adding [%s,%s] = 0x%.04"PRIX16, mccPtr, mncPtr, ratMask);

    linkPtr = le_dls_Peek(preferredOperatorsListPtr);

    while (linkPtr != NULL)
    {
        // Get the node from MsgList
        nodePtr = CONTAINER_OF(linkPtr, pa_mrc_PreferredNetworkOperator_t, link);

        if ( (strncmp(nodePtr->mobileCode.mcc, mccPtr, LE_MRC_MCC_BYTES) == 0) &&
             (strncmp(nodePtr->mobileCode.mnc, mncPtr, LE_MRC_MNC_BYTES) == 0))
        {
            if (nodePtr->ratMask != ratMask)
            {
                uint8_t bitMask = 0x00;
                bitMask = (uint8_t)(nodePtr->ratMask | ratMask);
                LE_DEBUG("Found [%s,%s] in the list, change ratMask from %d to %d",
                                mccPtr, mncPtr, nodePtr->ratMask, bitMask);
                nodePtr->ratMask = (le_mrc_RatBitMask_t)bitMask;
            }
            else
            {
                LE_DEBUG("Found [%s,%s] in the list, no change", mccPtr, mncPtr);
            }
            return LE_OK;
        }

        // Get Next element
        linkPtr = le_dls_PeekNext(preferredOperatorsListPtr, linkPtr);
    }

    nodePtr = le_mem_ForceAlloc(PreferredNetworkOperatorPool);
    if (nodePtr == NULL)
    {
        return LE_FAULT;
    }

    le_utf8_Copy(nodePtr->mobileCode.mcc, mccPtr, LE_MRC_MCC_BYTES, NULL);
    le_utf8_Copy(nodePtr->mobileCode.mnc, mncPtr, LE_MRC_MNC_BYTES, NULL);
    nodePtr->ratMask = ratMask;
    nodePtr->link = LE_DLS_LINK_INIT;

    le_dls_Queue(preferredOperatorsListPtr, &(nodePtr->link));

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to clear the preferred list.
 *
 * @return
 *      - LE_OK             on success
 *      - LE_FAULT          for all other errors
 */
//--------------------------------------------------------------------------------------------------
static void DeletePreferredOperatorsList
(
    le_dls_List_t      *preferredOperatorsListPtr ///< [IN] List of preferred network operator
)
{
    pa_mrc_PreferredNetworkOperator_t* nodePtr;
    le_dls_Link_t *linkPtr;

    while ((linkPtr=le_dls_Pop(preferredOperatorsListPtr)) != NULL)
    {
        nodePtr = CONTAINER_OF(linkPtr, pa_mrc_PreferredNetworkOperator_t, link);
        le_mem_Release(nodePtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the current preferred operators list.
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetPreferredOperatorsList
(
    le_dls_List_t*   preferredOperatorListPtr,    ///< [IN/OUT] The preferred operators list.
    bool  plmnStatic,      ///< [IN] Include Static preferred Operators.
    bool  plmnUser,        ///< [IN] Include Users preferred Operators.
    int32_t* nbItem        ///< [OUT] number of Preferred operator found if success.
)
{
    int32_t total = 0;
    le_result_t res;

    if (preferredOperatorListPtr == NULL)
    {
        LE_FATAL("preferredOperatorListPtr is NULL !");
    }

    if(pa_mrc_CountPreferredOperators(plmnStatic, plmnUser, &total) == LE_OK)
    {
        if(total > 0)
        {
            size_t i;
            pa_mrc_PreferredNetworkOperator_t mrcPrefNetworkPtr[total];
            memset(mrcPrefNetworkPtr, 0 , sizeof(pa_mrc_PreferredNetworkOperator_t)*total);

            res = pa_mrc_GetPreferredOperators(mrcPrefNetworkPtr, plmnStatic, plmnUser, &total);

            if (res == LE_OK)
            {
                for (i=0; i < total; i++)
                {
                    pa_mrc_PreferredNetworkOperator_t* prefNetworkPtr =
                                    (pa_mrc_PreferredNetworkOperator_t*)
                                    le_mem_ForceAlloc(PreferredNetworkOperatorPool);

                    memcpy(prefNetworkPtr, &mrcPrefNetworkPtr[i],
                        sizeof(pa_mrc_PreferredNetworkOperator_t));

                    prefNetworkPtr->index = i;
                    prefNetworkPtr->link = LE_DLS_LINK_INIT;
                    le_dls_Queue(preferredOperatorListPtr, &(prefNetworkPtr->link));

                    LE_DEBUG("3GPP [%d] MCC.%s MNC.%s QmiRat %04X", prefNetworkPtr->index,
                        prefNetworkPtr->mobileCode.mcc,
                        prefNetworkPtr->mobileCode.mnc,
                        prefNetworkPtr->ratMask);
                }
            }
        }
        *nbItem = total;
        return LE_OK;
    }

    return LE_FAULT;
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

    ScanInformationListPool = le_mem_CreatePool("ScanInformationListPool",
                                                sizeof(ScanInfoList_t));

    ScanInformationSafeRefPool = le_mem_CreatePool("ScanInformationSafeRefPool",
                                                   sizeof(ScanInfoSafeRef_t));

    // Create the Safe Reference Map to use for Scan Information List object Safe References.
    ScanInformationListRefMap = le_ref_CreateMap("ScanInformationListMap", MRC_MAX_SCANLIST);

    // Create the Safe Reference Map to use for Scan Information List object Safe References.
    ScanInformationRefMap = le_ref_CreateMap("ScanInformationMap", MRC_MAX_SCAN);

    // Create the pool for cells information list.
    CellListPool = le_mem_CreatePool("CellListPool", sizeof(CellList_t));

    // Create the pool for Preferred cells information list.
    PrefOpsListPool = le_mem_CreatePool("PrefOpListPool", sizeof(PreferredOperatorsList_t));

    // Create the Safe Reference Map to use for neighboring cells information object Safe
    // References.
    CellRefMap = le_ref_CreateMap("CellInfoCellMap", MAX_NUM_NEIGHBORS);

    // Create the pool for cells information safe ref list.
    CellInfoSafeRefPool = le_mem_CreatePool("CellInfoSafeRefPool", sizeof(CellSafeRef_t));

    // Create the Safe Reference Map to use for neighboring cells information list object Safe
    //  References.
    CellListRefMap = le_ref_CreateMap("CellListRefMap", MAX_NUM_NEIGHBOR_LISTS);

    // Create the Safe Reference Map to use for preferred Operators information object Safe
    //  References.
    PreferredOperatorsRefMap = le_ref_CreateMap("PreferredOperatorsMap",
                    MAX_NUM_PREFERRED_OPERATORS);

    // Create the pool for preferred Operators safe ref list.
    PreferredOperatorsSafeRefPool = le_mem_CreatePool("PreferredOperatorsSafeRefPool",
                    sizeof(CellSafeRef_t));

    // Create the Safe Reference Map to use for preferred Operators information list object Safe
    //  References.
    PreferredOperatorsListRefMap = le_ref_CreateMap("PreferredOperatorsListRefMap",
                    MAX_NUM_PREFERRED_OPERATORS_LISTS);

    PreferredNetworkOperatorPool = le_mem_CreatePool("PreferredNetworkOperatorPool",
                                             sizeof(pa_mrc_PreferredNetworkOperator_t));

    // Create the pool for Signal Metrics.
    MetricsPool = le_mem_CreatePool("MetricsPool", sizeof(pa_mrc_SignalMetrics_t));

    // Create the Safe Reference Map to use for Signal Metrics object Safe References.
    MetricsRefMap = le_ref_CreateMap("MetricsRefMap", MAX_NUM_METRICS);

    // Create an event Id for new Network Registration State notification
    NewNetRegStateId = le_event_CreateIdWithRefCounting("NewNetRegState");

    // Create an event Id for RAT change notification
    RatChangeId = le_event_CreateIdWithRefCounting("RatChange");

    // Create an event Id for Circuit Switched change notification
    CSChangeId = le_event_CreateIdWithRefCounting("CSChange");
    // Create an event Id for Packet Switched change notification
    PSChangeId = le_event_CreateIdWithRefCounting("PSChange");

    // Create an event Id for Signal Strength change notification
    GsmSsChangeId = le_event_CreateIdWithRefCounting("GsmSsChange");
    UmtsSsChangeId = le_event_CreateIdWithRefCounting("UmtsSsChange");
    LteSsChangeId = le_event_CreateIdWithRefCounting("LteSsChange");
    CdmaSsChangeId = le_event_CreateIdWithRefCounting("CdmaSsChange");

    // Register a handler function for new Registration State indication
    pa_mrc_AddNetworkRegHandler(NewRegStateHandler);

    // Register a handler function for new RAT change indication
    pa_mrc_SetRatChangeHandler(RatChangeHandler);

    // Register a handler function for new Circuit Switched change indication
    pa_mrc_SetCSChangeHandler(CSChangeHandler);

    // Register a handler function for new Packet Switched change indication
    pa_mrc_SetPSChangeHandler(PSChangeHandler);

    // Register a handler function for Signal Strength change indication
    pa_mrc_AddSignalStrengthIndHandler(SignalStrengthIndHandlerFunc, NULL);

    MrcCommandEventId = le_event_CreateId("CommandEvent", sizeof(CmdRequest_t));

    // initSemaphore is used to wait for MrcCommandThread() execution. It ensures that the thread is
    // ready when we exit from le_mrc_Init().
    le_sem_Ref_t initSemaphore = le_sem_Create("InitSem", 0);
    le_thread_Start(le_thread_Create("MrcManualSelectionThread",
                                     MrcCommandThread,
                                     (void*)initSemaphore));
    le_sem_Wait(initSemaphore);
    le_sem_Delete(initSemaphore);

    // Get & Set the Network registration state notification
    LE_DEBUG("Get the Network registration state notification configuration");
    result=pa_mrc_GetNetworkRegConfig(&setting);
    if ((result != LE_OK) || (setting == PA_MRC_DISABLE_REG_NOTIFICATION))
    {
        LE_ERROR_IF((result != LE_OK),
                    "Fails to get the Network registration state notification configuration");

        LE_INFO("Enable the Network registration state notification");
        pa_mrc_ConfigureNetworkReg(PA_MRC_ENABLE_REG_NOTIFICATION);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Enable the automatic Selection Register mode.
 *
 * @return
 *  - LE_FAULT  Function failed.
 *  - LE_OK     Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_SetAutomaticRegisterMode
(
    void
)
{
    if ( pa_mrc_SetAutomaticNetworkRegistration() != LE_OK )
    {
        LE_ERROR("Cannot set the Automatic Network Registration");
        return LE_FAULT;
    }
    else
    {
        return LE_OK;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the manual Selection Register mode with the MCC/MNC parameters.
 *
 * @return
 *  - LE_FAULT  Function failed.
 *  - LE_OK     Function succeeded.
 *
 * @note If strings are not set, too long (bigger than LE_MRC_MCC_LEN/LE_MRC_MNC_LEN digits), or too
 *       short (less than LE_MRC_MCC_LEN/LE_MRC_MNC_LEN-1 digits) it's a fatal error, the function
 *       won't return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_SetManualRegisterMode
(
    const char*      mccPtr,   ///< [IN] Mobile Country Code
    const char*      mncPtr    ///< [IN] Mobile Network Code
)
{
    le_result_t res;

    if (LE_OK != TestMccMnc(mccPtr, mncPtr))
    {
        LE_KILL_CLIENT("Invalid mcc or mnc");
        return LE_FAULT;
    }

    LOCK();
    res = pa_mrc_RegisterNetwork(mccPtr, mncPtr);
    UNLOCK();

    if ( res != LE_OK )
    {
        LE_ERROR("Cannot Register to Network [%s,%s]", mccPtr, mncPtr);
        return LE_FAULT;
    }
    else
    {
        return LE_OK;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the manual selection register mode asynchronously. This function is not blocking,
 *  the response will be returned with a handler function.
 *
 * @note If strings are not set, too long (bigger than LE_MRC_MCC_LEN/LE_MRC_MNC_LEN digits), or too
 *       short (less than LE_MRC_MCC_LEN/LE_MRC_MNC_LEN-1 digits) it's a fatal error, the function
 *       won't return.
 */
//--------------------------------------------------------------------------------------------------
void le_mrc_SetManualRegisterModeAsync
(
    const char* mccPtr,
        ///< [IN]
        ///< Mobile Country Code

    const char* mncPtr,
        ///< [IN]
        ///< Mobile Network Code

    le_mrc_ManualSelectionHandlerFunc_t handlerPtr,
        ///< [IN]

    void* contextPtr
        ///< [IN]
)
{
    CmdRequest_t cmd;

    if (LE_OK != TestMccMnc(mccPtr, mncPtr))
    {
        LE_KILL_CLIENT("Invalid mcc or mnc");
    }

    memset(&cmd, 0, sizeof(CmdRequest_t));
    cmd.context = contextPtr;
    cmd.callBackPtr = handlerPtr;
    cmd.command = LE_MRC_CMD_TYPE_ASYNC_REGISTRATION;
    strncpy(cmd.selection.mccStr, mccPtr, LE_MRC_MCC_LEN);
    strncpy(cmd.selection.mncStr, mncPtr, LE_MRC_MNC_LEN);

    // Sending manual Selection command
    LE_DEBUG("Send manual Selection command (mcc:%s, mnc:%s)",
             cmd.selection.mccStr,
             cmd.selection.mncStr);
    le_event_Report(MrcCommandEventId, &cmd, sizeof(cmd));
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the selected Registration mode.
 *
 * @return
 *  - LE_FAULT  Function failed.
 *  - LE_OK     Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_GetRegisterMode
(
    bool*   isManualPtr,  ///< [OUT] true if the scan mode is manual, false if it is automatic.
    char*   mccPtr,       ///< [OUT] Mobile Country Code
    size_t  mccPtrSize,   ///< [IN] mccPtr buffer size
    char*   mncPtr,       ///< [OUT] Mobile Network Code
    size_t  mncPtrSize    ///< [IN] mncPtr buffer size
)
{
    char mcc[LE_MRC_MCC_BYTES] = {0};
    char mnc[LE_MRC_MNC_BYTES] = {0};

    if (isManualPtr == NULL)
    {
        LE_KILL_CLIENT("isManualPtr is NULL !");
        return LE_FAULT;
    }

    le_result_t res = pa_mrc_GetNetworkRegistrationMode(isManualPtr,
                    mcc, LE_MRC_MCC_BYTES, mnc,  LE_MRC_MNC_BYTES);

    if ( res != LE_OK )
    {
        LE_ERROR("Cannot not get RegisterMode");
        return LE_FAULT;
    }
    else
    {
        if (mccPtr == NULL)
        {
            LE_KILL_CLIENT("mccPtr is NULL !");
            return LE_FAULT;
        }

        if (mncPtr == NULL)
        {
            LE_KILL_CLIENT("mncPtr is NULL !");
            return LE_FAULT;
        }

        if(mccPtrSize < LE_MRC_MCC_BYTES)
        {
            LE_KILL_CLIENT("mccPtrSize < %d", LE_MRC_MCC_BYTES);
            return LE_FAULT;
        }

        if(mncPtrSize < LE_MRC_MNC_BYTES)
        {
            LE_KILL_CLIENT("mccPtrSize < %d", LE_MRC_MNC_BYTES);
            return LE_FAULT;
        }
        le_utf8_Copy(mccPtr, mcc, LE_MRC_MCC_BYTES, NULL);
        le_utf8_Copy(mncPtr, mnc, LE_MRC_MCC_BYTES, NULL);

        return LE_OK;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the platform specific network registration error code.
 *
 * @return the platform specific registration error code
 *
 */
//--------------------------------------------------------------------------------------------------
int32_t le_mrc_GetPlatformSpecificRegistrationErrorCode
(
    void
)
{
    return pa_mrc_GetPlatformSpecificRegistrationErrorCode();
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the Radio Access Technology preferences by using a bit mask.
 *
 * @return
 *  - LE_FAULT  Function failed.
 *  - LE_OK     Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_SetRatPreferences
(
    le_mrc_RatBitMask_t ratMask ///< [IN] Bit mask for the Radio Access Technology preferences.
)
{
    if (ratMask == 0)
    {
        LE_ERROR("Rat preference not selected !");
        return LE_FAULT;
    }
    else if (ratMask == LE_MRC_BITMASK_RAT_ALL)
    {
        if ( pa_mrc_SetAutomaticRatPreference() != LE_OK )
        {
            LE_ERROR("Unable to set the Automatic Radio Access Technology preferences.");
            return LE_FAULT;
        }
    }
    else
    {
        if ( pa_mrc_SetRatPreferences(ratMask) != LE_OK )
        {
            LE_ERROR("Unable to set the Radio Access Technology preferences.");
            return LE_FAULT;
        }
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the Radio Access Technology preferences
 *
 * @return
 * - LE_FAULT  Function failed.
 * - LE_OK     Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_GetRatPreferences
(
    le_mrc_RatBitMask_t* ratMaskPtr ///< [OUT] Bit mask for the Radio Access Technology preferences.
)
{
    if (ratMaskPtr == NULL)
    {
        LE_KILL_CLIENT("ratMaskPtr is NULL !");
        return LE_FAULT;
    }

    if ( pa_mrc_GetRatPreferences(ratMaskPtr) != LE_OK )
    {
        LE_ERROR("Unable to get the Radio Access Technology preferences.");
        return LE_FAULT;
    }
    else
    {
        return LE_OK;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the 2G/3G Band preferences by using a bit mask.
 *
 * @return
 *  - LE_FAULT  Function failed.
 *  - LE_OK     Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_SetBandPreferences
(
    le_mrc_BandBitMask_t bandMask ///< [IN] Bit mask for 2G/3G Band preferences.
)
{
    if (bandMask == 0)
    {
        LE_ERROR("No Band Selected");
        return LE_FAULT;
    }

    if ( pa_mrc_SetBandPreferences(bandMask) != LE_OK )
    {
        LE_ERROR("Unable to set the 2G/3G Band preferences.");
        return LE_FAULT;
    }
    else
    {
        return LE_OK;
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Get the Bit mask for 2G/3G Band preferences.
 *
 * @return
 *  - LE_FAULT  Function failed.
 *  - LE_OK     Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_GetBandPreferences
(
    le_mrc_BandBitMask_t* bandMaskPtr ///< [OUT] Bit mask for 2G/3G Band preferences.
)
{
    if (bandMaskPtr == NULL)
    {
        LE_KILL_CLIENT("bandMaskPtr is NULL !");
        return LE_FAULT;
    }

    if ( pa_mrc_GetBandPreferences(bandMaskPtr) != LE_OK )
    {
        LE_ERROR("Unable to get band Preferences.");
        return LE_FAULT;
    }
    else
    {
        return LE_OK;
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Set the LTE Band preferences by using a bit mask.
 *
 * @return
 *  - LE_FAULT  Function failed.
 *  - LE_OK     Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_SetLteBandPreferences
(
    le_mrc_LteBandBitMask_t bandMask  ///< [IN] Bit mask for LTE Band preferences.
)
{
    if (bandMask == 0)
    {
        LE_ERROR("No Band Selected");
        return LE_FAULT;
    }

    if ( pa_mrc_SetLteBandPreferences(bandMask) != LE_OK )
    {
        LE_ERROR("Unable to set the LTE Band preferences.");
        return LE_FAULT;
    }
    else
    {
        return LE_OK;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the Bit mask for LTE Band preferences.
 *
 * @return
 *  - LE_FAULT  Function failed.
 *  - LE_OK     Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_GetLteBandPreferences
(
    le_mrc_LteBandBitMask_t* bandMaskPtr  ///< [OUT] Bit mask for LTE Band preferences.
)
{
    if (bandMaskPtr == NULL)
    {
        LE_KILL_CLIENT("bandMaskPtr is NULL !");
        return LE_FAULT;
    }

    if ( pa_mrc_GetLteBandPreferences(bandMaskPtr) != LE_OK )
    {
        LE_ERROR("Unable to get LTE band Preferences.");
        return LE_FAULT;
    }
    else
    {
        return LE_OK;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the TD-SCDMA Band preferences by using a bit mask.
 *
 * @return
 *  - LE_FAULT  Function failed.
 *  - LE_OK     Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_SetTdScdmaBandPreferences
(
    le_mrc_TdScdmaBandBitMask_t bandMask ///< [IN] Bit mask for TD-SCDMA Band preferences.
)
{
    if (bandMask == 0)
    {
        LE_ERROR("No Band Selected");
        return LE_FAULT;
    }

    if ( pa_mrc_SetTdScdmaBandPreferences(bandMask) != LE_OK )
    {
        LE_ERROR("Unable to set the TD-SCDMA Band preferences.");
        return LE_FAULT;
    }
    else
    {
        return LE_OK;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the Bit mask for TD-SCDMA Band preferences.
 *
 * @return
 *  - LE_FAULT  Function failed.
 *  - LE_OK     Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_GetTdScdmaBandPreferences
(
    le_mrc_TdScdmaBandBitMask_t* bandMaskPtr ///< [OUT] Bit mask for TD-SCDMA Band preferences.
)
{
    if (bandMaskPtr == NULL)
    {
        LE_KILL_CLIENT("bandMaskPtr is NULL !");
        return LE_FAULT;
    }

    if ( pa_mrc_GetTdScdmaBandPreferences(bandMaskPtr) != LE_OK )
    {
        LE_ERROR("Unable to get TD-SCDMA Band preferences.");
        return LE_FAULT;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Add a preferred operator by specifying the MCC/MNC and the Radio Access Technology.
 *
 * @return
 *  - LE_UNSUPPORTED   List of User Preferred operators not available.
 *  - LE_FAULT         Function failed.
 *  - LE_BAD_PARAMETER RAT mask is invalid.
 *  - LE_OK            Function succeeded.
 *
 * @note If strings are not set, too long (bigger than LE_MRC_MCC_LEN/LE_MRC_MNC_LEN digits), or too
 *       short (less than LE_MRC_MCC_LEN/LE_MRC_MNC_LEN-1 digits) it's a fatal error, the function
 *       won't return.
 *
 * @note <b>NOT multi-app safe</b>
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_AddPreferredOperator
(
    const char* mccPtr,
        ///< [IN]
        ///< Mobile Country Code

    const char* mncPtr,
        ///< [IN]
        ///< Mobile Network Code

    le_mrc_RatBitMask_t ratMask
        ///< [IN]
        ///< Bit mask for the Radio Access Technology preferences.
)
{
    le_dls_List_t preferredOperatorsList = LE_DLS_LIST_INIT;
    int32_t nbEntries;

    if (LE_OK != TestMccMnc(mccPtr, mncPtr))
    {
        LE_KILL_CLIENT("Invalid mcc or mnc");
        return LE_FAULT;
    }

    /*
     * RATs allowed:
     *   [ LE_MRC_BITMASK_RAT_ALL ]
     * or
     *   [   <LE_MRC_BITMASK_RAT_GSM>  + <LE_MRC_BITMASK_RAT_UMTS>
     *     + <LE_MRC_BITMASK_RAT_LTE>  + <LE_MRC_BITMASK_RAT_CDMA> ]
     */
    if((!ratMask) || (ratMask > LE_MRC_BITMASK_RAT_ALL))
    {
        LE_ERROR("RAT mask is invalid");
        return LE_BAD_PARAMETER;
    }

    if (GetPreferredOperatorsList(&preferredOperatorsList, false, true, &nbEntries) != LE_OK)
    {
        LE_ERROR("No preferred Operator list available!!");
        return LE_UNSUPPORTED;
    }

    LE_WARN_IF((nbEntries == 0), "Preferred PLMN Operator list is empty!");

    if ( AddPreferredOperators(&preferredOperatorsList, mccPtr, mncPtr, ratMask) != LE_OK )
    {
        LE_ERROR("Could not add [%s,%s] into the preferred operator list", mccPtr, mncPtr);
        DeletePreferredOperatorsList(&preferredOperatorsList);
        return LE_FAULT;
    }

    if ( pa_mrc_SavePreferredOperators(&preferredOperatorsList) != LE_OK )
    {
        LE_ERROR("Could not save the preferred operator list");
        DeletePreferredOperatorsList(&preferredOperatorsList);
        return LE_FAULT;
    }

    DeletePreferredOperatorsList(&preferredOperatorsList);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove a preferred operator by specifying the MCC/MNC.
 *
 * @return
 *  - LE_UNSUPPORTED    List of User Preferred operators not available.
 *  - LE_NOT_FOUND      Operator not found in the User Preferred operators list.
 *  - LE_FAULT          Function failed.
 *  - LE_OK             Function succeeded.
 *
 * @note If strings are not set, too long (bigger than LE_MRC_MCC_LEN/LE_MRC_MNC_LEN digits), or too
 *       short (less than LE_MRC_MCC_LEN/LE_MRC_MNC_LEN-1 digits) it's a fatal error, the function
 *       won't return.
 *
 * @note <b>NOT multi-app safe</b>
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_RemovePreferredOperator
(
    const char* mccPtr,
        ///< [IN]
        ///< Mobile Country Code

    const char* mncPtr
        ///< [IN]
        ///< Mobile Network Code
)
{
    le_dls_List_t preferredOperatorsList = LE_DLS_LIST_INIT;
    int32_t nbItem;
    le_result_t res;

    if (LE_OK != TestMccMnc(mccPtr, mncPtr))
    {
        LE_KILL_CLIENT("Invalid mcc or mnc");
        return LE_FAULT;
    }

    res = GetPreferredOperatorsList(&preferredOperatorsList, false, true, &nbItem);
    if (res == LE_OK)
    {
        res = RemovePreferredOperators(&preferredOperatorsList, mccPtr, mncPtr);
        if (res == LE_OK)
        {
            if (pa_mrc_SavePreferredOperators(&preferredOperatorsList) != LE_OK)
            {
                LE_ERROR("Could not save the preferred operator list");
                res = LE_FAULT;
            }
        }
        else
        {
            LE_ERROR("Could not remove [%s,%s] into the preferred operator list", mccPtr, mncPtr);
            // Error code of RemovePreferredOperators is returned
        }
        DeletePreferredOperatorsList(&preferredOperatorsList);
    }
    else
    {
        LE_ERROR("No preferred Operator present in modem!");
        res = LE_UNSUPPORTED;
    }

    return res;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to retrieve a list of the preferred operators.
 *
 * @return
 * - Reference    to the List object.
 * - Null pointer if there is no preferences list.
 */
//--------------------------------------------------------------------------------------------------
le_mrc_PreferredOperatorListRef_t le_mrc_GetPreferredOperatorsList
(
    void
)
{
    PreferredOperatorsList_t * OperatorListPtr =
                    (PreferredOperatorsList_t *) le_mem_ForceAlloc(PrefOpsListPool);

    if (OperatorListPtr != NULL)
    {
        OperatorListPtr->paPrefOpList = LE_DLS_LIST_INIT;
        OperatorListPtr->safeRefPrefOpList = LE_DLS_LIST_INIT;
        OperatorListPtr->currentLinkPtr = NULL;
        le_result_t res = GetPreferredOperatorsList(
            &(OperatorListPtr->paPrefOpList), false, true, &OperatorListPtr->opsCount);

        if (res == LE_OK)
        {
            if (OperatorListPtr->opsCount)
            {
                // Create and return a Safe Reference for this List object.
                return le_ref_CreateRef(PreferredOperatorsListRefMap, OperatorListPtr);
            }
            else
            {
                // no item in list
                return NULL;
            }
        }
        else
        {
            le_mem_Release(OperatorListPtr);
            LE_WARN("Unable to retrieve the list of the preferred operators!");
            return NULL;
        }
    }

    LE_ERROR("Unable allocated memory for the list of the preferred operators!");
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the first Operator object reference in the list of the
 * preferred operators retrieved with le_mrc_GetPreferredOperators().
 *
 * @return
 * - NULL                          No operator information found.
 * - le_mrc_PreferredOperatorRef   The Operator object reference.
 *
 * @note If the caller is passing a bad reference into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_mrc_PreferredOperatorRef_t le_mrc_GetFirstPreferredOperator
(
    le_mrc_PreferredOperatorListRef_t  preferredOperatorListRef ///< [IN] The list of the preferred
                                                                ///  operators.
)
{
    pa_mrc_PreferredNetworkOperator_t* nodePtr;
    le_dls_Link_t*             linkPtr;
    PreferredOperatorsList_t * prefOperatorListPtr =
                    le_ref_Lookup(PreferredOperatorsListRefMap, preferredOperatorListRef);

    if (prefOperatorListPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", preferredOperatorListRef);
        return NULL;
    }

    linkPtr = le_dls_Peek(&(prefOperatorListPtr->paPrefOpList));
    if (linkPtr != NULL)
    {
        nodePtr = CONTAINER_OF(linkPtr, pa_mrc_PreferredNetworkOperator_t , link);
        prefOperatorListPtr->currentLinkPtr = linkPtr;

        PreferredOperatorsSafeRef_t * newPrefOpsInfoPtr =
                        le_mem_ForceAlloc(PreferredOperatorsSafeRefPool);
        newPrefOpsInfoPtr->safeRef = le_ref_CreateRef(PreferredOperatorsRefMap, nodePtr);
        newPrefOpsInfoPtr->link = LE_DLS_LINK_INIT;
        le_dls_Queue(&(prefOperatorListPtr->safeRefPrefOpList), &(newPrefOpsInfoPtr->link));

        return ((le_mrc_PreferredOperatorRef_t)newPrefOpsInfoPtr->safeRef);
    }

    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the next Operator object reference in the list of the
 * preferred operators retrieved with le_mrc_GetPreferredOperators().
 *
 * @return
 * - NULL                          No operator information found.
 * - le_mrc_PreferredOperatorRef   The Operator object reference.
 *
 * @note If the caller is passing a bad reference into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_mrc_PreferredOperatorRef_t le_mrc_GetNextPreferredOperator
(
    le_mrc_PreferredOperatorListRef_t  preferredOperatorListRef ///< [IN] The list of the preferred
                                                                ///<  operators.
)
{
    pa_mrc_PreferredNetworkOperator_t* nodePtr;
    le_dls_Link_t*             linkPtr;
    PreferredOperatorsList_t * prefOperatorsListPtr =
                    le_ref_Lookup(PreferredOperatorsListRefMap, preferredOperatorListRef);

    if (prefOperatorsListPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", preferredOperatorListRef);
        return NULL;
    }

    linkPtr = le_dls_PeekNext(&(prefOperatorsListPtr->paPrefOpList),
                    prefOperatorsListPtr->currentLinkPtr);
    if (linkPtr != NULL)
    {
        nodePtr = CONTAINER_OF(linkPtr, pa_mrc_PreferredNetworkOperator_t, link);
        prefOperatorsListPtr->currentLinkPtr = linkPtr;

        PreferredOperatorsSafeRef_t * newPrefOpsInfoPtr =
                        le_mem_ForceAlloc(PreferredOperatorsSafeRefPool);
        newPrefOpsInfoPtr->safeRef = le_ref_CreateRef(PreferredOperatorsRefMap, nodePtr);
        newPrefOpsInfoPtr->link = LE_DLS_LINK_INIT;
        le_dls_Queue(&(prefOperatorsListPtr->safeRefPrefOpList) ,&(newPrefOpsInfoPtr->link));

        return ((le_mrc_PreferredOperatorRef_t)newPrefOpsInfoPtr->safeRef);
    }

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to delete the list of the preferred operators retrieved with
 * le_mrc_GetPreferredOperators().
 *
 * @note On failure, the process exits, so you don't have to worry about checking the returned
 *       reference for validity.
 */
//--------------------------------------------------------------------------------------------------
void le_mrc_DeletePreferredOperatorsList
(
    le_mrc_PreferredOperatorListRef_t  preferredOperatorListRef ///< The [IN] list of the preferred
                                                                ///<  operators.
)
{
    PreferredOperatorsList_t * prefOperatorsListPtr =
                    le_ref_Lookup(PreferredOperatorsListRefMap, preferredOperatorListRef);

    if (prefOperatorsListPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", preferredOperatorListRef);
        return;
    }

    prefOperatorsListPtr->currentLinkPtr = NULL;

    DeletePreferredOperatorsList(&prefOperatorsListPtr->paPrefOpList);

    // Delete the safe Reference list.
    DeletePreferredOperatorsSafeRefList(&(prefOperatorsListPtr->safeRefPrefOpList));

    // Invalidate the Safe Reference.
    le_ref_DeleteRef(PreferredOperatorsListRefMap, preferredOperatorListRef);

    le_mem_Release(prefOperatorsListPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the Operator information details.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the MCC or MNC would not fit in buffer
 *      - LE_FAULT for all other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_GetPreferredOperatorDetails
(
    le_mrc_PreferredOperatorRef_t preferredOperatorRef,
        ///< [IN]
        ///< Operator object reference.

    char* mccPtr,
        ///< [OUT]
        ///< Mobile Country Code.

    size_t mccPtrNumElements,
        ///< [IN]

    char* mncPtr,
        ///< [OUT]
        ///< Mobile Network Code.

    size_t mncPtrNumElements,
        ///< [IN]

    le_mrc_RatBitMask_t* ratMaskPtr
        ///< [OUT]
        ///< Bit mask for the RAT preferences.
)
{
    pa_mrc_PreferredNetworkOperator_t* prefOperatorInfoPtr =
                    le_ref_Lookup(PreferredOperatorsRefMap, preferredOperatorRef);

    if (mccPtr == NULL)
    {
        LE_KILL_CLIENT("mccPtr mccPtris NULL !");
        return LE_FAULT;
    }

    if (mncPtr == NULL)
    {
        LE_KILL_CLIENT("mncPtr is NULL !");
        return LE_FAULT;
    }

    if (ratMaskPtr == NULL)
    {
        LE_KILL_CLIENT("ratMaskPtr is NULL !");
        return LE_FAULT;
    }

    if (prefOperatorInfoPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", preferredOperatorRef);
        return LE_FAULT;
    }


    if( le_utf8_Copy(mccPtr, prefOperatorInfoPtr->mobileCode.mcc, mccPtrNumElements, NULL) ==
                    LE_OVERFLOW )
    {
        LE_ERROR("Mobile Country Code string size is greater than mccPtrNumElements");
        return LE_OVERFLOW;
    }

    if( le_utf8_Copy(mncPtr, prefOperatorInfoPtr->mobileCode.mnc, mncPtrNumElements, NULL) ==
                   LE_OVERFLOW )
    {
        LE_ERROR("Mobile Network Code string size is greater than mncPtrNumElements");
        return LE_OVERFLOW;
    }


    *ratMaskPtr = prefOperatorInfoPtr->ratMask;

    return LE_OK;
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
le_mrc_NetRegStateEventHandlerRef_t le_mrc_AddNetRegStateEventHandler
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

    return (le_mrc_NetRegStateEventHandlerRef_t)(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to remove an handler for Network registration state changes.
 */
//--------------------------------------------------------------------------------------------------
void le_mrc_RemoveNetRegStateEventHandler
(
    le_mrc_NetRegStateEventHandlerRef_t    handlerRef ///< [IN] The handler reference.
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register an handler for Radio Access Technology changes.
 *
 * @return A handler reference, which is only needed for later removal of the handler.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_mrc_RatChangeHandlerRef_t le_mrc_AddRatChangeHandler
(
    le_mrc_RatChangeHandlerFunc_t handlerFuncPtr, ///< [IN] The handler function.
    void*                         contextPtr      ///< [IN] The handler's context.
)
{
    le_event_HandlerRef_t        handlerRef;

    if (handlerFuncPtr == NULL)
    {
        LE_KILL_CLIENT("Handler function is NULL !");
        return NULL;
    }

    handlerRef = le_event_AddLayeredHandler("RatChangeHandler",
                                            RatChangeId,
                                            FirstLayerRatChangeHandler,
                                            (le_event_HandlerFunc_t)handlerFuncPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (le_mrc_RatChangeHandlerRef_t)(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to remove an handler for Radio Access Technology changes.
 */
//--------------------------------------------------------------------------------------------------
void le_mrc_RemoveRatChangeHandler
(
    le_mrc_RatChangeHandlerRef_t    handlerRef ///< [IN] The handler reference.
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'le_mrc_PacketSwitchedChange'
 *
 * This event provides information on Packet Switched service changes.
 *
 * @note <b>multi-app safe</b>
 */
//--------------------------------------------------------------------------------------------------
le_mrc_PacketSwitchedChangeHandlerRef_t le_mrc_AddPacketSwitchedChangeHandler
(
    le_mrc_PacketSwitchedChangeHandlerFunc_t handlerFuncPtr, ///< [IN]
    void* contextPtr  ///< [IN]
)
{
    le_event_HandlerRef_t        handlerRef;

    if (handlerFuncPtr == NULL)
    {
        LE_KILL_CLIENT("Handler function is NULL !");
        return NULL;
    }

    handlerRef = le_event_AddLayeredHandler("PacketSwitchedChangeHandler",
        PSChangeId,
        FirstLayerPSChangeHandler,
        (le_event_HandlerFunc_t)handlerFuncPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (le_mrc_PacketSwitchedChangeHandlerRef_t)(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'le_mrc_PacketSwitchedChange'
 */
//--------------------------------------------------------------------------------------------------
void le_mrc_RemovePacketSwitchedChangeHandler
(
    le_mrc_PacketSwitchedChangeHandlerRef_t handlerRef ///< [IN]
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'le_mrc_CircuitSwitchedChange'
 *
 * This event provides information on Circuit Switched service changes.
 *
 * @note <b>multi-app safe</b>
 */
//--------------------------------------------------------------------------------------------------
le_mrc_CircuitSwitchedChangeHandlerRef_t le_mrc_AddCircuitSwitchedChangeHandler
(
    le_mrc_CircuitSwitchedChangeHandlerFunc_t handlerFuncPtr,  ///< [IN]
    void* contextPtr       ///< [IN]
)
{
    le_event_HandlerRef_t        handlerRef;

    if (handlerFuncPtr == NULL)
    {
        LE_KILL_CLIENT("Handler function is NULL !");
        return NULL;
    }

    handlerRef = le_event_AddLayeredHandler("CicuitSwitchedChangeHandler",
        CSChangeId,
        FirstLayerCSChangeHandler,
        (le_event_HandlerFunc_t)handlerFuncPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (le_mrc_CircuitSwitchedChangeHandlerRef_t)(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'le_mrc_CircuitSwitchedChange'
 */
//--------------------------------------------------------------------------------------------------
void le_mrc_RemoveCircuitSwitchedChangeHandler
(
    le_mrc_CircuitSwitchedChangeHandlerRef_t handlerRef    ///< [IN]
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
 * @return LE_FAULT         The function failed to get the Radio Module power state.
 * @return LE_BAD_PARAMETER if powerPtr is NULL.
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
        return LE_BAD_PARAMETER;
    }

    res=pa_mrc_GetRadioPower(powerPtr);

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
 * This function must be called to get the current Radio Access Technology in use.
 *
 * @return LE_FAULT         Function failed to get the Radio Access Technology.
 * @return LE_BAD_PARAMETER A bad parameter was passed.
 * @return LE_OK            Function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_GetRadioAccessTechInUse
(
    le_mrc_Rat_t*   ratPtr  ///< [OUT] The Radio Access Technology.
)
{
    if (ratPtr == NULL)
    {
        LE_KILL_CLIENT("ratPtr is NULL !");
        return LE_BAD_PARAMETER;
    }

    if (pa_mrc_GetRadioAccessTechInUse(ratPtr) == LE_OK)
    {
        return LE_OK;
    }
    else
    {
        return LE_FAULT;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the Network registration state.
 *
 * @return LE_FAULT         The function failed to get the Network registration state.
 * @return LE_BAD_PARAMETER A bad parameter was passed.
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
        return LE_BAD_PARAMETER;
    }

    if (pa_mrc_GetNetworkRegState(statePtr) == LE_OK)
    {
        return LE_OK;
    }
    else
    {
        return LE_FAULT;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the Signal Quality information.
 *
 * @return LE_FAULT         The function failed to get the Signal Quality information.
 * @return LE_BAD_PARAMETER A bad parameter was passed.
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
        return LE_BAD_PARAMETER;
    }

    if ((res=pa_mrc_GetSignalStrength(&rssi)) == LE_OK)
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

        LE_DEBUG("pa_mrc_GetSignalStrength has returned rssi=%ddBm", rssi);
        return LE_OK;
    }
    else if (res == LE_OUT_OF_RANGE)
    {
        LE_DEBUG("pa_mrc_GetSignalStrength has returned LE_OUT_OF_RANGE");
        *qualityPtr = 0;
        return LE_OK;
    }
    else
    {
        LE_ERROR("pa_mrc_GetSignalStrength has returned %d", res);
        *qualityPtr = 0;
        return LE_FAULT;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the Current Network Name information.
 *
 * @return
 *      - LE_OK             on success
 *      - LE_BAD_PARAMETER  if nameStr is NULL
 *      - LE_OVERFLOW       if the Home Network Name can't fit in nameStr
 *      - LE_FAULT          on any other failure
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_GetCurrentNetworkName
(
    char       *nameStr,               ///< [OUT] the home network Name
    size_t      nameStrSize            ///< [IN] the nameStr size
)
{
    if (nameStr == NULL)
    {
        LE_KILL_CLIENT("nameStr is NULL !");
        return LE_BAD_PARAMETER;
    }

    return pa_mrc_GetCurrentNetwork(nameStr, nameStrSize, NULL, 0, NULL, 0);
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the current network PLMN information in numeric format.
 *
 * @return
 *      - LE_OK       on success
 *      - LE_FAULT    on any other failure
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_GetCurrentNetworkMccMnc
(
    char* mccStr,               ///< [OUT] the mobile country code
    size_t mccStrNumElements,   ///< [IN] the mccStr size
    char* mncStr,               ///< [OUT] the mobile network code
    size_t mncStrNumElements    ///< [IN] the mncStr size
)
{
    if (mccStr == NULL)
    {
        LE_KILL_CLIENT("mccStr is NULL !");
        return LE_FAULT;
    }

    if (mncStr == NULL)
    {
        LE_KILL_CLIENT("mncStr is NULL !");
        return LE_FAULT;
    }

    if(mccStrNumElements < LE_MRC_MCC_BYTES)
    {
        LE_ERROR("mccStrNumElements is < %d",LE_MRC_MCC_BYTES);
        return LE_FAULT;
    }

    if(mncStrNumElements < LE_MRC_MNC_BYTES)
    {
        LE_ERROR("mncStrNumElements is < %d",LE_MRC_MNC_BYTES);
        return LE_FAULT;
    }

    if ( pa_mrc_GetCurrentNetwork(NULL, 0, mccStr, mccStrNumElements, mncStr, mncStrNumElements)
                    == LE_OK)
    {
        return LE_OK;
    }
    else
    {
        return LE_FAULT;

    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the Packet Switched state.
 *
 * @return
 *  - LE_FAULT  Function failed.
 *  - LE_OK     Function succeeded.
 *
 * @note <b>multi-app safe</b>
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_GetPacketSwitchedState
(
    le_mrc_ServiceState_t* statePtr
        ///< [OUT] The current Packet switched state.
)
{
    return pa_mrc_GetPacketSwitchedState(statePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the Circuit Switched state.
 *
 * @return
 *  - LE_FAULT  Function failed.
 *  - LE_OK     Function succeeded.
 *
 * @note <b>multi-app safe</b>
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_GetCircuitSwitchedState
(
    le_mrc_ServiceState_t* statePtr
        ///< [OUT] The current Circuit switched state.
)
{
    return pa_mrc_GetCircuitSwitchedState(statePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to perform a cellular network scan.
 *
 * @return
 *      Reference to the List object. Null pointer if the scan failed.
 */
//--------------------------------------------------------------------------------------------------
le_mrc_ScanInformationListRef_t le_mrc_PerformCellularNetworkScan
(
    le_mrc_RatBitMask_t ratMask ///< [IN] Radio Access Technology bitmask
)
{
    le_result_t result;
    ScanInfoList_t* newScanInformationListPtr = NULL;

    newScanInformationListPtr = le_mem_ForceAlloc(ScanInformationListPool);
    newScanInformationListPtr->paScanInfoList = LE_DLS_LIST_INIT;
    newScanInformationListPtr->safeRefScanInfoList = LE_DLS_LIST_INIT;
    newScanInformationListPtr->currentLink = NULL;

    LOCK();
    result = pa_mrc_PerformNetworkScan(ratMask,
                                       PA_MRC_SCAN_PLMN,
                                       &(newScanInformationListPtr->paScanInfoList));
    UNLOCK();

    if (result != LE_OK)
    {
        le_mem_Release(newScanInformationListPtr);
        return NULL;
    }

    return le_ref_CreateRef(ScanInformationListRefMap, newScanInformationListPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to perform a cellular network scan asynchronously. This function
 * is not blocking, the response will be returned with a handler function.
 *
 */
//--------------------------------------------------------------------------------------------------
void le_mrc_PerformCellularNetworkScanAsync
(
    le_mrc_RatBitMask_t ratMask,
        ///< [IN]
        ///< Radio Access Technology mask

    le_mrc_CellularNetworkScanHandlerFunc_t handlerPtr,
        ///< [IN]

    void* contextPtr
        ///< [IN]
)
{
    CmdRequest_t cmd;

    cmd.context = contextPtr;
    cmd.callBackPtr = handlerPtr;
    cmd.command = LE_MRC_CMD_TYPE_ASYNC_SCAN;
    cmd.scan.ratMask = ratMask;

    // Sending Cellular Network scan command
    LE_DEBUG("Send asynchronous Cellular Network scan command");
    le_event_Report(MrcCommandEventId, &cmd, sizeof(cmd));
}



//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the first Scan Information object reference in the list of
 * scan Information retrieved with le_mrc_PerformCellularNetworkScan().
 *
 * @return NULL                         No scan information found.
 * @return le_mrc_ScanInformationRef_t The Scan Information object reference.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_mrc_ScanInformationRef_t le_mrc_GetFirstCellularNetworkScan
(
    le_mrc_ScanInformationListRef_t  scanInformationListRef ///< [IN] The list of scan information.
)
{
    pa_mrc_ScanInformation_t* nodePtr;
    le_dls_Link_t*          linkPtr;

    ScanInfoList_t* scanInformationListPtr = le_ref_Lookup(ScanInformationListRefMap,
                                                                         scanInformationListRef);

    if (scanInformationListPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", scanInformationListRef);
        return NULL;
    }

    linkPtr = le_dls_Peek(&(scanInformationListPtr->paScanInfoList));
    if (linkPtr != NULL)
    {
        nodePtr = CONTAINER_OF(linkPtr, pa_mrc_ScanInformation_t, link);
        scanInformationListPtr->currentLink = linkPtr;

        ScanInfoSafeRef_t* newScanInformationPtr = le_mem_ForceAlloc(ScanInformationSafeRefPool);
        newScanInformationPtr->safeRef = le_ref_CreateRef(ScanInformationRefMap,nodePtr);
        newScanInformationPtr->link = LE_DLS_LINK_INIT;
        le_dls_Queue(&(scanInformationListPtr->safeRefScanInfoList),&(newScanInformationPtr->link));

        return (le_mrc_ScanInformationRef_t)newScanInformationPtr->safeRef;
    }
    else
    {
        return NULL;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the next Scan Information object reference in the list of
 * scan Information retrieved with le_mrc_PerformCellularNetworkScan().
 *
 * @return NULL                         No scan information found.
 * @return le_mrc_ScanInformationRef_t The Scan Information object reference.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_mrc_ScanInformationRef_t le_mrc_GetNextCellularNetworkScan
(
    le_mrc_ScanInformationListRef_t  scanInformationListRef ///< [IN] The list of scan information.
)
{
    pa_mrc_ScanInformation_t* nodePtr;
    le_dls_Link_t*          linkPtr;

    ScanInfoList_t* scanInformationListPtr = le_ref_Lookup(ScanInformationListRefMap,
                                                                         scanInformationListRef);


    if (scanInformationListPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", scanInformationListRef);
        return NULL;
    }

    linkPtr = le_dls_PeekNext(&(scanInformationListPtr->paScanInfoList),
                                scanInformationListPtr->currentLink);
    if (linkPtr != NULL)
    {
        nodePtr = CONTAINER_OF(linkPtr, pa_mrc_ScanInformation_t, link);
        scanInformationListPtr->currentLink = linkPtr;

        ScanInfoSafeRef_t* newScanInformationPtr = le_mem_ForceAlloc(ScanInformationSafeRefPool);
        newScanInformationPtr->safeRef = le_ref_CreateRef(ScanInformationRefMap,nodePtr);
        newScanInformationPtr->link = LE_DLS_LINK_INIT;
        le_dls_Queue(&(scanInformationListPtr->safeRefScanInfoList),
            &(newScanInformationPtr->link));

        return (le_mrc_ScanInformationRef_t)newScanInformationPtr->safeRef;
    }
    else
    {
        return NULL;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to delete the list of the Scan Information retrieved with
 * le_mrc_PerformCellularNetworkScan().
 *
 * @note
 *      On failure, the process exits, so you don't have to worry about checking the returned
 *      reference for validity.
 */
//--------------------------------------------------------------------------------------------------
void le_mrc_DeleteCellularNetworkScan
(
    le_mrc_ScanInformationListRef_t  scanInformationListRef ///< [IN] The list of scan information.
)
{
    ScanInfoList_t* scanInformationListPtr = le_ref_Lookup(ScanInformationListRefMap,
                                                                         scanInformationListRef);

    if (scanInformationListPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", scanInformationListRef);
        return;
    }

    scanInformationListPtr->currentLink = NULL;
    pa_mrc_DeleteScanInformation(&(scanInformationListPtr->paScanInfoList));

    // Delete the safe Reference list.
    DeleteSafeRefList(&(scanInformationListPtr->safeRefScanInfoList));

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
 *      - LE_OVERFLOW if the MCC or MNC would not fit in buffer
 *      - LE_FAULT for all other errors
 *
 * @note
 *      On failure, the process exits, so you don't have to worry about checking the returned
 *      reference for validity.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_GetCellularNetworkMccMnc
(
    le_mrc_ScanInformationRef_t scanInformationRef,    ///< [IN] Scan information reference
    char                        *mccPtr,                ///< [OUT] Mobile Country Code
    size_t                       mccPtrSize,            ///< [IN] mccPtr buffer size
    char                        *mncPtr,                ///< [OUT] Mobile Network Code
    size_t                       mncPtrSize             ///< [IN] mncPtr buffer size
)
{
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

    if ( le_utf8_Copy(mccPtr,scanInformationPtr->mobileCode.mcc,mccPtrSize,NULL) != LE_OK )
    {
        LE_WARN("Could not copy all mcc");
        return LE_OVERFLOW;
    }

    if ( le_utf8_Copy(mncPtr,scanInformationPtr->mobileCode.mnc,mncPtrSize,NULL) != LE_OK )
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
 *      - LE_FAULT for all other errors
 *
 * @note
 *      On failure, the process exits, so you don't have to worry about checking the returned
 *      reference for validity.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_GetCellularNetworkName
(
    le_mrc_ScanInformationRef_t scanInformationRef,    ///< [IN] Scan information reference
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
 * This function must be called to get the radio access technology of a scanInformationRef.
 *
 * @return
 *      - the radio access technology
 *
 * @note
 *      On failure, the process exits.
 */
//--------------------------------------------------------------------------------------------------
le_mrc_Rat_t le_mrc_GetCellularNetworkRat
(
    le_mrc_ScanInformationRef_t scanInformationRef    ///< [IN] Scan information reference
)
{
    pa_mrc_ScanInformation_t* scanInformationPtr = le_ref_Lookup(ScanInformationRefMap,
                                                                 scanInformationRef);

    if (scanInformationPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", scanInformationRef);
        return LE_FAULT;
    }

    return scanInformationPtr->rat;
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
    le_mrc_ScanInformationRef_t scanInformationRef    ///< [IN] Scan information reference
)
{
    pa_mrc_ScanInformation_t* scanInformationPtr = le_ref_Lookup(ScanInformationRefMap,
                                                                 scanInformationRef);

    if (scanInformationPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", scanInformationRef);
        return false;
    }

    return scanInformationPtr->isInUse;
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
    le_mrc_ScanInformationRef_t scanInformationRef    ///< [IN] Scan information reference
)
{
    pa_mrc_ScanInformation_t* scanInformationPtr = le_ref_Lookup(ScanInformationRefMap,
                                                                 scanInformationRef);

    if (scanInformationPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", scanInformationRef);
        return false;
    }

    return scanInformationPtr->isAvailable;
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
    le_mrc_ScanInformationRef_t scanInformationRef    ///< [IN] Scan information reference
)
{
    pa_mrc_ScanInformation_t* scanInformationPtr = le_ref_Lookup(ScanInformationRefMap,
                                                                 scanInformationRef);

    if (scanInformationPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", scanInformationRef);
        return false;
    }

    return scanInformationPtr->isHome;
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
    le_mrc_ScanInformationRef_t scanInformationRef    ///< [IN] Scan information reference
)
{
    pa_mrc_ScanInformation_t* scanInformationPtr = le_ref_Lookup(ScanInformationRefMap,
                                                                 scanInformationRef);

    if (scanInformationPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", scanInformationRef);
        return false;
    }

    return scanInformationPtr->isForbidden;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to retrieve the Neighboring Cells information. It creates and
 * returns a reference to the Neighboring Cells information.
 *
 * @return A reference to the Neighboring Cells information.
 * @return NULL if no Cells Information are available.
 */
//--------------------------------------------------------------------------------------------------
le_mrc_NeighborCellsRef_t le_mrc_GetNeighborCellsInfo
(
    void
)
{
    CellList_t* ngbrCellsInfoListPtr = (CellList_t*)le_mem_ForceAlloc(CellListPool);

    if (ngbrCellsInfoListPtr != NULL)
    {
        ngbrCellsInfoListPtr->paNgbrCellInfoList = LE_DLS_LIST_INIT;
        ngbrCellsInfoListPtr->safeRefCellInfoList = LE_DLS_LIST_INIT;
        ngbrCellsInfoListPtr->currentLinkPtr = NULL;
        ngbrCellsInfoListPtr->cellsCount = pa_mrc_GetNeighborCellsInfo(
                                                &(ngbrCellsInfoListPtr->paNgbrCellInfoList));
        if (ngbrCellsInfoListPtr->cellsCount > 0)
        {
            // Create and return a Safe Reference for this List object.
            return le_ref_CreateRef(CellListRefMap, ngbrCellsInfoListPtr);
        }
        else
        {
            le_mem_Release(ngbrCellsInfoListPtr);
            LE_WARN("Unable to retrieve the Neighboring Cells information!");
            return NULL;
        }
    }
    else
    {
        LE_WARN("Unable to retrieve the Neighboring Cells information!");
        return NULL;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to delete the Neighboring Cells information.
 *
 * @note
 *      On failure, the process exits, so you don't have to worry about checking the returned
 *      reference for validity.
 */
//--------------------------------------------------------------------------------------------------
void le_mrc_DeleteNeighborCellsInfo
(
    le_mrc_NeighborCellsRef_t ngbrCellsRef ///< [IN] The Neighboring Cells reference
)
{
    CellList_t* ngbrCellsInfoListPtr = le_ref_Lookup(CellListRefMap, ngbrCellsRef);
    if (ngbrCellsInfoListPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ngbrCellsRef);
        return;
    }

    ngbrCellsInfoListPtr->currentLinkPtr = NULL;
    pa_mrc_DeleteNeighborCellsInfo(&(ngbrCellsInfoListPtr->paNgbrCellInfoList));

    // Delete the safe Reference list.
    DeleteCellInfoSafeRefList(&(ngbrCellsInfoListPtr->safeRefCellInfoList));
    // Invalidate the Safe Reference.
    le_ref_DeleteRef(CellListRefMap, ngbrCellsRef);

    le_mem_Release(ngbrCellsInfoListPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the first Cell Information reference in the list of
 * Neighboring Cells information retrieved with le_mrc_GetNeighborCellsInfo().
 *
 * @return NULL                   No Cell information object found.
 * @return le_mrc_CellInfoRef_t  The Cell information object reference.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_mrc_CellInfoRef_t le_mrc_GetFirstNeighborCellInfo
(
    le_mrc_NeighborCellsRef_t     ngbrCellsRef ///< [IN] The Neighboring Cells reference
)
{
    pa_mrc_CellInfo_t*      nodePtr;
    le_dls_Link_t*          linkPtr;
    CellList_t*  ngbrCellsInfoListPtr = le_ref_Lookup(CellListRefMap, ngbrCellsRef);
    if (ngbrCellsInfoListPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ngbrCellsRef);
        return NULL;
    }

    linkPtr = le_dls_Peek(&(ngbrCellsInfoListPtr->paNgbrCellInfoList));
    if (linkPtr != NULL)
    {
        nodePtr = CONTAINER_OF(linkPtr, pa_mrc_CellInfo_t, link);
        ngbrCellsInfoListPtr->currentLinkPtr = linkPtr;

        CellSafeRef_t* newNbgrInfoPtr = le_mem_ForceAlloc(CellInfoSafeRefPool);
        newNbgrInfoPtr->safeRef = le_ref_CreateRef(CellRefMap, nodePtr);
        newNbgrInfoPtr->link = LE_DLS_LINK_INIT;
        le_dls_Queue(&(ngbrCellsInfoListPtr->safeRefCellInfoList), &(newNbgrInfoPtr->link));

        return ((le_mrc_CellInfoRef_t)newNbgrInfoPtr->safeRef);
    }
    else
    {
        return NULL;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the next Cell Information reference in the list of
 * Neighboring Cells information retrieved with le_mrc_GetNeighborCellsInfo().
 *
 * @return NULL                   No Cell information object found.
 * @return le_mrc_CellInfoRef_t  The Cell information object reference.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_mrc_CellInfoRef_t le_mrc_GetNextNeighborCellInfo
(
    le_mrc_NeighborCellsRef_t     ngbrCellsRef ///< [IN] The Neighboring Cells reference
)
{
    pa_mrc_CellInfo_t*      nodePtr;
    le_dls_Link_t*          linkPtr;
    CellList_t*  ngbrCellsInfoListPtr = le_ref_Lookup(CellListRefMap, ngbrCellsRef);
    if (ngbrCellsInfoListPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ngbrCellsRef);
        return NULL;
    }

    linkPtr = le_dls_PeekNext(&(ngbrCellsInfoListPtr->paNgbrCellInfoList),
        ngbrCellsInfoListPtr->currentLinkPtr);
    if (linkPtr != NULL)
    {
        nodePtr = CONTAINER_OF(linkPtr, pa_mrc_CellInfo_t, link);
        ngbrCellsInfoListPtr->currentLinkPtr = linkPtr;

        CellSafeRef_t* newNbgrInfoPtr = le_mem_ForceAlloc(CellInfoSafeRefPool);
        newNbgrInfoPtr->safeRef = le_ref_CreateRef(CellRefMap, nodePtr);
        newNbgrInfoPtr->link = LE_DLS_LINK_INIT;
        le_dls_Queue(&(ngbrCellsInfoListPtr->safeRefCellInfoList) ,&(newNbgrInfoPtr->link));

        return ((le_mrc_CellInfoRef_t)newNbgrInfoPtr->safeRef);
    }
    else
    {
        return NULL;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the Cell Identifier.
 *
 * @return The Cell Identifier.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
uint32_t le_mrc_GetNeighborCellId
(
    le_mrc_CellInfoRef_t     ngbrCellInfoRef ///< [IN] The Cell information reference
)
{
    pa_mrc_CellInfo_t* cellInfoPtr = le_ref_Lookup(CellRefMap, ngbrCellInfoRef);
    if (cellInfoPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ngbrCellInfoRef);
        return LE_FAULT;
    }

    return (cellInfoPtr->id);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the Location Area Code of a cell.
 *
 * @return The Location Area Code of a cell. 0xFFFF value is returned if the value is not available.
 *
 * @note If the caller is passing a bad pointer into this function, it's a fatal error, the
 *       function won't return.
 */
//--------------------------------------------------------------------------------------------------
uint32_t le_mrc_GetNeighborCellLocAreaCode
(
    le_mrc_CellInfoRef_t     ngbrCellInfoRef ///< [IN] The Cell information reference
)
{
    pa_mrc_CellInfo_t* cellInfoPtr = le_ref_Lookup(CellRefMap, ngbrCellInfoRef);
    if (cellInfoPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ngbrCellInfoRef);
        return LE_FAULT;
    }

    return (cellInfoPtr->lac);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the signal strength of a cell.
 *
 * @return The signal strength of a cell in dBm.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
int32_t le_mrc_GetNeighborCellRxLevel
(
    le_mrc_CellInfoRef_t     ngbrCellInfoRef ///< [IN] The Cell information reference
)
{
    pa_mrc_CellInfo_t* cellInfoPtr = le_ref_Lookup(CellRefMap, ngbrCellInfoRef);
    if (cellInfoPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ngbrCellInfoRef);
        return LE_FAULT;
    }

    return (cellInfoPtr->rxLevel);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the Radio Access Technology of a cell.
 *
 * @return The Radio Access Technology of a cell.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_mrc_Rat_t le_mrc_GetNeighborCellRat
(
    le_mrc_CellInfoRef_t     ngbrCellInfoRef ///< [IN] The Cell information reference
)
{
    pa_mrc_CellInfo_t* cellInfoPtr = le_ref_Lookup(CellRefMap, ngbrCellInfoRef);
    if (cellInfoPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ngbrCellInfoRef);
        return LE_FAULT;
    }

    return (cellInfoPtr->rat);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the Ec/Io; the received energy per chip divided by the power
 * density in the band measured in dBm on the primary CPICH channel of serving cell.
 *
 * @return
 *  - The Ec/Io of a cell given in dB with 1 decimal place. (only applicable for UMTS network).
 *  - 0xFFFFFFFF when the value is not available.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
int32_t le_mrc_GetNeighborCellUmtsEcIo
(
    le_mrc_CellInfoRef_t     ngbrCellInfoRef ///< [IN] The Cell information reference
)
{
    pa_mrc_CellInfo_t* cellInfoPtr = le_ref_Lookup(CellRefMap, ngbrCellInfoRef);
    if (cellInfoPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ngbrCellInfoRef);
        return LE_FAULT;
    }

    return (cellInfoPtr->umtsEcIo);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the RSRP and RSRQ of the Intrafrequency of a LTE cell.
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on failure
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_GetNeighborCellLteIntraFreq
(
    le_mrc_CellInfoRef_t  ngbrCellInfoRef, ///< [IN] The Cell information reference
    int32_t*              rsrqPtr,         ///< [OUT] Reference Signal Received Quality value in dB
                                           ///< with 1 decimal place
    int32_t*              rsrpPtr          ///< [OUT] Reference Signal Receiver Power value in dBm
                                           ///< with 1 decimal place
)
{
    pa_mrc_CellInfo_t* cellInfoPtr = le_ref_Lookup(CellRefMap, ngbrCellInfoRef);
    if (cellInfoPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ngbrCellInfoRef);
        return LE_FAULT;
    }
    else
    {
        *rsrpPtr = cellInfoPtr->lteIntraRsrp;
        *rsrqPtr = cellInfoPtr->lteIntraRsrq;
        return LE_OK;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the RSRP and RSRQ of the Interfrequency of a LTE cell.
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on failure
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_GetNeighborCellLteInterFreq
(
    le_mrc_CellInfoRef_t  ngbrCellInfoRef, ///< [IN] The Cell information reference
    int32_t*              rsrqPtr,         ///< [OUT] Reference Signal Received Quality value in dB
                                           ///< with 1 decimal place
    int32_t*              rsrpPtr          ///< [OUT] Reference Signal Receiver Power value in dBm
                                           ///< with 1 decimal place
)
{
    pa_mrc_CellInfo_t* cellInfoPtr = le_ref_Lookup(CellRefMap, ngbrCellInfoRef);
    if (cellInfoPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ngbrCellInfoRef);
        return LE_FAULT;
    }
    else
    {
        *rsrpPtr = cellInfoPtr->lteInterRsrp;
        *rsrqPtr = cellInfoPtr->lteInterRsrq;
        return LE_OK;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to measure the signal metrics. It creates and returns a reference
 * to the signal metrics.
 *
 * @return A reference to the signal metrics.
 * @return NULL if no signal metrics are available.
 */
//--------------------------------------------------------------------------------------------------
le_mrc_MetricsRef_t le_mrc_MeasureSignalMetrics
(
    void
)
{
    pa_mrc_SignalMetrics_t* metricsPtr = (pa_mrc_SignalMetrics_t*)le_mem_ForceAlloc(MetricsPool);

    if (metricsPtr != NULL)
    {
        if (pa_mrc_MeasureSignalMetrics(metricsPtr) == LE_OK)
        {
            // Create and return a Safe Reference for this object.
            return le_ref_CreateRef(MetricsRefMap, metricsPtr);
        }
        else
        {
            le_mem_Release(metricsPtr);
            LE_ERROR("Unable to measure the signal metrics!");
            return NULL;
        }
    }
    else
    {
        LE_ERROR("Unable to allocate memory for signal metrics!");
        return NULL;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to delete the the signal metrics.
 *
 * @note
 *      On failure, the process exits, so you don't have to worry about checking the returned
 *      reference for validity.
 */
//--------------------------------------------------------------------------------------------------
void le_mrc_DeleteSignalMetrics
(
    le_mrc_MetricsRef_t  MetricsRef ///< [IN] The signal metrics reference.
)
{
    pa_mrc_SignalMetrics_t* metricsPtr = le_ref_Lookup(MetricsRefMap, MetricsRef);
    if (metricsPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", MetricsRef);
        return;
    }

    // Invalidate the Safe Reference.
    le_ref_DeleteRef(MetricsRefMap, MetricsRef);

    le_mem_Release(metricsPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function returns the Radio Access Technology of the signal metrics.
 *
 * @return The Radio Access Technology of the signal measure.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_mrc_Rat_t le_mrc_GetRatOfSignalMetrics
(
    le_mrc_MetricsRef_t  MetricsRef ///< [IN] The signal metrics reference.
)
{
    pa_mrc_SignalMetrics_t* metricsPtr = le_ref_Lookup(MetricsRefMap, MetricsRef);
    if (metricsPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", MetricsRef);
        return LE_MRC_RAT_UNKNOWN;
    }
    else
    {
        return (metricsPtr->rat);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function returns the signal strength in dBm and the bit error rate measured on GSM network.
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on failure
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_GetGsmSignalMetrics
(
    le_mrc_MetricsRef_t MetricsRef, ///< [IN] The signal metrics reference.
    int32_t*            rssiPtr,    ///< [OUT] Signal strength in dBm
    uint32_t*           berPtr      ///< [OUT] Bit error rate.
)
{
    pa_mrc_SignalMetrics_t* metricsPtr = le_ref_Lookup(MetricsRefMap, MetricsRef);
    if (metricsPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", MetricsRef);
        return LE_FAULT;
    }
    else
    {
        if (metricsPtr->rat == LE_MRC_RAT_GSM)
        {
            *rssiPtr = metricsPtr->ss;
            *berPtr = metricsPtr->er;

            return LE_OK;
        }
        else
        {
            LE_ERROR("The measured signal is not GSM (RAT.%d)", metricsPtr->rat);
            return LE_FAULT;
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function returns the signal metrics measured on UMTS network.
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on failure
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_GetUmtsSignalMetrics
(
    le_mrc_MetricsRef_t MetricsRef, ///< [IN] The signal metrics reference.
    int32_t*            ssPtr,      ///< [OUT] Signal strength in dBm
    uint32_t*           blerPtr,    ///< [OUT] Block error rate
    int32_t*            ecioPtr,    ///< [OUT] Ec/Io value  in dB with 1 decimal place (15 = 1.5 dB)
    int32_t*            rscpPtr,    ///< [OUT] Measured RSCP in dBm (only applicable for TD-SCDMA
                                    ///<       network)
    int32_t*            sinrPtr     ///< [OUT] Measured SINR in dB (only applicable for TD-SCDMA
                                    ///<       network)
)
{
    pa_mrc_SignalMetrics_t* metricsPtr = le_ref_Lookup(MetricsRefMap, MetricsRef);
    if (metricsPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", MetricsRef);
        return LE_FAULT;
    }
    else
    {
        if (metricsPtr->rat == LE_MRC_RAT_UMTS)
        {
            *ssPtr = metricsPtr->ss;
            *blerPtr = metricsPtr->er;
            *ecioPtr = metricsPtr->umtsMetrics.ecio;
            *rscpPtr = metricsPtr->umtsMetrics.rscp;
            *sinrPtr = metricsPtr->umtsMetrics.sinr;

            return LE_OK;
        }
        else
        {
            LE_ERROR("The measured signal is not UMTS (RAT.%d)", metricsPtr->rat);
            return LE_FAULT;
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function returns the signal metrics measured on LTE network.
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on failure
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_GetLteSignalMetrics
(
    le_mrc_MetricsRef_t MetricsRef, ///< [IN] The signal metrics reference.
    int32_t*            ssPtr,      ///< [OUT] Signal strength in dBm
    uint32_t*           blerPtr,    ///< [OUT] Block error rate
    int32_t*            rsrqPtr,    ///< [OUT] RSRQ value in dB as measured by L1 with 1 decimal
                                    ///        place
    int32_t*            rsrpPtr,    ///< [OUT] Current RSRP in dBm as measured by L1 with 1 decimal
                                    ///        place
    int32_t*            snrPtr      ///< [OUT] SNR level in dB with 1 decimal place (15 = 1.5 dB)
)
{
    pa_mrc_SignalMetrics_t* metricsPtr = le_ref_Lookup(MetricsRefMap, MetricsRef);
    if (metricsPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", MetricsRef);
        return LE_FAULT;
    }
    else
    {
        if (metricsPtr->rat == LE_MRC_RAT_LTE)
        {
            *ssPtr = metricsPtr->ss;
            *blerPtr = metricsPtr->er;
            *rsrqPtr = metricsPtr->lteMetrics.rsrq;
            *rsrpPtr = metricsPtr->lteMetrics.rsrp;
            *snrPtr = metricsPtr->lteMetrics.snr;

            return LE_OK;
        }
        else
        {
            LE_ERROR("The measured signal is not LTE (RAT.%d)", metricsPtr->rat);
            return LE_FAULT;
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function returns the signal metrics measured on CDMA network.
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on failure
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_GetCdmaSignalMetrics
(
    le_mrc_MetricsRef_t MetricsRef, ///< [IN] The signal metrics reference.
    int32_t*            ssPtr,      ///< [OUT] Signal strength in dBm
    uint32_t*           erPtr,      ///< [OUT] Frame/Packet error rate
    int32_t*            ecioPtr,    ///< [OUT] ECIO value in dB with 1 decimal place (15 = 1.5 dB)
    int32_t*            sinrPtr,    ///< [OUT] SINR level in dB with 1 decimal place, (only
                                    ///<       applicable for 1xEV-DO)
    int32_t*            ioPtr       ///< [OUT] Received IO in dBm (only applicable for 1xEV-DO)
)
{
    pa_mrc_SignalMetrics_t* metricsPtr = le_ref_Lookup(MetricsRefMap, MetricsRef);
    if (metricsPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", MetricsRef);
        return LE_FAULT;
    }
    else
    {
        if (metricsPtr->rat == LE_MRC_RAT_CDMA)
        {
            *ssPtr = metricsPtr->ss;
            *erPtr = metricsPtr->er;
            *ecioPtr = metricsPtr->cdmaMetrics.ecio;
            *sinrPtr = metricsPtr->cdmaMetrics.sinr;
            *ioPtr = metricsPtr->cdmaMetrics.io;

            return LE_OK;
        }
        else
        {
            LE_ERROR("The measured signal is not CDMA (RAT.%d)", metricsPtr->rat);
            return LE_FAULT;
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register an handler for Signal Strength value changes.
 *
 * @return A handler reference, which is only needed for later removal of the handler.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_mrc_SignalStrengthChangeHandlerRef_t le_mrc_AddSignalStrengthChangeHandler
(
    le_mrc_Rat_t                             rat,                 ///< [IN] Radio Access Technology
    int32_t                                  lowerRangeThreshold, ///< [IN] lower-range Signal
                                                                  ///      strength threshold in dBm
    int32_t                                  upperRangeThreshold, ///< [IN] upper-range Signal
                                                                  ///      strength threshold in dBm
    le_mrc_SignalStrengthChangeHandlerFunc_t handlerFuncPtr,      ///< [IN] The handler function
    void*                                    contextPtr           ///< [IN] The handler's context
)
{
    le_event_HandlerRef_t        handlerRef;

    if (handlerFuncPtr == NULL)
    {
        LE_KILL_CLIENT("Handler function is NULL !");
        return NULL;
    }

    if((!lowerRangeThreshold) || (!upperRangeThreshold) ||
       (lowerRangeThreshold >= upperRangeThreshold)     ||
       (rat< LE_MRC_RAT_GSM) || (rat> LE_MRC_RAT_CDMA))
    {
        LE_KILL_CLIENT("Bad input parameters !");
        return NULL;
    }

    if (pa_mrc_SetSignalStrengthIndThresholds(rat,
                                              lowerRangeThreshold,
                                              upperRangeThreshold) != LE_OK)
    {
        LE_KILL_CLIENT("Failed to set PA Signal Strength Indication thresholds!");
        return NULL;
    }

    switch(rat)
    {
        case LE_MRC_RAT_GSM:
            handlerRef = le_event_AddLayeredHandler("GsmSsChangeHandler",
                                                    GsmSsChangeId,
                                                    FirstLayerGsmSsChangeHandler,
                                                    (le_event_HandlerFunc_t)handlerFuncPtr);
            break;

        case LE_MRC_RAT_UMTS:
            handlerRef = le_event_AddLayeredHandler("UmtsSsChangeHandler",
                                                    UmtsSsChangeId,
                                                    FirstLayerUmtsSsChangeHandler,
                                                    (le_event_HandlerFunc_t)handlerFuncPtr);
            break;

        case LE_MRC_RAT_LTE:
            handlerRef = le_event_AddLayeredHandler("LteSsChangeHandler",
                                                    LteSsChangeId,
                                                    FirstLayerLteSsChangeHandler,
                                                    (le_event_HandlerFunc_t)handlerFuncPtr);
            break;

        case LE_MRC_RAT_CDMA:
            handlerRef = le_event_AddLayeredHandler("CdmaSsChangeHandler",
                                                    CdmaSsChangeId,
                                                    FirstLayerCdmaSsChangeHandler,
                                                    (le_event_HandlerFunc_t)handlerFuncPtr);
            break;

        case LE_MRC_RAT_UNKNOWN:
        default:
            return NULL;
    }

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (le_mrc_SignalStrengthChangeHandlerRef_t)(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to remove an handler for Signal Strength value changes.
 */
//--------------------------------------------------------------------------------------------------
void le_mrc_RemoveSignalStrengthChangeHandler
(
    le_mrc_SignalStrengthChangeHandlerRef_t    handlerRef ///< [IN] The handler reference.
)
{
   le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the serving cell Identifier.
 *
 * @return The Cell Identifier. 0xFFFFFFFF value is returned if the value is not available.
 */
//--------------------------------------------------------------------------------------------------
uint32_t le_mrc_GetServingCellId
(
    void
)
{
    uint32_t cellId;

    if (pa_mrc_GetServingCellId(&cellId) == LE_OK)
    {
        return cellId;
    }
    else
    {
        LE_ERROR("Cannot retrieve the serving cell Identifier!");
        return 0xFFFFFFFF;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the Tracking Area Code of the serving cell.
 *
 * @return The Tracking Area Code. 0xFFFF value is returned if the value is not available.
 */
//--------------------------------------------------------------------------------------------------
uint16_t le_mrc_GetServingCellLteTracAreaCode
(
    void
)
{
    uint16_t tac;

    if (pa_mrc_GetServingCellLteTracAreaCode(&tac) == LE_OK)
    {
        return tac;
    }
    else
    {
        LE_ERROR("Cannot retrieve the serving cell Tracking area code!");
        return 0xFFFF;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the Location Area Code of the serving cell.
 *
 * @return The Location Area Code. 0xFFFFFFFF value is returned if the value is not available.
 */
//--------------------------------------------------------------------------------------------------
uint32_t le_mrc_GetServingCellLocAreaCode
(
    void
)
{
    uint32_t lac;

    if (pa_mrc_GetServingCellLocAreaCode(&lac) == LE_OK)
    {
        return lac;
    }
    else
    {
        LE_ERROR("Cannot retrieve the serving cell Identifier!");
        return 0xFFFFFFFF;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the Bit mask for 2G/3G Band capabilities.
 *
 * @return
 *  - LE_OK              on success
 *  - LE_FAULT           on failure
 *  - LE_UNSUPPORTED     The platform does not support this operation.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_GetBandCapabilities
(
    le_mrc_BandBitMask_t* bandMaskPtr ///< [OUT] Bit mask for 2G/3G Band capabilities.
)
{
    if (bandMaskPtr == NULL)
    {
        LE_KILL_CLIENT("bandMaskPtr is NULL !");
        return LE_FAULT;
    }

    return pa_mrc_GetBandCapabilities(bandMaskPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the Bit mask for LTE Band capabilities.
 *
 * @return
 *  - LE_OK              on success
 *  - LE_FAULT           on failure
 *  - LE_UNSUPPORTED     The platform does not support this operation.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_GetLteBandCapabilities
(
    le_mrc_LteBandBitMask_t* bandMaskPtr ///< [OUT] Bit mask for LTE Band capabilities.
)
{
    if (bandMaskPtr == NULL)
    {
        LE_KILL_CLIENT("bandMaskPtr is NULL !");
        return LE_FAULT;
    }

    return pa_mrc_GetLteBandCapabilities(bandMaskPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the Bit mask for TD-SCDMA Band capabilities.
 *
 * @return
 *  - LE_OK              on success
 *  - LE_FAULT           on failure
 *  - LE_UNSUPPORTED     The platform does not support this operation.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_GetTdScdmaBandCapabilities
(
    le_mrc_TdScdmaBandBitMask_t* bandMaskPtr ///< [OUT] Bit mask for TD-SCDMA Band capabilities.
)
{
    if (bandMaskPtr == NULL)
    {
        LE_KILL_CLIENT("bandMaskPtr is NULL !");
        return LE_FAULT;
    }

    return pa_mrc_GetTdScdmaBandCapabilities(bandMaskPtr);
}
