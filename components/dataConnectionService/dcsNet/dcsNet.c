//--------------------------------------------------------------------------------------------------
/**
 *  Data Channel Server's C code implementation of the support for networking APIs and
 *  functionalities.
 *
 *  Copyright (C) Sierra Wireless Inc.
 *
 */
//--------------------------------------------------------------------------------------------------

#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include "legato.h"
#include "interfaces.h"
#include "le_print.h"
#include "dcsNet.h"
#include "pa_dcs.h"
#include "dcs.h"


//--------------------------------------------------------------------------------------------------
/**
 * Structure used to back up the system's default network interface configs
 */
//--------------------------------------------------------------------------------------------------
pa_dcs_InterfaceDataBackup_t NetConfigBackup;


//--------------------------------------------------------------------------------------------------
/**
 * Function for retrieving the network interface state of the given network interface in the 1st
 * argument
 *
 * @return
 *     - The function returns the retrieved channel state in the 2nd argument
 *     - The function returns LE_OK upon a successful retrieval; otherwise, some other le_result_t
 *       failure cause
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_net_GetNetIntfState
(
    const char *connIntf,
    bool *state
)
{
    le_result_t ret = pa_dcs_GetInterfaceState(connIntf, state);
    if (ret != LE_OK)
    {
        LE_DEBUG("Failed to get state of channel interface %s; error: %d", connIntf, ret);
    }
    return ret;
}


//--------------------------------------------------------------------------------------------------
/**
 * Backup default GW config in the system
 */
//--------------------------------------------------------------------------------------------------
void le_net_BackupDefaultGW
(
    void
)
{
    le_result_t ret = pa_dcs_GetDefaultGateway(&NetConfigBackup);
    if (ret != LE_OK)
    {
        LE_INFO("No default GW currently set or retrievable");
        return;
    }

    LE_INFO("Default GW address %s on interface %s backed up",
            NetConfigBackup.defaultGateway, NetConfigBackup.defaultInterface);
}


//--------------------------------------------------------------------------------------------------
/**
 * Restore default GW config in the system
 *
 * @return
 *     - LE_OK upon success in restoring, otherwise, some other le_result_t failure code
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_net_RestoreDefaultGW
(
    void
)
{
    le_result_t result;

    pa_dcs_DeleteDefaultGateway();
    result = pa_dcs_SetDefaultGateway(NetConfigBackup.defaultInterface,
                                      NetConfigBackup.defaultGateway, false);
    if (result == LE_OK)
    {
        LE_INFO("Default GW address %s on interface %s restored",
                NetConfigBackup.defaultGateway, NetConfigBackup.defaultInterface);
    }

    memset(NetConfigBackup.defaultInterface, '\0', sizeof(NetConfigBackup.defaultInterface));
    memset(NetConfigBackup.defaultGateway, '\0', sizeof(NetConfigBackup.defaultGateway));
    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function for setting the system default GW to the default GW addr given to the given channel
 * specified in the input argument.  This default GW addr is retrieved from this channel's
 * technology
 *
 * @return
 *     - The function returns LE_OK upon a successful addr setting; otherwise, some other
 *       le_result_t failure cause
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_net_SetDefaultGW
(
    le_dcs_ChannelRef_t channelRef  ///< [IN] the channel on which interface its default GW
                                    ///< addr is to be set
)
{
    le_result_t ret;
    bool isIpv6;
    char intf[LE_DCS_INTERFACE_NAME_MAX_LEN] = {0};
    int intfSize = LE_DCS_INTERFACE_NAME_MAX_LEN;
    size_t gwAddrSize = LE_DCS_INTERFACE_NAME_MAX_LEN;
    char *channelName, gwAddr[LE_DCS_INTERFACE_NAME_MAX_LEN];
    pa_dcs_InterfaceDataBackup_t currentGwCfg;
    le_dcs_channelDb_t *channelDb = le_dcs_GetChannelDbFromRef(channelRef);
    if (!channelDb)
    {
        LE_ERROR("Invalid channel reference %p for setting default GW", channelRef);
        return LE_FAULT;
    }
    channelName = channelDb->channelName;

    if (channelDb->technology != LE_DCS_TECH_CELLULAR)
    {
        LE_ERROR("Channel's technology %s not supported",
                 le_dcs_ConvertTechEnumToName(channelDb->technology));
        return LE_UNSUPPORTED;
    }

    ret = le_dcsTech_GetNetInterface(channelDb->technology, channelRef, intf, intfSize);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to get network interface for channel %s of technology %s to set "
                 "default GW", channelName, le_dcs_ConvertTechEnumToName(channelDb->technology));
        return LE_FAULT;
    }

    ret = le_dcsTech_GetDefaultGWAddress(channelDb->technology, channelDb->techRef,
                                         &isIpv6, gwAddr, &gwAddrSize);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to get GW addr for channel %s of technology %s to set default GW",
                 channelName, le_dcs_ConvertTechEnumToName(channelDb->technology));
        return LE_FAULT;
    }

    if (LE_OK != pa_dcs_GetDefaultGateway(&currentGwCfg))
    {
        LE_DEBUG("No default GW currently set or retrievable");
    }
    else
    {
        LE_DEBUG("Default GW set at address %s on interface %s before change",
                 currentGwCfg.defaultInterface, currentGwCfg.defaultGateway);
    }

    ret = pa_dcs_SetDefaultGateway(intf, gwAddr, isIpv6);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to set default GW for channel %s of technology %s", channelName,
                 le_dcs_ConvertTechEnumToName(channelDb->technology));
        return LE_FAULT;
    }

    LE_INFO("Succeeded to set default GW addr to %s on interface %s for channel %s of "
            "technology %s", gwAddr, intf, channelName,
            le_dcs_ConvertTechEnumToName(channelDb->technology));

    return ret;
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the given DNS addresses into the system configs
 *
 * @return
 *     - LE_OK upon success in setting them, otherwise some other le_result_t failure code
 */
