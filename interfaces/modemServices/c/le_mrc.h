/** @file le_mrc.h
 *
 * Legato @ref c_mrc include file.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#ifndef LEGATO_MRC_INCLUDE_GUARD
#define LEGATO_MRC_INCLUDE_GUARD

#include "legato.h"

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------

// Mobile Country Code length
#define LE_MRC_MCC_LEN      3
#define LE_MRC_MCC_BYTES    (LE_MRC_MCC_LEN+1)

// Mobile network Code length
#define LE_MRC_MNC_LEN      3
#define LE_MRC_MNC_BYTES    (LE_MRC_MNC_LEN+1)

//--------------------------------------------------------------------------------------------------
/**
 * Radio Access Technology Bit Mask
 *
 */
//--------------------------------------------------------------------------------------------------
#define LE_MRC_BITMASK_RAT_CDMA         0x01
#define LE_MRC_BITMASK_RAT_GSM          0x02
#define LE_MRC_BITMASK_RAT_UMTS         0x04
#define LE_MRC_BITMASK_RAT_LTE          0x08


//--------------------------------------------------------------------------------------------------
/**
 * Band Bit Mask
 *
 */
//--------------------------------------------------------------------------------------------------
#define LE_MRC_BITMASK_BAND_CLASS_0_A_SYSTEM        0x0000000000000001
#define LE_MRC_BITMASK_BAND_CLASS_0_B_SYSTEM        0x0000000000000002
#define LE_MRC_BITMASK_BAND_CLASS_1_ALL_BLOCKS      0x0000000000000004
#define LE_MRC_BITMASK_BAND_CLASS_2_PLACEHOLDER     0x0000000000000008
#define LE_MRC_BITMASK_BAND_CLASS_3_A_SYSTEM        0x0000000000000010
#define LE_MRC_BITMASK_BAND_CLASS_4_ALL_BLOCKS      0x0000000000000020
#define LE_MRC_BITMASK_BAND_CLASS_5_ALL_BLOCKS      0x0000000000000040
#define LE_MRC_BITMASK_BAND_CLASS_6                 0x0000000000000080
#define LE_MRC_BITMASK_BAND_CLASS_7                 0x0000000000000100
#define LE_MRC_BITMASK_BAND_CLASS_8                 0x0000000000000200
#define LE_MRC_BITMASK_BAND_CLASS_9                 0x0000000000000400
#define LE_MRC_BITMASK_BAND_CLASS_10                0x0000000000000800
#define LE_MRC_BITMASK_BAND_CLASS_11                0x0000000000001000
#define LE_MRC_BITMASK_BAND_CLASS_12                0x0000000000002000
#define LE_MRC_BITMASK_BAND_CLASS_14                0x0000000000004000
#define LE_MRC_BITMASK_BAND_CLASS_15                0x0000000000008000
#define LE_MRC_BITMASK_BAND_CLASS_16                0x0000000000010000
#define LE_MRC_BITMASK_BAND_CLASS_17                0x0000000000020000
#define LE_MRC_BITMASK_BAND_CLASS_18                0x0000000000040000
#define LE_MRC_BITMASK_BAND_CLASS_19                0x0000000000080000
#define LE_MRC_BITMASK_BAND_GSM_DCS_1800            0x0000000000100000
#define LE_MRC_BITMASK_BAND_EGSM_900                0x0000000000200000
#define LE_MRC_BITMASK_BAND_PRI_GSM_900             0x0000000000400000
#define LE_MRC_BITMASK_BAND_GSM_450                 0x0000000000800000
#define LE_MRC_BITMASK_BAND_GSM_480                 0x0000000001000000
#define LE_MRC_BITMASK_BAND_GSM_750                 0x0000000002000000
#define LE_MRC_BITMASK_BAND_GSM_850                 0x0000000004000000
#define LE_MRC_BITMASK_BAND_GSMR_900                0x0000000008000000
#define LE_MRC_BITMASK_BAND_GSM_PCS_1900            0x0000000010000000
#define LE_MRC_BITMASK_BAND_WCDMA_EU_J_CH_IMT_2100  0x0000000020000000
#define LE_MRC_BITMASK_BAND_WCDMA_US_PCS_1900       0x0000000040000000
#define LE_MRC_BITMASK_BAND_WCDMA_EU_CH_DCS_1800    0x0000000080000000
#define LE_MRC_BITMASK_BAND_WCDMA_US_1700           0x0000000100000000
#define LE_MRC_BITMASK_BAND_WCDMA_US_850            0x0000000200000000
#define LE_MRC_BITMASK_BAND_WCDMA_J_800             0x0000000400000000
#define LE_MRC_BITMASK_BAND_WCDMA_EU_2600           0x0000000800000000
#define LE_MRC_BITMASK_BAND_WCDMA_EU_J_900          0x0000001000000000
#define LE_MRC_BITMASK_BAND_WCDMA_J_1700            0x0000002000000000

//--------------------------------------------------------------------------------------------------
/**
 * LTE Band Bit Mask
 *
 */
