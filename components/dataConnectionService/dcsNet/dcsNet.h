//--------------------------------------------------------------------------------------------------
/**
 *  Data Channel Server's header file for the code implementation of the support for networking
 *  APIs and functionalities.
 *
 *  Copyright (C) Sierra Wireless Inc.
 *
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_DCSNET_H_INCLUDE_GUARD
#define LEGATO_DCSNET_H_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Enumeration for DHCP info type that is to be retrieved from its lease files. Currently the 2
 * supported types are default GW addresses and DNS server addresses, as shown below
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_NET_DEFAULT_GATEWAY_ADDRESS,   ///< Default gateway address(es)
    LE_NET_DNS_SERVER_ADDRESS         ///< DNS server address(es)
}
le_net_DhcpInfoType_t;

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
LE_SHARED le_result_t net_GetNetIntfState
(
    const char *connIntf,
    bool *state
);

LE_SHARED le_result_t net_SetDefaultGW
(
    le_msg_SessionRef_t sessionRef, ///< [IN] messaging session initiating this request
    le_dcs_ChannelRef_t channelRef
);
LE_SHARED void net_BackupDefaultGW
(
    le_msg_SessionRef_t sessionRef    ///< [IN] messaging session initiating this request
);
LE_SHARED le_result_t net_RestoreDefaultGW
(
    le_msg_SessionRef_t sessionRef    ///< [IN] messaging session initiating this request
);
LE_SHARED le_result_t net_SetDNS
(
    le_msg_SessionRef_t sessionRef, ///< [IN] messaging session initiating this request
    le_dcs_ChannelRef_t channelRef  ///< [IN] the channel from which the DNS addresses retrieved
                                    ///< will be set into the system config
);
LE_SHARED void net_RestoreDNS
(
    le_msg_SessionRef_t sessionRef    ///< [IN] messaging session initiating this request
);

LE_SHARED le_result_t net_ChangeRoute(le_dcs_ChannelRef_t channelRef, const char *destAddr,
                                      const char *destMask, bool isAdd);

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
LE_SHARED le_result_t net_GetLeaseAddresses
(
    const char*     interfaceStrPtr,    ///< [IN]  Pointer to interface string
    le_net_DhcpInfoType_t infoType,     ///< [IN]  Lease file info type to return
    char*           v4AddrPtr,          ///< [OUT] Pointer to address
    size_t          v4AddrSize,         ///< [IN]  Size of each of the IPv4 addresses
    char*           v6AddrPtr,          ///< [OUT] 2 IPv6 DNS addresses to be installed
    size_t          v6AddrSize,         ///< [IN]  Size of each IPv6 DNS addresses
    uint16_t        numAddresses        ///< [IN]  Number of addresses of each type
);

#endif
