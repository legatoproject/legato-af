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
le_result_t pa_dcs_GetDefaultGateway
(
    pa_dcs_InterfaceDataBackup_t*  interfaceDataBackupPtr
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
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
    pa_dcs_InterfaceDataBackup_t*  interfaceDataBackupPtr
)
{
    LE_ERROR("Unsupported function called");
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the DNS configuration
 *
 * @return
 *      LE_FAULT        Function failed
 *      LE_OK           Function succeed
 *      LE_UNSUPPORTED  Function not supported by the target
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_dcs_SetDnsNameServers
(
    const char* dns1Ptr,    ///< [IN] Pointer on first DNS address
    const char* dns2Ptr     ///< [IN] Pointer on second DNS address
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
}

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
le_result_t pa_dcs_GetTimeWithTimeProtocol
(
    const char* serverStrPtr,       ///< [IN]  Time server
    pa_dcs_TimeStruct_t* timePtr    ///< [OUT] Time structure
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
}

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
le_result_t pa_dcs_GetTimeWithNetworkTimeProtocol
(
    const char* serverStrPtr,       ///< [IN]  Time server
    pa_dcs_TimeStruct_t* timePtr    ///< [OUT] Time structure
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
}

COMPONENT_INIT
{
}

