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
 * This define gives the max value of an IPv6 subnet prefix length, i.e. 128
 */
//--------------------------------------------------------------------------------------------------
#define IPV6_PREFIX_LENGTH_MAX (8 * 8 * 2)


//--------------------------------------------------------------------------------------------------
/**
 * The following 2 defines specify the length of the char array in which the numeric value of an
 * IPv6 subnet prefix length is. IPV6_PREFIX_LEN_STR_BYTES specifies the total byte length while
 * IPV6_PREFIX_LEN_STR_LENGTH the number of decimal digits taken by this value.
 */
//--------------------------------------------------------------------------------------------------
#define IPV6_PREFIX_LEN_STR_BYTES  4
#define IPV6_PREFIX_LEN_STR_LENGTH 3


//--------------------------------------------------------------------------------------------------
/**
 * Structure used to back up the system's default network interface configs
 */
//--------------------------------------------------------------------------------------------------
static pa_dcs_InterfaceDataBackup_t NetConfigBackup;


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
    if ((strlen(NetConfigBackup.defaultInterface) == 0) ||
        (strlen(NetConfigBackup.defaultGateway) == 0))
    {
        // Need both to set a default GW config; thus, bail out when not both are available
        LE_DEBUG("No backed up default GW address to restore");
        // memset in case either one isn't empty
        memset(NetConfigBackup.defaultInterface, '\0', sizeof(NetConfigBackup.defaultInterface));
        memset(NetConfigBackup.defaultGateway, '\0', sizeof(NetConfigBackup.defaultGateway));
        return LE_OK;
    }

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
 *     - The function returns LE_OK upon a successful addr setting; otherwise, LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_net_SetDefaultGW
(
    le_dcs_ChannelRef_t channelRef  ///< [IN] the channel on which interface its default GW
                                    ///< addr is to be set
)
{
    le_result_t ret, v4Ret = LE_FAULT, v6Ret = LE_FAULT;
    char intf[LE_DCS_INTERFACE_NAME_MAX_LEN] = {0};
    int intfSize = LE_DCS_INTERFACE_NAME_MAX_LEN;
    size_t v4GwAddrSize = PA_DCS_IPV4_ADDR_MAX_BYTES;
    size_t v6GwAddrSize = PA_DCS_IPV6_ADDR_MAX_BYTES;
    char *channelName, v4GwAddr[PA_DCS_IPV4_ADDR_MAX_BYTES], v6GwAddr[PA_DCS_IPV6_ADDR_MAX_BYTES];
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

    // Get network interface for setting default GW config
    ret = le_dcsTech_GetNetInterface(channelDb->technology, channelRef, intf, intfSize);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to get network interface for channel %s of technology %s to set "
                 "default GW", channelName, le_dcs_ConvertTechEnumToName(channelDb->technology));
        return LE_FAULT;
    }

    // Query technology for IPv4 and IPv6 default GW address assignments
    ret = le_dcsTech_GetDefaultGWAddress(channelDb->technology, channelDb->techRef,
                                         v4GwAddr, v4GwAddrSize, v6GwAddr, v6GwAddrSize);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to get GW addr for channel %s of technology %s to set default GW",
                 channelName, le_dcs_ConvertTechEnumToName(channelDb->technology));
        return ret;
    }

    if ((strlen(v6GwAddr) == 0) && (strlen(v4GwAddr) == 0))
    {
        LE_INFO("Given channel %s of technology %s got no default GW address assigned",
                channelName, le_dcs_ConvertTechEnumToName(channelDb->technology));
        return LE_FAULT;
    }

    // Get current default GW setting for reference
    if (LE_OK != pa_dcs_GetDefaultGateway(&currentGwCfg))
    {
        LE_DEBUG("No default GW currently set or retrievable on device");
    }
    else
    {
        LE_DEBUG("Default GW set at address %s on interface %s before change",
                 currentGwCfg.defaultGateway, currentGwCfg.defaultInterface);
    }

    // Seek to set IPv6 default GW address
    if (strlen(v6GwAddr) > 0)
    {
        v6Ret = pa_dcs_SetDefaultGateway(intf, v6GwAddr, true);
        if (v6Ret != LE_OK)
        {
            LE_ERROR("Failed to set IPv6 default GW for channel %s of technology %s", channelName,
                     le_dcs_ConvertTechEnumToName(channelDb->technology));
        }
    }

    // Seek to set IPv4 default GW address
    if (strlen(v4GwAddr) > 0)
    {
        v4Ret = pa_dcs_SetDefaultGateway(intf, v4GwAddr, false);
        if (v4Ret != LE_OK)
        {
            LE_ERROR("Failed to set IPv4 default GW for channel %s of technology %s", channelName,
                     le_dcs_ConvertTechEnumToName(channelDb->technology));
        }
    }

    if ((v4Ret == LE_OK) || (v6Ret == LE_OK))
    {
        LE_INFO("Succeeded to set default GW addr on interface %s for channel %s of technology %s",
                intf, channelName, le_dcs_ConvertTechEnumToName(channelDb->technology));
        return LE_OK;
    }
    return LE_FAULT;
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
    char *dns2Addr,                  ///< [IN] 2nd DNS IP addr to be installed
    int dnsAddrSize                  ///< [IN] array length of the DNS IP addresses to be installed
)
{
    le_result_t ret = pa_dcs_SetDnsNameServers(dns1Addr, dns2Addr);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to set DNS addresses %s and %s", dns1Addr, dns2Addr);
        return ret;
    }

    if (isIpv6)
    {
        le_utf8_Copy(NetConfigBackup.newDnsIPv6[0], dns1Addr, dnsAddrSize, NULL);
        le_utf8_Copy(NetConfigBackup.newDnsIPv6[1], dns2Addr, dnsAddrSize, NULL);
    }
    else
    {
        le_utf8_Copy(NetConfigBackup.newDnsIPv4[0], dns1Addr, dnsAddrSize, NULL);
        le_utf8_Copy(NetConfigBackup.newDnsIPv4[1], dns2Addr, dnsAddrSize, NULL);
    }

    LE_INFO("Succeeded to set DNS addresses %s and %s", dns1Addr, dns2Addr);
    return ret;
}


