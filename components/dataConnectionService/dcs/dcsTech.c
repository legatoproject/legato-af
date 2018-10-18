//--------------------------------------------------------------------------------------------------
/**
 *  Data Channel Server's C code implementation of its southbound interfaces with the
 *  technology-specific handlers and APIs.  This implementation may get significantly revised
 *  or even eliminated when we make the southbound components plug-and-play.
 *
 *  Copyright (C) Sierra Wireless Inc.
 *
 */
//--------------------------------------------------------------------------------------------------

#include <stdlib.h>
#include <string.h>
#include "legato.h"
#include "interfaces.h"
#include "le_print.h"
#include "pa_mdc.h"
#include "pa_dcs.h"
#include "dcs.h"
#include "dcsNet.h"
#include "dcsCellular.h"
#include "dcsWifi.h"


//--------------------------------------------------------------------------------------------------
/**
 * Create a technology specific channel db for the given channel according to its given
 * technology in its 1st argument
 *
 * @return
 *     - Return in the function's return value a void * pointer to this newly created db's object
 *       reference
 *     - Return NULL back if the creation has failed
 */
//--------------------------------------------------------------------------------------------------
void *le_dcsTech_CreateTechRef
(
    le_dcs_Technology_t tech,
    const char *channelName
)
{
    void *techRef = NULL;

    switch (tech)
    {
        case LE_DCS_TECH_CELLULAR:
            techRef = le_dcsCellular_CreateConnDb(channelName);
            break;
        case LE_DCS_TECH_WIFI:
        default:
            LE_ERROR("Unsupported technology %d", tech);
    }
    return techRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Release a technology specific channel db for the given channel according to its given
 * technology in its 1st argument
 */
//--------------------------------------------------------------------------------------------------
void le_dcsTech_ReleaseTechRef
(
    le_dcs_Technology_t tech,
    void *techRef
)
{
    switch (tech)
    {
        case LE_DCS_TECH_CELLULAR:
            le_dcsCellular_ReleaseConnDb(techRef);
            break;
        case LE_DCS_TECH_WIFI:
        default:
            LE_ERROR("Unsupported technology %d", tech);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Search thru DCS's master list of available technologies for the given technology specified in the
 * input argument by its enum & return its index # on this list
 *
 * @return
 *     - In the function's return value, return the index # found for the given technology, or -1
 *       if not found
 */
//--------------------------------------------------------------------------------------------------
int16_t le_dcsTech_GetListIndx
(
    le_dcs_Technology_t technology
)
{
    int i;
    for (i=0; i<LE_DCS_TECHNOLOGY_MAX_COUNT; i++)
    {
        if (DcsInfo.techListDb[i].techEnum == technology)
        {
            return i;
        }
    }
    return -1;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function to call the given technology in the 1st argument to get its list of all available
 * channels
 *
 * @return
 *     - The 2nd argument is to be filled out with the retrieved list where the 3rd argument
 *       specifies how long this given list's size is for filling out
 *     - The function returns LE_OK upon a successful retrieval; otherwise, some other
 *       le_result_t failure cause
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_dcsTech_GetChannelList
(
    le_dcs_Technology_t tech,
    le_dcs_ChannelInfo_t *channelList,
    size_t *listSize
)
{
    le_result_t ret;
    int16_t listLen = *listSize;
    uint16_t i;
    le_dcs_channelDb_t *channelDb;
    le_dcs_ChannelRef_t channelRef;

    const char *techName = le_dcs_ConvertTechEnumToName(tech);

    // check if the given technology is supported
    switch (tech)
    {
        case LE_DCS_TECH_CELLULAR:
            ret = le_dcsCellular_GetChannelList(channelList, &listLen);
            break;
        case LE_DCS_TECH_WIFI:
            ret = le_dcsWifi_GetChannelList(channelList, &listLen);
            break;
        default:
            LE_ERROR("Unsupported technology %d", tech);
            return LE_UNSUPPORTED;
    }

    if ((ret != LE_OK) || (listLen <= 0))
    {
        LE_ERROR("Failed to get channel list for technology %s; error: %d",
                 techName, ret);
        return ret;
    }

    // Create for any new channel its dbs & its reference into the struct to be returned
    for (i = 0; i < listLen; i++)
    {
        channelDb = dcsGetChannelDbFromName(channelList[i].name, tech);
        if (!channelDb)
        {
            // It's a newly learned channel; create its dbs
            channelRef = le_dcs_CreateChannelDb(tech, channelList[i].name);
            if (!channelRef)
            {
                LE_ERROR("Failed to create dbs for new channel %s of technology %d",
                         channelList[i].name, tech);
                memset(&channelList[i], 0x0, sizeof(le_dcs_ChannelInfo_t));
                continue;
            }
        }
        else
        {
            channelRef = channelDb->channelRef;
        }
        channelList[i].ref = channelRef;
    }

    *listSize = DcsInfo.techListDb[tech].channelCount = listLen;
    LE_DEBUG("# of channels retrieved from technology %s: %d", le_dcs_ConvertTechEnumToName(tech),
             listLen);
    return ret;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function for querying the network interface of the given channel specified in the 1st argument
 * after its technology type is retrieved
 *
 * @return
 *     - The retrieved network interface's name will be returned in the 2nd argument which allowed
 *       buffer length is specified in the 3rd argument that is to be observed strictly
 *     - The function returns LE_OK upon a successful retrieval; otherwise, some other le_result_t
 *       failure cause
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_dcsTech_GetNetInterface
(
    le_dcs_Technology_t tech,
    le_dcs_ChannelRef_t channelRef,
    char *intfName,
    int nameSize
)
{
    le_result_t ret;
    char *channelName;
    le_dcs_channelDb_t *channelDb;

    if (tech != LE_DCS_TECH_CELLULAR)
    {
        LE_ERROR("Channel's technology type %s not supported", le_dcs_ConvertTechEnumToName(tech));
        return LE_UNSUPPORTED;
    }

    channelDb = le_dcs_GetChannelDbFromRef(channelRef);
    if (!channelDb)
    {
        LE_ERROR("Invalid channel reference %p for getting network interface", channelRef);
        return LE_FAULT;
    }
    channelName = channelDb->channelName;

    intfName[0] = '\0';
    ret = le_dcsCellular_GetNetInterface(channelDb->techRef, intfName, nameSize);
    if (LE_OK != ret)
    {
        LE_ERROR("Failed to get network interface of channel %s of technology %s", channelName,
                 le_dcs_ConvertTechEnumToName(tech));
    }
    return ret;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function for requesting cellular to start the given data channel in the 1st argument
 * after its technology type is retrieved
 *
 * @return
 *     - The function returns LE_OK or LE_DUPLICATE upon a successful start; otherwise, some other
 *       le_result_t failure cause
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_dcsTech_Start
(
    const char *channelName,
    le_dcs_Technology_t tech
)
{
    le_result_t ret;
    le_dcs_channelDb_t *channelDb;

    channelDb = dcsGetChannelDbFromName(channelName, tech);
    if (!channelDb)
    {
        LE_ERROR("Channel %s isn't available", channelName);
        return LE_FAULT;
    }

    if (channelDb->technology != LE_DCS_TECH_CELLULAR)
    {
        LE_ERROR("Channel's technology type %s not supported",
                 le_dcs_ConvertTechEnumToName(tech));
        return LE_UNSUPPORTED;
    }

    LE_INFO("Request to start channel %s of technology %s", channelName,
            le_dcs_ConvertTechEnumToName(tech));
    ret = le_dcsCellular_Start(channelDb->techRef);
    if ((ret != LE_OK) && (ret != LE_DUPLICATE))
    {
        LE_ERROR("Failed to start channel %s; error: %d", channelName, ret);
    }
    else
    {
        LE_DEBUG("Succeeded to start channel %s", channelName);
    }
    return ret;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function for stopping the given data channel in the argument after its technology type
 * is retrieved
 *
 * @return
 *     - The function returns LE_OK upon a successful stop; otherwise, some other
 *       le_result_t failure cause
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_dcsTech_Stop
(
    const char *channelName,
    le_dcs_Technology_t tech
)
{
    le_result_t ret;
    le_dcs_channelDb_t *channelDb;

    channelDb = dcsGetChannelDbFromName(channelName, tech);
    if (!channelDb)
    {
        LE_ERROR("Db for channel %s not found", channelName);
        return LE_FAULT;
    }

    LE_INFO("Request to stop channel %s of technology %s", channelName,
            le_dcs_ConvertTechEnumToName(tech));
    ret = le_dcsCellular_Stop(channelDb->techRef);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to stop channel %s; error: %d", channelName, ret);
    }

    return ret;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function for checking if the given channel's operational state is up or not
 *
 * @return
 *     - In the function's return value, the bool is returned to indicate whether the given
 *       channel's tech db is up or not
 */
//--------------------------------------------------------------------------------------------------
bool le_dcsTech_GetOpState
(
    le_dcs_channelDb_t *channelDb
)
{
    switch (channelDb->technology)
    {
        case LE_DCS_TECH_CELLULAR:
            return le_dcsCellular_GetOpState(channelDb->techRef);
        case LE_DCS_TECH_WIFI:
        default:
            LE_ERROR("Unsupported technology %s",
                     le_dcs_ConvertTechEnumToName(channelDb->technology));
    }
    return false;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function for initiating the given channel to retry after a channel failure. Within the
 * technology itself, the failure cause & code are retrievable.
 */
//--------------------------------------------------------------------------------------------------
void le_dcsTech_RetryChannel
(
    le_dcs_channelDb_t *channelDb
)
{
    switch (channelDb->technology)
    {
        case LE_DCS_TECH_CELLULAR:
            (void)le_dcsCellular_RetryConn(channelDb->techRef);
            break;
        case LE_DCS_TECH_WIFI:
        default:
            LE_ERROR("Unsupported technology %s",
                     le_dcs_ConvertTechEnumToName(channelDb->technology));
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Function for querying the default GW address of the given connection specified in the 2nd
 * argument of the technology given in the 1st
 *
 * @return
 *     - The retrieved default GW address will be returned in the 3rd argument which allowed
 *       buffer length is specified in the 4th argument that is to be observed strictly
 *     - The function returns LE_OK upon a successful retrieval; otherwise, some other le_result_t
 *       failure cause
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_dcsTech_GetDefaultGWAddress
(
    le_dcs_Technology_t tech,
    void *techRef,
    bool *isIpv6,
    char *gwAddr,
    size_t *gwAddrSize
)
{
    switch (tech)
    {
        case LE_DCS_TECH_CELLULAR:
            return le_dcsCellular_GetDefaultGWAddress(techRef, isIpv6, gwAddr, gwAddrSize);
            break;
        case LE_DCS_TECH_WIFI:
        default:
            LE_ERROR("Unsupported technology %s", le_dcs_ConvertTechEnumToName(tech));
            return LE_UNSUPPORTED;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Function for querying the DNS addresses of the given connection specified in the 2nd
 * argument of the technology given in the 1st
 *
 * @return
 *     - The retrieved 2 DNS addresses will be returned in the 3rd & 5th arguments which allowed
 *       buffer lengths are specified in the 4th & 6th arguments correspondingly
 *     - The function returns LE_OK upon a successful retrieval; otherwise, some other le_result_t
 *       failure cause
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_dcsTech_GetDNSAddresses
(
    le_dcs_Technology_t tech,        ///< [IN] technology type of the connection
    void *techRef,                   ///< [IN] object reference of the connection
    bool *isIpv6,                    ///< [OUT] IP addr version: IPv6 (true) or IPv4 (false)
    char *dns1Addr,                  ///< [OUT] 1st DNS IP addr to be installed
    size_t *addr1Size,               ///< [INOUT] array length of the 1st DNS IP addr to be installed
    char *dns2Addr,                  ///< [OUT] 2nd DNS IP addr to be installed
    size_t *addr2Size                ///< [INOUT] array length of the 2nd DNS IP addr to be installed
)
{
    switch (tech)
    {
        case LE_DCS_TECH_CELLULAR:
            return le_dcsCellular_GetDNSAddrs(techRef, isIpv6, dns1Addr, addr1Size,
                                              dns2Addr, addr2Size);
            break;
        case LE_DCS_TECH_WIFI:
        default:
            LE_ERROR("Unsupported technology %s", le_dcs_ConvertTechEnumToName(tech));
            return LE_UNSUPPORTED;
    }
}
