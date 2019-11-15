//--------------------------------------------------------------------------------------------------
/**
 * ThreadX Data Connection Service Adapter
 * Provides adapter for threadX specific functionality needed by dataConnectionService component
 *
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "pa_dcs.h"

//--------------------------------------------------------------------------------------------------
/**
 * Add the provided DNS configurations into dns resolution configuration file. An empty string in
 * any of the 2 input arguments means that it has no DNS address to add in that field. And this
 * function's caller should have blocked the case in having both inputs empty.
 *
 * @return
 *      LE_FAULT        Function failed
 *      LE_OK           Function succeed
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
    // To do
    LE_ERROR("Unsupported");
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is only needed for WiFi client which is NOT part
 * for now of any LWIP based platform we use. Besides, LWIP dhcp (Altair)
 * is disabled  and we will hold of using AT client until we really need it.
 *
 *
 * @return
 *      - LE_OK     Function successful
 *      - LE_FAULT  Function failed
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_dcs_AskForIpAddress
(
    const char*    interfaceStrPtr
)
{
    LE_ERROR("Unsupported");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Executes change route
 *
 * return
 *      LE_OK           Function succeed
 *      LE_FAULT        Function failed
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_dcs_ChangeRoute
(
    pa_dcs_RouteAction_t   routeAction,
    const char*            ipDestAddrStrPtr,
    const char*            prefixLengthPtr,
    const char*            interfaceStrPtr
)
{
    // To do
    LE_ERROR("Unsupported");
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the default gateway in the system
 *
 * return
 *      LE_OK           Function succeed
 *      LE_FAULT        Function failed
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_dcs_SetDefaultGateway
(
    const char* interfaceNamePtr,  ///< [IN] Pointer to the interface name
    const char* gatewayPtr,        ///< [IN] Pointer to the gateway name/address
    bool        isIpv6             ///< [IN] IPv6 or not
)
{
    // To do
    LE_ERROR("Unsupported");
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Save the default route
 */
//--------------------------------------------------------------------------------------------------
void pa_dcs_SaveDefaultGateway
(
    pa_dcs_DefaultGwBackup_t* defGwConfigBackupPtr
)
{
    // To do
    LE_ERROR("Unsupported");
}

//--------------------------------------------------------------------------------------------------
/**
 * Used the data backup upon connection to remove DNS entries locally added
 */
//--------------------------------------------------------------------------------------------------
void pa_dcs_RestoreInitialDnsNameServers
(
    pa_dcs_DnsBackup_t* dnsConfigBackupPtr
)
{
    // To do
    LE_ERROR("Unsupported");
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
    // Use TIME protocol to obtain time information is obsolete. Use NTP instead.
    return pa_dcs_GetTimeWithNetworkTimeProtocol(serverStrPtr, timePtr);
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
    struct timeval  presentTime;
    struct tm       brokenDownTime;

    // The SNTP client's servers are configured separately - there is no one-off request mechanism,
    // so here we just return the system time, which should be set from either the cellular network
    // or SNTP.
    //(void) serverStrPtr;
    if ((serverStrPtr == NULL) || (timePtr == NULL))
    {
        LE_ERROR("Invalid parameters");
        return LE_BAD_PARAMETER;
    }

    if (gettimeofday(&presentTime, NULL) != 0 ||
        gmtime_r(&presentTime.tv_sec, &brokenDownTime) == NULL)
    {
        LE_ERROR("Failed to obtain UTC time");
        return LE_FAULT;
    }

    timePtr->msec = presentTime.tv_usec / 1000L;
    timePtr->sec = brokenDownTime.tm_sec;
    timePtr->min = brokenDownTime.tm_min;
    timePtr->hour = brokenDownTime.tm_hour;
    timePtr->day = brokenDownTime.tm_mday;
    timePtr->mon = brokenDownTime.tm_mon + 1;       // tm_mon range is 0-11, mon range is 1-12
    timePtr->year = brokenDownTime.tm_year + 1900;  // tm_year is measured from 1900

    LE_DEBUG("year %d, mon %d, day %d, hour %d, min %d, sec %d, sec %d, msec %d", timePtr->year,
             timePtr->mon, timePtr->day, timePtr->hour, timePtr->min, timePtr->sec,
             timePtr->msec);
    return LE_OK;
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
le_result_t pa_dcs_GetInterfaceState
(
    const char *interfacePtr,  ///< [IN] network interface name
    bool *ipv4IsUpPtr,         ///< [INOUT] IPV4 is not assigned/assigned as false/true
    bool *ipv6IsUpPtr          ///< [INOUT] IPV6 is not assigned/assigned as false/true
)
{
    LE_UNUSED(interfacePtr);
    LE_UNUSED(ipv4IsUpPtr);
    LE_UNUSED(ipv6IsUpPtr);
    LE_ERROR("Unsupported");
    return LE_UNSUPPORTED;
}

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
)
{
    LE_UNUSED(interfaceStrPtr);
    LE_UNUSED(pathPtr);
    LE_UNUSED(bufferSize);
    LE_ERROR("Unsupported");
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Save the current default route or GW address setting on the system into the input data structure
 * provided, as well as the interface on which it is set, including both IPv4 and IPv6
 */
//--------------------------------------------------------------------------------------------------
void pa_dcs_GetDefaultGateway
(
    pa_dcs_DefaultGwBackup_t* defGwConfigBackupPtr,
    le_result_t* v4Result,
    le_result_t* v6Result
)
{
    LE_ERROR("Unsupported");
}

//--------------------------------------------------------------------------------------------------
/**
 * Component initialization
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
}