//--------------------------------------------------------------------------------------------------
#define LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_1    0x0000000000000001
#define LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_2    0x0000000000000002
#define LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_3    0x0000000000000004
#define LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_4    0x0000000000000008
#define LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_5    0x0000000000000010
#define LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_6    0x0000000000000020
#define LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_7    0x0000000000000040
#define LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_8    0x0000000000000080
#define LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_9    0x0000000000000100
#define LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_10   0x0000000000000200
#define LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_11   0x0000000000000400
#define LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_12   0x0000000000000800
#define LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_13   0x0000000000001000
#define LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_14   0x0000000000002000
#define LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_17   0x0000000000004000
#define LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_18   0x0000000000008000
#define LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_19   0x0000000000010000
#define LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_20   0x0000000000020000
#define LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_21   0x0000000000040000
#define LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_24   0x0000000000080000
#define LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_25   0x0000000000100000
#define LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_33   0x0000000000200000
#define LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_34   0x0000000000400000
#define LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_35   0x0000000000800000
#define LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_36   0x0000000001000000
#define LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_37   0x0000000002000000
#define LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_38   0x0000000004000000
#define LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_39   0x0000000008000000
#define LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_40   0x0000000010000000
#define LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_41   0x0000000020000000
#define LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_42   0x0000000040000000
#define LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_43   0x0000000080000000

//--------------------------------------------------------------------------------------------------
/**
 * TDSCDMA Band Bit Mask
 *
 */
//--------------------------------------------------------------------------------------------------
#define LE_MRC_BITMASK_TDSCDMA_BAND_A  0x01
#define LE_MRC_BITMASK_TDSCDMA_BAND_B  0x02
#define LE_MRC_BITMASK_TDSCDMA_BAND_C  0x04
#define LE_MRC_BITMASK_TDSCDMA_BAND_D  0x08
#define LE_MRC_BITMASK_TDSCDMA_BAND_E  0x10
#define LE_MRC_BITMASK_TDSCDMA_BAND_F  0x20


//--------------------------------------------------------------------------------------------------
/**
 * Network Registration states.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_MRC_REG_NONE      = 0, ///< Not registered and not currently searching for new operator.
    LE_MRC_REG_HOME      = 1, ///< Registered, home network.
    LE_MRC_REG_SEARCHING = 2, ///< Not registered but currently searching for a new operator.
    LE_MRC_REG_DENIED    = 3, ///< Registration was denied, usually because of invalid access credentials.
    LE_MRC_REG_ROAMING   = 5, ///< Registered to a roaming network.
    LE_MRC_REG_UNKNOWN   = 4, ///< Unknown state.
}
le_mrc_NetRegState_t;

//--------------------------------------------------------------------------------------------------
/**
 * Radio Access Technology
 *
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_MRC_RAT_UNKNOWN  = 0,     ///< Unknown
    LE_MRC_RAT_GSM      = 1<<0,  ///< GSM network
    LE_MRC_RAT_UMTS     = 1<<1,  ///< UMTS network
    LE_MRC_RAT_LTE      = 1<<2,  ///< LTE network
    LE_MRC_RAT_CDMA     = 1<<3,  ///< CDMA  network
    LE_MRC_RAT_ALL      = LE_MRC_RAT_GSM  |
                          LE_MRC_RAT_UMTS |
                          LE_MRC_RAT_LTE  |
                          LE_MRC_RAT_CDMA   ///< All technology
}
le_mrc_Rat_t;

//--------------------------------------------------------------------------------------------------
// APIs.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Declare a reference type for referring to MRC Scan Information objects.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_mrc_ScanInformation* le_mrc_ScanInformationRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * Reference type for Scan Information Listing.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_mrc_ScanInformationList* le_mrc_ScanInformationListRef_t;

//--------------------------------------------------------------------------------------------------
/**
 *  Reference type for one Cell Information.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_mrc_CellInfo* le_mrc_CellInfoRef_t;

//--------------------------------------------------------------------------------------------------
/**
 *  Reference type for all Neighboring Cells Information.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_mrc_NeighborCells* le_mrc_NeighborCellsRef_t;

//--------------------------------------------------------------------------------------------------
/**
 *  Reference type for Network Registration State's Changes Handler references.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_mrc_NetRegStateHandler* le_mrc_NetRegStateHandlerRef_t;

//--------------------------------------------------------------------------------------------------
/**
 *  Reference type for Radio Access Technology changes Handler references.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_mrc_RatChangeHandler* le_mrc_RatChangeHandlerRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * Prototype for handler functions used to report that the network registration state has changed.
 *
 * @param state       Parameter ready to receive the Network Registration state.
 * @param contextPtr  Context information the event handler may require.
 */
