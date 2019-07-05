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
 * DHCP Lease File Options. These are used as keys to search in the DHCP lease file
 */
//--------------------------------------------------------------------------------------------------
#define DEFAULT_GW_OPTION "routers"
#define DNS_ADDRESS_OPTION "domain-name-servers"

//--------------------------------------------------------------------------------------------------
/**
 * File Path Length
 */
//--------------------------------------------------------------------------------------------------
#define FILE_PATH_LENGTH_BYTES 128

//--------------------------------------------------------------------------------------------------
/**
 * Possible number of each type of IP version addresses that can be found in a lease file
 */
//--------------------------------------------------------------------------------------------------
#define MAX_NUM_DEFAULT_GATEWAY_ADDRESS_BY_TYPE 1
#define MAX_NUM_DNS_ADDRESS_BY_TYPE             2

//--------------------------------------------------------------------------------------------------
/**
 * Max length of a DHCP line to read
 */
//--------------------------------------------------------------------------------------------------
#define LEASE_FILE_MAX_LINE_LENGTH_BYTES  (DHCP_LEASE_OPTION_MAX_LEN_BYTES \
                                        + (MAX_NUM_DNS_ADDRESS_BY_TYPE*PA_DCS_IPV6_ADDR_MAX_BYTES) \
                                        + (MAX_NUM_DNS_ADDRESS_BY_TYPE*PA_DCS_IPV4_ADDR_MAX_BYTES))

//--------------------------------------------------------------------------------------------------
/**
 * Maximal length of a DHCP Lease option
 */
//--------------------------------------------------------------------------------------------------
#define DHCP_LEASE_OPTION_MAX_LEN_BYTES 50

//--------------------------------------------------------------------------------------------------
/**
 * Enumeration for DHCP Address
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_NET_DEFAULT_GATEWAY_ADDRESS,   ///< Default gateway address(es)
    LE_NET_DNS_SERVER_ADDRESS         ///< DNS server address(es)
}
DhcpAddress_t;

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
    bool ipv4AddrAssigned = false;
    bool ipv6AddrAssigned = false;
    *state = false;
    le_result_t ret = pa_dcs_GetInterfaceState(connIntf,
                                               &ipv4AddrAssigned,
                                               &ipv6AddrAssigned);
    if (ret != LE_OK)
    {
        LE_DEBUG("Failed to get state of channel interface %s; error: %d", connIntf, ret);
    }

    if (ipv4AddrAssigned || ipv6AddrAssigned)
    {
        *state = true;
    }
    return ret;
}


//--------------------------------------------------------------------------------------------------
/**
 * Parses DHCP lease file and returns string for specified option
 *
 * @return
 *      LE_NOT_FOUND    Lease file does not exist, does not contain what is being looked for, or
 *                      cannot be opened
 *      LE_OVERFLOW     Destination buffer too small and output will be truncated
 *      LE_FAULT        Function failed
 *      LE_OK           Function succeed
 */
