/*
 * ====================== WARNING ======================
 *
 * THE CONTENTS OF THIS FILE HAVE BEEN AUTO-GENERATED.
 * DO NOT MODIFY IN ANY WAY.
 *
 * ====================== WARNING ======================
 */

/**
 * @page c_mrc Modem Radio Control
 *
 * @ref mrc_interface.h "Click here for the API reference documentation."
 *
 * <HR>
 *
 * This file contains the the prototype definitions of the high level Modem Radio Control (MRC)
 * APIs.
 *
 * @ref le_mrc_power <br>
 * @ref le_mrc_rat <br>
 * @ref le_mrc_registration <br>
 * @ref le_mrc_signal <br>
 * @ref le_mrc_network_information <br>
 * @ref le_mrc_networkScan <br>
 * @ref le_mrc_ngbr <br>
 * @ref le_mrc_configdb <br>
 *
 * It's important for many M2M apps to know details about cellular network environments (like
 * network registration and signal quality).
 * It allows you to limit some M2M services based on the reliability of the network environment, and
 * provides information to control power consumption (power on or shutdown the radio module).
 *
 * @section le_mrc_power Radio Power Management
 * @c le_mrc_SetRadioPower() API allows the application to power up or shutdown the radio module.
 *
 * @c le_mrc_GetRadioPower() API displays radio module power state.
 *
 * @section le_mrc_rat Radio Access Technology
 * @c le_mrc_GetRadioAccessTechInUse() API retrieves the current active Radio Access Technology.
 *
 * The application can register a handler function to retrieve the Radio Access Technology each time
 * the RAT changes.
 *
 * @c le_mrc_AddRatChangeHandler() API installs a RAT change handler.
 *
 * @c le_mrc_RemoveRatChangeHandler() API uninstalls the handler function.
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
 * @c le_mrc_RegisterCellularNetwork() API registers on a cellular network.
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

    le_mrc_ScanInformationListRef_t scanInformationList = NULL;

    scanInformationList = le_mrc_PerformCellularNetworkScan(LE_MRC_RAT_ALL);

    if (!scanInformationList)
    {
        fprintf(stdout, "Could not perform scan\n");
        return;
    }

    le_mrc_ScanInformationRef_t cellRef;

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
 *
 * @section le_mrc_ngbr Neighboring Cells Information
 * You must call @c le_mrc_GetNeighborCellsInfo() to retrieve the neighboring cells
 * information. It returns a reference of @c le_mrc_NeighborCellsRef_t type.
 *
 * When the neighboring cells information is no longer needed, you must call
 * @c le_mrc_DeleteNeighborCellsInfo() to free all allocated ressources associated with the
 * object.
 *
 * Then, you can use the following function to get the information:
 * @c le_mrc_GetFirstNeighborCellInfo() and @c le_mrc_GetFirstNeighborCellInfo() allow to go among
 * the single cell information retrieved with @c le_mrc_GetNeighborCellsInfo(). These two functions
 * return a reference of @c le_mrc_CellInfoRef_t type.
 * @c le_mrc_GetNeighborCellId() gets the identifier of the cell specified with the
 * @c le_mrc_CellInfoRef_t parameter.
 * @c le_mrc_GetNeighborCellLac() gets the location area code of the cell specified with the
 * @c le_mrc_CellInfoRef_t parameter.
 * @c le_mrc_GetNeighborCellRxLevel() gets the signal strength (in dBm) of the cell specified
 * @c le_mrc_CellInfoRef_t parameter.
 *
 * @section le_mrc_configdb Radio configuration tree
 * @copydoc le_mrc_configdbPage_Hide
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless, Inc. 2014. Use of this work is subject to license.
 */
