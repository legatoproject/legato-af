/** @file pa_mrc.c
 *
 * AT implementation of c_pa_mrc API.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#include "legato.h"

#include "pa.h"
#include "pa_mrc.h"
#include "pa_common_local.h"

#include "atManager/inc/atCmdSync.h"
#include "atManager/inc/atPorts.h"

#define DEFAULT_REGSTATE_POOL_SIZE  1

static le_mem_PoolRef_t       RegStatePoolRef;

static le_event_Id_t          EventUnsolicitedId;
static le_event_Id_t          EventNewRcStatusId;

static pa_mrc_NetworkRegSetting_t ThisMode=PA_MRC_DISABLE_REG_NOTIFICATION;

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize pattern matching for unsolicited +CREG notification.
 *
 */
//--------------------------------------------------------------------------------------------------
static void SubscribeUnsolCREG
(
    pa_mrc_NetworkRegSetting_t  mode ///< [IN] The selected Network registration mode.
)
{
    atmgr_UnSubscribeUnsolReq(atports_GetInterface(ATPORT_COMMAND),EventUnsolicitedId,"+CREG:");

    if ((mode==PA_MRC_ENABLE_REG_NOTIFICATION) || (mode==PA_MRC_ENABLE_REG_LOC_NOTIFICATION))
    {
        atmgr_SubscribeUnsolReq(atports_GetInterface(ATPORT_COMMAND),
                                       EventUnsolicitedId,
                                       "+CREG:",
                                       false);
    }

    ThisMode=mode;
}

//--------------------------------------------------------------------------------------------------
/**
 * The handler for a new Network Registration Notification.
 *
 */
