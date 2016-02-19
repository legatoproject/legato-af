/**
 * @file pa_mrc_simu.c
 *
 * Simulation implementation of @ref c_pa_mrc API.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "pa_mrc.h"

#include "pa_simu.h"

//--------------------------------------------------------------------------------------------------
/**
 * The internal current RAT setting
 */
//--------------------------------------------------------------------------------------------------
static le_mrc_Rat_t   Rat = LE_MRC_RAT_GSM;

//--------------------------------------------------------------------------------------------------
/**
 * The internal current BAND settings
 */
//--------------------------------------------------------------------------------------------------
static le_mrc_BandBitMask_t CurrentBand = LE_MRC_BITMASK_BAND_GSM_DCS_1800;
static le_mrc_LteBandBitMask_t CurrentLteBand = LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_11;
static le_mrc_TdScdmaBandBitMask_t CurrentTdScdmaBand = LE_MRC_BITMASK_TDSCDMA_BAND_C;

//--------------------------------------------------------------------------------------------------
/**
 * This event is reported when a Radio Access Technology change indication is received from the
 * modem. The report data is allocated from the associated pool.
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t RatChangeEvent;

//--------------------------------------------------------------------------------------------------
/**
 * This event is reported when a registration state indication is received from the modem. The
 * report data is allocated from the associated pool.
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t NewRegStateEvent;

//--------------------------------------------------------------------------------------------------
/**
 * The pa_mrc_ScanInformation_t pool
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t ScanInformationPool;

//--------------------------------------------------------------------------------------------------
/**
 * The internal radio power state
 */
//--------------------------------------------------------------------------------------------------
static le_onoff_t RadioPower = LE_ON;

//--------------------------------------------------------------------------------------------------
/**
 * The internal manual section mode status
 */
//--------------------------------------------------------------------------------------------------
static bool IsManual = false;


static char CurentMccStr[4];
static char CurentMncStr[4];

//--------------------------------------------------------------------------------------------------
/**
 * This function determine if the tupple (rat,mcc,mnc) is currently provided by the simulation.
 */
//--------------------------------------------------------------------------------------------------
static bool IsNetworkInUse
(
    le_mrc_Rat_t   rat,      /// [IN] RAT
    const char    *mccPtr,   /// [IN] MCC
    const char    *mncPtr    /// [IN] MNC
)
{
    le_mrc_Rat_t currentRat;
    le_result_t res;

    res =  pa_mrc_GetRadioAccessTechInUse(&currentRat);
    LE_FATAL_IF( (res != LE_OK), "Unable to get current RAT");

    if(currentRat != rat)
    {
        return false;
    }

    // @TODO Compare MCC/MNC

    return true;
}

//--------------------------------------------------------------------------------------------------
/**
 * Append a simulated result for the specified RAT to the list of Scan Information
 */
