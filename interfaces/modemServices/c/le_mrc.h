/**
 * @page c_mrc Modem Radio Control API
 *
 * @ref le_mrc.h "Click here for the API reference documentation."
 *
 * <HR>
 *
 * This file contains the the prototype definitions of the high level Modem Radio Control (MRC)
 * APIs.
 *
 * @ref le_mrc_power <br>
 * @ref le_mrc_registration <br>
 * @ref le_mrc_signal <br>
 * @ref le_mrc_network_information <br>
 * @ref le_mrc_networkScan <br>
 * @ref le_mrc_configdb <br>
 *
 * It's important for many M2M apps to know details about cellular network environments (like network registration and signal quality).
 * It allows you to limit some M2M services based on the reliability of the network environment, and
 * provides information to control power consumption (power on or shutdown the radio module).
 *
 * @section le_mrc_power Radio Power Management
 * @c le_mrc_SetRadioPower() API allows the application to power up or shutdown the radio module.
 *
 * @c le_mrc_GetRadioPower() API displays radio module power state.
 *
 * @section le_mrc_registration Network Registration
 * @c le_mrc_GetNetRegState() API retrieves the radio module network registration status.
 *
 * The application can register a handler function to retrieve the registration status each time the
 * registration state changes.
 *
 * @c le_mrc_AddNetRegStateHandler() API installs a registration state handler.
 *
 * @c le_mrc_RemoveNetRegStateHandler() API uninstalls the handler function.
 * @note If only one handler is registered, the le_mrc_RemoveNetRegStateHandler() API
 *       resets the registration mode to its original value before any handler functions were added.
 *
 * @c le_mrc_ConnectCellularNetwork() API connects to one network.
 *
 * @section le_mrc_signal Signal Quality
 * @c le_mrc_GetSignalQual() retrieves the received signal strength details.
 *
 * @section le_mrc_network_information Home Network Information
 * @c le_mrc_GetHomeNetworkName() retrieves the Home Network Name. This value can be empty even if
 * connected to a GSM network.
 * @note Maybe need to provide an API to get 'Mobile country code' and 'Mobile Network Code'.
 *
 * @section le_mrc_networkScan Network Scan
 *
 * Call @c le_mrc_PerformCellularNetworkScan() to fill a list of all network in sight.
 * You can go through all Scan Information by calling @c le_mrc_GetFirstCellularNetworkScan() and
 * @c le_mrc_GetNextCellularNetworkScan().
 *
 * For each Scan Information, you can call:
 *
 *  - @c le_mrc_GetCellularNetworkMccMnc() to have the operator code.
 *  - @c le_mrc_GetCellularNetworkName() to get the operator name.
 *  - @c le_mrc_IsCellularNetworkRatAvailable() to check is this is the radio access technology.
 *  - @c le_mrc_IsCellularNetworkInUse() to check if this is currently in use by the network.
 *  - @c le_mrc_IsCellularNetworkAvailable() to check if this is available.
 *  - @c le_mrc_IsCellularNetworkHome() to check if this is in home status.
 *  - @c le_mrc_IsCellularNetworkForbidden() to check if this is forbidden by the network.
 *
 * @c le_mrc_DeleteCellularNetworkScan() should be called when you do not need the list anymore.
 *
 *
 * Usage example:

@code

    le_mrc_ScanInformation_ListRef_t scanInformationList = NULL;

    scanInformationList = le_mrc_PerformCellularNetworkScan(LE_MRC_RAT_ALL);

    if (!scanInformationList)
    {
        fprintf(stdout, "Could not perform scan\n");
        return;
    }

    le_mrc_ScanInformation_Ref_t cellRef;

    uint32_t i;
    for (i=1;i<LE_MRC_RAT_ALL;i=i<<1)
    {

        for (cellRef=le_mrc_GetFirstCellularNetworkScan(scanInformationList);
             cellRef!=NULL;
             cellRef=le_mrc_GetNextCellularNetworkScan(scanInformationList))
        {
            char mcc[4],mnc[4];
            char name[100];

            if (le_mrc_IsCellularNetworkRatAvailable(cellRef,i)) {

                if (le_mrc_GetCellularNetworkMccMnc(cellRef,mcc,sizeof(mcc),mnc,sizeof(mnc))!=LE_OK)
                {
                    fprintf(stdout, "Failed to get operator code.\n");
                }
                else
                {
                    fprintf(stdout, "[%s-%s] ",mcc,mnc);
                }

                if (le_mrc_GetCellularNetworkName(cellRef, name, sizeof(name)) != LE_OK)
                {
                    fprintf(stdout, "Failed to get operator name.\n");
                }
                else
                {
                    fprintf(stdout, "%-32s",name);
                }

                fprintf(stdout,"%-15s,",le_mrc_IsCellularNetworkInUse(cellRef)?"Is used":"Is not used");

                fprintf(stdout,"%-20s,",le_mrc_IsCellularNetworkAvailable(cellRef)?"Is available":"Is not available");

                fprintf(stdout,"%-10s,",le_mrc_IsCellularNetworkHome(cellRef)?"Home":"Roaming");

                fprintf(stdout,"%-10s]\n",le_mrc_IsCellularNetworkForbidden(cellRef)?"Forbidden":"Allowed");
            }
        }
    }

    le_mrc_DeleteCellularNetworkScan(scanInformationList);

@endcode

 *
 * @section le_mrc_configdb Configuration tree
 *
 * The configuration database path for the MRC is:
 * @verbatim
   /
       modemServices/
           radioControl/
               preferredList/
                   network-0/
                       mcc<int> = <MCC_VALUE>
                       mnc<int> = <MNC_VALUE>
                       rat/
                           rat-0<string> = <RAT_VALUE>
                           ...
                           rat-n<string> = <RAT_VALUE>
                   ...
                   network-n/
                       mcc<int> = <MCC_VALUE>
                       mnc<int> = <MNC_VALUE>
                       rat/
                           rat-0<string> = <RAT_VALUE>
                           ...
                           rat-n<string> = <RAT_VALUE>
               scanMode/
                   manual<bool>
                   mcc<int> = <MCC_VALUE>
                   mnc<int> = <MNC_VALUE>

  @endverbatim
 *
 *  - MCC_VALUE is the Mobile Country Code
 *  - MNC_VALUE is the Mobile Network Code
 *  - RAT_VALUE is the Radio Access technology.Possible values are:
 *      - "GSM"
 *      - "UTMS"
 *      - "LTE"
 *      - "GSM compact"
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */


