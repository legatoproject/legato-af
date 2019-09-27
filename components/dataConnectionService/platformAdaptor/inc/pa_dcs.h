/**
 * @page pa_dcs Data Connection Service Adapter API
 *
 * @ref pa_dcs.h "API Reference"
 *
 * <HR>
 *
 * @section pa_dcs_toc Table of Contents
 *
 *  - @ref pa_dcs_intro
 *  - @ref pa_dcs_rational
 *
 *
 * @section pa_dcs_intro Introduction
 *
 * As Sierra Wireless is moving into supporting multiple OS platforms,
 * we need to abstract Data Connection Services layer
 *
 * @section pa_dcs_rational Rational
 * Up to now, only Linux OS was supported. Now, as support for RTOS
 * and other OSs is being made available, there is a need for
 * this kind of platform adapter
 *
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc.
 */


#ifndef LEGATO_PA_DCS_INCLUDE_GUARD
#define LEGATO_PA_DCS_INCLUDE_GUARD

#include "legato.h"
#include "interfaces.h"

#define PA_DCS_INTERFACE_NAME_MAX_BYTES 20
#define PA_DCS_IPV4_ADDR_MAX_BYTES      16
#define PA_DCS_IPV6_ADDR_MAX_BYTES      46

//--------------------------------------------------------------------------------------------------
/**
 * Enumeration for the routing actions
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    PA_DCS_ROUTE_ADD,      ///< Add a route
    PA_DCS_ROUTE_DELETE    ///< Delete a route
}
pa_dcs_RouteAction_t;

//--------------------------------------------------------------------------------------------------
/**
 * Data structures for backing up network configs:
 *     pa_dcs_DefaultGwBackup_t: default GW configs before change
 *     pa_dcs_DnsBackup_t: DNS configs being added in the change
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_msg_SessionRef_t appSessionRef;
    char defaultV4GW[PA_DCS_IPV4_ADDR_MAX_BYTES];
    char defaultV4Interface[PA_DCS_INTERFACE_NAME_MAX_BYTES];
    bool setV4GwToSystem;
    char defaultV6GW[PA_DCS_IPV6_ADDR_MAX_BYTES];
    char defaultV6Interface[PA_DCS_INTERFACE_NAME_MAX_BYTES];
    bool setV6GwToSystem;
}
pa_dcs_DefaultGwBackup_t;

typedef struct
{
    le_msg_SessionRef_t appSessionRef;
    char dnsIPv4[2][PA_DCS_IPV4_ADDR_MAX_BYTES];
    bool setDnsV4ToSystem[2];
    char dnsIPv6[2][PA_DCS_IPV6_ADDR_MAX_BYTES];
    bool setDnsV6ToSystem[2];
}
pa_dcs_DnsBackup_t;


//--------------------------------------------------------------------------------------------------
/**
 * Data associated to retrieve the time from a time server
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    int msec;   ///< Milliseconds [0-999]
    int sec;    ///< Seconds      [0-60]
    int min;    ///< Minutes      [0-59]
    int hour;   ///< Hours        [0-23]
    int day;    ///< Day          [1-31]
    int mon;    ///< Month        [1-12]
    int year;   ///< Year
}
pa_dcs_TimeStruct_t;

/*********************************************************
 *
 *     APIs
 *
 *********************************************************
 */