//--------------------------------------------------------------------------------------------------
static void AppendNetworkScanResult
(
    le_mrc_Rat_t   rat,                   /// [IN] Requested simulated RAT result
    le_dls_List_t *scanInformationListPtr ///< [OUT] list of pa_mrc_ScanInformation_t
)
{
    pa_mrc_ScanInformation_t *newScanInformationPtr = NULL;

    char mccStr[LE_MRC_MCC_BYTES];
    char mncStr[LE_MRC_MNC_BYTES];

    newScanInformationPtr = le_mem_ForceAlloc(ScanInformationPool);

    memset(newScanInformationPtr, 0, sizeof(*newScanInformationPtr));
    newScanInformationPtr->link = LE_DLS_LINK_INIT;

    // @TODO Default to SIM MCC/MNC
    strcpy(mccStr, PA_SIMU_SIM_DEFAULT_MCC);
    strcpy(mncStr, PA_SIMU_SIM_DEFAULT_MNC);

    newScanInformationPtr->rat = rat;
    strcpy(newScanInformationPtr->mobileCode.mcc, mccStr);
    strcpy(newScanInformationPtr->mobileCode.mnc, mncStr);
    newScanInformationPtr->isInUse = IsNetworkInUse(rat, mccStr, mncStr);
    newScanInformationPtr->isAvailable = !(newScanInformationPtr->isInUse);
    newScanInformationPtr->isHome = true;
    newScanInformationPtr->isForbidden = false;

    le_dls_Queue(scanInformationListPtr, &(newScanInformationPtr->link));
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the power of the Radio Module.
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_SetRadioPower
(
    le_onoff_t    power   ///< [IN] The power state.
)
{
    if(RadioPower == power)
        return LE_OK;

    switch(power)
    {
        case LE_ON:
        case LE_OFF:
            RadioPower = power;
            break;
        default:
            return LE_FAULT;
    }

    LE_INFO("Turning radio %s", (RadioPower == LE_ON) ? "On" : "Off");

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the Radio Module power state.
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetRadioPower
(
     le_onoff_t*    powerPtr   ///< [OUT] The power state.
)
{
    *powerPtr = RadioPower;
    return LE_OK;
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
    LE_ASSERT(handlerFuncPtr != NULL);

    return le_event_AddHandler(
                "RatChangeHandler",
                RatChangeEvent,
                (le_event_HandlerFunc_t) handlerFuncPtr);
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
    le_event_RemoveHandler(handlerRef);
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
                                                ///        Network registration state.
)
{
    LE_ASSERT(regStateHandler != NULL);

    return le_event_AddHandler(
                "NewRegStateHandler",
                NewRegStateEvent,
                (le_event_HandlerFunc_t) regStateHandler);
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
 * @return LE_NOT_POSSIBLE  The function failed to configure the Network registration setting.
 * @return LE_BAD_PARAMETER The parameters are invalid.
 * @return LE_OUT_OF_RANGE  The parameters values are not in the allowed range.
 * @return LE_COMM_ERROR    The communication device has returned an error.
 * @return LE_TIMEOUT       No response was received from the Modem.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_ConfigureNetworkReg
(
    pa_mrc_NetworkRegSetting_t  setting ///< [IN] The selected Network registration setting.
)
{
    if(setting == PA_MRC_ENABLE_REG_NOTIFICATION)
        return LE_OK;

    return LE_NOT_POSSIBLE;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the Network registration setting.
 *
 * @return LE_NOT_POSSIBLE The function failed to get the Network registration setting.
 * @return LE_COMM_ERROR   The communication device has returned an error.
 * @return LE_TIMEOUT      No response was received from the Modem.
 * @return LE_OK           The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetNetworkRegConfig
(
    pa_mrc_NetworkRegSetting_t*  settingPtr   ///< [OUT] The selected Network registration setting.
)
{
    *settingPtr = PA_MRC_ENABLE_REG_NOTIFICATION;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the Network registration state.
 *
 * @return LE_NOT_POSSIBLE The function failed to get the Network registration state.
 * @return LE_COMM_ERROR   The communication device has returned an error.
 * @return LE_TIMEOUT      No response was received from the Modem.
 * @return LE_OK           The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetNetworkRegState
(
    le_mrc_NetRegState_t* statePtr  ///< [OUT] The network registration state.
)
{
    *statePtr = LE_MRC_REG_HOME;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the Signal Strength information.
 *
 * @return LE_BAD_PARAMETER Bad parameter passed to the function
 * @return LE_OUT_OF_RANGE  The signal strength values are not known or not detectable.
 * @return LE_NOT_POSSIBLE  The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetSignalStrength
(
    int32_t*          rssiPtr    ///< [OUT] The received signal strength (in dBm).
)
{
    if(RadioPower != LE_ON)
    {
        return LE_OUT_OF_RANGE;
    }

    *rssiPtr = -60;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the current network information.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the current network name can't fit in nameStr
 *      - LE_NOT_POSSIBLE on any other failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetCurrentNetwork
(
    char       *nameStr,               ///< [OUT] the home network Name
    size_t      nameStrSize,           ///< [IN]  the nameStr size
    char       *mccStr,                ///< [OUT] the mobile country code
    size_t      mccStrNumElements,     ///< [IN]  the mccStr size
    char       *mncStr,                ///< [OUT] the mobile network code
    size_t      mncStrNumElements      ///< [IN]  the mncStr size
)
{
    le_result_t res = LE_OK;

    if (RadioPower != LE_ON)
    {
        *nameStr = '\0';
        return LE_NOT_POSSIBLE;
    }

    if (nameStr != NULL)
    {
        res = le_utf8_Copy(nameStr, PA_SIMU_MRC_DEFAULT_NAME, nameStrSize, NULL);
        if (res != LE_OK)
            return res;
    }

    if (mccStr != NULL)
    {
        res = le_utf8_Copy(mccStr, CurentMccStr, mccStrNumElements, NULL);
        if (res != LE_OK)
            return res;
    }

    if (mncStr != NULL)
    {
        res = le_utf8_Copy(mncStr, CurentMncStr, mncStrNumElements, NULL);
    }

    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to delete the list of Scan Information
 *
 * @TODO    implementation
 */
//--------------------------------------------------------------------------------------------------
void pa_mrc_DeleteScanInformation
(
    le_dls_List_t *scanInformationListPtr ///< [IN] list of pa_mrc_ScanInformation_t
)
{
    pa_mrc_ScanInformation_t * nodePtr;
    le_dls_Link_t * linkPtr;

    while ((linkPtr = le_dls_Pop(scanInformationListPtr)) != NULL)
    {
        nodePtr = CONTAINER_OF(linkPtr, pa_mrc_ScanInformation_t, link);
        le_mem_Release(nodePtr);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to perform a network scan.
 *
 * @return LE_FAULT         The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_COMM_ERROR    Radio link failure occurred.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_PerformNetworkScan
(
    le_mrc_RatBitMask_t ratMask,               ///< [IN] The network mask
    pa_mrc_ScanType_t   scanType,              ///< [IN] the scan type
    le_dls_List_t      *scanInformationListPtr ///< [OUT] list of pa_mrc_ScanInformation_t
)
{
    if(RadioPower != LE_ON)
    {
        return LE_NOT_POSSIBLE;
    }

    switch(scanType)
    {
        case PA_MRC_SCAN_PLMN:
            break;

        case PA_MRC_SCAN_CSG:
            break;
    }

    if (ratMask & LE_MRC_BITMASK_RAT_GSM)
    {
        AppendNetworkScanResult(LE_MRC_RAT_GSM, scanInformationListPtr);
    }
    if (ratMask & LE_MRC_BITMASK_RAT_UMTS)
    {
        AppendNetworkScanResult(LE_MRC_RAT_UMTS, scanInformationListPtr);
    }
    if (ratMask & LE_MRC_BITMASK_RAT_LTE)
    {
        AppendNetworkScanResult(LE_MRC_RAT_LTE, scanInformationListPtr);
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
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetScanInformationName
(
    pa_mrc_ScanInformation_t *scanInformationPtr,   ///< [IN] The scan information
    char *namePtr, ///< [OUT] Name of operator
    size_t nameSize ///< [IN] The size in bytes of the namePtr buffer
)
{
    if ((!scanInformationPtr) || (!namePtr))
    {
        return LE_NOT_POSSIBLE;
    }

    // @TODO Handle other names than default SIM MCC/MNC
    if ( (0 == strcmp(scanInformationPtr->mobileCode.mcc, PA_SIMU_SIM_DEFAULT_MCC)) &&
         (0 == strcmp(scanInformationPtr->mobileCode.mnc, PA_SIMU_SIM_DEFAULT_MNC)) )
    {
        return le_utf8_Copy(namePtr, PA_SIMU_MRC_DEFAULT_NAME, nameSize, NULL);
    }

    return LE_NOT_POSSIBLE;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the current preferred operators list.
 *
 * @return
 *   - A positive value on success (number of Preferred operator found).
 *   - LE_NOT_FOUND if preferred operator list is not available.
 */
//--------------------------------------------------------------------------------------------------
int32_t pa_mrc_GetPreferredOperatorsList
(
    le_dls_List_t*   preferredOperatorListPtr,    ///< [IN/OUT] The preferred operators list.
    bool  plmnStatic,   ///< [IN] Include Static preferred Operators.
    bool  plmnUser      ///< [IN] Include Users preferred Operators.
)
{
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
le_result_t pa_mrc_AddPreferredOperators
(
    le_dls_List_t      *preferredOperatorsListPtr,   ///< [OUT] List of preferred network operator
    const char*         mccPtr,                      ///< [IN] Mobile Country Code
    const char*         mncPtr,                      ///< [IN] Mobile Network Code
    le_mrc_RatBitMask_t ratMask                      ///< [IN] Radio Access Technology mask
)
{
    LE_DEBUG("Adding [%s,%s] = 0x%.04"PRIX16, mccPtr, mncPtr, ratMask);
    return LE_FAULT;
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
le_result_t pa_mrc_RemovePreferredOperators
(
    le_dls_List_t      *preferredOperatorsListPtr,   ///< [IN] List of preferred network operator
    const char*         mccPtr,                      ///< [IN] Mobile Country Code
    const char*         mncPtr                       ///< [IN] Mobile Network Code
)
{
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to clear the preferred list.
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_mrc_DeletePreferredOperatorsList
(
    le_dls_List_t      *preferredOperatorsListPtr ///< [IN] List of preferred network operator
)
{
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to apply the preferred list into the modem
 *
 * @return
 *      - LE_OK             on success
 *      - LE_FAULT          for all other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_SavePreferredOperators
(
    le_dls_List_t      *PreferredOperatorsListPtr ///< [IN] List of preferred network operator
)
{
    return LE_FAULT;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register on a mobile network [mcc;mnc]
 *
 * @return LE_NOT_POSSIBLE  The function failed to register.
 * @return LE_OK            The function succeeded to register,
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_RegisterNetwork
(
    const char *mccPtr,   ///< [IN] Mobile Country Code
    const char *mncPtr    ///< [IN] Mobile Network Code
)
{

    IsManual = true;

    le_utf8_Copy(CurentMccStr, mccPtr, sizeof(CurentMccStr), NULL);
    le_utf8_Copy(CurentMncStr, mncPtr, sizeof(CurentMncStr), NULL);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register automatically on network
 *
 * @return
 *      - LE_OK             on success
 *      - LE_FAULT          for all other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_SetAutomaticNetworkRegistration
(
    void
)
{
    IsManual = false;

    le_utf8_Copy(CurentMccStr, PA_SIMU_MRC_DEFAULT_MCC, sizeof(CurentMccStr), NULL);
    le_utf8_Copy(CurentMncStr, PA_SIMU_MRC_DEFAULT_MNC, sizeof(CurentMncStr), NULL);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function set the current Radio Access Technology in use.
 */
//--------------------------------------------------------------------------------------------------
void pa_mrcSimu_SetRadioAccessTechInUse
(
    le_mrc_Rat_t   rat  ///< [IN] The Radio Access Technology.
)
{
    Rat = rat;
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
    *ratPtr = Rat;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the Radio Access Technology Preferences
 *
 * @return
 * - LE_OK              on success
 * - LE_FAULT           on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_SetRatPreferences
(
    le_mrc_RatBitMask_t bitMask ///< [IN] A bit mask to set the Radio Access Technology preference.
)
{
    // TODO: implement this function
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the Rat Automatic Radio Access Technology Preference
 *
 * @return
 * - LE_OK              on success
 * - LE_FAULT           on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_SetAutomaticRatPreference
(
    void
)
{
    // TODO: implement this function
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the Radio Access Technology Preferences
 *
 * @return
 * - LE_OK              on success
 * - LE_FAULT           on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetRatPreferences
(
    le_mrc_RatBitMask_t* ratMaskPtr ///< [OUT] A bit mask to get the Radio Access Technology
                                        ///<  preferences.
)
{
    // TODO: implement this function
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the Band Preferences
 *
 * @return
 * - LE_OK              on success
 * - LE_FAULT           on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_SetBandPreferences
(
    le_mrc_BandBitMask_t bands ///< [IN] A bit mask to set the Band preferences.
)
{
    CurrentBand = bands;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the Band Preferences
 *
 * @return
 * - LE_OK              on success
 * - LE_FAULT           on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetBandPreferences
(
    le_mrc_BandBitMask_t* bandsPtr ///< [OUT] A bit mask to get the Band preferences.
)
{
    *bandsPtr = CurrentBand;
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the LTE Band Preferences
 *
 * @return
 * - LE_OK              on success
 * - LE_FAULT           on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_SetLteBandPreferences
(
    le_mrc_LteBandBitMask_t bands ///< [IN] A bit mask to set the LTE Band preferences.
)
{

    CurrentLteBand = bands;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the LTE Band Preferences
 *
 * @return
 * - LE_OK              on success
 * - LE_FAULT           on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetLteBandPreferences
(
    le_mrc_LteBandBitMask_t* bandsPtr ///< [OUT] A bit mask to get the LTE Band preferences.
)
{
    *bandsPtr = CurrentLteBand;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the TD-SCDMA Band Preferences
 *
 * @return
 * - LE_OK              on success
 * - LE_FAULT           on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_SetTdScdmaBandPreferences
(
    le_mrc_TdScdmaBandBitMask_t bands ///< [IN] A bit mask to set the TD-SCDMA Band Preferences.
)
{
    CurrentTdScdmaBand = bands;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the TD-SCDMA Band Preferences
 *
 * @return
 * - LE_OK              on success
 * - LE_FAULT           on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetTdScdmaBandPreferences
(
    le_mrc_TdScdmaBandBitMask_t* bandsPtr ///< [OUT] A bit mask to set the TD-SCDMA Band
                                          ///<  preferences.
)
{
    *bandsPtr = CurrentTdScdmaBand;
    return LE_OK;
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

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get current registration mode
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetNetworkRegistrationMode
(
    bool*   isManualPtr,  ///< [OUT] true if the scan mode is manual, false if it is automatic.
    char*   mccPtr,       ///< [OUT] Mobile Country Code
    size_t  mccPtrSize,   ///< [IN] mccPtr buffer size
    char*   mncPtr,       ///< [OUT] Mobile Network Code
    size_t  mncPtrSize    ///< [IN] mncPtr buffer size
)
{

    le_utf8_Copy(mccPtr, CurentMccStr, mccPtrSize, NULL);
    le_utf8_Copy(mncPtr, CurentMncStr, mncPtrSize, NULL);

    *isManualPtr = IsManual;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function measures the Signal metrics.
 *
 * @return LE_FAULT         The function failed.
 * @return LE_OK            The function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_MeasureSignalMetrics
(
    pa_mrc_SignalMetrics_t* metricsPtr    ///< [OUT] The signal metrics.
)
{
    // TODO: implement this function
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register a handler for Signal Strength change handling.
 *
 * @return A handler reference, which is only needed for later removal of the handler.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_event_HandlerRef_t pa_mrc_AddSignalStrengthIndHandler
(
    pa_mrc_SignalStrengthIndHdlrFunc_t ssIndHandler, ///< [IN] The handler function to handle the
                                                     ///        Signal Strength change indication.
    void*                              contextPtr    ///< [IN] The context to be given to the handler.
)
{
    // TODO: implement this function
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to unregister the handler for Signal Strength change handling.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
void pa_mrc_RemoveSignalStrengthIndHandler
(
    le_event_HandlerRef_t handlerRef
)
{
    // TODO: implement this function
    return ;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set and activate the signal strength thresholds for signal
 * strength indications
 *
 * @return
 *  - LE_FAULT  Function failed.
 *  - LE_OK     Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_SetSignalStrengthIndThresholds
(
    le_mrc_Rat_t rat,                 ///< Radio Access Technology
    int32_t      lowerRangeThreshold, ///< [IN] lower-range threshold in dBm
    int32_t      upperRangeThreshold  ///< [IN] upper-range strength threshold in dBm
)
{
    // TODO: implement this function
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the serving cell Identifier.
 *
 * @return
 *  - LE_FAULT  Function failed.
 *  - LE_OK     Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetServingCellId
(
    uint32_t* cellIdPtr ///< [OUT] main Cell Identifier.
)
{
    // TODO: implement this function
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the Location Area Code of the serving cell.
 *
 * @return
 *  - LE_FAULT  Function failed.
 *  - LE_OK     Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetServingCellLocAreaCode
(
    uint32_t* lacPtr ///< [OUT] Location Area Code of the serving cell.
)
{
    // TODO: implement this function
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the Band capabilities
 *
 * @return
 * - LE_OK              on success
 * - LE_FAULT           on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetBandCapabilities
(
    le_mrc_BandBitMask_t* bandsPtr ///< [OUT] A bit mask to get the Band capabilities.
)
{
    *bandsPtr = LE_MRC_BITMASK_BAND_CLASS_1_ALL_BLOCKS | LE_MRC_BITMASK_BAND_GSM_DCS_1800;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the LTE Band capabilities
 *
 * @return
 * - LE_OK              on success
 * - LE_FAULT           on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetLteBandCapabilities
(
    le_mrc_LteBandBitMask_t* bandsPtr ///< [OUT] Bit mask to get the LTE Band capabilities.
)
{
    *bandsPtr = LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_3 | LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_7;
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the TD-SCDMA Band capabilities
 *
 * @return
 * - LE_OK              on success
 * - LE_FAULT           on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetTdScdmaBandCapabilities
(
    le_mrc_TdScdmaBandBitMask_t* bandsPtr ///< [OUT] Bit mask to get the TD-SCDMA Band capabilities.
)
{
    *bandsPtr = LE_MRC_BITMASK_TDSCDMA_BAND_A | LE_MRC_BITMASK_TDSCDMA_BAND_C;
    return LE_OK;
}


/**
 * MRC Stub initialization.
 *
 * @return LE_OK           The function succeeded.
 */
le_result_t mrc_simu_Init
(
    void
)
{
    LE_INFO("PA MRC Init");

    NewRegStateEvent = le_event_CreateIdWithRefCounting("NewRegStateEvent");
    RatChangeEvent = le_event_CreateIdWithRefCounting("RatChangeEvent");

    ScanInformationPool = le_mem_CreatePool("ScanInformationPool", sizeof(pa_mrc_ScanInformation_t));

    return LE_OK;
}

bool mrc_simu_IsOnline
(
    void
)
{
    le_mrc_NetRegState_t state;
    le_result_t res;

    res = pa_mrc_GetNetworkRegState(&state);
    if(res != LE_OK)
    {
        return false;
    }

    return ( (state == LE_MRC_REG_HOME) || (state == LE_MRC_REG_ROAMING) );
}

