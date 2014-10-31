/** @file le_mrc.c
 *
 * This file contains the data structures and the source code of the MRC (Modem Radio Control) APIs.
 *
 *
 * The implementation of @c le_mrc_PerformCellularNetworkScan() and @c le_mrc_GetNeighborCellsInfo()
 * functions requires the use of lists of safe reference mappings.
 * For instance, @c le_mrc_GetNeighborCellsInfo() returns a safe reference for a @c CellList_t
 * object, this object contains a list of cells information and a list of @c CellSafeRef_t objects.
 * One node of @c CellSafeRef_t is a safe reference for an object that gathers the information of
 * one cell. This allows to have several safe references that point to the the same cell information
 * object. The @c le_mrc_GetFirstNeighborCellInfo() and @c le_mrc_GetNextNeighborCellInfo() return
 * a node of a @c CellSafeRef_t object.
 *
 * We need the extra  @c CellSafeRef_t objects so that we can free up all those safe references
 * when the @c CellList_t object is released without having to multi-pass search the @c CellRefMap.
 *
 * This rationale is also used to implement the @c le_mrc_PerformCellularNetworkScan(),
 * @c le_mrc_GetFirstCellularNetworkScan() and @c le_mrc_GetNextCellularNetworkScan() functions.
 *
 *
 * Copyright (C) Sierra Wireless, Inc. 2014. Use of this work is subject to license.
 */


#include "legato.h"
#include "interfaces.h"
#include "pa_mrc.h"
#include "mdmCfgEntries.h"


//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of neighboring cells information we expect to have at one time.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_NUM_NEIGHBORS        6

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of neighboring cells information lists we expect to have at one time.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_NUM_NEIGHBOR_LISTS    5

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of Scan Information List objects we expect to have at one time.
 */
//--------------------------------------------------------------------------------------------------
#define MRC_MAX_SCANLIST    5

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of Scan Information objects we expect to have at one time.
 */
//--------------------------------------------------------------------------------------------------
#define MRC_MAX_SCAN    10


//--------------------------------------------------------------------------------------------------
// Data structures.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Neighboring Cells Information safe Reference list structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    void*         safeRef;
    le_dls_Link_t link;
} CellSafeRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * Neighboring Cells Information list structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    int32_t        cellsCount;          // number of detected cells
    le_dls_List_t  paNgbrCellInfoList;  // list of pa_mrc_CellInfo_t
    le_dls_List_t  safeRefCellInfoList; // list of CellSafeRef_t
    le_dls_Link_t *currentLinkPtr;      // link for current CellSafeRef_t reference
} CellList_t;

//--------------------------------------------------------------------------------------------------
/**
 * List Scan Information structure safe Reference.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    void*         safeRef;
    le_dls_Link_t link;
} ScanInfoSafeRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * List Scan Information structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_dls_List_t  paScanInfoList;      // list of pa_mrc_ScanInformation_t
    le_dls_List_t  safeRefScanInfoList; // list of ScanInfoSafeRef_t
    le_dls_Link_t *currentLink;         // link for iterator
} ScanInfoList_t;


//--------------------------------------------------------------------------------------------------
// Static declarations.
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
/**
 * Pool for neighboring cells information list.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t CellListPool;

//--------------------------------------------------------------------------------------------------
/**
 * Pool for cell information safe reference.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t  CellInfoSafeRefPool;

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for all neighboring cells information list objects
 *
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t CellListRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for one neighboring cell information objects
 *
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t CellRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Event ID for New Network Registration State notification.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t NewNetRegStateId;

//--------------------------------------------------------------------------------------------------
/**
 * Memory Pool for Listed ScanInformation.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t  ScanInformationListPool;

//--------------------------------------------------------------------------------------------------
/**
 * Memory Pool for Listed Information structure safe reference.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t  ScanInformationSafeRefPool;

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for Scan Information List.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t ScanInformationListRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for one Scan Information.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t ScanInformationRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Event ID for New RAT change notification.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t RatChangeId;


//--------------------------------------------------------------------------------------------------
/**
 * Function to destroy all safeRef elements in the CellInfoSafeRef list.
 *
 */