//--------------------------------------------------------------------------------------------------
/**
 * Ask For Ip Address
 *
 * @return
 *      - LE_OK on success
 *      - LE_UNSUPPORTED if not supported by the target
 *      - LE_FAULT for all other errors
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_dcs_AskForIpAddress
(
    const char* interfaceStrPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Stop DHCP running specific network interface
 *
 * @return
 *      - LE_OK             Function succeeded
 *      - LE_BAD_PARAMETER  Invalid parameter
 *      - LE_FAULT          Function failed
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_dcs_StopDhcp
(
    const char *interface   ///< [IN] Network interface name
);

//--------------------------------------------------------------------------------------------------
/**
 * Returns DHCP lease file location
 *
 * @return
 *      LE_OVERFLOW     Destination buffer too small and output will be truncated
 *      LE_UNSUPPORTED  If not supported by OS
 *      LE_FAULT        Function failed
 *      LE_OK           Function succeed
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_dcs_GetDhcpLeaseFilePath
(
    const char*  interfaceStrPtr,   ///< [IN] Pointer on the interface name
    char*        pathPtr,           ///< [OUT] Output 1 pointer
    size_t       bufferSize         ///< [IN]  Size of buffer
);

//--------------------------------------------------------------------------------------------------
/**
 * Set the default gateway in the system
 *
 * @return
 *      LE_OK           Function succeed
 *      LE_FAULT        Function failed
 *      LE_UNSUPPORTED  Function not supported by the target
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_dcs_SetDefaultGateway
(
    const char*  interfacePtr,   ///< [IN] Pointer on the interface name
    const char*  gatewayPtr,     ///< [IN] Pointer on the gateway name
    bool         isIpv6          ///< [IN] IPv6 or not
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the default route
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void pa_dcs_GetDefaultGateway
(
    pa_dcs_DefaultGwBackup_t*  defGwConfigBackupPtr,
    le_result_t* v4Result,
    le_result_t* v6Result
);

//--------------------------------------------------------------------------------------------------
/**
 * Executes change route
 *
 * @return
 *      LE_OK           Function succeed
 *      LE_FAULT        Function failed
 *      LE_UNSUPPORTED  Function not supported by the target
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_dcs_ChangeRoute
(
    pa_dcs_RouteAction_t   routeAction,
    const char*            ipDestAddrStrPtr,
    const char*            ipDestMaskStrPtr,
    const char*            interfaceStrPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Used the data backup upon connection to remove DNS entries locally added
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void pa_dcs_RestoreInitialDnsNameServers
(
    pa_dcs_DnsBackup_t*  dnsConfigBackupPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Add the provided DNS configurations into /etc/resolv.conf. An empty string in any of the 2
 * input arguments means that it has no DNS address to add in that field. And this function's
 * caller should have blocked the case in having both inputs empty.
 *
 * @return
 *      LE_OK           Function succeed
 *      LE_DUPLICATE    Function found no need to add as the given inputs are already set in
 *      LE_UNSUPPORTED  Function not supported by the target
 *      LE_FAULT        Function failed
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_dcs_SetDnsNameServers
(
    const char* dns1Ptr,    ///< [IN] Pointer to 1st DNS address; empty string means nothing to add
    const char* dns2Ptr,    ///< [IN] Pointer to 2nd DNS address; empty string means nothing to add
    bool* isDns1Added,      ///< [OUT] Whether the 1st DNS address is added or not
    bool* isDns2Added       ///< [OUT] Whether the 2nd DNS address is added or not
);

//--------------------------------------------------------------------------------------------------
/**
 * Retrieve time from a server using the Time Protocol.
 *
 * @return
 *      - LE_OK             Function successful
 *      - LE_BAD_PARAMETER  A parameter is incorrect
 *      - LE_FAULT          Function failed
 *      - LE_UNSUPPORTED    Function not supported by the target
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_dcs_GetTimeWithTimeProtocol
(
    const char* serverStrPtr,       ///< [IN]  Time server
    pa_dcs_TimeStruct_t* timePtr    ///< [OUT] Time structure
);

//--------------------------------------------------------------------------------------------------
/**
 * Retrieve time from a server using the Network Time Protocol.
 *
 * @return
 *      - LE_OK             Function successful
 *      - LE_BAD_PARAMETER  A parameter is incorrect
 *      - LE_FAULT          Function failed
 *      - LE_UNSUPPORTED    Function not supported by the target
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_dcs_GetTimeWithNetworkTimeProtocol
(
    const char* serverStrPtr,       ///< [IN]  Time server
    pa_dcs_TimeStruct_t* timePtr    ///< [OUT] Time structure
);


//--------------------------------------------------------------------------------------------------
/**
 * Query for a connection's network interface state
 *
 * @return
 *      - LE_OK             Function successful
 *      - LE_BAD_PARAMETER  A parameter is incorrect
 *      - LE_FAULT          Function failed
 *      - LE_UNSUPPORTED    Function not supported by the target
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_dcs_GetInterfaceState
(
    const char *interface,  ///< [IN] network interface name
    bool *ipv4IsUp,         ///< [INOUT] IPV4 is not assigned/assigned as false/true
    bool *ipv6IsUp          ///< [INOUT] IPV6 is not assigned/assigned as false/true
);

#endif