//--------------------------------------------------------------------------------------------------
le_result_t GetDhcpLeaseFileEntry
(
    const char*     interfaceStrPtr,    ///< [IN]    Pointer to interface string
    DhcpAddress_t   infoType,           ///< [IN]    Lease file address to return
    char*           destPtr,            ///< [OUT]   Output destination
    size_t*         destSizeBytes       ///< [INOUT] Size of buffer in,
                                        ///< Number of bytes filled out
)
{
    le_result_t result = LE_NOT_FOUND;
    FILE*       leaseFile;
    char        pathBuff[FILE_PATH_LENGTH_BYTES];
    char        line[LEASE_FILE_MAX_LINE_LENGTH_BYTES];
    char        searchStr[DHCP_LEASE_OPTION_MAX_LEN_BYTES];
    char*       searchPtr = NULL;

    // Nullify output string
    memset(destPtr, '\0', *destSizeBytes);

    // Build Path
    result = pa_dcs_GetDhcpLeaseFilePath(interfaceStrPtr,
                                         pathBuff,
                                         sizeof(pathBuff));

    if (result != LE_OK)
    {
        LE_ERROR("Unable to get %s DHCP lease file path", interfaceStrPtr);
        return LE_FAULT;
    }

    // Determine what you're looking for in the lease file
    switch (infoType)
    {
        case LE_NET_DNS_SERVER_ADDRESS:
            le_utf8_Copy(searchStr,
                         DNS_ADDRESS_OPTION,
                         DHCP_LEASE_OPTION_MAX_LEN_BYTES,
                         NULL);
            break;

        case LE_NET_DEFAULT_GATEWAY_ADDRESS:
            le_utf8_Copy(searchStr,
                         DEFAULT_GW_OPTION,
                         DHCP_LEASE_OPTION_MAX_LEN_BYTES,
                         NULL);
            break;

        default:
            LE_ERROR("Unknown info type %d", (uint16_t)infoType);
            return LE_FAULT;
    }

    LE_DEBUG("Attempting to read in %s DHCP lease file", interfaceStrPtr);

    // Open file
    leaseFile = le_flock_TryOpenStream(pathBuff, LE_FLOCK_READ, &result);
    if (NULL == leaseFile)
    {
        LE_ERROR("Could not open %s DHCP lease file. Error: %d", pathBuff, result);
        return result;
    }

    LE_DEBUG("Lease file successfully read");

    // Search through lease file for desired item
    while (fgets(line, sizeof(line), leaseFile))
    {
        searchPtr = strstr(line, searchStr);
        if (searchPtr)
        {
            result = le_utf8_Copy(destPtr,
                                  searchPtr + strnlen(searchStr, sizeof(searchStr)) + 1,
                                  *destSizeBytes,
                                  destSizeBytes);

            // Remove semicolon if there is one
            char* semicolonPtr = strchr(destPtr, ';');
            if (semicolonPtr != NULL)
            {
                *semicolonPtr = '\0';
                (*destSizeBytes)--;
            }

            break;
        }
    }

    le_flock_CloseStream(leaseFile);

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Returns the DHCP addresses specified
 *
 * @return
 *      LE_NOT_FOUND    Lease file does not exist or does not contain what is being looked for
 *      LE_OVERFLOW     Destination buffer
 *      LE_FAULT        Function failed
 *      LE_OK           Function succeeded
 */
//--------------------------------------------------------------------------------------------------
le_result_t GetLeaseAddresses
(
    const char*     interfaceStrPtr,    ///< [IN]  Pointer to interface string
    DhcpAddress_t   infoType,           ///< [IN]  Lease file address to return
    char*           v4AddrPtr,          ///< [OUT] Pointer to address
    size_t          v4AddrSize,         ///< [IN]  Size of each of the IPv4 addresses
    char*           v6AddrPtr,          ///< [OUT] 2 IPv6 DNS addresses to be installed
    size_t          v6AddrSize,         ///< [IN]  Size of each IPv6 DNS addresses
    size_t          numAddresses        ///< [IN]  Number of addresses of each type
)
{
    int     result;
    char    addressBuffer[MAX_NUM_DNS_ADDRESS_BY_TYPE
                        * (PA_DCS_IPV4_ADDR_MAX_BYTES + PA_DCS_IPV6_ADDR_MAX_BYTES + 1)];
    char    *token, *rest;
    int     ipv4AddrCnt             = 0;
    int     ipv6AddrCnt             = 0;
    size_t  addrBufferSizeBytes     = sizeof(addressBuffer);
    bool    isIpv6                  = false;

    if (numAddresses > MAX_NUM_DNS_ADDRESS_BY_TYPE)
    {
        LE_ERROR("Too many addresses requested. Requested %zu but max allowed is %d",
                 numAddresses,
                 MAX_NUM_DNS_ADDRESS_BY_TYPE);
        return LE_FAULT;
    }

    // Nullify output buffers
    memset(v4AddrPtr, '\0', v4AddrSize*numAddresses);
    memset(v6AddrPtr, '\0', v6AddrSize*numAddresses);

    // Get a string for the address entry in the lease file to parse
    result = GetDhcpLeaseFileEntry(interfaceStrPtr,
                                   infoType,
                                   addressBuffer,
                                   &addrBufferSizeBytes);


    LE_DEBUG("Trying to parse: %s", addressBuffer);

    if (LE_OK == result)
    {
        rest = addressBuffer;

        // Addresses should be separated by spaces
        while ((token = strtok_r(rest, " ", &rest)))
        {
            // If it contains a colon, it's likely an IPv6 address
            isIpv6 = ((strchr(token, ':') == NULL) ? false : true);

            if (!isIpv6 && (ipv4AddrCnt < numAddresses))
            {
                strncpy(v4AddrPtr, token, v4AddrSize);
                v4AddrPtr += v4AddrSize;
                ipv4AddrCnt++;
            }
            else if (isIpv6 && (ipv6AddrCnt < numAddresses))
            {
                strncpy(v6AddrPtr, token, v6AddrSize);
                v6AddrPtr += v6AddrSize;
                ipv6AddrCnt++;
            }
            else
            {
                continue;
            }
        }
    }

    return result;
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

    if ((LE_DCS_TECH_UNKNOWN == channelDb->technology) ||
        (LE_DCS_TECH_MAX <= channelDb->technology))
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
    if (channelDb->technology == LE_DCS_TECH_CELLULAR)
    {
        ret = le_dcsTech_GetDefaultGWAddress(channelDb->technology, channelDb->techRef,
                                             v4GwAddr, v4GwAddrSize, v6GwAddr, v6GwAddrSize);
    }
    else
    {
        ret = GetLeaseAddresses(intf, LE_NET_DEFAULT_GATEWAY_ADDRESS,
                                v4GwAddr, v4GwAddrSize, v6GwAddr, v6GwAddrSize,
                                MAX_NUM_DEFAULT_GATEWAY_ADDRESS_BY_TYPE);
    }

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
 * Gets the DNS server addresses for the given data channel
 *
 * @return
 *      - LE_OK upon success, otherwise LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_net_GetDefaultGW
(
    le_dcs_ChannelRef_t               channelRef, ///< [IN]    Channel to retrieve GW addresses
    le_net_DefaultGatewayAddresses_t* addr        ///< [OUT]   Channel's Default GW Addresses
)
{
    le_result_t ret;
    char intf[LE_DCS_INTERFACE_NAME_MAX_LEN] = {0};
    int intfSize = LE_DCS_INTERFACE_NAME_MAX_LEN;
    le_dcs_channelDb_t *channelDb = le_dcs_GetChannelDbFromRef(channelRef);

    if (addr == NULL)
    {
        LE_ERROR("Passing a NULL ptr refence is not allowed");
        return LE_FAULT;
    }
    if (!channelDb)
    {
        LE_ERROR("Invalid channel reference %p for setting default GW", channelRef);
        return LE_FAULT;
    }

    // Get network interface for setting default GW config
    ret = le_dcsTech_GetNetInterface(channelDb->technology, channelRef, intf, intfSize);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to get network interface for channel %s of technology %s to set default GW",
                 channelDb->channelName, le_dcs_ConvertTechEnumToName(channelDb->technology));
        return LE_FAULT;
    }

    // Clear addresses
    addr->ipv4Addr[0] = '\0';
    addr->ipv6Addr[0] = '\0';

    return GetLeaseAddresses(intf, LE_NET_DEFAULT_GATEWAY_ADDRESS,
                             addr->ipv4Addr, sizeof(addr->ipv4Addr),
                             addr->ipv6Addr, sizeof(addr->ipv6Addr),
                             MAX_NUM_DEFAULT_GATEWAY_ADDRESS_BY_TYPE);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the given DNS addresses into the system configs
 *
 * @return
 *      LE_OK           Function succeed
 *      LE_DUPLICATE    Function found no need to add as the given inputs are already set in
 *      LE_UNSUPPORTED  Function not supported by the target
 *      LE_FAULT        Function failed
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
    if (ret == LE_DUPLICATE)
    {
        LE_DEBUG("Given DNS addresses already set");
        return ret;
    }
    else if (ret != LE_OK)
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
 * address and subnet's prefix length onto the given network interface.
 *
 * @return
 *      - LE_OK upon success, otherwise another le_result_t failure code
 */
//--------------------------------------------------------------------------------------------------
static le_result_t DcsNetChangeRoute
(
    const char *destAddr,       ///< Destination address
    const char *prefixLength,   ///< Destination's subnet prefix length
    const char *interface,      ///< Network interface onto which to add the route
    bool isAdd                  ///< Add or remove the route
)
{
    le_result_t ret;
    pa_dcs_RouteAction_t action;
    char *actionStr;

    action = isAdd ? PA_DCS_ROUTE_ADD : PA_DCS_ROUTE_DELETE;
    actionStr = isAdd ? "add" : "delete";
    if (!prefixLength)
    {
        // Set it to a null string for convenience in debug printing
        prefixLength = "";
    }
    ret = pa_dcs_ChangeRoute(action, destAddr, prefixLength, interface);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to %s route on interface %s for destination %s subnet %s", actionStr,
                 interface, destAddr, strlen(prefixLength) ? prefixLength : "none");
    }
    else
    {
        LE_INFO("Succeeded to %s route on interface %s for destination %s subnet %s", actionStr,
                interface, destAddr, strlen(prefixLength) ? prefixLength : "none");
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
    char intf[LE_DCS_INTERFACE_NAME_MAX_LEN] = {0};
    int intfSize = LE_DCS_INTERFACE_NAME_MAX_LEN;
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

    if ((LE_DCS_TECH_UNKNOWN == channelDb->technology) ||
        (LE_DCS_TECH_MAX <= channelDb->technology))
    {
        LE_ERROR("Channel's technology %s not supported",
                 le_dcs_ConvertTechEnumToName(channelDb->technology));
        return LE_UNSUPPORTED;
    }

    // Query technology for IPv4 and IPv6 DNS server address assignments
    if (channelDb->technology == LE_DCS_TECH_CELLULAR)
    {
        ret = le_dcsTech_GetDNSAddresses(channelDb->technology, channelDb->techRef,
                                        (char *)v4DnsAddrs, PA_DCS_IPV4_ADDR_MAX_BYTES,
                                        (char *)v6DnsAddrs, PA_DCS_IPV6_ADDR_MAX_BYTES);
    }
    else
    {
        ret = le_dcsTech_GetNetInterface(channelDb->technology, channelRef, intf, intfSize);
        if (ret != LE_OK)
        {
            LE_ERROR("Failed to get network interface for channel %s of technology %s to set "
                    "default GW", channelName, le_dcs_ConvertTechEnumToName(channelDb->technology));
            return LE_FAULT;
        }
        ret = GetLeaseAddresses(intf, LE_NET_DNS_SERVER_ADDRESS,
                                (char *)v4DnsAddrs, PA_DCS_IPV4_ADDR_MAX_BYTES,
                                (char *)v6DnsAddrs, PA_DCS_IPV6_ADDR_MAX_BYTES,
                                MAX_NUM_DNS_ADDRESS_BY_TYPE);
    }

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
        if ((v6Ret != LE_OK) && (v6Ret != LE_DUPLICATE))
        {
            LE_ERROR("Failed to set DNS addresses for channel %s of technology %s", channelName,
                     le_dcs_ConvertTechEnumToName(channelDb->technology));
        }
    }

    // Set IPv4 DNS server addresses
    if ((strlen(v4DnsAddrs[0]) > 0) || (strlen(v4DnsAddrs[1]) > 0))
    {
        v4Ret = DcsNetSetDNS(false, v4DnsAddrs[0], v4DnsAddrs[1], PA_DCS_IPV4_ADDR_MAX_BYTES);
        if ((v4Ret != LE_OK) && (v4Ret != LE_DUPLICATE))
        {
            LE_ERROR("Failed to set DNS addresses for channel %s of technology %s", channelName,
                     le_dcs_ConvertTechEnumToName(channelDb->technology));
        }
    }

    if ((v4Ret == LE_DUPLICATE) || (v6Ret == LE_DUPLICATE))
    {
        LE_INFO("DNS addresses of channel %s of technology %s already set in",
                channelName, le_dcs_ConvertTechEnumToName(channelDb->technology));
        return LE_DUPLICATE;
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
 * Gets the DNS server addresses for the given data channel
 *
 * @return
 *      - LE_OK upon success, otherwise LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_net_GetDNS
(
    le_dcs_ChannelRef_t          channelRef, ///< [IN]    Channel to retrieve DNS server addresses
    le_net_DnsServerAddresses_t* addr        ///< [OUT]   DNS server Addresses
)
{
    le_result_t result;
    char intf[LE_DCS_INTERFACE_NAME_MAX_LEN] = {0};
    char v4DnsAddrs[2][PA_DCS_IPV4_ADDR_MAX_BYTES] = {{0}, {0}};
    char v6DnsAddrs[2][PA_DCS_IPV6_ADDR_MAX_BYTES] = {{0}, {0}};
    int intfSize = LE_DCS_INTERFACE_NAME_MAX_LEN;
    le_dcs_channelDb_t *channelDb = le_dcs_GetChannelDbFromRef(channelRef);

    if (addr == NULL)
    {
        LE_ERROR("Passing a NULL ptr refence is not allowed");
        return LE_FAULT;
    }
    if (!channelDb)
    {
        LE_ERROR("Invalid channel reference %p for setting default GW", channelRef);
        return LE_FAULT;
    }

    // Get Network Interface
    result = le_dcsTech_GetNetInterface(channelDb->technology, channelRef, intf, intfSize);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get network interface for channel %s of technology %s to set "
                "default GW", channelDb->channelName,
                le_dcs_ConvertTechEnumToName(channelDb->technology));
        return LE_FAULT;
    }

    // Clear addresses
    memset(addr, '\0', sizeof(*addr));

    result = GetLeaseAddresses(intf, LE_NET_DNS_SERVER_ADDRESS,
                               (char *)v4DnsAddrs, PA_DCS_IPV4_ADDR_MAX_BYTES,
                               (char *)v6DnsAddrs, PA_DCS_IPV6_ADDR_MAX_BYTES,
                               MAX_NUM_DNS_ADDRESS_BY_TYPE);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get DNS lease addresses for %s interface",
                 intf);
        return LE_FAULT;
    }

    // Copy addresses to struct
    le_utf8_Copy(addr->ipv4Addr1, v4DnsAddrs[0], sizeof(addr->ipv4Addr1), NULL);
    le_utf8_Copy(addr->ipv4Addr2, v4DnsAddrs[1], sizeof(addr->ipv4Addr2), NULL);
    le_utf8_Copy(addr->ipv6Addr1, v6DnsAddrs[0], sizeof(addr->ipv6Addr1), NULL);
    le_utf8_Copy(addr->ipv6Addr2, v6DnsAddrs[1], sizeof(addr->ipv6Addr2), NULL);

    return result;
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
 * Get the prefix length from a subnet mask.
 * For instance, 255.255.255.0 = 24.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ConvertSubnetMaskToPrefixLength
