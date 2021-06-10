//--------------------------------------------------------------------------------------------------
/**
 *  Data Channel Service's Technology Rank Manager
 *
 *  Copyright (C) Sierra Wireless Inc.
 *
 */
//--------------------------------------------------------------------------------------------------

#include <stdlib.h>

#include "legato.h"
#include "interfaces.h"
#ifdef LE_CONFIG_ENABLE_CONFIG_TREE
#include "le_cfg_interface.h"
#endif
#include "pa_mdc.h"
#include "dcsServer.h"
#include "dcs.h"
#include "dcsCellular.h"
#include "dcsTechRank.h"


//--------------------------------------------------------------------------------------------------
/**
 * Number of technologies
 */
//--------------------------------------------------------------------------------------------------
#define DCS_TECH_NUMBER     LE_DATA_MAX

//--------------------------------------------------------------------------------------------------
/**
 * The technology string max length
 */
//--------------------------------------------------------------------------------------------------
#define DCS_TECH_LEN      16
#define DCS_TECH_BYTES    (DCS_TECH_LEN+1)

//--------------------------------------------------------------------------------------------------
/**
 * Data associated with a technology record in the preference list
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_data_Technology_t tech;  ///< Technology
    uint32_t             rank;  ///< Technology rank
    le_dls_Link_t        link;  ///< Link in preference list
}
TechRecord_t;

//--------------------------------------------------------------------------------------------------
/**
 * List of used technologies
 */
//--------------------------------------------------------------------------------------------------
static le_dls_List_t TechList = LE_DLS_LIST_DECL_INIT;

//--------------------------------------------------------------------------------------------------
/**
 * Default list of technologies to use
 */
//--------------------------------------------------------------------------------------------------
static le_data_Technology_t DefaultTechList[DCS_TECH_NUMBER] = {LE_DATA_WIFI, LE_DATA_CELLULAR};

//--------------------------------------------------------------------------------------------------
/**
 * Pool used to store the list of technologies to use
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(TechListPoolRef, DCS_TECH_NUMBER, sizeof(TechRecord_t));
static le_mem_PoolRef_t TechListPoolRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Technologies availability
 */
//--------------------------------------------------------------------------------------------------
static bool TechAvailability[DCS_TECH_NUMBER];

//--------------------------------------------------------------------------------------------------
/**
 * Pointer on current technology in list
 */
//--------------------------------------------------------------------------------------------------
static le_dls_Link_t* CurrTechPtr = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Index profile use for data connection on cellular
 */
//--------------------------------------------------------------------------------------------------
static int32_t MdcIndexProfile = LE_MDC_DEFAULT_PROFILE;


//--------------------------------------------------------------------------------------------------
/**
 * This function converts le_data's technology type enum to le_dcs's technology type enum
 *
 * @return
 *     - le_dcs_tech enum that corresponds to the same technology type of the input enum
 */
//--------------------------------------------------------------------------------------------------
le_dcs_Technology_t dcsTechRank_ConvertToDcsTechEnum
(
    le_data_Technology_t leDataTech               ////< [IN] le_data technology type enum
)
{
    switch (leDataTech)
    {
#ifdef LE_CONFIG_ENABLE_WIFI
        case LE_DATA_WIFI:
            return LE_DCS_TECH_WIFI;
#endif
        case LE_DATA_CELLULAR:
            return LE_DCS_TECH_CELLULAR;
        default:
            return LE_DCS_TECH_UNKNOWN;
    }
}

#ifdef LE_CONFIG_ENABLE_WIFI
//--------------------------------------------------------------------------------------------------
/**
 * Try to retrieve the configured SSID from the config tree and set it into the global variable
 * Ssid if found.
 *
 * @return
 *     - true upon success; false otherwise
 */
