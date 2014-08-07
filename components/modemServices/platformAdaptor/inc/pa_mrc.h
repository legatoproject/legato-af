/**
 * @page c_pa_mrc Modem Radio Control Platform Adapter API
 *
 * @ref pa_mrc.h "Click here for the API reference documentation."
 *
 * <HR>
 *
 * These APIs are on the top of the platform-dependent adapter layer. They are independent of the
 * implementation. They guarantee the portability on different kind of platform without any changes
 * for the components developed upon these APIs.
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */


/** @file pa_mrc.h
 *
 * Legato @ref c_pa_mrc include file.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
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
    le_dls_Link_t    link;		 ///< Structure is part of a @ref c_doublyLinkedList
    uint32_t         index;      ///< The cell number.
    uint32_t         id;         ///< The cell identifier.
    uint16_t         lac;        ///< The location area code.
    int16_t          rxLevel;    ///< The cell Rx level measurement.
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
    char mcc[LE_MRC_MCC_BYTES];	///< MCC: Mobile Country Code
    char mnc[LE_MRC_MNC_BYTES];	///< MNC: Mobile Network Code
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
    le_mrc_Rat_t        ratMask;    ///< mask of network technology
    le_dls_Link_t       link;       ///< link for the list
}
pa_mrc_PreferredNetwork_t;

//--------------------------------------------------------------------------------------------------
/**
 * Network Scan Information.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    pa_mrc_MobileCode_t mobileCode;     ///< Mobile code
    le_mrc_Rat_t        rat;            ///< radio access technology
    bool                isInUse;        ///< network is in use
    bool                isAvailable;    ///< network can be connected
    bool                isHome;         ///< home status
    bool                isForbidden;    ///< forbidden status
    le_dls_Link_t       link;           ///< link for the list
}
pa_mrc_ScanInformation_t;

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
le_result_t pa_mrc_SetRadioPower
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
le_result_t pa_mrc_GetRadioPower
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
le_event_HandlerRef_t pa_mrc_SetRatChangeHandler
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
void pa_mrc_RemoveRatChangeHandler
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
le_event_HandlerRef_t pa_mrc_AddNetworkRegHandler
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
le_result_t pa_mrc_RemoveNetworkRegHandler
(
    le_event_HandlerRef_t handlerRef
);

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
);

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the Network registration setting.
 *
 * @return LE_NOT_POSSIBLE The function failed to get the Network registration setting.
 * @return LE_TIMEOUT      No response was received from the Modem.
 * @return LE_OK           The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetNetworkRegConfig
(
    pa_mrc_NetworkRegSetting_t*  settingPtr   ///< [OUT] The selected Network registration setting.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the Network registration state.
 *
 * @return LE_NOT_POSSIBLE The function failed to get the Network registration state.
 * @return LE_TIMEOUT      No response was received from the Modem.
 * @return LE_OK           The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetNetworkRegState
(
    le_mrc_NetRegState_t* statePtr  ///< [OUT] The network registration state.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the Signal Quality information.
 *
 * @return LE_OUT_OF_RANGE  The signal quality values are not known or not detectable.
 * @return LE_NOT_POSSIBLE  The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetSignalQuality
(
    int32_t*          rssiPtr    ///< [OUT] The received signal strength (in dBm).
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the Home Network Name information.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the Home Network Name can't fit in nameStr
 *      - LE_NOT_POSSIBLE on any other failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetHomeNetworkName
(
    char       *nameStr,               ///< [OUT] the home network Name
    size_t      nameStrSize            ///< [IN] the nameStr size
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to delete the list of Scan Information
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_mrc_DeleteScanInformation
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
le_result_t pa_mrc_PerformNetworkScan
(
    le_mrc_Rat_t        networkMask,           ///< [IN] The network mask
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
 *      - LE_NOT_POSSIBLE for all other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetScanInformationName
(
    pa_mrc_ScanInformation_t *scanInformationPtr,   ///< [IN] The scan information
    char *namePtr, ///< [OUT] Name of operator
    size_t nameSize ///< [IN] The size in bytes of the namePtr buffer
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to add a new mobile country/network code in the list
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_POSSIBLE for all other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_AddPreferredOperators
(
    le_dls_List_t      *PreferredOperatorsListPtr,   ///< [OUT] List of preferred network operator
    char                mcc[LE_MRC_MCC_BYTES],      ///< [IN] Mobile Country Code
    char                mnc[LE_MRC_MNC_BYTES],      ///< [IN] Mobile Network Code
    le_mrc_Rat_t        ratMask                     ///< [IN] Radio Access Technology mask
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to clear the preferred list.
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_mrc_DeletePreferredOperators
(
    le_dls_List_t      *PreferredOperatorsListPtr ///< [IN] List of preferred network operator
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to apply the preferred list into the modem
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_POSSIBLE for all other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_SavePreferredOperators
(
    le_dls_List_t      *PreferredOperatorsListPtr ///< [IN] List of preferred network operator
);

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
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register automatically on network
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_POSSIBLE for all other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_SetAutomaticNetworkRegistration
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
le_result_t pa_mrc_GetRadioAccessTechInUse
(
    le_mrc_Rat_t*   ratPtr    ///< [OUT] The Radio Access Technology.
);

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
);

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
);

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
);

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
int32_t pa_mrc_GetNeighborCellsInfo
(
    le_dls_List_t*   cellInfoListPtr    ///< [IN/OUT] The Neighboring Cells information.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to delete the list of neighboring cells information.
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_mrc_DeleteNeighborCellsInfo
(
    le_dls_List_t *cellInfoListPtr ///< [IN] list of pa_mrc_CellInfo_t
);

#endif // LEGATO_PARC_INCLUDE_GUARD