(
    const char * subnetMask,
    char * prefixLengthStr,
    size_t prefixLengthSz
)
{
    // Need to convert the netmask into a prefix length
    struct in_addr subnetStruct;
    if (inet_aton(subnetMask, &subnetStruct) != 1)
    {
        LE_ERROR("Unable to parse %s", subnetMask);
        return LE_FAULT;
    }

    int prefixLength = 0;
    while(subnetStruct.s_addr != 0)
    {
        if(subnetStruct.s_addr & 0x1)
        {
            prefixLength++;
        }
        subnetStruct.s_addr >>= 1;
    }

    LE_DEBUG("Computed prefix length %d from netmask %s",
             prefixLength, subnetMask);

    if (snprintf(prefixLengthStr, prefixLengthSz, "%d", prefixLength) > prefixLengthSz)
    {
        return LE_OVERFLOW;
    }

    return LE_OK;
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
    const char *prefixLength, ///< [IN] Destination's subnet prefix length
    bool isAdd                ///< [IN] The change is to add (true) or delete (false)
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
    if ((LE_DCS_TECH_UNKNOWN == channelDb->technology) ||
        (LE_DCS_TECH_MAX <= channelDb->technology))
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
    if (prefixLength)
    {
        stringLen = strlen(prefixLength);
        for (i=0; (i<stringLen) && isspace(*prefixLength); i++)
        {
            prefixLength++;
        }
    }

    char bufPrefixLength[3];
    if (LE_OK == DcsNetValidateIpAddress(AF_INET, destAddr))
    {
        int16_t prefixLen;
        if (prefixLength)
        {
            prefixLen = DcsConvertPrefixLengthString(prefixLength);
            if ((prefixLen < 0) || (prefixLen > IPV6_PREFIX_LENGTH_MAX))
            {
                LE_WARN("Input IPv4 subnet mask prefix length %s invalid", prefixLength);

                // For IPv4, the parameter used to be a subnet mask, so provide some
                // compatibility code in case it was already used.
                if (LE_OK == DcsNetValidateIpAddress(AF_INET, prefixLength))
                {
                    LE_WARN("Deprecated, a prefix length is expected and not a network mask.");
                    if (ConvertSubnetMaskToPrefixLength(prefixLength,
                                                        bufPrefixLength,
                                                        sizeof(bufPrefixLength)) != LE_OK)
                    {
                        LE_ERROR("Unable to convert mask %s to prefix length.", prefixLength);
                        return LE_BAD_PARAMETER;
                    }

                    prefixLength = bufPrefixLength;
                }
                else
                {
                    return LE_BAD_PARAMETER;
                }
            }
            else if (prefixLen == 0)
            {
                // Case: prefixLength is a non-null string with all spaces; pass on a null string
                prefixLength = "";
            }
        }

    }
    else if (LE_OK == DcsNetValidateIpAddress(AF_INET6, destAddr))
    {
        int16_t prefixLen;
        if (prefixLength)
        {
            prefixLen = DcsConvertPrefixLengthString(prefixLength);
            if ((prefixLen < 0) || (prefixLen > IPV6_PREFIX_LENGTH_MAX))
            {
                LE_ERROR("Input IPv6 subnet mask prefix length %s invalid", prefixLength);
                return LE_BAD_PARAMETER;
            }
            else if (prefixLen == 0)
            {
                // Case: prefixLength is a non-null string with all spaces; pass on a null string
                prefixLength = "";
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
    ret = DcsNetChangeRoute(destAddr, prefixLength, intfName, isAdd);
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