//--------------------------------------------------------------------------------------------------
static void CREGUnsolHandler(void* reportPtr) {
    atmgr_UnsolResponse_t* unsolPtr = reportPtr;
    uint32_t  numParam=0;
    le_mrc_NetRegState_t  *statePtr;

    LE_DEBUG("Handler received -%s-",unsolPtr->line);

    numParam = atcmd_CountLineParameter(unsolPtr->line);

    if ( ThisMode == PA_MRC_ENABLE_REG_NOTIFICATION )
    {
        if (numParam == 2) {
            statePtr = le_mem_ForceAlloc(RegStatePoolRef);
            switch(atoi(atcmd_GetLineParameter(unsolPtr->line,2)))
            {
                case 0:
                    *statePtr = LE_MRC_REG_NONE;
                    break;
                case 1:
                    *statePtr = LE_MRC_REG_HOME;
                    break;
                case 2:
                    *statePtr = LE_MRC_REG_SEARCHING;
                    break;
                case 3:
                    *statePtr = LE_MRC_REG_DENIED;
                    break;
                case 4:
                    *statePtr = LE_MRC_REG_UNKNOWN;
                    break;
                case 5:
                    *statePtr = LE_MRC_REG_ROAMING;
                    break;
                default:
                    *statePtr = LE_MRC_REG_UNKNOWN;
                    break;
            }
            LE_DEBUG("Send Event with state %d",*statePtr);
            le_event_ReportWithRefCounting(EventNewRcStatusId,statePtr);
        } else {
            LE_WARN("this Response pattern is not expected -%s-",unsolPtr->line);
        }
    } else if (ThisMode == PA_MRC_ENABLE_REG_LOC_NOTIFICATION)
    {
        if (numParam == 5) {
            statePtr = le_mem_ForceAlloc(RegStatePoolRef);
            switch(atoi(atcmd_GetLineParameter(unsolPtr->line,2)))
            {
                case 0:
                    *statePtr = LE_MRC_REG_NONE;
                    break;
                case 1:
                    *statePtr = LE_MRC_REG_HOME;
                    break;
                case 2:
                    *statePtr = LE_MRC_REG_SEARCHING;
                    break;
                case 3:
                    *statePtr = LE_MRC_REG_DENIED;
                    break;
                case 4:
                    *statePtr = LE_MRC_REG_UNKNOWN;
                    break;
                case 5:
                    *statePtr = LE_MRC_REG_ROAMING;
                    break;
                default:
                    *statePtr = LE_MRC_REG_UNKNOWN;
                    break;
            }
            LE_DEBUG("Send Event with state %d",*statePtr);
            le_event_ReportWithRefCounting(EventNewRcStatusId,statePtr);
        } else {
            LE_WARN("this Response pattern is not expected -%s-",unsolPtr->line);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the mrc module
 *
 * @return LE_NOT_POSSIBLE  The function failed to initialize the module.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_Init
(
    void
)
{
    if (atports_GetInterface(ATPORT_COMMAND)==NULL) {
        LE_WARN("radio control Module is not initialize in this session");
        return LE_NOT_POSSIBLE;
    }

    EventUnsolicitedId    = le_event_CreateId("RCEventIdUnsol",sizeof(atmgr_UnsolResponse_t));
    EventNewRcStatusId    = le_event_CreateIdWithRefCounting("EventNewRcStatus");

    le_event_AddHandler("RCUnsolHandler",EventUnsolicitedId  ,CREGUnsolHandler);

    RegStatePoolRef = le_mem_CreatePool("regStatePool",sizeof(le_mrc_NetRegState_t));
    RegStatePoolRef = le_mem_ExpandPool(RegStatePoolRef,DEFAULT_REGSTATE_POOL_SIZE);

    SubscribeUnsolCREG(PA_MRC_ENABLE_REG_LOC_NOTIFICATION);

    pa_mrc_GetNetworkRegConfig(&ThisMode);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the power of the Radio Module.
 *
 * @return LE_FAULT         The function failed, bad power mode.
 * @return LE_NOT_POSSIBLE  The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_SetRadioPower
(
    le_onoff_t    power   ///< [IN] The power state.
)
{
    char atcommand[ATCOMMAND_SIZE] ;

    if (power == LE_ON)
    {
        atcmdsync_PrepareString(atcommand,ATCOMMAND_SIZE,"at+cfun=1");
    }
    else if (power == LE_OFF)
    {
        atcmdsync_PrepareString(atcommand,ATCOMMAND_SIZE,"at+cfun=0");
    }
    else
    {
        return LE_FAULT;
    }

    return atcmdsync_SendStandard(atports_GetInterface(ATPORT_COMMAND),
                                  atcommand,
                                  NULL,
                                  NULL,
                                  30000);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the Radio Module power state.
 *
 * @return LE_NOT_POSSIBLE  The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetRadioPower
(
     le_onoff_t*    powerPtr   ///< [OUT] The power state.
)
{
    int32_t result=LE_NOT_POSSIBLE;
    atcmdsync_ResultRef_t  resRef = NULL;
    const char* interRespPtr[] = {"+CFUN:",NULL};

    result = atcmdsync_SendStandard(atports_GetInterface(ATPORT_COMMAND),
                                    "at+cfun?",
                                    &resRef,
                                    interRespPtr,
                                    30000);

    if ( result != LE_OK ) {
        le_mem_Release(resRef); // release atcmdsync_SendCommandDefaultExt
        return result;
    }
    // If there is more than one line then it mean that the command is OK so the first line is
    // the intermediate one
    if (atcmdsync_GetNumLines(resRef) == 2)
    {
        // it parse just the first line because of '\0'
        char* line = atcmdsync_GetLine(resRef,0);
        // it parse just the first line because of '\0'
        uint32_t numParam = atcmd_CountLineParameter(line);

        // Check is the +CREG intermediate response is in good format
        if (FIND_STRING("+CFUN:",atcmd_GetLineParameter(line,1)))
        {
            if (numParam==2)
            {
                if(atoi(atcmd_GetLineParameter(line,2)) != 0)
                {
                    *powerPtr = LE_ON;
                }
                else
                {
                    *powerPtr = LE_OFF;
                }
                result = LE_OK;
            } else {
                LE_WARN("this pattern is not expected");
                result=LE_NOT_POSSIBLE;
            }
        } else {
            LE_WARN("this pattern is not expected");
            result=LE_NOT_POSSIBLE;
        }
    }

    le_mem_Release(resRef);     // Release atcmdsync_SendCommand

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register a handler for Radio Access Technology change handling.
 *
 * @return A handler reference, which is only needed for later removal of the handler.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_event_HandlerRef_t pa_mrc_SetRatChangeHandler
(
    pa_mrc_RatChangeHdlrFunc_t handlerFuncPtr ///< [IN] The handler function.
)
{
    // TODO: implement this function
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to unregister the handler for Radio Access Technology change
 * handling.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
void pa_mrc_RemoveRatChangeHandler
(
    le_event_HandlerRef_t handlerRef
)
{
    // TODO: implement this function
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register a handler for Network registration state handling.
 *
 * @return A handler reference, which is only needed for later removal of the handler.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_event_HandlerRef_t pa_mrc_AddNetworkRegHandler
(
    pa_mrc_NetworkRegHdlrFunc_t regStateHandler ///< [IN] The handler function to handle the
                                                 ///       Network registration state.
)
{
    LE_DEBUG("Set new Radio Control handler");

    if (regStateHandler==NULL)
    {
        LE_FATAL("new Radio Control handler is NULL");
    }

    return (le_event_AddHandler("NewRegStateHandler",
                                EventNewRcStatusId,
                                (le_event_HandlerFunc_t) regStateHandler));
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to unregister the handler for Network registration state handling.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_RemoveNetworkRegHandler
(
    le_event_HandlerRef_t handlerRef
)
{
    le_event_RemoveHandler(handlerRef);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function configures the Network registration setting.
 *
 * @return LE_NOT_POSSIBLE  The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_ConfigureNetworkReg
(
    pa_mrc_NetworkRegSetting_t  setting ///< [IN] The selected Network registration setting.
)
{
    char atcommand[ATCOMMAND_SIZE] ;

    atcmdsync_PrepareString(atcommand,ATCOMMAND_SIZE,"at+creg=%d", setting);

    return atcmdsync_SendStandard(atports_GetInterface(ATPORT_COMMAND),
                                  atcommand,
                                  NULL,
                                  NULL,
                                  30000);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function gets information of the Network registration.
 *
 * @return LE_NOT_POSSIBLE  The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK           The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetNetworkReg
(
    bool        first,      ///< true -> mode, false -> state
    int32_t*    valuePtr    ///< value that will be return
)
{
    int32_t result=LE_NOT_POSSIBLE;
    atcmdsync_ResultRef_t  resRef = NULL;
    const char* interRespPtr[] = {"+CREG:",NULL};

    result = atcmdsync_SendStandard(atports_GetInterface(ATPORT_COMMAND),
                                    "at+creg?",
                                    &resRef,
                                    interRespPtr,
                                    30000);

    if ( result != LE_OK ) {
        le_mem_Release(resRef);     // release atcmdsync_SendCommandDefaultExt
        return result;
    }

    // If there is more than one line then it mean that the command is OK so the first line is
    // the intermediate one
    if (atcmdsync_GetNumLines(resRef) == 2)
    {
        // it parse just the first line because of '\0'
        char* line = atcmdsync_GetLine(resRef,0);
        uint32_t numParam = atcmd_CountLineParameter(line);
        // it parse just the first line because of '\0'

        if (FIND_STRING("+CREG:",atcmd_GetLineParameter(line,1)))
        {
            if ((numParam>2) && (numParam<7))
            {
                int32_t val;
                if (first) {
                    val=(int32_t)atoi(atcmd_GetLineParameter(line,2));
                } else {
                    val=(int32_t)atoi(atcmd_GetLineParameter(line,3));
                }
                switch(val)
                {
                    case 0:
                        *valuePtr = LE_MRC_REG_NONE;
                        break;
                    case 1:
                        *valuePtr = LE_MRC_REG_HOME;
                        break;
                    case 2:
                        *valuePtr = LE_MRC_REG_SEARCHING;
                        break;
                    case 3:
                        *valuePtr = LE_MRC_REG_DENIED;
                        break;
                    case 4:
                        *valuePtr = LE_MRC_REG_UNKNOWN;
                        break;
                    case 5:
                        *valuePtr = LE_MRC_REG_ROAMING;
                        break;
                    default:
                        *valuePtr = LE_MRC_REG_UNKNOWN;
                        break;
                }
                result = LE_OK;
            } else {
                LE_WARN("this pattern is not expected");
                result=LE_NOT_POSSIBLE;
            }
        } else {
            LE_WARN("this pattern is not expected");
            result=LE_NOT_POSSIBLE;
        }
    }

    le_mem_Release(resRef);     // Release atcmdsync_SendCommand

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the Network registration setting.
 *
 * @return LE_BAD_PARAMETER Bad parameter passed to the function
 * @return LE_NOT_POSSIBLE  The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK           The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetNetworkRegConfig
(
    pa_mrc_NetworkRegSetting_t*  settingPtr   ///< [OUT] The selected Network registration setting.
)
{
    le_result_t result=LE_NOT_POSSIBLE;
    int32_t val;

    if (!settingPtr)
    {
        LE_WARN("One parameter is NULL");
        return LE_BAD_PARAMETER;
    }

    result = GetNetworkReg(true,&val);

    if ( result == LE_OK )
    {
        *settingPtr = (pa_mrc_NetworkRegSetting_t)val;
        ThisMode = val;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the Network registration state.
 *
 * @return LE_BAD_PARAMETER Bad parameter passed to the function
 * @return LE_NOT_POSSIBLE  The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK           The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetNetworkRegState
(
    le_mrc_NetRegState_t* statePtr  ///< [OUT] The network registration state.
)
{
    le_result_t result=LE_NOT_POSSIBLE;
    int32_t val;

    if (!statePtr)
    {
        LE_WARN("One parameter is NULL");
        return LE_BAD_PARAMETER;
    }

    result = GetNetworkReg(false,&val);

    if ( result == LE_OK )
    {
        *statePtr = (le_mrc_NetRegState_t)val;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the Signal Quality information.
 *
 * @return LE_BAD_PARAMETER Bad parameter passed to the function
 * @return LE_OUT_OF_RANGE  The signal quality values are not known or not detectable.
 * @return LE_NOT_POSSIBLE  The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetSignalQuality
(
    int32_t*          rssiPtr    ///< [OUT] The received signal strength (in dBm).
)
{
    int32_t result=LE_NOT_POSSIBLE;
    atcmdsync_ResultRef_t  resRef = NULL;
    const char* interRespPtr[] = {"+CSQ:",NULL};

    if (!rssiPtr)
    {
        LE_WARN("One parameter is NULL");
        return LE_BAD_PARAMETER;
    }

    result = atcmdsync_SendStandard(atports_GetInterface(ATPORT_COMMAND),
                                    "at+csq",
                                    &resRef,
                                    interRespPtr,
                                    30000);

    if ( result != LE_OK ) {
        le_mem_Release(resRef);     // Release atcmdsync_SendCommandDefaultExt
        return result;
    }

    // If there is more than one line then it mean that the command is OK so the first line is
    if (atcmdsync_GetNumLines(resRef) == 2)
    {
        // it parse just the first line because of '\0'
        char* line = atcmdsync_GetLine(resRef,0);
        uint32_t numParam = atcmd_CountLineParameter(line);
        // it parse just the first line because of '\0'

        if (FIND_STRING("+CSQ:",atcmd_GetLineParameter(line,1)))
        {
            if (numParam==3)
            {
                uint32_t val2 = atoi(atcmd_GetLineParameter(line,2));
                if (val2==99)
                {
                    LE_WARN("Quality signal not detectable");
                    result = LE_OUT_OF_RANGE;
                }
                else
                {
                    *rssiPtr = (-113+(2*val2));
                    result = LE_OK;
                }
            } else {
                LE_WARN("this pattern is not expected");
                result=LE_NOT_POSSIBLE;
            }
        } else {
            LE_WARN("this pattern is not expected");
            result=LE_NOT_POSSIBLE;
        }
    }

    le_mem_Release(resRef);     // Release atcmdsync_SendCommand

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the Home Network Name information.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the Home Network Name can't fit in nameStr
 *      - LE_NOT_POSSIBLE on any other failure
 * @TODO
 *      implementation
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetHomeNetworkName
(
    char       *nameStr,               ///< [OUT] the home network Name
    size_t      nameStrSize            ///< [IN] the nameStr size
)
{
    return LE_NOT_POSSIBLE;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to delete the list of Scan Information
 *
 *
 * @TODO     implementation
 */
//--------------------------------------------------------------------------------------------------
void pa_mrc_DeleteScanInformation
(
    le_dls_List_t *scanInformationListPtr ///< [IN] list of pa_mrc_ScanInformation_t
)
{
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to perform a network scan.
 *
 * @return LE_FAULT         The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_COMM_ERROR    Radio link failure occurred.
 * @return LE_OK            The function succeeded.
 *
 * @TODO     implementation
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_PerformNetworkScan
(
    le_mrc_Rat_t        networkMask,           ///< [IN] The network mask
    pa_mrc_ScanType_t   scanType,              ///< [IN] the scan type
    le_dls_List_t      *scanInformationListPtr ///< [OUT] list of pa_mrc_ScanInformation_t
)
{
    return LE_NOT_POSSIBLE;
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
 * @TODO     implementation
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetScanInformationName
(
    pa_mrc_ScanInformation_t *scanInformationPtr,   ///< [IN] The scan information
    char *namePtr, ///< [OUT] Name of operator
    size_t nameSize ///< [IN] The size in bytes of the namePtr buffer
)
{
    return LE_NOT_POSSIBLE;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to add a new mobile country/network code in the list
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_POSSIBLE for all other errors
 *
 * @TODO     implementation
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_AddPreferredOperators
(
    le_dls_List_t      *PreferredOperatorsListPtr,   ///< [OUT] List of preferred network operator
    char                mcc[LE_MRC_MCC_BYTES],      ///< [IN] Mobile Country Code
    char                mnc[LE_MRC_MNC_BYTES],      ///< [IN] Mobile Network Code
    le_mrc_Rat_t        ratMask                     ///< [IN] Radio Access Technology mask
)
{
    return LE_NOT_POSSIBLE;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to clear the preferred list.
 *
 * @TODO     implementation
 */
//--------------------------------------------------------------------------------------------------
void pa_mrc_DeletePreferredOperators
(
    le_dls_List_t      *PreferredOperatorsListPtr ///< [IN] List of preferred network operator
)
{
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to apply the preferred list into the modem
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_POSSIBLE for all other errors
 *
 * @TODO     implementation
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_SavePreferredOperators
(
    le_dls_List_t      *PreferredOperatorsListPtr ///< [IN] List of preferred network operator
)
{
    return LE_NOT_POSSIBLE;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register on a mobile network [mcc;mnc]
 *
 * @return LE_NOT_POSSIBLE  The function failed to register.
 * @return LE_OK            The function succeeded to register,
 *
 * @TODO     implementation
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_RegisterNetwork
(
    const char *mccPtr,   ///< [IN] Mobile Country Code
    const char *mncPtr    ///< [IN] Mobile Network Code
)
{
    return LE_NOT_POSSIBLE;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register automatically on network
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_POSSIBLE for all other errors
 *
 * @TODO     implementation
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_SetAutomaticNetworkRegistration
(
    void
)
{
    return LE_NOT_POSSIBLE;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the Radio Access Technology.
 *
 * @return LE_FAULT The function failed to get the Signal Quality information.
 * @return LE_OK    The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetRadioAccessTechInUse
(
    le_mrc_Rat_t*   ratPtr    ///< [OUT] The Radio Access Technology.
)
{
    // TODO: implement this function

    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the Radio Access Technology Preference
 *
 * @return LE_OK on success
 * @return LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_SetRatPreference
(
    uint8_t rats ///< [IN] A bit mask to set the Radio Access Technology preference.
)
{
    // TODO: implement this function
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the Band Preference
 *
 * @return LE_OK on success
 * @return LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_SetBandPreference
(
    uint64_t bands ///< [IN] A bit mask to set the Band preference.
)
{
    // TODO: implement this function
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the LTE Band Preference
 *
 * @return LE_OK on success
 * @return LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_SetLteBandPreference
(
    uint64_t bands ///< [IN] A bit mask to set the LTE Band preference.
)
{
    // TODO: implement this function
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the TD-SCDMA Band Preference
 *
 * @return LE_OK on success
 * @return LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_SetTdScdmaBandPreference
(
    uint8_t tdsCdmaBands ///< [IN] A bit mask to set the TD-SCDMA Band preference.
)
{
    // TODO: implement this function
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function retrieves the Neighboring Cells information.
 * Each cell information is queued in the list specified with the IN/OUT parameter.
 * Neither add nor remove of elements in the list can be done outside this function.
 *
 * @return LE_FAULT          The function failed to retrieve the Neighboring Cells information.
 * @return a positive value  The function succeeded. The number of cells which the information have
 *                           been retrieved.
 */
//--------------------------------------------------------------------------------------------------
int32_t pa_mrc_GetNeighborCellsInfo
(
    le_dls_List_t*   cellInfoListPtr    ///< [OUT] The Neighboring Cells information.
)
{
    // TODO: implement this function
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to delete the list of neighboring cells information.
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_mrc_DeleteNeighborCellsInfo
(
    le_dls_List_t *cellInfoListPtr ///< [IN] list of pa_mrc_CellInfo_t
)
{
    // TODO: implement this function
    return ;
}