//--------------------------------------------------------------------------------------------------
/**
 * Validate IPv4/v6 address format
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
    struct sockaddr_in6 sa;

    if (inet_pton(af, addStr, &(sa.sin6_addr)))
    {
        return LE_OK;
    }
    return LE_FAULT;
}


//--------------------------------------------------------------------------------------------------
/**
 * Add or remove a route according to the input flag in the last argument for the given destination
 * address and subnet (IPv4 netmask or IPv6 prefix length) onto the given network interface
 *
 * @return
 *      - LE_OK upon success, otherwise another le_result_t failure code
 */
//--------------------------------------------------------------------------------------------------
static le_result_t DcsNetChangeRoute
(
    const char *destAddr,       ///< Destination address
    const char *destSubnet,     ///< Destination's subnet: IPv4 netmask or IPv6 prefix length
    const char *interface,      ///< Network interface onto which to add the route
    bool isAdd                  ///< Add or remove the route
)
{
    le_result_t ret;
    pa_dcs_RouteAction_t action;
    char *actionStr;

    action = isAdd ? PA_DCS_ROUTE_ADD : PA_DCS_ROUTE_DELETE;
    actionStr = isAdd ? "add" : "delete";
    if (!destSubnet)
    {
        // Set it to a null string for convenience in debug printing
        destSubnet = "";
    }
    ret = pa_dcs_ChangeRoute(action, destAddr, destSubnet, interface);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to %s route on interface %s for destionation %s subnet %s", actionStr,
                 interface, destAddr, strlen(destSubnet) ? destSubnet : "none");
    }
    else
    {
        LE_INFO("Succeeded to %s route on interface %s for destination %s subnet %s", actionStr,
                interface, destAddr, strlen(destSubnet) ? destSubnet : "none");
    }
    return ret;
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the system DNS addresses to those given to the given channel specified in the input argument.
 * These DNS addresses are retrieved from this channel's technology
 *
 * @return
 *     - The function returns LE_OK upon a successful addr setting; otherwise, LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_net_SetDNS
