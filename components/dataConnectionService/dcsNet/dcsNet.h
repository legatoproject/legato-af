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
LE_SHARED le_result_t le_net_GetNetIntfState
(
    const char *connIntf,
    bool *state
);

LE_SHARED le_result_t le_net_SetDefaultGW(le_dcs_ChannelRef_t channelRef);
LE_SHARED void le_net_BackupDefaultGW(void);
LE_SHARED le_result_t le_net_RestoreDefaultGW(void);
LE_SHARED le_result_t le_net_SetDNS(le_dcs_ChannelRef_t channelRef);
LE_SHARED void le_net_RestoreDNS(void);
LE_SHARED le_result_t le_net_ChangeRoute(le_dcs_ChannelRef_t channelRef, const char *destAddr,
                                         const char *destMask, bool isAdd);
LE_SHARED le_result_t le_net_GetLeaseAddresses(const char* interfaceStrPtr,
                                               le_net_DhcpInfoType_t infoType,
                                               char* v4AddrPtr, size_t v4AddrSize,
                                               char* v6AddrPtr, size_t v6AddrSize,
                                               uint16_t numAddresses);

#endif
