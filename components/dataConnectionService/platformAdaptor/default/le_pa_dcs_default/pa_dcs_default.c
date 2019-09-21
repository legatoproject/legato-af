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

#include "legato.h"
#include "pa_dcs.h"

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
le_result_t pa_dcs_AskForIpAddress
(
    const char* interfaceStrPtr
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
}

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
le_result_t pa_dcs_SetDefaultGateway
(
    const char*  interfacePtr,   ///< [IN] Pointer on the interface name
    const char*  gatewayPtr,     ///< [IN] Pointer on the gateway name
    bool         isIpv6          ///< [IN] IPv6 or not
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Save the default route
 */
//--------------------------------------------------------------------------------------------------
void pa_dcs_GetDefaultGateway
(
    pa_dcs_DefaultGwBackup_t*  defGwConfigBackupPtr,
    le_result_t* v4Result,
    le_result_t* v6Result
)
{
    LE_ERROR("Unsupported function called");
    if (v4Result)
    {
        *v4Result = LE_FAULT;
    }
    if (v6Result)
    {
        *v6Result = LE_FAULT;
    }
}

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
le_result_t pa_dcs_ChangeRoute
(
    pa_dcs_RouteAction_t   routeAction,
    const char*            ipDestAddrStrPtr,
    const char*            ipDestMaskStrPtr,
    const char*            interfaceStrPtr
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Used the data backup upon connection to remove DNS entries locally added
 */
//--------------------------------------------------------------------------------------------------
void pa_dcs_RestoreInitialDnsNameServers
(
    pa_dcs_DnsBackup_t*  dnsConfigBackupPtr
)
{
    LE_ERROR("Unsupported function called");
}

//--------------------------------------------------------------------------------------------------
/**
 * Returns DHCP lease file location
 *
 * @return
 *      LE_OVERFLOW     Destination buffer too small and output will be truncated
 *      LE_UNSUPPORTED  If not supported by OS
 *      LE_FAULT        Function failed
 *      LE_OK           Function succeedv
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_dcs_GetDhcpLeaseFilePath
(
    const char*  interfaceStrPtr,   ///< [IN] Pointer on the interface name
    char*        pathPtr,           ///< [OUT] Output 1 pointer
    size_t       bufferSize         ///< [IN]  Size of buffer
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Add the provided DNS configurations into /etc/resolv.conf. An empty string in any of the 2
 * input arguments means that it has no DNS address to add in that field. And this function's
 * caller should have blocked the case in having both inputs empty.
 *
 * @return
 *      LE_FAULT        Function failed
 *      LE_OK           Function succeed
 *      LE_UNSUPPORTED  Function not supported by the target
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_dcs_SetDnsNameServers
(
    const char* dns1Ptr,    ///< [IN] Pointer to 1st DNS address; empty string means nothing to add
    const char* dns2Ptr,    ///< [IN] Pointer to 2nd DNS address; empty string means nothing to add
    bool* isDns1Added,      ///< [OUT] Whether the 1st DNS address is added or not
    bool* isDns2Added       ///< [OUT] Whether the 2nd DNS address is added or not
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
}

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
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
}

COMPONENT_INIT
{
}