/**
 * @interface le_mrc_configdbPage_Hide
 *
 * The configuration database path for the Radio is:
 *
 * @verbatim
    /
        modemServices/
            radio/
                preferredOperators/
                    0/
                        mcc<int> = <MCC_VALUE>
                        mnc<int> = <MNC_VALUE>
                        rat/
                            0<string> = <RAT_VALUE>
                            1<string> = <RAT_VALUE>
                            ...
                    1/
                        mcc<int> = <MCC_VALUE>
                        mnc<int> = <MNC_VALUE>
                        rat/
                            0<string> = <RAT_VALUE>
                            1<string> = <RAT_VALUE>
                            ...
                    ...

                scanMode/
                    manual<bool>
                    mcc<int> = <MCC_VALUE>
                    mnc<int> = <MNC_VALUE>

                preferences/
                    rat/
                        0<string> == <Rat>
                        1<string> == <Rat>
                        2<string> == <Rat>
                        ...
                    band/
                        0<string> == <Band>
                        1<string> == <Band>
                        2<string> == <Band>
                        ...
                    lteBand/
                        0<string> == <LteBand>
                        1<string> = <LteBand>
                        2<string> = <LteBand>
                        ...
                    tdScdmaBand/
                        0<string> = <TdScdmaBand>
                        1<string> = <TdScdmaBand>
                        2<string> = <TdScdmaBand>
                        ...
   @endverbatim
 *
 * The preferred operators can be set using the following choices (string type):
 *  - MCC_VALUE is the Mobile Country Code
 *  - MNC_VALUE is the Mobile Network Code
 *  - RAT_VALUE is the Radio Access technology.Possible values are:
 *      - "GSM"
 *      - "UMTS"
 *      - "LTE"
 *
 * The Radio Access Technology preferences can be set with the following choices (string type):
 * - "CDMA" (CDMA2000-1X + CDMA2000-HRPD)
 * - "GSM"
 * - "UMTS" (UMTS + TD-SCDMA)
 * - "LTE"
 *
 * The 2G/3G Band preferences can be set with the following choices (string type):
 * - "Band-Class-0-A-System"
 * - "Band-Class-0-B-System"
 * - "Band-Class-1-All-Blocks"
 * - "Band-Class-2-Placeholder"
 * - "Band-Class-3-A-System"
 * - "Band-Class-4-All-Blocks"
 * - "Band-Class-5-All-Blocks"
 * - "Band-Class-6"
 * - "Band-Class-7"
 * - "Band-Class-8"
 * - "Band-Class-9"
 * - "Band-Class-10"
 * - "Band-Class-11"
 * - "Band-Class-12"
 * - "Band-Class-13"
 * - "Band-Class-14"
 * - "Band-Class-15"
 * - "Band-Class-16"
 * - "Band-Class-17"
 * - "Band-Class-18"
 * - "Band-Class-19"
 * - "GSM-DCS-1800"
 * - "E-GSM-900" (for Extended GSM 900 band)
 * - "Primary-GSM-900"
 * - "GSM-450"
 * - "GSM-480"
 * - "GSM-750"
 * - "GSM-850"
 * - "GSMR-900" (for GSM Railways GSM 900 band)
 * - "GSM-PCS-1900"
 * - "WCDMA-EU-J-CH-IMT-2100" (for WCDMA Europe, Japan, and China IMT 2100 band)
 * - "WCDMA-US-PCS-1900" (for WCDMA U.S. PCS 1900 band)
 * - "WCDMA-EU-CH-DCS-1800" (for WCDMA Europe and China DCS 1800 band)
 * - "WCDMA-US-1700" (for WCDMA U.S. 1700 band)
 * - "WCDMA-US-850" (for WCDMA U.S. 850 band)
 * - "WCDMA-J-800" (for WCDMA Japan 800 band)
 * - "WCDMA-EU-2600" (for WCDMA Europe 2600 band)
 * - "WCDMA-EU-J-900" (for WCDMA Europe and Japan 900 band)
 * - "WCDMA-J-1700" (for WCDMA Japan 1700 band)
 *
 * The LTE Band preferences can be set by specifying the number of E-UTRA operating band, 0 to 43
 * except: 15, 16, 22, 23, and 26 to 32.
 *
 * The TD-SCDMA Band preferences can be set with the following choices (string type): "A" to "F".
 */
/**
 * @file mrc_interface.h
 *
 * Legato @ref c_mrc include file.
 *
 * Copyright (C) Sierra Wireless, Inc. 2014. Use of this work is subject to license.
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
 * Reference type for le_mrc_RatChangeHandler handler ADD/REMOVE functions
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_mrc_RatChangeHandler* le_mrc_RatChangeHandlerRef_t;


//--------------------------------------------------------------------------------------------------
/**
 * Handler for Network registration state changes.
 *
 *
 * @param state
 *        Parameter ready to receive the Network Registration state.
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
 * Handler for Radio Access Technologie changes.
 *
 *
 * @param rat
 *        Parameter ready to receive the Radio Access Technology.
 * @param contextPtr
 */