//--------------------------------------------------------------------------------------------------
static void DeleteCellInfoSafeRefList
(
    le_dls_List_t* listPtr
)
{
    CellSafeRef_t* nodePtr;
    le_dls_Link_t *linkPtr;

    while ((linkPtr=le_dls_Pop(listPtr)) != NULL)
    {
        nodePtr = CONTAINER_OF(linkPtr, CellSafeRef_t, link);
        le_ref_DeleteRef(CellRefMap, nodePtr->safeRef);
        le_mem_Release(nodePtr);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the TDSCDMA Band bit mask from the config DB entry.
 *
 * @return The TDSCDMA Band bit mask.
 */
//--------------------------------------------------------------------------------------------------
static le_mrc_RatBitMask_t GetRatBitMask
(
    char* ratPtr
)
{
    if (!strcmp(ratPtr, "CDMA"))
    {
        return LE_MRC_BITMASK_RAT_CDMA;
    }
    else if (!strcmp(ratPtr, "GSM"))
    {
        return LE_MRC_BITMASK_RAT_GSM;
    }
    else if (!strcmp(ratPtr, "UMTS"))
    {
        return LE_MRC_BITMASK_RAT_UMTS;
    }
    else if (!strcmp(ratPtr, "LTE"))
    {
        return LE_MRC_BITMASK_RAT_LTE;
    }
    else
    {
        LE_WARN("Invalid Radio Access Technology choice!");
        return 0x00;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the Band bit mask from the config DB entry.
 *
 * @return The Band bit mask.
 */
//--------------------------------------------------------------------------------------------------
static uint64_t GetBandBitMask
(
    char* bandPtr
)
{
    if (!strcmp(bandPtr, "Band-Class-0-A-System"))
    {
        return LE_MRC_BITMASK_BAND_CLASS_0_A_SYSTEM;
    }
    else if (!strcmp(bandPtr, "Band-Class-0-B-System"))
    {
        return LE_MRC_BITMASK_BAND_CLASS_0_B_SYSTEM;
    }
    else if (!strcmp(bandPtr, "Band-Class-1-All-Blocks"))
    {
        return LE_MRC_BITMASK_BAND_CLASS_1_ALL_BLOCKS;
    }
    else if (!strcmp(bandPtr, "Band-Class-2-Placeholder"))
    {
        return LE_MRC_BITMASK_BAND_CLASS_2_PLACEHOLDER;
    }
    else if (!strcmp(bandPtr, "Band-Class-3-A-System"))
    {
        return LE_MRC_BITMASK_BAND_CLASS_3_A_SYSTEM;
    }
    else if (!strcmp(bandPtr, "Band-Class-4-All-Blocks"))
    {
        return LE_MRC_BITMASK_BAND_CLASS_4_ALL_BLOCKS;
    }
    else if (!strcmp(bandPtr, "Band-Class-5-All-Blocks"))
    {
        return LE_MRC_BITMASK_BAND_CLASS_5_ALL_BLOCKS;
    }
    else if (!strcmp(bandPtr, "Band-Class-6"))
    {
        return LE_MRC_BITMASK_BAND_CLASS_6;
    }
    else if (!strcmp(bandPtr, "Band-Class-7"))
    {
        return LE_MRC_BITMASK_BAND_CLASS_7;
    }
    else if (!strcmp(bandPtr, "Band-Class-8"))
    {
        return LE_MRC_BITMASK_BAND_CLASS_8;
    }
    else if (!strcmp(bandPtr, "Band-Class-9"))
    {
        return LE_MRC_BITMASK_BAND_CLASS_9;
    }
    else if (!strcmp(bandPtr, "Band-Class-10"))
    {
        return LE_MRC_BITMASK_BAND_CLASS_10;
    }
    else if (!strcmp(bandPtr, "Band-Class-11"))
    {
        return LE_MRC_BITMASK_BAND_CLASS_11;
    }
    else if (!strcmp(bandPtr, "Band-Class-12"))
    {
        return LE_MRC_BITMASK_BAND_CLASS_12;
    }
    else if (!strcmp(bandPtr, "Band-Class-14"))
    {
        return LE_MRC_BITMASK_BAND_CLASS_14;
    }
    else if (!strcmp(bandPtr, "Band-Class-15"))
    {
        return LE_MRC_BITMASK_BAND_CLASS_15;
    }
    else if (!strcmp(bandPtr, "Band-Class-16"))
    {
        return LE_MRC_BITMASK_BAND_CLASS_16;
    }
    else if (!strcmp(bandPtr, "Band-Class-17"))
    {
        return LE_MRC_BITMASK_BAND_CLASS_17;
    }
    else if (!strcmp(bandPtr, "Band-Class-18"))
    {
        return LE_MRC_BITMASK_BAND_CLASS_18;
    }
    else if (!strcmp(bandPtr, "Band-Class-19"))
    {
        return LE_MRC_BITMASK_BAND_CLASS_19;
    }
    else if (!strcmp(bandPtr, "GSM-DCS-1800"))
    {
        return LE_MRC_BITMASK_BAND_GSM_DCS_1800;
    }
    else if (!strcmp(bandPtr, "E-GSM-900"))
    {
        return LE_MRC_BITMASK_BAND_EGSM_900;
    }
    else if (!strcmp(bandPtr, "Primary-GSM-900"))
    {
        return LE_MRC_BITMASK_BAND_PRI_GSM_900;
    }
    else if (!strcmp(bandPtr, "GSM-450"))
    {
        return LE_MRC_BITMASK_BAND_GSM_450;
    }
    else if (!strcmp(bandPtr, "GSM-480"))
    {
        return LE_MRC_BITMASK_BAND_GSM_480;
    }
    else if (!strcmp(bandPtr, "GSM-750"))
    {
        return LE_MRC_BITMASK_BAND_GSM_750;
    }
    else if (!strcmp(bandPtr, "GSM-850"))
    {
        return LE_MRC_BITMASK_BAND_GSM_850;
    }
    else if (!strcmp(bandPtr, "GSMR-900"))
    {
        return LE_MRC_BITMASK_BAND_GSMR_900;
    }
    else if (!strcmp(bandPtr, "GSM-PCS-1900"))
    {
        return LE_MRC_BITMASK_BAND_GSM_PCS_1900;
    }
    else if (!strcmp(bandPtr, "WCDMA-EU-J-CH-IMT-2100"))
    {
        return LE_MRC_BITMASK_BAND_WCDMA_EU_J_CH_IMT_2100;
    }
    else if (!strcmp(bandPtr, "WCDMA-US-PCS-1900"))
    {
        return LE_MRC_BITMASK_BAND_WCDMA_US_PCS_1900;
    }
    else if (!strcmp(bandPtr, "WCDMA-EU-CH-DCS-1800"))
    {
        // return LE_MRC_BITMASK_BAND_WCDMA_EU_CH_DCS_1800;
        return 0x0000000080000000;
    }
    else if (!strcmp(bandPtr, "WCDMA-US-1700"))
    {
        return LE_MRC_BITMASK_BAND_WCDMA_US_1700;
    }
    else if (!strcmp(bandPtr, "WCDMA-US-850"))
    {
        return LE_MRC_BITMASK_BAND_WCDMA_US_850;
    }
    else if (!strcmp(bandPtr, "WCDMA-J-800"))
    {
        return LE_MRC_BITMASK_BAND_WCDMA_J_800;
    }
    else if (!strcmp(bandPtr, "WCDMA-EU-2600"))
    {
        return LE_MRC_BITMASK_BAND_WCDMA_EU_2600;
    }
    else if (!strcmp(bandPtr, "WCDMA-EU-J-900"))
    {
        return LE_MRC_BITMASK_BAND_WCDMA_EU_J_900;
    }
    else if (!strcmp(bandPtr, "WCDMA-J-1700"))
    {
        return LE_MRC_BITMASK_BAND_WCDMA_J_1700;
    }
    else
    {
        LE_WARN("Invalid TDSCDMA Band choice!");
        return 0ULL;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the LTE Band bit mask from the config DB entry.
 *
 * @return The LTE Band bit mask.
 */
//--------------------------------------------------------------------------------------------------
static uint64_t GetLteBandBitMask
(
    char* bandPtr
)
{
    if (!strcmp(bandPtr, "1"))
    {
        return LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_1;
    }
    else if (!strcmp(bandPtr, "2"))
    {
        return LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_2;
    }
    else if (!strcmp(bandPtr, "3"))
    {
        return LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_3;
    }
    else if (!strcmp(bandPtr, "4"))
    {
        return LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_4;
    }
    else if (!strcmp(bandPtr, "5"))
    {
        return LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_5;
    }
    else if (!strcmp(bandPtr, "6"))
    {
        return LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_6;
    }
    else if (!strcmp(bandPtr, "7"))
    {
        return LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_7;
    }
    else if (!strcmp(bandPtr, "8"))
    {
        return LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_8;
    }
    else if (!strcmp(bandPtr, "9"))
    {
        return LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_9;
    }
    else if (!strcmp(bandPtr, "10"))
    {
        return LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_10;
    }
    else if (!strcmp(bandPtr, "11"))
    {
        return LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_11;
    }
    else if (!strcmp(bandPtr, "12"))
    {
        return LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_12;
    }
    else if (!strcmp(bandPtr, "13"))
    {
        return LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_13;
    }
    else if (!strcmp(bandPtr, "14"))
    {
        return LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_14;
    }
    else if (!strcmp(bandPtr, "17"))
    {
        return LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_17;
    }
    else if (!strcmp(bandPtr, "18"))
    {
        return LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_18;
    }
    else if (!strcmp(bandPtr, "19"))
    {
        return LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_19;
    }
    else if (!strcmp(bandPtr, "20"))
    {
        return LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_20;
    }
    else if (!strcmp(bandPtr, "21"))
    {
        return LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_21;
    }
    else if (!strcmp(bandPtr, "24"))
    {
        return LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_24;
    }
    else if (!strcmp(bandPtr, "25"))
    {
        return LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_25;
    }
    else if (!strcmp(bandPtr, "33"))
    {
        return LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_33;
    }
    else if (!strcmp(bandPtr, "34"))
    {
        return LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_34;
    }
    else if (!strcmp(bandPtr, "35"))
    {
        return LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_35;
    }
    else if (!strcmp(bandPtr, "36"))
    {
        return LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_36;
    }
    else if (!strcmp(bandPtr, "37"))
    {
        return LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_37;
    }
    else if (!strcmp(bandPtr, "38"))
    {
        return LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_38;
    }
    else if (!strcmp(bandPtr, "39"))
    {
        return LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_39;
    }
    else if (!strcmp(bandPtr, "40"))
    {
        return LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_40;
    }
    else if (!strcmp(bandPtr, "41"))
    {
        return LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_41;
    }
    else if (!strcmp(bandPtr, "42"))
    {
        return LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_42;
    }
    else if (!strcmp(bandPtr, "43"))
    {
        // return LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_43;
        return 0x0000000080000000;
    }
    else
    {
        LE_WARN("Invalid LTE Band choice!");
        return 0ULL;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the TDSCDMA Band bit mask from the config DB entry.
 *
 * @return The TDSCDMA Band bit mask.
 */
//--------------------------------------------------------------------------------------------------
static uint8_t GetTdScdmaBandBitMask
(
    char* tdScdmaBandPtr
)
{
    if (!strcmp(tdScdmaBandPtr, "A"))
    {
        return LE_MRC_BITMASK_TDSCDMA_BAND_A;
    }
    else if (!strcmp(tdScdmaBandPtr, "B"))
    {
        return LE_MRC_BITMASK_TDSCDMA_BAND_B;
    }
    else if (!strcmp(tdScdmaBandPtr, "C"))
    {
        return LE_MRC_BITMASK_TDSCDMA_BAND_C;
    }
    else if (!strcmp(tdScdmaBandPtr, "D"))
    {
        return LE_MRC_BITMASK_TDSCDMA_BAND_D;
    }
    else if (!strcmp(tdScdmaBandPtr, "E"))
    {
        return LE_MRC_BITMASK_TDSCDMA_BAND_E;
    }
    else if (!strcmp(tdScdmaBandPtr, "F"))
    {
        return LE_MRC_BITMASK_TDSCDMA_BAND_F;
    }
    else
    {
        LE_WARN("Invalid TDSCDMA Band choice!");
        return 0x00;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Load the preferences from the configuration tree
 */
//--------------------------------------------------------------------------------------------------
static void LoadPreferencesFromConfigDb
(
    void
)
{
    le_cfg_IteratorRef_t mrcCfg = NULL;
    char cfgNodeLoc[8] = {0};
    char preference[64] = {0};
    uint8_t i = 0;

    // Set the preferred Radio Access Technology
    mrcCfg = le_cfg_CreateReadTxn(CFG_MODEMSERVICE_MRC_RAT_PATH);
    le_mrc_RatBitMask_t ratMask = 0x00;
    i = 0;
    sprintf (cfgNodeLoc, "%d", i);
    while (!le_cfg_IsEmpty(mrcCfg, cfgNodeLoc))
    {
        if ( le_cfg_GetString(mrcCfg, cfgNodeLoc, preference, sizeof(preference), "") != LE_OK )
        {
            LE_WARN("Node value string for '%s' too large.", preference);
            break;
        }
        if ( strncmp(preference, "", sizeof(preference)) == 0 )
        {
            LE_WARN("No node value set for '%s'", preference);
            break;
        }

        ratMask |= GetRatBitMask(preference);

        LE_DEBUG("New RAT <%s> set", preference);
        i++;
        sprintf (cfgNodeLoc, "%d", i);
    }
    LE_DEBUG("Set RAT bit mask: 0x%01X", ratMask);
    if (ratMask)
    {
        if ( pa_mrc_SetRatPreference(ratMask) != LE_OK )
        {
            LE_WARN("Unable to set the Radio Access Technology preference in the configDb.");
        }
    }
    le_cfg_CancelTxn(mrcCfg);
    memset(preference, '\0', sizeof(preference));

    // Set the preferred Bands
    mrcCfg = le_cfg_CreateReadTxn(CFG_MODEMSERVICE_MRC_BAND_PATH);
    uint64_t bandMask = 0ULL;
    i=0;
    sprintf (cfgNodeLoc, "%d", i);
    while (!le_cfg_IsEmpty(mrcCfg, cfgNodeLoc))
    {
        if ( le_cfg_GetString(mrcCfg, cfgNodeLoc, preference, sizeof(preference), "") != LE_OK )
        {
            LE_WARN("Node value string for '%s' too large.", preference);
            break;
        }
        if ( strncmp(preference, "", sizeof(preference)) == 0 )
        {
            LE_WARN("No node value set for '%s'", preference);
            break;
        }

        bandMask |= GetBandBitMask(preference);

        LE_DEBUG("New Band <%s> set", preference);
        i++;
        sprintf (cfgNodeLoc, "%d", i);
    }
    LE_DEBUG("Set Band Preference bit mask: 0x%016"PRIX64, bandMask);
    if (bandMask)
    {
        if ( pa_mrc_SetBandPreference(bandMask) != LE_OK )
        {
            LE_WARN("Unable to set the Band preference in the configDb.");
        }
    }
    le_cfg_CancelTxn(mrcCfg);
    memset(preference, '\0', sizeof(preference));

    // Set the preferred LTE Bands
    mrcCfg = le_cfg_CreateReadTxn(CFG_MODEMSERVICE_MRC_LTE_BAND_PATH);
    uint64_t lteBandMask = 0ULL;
    i=0;
    sprintf (cfgNodeLoc, "%d", i);
    while (!le_cfg_IsEmpty(mrcCfg, cfgNodeLoc))
    {
        if ( le_cfg_GetString(mrcCfg, cfgNodeLoc, preference, sizeof(preference), "") != LE_OK )
        {
            LE_WARN("Node value string for '%s' too large.", preference);
            break;
        }
        if ( strncmp(preference, "", sizeof(preference)) == 0 )
        {
            LE_WARN("No node value set for '%s'", preference);
            break;
        }

        lteBandMask |= GetLteBandBitMask(preference);

        LE_DEBUG("New LTE Band <%s> set", preference);
        i++;
        sprintf (cfgNodeLoc, "%d", i);
    }
    LE_DEBUG("Set LTE Band Preference bit mask: 0x%016"PRIX64, lteBandMask);
    if (lteBandMask)
    {
        if ( pa_mrc_SetLteBandPreference(lteBandMask) != LE_OK )
        {
            LE_WARN("Unable to set the LTE Band preference in the configDb.");
        }
    }
    le_cfg_CancelTxn(mrcCfg);
    memset(preference, '\0', sizeof(preference));

    // Set the preferred TDS-CDMA Bands
    mrcCfg = le_cfg_CreateReadTxn(CFG_MODEMSERVICE_MRC_TDSCDMA_BAND_PATH);
    uint8_t tdScdmaBandMask = 0x00;
    i=0;
    sprintf (cfgNodeLoc, "%d", i);
    while (!le_cfg_IsEmpty(mrcCfg, cfgNodeLoc))
    {
        if ( le_cfg_GetString(mrcCfg, cfgNodeLoc, preference, sizeof(preference), "") != LE_OK )
        {
            LE_WARN("Node value string for '%s' too large.", preference);
            break;
        }
        if ( strncmp(preference, "", sizeof(preference)) == 0 )
        {
            LE_WARN("No node value set for '%s'", preference);
            break;
        }

        tdScdmaBandMask |= GetTdScdmaBandBitMask(preference);

        LE_DEBUG("New TD-SCDMA <%s> set", preference);
        i++;
        sprintf (cfgNodeLoc, "%d", i);
    }
    LE_DEBUG("Set TD-SCDMA Band Preference bit mask: 0x%01X", tdScdmaBandMask);
    if (tdScdmaBandMask)
    {
        if ( pa_mrc_SetTdScdmaBandPreference(tdScdmaBandMask) != LE_OK )
        {
            LE_WARN("Unable to set the TD-SCDMA Band preference in the configDb.");
        }
    }
    le_cfg_CancelTxn(mrcCfg);
}

//--------------------------------------------------------------------------------------------------
/**
 * The first-layer Network Registration State Change Handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static void FirstLayerNetRegStateChangeHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    le_mrc_NetRegState_t*           statePtr = reportPtr;
    le_mrc_NetRegStateHandlerFunc_t clientHandlerFunc = secondLayerHandlerFunc;

    clientHandlerFunc(*statePtr, le_event_GetContextPtr());

    // The reportPtr is a reference counted object, so need to release it
    le_mem_Release(reportPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * New Network Registration State handler function.
 *
 */
//--------------------------------------------------------------------------------------------------
static void NewRegStateHandler
(
    le_mrc_NetRegState_t* regStatePtr
)
{
    LE_DEBUG("Handler Function called with regStat %d", *regStatePtr);

    // Notify all the registered client's handlers
    le_event_ReportWithRefCounting(NewNetRegStateId, regStatePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Function to destroy all safeRef elements in the list.
 *
 */
//--------------------------------------------------------------------------------------------------
static void DeleteSafeRefList
(
    le_dls_List_t* listPtr
)
{
    ScanInfoSafeRef_t* nodePtr;
    le_dls_Link_t *linkPtr;

    while ((linkPtr=le_dls_Pop(listPtr)) != NULL)
    {
        nodePtr = CONTAINER_OF(linkPtr, ScanInfoSafeRef_t, link);
        le_ref_DeleteRef(ScanInformationRefMap, nodePtr->safeRef);
        le_mem_Release(nodePtr);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Function to convert configDB string into bitMask value for rat
 *
 */
//--------------------------------------------------------------------------------------------------
static le_mrc_RatBitMask_t ConvertRatValue
(
    const char* ratValue
)
{
    if      ( strcmp(ratValue, "GSM") == 0 )
    {
        return LE_MRC_BITMASK_RAT_GSM;
    }
    else if ( strcmp(ratValue, "UMTS") == 0 )
    {
        return LE_MRC_BITMASK_RAT_UMTS;
    }
    else if ( strcmp(ratValue, "LTE") == 0 )
    {
        return LE_MRC_BITMASK_RAT_LTE;
    }
    else if ( strcmp(ratValue, "CDMA") == 0 )
    {
        return LE_MRC_BITMASK_RAT_CDMA;
    }

    LE_WARN("This rat value '%s' is not supported",ratValue);
    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function to load all rat preference for a given ratPath
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t LoadRatList
(
    const char          *ratPath,
    le_mrc_RatBitMask_t *ratMaskPtr
)
{
    uint32_t idx=0;
    LE_DEBUG("Load Rat Preference <%s>",ratPath);

    le_cfg_IteratorRef_t ratCfg = le_cfg_CreateReadTxn(ratPath);

    *ratMaskPtr = 0;
    do {
        // Get the node name.
        char ratNodeName[LIMIT_MAX_PATH_BYTES] = {0};
        char ratNodeValue[LIMIT_MAX_PATH_BYTES] = {0};

        sprintf(ratNodeName,"%d",idx);

        // This is the exist state for the loop
        if (le_cfg_IsEmpty(ratCfg, ratNodeName))
        {
            LE_DEBUG("'%s' does not exist. stop reading configuration", ratNodeName);
            break;
        }

        if ( le_cfg_GetString(ratCfg,ratNodeName,ratNodeValue,sizeof(ratNodeValue), "") != LE_OK )
        {
            LE_WARN("Node value string for '%s' too large.",ratNodeName);
            le_cfg_CancelTxn(ratCfg);
            return LE_NOT_POSSIBLE;
        }

        if ( strncmp(ratNodeName,"",sizeof(ratNodeName)) == 0 )
        {
            LE_WARN("No node value set for '%s'",ratNodeName);
            le_cfg_CancelTxn(ratCfg);
            return LE_NOT_POSSIBLE;
        }

        *ratMaskPtr |= ConvertRatValue(ratNodeValue);

        ++idx;
    } while (true);

    le_cfg_CancelTxn(ratCfg);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Load the preferred operators configuration
 *
 */
//--------------------------------------------------------------------------------------------------
static void LoadPreferredOperators
(
)
{
    uint32_t idx = 0;
    le_dls_List_t preferredOperatorsList = LE_DLS_LIST_INIT;

    // Check that the modemRadioControl has a configuration value for preferred list.
    le_cfg_IteratorRef_t mrcCfg = le_cfg_CreateReadTxn(CFG_MODEMSERVICE_MRC_PATH"/"CFG_NODE_PREF_OPERATORS);

    if (le_cfg_NodeExists(mrcCfg,"") == false)
    {
        LE_DEBUG("'%s' does not exist. Stop reading configuration",
                    CFG_MODEMSERVICE_MRC_PATH"/"CFG_NODE_PREF_OPERATORS);
        le_cfg_CancelTxn(mrcCfg);
        return;
    }

    // Read all network from configDB
    do
    {
        le_mrc_RatBitMask_t ratMask;
        char mccNodePath[LIMIT_MAX_PATH_BYTES] = {0};
        char mncNodePath[LIMIT_MAX_PATH_BYTES] = {0};
        char ratNodePath[LIMIT_MAX_PATH_BYTES] = {0};
        char mccStr[LIMIT_MAX_PATH_BYTES] = {0};
        char mncStr[LIMIT_MAX_PATH_BYTES] = {0};

        // Get the node name.
        char nodeName[LIMIT_MAX_PATH_BYTES] = {0};

        sprintf(nodeName,"%d",idx);

        if (le_cfg_IsEmpty(mrcCfg, nodeName))
        {
            LE_DEBUG("'%s' does not exist. stop reading configuration", nodeName);
            break;
        }

        snprintf(mccNodePath, sizeof(mccNodePath), "%s/%s",nodeName,CFG_NODE_MCC);
        snprintf(mncNodePath, sizeof(mncNodePath), "%s/%s",nodeName,CFG_NODE_MNC);
        snprintf(ratNodePath, sizeof(ratNodePath),
                 CFG_MODEMSERVICE_MRC_PATH"/"CFG_NODE_PREF_OPERATORS"/%s/%s",nodeName,CFG_NODE_RAT);

        if ( le_cfg_GetString(mrcCfg,mccNodePath,mccStr,sizeof(mccStr),"") != LE_OK )
        {
            LE_WARN("String value for '%s' too large.",mccNodePath);
            break;
        }

        if ( strcmp(mccStr,"") == 0 )
        {
            LE_WARN("No node value set for '%s'",mccNodePath);
            break;
        }

        if ( le_cfg_GetString(mrcCfg,mncNodePath,mncStr,sizeof(mncStr),"") != LE_OK )
        {
            LE_WARN("String value for '%s' too large.",mncNodePath);
            break;
        }

        if ( strcmp(mncStr,"") == 0 )
        {
            LE_WARN("No node value set for '%s'",mncNodePath);
            break;
        }

        if ( LoadRatList(ratNodePath,&ratMask) != LE_OK )
        {
            LE_WARN("Could not read rat information in '%s'",ratNodePath);
            break;
        }

        if ( pa_mrc_AddPreferredOperators(&preferredOperatorsList,mccStr,mncStr,ratMask) != LE_OK )
        {
            LE_WARN("Could not add [%s,%s] into the preferred list",mccStr,mncStr);
        }

        ++idx;
    }
    while (true);

    le_cfg_CancelTxn(mrcCfg);

    if ( pa_mrc_SavePreferredOperators(&preferredOperatorsList) != LE_OK )
    {
        LE_WARN("Could not save the preferred list");
    }
    pa_mrc_DeletePreferredOperators(&preferredOperatorsList);
}

//--------------------------------------------------------------------------------------------------
/**
 * Load the scanMode configuration
 *
 */
//--------------------------------------------------------------------------------------------------
static void LoadScanMode
(
)
{
    char configPath[LIMIT_MAX_PATH_BYTES];
    snprintf(configPath, sizeof(configPath), "%s/%s",CFG_MODEMSERVICE_MRC_PATH,CFG_NODE_SCANMODE);

    LE_DEBUG("Start reading MRC scanMode information in ConfigDB");

    le_cfg_IteratorRef_t mrcCfg = le_cfg_CreateReadTxn(configPath);

    do
    {
        if ( le_cfg_GetBool(mrcCfg,CFG_NODE_MANUAL,false) )
        {

            char mccStr[LIMIT_MAX_PATH_BYTES] = {0};
            char mncStr[LIMIT_MAX_PATH_BYTES] = {0};

            if ( le_cfg_GetString(mrcCfg,CFG_NODE_MCC,mccStr,sizeof(mccStr),"") != LE_OK )
            {
                LE_WARN("String value for '%s' too large.",CFG_NODE_MCC);
                break;
            }

            if ( strcmp(mccStr,"") == 0 )
            {
                LE_WARN("No node value set for '%s'",CFG_NODE_MCC);
                break;
            }

            if ( le_cfg_GetString(mrcCfg,CFG_NODE_MNC,mncStr,sizeof(mncStr),"") != LE_OK )
            {
                LE_WARN("String value for '%s' too large.",CFG_NODE_MNC);
                break;
            }

            if ( strcmp(mncStr,"") == 0 )
            {
                LE_WARN("No node value set for '%s'",CFG_NODE_MNC);
                break;
            }

            if ( le_mrc_RegisterCellularNetwork(mccStr,mncStr) != LE_OK )
            {
                LE_WARN("Could not Register to Network [%s,%s]",mccStr ,mncStr);
                break;
            }
        }
        else
        {
            if ( pa_mrc_SetAutomaticNetworkRegistration() != LE_OK )
            {
                LE_WARN("Could not set the Automatic Network Registration");
                break;
            }
        }

    } while (false);

    le_cfg_CancelTxn(mrcCfg);
}

//--------------------------------------------------------------------------------------------------
/**
 * Load the configuration tree
 *
 */
//--------------------------------------------------------------------------------------------------
static void LoadMrcConfigurationFromConfigDB
(
    void
)
{
    LE_DEBUG("Start reading MRC information in ConfigDB");

    LoadPreferencesFromConfigDb();

    LoadPreferredOperators();

    LoadScanMode();
}

//--------------------------------------------------------------------------------------------------
/**
 * The first-layer Radio Access Technology Change Handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static void FirstLayerRatChangeHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    le_mrc_Rat_t*           ratPtr = reportPtr;
    le_mrc_RatChangeHandlerFunc_t clientHandlerFunc = secondLayerHandlerFunc;

    clientHandlerFunc(*ratPtr, le_event_GetContextPtr());

    // The reportPtr is a reference counted object, so need to release it
    le_mem_Release(reportPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Radio Access Technology Change handler function.
 *
 */
//--------------------------------------------------------------------------------------------------
static void RatChangeHandler
(
    le_mrc_Rat_t* ratPtr
)
{
    LE_DEBUG("Handler Function called with RAT %d", *ratPtr);

    // Notify all the registered client's handlers
    le_event_ReportWithRefCounting(RatChangeId, ratPtr);
}

//--------------------------------------------------------------------------------------------------
// APIs.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the MRC component.
 */
//--------------------------------------------------------------------------------------------------
void le_mrc_Init
(
    void
)
{
    le_result_t                 result=LE_OK;
    pa_mrc_NetworkRegSetting_t  setting;

    ScanInformationListPool = le_mem_CreatePool("ScanInformationListPool",
                                                sizeof(ScanInfoList_t));

    ScanInformationSafeRefPool = le_mem_CreatePool("ScanInformationSafeRefPool",
                                                   sizeof(ScanInfoSafeRef_t));

    // Create the Safe Reference Map to use for Scan Information List object Safe References.
    ScanInformationListRefMap = le_ref_CreateMap("ScanInformationListMap", MRC_MAX_SCANLIST);

    // Create the Safe Reference Map to use for Scan Information List object Safe References.
    ScanInformationRefMap = le_ref_CreateMap("ScanInformationMap", MRC_MAX_SCAN);

    // Create the pool for cells information list.
    CellListPool = le_mem_CreatePool("CellListPool",
                                              sizeof(CellList_t));

    // Create the Safe Reference Map to use for neighboring cells information object Safe References.
    CellRefMap = le_ref_CreateMap("CellInfoCellMap", MAX_NUM_NEIGHBORS);

    // Create the pool for cells information safe ref list.
    CellInfoSafeRefPool = le_mem_CreatePool("CellInfoSafeRefPool", sizeof(CellSafeRef_t));

    // Create the Safe Reference Map to use for neighboring cells information list object Safe References.
    CellListRefMap = le_ref_CreateMap("CellListRefMap", MAX_NUM_NEIGHBOR_LISTS);

    // Create an event Id for new Network Registration State notification
    NewNetRegStateId = le_event_CreateIdWithRefCounting("NewNetRegState");

    // Create an event Id for RAT change notification
    RatChangeId = le_event_CreateIdWithRefCounting("RatChange");

    LoadMrcConfigurationFromConfigDB();

    // Register a handler function for new Registration State indication
    pa_mrc_AddNetworkRegHandler(NewRegStateHandler);

    // Register a handler function for new RAT change indication
    pa_mrc_SetRatChangeHandler(RatChangeHandler);

    // Get & Set the Network registration state notification
    LE_DEBUG("Get the Network registration state notification configuration");
    result=pa_mrc_GetNetworkRegConfig(&setting);
    if ((result != LE_OK) || (setting == PA_MRC_DISABLE_REG_NOTIFICATION))
    {
        LE_ERROR_IF((result != LE_OK),
                    "Fails to get the Network registration state notification configuration");

        LE_INFO("Enable the Network registration state notification");
        pa_mrc_ConfigureNetworkReg(PA_MRC_ENABLE_REG_NOTIFICATION);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register an handler for Network registration state change.
 *
 * @return A handler reference, which is only needed for later removal of the handler.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_mrc_NetRegStateHandlerRef_t le_mrc_AddNetRegStateHandler
(
    le_mrc_NetRegStateHandlerFunc_t handlerFuncPtr, ///< [IN] The handler function.
    void*                           contextPtr      ///< [IN] The handler's context.
)
{
    le_event_HandlerRef_t        handlerRef;

    if (handlerFuncPtr == NULL)
    {
        LE_KILL_CLIENT("Handler function is NULL !");
        return NULL;
    }

    handlerRef = le_event_AddLayeredHandler("NewNetRegStateHandler",
                                            NewNetRegStateId,
                                            FirstLayerNetRegStateChangeHandler,
                                            (le_event_HandlerFunc_t)handlerFuncPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (le_mrc_NetRegStateHandlerRef_t)(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to remove an handler for Network registration state changes.
 */
//--------------------------------------------------------------------------------------------------
void le_mrc_RemoveNetRegStateHandler
(
    le_mrc_NetRegStateHandlerRef_t    handlerRef ///< [IN] The handler reference.
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

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
)
{
    le_event_HandlerRef_t        handlerRef;

    if (handlerFuncPtr == NULL)
    {
        LE_KILL_CLIENT("Handler function is NULL !");
        return NULL;
    }

    handlerRef = le_event_AddLayeredHandler("RatChangeHandler",
                                            RatChangeId,
                                            FirstLayerRatChangeHandler,
                                            (le_event_HandlerFunc_t)handlerFuncPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (le_mrc_RatChangeHandlerRef_t)(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to remove an handler for Radio Access Technology changes.
 */
//--------------------------------------------------------------------------------------------------
void le_mrc_RemoveRatChangeHandler
(
    le_mrc_RatChangeHandlerRef_t    handlerRef ///< [IN] The handler reference.
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}
//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the power of the Radio Module.
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_SetRadioPower
(
    le_onoff_t    power   ///< [IN] The power state.
)
{
    le_result_t res;

    res=pa_mrc_SetRadioPower(power);

    if (res != LE_OK)
    {
        return LE_FAULT;
    }
    else
    {
        return LE_OK;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the Radio Module power state.
 *
 * @return LE_NOT_POSSIBLE  The function failed to get the Radio Module power state.
 * @return LE_OK            The function succeed.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_GetRadioPower
(
    le_onoff_t*    powerPtr   ///< [OUT] The power state.
)
{
    le_result_t res;
    if (powerPtr == NULL)
    {
        LE_KILL_CLIENT("powerPtr is NULL !");
        return LE_FAULT;
    }

    res=pa_mrc_GetRadioPower(powerPtr);

    if (res != LE_OK)
    {
        return LE_NOT_POSSIBLE;
    }
    else
    {
        return LE_OK;
    }
}

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
)
{
    if (ratPtr == NULL)
    {
        LE_KILL_CLIENT("ratPtr is NULL !");
        return LE_FAULT;
    }

    if (pa_mrc_GetRadioAccessTechInUse(ratPtr) == LE_OK)
    {
        return LE_OK;
    }
    else
    {
        return LE_NOT_POSSIBLE;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the Network registration state.
 *
 * @return LE_NOT_POSSIBLE  The function failed to get the Network registration state.
 * @return LE_OK            The function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_GetNetRegState
(
    le_mrc_NetRegState_t*   statePtr  ///< [OUT] The Network Registration state.
)
{
    if (statePtr == NULL)
    {
        LE_KILL_CLIENT("statePtr is NULL !");
        return LE_FAULT;
    }

    if (pa_mrc_GetNetworkRegState(statePtr) == LE_OK)
    {
        return LE_OK;
    }
    else
    {
        return LE_NOT_POSSIBLE;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the Signal Quality information.
 *
 * @return LE_NOT_POSSIBLE  The function failed to get the Signal Quality information.
 * @return LE_OK            The function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_GetSignalQual
(
    uint32_t*   qualityPtr  ///< [OUT] The received signal strength quality (0 = no signal strength,
                            ///        5 = very good signal strength).
)
{
    le_result_t   res;
    int32_t       rssi;   // The received signal strength (in dBm).
    int32_t       thresholds[] = {-113, -100, -90, -80, -65}; // TODO: Verify thresholds !
    uint32_t      i=0;
    size_t        thresholdsCount = NUM_ARRAY_MEMBERS(thresholds);

    if (qualityPtr == NULL)
    {
        LE_KILL_CLIENT("qualityPtr is NULL !");
        return LE_FAULT;
    }

    if ((res=pa_mrc_GetSignalStrength(&rssi)) == LE_OK)
    {
        for (i=0; i<thresholdsCount; i++)
        {
            if (rssi <= thresholds[i])
            {
                *qualityPtr = i;
                break;
            }
        }
        if (i == thresholdsCount)
        {
            *qualityPtr = i;
        }

        LE_DEBUG("pa_mrc_GetSignalStrength has returned rssi=%ddBm", rssi);
        return LE_OK;
    }
    else if (res == LE_OUT_OF_RANGE)
    {
        LE_DEBUG("pa_mrc_GetSignalStrength has returned LE_OUT_OF_RANGE");
        *qualityPtr = 0;
        return LE_OK;
    }
    else
    {
        LE_ERROR("pa_mrc_GetSignalStrength has returned %d", res);
        *qualityPtr = 0;
        return LE_NOT_POSSIBLE;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the Current Network Name information.
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT if nameStr is NULL
 *      - LE_OVERFLOW if the Home Network Name can't fit in nameStr
 *      - LE_NOT_POSSIBLE on any other failure
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_GetCurrentNetworkName
(
    char       *nameStr,               ///< [OUT] the home network Name
    size_t      nameStrSize            ///< [IN] the nameStr size
)
{
    if (nameStr == NULL)
    {
        LE_KILL_CLIENT("nameStr is NULL !");
        return LE_FAULT;
    }

    return pa_mrc_GetCurrentNetworkName(nameStr,nameStrSize);
}



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

)
{
    if(strlen(mccPtr) > LE_MRC_MCC_LEN)
    {
        LE_KILL_CLIENT("strlen(mcc) > %d", LE_MRC_MCC_LEN);
        return LE_FAULT;
    }

    if(strlen(mncPtr) > LE_MRC_MNC_LEN)
    {
        LE_KILL_CLIENT("strlen(mnc) > %d", LE_MRC_MNC_LEN);
        return LE_FAULT;
    }

    return pa_mrc_RegisterNetwork(mccPtr,mncPtr);
}


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
    le_mrc_RatBitMask_t ratMask ///< [IN] Radio Access Technology bitmask
)
{
    le_result_t result;
    ScanInfoList_t* newScanInformationListPtr = NULL;

    newScanInformationListPtr = le_mem_ForceAlloc(ScanInformationListPool);
    newScanInformationListPtr->paScanInfoList = LE_DLS_LIST_INIT;
    newScanInformationListPtr->safeRefScanInfoList = LE_DLS_LIST_INIT;
    newScanInformationListPtr->currentLink = NULL;

    result = pa_mrc_PerformNetworkScan(ratMask,PA_MRC_SCAN_PLMN,
                                       &(newScanInformationListPtr->paScanInfoList));

    if (result != LE_OK)
    {
        le_mem_Release(newScanInformationListPtr);

        return NULL;
    }

    return le_ref_CreateRef(ScanInformationListRefMap, newScanInformationListPtr);
}

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
)
{
    pa_mrc_ScanInformation_t* nodePtr;
    le_dls_Link_t*          linkPtr;

    ScanInfoList_t* scanInformationListPtr = le_ref_Lookup(ScanInformationListRefMap,
                                                                         scanInformationListRef);

    if (scanInformationListPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", scanInformationListRef);
        return NULL;
    }

    linkPtr = le_dls_Peek(&(scanInformationListPtr->paScanInfoList));
    if (linkPtr != NULL)
    {
        nodePtr = CONTAINER_OF(linkPtr, pa_mrc_ScanInformation_t, link);
        scanInformationListPtr->currentLink = linkPtr;

        ScanInfoSafeRef_t* newScanInformationPtr = le_mem_ForceAlloc(ScanInformationSafeRefPool);
        newScanInformationPtr->safeRef = le_ref_CreateRef(ScanInformationRefMap,nodePtr);
        newScanInformationPtr->link = LE_DLS_LINK_INIT;
        le_dls_Queue(&(scanInformationListPtr->safeRefScanInfoList),&(newScanInformationPtr->link));

        return (le_mrc_ScanInformationRef_t)newScanInformationPtr->safeRef;
    }
    else
    {
        return NULL;
    }
}

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
)
{
    pa_mrc_ScanInformation_t* nodePtr;
    le_dls_Link_t*          linkPtr;

    ScanInfoList_t* scanInformationListPtr = le_ref_Lookup(ScanInformationListRefMap,
                                                                         scanInformationListRef);


    if (scanInformationListPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", scanInformationListRef);
        return NULL;
    }

    linkPtr = le_dls_PeekNext(&(scanInformationListPtr->paScanInfoList),
                                scanInformationListPtr->currentLink);
    if (linkPtr != NULL)
    {
        nodePtr = CONTAINER_OF(linkPtr, pa_mrc_ScanInformation_t, link);
        scanInformationListPtr->currentLink = linkPtr;

        ScanInfoSafeRef_t* newScanInformationPtr = le_mem_ForceAlloc(ScanInformationSafeRefPool);
        newScanInformationPtr->safeRef = le_ref_CreateRef(ScanInformationRefMap,nodePtr);
        newScanInformationPtr->link = LE_DLS_LINK_INIT;
        le_dls_Queue(&(scanInformationListPtr->safeRefScanInfoList),&(newScanInformationPtr->link));

        return (le_mrc_ScanInformationRef_t)newScanInformationPtr->safeRef;
    }
    else
    {
        return NULL;
    }
}

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
)
{
    ScanInfoList_t* scanInformationListPtr = le_ref_Lookup(ScanInformationListRefMap,
                                                                         scanInformationListRef);

    if (scanInformationListPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", scanInformationListRef);
        return;
    }

    scanInformationListPtr->currentLink = NULL;
    pa_mrc_DeleteScanInformation(&(scanInformationListPtr->paScanInfoList));

    // Delete the safe Reference list.
    DeleteSafeRefList(&(scanInformationListPtr->safeRefScanInfoList));

    // Invalidate the Safe Reference.
    le_ref_DeleteRef(ScanInformationListRefMap, scanInformationListRef);

    le_mem_Release(scanInformationListPtr);
}


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
)
{
    pa_mrc_ScanInformation_t* scanInformationPtr = le_ref_Lookup(ScanInformationRefMap,
                                                                 scanInformationRef);

    if (scanInformationPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", scanInformationRef);
        return LE_FAULT;
    }

    if (mccPtr == NULL)
    {
        LE_KILL_CLIENT("mccPtr is NULL");
        return LE_FAULT;
    }

    if (mncPtr == NULL)
    {
        LE_KILL_CLIENT("mncPtr is NULL");
        return LE_FAULT;
    }

    if ( le_utf8_Copy(mccPtr,scanInformationPtr->mobileCode.mcc,mccPtrSize,NULL) != LE_OK )
    {
        LE_WARN("Could not copy all mcc");
        return LE_OVERFLOW;
    }

    if ( le_utf8_Copy(mncPtr,scanInformationPtr->mobileCode.mnc,mncPtrSize,NULL) != LE_OK )
    {
        LE_WARN("Could not copy all mnc");
        return LE_OVERFLOW;
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
)
{
    pa_mrc_ScanInformation_t* scanInformationPtr = le_ref_Lookup(ScanInformationRefMap,
                                                                 scanInformationRef);

    if (scanInformationPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", scanInformationRef);
        return LE_FAULT;
    }

    return pa_mrc_GetScanInformationName(scanInformationPtr,namePtr,nameSize);
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the radio access technology of a scanInformationRef.
 *
 * @return
 *      - the radio access technology
 *
 * @note
 *      On failure, the process exits.
 */
//--------------------------------------------------------------------------------------------------
le_mrc_Rat_t le_mrc_GetCellularNetworkRat
(
    le_mrc_ScanInformationRef_t scanInformationRef    ///< [IN] Scan information reference
)
{
    pa_mrc_ScanInformation_t* scanInformationPtr = le_ref_Lookup(ScanInformationRefMap,
                                                                 scanInformationRef);

    if (scanInformationPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", scanInformationRef);
        return LE_FAULT;
    }

    return scanInformationPtr->rat;
}


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
)
{
    pa_mrc_ScanInformation_t* scanInformationPtr = le_ref_Lookup(ScanInformationRefMap,
                                                                 scanInformationRef);

    if (scanInformationPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", scanInformationRef);
        return false;
    }

    return scanInformationPtr->isInUse;
}

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
)
{
    pa_mrc_ScanInformation_t* scanInformationPtr = le_ref_Lookup(ScanInformationRefMap,
                                                                 scanInformationRef);

    if (scanInformationPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", scanInformationRef);
        return false;
    }

    return scanInformationPtr->isAvailable;
}


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
)
{
    pa_mrc_ScanInformation_t* scanInformationPtr = le_ref_Lookup(ScanInformationRefMap,
                                                                 scanInformationRef);

    if (scanInformationPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", scanInformationRef);
        return false;
    }

    return scanInformationPtr->isHome;
}


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
)
{
    pa_mrc_ScanInformation_t* scanInformationPtr = le_ref_Lookup(ScanInformationRefMap,
                                                                 scanInformationRef);

    if (scanInformationPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", scanInformationRef);
        return false;
    }

    return scanInformationPtr->isForbidden;
}


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
)
{
    CellList_t* ngbrCellsInfoListPtr = (CellList_t*)le_mem_ForceAlloc(CellListPool);

    if (ngbrCellsInfoListPtr != NULL)
    {
        ngbrCellsInfoListPtr->paNgbrCellInfoList = LE_DLS_LIST_INIT;
        ngbrCellsInfoListPtr->safeRefCellInfoList = LE_DLS_LIST_INIT;
        ngbrCellsInfoListPtr->currentLinkPtr = NULL;
        ngbrCellsInfoListPtr->cellsCount = pa_mrc_GetNeighborCellsInfo(
                                                &(ngbrCellsInfoListPtr->paNgbrCellInfoList));
        if (ngbrCellsInfoListPtr->cellsCount > 0)
        {
            // Create and return a Safe Reference for this List object.
            return le_ref_CreateRef(CellListRefMap, ngbrCellsInfoListPtr);
        }
        else
        {
            le_mem_Release(ngbrCellsInfoListPtr);
            LE_WARN("Unable to retrieve the Neighboring Cells information!");
            return NULL;
        }
    }
    else
    {
        LE_WARN("Unable to retrieve the Neighboring Cells information!");
        return NULL;
    }
}

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
)
{
    CellList_t* ngbrCellsInfoListPtr = le_ref_Lookup(CellListRefMap, ngbrCellsRef);
    if (ngbrCellsInfoListPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ngbrCellsRef);
        return;
    }

    ngbrCellsInfoListPtr->currentLinkPtr = NULL;
    pa_mrc_DeleteNeighborCellsInfo(&(ngbrCellsInfoListPtr->paNgbrCellInfoList));

    // Delete the safe Reference list.
    DeleteCellInfoSafeRefList(&(ngbrCellsInfoListPtr->safeRefCellInfoList));
    // Invalidate the Safe Reference.
    le_ref_DeleteRef(CellListRefMap, ngbrCellsRef);

    le_mem_Release(ngbrCellsInfoListPtr);
}

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
)
{
    pa_mrc_CellInfo_t*      nodePtr;
    le_dls_Link_t*          linkPtr;
    CellList_t*  ngbrCellsInfoListPtr = le_ref_Lookup(CellListRefMap, ngbrCellsRef);
    if (ngbrCellsInfoListPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ngbrCellsRef);
        return NULL;
    }

    linkPtr = le_dls_Peek(&(ngbrCellsInfoListPtr->paNgbrCellInfoList));
    if (linkPtr != NULL)
    {
        nodePtr = CONTAINER_OF(linkPtr, pa_mrc_CellInfo_t, link);
        ngbrCellsInfoListPtr->currentLinkPtr = linkPtr;

        CellSafeRef_t* newNbgrInfoPtr = le_mem_ForceAlloc(CellInfoSafeRefPool);
        newNbgrInfoPtr->safeRef = le_ref_CreateRef(CellRefMap, nodePtr);
        newNbgrInfoPtr->link = LE_DLS_LINK_INIT;
        le_dls_Queue(&(ngbrCellsInfoListPtr->safeRefCellInfoList), &(newNbgrInfoPtr->link));

        return ((le_mrc_CellInfoRef_t)newNbgrInfoPtr->safeRef);
    }
    else
    {
        return NULL;
    }
}

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
)
{
    pa_mrc_CellInfo_t*      nodePtr;
    le_dls_Link_t*          linkPtr;
    CellList_t*  ngbrCellsInfoListPtr = le_ref_Lookup(CellListRefMap, ngbrCellsRef);
    if (ngbrCellsInfoListPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ngbrCellsRef);
        return NULL;
    }

    linkPtr = le_dls_PeekNext(&(ngbrCellsInfoListPtr->paNgbrCellInfoList), ngbrCellsInfoListPtr->currentLinkPtr);
    if (linkPtr != NULL)
    {
        nodePtr = CONTAINER_OF(linkPtr, pa_mrc_CellInfo_t, link);
        ngbrCellsInfoListPtr->currentLinkPtr = linkPtr;

        CellSafeRef_t* newNbgrInfoPtr = le_mem_ForceAlloc(CellInfoSafeRefPool);
        newNbgrInfoPtr->safeRef = le_ref_CreateRef(CellRefMap, nodePtr);
        newNbgrInfoPtr->link = LE_DLS_LINK_INIT;
        le_dls_Queue(&(ngbrCellsInfoListPtr->safeRefCellInfoList) ,&(newNbgrInfoPtr->link));

        return ((le_mrc_CellInfoRef_t)newNbgrInfoPtr->safeRef);
    }
    else
    {
        return NULL;
    }
}

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
)
{
    pa_mrc_CellInfo_t* cellInfoPtr = le_ref_Lookup(CellRefMap, ngbrCellInfoRef);
    if (cellInfoPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ngbrCellInfoRef);
        return LE_FAULT;
    }

    return (cellInfoPtr->id);
}

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
)
{
    pa_mrc_CellInfo_t* cellInfoPtr = le_ref_Lookup(CellRefMap, ngbrCellInfoRef);
    if (cellInfoPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ngbrCellInfoRef);
        return LE_FAULT;
    }

    return (cellInfoPtr->lac);
}

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
)
{
    pa_mrc_CellInfo_t* cellInfoPtr = le_ref_Lookup(CellRefMap, ngbrCellInfoRef);
    if (cellInfoPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ngbrCellInfoRef);
        return LE_FAULT;
    }

    return (cellInfoPtr->rxLevel);
}