//--------------------------------------------------------------------------------------------------
static le_result_t DcsNetSetDNS
(
    bool isIpv6,                     ///< [IN] it's of type IPv6 (true) or IPv4 (false)
    char *dns1Addr,                  ///< [IN] 1st DNS IP addr to be installed
    int addr1Size,                   ///< [IN] array length of the 1st DNS IP addr to be installed
    char *dns2Addr,                  ///< [IN] 2nd DNS IP addr to be installed
    int addr2Size                    ///< [IN] array length of the 2nd DNS IP addr to be installed
)
{
    le_result_t ret = pa_dcs_SetDnsNameServers(dns1Addr, dns2Addr);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to set DNS addrs %s and %s", dns1Addr, dns2Addr);
        return ret;
    }

    LE_INFO("Succeeded to set DNS addrs %s and %s", dns1Addr, dns2Addr);

    if (isIpv6)
    {
        le_utf8_Copy(NetConfigBackup.newDnsIPv6[0], dns1Addr, addr1Size, NULL);
        le_utf8_Copy(NetConfigBackup.newDnsIPv6[1], dns2Addr, addr2Size, NULL);
        NetConfigBackup.newDnsIPv4[0][0] = '\0';
        NetConfigBackup.newDnsIPv4[1][0] = '\0';
    }
    else
    {
        le_utf8_Copy(NetConfigBackup.newDnsIPv4[0], dns1Addr, addr1Size, NULL);
        le_utf8_Copy(NetConfigBackup.newDnsIPv4[1], dns2Addr, addr2Size, NULL);
        NetConfigBackup.newDnsIPv6[0][0] = '\0';
        NetConfigBackup.newDnsIPv6[1][0] = '\0';
    }
    return ret;
}


//--------------------------------------------------------------------------------------------------
/**
 * Validate IP address
 *
 * @return
 *      - LE_OK     on success
 *      - LE_FAULT  on failure
 */
//--------------------------------------------------------------------------------------------------
static le_result_t DcsNetValidateIpAddress
(
    int af,             ///< Address family
    const char* addStr  ///< IP address to check
)
{
    struct sockaddr_in sa;

    if (inet_pton(af, addStr, &(sa.sin_addr)))
    {
        return LE_OK;
    }
    return LE_FAULT;
}


//--------------------------------------------------------------------------------------------------
/**
 * Add or remove a route according to the input flag in the last argument for the given destination
 * address and netmask onto the given network interface
 *
 * @return
 *      - LE_OK upon success, otherwise another le_result_t failure code
 */
//--------------------------------------------------------------------------------------------------
static le_result_t DcsNetChangeRoute
(
    const char *destAddr,       ///< Destination address
    const char *destMask,       ///< Address' netmask
    const char *interface,      ///< Network interface onto which to add the route
    bool isAdd                  ///< Add or remove the route
)
{
    le_result_t ret;
    pa_dcs_RouteAction_t action;
    char *actionStr;

    if (LE_OK != DcsNetValidateIpAddress(AF_INET, destAddr))
    {
        LE_ERROR("Given IP address %s invalid", destAddr);
        return LE_BAD_PARAMETER;
    }
    if ((strlen(destMask) > 0) && LE_OK != DcsNetValidateIpAddress(AF_INET, destMask))
    {
        LE_ERROR("Given IP netmask %s invalid", destMask);
        return LE_BAD_PARAMETER;
    }

    action = isAdd ? PA_DCS_ROUTE_ADD : PA_DCS_ROUTE_DELETE;
    actionStr = isAdd ? "add" : "delete";
    ret = pa_dcs_ChangeRoute(action, destAddr, destMask, interface);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to %s route on interface %s for destionation %s/%s", actionStr,
                 interface, destAddr, destMask);
    }
    else
    {
        LE_INFO("Succeeded to %s route on interface %s for destination %s/%s", actionStr,
                 interface, destAddr, destMask);
    }
    return ret;
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the system DNS addrs to those given to the given channel specified in the input argument.
 * These DNS addrs are retrieved from this channel's technology
 *
 * @return
 *     - The function returns LE_OK upon a successful addr setting; otherwise, some other

 */
