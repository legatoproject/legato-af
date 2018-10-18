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
 * Data associated to retrieve the state before the DCS started managing the default connection
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char defaultGateway[PA_DCS_IPV6_ADDR_MAX_BYTES];
    char defaultInterface[PA_DCS_INTERFACE_NAME_MAX_BYTES];
    char newDnsIPv4[2][PA_DCS_IPV4_ADDR_MAX_BYTES];
    char newDnsIPv6[2][PA_DCS_IPV6_ADDR_MAX_BYTES];
}
pa_dcs_InterfaceDataBackup_t;

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
 * Delete the current default gateway config on the system
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_dcs_DeleteDefaultGateway
(
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
LE_SHARED le_result_t pa_dcs_GetDefaultGateway
(
    pa_dcs_InterfaceDataBackup_t*  interfaceDataBackupPtr
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
    pa_dcs_InterfaceDataBackup_t*  interfaceDataBackupPtr
);

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
LE_SHARED le_result_t pa_dcs_SetDnsNameServers
(
    const char* dns1Ptr,    ///< [IN] Pointer on first DNS address
    const char* dns2Ptr     ///< [IN] Pointer on second DNS address
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
    bool *stateIsUp         ///< [OUT] interface state down/up as false/true
);

#endif