//--------------------------------------------------------------------------------------------------
static bool RetrieveWifiCfgSsid
(
    char *ssid
)
{
    char configPath[LE_CFG_STR_LEN_BYTES];

    if (!ssid)
    {
        return false;
    }

    snprintf(configPath, sizeof(configPath), "%s/%s", DCS_CONFIG_TREE_ROOT_DIR, CFG_PATH_WIFI);
    le_cfg_IteratorRef_t cfg = le_cfg_CreateReadTxn(configPath);
    if (!le_cfg_NodeExists(cfg, CFG_NODE_SSID))
    {
        LE_WARN("No value set for '%s'!", CFG_NODE_SSID);
        le_cfg_CancelTxn(cfg);
        return false;
    }

    if (LE_OK != le_cfg_GetString(cfg, CFG_NODE_SSID, ssid, LE_WIFIDEFS_MAX_SSID_LENGTH,
                                  "testSsid"))
    {
        LE_WARN("String value for '%s' too large", CFG_NODE_SSID);
        le_cfg_CancelTxn(cfg);
        return false;
    }

    // SSID successfully retrieved
    LE_DEBUG("AP configuration, SSID: '%s'", ssid);
    le_cfg_CancelTxn(cfg);
    return true;
}
#endif

//--------------------------------------------------------------------------------------------------
/**
 * This function sets the given technology in the input argument as the current technology for use
 * and selects out of it a channel. For cellular it'll retrieve the default cellular profile; for
 * Wifi it'll retrieve the configured SSID from the config tree. Upon any failure to select a
 * channel, the current technology is still set to the given one but the selected channel will be
 * left blank.
 *
 * @return
 *     - LE_OK if a channel of the given technology has been successfully retrieved & set;
 *       otherwise LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
le_result_t dcsTechRank_SelectDataChannel
(
    le_data_Technology_t technology
)
{
    int32_t index;
    char dataChannelName[LE_DCS_CHANNEL_NAME_MAX_LEN] = {0};
    le_dcs_Technology_t dcsTech = dcsTechRank_ConvertToDcsTechEnum(technology);
#ifdef LE_CONFIG_ENABLE_WIFI
    char ssid[LE_WIFIDEFS_MAX_SSID_BYTES] = {0};
#endif
    le_dcs_ChannelRef_t dataChannelRef;

    if (dcs_ChannelIsConnected())
    {
        LE_ERROR("Failed to select the given technology %d in the present of an active connection",
                 technology);
        return LE_FAULT;
    }

    dcs_ChannelSetCurrentTech(technology);
    switch (technology)
    {
        case LE_DATA_CELLULAR:
            index = le_dcsCellular_GetProfileIndex(MdcIndexProfile);
            if (index < 0)
            {
                LE_ERROR("Unable to use cellular with profile index %d", index);
                return LE_FAULT;
            }
            le_dcsCellular_GetNameFromIndex(index, dataChannelName);
            dataChannelRef = dcs_GetReference(dataChannelName, dcsTech);
            if (!dataChannelRef)
            {
                // Allow to create the channel Db even the request channel is not in channel list.
                dataChannelRef = dcs_CreateChannelDb(dcsTech, dataChannelName);
                if (!dataChannelRef)
                {
                    LE_ERROR("Failed to create dbs for channel %s of technology %d",
                             dataChannelName, dcsTech);
                    return LE_FAULT;
                }
                LE_DEBUG("Dbs successfully created for channel %s of technology %d",
                         dataChannelName, dcsTech);
            }
            dcs_ChannelSetChannelName(dataChannelName);
            dcs_ChannelSetCurrentReference(dataChannelRef);
            MdcIndexProfile = index;
            LE_INFO("Selected channel name %s", dataChannelName);
            return LE_OK;
#ifdef LE_CONFIG_ENABLE_WIFI
        case LE_DATA_WIFI:
            if (!RetrieveWifiCfgSsid(ssid) || (strlen(ssid) == 0))
            {
                LE_ERROR("Failed to retrieve wifi config SSID");
                return LE_FAULT;
            }
            dataChannelRef = dcs_GetReference(ssid, dcsTech);
            if (!dataChannelRef)
            {
                // Allow to create the channel Db even the request channel is not in channel list.
                dataChannelRef = dcs_CreateChannelDb(dcsTech, ssid);
                if (!dataChannelRef)
                {
                    LE_ERROR("Failed to create dbs for channel %s of technology %d",
                             ssid, dcsTech);
                    return LE_FAULT;
                }
                LE_DEBUG("Dbs successfully created for channel %s of technology %d", ssid, dcsTech);
            }
            // Copy the validated SSID into the selected DataChannelName
            dcs_ChannelSetChannelName(ssid);
            dcs_ChannelSetCurrentReference(dataChannelRef);
            LE_INFO("Selected channel name %s", ssid);
            return LE_OK;
#endif
        default:
            LE_ERROR("Can't choose unknown technology %d", dcsTech);
            return LE_FAULT;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks if the specified technology is already in the list
 *
 * @return
 *      - pointer on technology record if technology is present
 *      - NULL otherwise
 */
