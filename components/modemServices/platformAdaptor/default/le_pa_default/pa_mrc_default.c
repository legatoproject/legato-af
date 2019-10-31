/**
 * @file pa_mrc_default.c
 *
 * Default implementation of @ref c_pa_mrc.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"
#include "pa_mrc.h"

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
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
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
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
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
    LE_ERROR("Unsupported function called");
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to unregister the handler for Radio Access Technology change
 * handling.
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_mrc_RemoveRatChangeHandler
(
    le_event_HandlerRef_t handlerRef
)
{
    LE_ERROR("Unsupported function called");
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
    LE_ERROR("Unsupported function called");
    return NULL;
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
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register a handler for Circuit Switched change handling.
 *
 * @return A handler reference, which is only needed for later removal of the handler.
 *
 */
//--------------------------------------------------------------------------------------------------
le_event_HandlerRef_t pa_mrc_SetCSChangeHandler
(
    pa_mrc_ServiceChangeHdlrFunc_t handlerFuncPtr ///< [IN] The handler function.
)
{
    LE_ERROR("Unsupported function called");
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to unregister the handler for Circuit Switched change
 * handling.
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_mrc_RemoveCSChangeHandler
(
    le_event_HandlerRef_t handlerRef
)
{
    LE_ERROR("Unsupported function called");
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register a handler for Packet Switched change handling.
 *
 * @return A handler reference, which is only needed for later removal of the handler.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_event_HandlerRef_t pa_mrc_SetPSChangeHandler
(
    pa_mrc_ServiceChangeHdlrFunc_t handlerFuncPtr ///< [IN] The handler function.
)
{
    LE_ERROR("Unsupported function called");
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to unregister the handler for Packet Switched change
 * handling.
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_mrc_RemovePSChangeHandler
(
    le_event_HandlerRef_t handlerRef
)
{
    LE_ERROR("Unsupported function called");
}


//--------------------------------------------------------------------------------------------------
/**
 * This function configures the Network registration setting.
 *
 * @return LE_FAULT         The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_ConfigureNetworkReg
(
    pa_mrc_NetworkRegSetting_t  setting ///< [IN] The selected Network registration setting.
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the Network registration setting.
 *
 * @return LE_BAD_PARAMETER Bad parameter passed to the function
 * @return LE_FAULT         The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetNetworkRegConfig
(
    pa_mrc_NetworkRegSetting_t*  settingPtr   ///< [OUT] The selected Network registration setting.
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the platform specific network registration error code.
 *
 * @return
 *      - LE_OK             on success
 *      - LE_FAULT          for all other errors
 */
//--------------------------------------------------------------------------------------------------
int32_t pa_mrc_GetPlatformSpecificRegistrationErrorCode
(
    void
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the Network registration state.
 *
 * @return LE_BAD_PARAMETER Bad parameter passed to the function
 * @return LE_FAULT         The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetNetworkRegState
(
    le_mrc_NetRegState_t* statePtr  ///< [OUT] The network registration state.
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the Signal Strength information.
 *
 * @return LE_BAD_PARAMETER Bad parameter passed to the function
 * @return LE_OUT_OF_RANGE  The signal strength values are not known or not detectable.
 * @return LE_FAULT         The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetSignalStrength
(
    int32_t*          rssiPtr    ///< [OUT] The received signal strength (in dBm).
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the Current Network information.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the current network name can't fit in nameStr
 *      - LE_FAULT on any other failure
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
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to delete the list of Scan Information
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_mrc_DeleteScanInformation
(
    le_dls_List_t *scanInformationListPtr ///< [IN] list of pa_mrc_ScanInformation_t
)
{
    LE_ERROR("Unsupported function called");}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to delete the list of pci Scan Information
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_mrc_DeletePciScanInformation
(
    le_dls_List_t *scanInformationListPtr ///< [IN] list of pa_mrc_ScanInformation_t
)
{
    LE_ERROR("Unsupported function called");}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to delete the list of Plmn Information
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_mrc_DeletePlmnScanInformation
(
    le_dls_List_t *scanInformationListPtr ///< [IN] list of pa_mrc_PlmnInformation_t
)
{
    LE_ERROR("Unsupported function called");}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to perform a network scan.
 *
 * @return LE_FAULT         The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_COMM_ERROR    Radio link failure occurred.
 * @return LE_OK            The function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_PerformNetworkScan
(
    le_mrc_RatBitMask_t ratMask,               ///< [IN] The network mask
    pa_mrc_ScanType_t   scanType,              ///< [IN] the scan type
    le_dls_List_t      *scanInformationListPtr ///< [OUT] list of pa_mrc_ScanInformation_t
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to perform a network scan.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the operator name would not fit in buffer
 *      - LE_FAULT for all other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetScanInformationName
(
    pa_mrc_ScanInformation_t *scanInformationPtr,   ///< [IN] The scan information
    char *namePtr, ///< [OUT] Name of operator
    size_t nameSize ///< [IN] The size in bytes of the namePtr buffer
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the number of preferred operators present in the list.
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_CountPreferredOperators
(
    bool      plmnStatic,   ///< [IN] Include Static preferred Operators.
    bool      plmnUser,     ///< [IN] Include Users preferred Operators.
    int32_t*  nbItemPtr     ///< [OUT] number of Preferred operator found if success.
)
{
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the current preferred operators.
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_FOUND if Preferred operator list is not available
 */
//--------------------------------------------------------------------------------------------------
int32_t pa_mrc_GetPreferredOperators
(
    pa_mrc_PreferredNetworkOperator_t*   preferredOperatorPtr,
                       ///< [IN/OUT] The preferred operators pointer.
    bool  plmnStatic,  ///< [IN] Include Static preferred Operators.
    bool  plmnUser,    ///< [IN] Include Users preferred Operators.
    int32_t* nbItemPtr ///< [IN/OUT] number of Preferred operator to find (in) and written (out).
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
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
    le_dls_List_t      *preferredOperatorsListPtr ///< [IN] List of preferred network operator
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register on a mobile network [mcc;mnc]
 *
 * @return LE_FAULT         The function failed to register.
 * @return LE_DUPLICATE     There is already a registration in progress for [mcc;mnc].
 * @return LE_TIMEOUT       Registration attempt timed out.
 * @return LE_OK            The function succeeded to register.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_RegisterNetwork
(
    const char *mccPtr,   ///< [IN] Mobile Country Code
    const char *mncPtr    ///< [IN] Mobile Network Code
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
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
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the Radio Access Technology currently in use.
 *
 * @return
 * - LE_OK              On success
 * - LE_FAULT           On failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetRadioAccessTechInUse
(
    le_mrc_Rat_t*   ratPtr    ///< [OUT] The Radio Access Technology.
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the Radio Access Technology Preferences
 *
 * @return
 * - LE_OK              On success.
 * - LE_FAULT           On failure.
 * - LE_UNSUPPORTED     Not supported by platform.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_SetRatPreferences
(
    le_mrc_RatBitMask_t ratMask  ///< [IN] A bit mask to set the Radio Access Technology
                                 ///<    preferences.
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
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
    LE_ERROR("Unsupported function called");
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
    le_mrc_RatBitMask_t* ratMaskPtr  ///< [OUT] A bit mask to get the Radio Access Technology
                                     ///<  preferences.
)
{
    LE_ERROR("Unsupported function called");
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
    le_mrc_BandBitMask_t bands      ///< [IN] A bit mask to set the Band preferences.
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
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
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the LTE Band Preferences
 *
 * @return
 * - LE_OK              on success
 * - LE_FAULT           on failure
 * - LE_UNSUPPORTED     the platform doesn't support setting LTE Band preferences.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_SetLteBandPreferences
(
    le_mrc_LteBandBitMask_t bands ///< [IN] A bit mask to set the LTE Band preferences.
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
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
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the TD-SCDMA Band Preferences
 *
 * @return
 * - LE_OK           On success
 * - LE_FAULT        On failure
 * - LE_UNSUPPORTED  The platform doesn't support setting TD-SCDMA Band preferences.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_SetTdScdmaBandPreferences
(
    le_mrc_TdScdmaBandBitMask_t bands ///< [IN] A bit mask to set the TD-SCDMA Band Preferences.
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the TD-SCDMA Band Preferences
 *
 * @return
 * - LE_OK           On success
 * - LE_FAULT        On failure
 * - LE_UNSUPPORTED  The platform doesn't support getting TD-SCDMA Band preferences.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetTdScdmaBandPreferences
(
    le_mrc_TdScdmaBandBitMask_t* bandsPtr ///< [OUT] A bit mask to get the TD-SCDMA Band
                                          ///<       preferences.
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
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
    le_dls_List_t*   cellInfoListPtr    ///< [IN/OUT] The Neighboring Cells information.
)
{
    LE_ERROR("Unsupported function called");
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
    LE_ERROR("Unsupported function called");
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
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
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
    LE_ERROR("Unsupported function called");
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
    LE_ERROR("Unsupported function called");
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
    LE_ERROR("Unsupported function called");
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
    le_mrc_Rat_t rat,                 ///< [IN] Radio Access Technology
    int32_t      lowerRangeThreshold, ///< [IN] lower-range threshold in dBm
    int32_t      upperRangeThreshold  ///< [IN] upper-range strength threshold in dBm
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set and activate the delta for signal strength indications.
 *
 * @return
 *  - LE_FAULT  Function failed.
 *  - LE_OK     Function succeeded.
 *  - LE_BAD_PARAMETER  Bad parameters.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_SetSignalStrengthIndDelta
(
    le_mrc_Rat_t rat,    ///< [IN] Radio Access Technology
    uint16_t     delta   ///< [IN] Signal delta in units of 0.1 dB
)
{
    LE_ERROR("Unsupported function called");
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
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the Tracking Area Code of the serving cell.
 *
 * @return
 *  - LE_FAULT  Function failed.
 *  - LE_OK     Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetServingCellLteTracAreaCode
(
    uint16_t* tacPtr ///< [OUT] Tracking Area Code of the serving cell.
)
{
    LE_ERROR("Unsupported function called");
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
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the Band capabilities
 *
 * @return
 *  - LE_OK              on success
 *  - LE_FAULT           on failure
 *  - LE_UNSUPPORTED     The platform does not support this operation.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetBandCapabilities
(
    le_mrc_BandBitMask_t* bandsPtr ///< [OUT] A bit mask to get the Band capabilities.
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the LTE Band capabilities
 *
 * @return
 *  - LE_OK              on success
 *  - LE_FAULT           on failure
 *  - LE_UNSUPPORTED     The platform does not support this operation.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetLteBandCapabilities
(
    le_mrc_LteBandBitMask_t* bandsPtr ///< [OUT] Bit mask to get the LTE Band capabilities.
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the TD-SCDMA Band capabilities
 *
 * @return
 *  - LE_OK              on success
 *  - LE_FAULT           on failure
 *  - LE_UNSUPPORTED     The platform does not support this operation.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetTdScdmaBandCapabilities
(
    le_mrc_TdScdmaBandBitMask_t* bandsPtr ///< [OUT] Bit mask to get the TD-SCDMA Band capabilities.
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the Packet Switched state.
 *
 * @return
 *  - LE_FAULT  Function failed.
 *  - LE_OK     Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetPacketSwitchedState
(
    le_mrc_NetRegState_t* statePtr  ///< [OUT] The current Packet switched state.
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register a handler to report network reject code.
 *
 * @return A handler reference, which is only needed for later removal of the handler.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_event_HandlerRef_t pa_mrc_AddNetworkRejectIndHandler
(
    pa_mrc_NetworkRejectIndHdlrFunc_t networkRejectIndHandler, ///< [IN] The handler function to
                                                               ///< report network reject
                                                               ///< indication.
    void*                             contextPtr               ///< [IN] The context to be given to
                                                               ///< the handler.
)
{
    LE_ERROR("Unsupported function called");
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to unregister the handler for network reject indication handling.
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_mrc_RemoveNetworkRejectIndHandler
(
    le_event_HandlerRef_t handlerRef
)
{
    LE_ERROR("Unsupported function called");
}

//--------------------------------------------------------------------------------------------------
/**
 * This function activates or deactivates jamming detection notification.
 *
 * * @return
 *      - LE_OK on success
 *      - LE_FAULT on failure
 *      - LE_DUPLICATE if jamming detection is already activated and an activation is requested
 *      - LE_UNSUPPORTED if jamming detection is not supported
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_SetJammingDetection
(
    bool activation     ///< [IN] Notification activation request
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function returns the jamming detection notification status.
 *
 * * @return
 *      - LE_OK on success
 *      - LE_BAD_PARAMETER if the parameter is invalid
 *      - LE_FAULT on failure
 *      - LE_UNSUPPORTED if jamming detection is not supported or if this request is not supported
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetJammingDetection
(
    bool* activationPtr     ///< [IN] Notification activation request
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the SAR backoff state
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_FAULT          The function failed.
 *  - LE_UNSUPPORTED    The feature is not supported.
 *  - LE_OUT_OF_RANGE   The provided index is out of range.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_SetSarBackoffState
(
    uint8_t state      ///< [IN] New state to enable.
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the SAR backoff state
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_FAULT          The function failed.
 *  - LE_UNSUPPORTED    The feature is not supported.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetSarBackoffState
(
    uint8_t* statePtr     ///< [OUT] Current state
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function retrieves the network time from the modem.
 *
 * @return
 * - LE_FAULT         The function failed to get the value.
 * - LE_UNAVAILABLE   No valid user time was returned.
 * - LE_UNSUPPORTED   The feature is not supported.
 * - LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_SyncNetworkTime
(
    void
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register a handler to report jamming detection notification.
 *
 * @return A handler reference, which is only needed for later removal of the handler.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_event_HandlerRef_t pa_mrc_AddJammingDetectionIndHandler
(
    pa_mrc_JammingDetectionHandlerFunc_t jammingDetectionIndHandler, ///< [IN] The handler function
                                                                     ///  to handle jamming
                                                                     ///  detection indication.
    void*                               contextPtr                   ///< [IN] The context to be
                                                                     ///  given to the handler.
)
{
    LE_ERROR("Unsupported function called");
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register a handler to report a Network Time event.
 *
 * @return A handler reference, which is only needed for later removal of the handler.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_event_HandlerRef_t pa_mrc_AddNetworkTimeIndHandler
(
    pa_mrc_NetworkTimeHandlerFunc_t networkTimeIndHandler  ///< [IN] The handler function
                                                           ///  to handle network
                                                           ///  time indication.
)
{
    LE_ERROR("Unsupported function called");
    return NULL;
}