(
    le_dcs_ChannelRef_t channelRef  ///< [IN] the channel from which the DNS addresses retrieved
                                    ///< will be set into the system config
)
{
    le_result_t ret, v4Ret = LE_FAULT, v6Ret = LE_FAULT;
    char *channelName;
    char v4DnsAddrs[2][PA_DCS_IPV4_ADDR_MAX_BYTES] = {{0}, {0}};
    char v6DnsAddrs[2][PA_DCS_IPV6_ADDR_MAX_BYTES] = {{0}, {0}};
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

    // Query technology for IPv4 and IPv6 DNS server address assignments
    ret = le_dcsTech_GetDNSAddresses(channelDb->technology, channelDb->techRef,
                                     (char *)v4DnsAddrs, PA_DCS_IPV4_ADDR_MAX_BYTES,
                                     (char *)v6DnsAddrs, PA_DCS_IPV6_ADDR_MAX_BYTES);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to get DNS addresses for channel %s of technology %s to set DNS config",
                 channelName, le_dcs_ConvertTechEnumToName(channelDb->technology));
        return ret;
    }

    if ((strlen(v4DnsAddrs[0]) == 0) && (strlen(v4DnsAddrs[1]) == 0) &&
        (strlen(v6DnsAddrs[0]) == 0) && (strlen(v6DnsAddrs[1]) == 0))
    {
        LE_INFO("Given channel %s of technology %s got no DNS server address assigned",
                channelName, le_dcs_ConvertTechEnumToName(channelDb->technology));
        return LE_FAULT;
    }

    // Set IPv6 DNS server addresses
    if ((strlen(v6DnsAddrs[0]) > 0) || (strlen(v6DnsAddrs[1]) > 0))
    {
        v6Ret = DcsNetSetDNS(true, v6DnsAddrs[0], v6DnsAddrs[1], PA_DCS_IPV6_ADDR_MAX_BYTES);
        if (v6Ret != LE_OK)
        {
            LE_ERROR("Failed to set DNS addresses for channel %s of technology %s", channelName,
                     le_dcs_ConvertTechEnumToName(channelDb->technology));
        }
    }

    // Set IPv4 DNS server addresses
    if ((strlen(v4DnsAddrs[0]) > 0) || (strlen(v4DnsAddrs[1]) > 0))
    {
        v4Ret = DcsNetSetDNS(false, v4DnsAddrs[0], v4DnsAddrs[1], PA_DCS_IPV4_ADDR_MAX_BYTES);
        if (v4Ret != LE_OK)
        {
            LE_ERROR("Failed to set DNS addresses for channel %s of technology %s", channelName,
                     le_dcs_ConvertTechEnumToName(channelDb->technology));
        }
    }

    if ((v4Ret == LE_OK) || (v6Ret == LE_OK))
    {
        LE_INFO("Succeeded to set onto device DNS addresses of channel %s of technology %s",
                channelName, le_dcs_ConvertTechEnumToName(channelDb->technology));
        return LE_OK;
    }
    return LE_FAULT;
}


//--------------------------------------------------------------------------------------------------
/**
 * Remove the last added DNS addresses via the le_dcs_SetDNS API
 */
//--------------------------------------------------------------------------------------------------
void le_net_RestoreDNS
(
    void
)
{
    LE_DEBUG("Removing lastly added DNS addresses: IPv4: %s %s; IPv6: %s %s",
             NetConfigBackup.newDnsIPv4[0], NetConfigBackup.newDnsIPv4[1],
             NetConfigBackup.newDnsIPv6[0], NetConfigBackup.newDnsIPv6[1]);
    pa_dcs_RestoreInitialDnsNameServers(&NetConfigBackup);
}


//--------------------------------------------------------------------------------------------------
/**
 * Utility for converting the numeric value in a char string less than 4 digits long (i.e.
 * IPV6_PREFIX_LEN_STR_BYTES) into an int16 value
 */
