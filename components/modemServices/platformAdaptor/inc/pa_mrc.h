/**
 * @page c_pa_mrc Modem Radio Control Platform Adapter API
 *
 * @ref pa_mrc.h "API Reference"
 *
 * <HR>
 *
 * These APIs are on the top of the platform-dependent adapter layer. They are independent of the
 * implementation. They guarantee the portability on different kind of platform without any changes
 * for the components developed upon these APIs.
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */


/** @file pa_mrc.h
 *
 * Legato @ref c_pa_mrc include file.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#ifndef LEGATO_PARC_INCLUDE_GUARD
#define LEGATO_PARC_INCLUDE_GUARD

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Network Registration setting.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    PA_MRC_DISABLE_REG_NOTIFICATION    = 0, ///< Disable network registration notification result
                                            ///  code.
    PA_MRC_ENABLE_REG_NOTIFICATION     = 1, ///< Enable network registration notification code.
    PA_MRC_ENABLE_REG_LOC_NOTIFICATION = 2, ///< Enable network registration and location
                                            ///  information notification result code if there is a
                                            ///  change of network cell.
}
pa_mrc_NetworkRegSetting_t;

//--------------------------------------------------------------------------------------------------
/**
 * Cell Information structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct pa_mrc_CellInfo
{
    le_dls_Link_t    link;          ///< Structure is part of a @ref c_doublyLinkedList
    uint32_t         index;         ///< The cell number.
    uint32_t         id;            ///< The cell identifier.
    uint16_t         lac;           ///< The location area code.
    int16_t          rxLevel;       ///< The cell Rx level measurement.
    le_mrc_Rat_t     rat;           ///< The cell Radio Access Technology.
    int32_t          umtsEcIo;      ///< The Ec/Io of a UMTS cell.
    int32_t          lteIntraRsrp;  ///< The Reference Signal Receiver Power value of the
                                    ///  Intrafrequency of a LTE cell.
    int32_t          lteIntraRsrq;  ///< The Reference Signal Receiver Quality value of the
                                    ///  Intrafrequency of a LTE cell.
    int32_t          lteInterRsrp;  ///< The Reference Signal Receiver Power value of the
                                    ///  Intrafrequency of a LTE cell.
    int32_t          lteInterRsrq;  ///< The Reference Signal Receiver Quality value of the
                                    ///  Intrafrequency of a LTE cell.
}
pa_mrc_CellInfo_t;

//--------------------------------------------------------------------------------------------------
/**
 * Network Scan Type.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    PA_MRC_SCAN_PLMN = 0,       ///< Scan PLMN
    PA_MRC_SCAN_CSG,            ///< Scan closed subscriber group
}
pa_mrc_ScanType_t;

//--------------------------------------------------------------------------------------------------
/**
 * Mobile code.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char mcc[LE_MRC_MCC_BYTES]; ///< MCC: Mobile Country Code
    char mnc[LE_MRC_MNC_BYTES]; ///< MNC: Mobile Network Code
}
pa_mrc_MobileCode_t;

//--------------------------------------------------------------------------------------------------
/**
 * Preferred Network.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    pa_mrc_MobileCode_t mobileCode; ///< Mobile code
    le_mrc_RatBitMask_t ratMask;    ///< mask of network technology
    le_dls_Link_t       link;       ///< link for the list
    int16_t             index;      ///< Index of the preferred PLMM storage
}
pa_mrc_PreferredNetworkOperator_t;

//--------------------------------------------------------------------------------------------------
/**
 * Network Scan Information.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    pa_mrc_MobileCode_t mobileCode;     ///< Mobile code
    le_mrc_Rat_t        rat;            ///< Radio Access Technology
    bool                isInUse;        ///< network is in use
    bool                isAvailable;    ///< network can be connected
    bool                isHome;         ///< home status
    bool                isForbidden;    ///< forbidden status
    le_dls_Link_t       link;           ///< link for the list
}
pa_mrc_ScanInformation_t;

//--------------------------------------------------------------------------------------------------
/**
 * UMTS metrics.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    int32_t   ecio; ///< Ec/Io value  in dB with 1 decimal place (15 = 1.5 dB)
    int32_t   rscp; ///< Measured RSCP in dBm (only applicable for TD-SCDMA network)
    int32_t   sinr; ///< Measured SINR in dB (only applicable for TD-SCDMA network)
}
pa_mrc_UmtsMetrics_t;

//--------------------------------------------------------------------------------------------------
/**
 * LTE metrics.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    int32_t  rsrq; ///< RSRQ value in dB as measured by L1
    int32_t  rsrp; ///< Current RSRP in dBm as measured by L1
    int32_t  snr;  ///< SNR level in dB with 1 decimal place (15 = 1.5 dB)
}
pa_mrc_LteMetrics_t;

//--------------------------------------------------------------------------------------------------
/**
 * CDMA metrics.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    int32_t  ecio;  ///< ECIO value in dB with 1 decimal place (15 = 1.5 dB)
    int32_t  sinr;  ///< SINR level in dB with 1 decimal place, (only applicable for 1xEV-DO)
    int32_t  io;    ///< Received IO in dBm (only applicable for 1xEV-DO)
}
pa_mrc_CdmaMetrics_t;

//--------------------------------------------------------------------------------------------------
/**
 * Signal metrics.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_mrc_Rat_t             rat;          ///< RAT of the measured signal
    int32_t                  ss;           ///< Signal strength in dBm
    uint32_t                 er;           ///< Bit/Block/Frame/Packet error rate
    union {                                ///< Additional information for UMTS/LTE/CDMA
        pa_mrc_UmtsMetrics_t umtsMetrics;
        pa_mrc_LteMetrics_t  lteMetrics;
        pa_mrc_CdmaMetrics_t cdmaMetrics;
    };
}
pa_mrc_SignalMetrics_t;

//--------------------------------------------------------------------------------------------------
/**
 * Signal Strength change indication structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct {
    le_mrc_Rat_t  rat;          ///< RAT of the measured signal
    int32_t       ss;           ///< Signal strength in dBm
}
pa_mrc_SignalStrengthIndication_t;

//--------------------------------------------------------------------------------------------------
/**
 * Prototype for handler functions used to report Signal Strength changes.
 *
 * @param ssIndPtr The Signal Strength change indication.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*pa_mrc_SignalStrengthIndHdlrFunc_t)
(
    pa_mrc_SignalStrengthIndication_t* ssIndPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Prototype for handler functions used to report the Network registration state.
 *
 * @param regStatePtr The Network registration state.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*pa_mrc_NetworkRegHdlrFunc_t)
(
    le_mrc_NetRegState_t* regStatePtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Prototype for handler functions used to report the Radio Access Technology change.
 *
 * @param ratPtr The Radio Access Technology.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*pa_mrc_RatChangeHdlrFunc_t)
(
    le_mrc_Rat_t* ratPtr
);

//--------------------------------------------------------------------------------------------------
// APIs.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the power of the Radio Module.
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_mrc_SetRadioPower
(
    le_onoff_t    power   ///< [IN] The power state.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the Radio Module power state.
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_mrc_GetRadioPower
(
     le_onoff_t*    powerPtr   ///< [OUT] The power state.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register a handler for Radio Access Technology change handling.
 *
 * @return A handler reference, which is only needed for later removal of the handler.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_event_HandlerRef_t pa_mrc_SetRatChangeHandler
(
    pa_mrc_RatChangeHdlrFunc_t handlerFuncPtr ///< [IN] The handler function.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to unregister the handler for Radio Access Technology change
 * handling.
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void pa_mrc_RemoveRatChangeHandler
(
    le_event_HandlerRef_t handlerRef
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register a handler for Network registration state handling.
 *
 * @return A handler reference, which is only needed for later removal of the handler.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_event_HandlerRef_t pa_mrc_AddNetworkRegHandler
(
    pa_mrc_NetworkRegHdlrFunc_t regStateHandler ///< [IN] The handler function to handle the
                                                ///        Network registration state.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to unregister the handler for Network registration state handling.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_mrc_RemoveNetworkRegHandler
(
    le_event_HandlerRef_t handlerRef
);

//--------------------------------------------------------------------------------------------------
/**
 * This function configures the Network registration setting.
 *
 * @return LE_FAULT         The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_mrc_ConfigureNetworkReg
(
    pa_mrc_NetworkRegSetting_t  setting ///< [IN] The selected Network registration setting.
);

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
LE_SHARED le_result_t pa_mrc_GetNetworkRegConfig
(
    pa_mrc_NetworkRegSetting_t*  settingPtr   ///< [OUT] The selected Network registration setting.
);

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
LE_SHARED le_result_t pa_mrc_GetNetworkRegState
(
    le_mrc_NetRegState_t* statePtr  ///< [OUT] The network registration state.
);

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
LE_SHARED le_result_t pa_mrc_GetSignalStrength
(
    int32_t*          rssiPtr    ///< [OUT] The received signal strength (in dBm).
);

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
LE_SHARED le_result_t pa_mrc_GetCurrentNetwork
(
    char       *nameStr,               ///< [OUT] the home network Name
    size_t      nameStrSize,           ///< [IN]  the nameStr size
    char       *mccStr,                ///< [OUT] the mobile country code
    size_t      mccStrNumElements,     ///< [IN]  the mccStr size
    char       *mncStr,                ///< [OUT] the mobile network code
    size_t      mncStrNumElements      ///< [IN]  the mncStr size
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to delete the list of Scan Information
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void pa_mrc_DeleteScanInformation
(
    le_dls_List_t *scanInformationListPtr ///< [IN] list of pa_mrc_ScanInformation_t
);

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
LE_SHARED le_result_t pa_mrc_PerformNetworkScan
(
    le_mrc_RatBitMask_t ratMask,               ///< [IN] The network mask
    pa_mrc_ScanType_t   scanType,              ///< [IN] the scan type
    le_dls_List_t      *scanInformationListPtr ///< [OUT] list of pa_mrc_ScanInformation_t
);

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
LE_SHARED le_result_t pa_mrc_GetScanInformationName
(
    pa_mrc_ScanInformation_t *scanInformationPtr,   ///< [IN] The scan information
    char *namePtr, ///< [OUT] Name of operator
    size_t nameSize ///< [IN] The size in bytes of the namePtr buffer
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the current preferred operators list.
 *
 * @return
 *   - A positive value on success (number of Preferred operator found).
 *   - LE_NOT_FOUND if preferred operator list is not available.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED int32_t pa_mrc_GetPreferredOperatorsList
(
    le_dls_List_t*   preferredOperatorListPtr,    ///< [IN/OUT] The preferred operators list.
    bool  plmnStatic,   ///< [IN] Include Static preferred Operators.
    bool  plmnUser      ///< [IN] Include Users preferred Operators.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to add a new mobile country/network code in the list
 *
 * @return
 *  - LE_FAULT         Function failed.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_mrc_AddPreferredOperators
(
    le_dls_List_t      *preferredOperatorsListPtr,   ///< [IN] List of preferred network operator
    const char*         mccPtr,                      ///< [IN] Mobile Country Code
    const char*         mncPtr,                      ///< [IN] Mobile Network Code
    le_mrc_RatBitMask_t ratMask                      ///< [IN] Radio Access Technology mask
);

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
LE_SHARED le_result_t pa_mrc_RemovePreferredOperators
(
    le_dls_List_t      *preferredOperatorsListPtr,   ///< [IN] List of preferred network operator
    const char*         mccPtr,                      ///< [IN] Mobile Country Code
    const char*         mncPtr                       ///< [IN] Mobile Network Code
);


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to clear the preferred list.
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void pa_mrc_DeletePreferredOperatorsList
(
    le_dls_List_t      *preferredOperatorsListPtr ///< [IN] List of preferred network operator
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to apply the preferred list into the modem
 *
 * @return
 *      - LE_OK             on success
 *      - LE_FAULT          for all other errors
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_mrc_SavePreferredOperators
(
    le_dls_List_t      *preferredOperatorsListPtr ///< [IN] List of preferred network operator
);

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
LE_SHARED le_result_t pa_mrc_RegisterNetwork
(
    const char *mccPtr,   ///< [IN] Mobile Country Code
    const char *mncPtr    ///< [IN] Mobile Network Code
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register automatically on network
 *
 * @return
 *      - LE_OK             on success
 *      - LE_FAULT          for all other errors
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_mrc_SetAutomaticNetworkRegistration
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the Radio Access Technology currently in use.
 *
 * @return LE_FAULT The function failed to get the Radio Access Technology.
 * @return LE_OK    The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_mrc_GetRadioAccessTechInUse
(
    le_mrc_Rat_t*   ratPtr    ///< [OUT] The Radio Access Technology.
);

//--------------------------------------------------------------------------------------------------
/**
 * Set the Radio Access Technology Preferences
 *
 * @return
 * - LE_OK              on success
 * - LE_FAULT           on failure
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_mrc_SetRatPreferences
(
    le_mrc_RatBitMask_t ratMask  ///< [IN] A bit mask to set the Radio Access Technology
                                 ///<    preferences.
);

//--------------------------------------------------------------------------------------------------
/**
 * Set the Rat Automatic Radio Access Technology Preference
 *
 * @return
 * - LE_OK              on success
 * - LE_FAULT           on failure
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_mrc_SetAutomaticRatPreference
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the Radio Access Technology Preferences
 *
 * @return
 * - LE_OK              on success
 * - LE_FAULT           on failure
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_mrc_GetRatPreferences
(
    le_mrc_RatBitMask_t* ratMaskPtr  ///< [OUT] A bit mask to get the Radio Access Technology
                                     ///<  preferences.
);

//--------------------------------------------------------------------------------------------------
/**
 * Set the Band Preferences
 *
 * @return
 * - LE_OK              on success
 * - LE_FAULT           on failure
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_mrc_SetBandPreferences
(
    le_mrc_BandBitMask_t bands      ///< [IN] A bit mask to set the Band preferences.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the Band Preferences
 *
 * @return
 * - LE_OK              on success
 * - LE_FAULT           on failure
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_mrc_GetBandPreferences
(
    le_mrc_BandBitMask_t* bandsPtr ///< [OUT] A bit mask to get the Band preferences.
);

//--------------------------------------------------------------------------------------------------
/**
 * Set the LTE Band Preferences
 *
 * @return
 * - LE_OK              on success
 * - LE_FAULT           on failure
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_mrc_SetLteBandPreferences
(
    le_mrc_LteBandBitMask_t bands ///< [IN] A bit mask to set the LTE Band preferences.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the LTE Band Preferences
 *
 * @return
 * - LE_OK              on success
 * - LE_FAULT           on failure
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_mrc_GetLteBandPreferences
(
    le_mrc_LteBandBitMask_t* bandsPtr ///< [OUT] A bit mask to get the LTE Band preferences.
);

//--------------------------------------------------------------------------------------------------
/**
 * Set the TD-SCDMA Band Preferences
 *
 * @return
 * - LE_OK              on success
 * - LE_FAULT           on failure
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_mrc_SetTdScdmaBandPreferences
(
    le_mrc_TdScdmaBandBitMask_t bands ///< [IN] A bit mask to set the TD-SCDMA Band Preferences.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the TD-SCDMA Band Preferences
 *
 * @return
 * - LE_OK              on success
 * - LE_FAULT           on failure
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_mrc_GetTdScdmaBandPreferences
(
    le_mrc_TdScdmaBandBitMask_t* bandsPtr ///< [OUT] A bit mask to get the TD-SCDMA Band
                                          ///<  preferences.
);

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
LE_SHARED int32_t pa_mrc_GetNeighborCellsInfo
(
    le_dls_List_t*   cellInfoListPtr    ///< [IN/OUT] The Neighboring Cells information.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to delete the list of neighboring cells information.
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void pa_mrc_DeleteNeighborCellsInfo
(
    le_dls_List_t *cellInfoListPtr ///< [IN] list of pa_mrc_CellInfo_t
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get current registration mode
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_mrc_GetNetworkRegistrationMode
(
    bool*   isManualPtr,  ///< [OUT] true if the scan mode is manual, false if it is automatic.
    char*   mccPtr,       ///< [OUT] Mobile Country Code
    size_t  mccPtrSize,   ///< [IN] mccPtr buffer size
    char*   mncPtr,       ///< [OUT] Mobile Network Code
    size_t  mncPtrSize    ///< [IN] mncPtr buffer size
);

//--------------------------------------------------------------------------------------------------
/**
 * This function measures the Signal metrics.
 *
 * @return LE_FAULT         The function failed.
 * @return LE_OK            The function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_mrc_MeasureSignalMetrics
(
    pa_mrc_SignalMetrics_t* metricsPtr    ///< [OUT] The signal metrics.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register a handler for Signal Strength change handling.
 *
 * @return A handler reference, which is only needed for later removal of the handler.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_event_HandlerRef_t pa_mrc_AddSignalStrengthIndHandler
(
    pa_mrc_SignalStrengthIndHdlrFunc_t ssIndHandler, ///< [IN] The handler function to handle the
                                                     ///        Signal Strength change indication.
    void*                              contextPtr    ///< [IN] The context to be given to the handler.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to unregister the handler for Signal Strength change handling.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void pa_mrc_RemoveSignalStrengthIndHandler
(
    le_event_HandlerRef_t handlerRef
);

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
LE_SHARED le_result_t pa_mrc_SetSignalStrengthIndThresholds
(
    le_mrc_Rat_t rat,                 ///< Radio Access Technology
    int32_t      lowerRangeThreshold, ///< [IN] lower-range threshold in dBm
    int32_t      upperRangeThreshold  ///< [IN] upper-range strength threshold in dBm
);


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the serving cell Identifier.
 *
 * @return
 *  - LE_FAULT  Function failed.
 *  - LE_OK     Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_mrc_GetServingCellId
(
    uint32_t* cellIdPtr ///< [OUT] main Cell Identifier.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the Location Area Code of the serving cell.
 *
 * @return
 *  - LE_FAULT  Function failed.
 *  - LE_OK     Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_mrc_GetServingCellLocAreaCode
(
    uint32_t* lacPtr ///< [OUT] Location Area Code of the serving cell.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the Band capabilities
 *
 * @return
 * - LE_OK              on success
 * - LE_FAULT           on failure
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_mrc_GetBandCapabilities
(
    le_mrc_BandBitMask_t* bandsPtr ///< [OUT] A bit mask to get the Band capabilities.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the LTE Band capabilities
 *
 * @return
 * - LE_OK              on success
 * - LE_FAULT           on failure
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_mrc_GetLteBandCapabilities
(
    le_mrc_LteBandBitMask_t* bandsPtr ///< [OUT] Bit mask to get the LTE Band capabilities.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the TD-SCDMA Band capabilities
 *
 * @return
 * - LE_OK              on success
 * - LE_FAULT           on failure
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_mrc_GetTdScdmaBandCapabilities
(
    le_mrc_TdScdmaBandBitMask_t* bandsPtr ///< [OUT] Bit mask to get the TD-SCDMA Band capabilities.
);


#endif // LEGATO_PARC_INCLUDE_GUARD