//--------------------------------------------------------------------------------------------------
typedef void (*le_mrc_RatChangeHandlerFunc_t)
(
    le_mrc_Rat_t rat,
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
 * le_mrc_RatChangeHandler handler ADD function
 */
//--------------------------------------------------------------------------------------------------
le_mrc_RatChangeHandlerRef_t le_mrc_AddRatChangeHandler
(
    le_mrc_RatChangeHandlerFunc_t handlerPtr,
        ///< [IN]

    void* contextPtr
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * le_mrc_RatChangeHandler handler REMOVE function
 */
//--------------------------------------------------------------------------------------------------
void le_mrc_RemoveRatChangeHandler
(
    le_mrc_RatChangeHandlerRef_t addHandlerRef
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
 * This function must be called to register on a cellular network [mcc;mnc]
 *
 * @return LE_NOT_POSSIBLE  The function failed to register on the network.
 * @return LE_OK            The function succeeded.
 *
 * @note If one code is too long (max 3 digits), it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_RegisterCellularNetwork
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
le_mrc_ScanInformationListRef_t le_mrc_PerformCellularNetworkScan
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
 * @return le_mrc_ScanInformationRef_t  The Scan Information object reference.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_mrc_ScanInformationRef_t le_mrc_GetFirstCellularNetworkScan
(
    le_mrc_ScanInformationListRef_t scanInformationListRef
        ///< [IN]
        ///< The list of scan information.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the next Scan Information object reference in the list of
 * scan Information retrieved with le_mrc_PerformNetworkScan().
 *
 * @return NULL                         No scan information found.
 * @return le_mrc_ScanInformationRef_t  The Scan Information object reference.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_mrc_ScanInformationRef_t le_mrc_GetNextCellularNetworkScan
(
    le_mrc_ScanInformationListRef_t scanInformationListRef
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
    le_mrc_ScanInformationListRef_t scanInformationListRef
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
    le_mrc_ScanInformationRef_t scanInformationRef,
        ///< [IN]
        ///< Scan information reference

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
    le_mrc_ScanInformationRef_t scanInformationRef,
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
    le_mrc_ScanInformationRef_t scanInformationRef,
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
    le_mrc_ScanInformationRef_t scanInformationRef
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
    le_mrc_ScanInformationRef_t scanInformationRef
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
    le_mrc_ScanInformationRef_t scanInformationRef
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
    le_mrc_ScanInformationRef_t scanInformationRef
        ///< [IN]
        ///< Scan information reference
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
    char* nameStr,
        ///< [OUT]
        ///< the home network Name

    size_t nameStrNumElements
        ///< [IN]
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
    le_mrc_Rat_t* ratPtr
        ///< [OUT]
        ///< The Radio Access Technology.
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
    le_mrc_NeighborCellsRef_t ngbrCellsRef
        ///< [IN]
        ///< The Neighboring Cells reference.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the first Cell Information reference in the list of
 * Neighboring Cells information retrieved with le_mrc_GetNeighborCellsInfo().
 *
 * @return NULL                   No Cell information object found.
 * @return le_mrc_CellInfoRef_t   The Cell information object reference.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_mrc_CellInfoRef_t le_mrc_GetFirstNeighborCellInfo
(
    le_mrc_NeighborCellsRef_t ngbrCellsRef
        ///< [IN]
        ///< The Neighboring Cells reference.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the next Cell Information reference in the list of
 * Neighboring Cells information retrieved with le_mrc_GetNeighborCellsInfo().
 *
 * @return NULL                   No Cell information object found.
 * @return le_mrc_CellInfoRef_t   The Cell information object reference.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_mrc_CellInfoRef_t le_mrc_GetNextNeighborCellInfo
(
    le_mrc_NeighborCellsRef_t ngbrCellsRef
        ///< [IN]
        ///< The Neighboring Cells reference.
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
    le_mrc_CellInfoRef_t ngbrCellInfoRef
        ///< [IN]
        ///< The Cell information reference.
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
    le_mrc_CellInfoRef_t ngbrCellInfoRef
        ///< [IN]
        ///< The Cell information reference.
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
    le_mrc_CellInfoRef_t ngbrCellInfoRef
        ///< [IN]
        ///< The Cell information reference.
);


#endif // MRC_INTERFACE_H_INCLUDE_GUARD