/** @file le_mrc.h
 *
 * Legato @ref c_mrc include file.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#ifndef LEGATO_MRC_INCLUDE_GUARD
#define LEGATO_MRC_INCLUDE_GUARD

#include "legato.h"
#include "le_mdm_defs.h"



//--------------------------------------------------------------------------------------------------
// APIs.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Declare a reference type for referring to MRC Scan Information objects.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_mrc_ScanInformation* le_mrc_ScanInformation_Ref_t;

//--------------------------------------------------------------------------------------------------
/**
 * Opaque type for Scan Information Listing.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_mrc_ScanInformationList* le_mrc_ScanInformation_ListRef_t;

//--------------------------------------------------------------------------------------------------
/**
 *  Reference type for Network Registration State's Changes Handler references.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_mrc_NetRegStateHandler* le_mrc_NetRegStateHandlerRef_t;

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
    le_mrc_Rat_t ratMask ///< [IN] Technology mask
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
    le_mrc_ScanInformation_ListRef_t  scanInformationListRef ///< [IN] The list of scan information.
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
    le_mrc_ScanInformation_ListRef_t  scanInformationListRef ///< [IN] The list of scan information.
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
    le_mrc_ScanInformation_ListRef_t  scanInformationListRef ///< [IN] The list of scan information.
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
    le_mrc_ScanInformation_Ref_t scanInformationRef,    ///< [IN] Scan information reference
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
    le_mrc_ScanInformation_Ref_t scanInformationRef,    ///< [IN] Scan information reference
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
    le_mrc_ScanInformation_Ref_t scanInformationRef,    ///< [IN] Scan information reference
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
    le_mrc_ScanInformation_Ref_t scanInformationRef    ///< [IN] Scan information reference
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
    le_mrc_ScanInformation_Ref_t scanInformationRef    ///< [IN] Scan information reference
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
    le_mrc_ScanInformation_Ref_t scanInformationRef    ///< [IN] Scan information reference
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
    le_mrc_ScanInformation_Ref_t scanInformationRef    ///< [IN] Scan information reference
);


#endif // LEGATO_MRC_INCLUDE_GUARD