//--------------------------------------------------------------------------------------------------
static TechRecord_t* IsTechInList
(
    le_data_Technology_t tech   ///< [IN] Technology index
)
{
    le_dls_Link_t* linkPtr = le_dls_Peek(&TechList);

    // Go through the list
    while (NULL != linkPtr)
    {
        TechRecord_t* techPtr = CONTAINER_OF(linkPtr, TechRecord_t, link);

        if (techPtr->tech == tech)
        {
            // Technology found
            return techPtr;
        }

        // Get next list element
        linkPtr = le_dls_PeekNext(&TechList, linkPtr);
    }

    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Increment the rank of all technologies present in the list beginning with
 * the pointer given in input
 */
//--------------------------------------------------------------------------------------------------
static void IncrementTechRanks
(
    le_dls_Link_t* linkPtr  ///< [IN] Pointer on the first node to update
)
{
    // Go through the list
    while (linkPtr != NULL)
    {
        TechRecord_t* techPtr = CONTAINER_OF(linkPtr, TechRecord_t, link);

        // Increase rank
        techPtr->rank++;

        // Get next list element
        linkPtr = le_dls_PeekNext(&TechList, linkPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the next technology to use after the one given as an input
 *
 * The only goal of this function is to get a technology to use for the default data connection,
 * the current one being unavailable. If the end of the list is reached, the first technology
 * is used again. The technology finally used (first one or not) is identified later when
 * the new connection status is notified.
 *
 * @return
 *      - The next technology if the end of the list is not reached
 *      - The first technology of the list if the end is reached
 */
//--------------------------------------------------------------------------------------------------
le_data_Technology_t dcsTechRank_GetNextTech
(
    le_data_Technology_t technology     ///< [IN] Technology to find in the list
)
{
    le_dls_Link_t* linkPtr = le_dls_Peek(&TechList);
    le_dls_Link_t* nextTechPtr = NULL;

    // Go through the list
    while (linkPtr)
    {
        TechRecord_t* techPtr = CONTAINER_OF(linkPtr, TechRecord_t, link);

        if (techPtr->tech == technology)
        {
            // Technology found, get the next one in the list
            // ToDo: Check against TechAvailability[]
            nextTechPtr = le_dls_PeekNext(&TechList, linkPtr);
            break;
        }

        // Get next list element
        linkPtr = le_dls_PeekNext(&TechList, linkPtr);
    }

    if (!nextTechPtr)
    {
        // No next technology, get the first one
        return le_data_GetFirstUsedTechnology();
    }

    // Next technology
    return CONTAINER_OF(nextTechPtr, TechRecord_t, link)->tech;
}


//--------------------------------------------------------------------------------------------------
/**
 * Convert le_data_Technology_t technology into a human-readable string
 *
 * @return
 *      - LE_OK             The string is copied to the output buffer
 *      - LE_OVERFLOW       The output buffer is too small
 *      - LE_BAD_PARAMETER  The technology is unknown
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetTechnologyString
(
    le_data_Technology_t tech,              ///< [IN] Technology
    char*                techStr,           ///< [IN] Buffer for technology string
    size_t               techStrNumElem     ///< [IN] Size of buffer
)
{
    le_result_t result = LE_OK;

    switch (tech)
    {
#ifdef LE_CONFIG_ENABLE_WIFI
        case LE_DATA_WIFI:
            if (LE_OK != le_utf8_Copy(techStr, "wifi", techStrNumElem, NULL))
            {
                LE_WARN("Technology buffer is too small");
                result = LE_OVERFLOW;
            }
            break;
#endif
        case LE_DATA_CELLULAR:
            if (LE_OK != le_utf8_Copy(techStr, "cellular", techStrNumElem, NULL))
            {
                LE_WARN("Technology buffer is too small");
                result = LE_OVERFLOW;
            }
            break;
        default:
            LE_ERROR("Unknown technology: %s", techStr);
            result = LE_BAD_PARAMETER;
            break;
    }

    return result;
}

#if LE_CONFIG_LINUX
//--------------------------------------------------------------------------------------------------
/**
 * Translate le_data technology definition to le_dcs technology definition
 */
//--------------------------------------------------------------------------------------------------
static le_dcs_Technology_t DataTechToDcsTech
(
    le_data_Technology_t dataTech
)
{
    switch (dataTech)
    {
        case LE_DATA_CELLULAR:
            return LE_DCS_TECH_CELLULAR;
        case LE_DATA_WIFI:
            return LE_DCS_TECH_WIFI;
        default:
            return LE_DCS_TECH_UNKNOWN;
    }
}
#endif

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the list of technologies to use with the default values
 */
//--------------------------------------------------------------------------------------------------
static void InitDefaultTechList
(
    void
)
{
    int i;
    // Start to fill the list at rank 1
    int listRank = 1;

    // Fill technologies list with default values
    for (i = 0; i < DCS_TECH_NUMBER; i++)
    {
#if LE_CONFIG_LINUX
        // if technology is supported, we will add it to the default tech list
        if (LE_OK == dcsTech_GetChannelList(DataTechToDcsTech(DefaultTechList[i])))
        {
            if (LE_OK == le_data_SetTechnologyRank(listRank, DefaultTechList[i]))
            {
                // Technology was correctly added to the list, increase the rank
                listRank++;
            }
        }
#else
        if (LE_OK == le_data_SetTechnologyRank(listRank, DefaultTechList[i]))
        {
            // Technology was correctly added to the list, increase the rank
            listRank++;
        }
#endif
    }
}


//--------------------------------------------------------------------------------------------------
// le_data APIs
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Get the cellular profile index used by the data connection service when the cellular technology
 * is active.
 *
 * @return
 *      - Cellular profile index
 */
//--------------------------------------------------------------------------------------------------
int32_t le_data_GetCellularProfileIndex
(
    void
)
{
    return le_dcsCellular_GetProfileIndex(MdcIndexProfile);
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the cellular profile index used by the data connection service when the cellular technology
 * is active.
 *
 * @return
 *      - LE_OK             on success
 *      - LE_BAD_PARAMETER  if the profile index is invalid
 *      - LE_BUSY           the cellular connection is in use
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_data_SetCellularProfileIndex
(
    int32_t profileIndex    ///< [IN] Cellular profile index to be used
)
{
    le_mrc_Rat_t rat;
    int32_t profileIndexMin;
    int32_t profileIndexMax;

    if (dcs_ChannelIsConnected() && (LE_DATA_CELLULAR == dcs_ChannelGetCurrentTech()))
    {
        LE_ERROR("Cellular connection in use");
        return LE_BUSY;
    }

    if (LE_OK != le_mrc_GetRadioAccessTechInUse(&rat))
    {
        rat = LE_MRC_RAT_GSM;
    }

    switch (rat)
    {
        /* 3GPP2 */
        case LE_MRC_RAT_CDMA:
            profileIndexMin = PA_MDC_MIN_INDEX_3GPP2_PROFILE;
            profileIndexMax = PA_MDC_MAX_INDEX_3GPP2_PROFILE;
        break;

        /* 3GPP */
        default:
            profileIndexMin = PA_MDC_MIN_INDEX_3GPP_PROFILE;
            profileIndexMax = PA_MDC_MAX_INDEX_3GPP_PROFILE;
        break;
    }

    if (((profileIndex >= profileIndexMin) && (profileIndex <= profileIndexMax))
        || (LE_MDC_DEFAULT_PROFILE == profileIndex))
    {
        MdcIndexProfile = profileIndex;
        if (le_dcsCellular_SetProfileIndex(profileIndex) != LE_OK)
        {
            LE_ERROR("Failed to set cellular profile index to %d", profileIndex);
            return LE_FAULT;
        }
        LE_DEBUG("MdcIndexProfile set to %d", MdcIndexProfile);
        return LE_OK;
    }

    LE_ERROR("Invalid cellular profile index %d to set", MdcIndexProfile);
    return LE_BAD_PARAMETER;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the first technology to use
 * @return
 *      - One of the technologies from @ref le_data_Technology_t enumerator if the list is not empty
 *      - @ref LE_DATA_MAX if the list is empty
 */
//--------------------------------------------------------------------------------------------------
le_data_Technology_t le_data_GetFirstUsedTechnology
(
    void
)
{
    le_data_Technology_t firstTech = LE_DATA_MAX;

    // Check if list is empty
    if (le_dls_IsEmpty(&TechList))
    {
        LE_INFO("Used technologies list is empty");
        return firstTech;
    }

    // Get first technology
    le_dls_Link_t* linkPtr = le_dls_Peek(&TechList);
    if (!linkPtr)
    {
        LE_WARN("Cannot get first used technology");
        return firstTech;
    }

    // Retrieve technology and rank
    TechRecord_t* techPtr = CONTAINER_OF(linkPtr, TechRecord_t, link);
    firstTech = techPtr->tech;
    // Store last peeked technology
    CurrTechPtr = linkPtr;

#if LE_DEBUG_ENABLED
    {
        char techStr[DCS_TECH_BYTES] = {0};
        uint32_t firstRank = techPtr->rank;

        GetTechnologyString(firstTech, techStr, sizeof(techStr));
        LE_DEBUG("First used technology: '%s' with rank %d", techStr, firstRank);
    }
#endif

    return firstTech;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the next technology to use
 * @return
 *      - One of the technologies from @ref le_data_Technology_t enumerator if the list is not empty
 *      - @ref LE_DATA_MAX if the list is empty or the end of the list is reached
 */
//--------------------------------------------------------------------------------------------------
le_data_Technology_t le_data_GetNextUsedTechnology
(
    void
)
{
    le_data_Technology_t nextTech = LE_DATA_MAX;

    // Check if list is empty
    if (le_dls_IsEmpty(&TechList))
    {
        LE_INFO("Used technologies list is empty");
        return nextTech;
    }

    // Check if CurrTechPtr is coherent
    if (!CurrTechPtr || !le_dls_IsInList(&TechList, CurrTechPtr))
    {
        LE_ERROR("Incoherent CurrTechPtr %p", CurrTechPtr);
        return nextTech;
    }

    le_dls_Link_t* linkPtr = le_dls_PeekNext(&TechList, CurrTechPtr);
    if (!linkPtr)
    {
        LE_DEBUG("End of used technologies list, cannot get the next one");
        return nextTech;
    }

    // Retrieve technology and rank
    TechRecord_t* techPtr = CONTAINER_OF(linkPtr, TechRecord_t, link);
    nextTech = techPtr->tech;
    // Store last peeked technology
    CurrTechPtr = linkPtr;

#if LE_DEBUG_ENABLED
    {
        char techStr[DCS_TECH_BYTES] = {0};
        uint32_t nextRank = techPtr->rank;

        GetTechnologyString(nextTech, techStr, sizeof(techStr));
        LE_DEBUG("Next used technology: '%s' with rank %d", techStr, nextRank);
    }
#endif

    return nextTech;
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the rank of the technology used for the data connection service
 *
 * @return
 *      - @ref LE_OK if the technology is added to the list
 *      - @ref LE_BAD_PARAMETER if the technology is unknown
 *      - @ref LE_UNSUPPORTED if the technology is not available
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_data_SetTechnologyRank
(
    uint32_t rank,                  ///< [IN] Rank of the used technology
    le_data_Technology_t technology ///< [IN] Technology
)
{
    // Check if technology is correct
    if (technology >= LE_DATA_MAX)
    {
        LE_WARN("Unknown technology %d, not added to the list", technology);
        return LE_BAD_PARAMETER;
    }

    // Get technology string
    char techStr[DCS_TECH_BYTES] = {0};
    GetTechnologyString(technology, techStr, sizeof(techStr));

    // Check if technology is available
    if (false == TechAvailability[technology])
    {
        LE_WARN("Unsupported technology '%s', not added to the list", techStr);
        return LE_UNSUPPORTED;
    }

    LE_DEBUG("Adding technology '%s' with the rank %d to the list", techStr, rank);

    // Check if technology is already in the list
    TechRecord_t* techPtr = IsTechInList(technology);
    if (NULL != techPtr)
    {
        if (rank != techPtr->rank)
        {
            // Remove the technology from it's current rank
            le_dls_Remove(&TechList, &techPtr->link);

            // The technology can now be added at the new rank
            LE_DEBUG("Technology %s was already in list with rank %d, setting new rank %d",
                     techStr, techPtr->rank, rank);
        }
        else
        {
            // Technology already in list with correct rank, nothing to do
            LE_DEBUG("Technology %s already in list with same rank %d", techStr, rank);
            return LE_OK;
        }
    }

    // Check if list is empty
    if (true == le_dls_IsEmpty(&TechList))
    {
        // Create the new technology node
        TechRecord_t* newTechPtr = le_mem_ForceAlloc(TechListPoolRef);
        newTechPtr->tech = technology;
        newTechPtr->rank = rank;
        newTechPtr->link = LE_DLS_LINK_INIT;

        // Add the technology node to the list
        le_dls_Stack(&TechList, &(newTechPtr->link));
    }
    else
    {
        // Insert in the list according to the rank
        le_dls_Link_t* currentLinkPtr = TechList.headLinkPtr;
        le_dls_Link_t* nextLinkPtr = NULL;
        uint32_t currRank = 0;
        uint32_t nextRank = UINT32_MAX;
        bool inserted = false;

        // Create the new technology node
        TechRecord_t* newTechPtr = le_mem_ForceAlloc(TechListPoolRef);
        newTechPtr->tech = technology;
        newTechPtr->rank = rank;
        newTechPtr->link = LE_DLS_LINK_INIT;

        // Find the correct rank for the new technology
        do
        {
            currRank = CONTAINER_OF(currentLinkPtr, TechRecord_t, link)->rank;
            nextLinkPtr = le_dls_PeekNext(&TechList, currentLinkPtr);
            nextRank = (NULL != nextLinkPtr ? CONTAINER_OF(nextLinkPtr, TechRecord_t, link)->rank
                                            : UINT32_MAX);

            if (rank < currRank)
            {
                // Lower rank for the new technology, add it before the current technology

                // Add the node before the current one
                le_dls_AddBefore(&TechList, currentLinkPtr, &(newTechPtr->link));
                inserted = true;
            }
            else if (rank == currRank)
            {
                // Same rank for the new technology, add it before the current technology
                // and increment the rank of the current and next ones

                // Add the node before the current one
                le_dls_AddBefore(&TechList, currentLinkPtr, &(newTechPtr->link));
                inserted = true;
                // The next ranks are incremented
                IncrementTechRanks(le_dls_PeekNext(&TechList, &(newTechPtr->link)));
            }
            else
            {
                // Higher rank for the new technology, check the next rank to know where
                // it should be inserted in the list

                if (rank < nextRank)
                {
                    // Higher next rank, add the new technology between the current one
                    // and the next one

                    // Add the node after the current one and before the next one
                    le_dls_AddAfter(&TechList, currentLinkPtr, &(newTechPtr->link));
                    inserted = true;
                }
                else if (rank == nextRank)
                {
                    // Same next rank, add the new technology between the current one
                    // and the next one, and increment the rank of the current and next technologies

                    // Add the node after the current one and replace the next one
                    le_dls_AddAfter(&TechList, currentLinkPtr, &(newTechPtr->link));
                    inserted = true;
                    // The next ranks are incremented
                    IncrementTechRanks(le_dls_PeekNext(&TechList, &(newTechPtr->link)));
                }
                else
                {
                    // Lower next rank, try the next link in list
                }
            }

            // Move to the next link
            currentLinkPtr = currentLinkPtr->nextPtr;

        } while ((currentLinkPtr != TechList.headLinkPtr) && (false == inserted));
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Initialize memory pools
 */
//--------------------------------------------------------------------------------------------------
void dcsTechRank_InitPools
(
    void
)
{
    TechListPoolRef = le_mem_InitStaticPool(TechListPoolRef, DCS_TECH_NUMBER, sizeof(TechRecord_t));
}

//--------------------------------------------------------------------------------------------------
/**
 * Init function for le_data's technology lists and ranks
 */
//--------------------------------------------------------------------------------------------------
void dcsTechRank_Init
(
    void
)
{
    // Services required by DCS

    // 1. Mobile services
    // Mobile services are always available
    TechAvailability[LE_DATA_CELLULAR] = true;

    // 2. Wifi service
    // Check wifi client availability
#ifdef LE_CONFIG_ENABLE_WIFI
    if (LE_OK == le_wifiClient_TryConnectService())
    {
        LE_INFO("Wifi client is available");
        TechAvailability[LE_DATA_WIFI] = true;
    }
    else
    {
        LE_INFO("Wifi client is not available");
        TechAvailability[LE_DATA_WIFI] = false;
    }
#else
    TechAvailability[LE_DATA_WIFI] = false;
#endif

    // Initialize technologies list with default values
    InitDefaultTechList();
}