//--------------------------------------------------------------------------------------------------
le_result_t le_net_SetDNS
(
    le_dcs_ChannelRef_t channelRef  ///< [IN] the channel from which the DNS addresses retrieved
                                    ///< will be set into the system config
)
{
    le_result_t ret;
    bool isIpv6;
    char *channelName;
    char dns1Addr[PA_DCS_IPV6_ADDR_MAX_BYTES] = {0};
    char dns2Addr[PA_DCS_IPV6_ADDR_MAX_BYTES] = {0};
    size_t addr1Size = sizeof(dns1Addr);
    size_t addr2Size = sizeof(dns2Addr);
    le_dcs_channelDb_t *channelDb = le_dcs_GetChannelDbFromRef(channelRef);
    if (!channelDb)
    {
        LE_ERROR("Invalid channel reference %p for setting default GW", channelRef);
        return LE_FAULT;
    }
    channelName = channelDb->channelName;

    if (channelDb->technology != LE_DCS_TECH_CELLULAR)
    {
        LE_ERROR("Channel's technology %s not supported",
                 le_dcs_ConvertTechEnumToName(channelDb->technology));
        return LE_UNSUPPORTED;
    }

    ret = le_dcsTech_GetDNSAddresses(channelDb->technology, channelDb->techRef, &isIpv6, dns1Addr,
                                     &addr1Size, dns2Addr, &addr2Size);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to get DNS addrs for channel %s of technology %s to set DNS config",
                 channelName, le_dcs_ConvertTechEnumToName(channelDb->technology));
        return LE_FAULT;
    }

    ret = DcsNetSetDNS(isIpv6, dns1Addr, sizeof(dns1Addr), dns2Addr, sizeof(dns2Addr));
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to set DNS addrs for channel %s of technology %s", channelName,
                 le_dcs_ConvertTechEnumToName(channelDb->technology));
        return LE_FAULT;
    }

    LE_INFO("Succeeded to set DNS addrs %s and %s for channel %s of technology %s", dns1Addr,
            dns2Addr, channelName, le_dcs_ConvertTechEnumToName(channelDb->technology));

    return ret;
}


//--------------------------------------------------------------------------------------------------
/**
 * Remove the last added DNS addrs via the le_dcs_SetDNS API
 */
//--------------------------------------------------------------------------------------------------
void le_net_RestoreDNS
(
    void
)
{
    pa_dcs_RestoreInitialDnsNameServers(&NetConfigBackup);
    LE_INFO("Last added DNS addresses removed");
}


//--------------------------------------------------------------------------------------------------
/**
 * Add or remove a route on the given channel according to the input flag in the last argument for
 * the given destination address its given netmask
 *
 * @return
 *      - LE_OK upon success, otherwise another le_result_t failure code
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_net_ChangeRoute
(
    le_dcs_ChannelRef_t channelRef,  ///< [IN] the channel onto which the route change is made
    const char *destAddr,            ///< [IN] Destination IP address for the route
    const char *destMask,            ///< [IN] The destination address' netmask
    bool isAdd                       ///< [IN] the change is to add (true) or delete (false)
)
{
    le_result_t ret;
    char *channelName, intfName[LE_DCS_INTERFACE_NAME_MAX_LEN] = {0};
    int intfNameSize = LE_DCS_INTERFACE_NAME_MAX_LEN;
    le_dcs_channelDb_t *channelDb = le_dcs_GetChannelDbFromRef(channelRef);
    if (!channelDb)
    {
        LE_ERROR("Invalid channel reference %p for changing route", channelRef);
        return LE_FAULT;
    }
    channelName = channelDb->channelName;

    if (channelDb->technology != LE_DCS_TECH_CELLULAR)
    {
        LE_ERROR("Channel's technology %s not supported",
                 le_dcs_ConvertTechEnumToName(channelDb->technology));
        return LE_UNSUPPORTED;
    }

    ret = le_dcsTech_GetNetInterface(channelDb->technology, channelRef, intfName, intfNameSize);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to get net interface of channel %s of technology %s to change route",
                 channelName, le_dcs_ConvertTechEnumToName(channelDb->technology));
        return LE_FAULT;
    }

    ret = DcsNetChangeRoute(destAddr, destMask, intfName, isAdd);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to %s route for channel %s of technology %s on interface %s",
                 isAdd ? "add" : "delete", channelName,
                 le_dcs_ConvertTechEnumToName(channelDb->technology), intfName);
    }
    else
    {
        LE_INFO("Succeeded to %s route for channel %s of technology %s on interface %s",
                isAdd ? "add" : "delete", channelName,
                le_dcs_ConvertTechEnumToName(channelDb->technology), intfName);
    }
    return ret;
}


//--------------------------------------------------------------------------------------------------
/**
 *  Server initialization
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("Data Channel Service's network component is ready");
}
