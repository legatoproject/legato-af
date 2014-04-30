/*
 * ====================== WARNING ======================
 *
 * THE CONTENTS OF THIS FILE HAVE BEEN AUTO-GENERATED.
 * DO NOT MODIFY IN ANY WAY.
 *
 * ====================== WARNING ======================
 */


#ifndef MRC_INTERFACE_H_INCLUDE_GUARD
#define MRC_INTERFACE_H_INCLUDE_GUARD


#include "legato.h"

// User customizable include file
#include "userInclude.h"


//--------------------------------------------------------------------------------------------------
/**
 * Start the service for the client main thread
 */
//--------------------------------------------------------------------------------------------------
void le_mrc_StartClient
(
    const char* serviceInstanceName
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * Stop the service for the current client thread
 */
//--------------------------------------------------------------------------------------------------
void le_mrc_StopClient
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Reference type for le_mrc_NetRegStateHandler handler ADD/REMOVE functions
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_mrc_NetRegStateHandler* le_mrc_NetRegStateHandlerRef_t;


//--------------------------------------------------------------------------------------------------
/**
 * This handler ...
 *
 * @param state
 * @param contextPtr
 */
//--------------------------------------------------------------------------------------------------
typedef void (*le_mrc_NetRegStateHandlerFunc_t)
(
    le_mrc_NetRegState_t state,
    void* contextPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * le_mrc_NetRegStateHandler handler ADD function
 */
//--------------------------------------------------------------------------------------------------
le_mrc_NetRegStateHandlerRef_t le_mrc_AddNetRegStateHandler
(
    le_mrc_NetRegStateHandlerFunc_t handlerPtr,
        ///< [IN]

    void* contextPtr
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * le_mrc_NetRegStateHandler handler REMOVE function
 */
//--------------------------------------------------------------------------------------------------
void le_mrc_RemoveNetRegStateHandler
(
    le_mrc_NetRegStateHandlerRef_t addHandlerRef
        ///< [IN]
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
    le_mrc_NetRegState_t* statePtr
        ///< [OUT]
        ///< Network Registration state.
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
    uint32_t* qualityPtr
        ///< [OUT]
        ///< [OUT] Received signal strength quality (0 = no signal strength,
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
    le_onoff_t power
        ///< [IN]
        ///< The power state.
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
    le_onoff_t* powerPtr
        ///< [OUT]
        ///< Power state.
);

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
    const char* mcc,
        ///< [IN]
        ///< Mobile Country Code

    const char* mnc
        ///< [IN]
        ///< Mobile Network Code
);

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
    le_mrc_Rat_t ratMask
        ///< [IN]
        ///< Technology mask
);

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
    le_mrc_ScanInformation_ListRef_t scanInformationListRef
        ///< [IN]
        ///< The list of scan information.
);

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
    le_mrc_ScanInformation_ListRef_t scanInformationListRef
        ///< [IN]
        ///< The list of scan information.
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
    le_mrc_ScanInformation_ListRef_t scanInformationListRef
        ///< [IN]
        ///< The list of scan information.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the Cellular Network Code [mcc:mnc]
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_POSSIBLE for all other errors
 *
 * @note
 *      On failure, the process exits, so you don't have to worry about checking the returned
 *      reference for validity.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_GetCellularNetworkMccMnc
(
    le_mrc_ScanInformation_Ref_t scanInformationRef,
        ///< [IN]
        ///< [IN] Scan information reference

    char* mccPtr,
        ///< [OUT]
        ///< Mobile Country Code

    size_t mccPtrNumElements,
        ///< [IN]

    char* mncPtr,
        ///< [OUT]
        ///< Mobile Network Code

    size_t mncPtrNumElements
        ///< [IN]
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
    le_mrc_ScanInformation_Ref_t scanInformationRef,
        ///< [IN]
        ///< Scan information reference

    char* namePtr,
        ///< [OUT]
        ///< Name of operator

    size_t namePtrNumElements
        ///< [IN]
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
    le_mrc_ScanInformation_Ref_t scanInformationRef,
        ///< [IN]
        ///< Scan information reference

    le_mrc_Rat_t rat
        ///< [IN]
        ///< The Radio Access Technology
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
    le_mrc_ScanInformation_Ref_t scanInformationRef
        ///< [IN]
        ///< Scan information reference
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
    le_mrc_ScanInformation_Ref_t scanInformationRef
        ///< [IN]
        ///< Scan information reference
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
    le_mrc_ScanInformation_Ref_t scanInformationRef
        ///< [IN]
        ///< Scan information reference
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
    le_mrc_ScanInformation_Ref_t scanInformationRef
        ///< [IN]
        ///< Scan information reference
);

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_GetHomeNetworkName
(
    char* nameStr,
        ///< [OUT]

    size_t nameStrNumElements
        ///< [IN]
);


#endif // MRC_INTERFACE_H_INCLUDE_GUARD