//--------------------------------------------------------------------------------------------------
typedef void(*le_mrc_NetRegStateHandlerFunc_t)
(
    le_mrc_NetRegState_t  state,
    void*                 contextPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Prototype for handler functions used to report that the Radio Access Technology has changed.
 *
 * @param rat         Parameter ready to receive the Radio Access Technology.
 * @param contextPtr  Context information the event handler may require.
 */
//--------------------------------------------------------------------------------------------------
typedef void(*le_mrc_RatChangeHandlerFunc_t)
(
    le_mrc_Rat_t  rat,
    void*         contextPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Register an handler for network registration state change.
 *
 * @return Handler reference: only needed to remove the handler.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_mrc_NetRegStateHandlerRef_t le_mrc_AddNetRegStateHandler
(
    le_mrc_NetRegStateHandlerFunc_t handlerFuncPtr, ///< [IN] The handler function.
    void*                           contextPtr      ///< [IN] The handler's context.
);

//--------------------------------------------------------------------------------------------------
/**
 * Remove a handler for network registration state changes.
 */
//--------------------------------------------------------------------------------------------------
void le_mrc_RemoveNetRegStateHandler
(
    le_mrc_NetRegStateHandlerRef_t    handlerRef ///< [IN] The handler reference.
);

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
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to remove an handler for Radio Access Technology changes.
 */
//--------------------------------------------------------------------------------------------------
void le_mrc_RemoveRatChangeHandler
(
    le_mrc_RatChangeHandlerRef_t    handlerRef ///< [IN] The handler reference.
);

//--------------------------------------------------------------------------------------------------
/**
 * Set the power of the Radio Module.
 *
 * @return LE_FAULT  Function failed.
 * @return LE_OK     Function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_SetRadioPower
(
    le_onoff_t    power   ///< [IN] The power state.
);

//--------------------------------------------------------------------------------------------------
/**
 * Must be called to get the Radio Module power state.
 *
 * @return LE_NOT_POSSIBLE  Function failed to get the Radio Module power state.
 * @return LE_OK            Function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_GetRadioPower
(
    le_onoff_t*    powerPtr   ///< [OUT] Power state.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the network registration state.
 *
 * @return LE_NOT_POSSIBLE  Function failed to get the Network registration state.
 * @return LE_OK            Function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_GetNetRegState
(
    le_mrc_NetRegState_t*   statePtr  ///< [OUT] Network Registration state.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the signal quality.
 *
 * @return LE_NOT_POSSIBLE  Function failed to obtain the signal quality.
 * @return LE_OK            Function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_GetSignalQual
(
    uint32_t*   qualityPtr  ///< [OUT] Received signal strength quality (0 = no signal strength,
                            ///        5 = very good signal strength).
);

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
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register on a cellular network [mcc;mnc]
 *
 * @return LE_NOT_POSSIBLE  The function failed to register on the network.
 * @return LE_OK            The function succeeded.
 *
 * @note If one code is too long (max 3 digits), it is a fatal error, the
 *       function will not return.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_RegisterCellularNetwork
(
    const char *mccPtr,   ///< [IN] Mobile Country Code
    const char *mncPtr    ///< [IN] Mobile Network Code
);

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
    le_mrc_Rat_t ratMask ///< [IN] Technology mask
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the first Scan Information object reference in the list of
 * scan Information retrieved with le_mrc_PerformNetworkScan().
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
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the next Scan Information object reference in the list of
 * scan Information retrieved with le_mrc_PerformNetworkScan().
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
);

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
    le_mrc_ScanInformationListRef_t  scanInformationListRef ///< [IN] The list of scan information.
);

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
    le_mrc_ScanInformationRef_t scanInformationRef,    ///< [IN] Scan information reference
    char                        *mccPtr,                ///< [OUT] Mobile Country Code
    size_t                       mccPtrSize,            ///< [IN] mccPtr buffer size
    char                        *mncPtr,                ///< [OUT] Mobile Network Code
    size_t                       mncPtrSize             ///< [IN] mncPtr buffer size
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the Cellular Network Name.
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
    le_mrc_ScanInformationRef_t scanInformationRef,    ///< [IN] Scan information reference
    char *namePtr, ///< [OUT] Name of operator
    size_t nameSize ///< [IN] The size in bytes of the namePtr buffer
);

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
    le_mrc_ScanInformationRef_t scanInformationRef,    ///< [IN] Scan information reference
    le_mrc_Rat_t                 rat                    ///< [IN] The Radio Access Technology
);

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
);

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
);

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
);

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
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the current Radio Access Technology in use.
 *
 * @return LE_NOT_POSSIBLE  Function failed to get the Radio Access Technology.
 * @return LE_OK            Function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_GetRadioAccessTechInUse
(
    le_mrc_Rat_t*   ratPtr  ///< [OUT] The Radio Access Technology.
);

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
);

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
);

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
);

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
);

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
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the Location Area Code of a cell.
 *
 * @return The Location Area Code of a cell.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
uint32_t le_mrc_GetNeighborCellLocAreaCode
(
    le_mrc_CellInfoRef_t     ngbrCellInfoRef ///< [IN] The Cell information reference
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the signal strength of a cell.
 *
 * @return The signal strength of a cell.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
int32_t le_mrc_GetNeighborCellRxLevel
(
    le_mrc_CellInfoRef_t     ngbrCellInfoRef ///< [IN] The Cell information reference
);


#endif // LEGATO_MRC_INCLUDE_GUARD
