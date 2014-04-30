/**
 * @page c_pa_mrc Modem Radio Control Platform Adapter API
 *
 * @ref pa_mrc.h "Click here for the API reference documentation."
 *
 * <HR>
 *
 * @section pa_mrc_toc Table of Contents
 *
 *  - @ref pa_mrc_intro
 *  - @ref pa_mrc_rational
 *
 *
 * @section pa_mrc_intro Introduction
 * These APIs are on the top of the platform-dependent adapter layer. They are independent of the
 * implementation. They guarantee the portability on different kind of platform without any changes
 * for the components developped upon these APIs.
 *
 *
 * @section pa_mrc_rational Rational
 * These functions are all blocking functions, so that they return when the modem has answered or
 * when a timeout has occured due to an interrupted communication with the modem.
 *
 * They all verify the validity and the range of the input parameters before performing the modem
 * operation.
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
#include "le_mdm_defs.h"

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
/**
 * Network Technology mask for Network Scan.
 *
 */
//--------------------------------------------------------------------------------------------------
#define PA_MRC_METWORK_MASK_GSM       1<<0
#define PA_MRC_METWORK_MASK_UTMS      1<<1
#define PA_MRC_METWORK_MASK_LTE       1<<2
#define PA_MRC_METWORK_MASK_TD_SCDMA  1<<3

//--------------------------------------------------------------------------------------------------
/**
 * Radio Access Technology mask.
 *
 */
//--------------------------------------------------------------------------------------------------
#define PA_MRC_METWORK_RATMASK_GSM          1<<0
#define PA_MRC_METWORK_RATMASK_UTMS         1<<1
#define PA_MRC_METWORK_RATMASK_LTE          1<<2
#define PA_MRC_METWORK_RATMASK_GSMCOMPACT   1<<3

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
 * Network Scan Type.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    PA_MRC_SCAN_PLMN = 0,       ///< Scan PLMN
    PA_MRC_SCAN_CSG,            ///< Scan closed subscriber group
}
le_mrc_ScanType_t;

//--------------------------------------------------------------------------------------------------
/**
 * Mobile code.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char mcc[LE_MRC_MCC_BYTES];
    char mnc[LE_MRC_MNC_BYTES];
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
    pa_mrc_MobileCode_t mobileCode; ///< Mobilde code
    uint32_t            ratMask;    ///< mask of network technology
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
    uint32_t            rat;            ///< radio access technology
    uint32_t            networkStatus;  ///< network status to translate
    le_dls_Link_t       link;           ///< link for the list
}
pa_mrc_ScanInformation_t;

//--------------------------------------------------------------------------------------------------
/**
 * Prototype for handler functions used to report the Network registration state.
 *
 * @param regStat The Network registration state.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*pa_mrc_NetworkRegHdlrFunc_t)(le_mrc_NetRegState_t* regState);


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
    uint32_t            networkMask,           ///< [IN] The network mask
    le_mrc_ScanType_t   scanType,              ///< [IN] the scan type
    le_dls_List_t      *scanInformationListPtr ///< [OUT] list of pa_mrc_ScanInformation_t
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the scanInformation code
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_POSSIBLE for all other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetScanInformationCode
(
    pa_mrc_ScanInformation_t *scanInformationPtr,   ///< [IN] The scan information
    pa_mrc_MobileCode_t      *mobileCodePtr         ///< [OUT] the mobile code
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
 * This function must be called to know the radio access technology
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_POSSIBLE for all other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetScanInformationRat
(
    pa_mrc_ScanInformation_t *scanInformationPtr,   ///< [IN] The scan information
    le_mrc_Rat_t             *ratPtr ///< [OUT] The radio access technology
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to know if pa_mrc_ScanInformation_t is in use
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_POSSIBLE for all other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetScanInformationInUse
(
    pa_mrc_ScanInformation_t *scanInformationPtr,   ///< [IN] The scan information
    bool *inUsePtr              ///< [OUT] In Use status
);


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to know if pa_mrc_ScanInformation_t is available
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_POSSIBLE for all other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetScanInformationAvailable
(
    pa_mrc_ScanInformation_t *scanInformationPtr,   ///< [IN] The scan information
    bool                     *isAvailablePtr        ///< [OUT] available status
);


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to know the home status
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_POSSIBLE for all other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetScanInformationHome
(
    pa_mrc_ScanInformation_t *scanInformationPtr,   ///< [IN] The scan information
    bool *isHomePtr ///< [OUT] the network home status
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to know the forbidden status
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_POSSIBLE for all other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetScanInformationForbidden
(
    pa_mrc_ScanInformation_t *scanInformationPtr,   ///< [IN] The scan information
    bool *isForbiddenPtr ///< [OUT] the forbidden status
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
le_result_t pa_mrc_AddPreferredNetwork
(
    le_dls_List_t      *PreferredNetworkListPtr,    ///< [OUT] List of preferred network
    char                mcc[LE_MRC_MCC_BYTES],      ///< [IN] Mobile Country Code
    char                mnc[LE_MRC_MNC_BYTES],      ///< [IN] Mobile Network Code
    uint32_t            ratMask                     ///< [IN] Radio Access Technology mask
);


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to clear the preferred list.
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_POSSIBLE for all other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_ClearPreferedList
(
    le_dls_List_t      *PreferredNetworkListPtr ///< [IN] List of preferred Network
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
le_result_t pa_mrc_SavePreferredList
(
    le_dls_List_t      *PreferredNetworkListPtr ///< [IN] List of preferred network
);


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to connect to a mobile network [mcc;mnc]
 *
 * @return LE_NOT_POSSIBLE  The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_ConnectNetwork
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


#endif // LEGATO_PARC_INCLUDE_GUARD