//--------------------------------------------------------------------------------------------------
static int16_t DcsConvertPrefixLengthString
(
    const char *input
)
{
    char *tmp_ptr = NULL;
    uint16_t inputLen;
    if (!input)
    {
        return 0;
    }
    inputLen = strlen(input);
    if (inputLen == 0)
    {
        return 0;
    }
    if (inputLen > IPV6_PREFIX_LEN_STR_LENGTH)
    {
        LE_ERROR("Invalid prefix length %d", inputLen);
        return -1;
    }
    return (int16_t)strtol(input, &tmp_ptr, 10);
}


//--------------------------------------------------------------------------------------------------
/**
 * Add or remove a route on the given channel according to the input flag in the last argument for
 * the given destination address its given subnet, which is a subnet mask for IPv4 and subnet mask's
 * prefix length for IPv6
 *
 * @return
 *      - LE_OK upon success, otherwise another le_result_t failure code
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_net_ChangeRoute
(
    le_dcs_ChannelRef_t channelRef,  ///< [IN] the channel onto which the route change is made
    const char *destAddr,     ///< [IN] Destination IP address for the route
    const char *destSubnet,   ///< [IN] Destination's subnet: IPv4 netmask or IPv6 prefix length
    bool isAdd                ///< [IN] the change is to add (true) or delete (false)
)
{
    le_result_t ret;
    uint16_t i, stringLen;
    char *channelName, intfName[LE_DCS_INTERFACE_NAME_MAX_LEN] = {0};
    int intfNameSize = LE_DCS_INTERFACE_NAME_MAX_LEN;
    le_dcs_channelDb_t *channelDb = le_dcs_GetChannelDbFromRef(channelRef);
    if (!channelDb)
    {
        LE_ERROR("Invalid channel reference %p for changing route", channelRef);
        return LE_FAULT;
    }
    channelName = channelDb->channelName;

    // Validate inputs
    if ((channelDb->technology != LE_DCS_TECH_CELLULAR) &&
        (channelDb->technology != LE_DCS_TECH_WIFI))
    {
        LE_ERROR("Channel's technology %s not supported",
                 le_dcs_ConvertTechEnumToName(channelDb->technology));
        return LE_UNSUPPORTED;
    }

    // Strip leading whitespaces
    if (destAddr)
    {
        stringLen = strlen(destAddr);
        for (i=0; (i<stringLen) && isspace(*destAddr); i++)
        {
            destAddr++;
        }
    }
    else
    {
        LE_ERROR("Invalid input destination address of null");
        return LE_BAD_PARAMETER;
    }
    if (destSubnet)
    {
        stringLen = strlen(destSubnet);
        for (i=0; (i<stringLen) && isspace(*destSubnet); i++)
        {
            destSubnet++;
        }
    }

    if (LE_OK == DcsNetValidateIpAddress(AF_INET, destAddr))
    {
        if (destSubnet && (strlen(destSubnet) > 0) &&
            (LE_OK != DcsNetValidateIpAddress(AF_INET, destSubnet)))
        {
            LE_ERROR("Input IPv4 subnet mask %s invalid in format", destSubnet);
            return LE_BAD_PARAMETER;
        }
    }
    else if (LE_OK == DcsNetValidateIpAddress(AF_INET6, destAddr))
    {
        int16_t prefixLen;
        if (destSubnet)
        {
            prefixLen = DcsConvertPrefixLengthString(destSubnet);
            if ((prefixLen < 0) || (prefixLen > IPV6_PREFIX_LENGTH_MAX))
            {
                LE_ERROR("Input IPv6 subnet mask prefix length %s invalid", destSubnet);
                return LE_BAD_PARAMETER;
            }
            else if (prefixLen == 0)
            {
                // Case: destSubnet is a non-null string with all spaces; pass on a null string
                destSubnet = "";
            }
        }
    }
    else
    {
        LE_ERROR("Input IP address %s invalid in format", destAddr);
        return LE_BAD_PARAMETER;
    }

    // Get network interface
    ret = le_dcsTech_GetNetInterface(channelDb->technology, channelRef, intfName, intfNameSize);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to get net interface of channel %s of technology %s to change route",
                 channelName, le_dcs_ConvertTechEnumToName(channelDb->technology));
        return ret;
    }

    // Initiate route change
    ret = DcsNetChangeRoute(destAddr, destSubnet, intfName, isAdd);
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
