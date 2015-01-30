/**
 * @file pa_mdc_simu.c
 *
 * Simulation implementation of @ref c_pa_mdc API.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 *
 */

#include "legato.h"
#include "pa_mdc.h"
#include "pa_simu.h"

#include <arpa/inet.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <unistd.h>

#include <linux/netlink.h>
#include <linux/rtnetlink.h>

//--------------------------------------------------------------------------------------------------
/**
 * Type to store profile index as call context.
 */
//--------------------------------------------------------------------------------------------------
typedef uintptr_t CallContext_t;

static pa_mdc_ProfileData_t Profiles[PA_MDC_MAX_PROFILE];
static int ConnectedProfileIndex = -1;

static pa_mdc_PktStatistics_t PktStatisticsOrig = {0};

//--------------------------------------------------------------------------------------------------
/**
 * This event is reported when a data session state change is received from the modem.  The
 * report data is allocated from the associated pool.   Only one event handler is allowed to be
 * registered at a time, so its reference is stored, in case it needs to be removed later.
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t NewSessionStateEvent;
static le_mem_PoolRef_t NewSessionStatePool;
static le_event_HandlerRef_t NewSessionStateHandlerRef = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Check if given profile is connected.
 */
//--------------------------------------------------------------------------------------------------
static bool IsProfileConnected
(
    uint32_t profileIndex
)
{
    return (ConnectedProfileIndex == profileIndex);
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if given profile index is a valid index.
 */
//--------------------------------------------------------------------------------------------------
static bool IsProfileIndexValid
(
    uint32_t profileIndex
)
{
    return ( (profileIndex != 0) && (profileIndex <= PA_MDC_MAX_PROFILE) );
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the index of the default profile (link to the platform)
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_GetDefaultProfileIndex
(
    uint32_t* profileIndexPtr
)
{
    *profileIndexPtr = 1;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the profile data for the given profile
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_POSSIBLE on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_ReadProfile
(
    uint32_t profileIndex,                  ///< [IN] The profile to read
    pa_mdc_ProfileData_t* profileDataPtr    ///< [OUT] The profile data
)
{
    if( !IsProfileIndexValid(profileIndex) )
    {
        return LE_NOT_POSSIBLE;
    }

    memcpy(profileDataPtr, &Profiles[profileIndex-1], sizeof(pa_mdc_ProfileData_t));
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Write the profile data for the given profile
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_POSSIBLE on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_WriteProfile
(
    uint32_t profileIndex,                  ///< [IN] The profile to write
    pa_mdc_ProfileData_t* profileDataPtr    ///< [IN] The profile data
)
{
    if( !IsProfileIndexValid(profileIndex) )
    {
        return LE_NOT_POSSIBLE;
    }

    memcpy(&Profiles[profileIndex-1], profileDataPtr, sizeof(pa_mdc_ProfileData_t));
    return LE_OK;
}

static void ReportNewState
(
    uint32_t profileIndex,
    pa_mdc_SessionState_t newState
)
{
    pa_mdc_SessionStateData_t* sessionStateDataPtr;

    // Init the data for the event report
    sessionStateDataPtr = le_mem_ForceAlloc(NewSessionStatePool);
    sessionStateDataPtr->profileIndex = profileIndex;
    sessionStateDataPtr->newState = newState;

    le_event_ReportWithRefCounting(NewSessionStateEvent, sessionStateDataPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Start a data session with the given profile using IPV4
 *
 * @return
 *      - LE_OK on success
 *      - LE_DUPLICATE if the data session is already connected
 *      - LE_NOT_POSSIBLE for other failures
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_StartSessionIPV4
(
    uint32_t profileIndex,          ///< [IN] The profile to use
    pa_mdc_CallRef_t* callRefPtr    ///< [OUT] Reference used for stopping the data session
)
{
    CallContext_t * callContextPtr = (CallContext_t *)callRefPtr;

    if( !IsProfileIndexValid(profileIndex) )
    {
        return LE_NOT_POSSIBLE;
    }

    LE_DEBUG("Start Profile %u: APN[%s]", profileIndex, Profiles[profileIndex-1].apn);

    /* Check APN */
    if( strncmp( Profiles[profileIndex-1].apn, PA_SIMU_MDC_DEFAULT_APN, PA_MDC_APN_MAX_LEN) != 0 )
    {
        LE_WARN("Bad APN '%s', expected '%s'", Profiles[profileIndex-1].apn, PA_SIMU_MDC_DEFAULT_APN);
        return LE_NOT_POSSIBLE;
    }

    /* Check duplicate */
    if(ConnectedProfileIndex == profileIndex)
    {
        LE_WARN("Already connected ! (index=%u)", ConnectedProfileIndex);
        return LE_DUPLICATE;
    }

    if(!mrc_simu_IsOnline())
    {
        LE_WARN("Not going online because network is offline.");
        return LE_NOT_POSSIBLE;
    }

    /* Connect */
    ConnectedProfileIndex = profileIndex;

    ReportNewState(profileIndex, PA_MDC_CONNECTED);

    *callContextPtr = profileIndex;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Start a data session with the given profile using IPV6
 *
 * @return
 *      - LE_OK on success
 *      - LE_DUPLICATE if the data session is already connected
 *      - LE_NOT_POSSIBLE for other failures
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_StartSessionIPV6
(
    uint32_t profileIndex,        ///< [IN] The profile to use
    pa_mdc_CallRef_t* callRefPtr  ///< [OUT] Reference used for stopping the data session
)
{
    return LE_NOT_POSSIBLE;
}

//--------------------------------------------------------------------------------------------------
/**
 * Start a data session with the given profile using IPV4-V6
 *
 * @return
 *      - LE_OK on success
 *      - LE_DUPLICATE if the data session is already connected
 *      - LE_NOT_POSSIBLE for other failures
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_StartSessionIPV4V6
(
    uint32_t profileIndex,        ///< [IN] The profile to use
    pa_mdc_CallRef_t* callRefPtr  ///< [OUT] Reference used for stopping the data session
)
{
    return LE_NOT_POSSIBLE;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get session type for the given profile ( IP V4 or V6 )
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_POSSIBLE for other failures
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_GetSessionType
(
    uint32_t profileIndex,              ///< [IN] The profile to use
    pa_mdc_SessionType_t* sessionIpPtr  ///< [OUT] IP family session
)
{
    if( !IsProfileIndexValid(profileIndex) || !IsProfileConnected(profileIndex) )
    {
        return LE_NOT_POSSIBLE;
    }

    *sessionIpPtr = PA_MDC_SESSION_IPV4;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Stop a data session for the given profile
 *
 * @return
 *      - LE_OK on success
 *      - LE_DUPLICATE if the data session has already been stopped (i.e. it is disconnected)
 *      - LE_NOT_POSSIBLE for other failures
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_StopSession
(
    pa_mdc_CallRef_t callRef         ///< [IN] The call reference returned when starting the sessions
)
{
    CallContext_t callContext = (CallContext_t)callRef;

    if(callRef == 0)
    {
        return LE_DUPLICATE;
    }

    if( !IsProfileIndexValid(callContext) )
    {
        return LE_NOT_POSSIBLE;
    }

    if( (-1 == ConnectedProfileIndex) ||
        (ConnectedProfileIndex != callContext) )
    {
        return LE_DUPLICATE;
    }

    ReportNewState(ConnectedProfileIndex, PA_MDC_DISCONNECTED);
    ConnectedProfileIndex =  -1;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the session state for the given profile
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_POSSIBLE on error
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_GetSessionState
(
    uint32_t profileIndex,                  ///< [IN] The profile to use
    pa_mdc_SessionState_t* sessionStatePtr  ///< [OUT] The data session state
)
{
    if( !IsProfileIndexValid(profileIndex) )
    {
        LE_WARN("Profile Index too high: %u", profileIndex);
        return LE_NOT_POSSIBLE;
    }

    if( IsProfileConnected(profileIndex) )
    {
        *sessionStatePtr = PA_MDC_CONNECTED;
    }
    else
    {
        *sessionStatePtr = PA_MDC_DISCONNECTED;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Register a handler for session state notifications.
 *
 * If the handler is NULL, then the previous handler will be removed.
 *
 * @note
 *      The process exits on failure
 */
//--------------------------------------------------------------------------------------------------
void pa_mdc_SetSessionStateHandler
(
    pa_mdc_SessionStateHandler_t handlerRef, ///< [IN] The session state handler function.
    void*                        contextPtr  ///< [IN] The context to be given to the handler.
)
{
    // Check if the old handler is replaced or deleted.
    if ( (NewSessionStateHandlerRef != NULL) || (handlerRef == NULL) )
    {
        LE_INFO("Clearing old handler");
        le_event_RemoveHandler(NewSessionStateHandlerRef);
        NewSessionStateHandlerRef = NULL;
    }

    // Check if new handler is being added
    if ( handlerRef != NULL )
    {
        NewSessionStateHandlerRef = le_event_AddHandler(
                                        "NewSessionStateHandler",
                                        NewSessionStateEvent,
                                        (le_event_HandlerFunc_t) handlerRef);

        le_event_SetContextPtr(NewSessionStateHandlerRef, contextPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the name of the network interface for the given profile, if the data session is connected.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the interface name would not fit in interfaceNameStr
 *      - LE_NOT_POSSIBLE for all other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_GetInterfaceName
(
    uint32_t profileIndex,                  ///< [IN] The profile to use
    char*  interfaceNameStr,                ///< [OUT] The name of the network interface
    size_t interfaceNameStrSize             ///< [IN] The size in bytes of the name buffer
)
{
    if ( !IsProfileConnected(profileIndex) )
    {
        return LE_NOT_POSSIBLE;
    }

    return le_utf8_Copy(interfaceNameStr, PA_SIMU_MDC_DEFAULT_IF, interfaceNameStrSize, NULL);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the IP address for the given profile, if the data session is connected.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the IP address would not fit in gatewayAddrStr
 *      - LE_NOT_POSSIBLE for all other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_GetIPAddress
(
    uint32_t profileIndex,             ///< [IN] The profile to use
    le_mdmDefs_IpVersion_t ipVersion,          ///< [IN] IP Version
    char*  ipAddrStr,                  ///< [OUT] The IP address in dotted format
    size_t ipAddrStrSize               ///< [IN] The size in bytes of the address buffer
)
{
    if ( !IsProfileConnected(profileIndex) )
    {
        return LE_NOT_POSSIBLE;
    }

    return le_utf8_Copy(ipAddrStr, PA_SIMU_MDC_DEFAULT_IP, ipAddrStrSize, NULL);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the gateway IP address for the given profile, if the data session is connected.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the IP address would not fit in gatewayAddrStr
 *      - LE_NOT_POSSIBLE for all other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_GetGatewayAddress
(
    uint32_t profileIndex,                  ///< [IN] The profile to use
    le_mdmDefs_IpVersion_t ipVersion,               ///< [IN] IP Version
    char*  gatewayAddrStr,                  ///< [OUT] The gateway IP address in dotted format
    size_t gatewayAddrStrSize               ///< [IN] The size in bytes of the address buffer
)
{
    if( !IsProfileIndexValid(profileIndex) |!IsProfileConnected(profileIndex) )
    {
        return LE_NOT_POSSIBLE;
    }

    return le_utf8_Copy(gatewayAddrStr, PA_SIMU_MDC_DEFAULT_GW, gatewayAddrStrSize, NULL);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the primary/secondary DNS addresses for the given profile, if the data session is connected.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the IP address would not fit in buffer
 *      - LE_NOT_POSSIBLE for all other errors
 *
 * @note
 *      If only one DNS address is available, then it will be returned, and an empty string will
 *      be returned for the unavailable address
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_GetDNSAddresses
(
    uint32_t profileIndex,                  ///< [IN] The profile to use
    le_mdmDefs_IpVersion_t ipVersion,               ///< [IN] IP Version
    char*  dns1AddrStr,                     ///< [OUT] The primary DNS IP address in dotted format
    size_t dns1AddrStrSize,                 ///< [IN] The size in bytes of the dns1AddrStr buffer
    char*  dns2AddrStr,                     ///< [OUT] The secondary DNS IP address in dotted format
    size_t dns2AddrStrSize                  ///< [IN] The size in bytes of the dns2AddrStr buffer
)
{
    if( !IsProfileIndexValid(profileIndex) |!IsProfileConnected(profileIndex) )
    {
        return LE_NOT_POSSIBLE;
    }

    le_utf8_Copy(dns1AddrStr, PA_SIMU_MDC_PRIMARY_DNS, dns1AddrStrSize, NULL);
    le_utf8_Copy(dns2AddrStr, PA_SIMU_MDC_SECONDARY_DNS, dns2AddrStrSize, NULL);
    return LE_OK;
}

#define NETLINK_MAX_PAYLOAD 1024 /* maximum payload size*/

//--------------------------------------------------------------------------------------------------
/**
 * Get data flow statistics since the last reset.
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_POSSIBLE for all other errors
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_GetDataFlowStatistics
(
    pa_mdc_PktStatistics_t *dataStatisticsPtr ///< [OUT] Statistics data
)
{
    int socketFd;
    ssize_t msgSize;
    struct sockaddr_nl srcAddr, destAddr;

    // Get statistics from interface using netlink

    socketFd = socket(PF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (socketFd < 0)
    {
        return LE_NOT_POSSIBLE;
    }

    /* Source: our process */
    memset(&srcAddr, 0, sizeof(srcAddr));
    srcAddr.nl_family = AF_NETLINK;
    srcAddr.nl_pid = getpid(); /* self pid */

    /* Dest: kernel */
    memset(&destAddr, 0, sizeof(destAddr));
    destAddr.nl_family = AF_NETLINK;

    /* Prepare request for RTM_GETLINK */
    {
        struct {
            struct nlmsghdr header;
            struct rtgenmsg genParam;
        } nlRequest;
        struct iovec ioRequest;
        struct msghdr messageHdr;

        memset(&nlRequest, 0, sizeof(nlRequest));
        memset(&ioRequest, 0, sizeof(ioRequest));
        memset(&messageHdr, 0, sizeof(messageHdr));

        nlRequest.header.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtgenmsg));
        nlRequest.header.nlmsg_type = RTM_GETLINK;
        nlRequest.header.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
        nlRequest.header.nlmsg_seq = 1;
        nlRequest.header.nlmsg_pid = getpid();
        nlRequest.genParam.rtgen_family = AF_PACKET; /*  no preferred AF, we will get *all* interfaces */

        ioRequest.iov_base = &nlRequest;
        ioRequest.iov_len = nlRequest.header.nlmsg_len;

        bind(socketFd, (struct sockaddr *)&srcAddr, sizeof(srcAddr));

        messageHdr.msg_iov = &ioRequest;
        messageHdr.msg_iovlen = 1;
        messageHdr.msg_name = &destAddr;
        messageHdr.msg_namelen = sizeof(destAddr);

        msgSize = sendmsg(socketFd, &messageHdr, 0);
        if(msgSize < 0)
        {
            LE_WARN("Error while sending netlink request: %d %s", errno, strerror(errno));
            close(socketFd);
            return LE_NOT_POSSIBLE;
        }
    }

    /* Handle response */
    {
        int len;
        char buffer[8192];
        struct nlmsghdr *msgHeaderPtr; /* pointer to current message part */

        struct msghdr messageHdr; /* generic msghdr structure for use with recvmsg */
        struct iovec ioResponse;

        memset(&ioResponse, 0, sizeof(ioResponse));
        memset(&messageHdr, 0, sizeof(messageHdr));

        ioResponse.iov_base = buffer;
        ioResponse.iov_len = sizeof(buffer);
        messageHdr.msg_iov = &ioResponse;
        messageHdr.msg_iovlen = 1;
        messageHdr.msg_name = &destAddr;
        messageHdr.msg_namelen = sizeof(destAddr);

        len = recvmsg(socketFd, &messageHdr, 0); /* read as much data as fits in the receive buffer */
        if(len < 0)
        {
            LE_WARN("Error while receiving netlink response: %d %s", errno, strerror(errno));
            close(socketFd);
            return LE_NOT_POSSIBLE;
        }

        /* Loop over all messages in buffer */
        for (msgHeaderPtr = (struct nlmsghdr *)buffer; NLMSG_OK(msgHeaderPtr, len); msgHeaderPtr = NLMSG_NEXT(msgHeaderPtr, len))
        {
            switch(msgHeaderPtr->nlmsg_type)
            {
                case NLMSG_DONE:
                    break;

                case RTM_NEWLINK:
                {
                    struct ifinfomsg * ifInfoPtr;
                    struct rtattr * attrPtr;
                    int len;
                    const char * ifNamePtr = NULL;

                    ifInfoPtr = NLMSG_DATA(msgHeaderPtr);
                    len = msgHeaderPtr->nlmsg_len - NLMSG_LENGTH(sizeof(*ifInfoPtr));

                    /* Loop over all attributes */
                    for (attrPtr = IFLA_RTA(ifInfoPtr); RTA_OK(attrPtr, len); attrPtr = RTA_NEXT(attrPtr, len))
                    {
                        switch(attrPtr->rta_type)
                        {
                            case IFLA_IFNAME:
                                ifNamePtr = (const char *)RTA_DATA(attrPtr);
                                LE_DEBUG("Interface %d: name[%s]", ifInfoPtr->ifi_index, ifNamePtr);
                                break;
                            case IFLA_STATS:
                            {
                                struct rtnl_link_stats * statsPtr = (struct rtnl_link_stats *) RTA_DATA(attrPtr);
                                LE_DEBUG("Interface %d: rx_bytes[%u] tx_bytes[%u]",
                                    ifInfoPtr->ifi_index,
                                    statsPtr->rx_bytes,
                                    statsPtr->tx_bytes );

                                if( (ifNamePtr != NULL) && (strcmp(ifNamePtr, PA_SIMU_MDC_DEFAULT_IF) == 0) )
                                {
                                    dataStatisticsPtr->receivedBytesCount = statsPtr->rx_bytes - PktStatisticsOrig.receivedBytesCount;
                                    dataStatisticsPtr->transmittedBytesCount = statsPtr->tx_bytes - PktStatisticsOrig.transmittedBytesCount;

                                    close(socketFd);
                                    return LE_OK;
                                }
                                break;
                            }
                            default:
                                break;
                        }
                    }
                    break;
                }

                default:
                    LE_WARN("Not handling message of type %d", msgHeaderPtr->nlmsg_type);
                    break;
            }
        }
    }

    close(socketFd);
    return LE_NOT_POSSIBLE;
}

//--------------------------------------------------------------------------------------------------
/**
 * Reset data flow statistics
 *
 * * @return
 *      - LE_OK on success
 *      - LE_NOT_POSSIBLE for all other errors
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_ResetDataFlowStatistics
(
    void
)
{
    pa_mdc_PktStatistics_t diffOrig;

    if(pa_mdc_GetDataFlowStatistics(&diffOrig) != LE_OK)
    {
        return LE_NOT_POSSIBLE;
    }

    PktStatisticsOrig.receivedBytesCount += diffOrig.receivedBytesCount;
    PktStatisticsOrig.transmittedBytesCount += diffOrig.transmittedBytesCount;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the Access Point Name for the given profile, if the data session is connected.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the Access Point Name would not fit in apnNameStr
 *      - LE_NOT_POSSIBLE for all other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_GetAccessPointName
(
    uint32_t profileIndex,             ///< [IN] The profile to use
    char*  apnNameStr,                 ///< [OUT] The Access Point Name
    size_t apnNameStrSize              ///< [IN] The size in bytes of the address buffer
)
{
    return le_utf8_Copy(apnNameStr, PA_SIMU_MDC_DEFAULT_APN, apnNameStrSize, NULL);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the Data Bearer Technology for the given profile, if the data session is connected.
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_POSSIBLE for all other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_GetDataBearerTechnology
(
    uint32_t                       profileIndex,              ///< [IN] The profile to use
    le_mdc_DataBearerTechnology_t* downlinkDataBearerTechPtr, ///< [OUT] downlink data bearer technology
    le_mdc_DataBearerTechnology_t* uplinkDataBearerTechPtr    ///< [OUT] uplink data bearer technology
)
{
    if( !IsProfileIndexValid(profileIndex) |!IsProfileConnected(profileIndex) )
    {
        return LE_NOT_POSSIBLE;
    }

    *downlinkDataBearerTechPtr = LE_MDC_DATA_BEARER_TECHNOLOGY_LTE;
    *uplinkDataBearerTechPtr = LE_MDC_DATA_BEARER_TECHNOLOGY_LTE;

    return LE_OK;
}

le_result_t mdc_simu_Init
(
    void
)
{
    NewSessionStateEvent = le_event_CreateIdWithRefCounting("NewSessionStateEvent");
    NewSessionStatePool = le_mem_CreatePool("NewSessionStatePool", sizeof(pa_mdc_SessionStateData_t));

    return LE_OK;
}
