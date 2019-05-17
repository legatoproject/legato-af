/**
 * @page c_pa_mdc Modem Data Control Platform Adapter API
 *
 * @ref pa_mdc.h "API Reference"
 *
 * <HR>
 *
 * @section pa_mdc_toc Table of Contents
 *
 *  - @ref pa_mdc_intro
 *  - @ref pa_mdc_rational
 *
 *
 * @section pa_mdc_intro Introduction
 *
 * These APIs are on the top of the platform-dependent adapter layer. They are independent of the
 * implementation. They guarantee the portability on different kind of platform without any changes
 * for the components developped upon these APIs.
 *
 *
 * @section pa_mdc_rational Rational
 * These functions are all blocking functions, so that they return when the modem has answered or
 * when a timeout has occured due to an interrupted communication with the modem.
 *
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc.
 */


/** @file pa_mdc.h
 *
 * Legato @ref c_pa_mdc include file.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_PA_MDC_INCLUDE_GUARD
#define LEGATO_PA_MDC_INCLUDE_GUARD

#include "legato.h"
#include "interfaces.h"


//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of profile objects supported
 */
//--------------------------------------------------------------------------------------------------
#if defined (PDP_MAX_PROFILE)
#define PA_MDC_MAX_PROFILE PDP_MAX_PROFILE
#else
#define PA_MDC_MAX_PROFILE 5
#endif


//--------------------------------------------------------------------------------------------------
/**
 * Minimum index value supported for 3GPP profile
 */
//--------------------------------------------------------------------------------------------------
#define PA_MDC_MIN_INDEX_3GPP_PROFILE 1

//--------------------------------------------------------------------------------------------------
/**
 * Maximum index value supported for 3GPP profile
 */
//--------------------------------------------------------------------------------------------------
#define PA_MDC_MAX_INDEX_3GPP_PROFILE 16

//--------------------------------------------------------------------------------------------------
/**
 * Minimum index value supported for 3GPP2 profile
 */
//--------------------------------------------------------------------------------------------------
#define PA_MDC_MIN_INDEX_3GPP2_PROFILE 101

//--------------------------------------------------------------------------------------------------
/**
 * Maximum index value supported for 3GPP profile
 */
//--------------------------------------------------------------------------------------------------
#define PA_MDC_MAX_INDEX_3GPP2_PROFILE 107

//--------------------------------------------------------------------------------------------------
/**
 * Define the maximum size of various modem data profile related fields.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Define the maximum length for an APN entry
 *
 * @todo Find out the real maximum length for the APN.  QMI max length 150.
 */
//--------------------------------------------------------------------------------------------------
#define PA_MDC_APN_MAX_LEN LE_MDC_APN_NAME_MAX_LEN

//--------------------------------------------------------------------------------------------------
/**
 * Define the maximum length for an APN null-ended string
 */
//--------------------------------------------------------------------------------------------------
#define PA_MDC_APN_MAX_BYTES (PA_MDC_APN_MAX_LEN+1)

//--------------------------------------------------------------------------------------------------
/**
 * Define the maximum length for an userName entry
 *
 * @todo Find out the real maximum length for the userName.
 */
//--------------------------------------------------------------------------------------------------
#define PA_MDC_USERNAME_MAX_LEN LE_MDC_USER_NAME_MAX_LEN

//--------------------------------------------------------------------------------------------------
/**
 * Define the maximum length for an userName null-ended string
 */
//--------------------------------------------------------------------------------------------------
#define PA_MDC_USERNAME_MAX_BYTES (PA_MDC_USERNAME_MAX_LEN+1)

//--------------------------------------------------------------------------------------------------
/**
 * Define the maximum length for a password entry
 *
 * @todo Find out the real maximum length for the password.
 */
//--------------------------------------------------------------------------------------------------
#define PA_MDC_PWD_MAX_LEN LE_MDC_PASSWORD_NAME_MAX_LEN

//--------------------------------------------------------------------------------------------------
/**
 * Define the maximum length for an password null-ended string
 */
//--------------------------------------------------------------------------------------------------
#define PA_MDC_PWD_MAX_BYTES (PA_MDC_PWD_MAX_LEN+1)

//--------------------------------------------------------------------------------------------------
/**
 * Authentication structure that contains modem specific profile authentication data
 */
//--------------------------------------------------------------------------------------------------
typedef struct {
    le_mdc_Auth_t type;                       ///< Authentication using PAP
    char userName[PA_MDC_USERNAME_MAX_BYTES]; ///< UserName used by authentication
    char password[PA_MDC_PWD_MAX_BYTES];      ///< Password used by authentication
}
pa_mdc_Authentication_t;

//--------------------------------------------------------------------------------------------------
/**
 * Session IP family
 */
//--------------------------------------------------------------------------------------------------
typedef enum {
    PA_MDC_SESSION_IPV4=0,  ///< IP V4
    PA_MDC_SESSION_IPV6,    ///< IP V6
    PA_MDC_SESSION_IPV4V6,  ///< IP V4-V6
}
pa_mdc_SessionType_t;

//--------------------------------------------------------------------------------------------------
/**
 * Data Control Profile structure that contains modem specific profile data
 */
//--------------------------------------------------------------------------------------------------
typedef struct {
    char apn[PA_MDC_APN_MAX_BYTES];         ///< Access Point Name (APN)
    pa_mdc_Authentication_t authentication; ///< Authentication
    le_mdc_Pdp_t pdp;                       ///< PDP type
}
pa_mdc_ProfileData_t;

//--------------------------------------------------------------------------------------------------
/**
 * Structure that provides the session state handler
 */
//--------------------------------------------------------------------------------------------------
typedef struct {
    uint32_t profileIndex;              ///< Profile that had the state change
    le_mdc_ConState_t newState;         ///< Data session connection status.
    le_mdc_Pdp_t pdp;                   ///< PDP type
    le_mdc_DisconnectionReason_t disc;  ///< Disconnection reason
    int32_t  discCode;                  ///< Platform specific disconnection code
}
pa_mdc_SessionStateData_t;


//--------------------------------------------------------------------------------------------------
/**
 * Structure that provides the connection failure codes.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_mdc_DisconnectionReason_t callEndFailure;  ///< Reason the call ended
    int32_t callEndFailureCode;                   ///< Platform specific Reason the call ended code
    int32_t callConnectionFailureType;            ///< Platform specific connection failure type
    int32_t callConnectionFailureCode;            ///< Platform specific connection failure code
}
pa_mdc_ConnectionFailureCode_t;

//--------------------------------------------------------------------------------------------------
/**
 * Packet statistics structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint64_t    transmittedBytesCount;  ///< Number of bytes transmitted without error.
    uint64_t    receivedBytesCount;     ///< Number of bytes received without error.
}
pa_mdc_PktStatistics_t;

//--------------------------------------------------------------------------------------------------
/**
 * Prototype for data session state handler function.
 *
 * This handler will receive reports of any changes to the data session state.
 *
 * @param
 *      sessionStatePtr The session state. Must be freed using @ref le_mem_Release() when done.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*pa_mdc_SessionStateHandler_t)
(
    pa_mdc_SessionStateData_t* sessionStatePtr
);

//--------------------------------------------------------------------------------------------------
// APIs.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Get the index of the default profile (link to the platform)
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_mdc_GetDefaultProfileIndex
(
    uint32_t* profileIndexPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the index of the default profile for Bearer Independent Protocol (link to the platform)
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_mdc_GetBipDefaultProfileIndex
(
    uint32_t* profileIndexPtr   ///< [OUT] index of the profile.
);

//--------------------------------------------------------------------------------------------------
/**
 * Read the profile data for the given profile
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_mdc_ReadProfile
(
    uint32_t profileIndex,                  ///< [IN] The profile to read
    pa_mdc_ProfileData_t* profileDataPtr    ///< [OUT] The profile data
);

//--------------------------------------------------------------------------------------------------
/**
 * Check whether the profile already exists on the modem ; if not, ask to the modem to create a new
 * profile.
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_mdc_InitializeProfile
(
    uint32_t   profileIndex     ///< [IN] The profile to write
);

//--------------------------------------------------------------------------------------------------
/**
 * Write the profile data for the given profile
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_mdc_WriteProfile
(
    uint32_t profileIndex,                  ///< [IN] The profile to write
    pa_mdc_ProfileData_t* profileDataPtr    ///< [IN] The profile data
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the connection failure reason
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void pa_mdc_GetConnectionFailureReason
(
    uint32_t profileIndex,              ///< [IN] The profile to use
    pa_mdc_ConnectionFailureCode_t** failureCodesPtr  ///< [OUT] The specific Failure Reason codes
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the connection failure reason for IPv4v6 mode
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void pa_mdc_GetConnectionFailureReasonExt
(
    uint32_t profileIndex,                           ///< [IN] The profile to use
    le_mdc_Pdp_t pdp,                                ///< [IN] The failure reason pdp type
    pa_mdc_ConnectionFailureCode_t** failureCodesPtr  ///< [OUT] The specific Failure Reason codes
);

//--------------------------------------------------------------------------------------------------
/**
 * Start a data session with the given profile using IPV4
 *
 * @return
 *      - LE_OK on success
 *      - LE_DUPLICATE if the data session is already connected
 *      - LE_FAULT for other failures
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_mdc_StartSessionIPV4
(
    uint32_t profileIndex          ///< [IN] The profile to use
);

//--------------------------------------------------------------------------------------------------
/**
 * Start a data session with the given profile using IPV6
 *
 * @return
 *      - LE_OK on success
 *      - LE_DUPLICATE if the data session is already connected
 *      - LE_FAULT for other failures
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_mdc_StartSessionIPV6
(
    uint32_t profileIndex        ///< [IN] The profile to use
);

//--------------------------------------------------------------------------------------------------
/**
 * Start a data session with the given profile using IPV4-V6
 *
 * @return
 *      - LE_OK on success
 *      - LE_DUPLICATE if the data session is already connected
 *      - LE_FAULT for other failures
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED LE_SHARED le_result_t pa_mdc_StartSessionIPV4V6
(
    uint32_t profileIndex        ///< [IN] The profile to use
);

//--------------------------------------------------------------------------------------------------
/**
 * Get session type for the given profile ( IP V4 or V6 )
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT for other failures
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_mdc_GetSessionType
(
    uint32_t profileIndex,              ///< [IN] The profile to use
    pa_mdc_SessionType_t* sessionIpPtr  ///< [OUT] IP family session
);

//--------------------------------------------------------------------------------------------------
/**
 * Stop a data session for the given profile
 *
 * @return
 *      - LE_OK on success
 *      - LE_BAD_PARAMETER if the input parameter is not valid
 *      - LE_FAULT for other failures
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_mdc_StopSession
(
    uint32_t profileIndex              ///< [IN] The profile to use
);

//--------------------------------------------------------------------------------------------------
/**
 * Reject a MT-PDP data session for the given profile
 *
 * @return
 *      - LE_OK on success
 *      - LE_UNSUPPORTED if not supported by the target
 *      - LE_FAULT for other failures
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_mdc_RejectMtPdpSession
(
    uint32_t profileIndex             ///< [IN] The profile to use
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the session state for the given profile
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_mdc_GetSessionState
(
    uint32_t profileIndex,                  ///< [IN] The profile to use
    le_mdc_ConState_t* sessionStatePtr      ///< [OUT] The data session state
);

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
LE_SHARED le_event_HandlerRef_t pa_mdc_AddSessionStateHandler
(
    pa_mdc_SessionStateHandler_t handlerRef, ///< [IN] The session state handler function.
    void*                        contextPtr  ///< [IN] The context to be given to the handler.

);

//--------------------------------------------------------------------------------------------------
/**
 * Get the name of the network interface for the given profile, if the data session is connected.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the interface name would not fit in interfaceNameStr
 *      - LE_FAULT for all other errors
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_mdc_GetInterfaceName
(
    uint32_t profileIndex,                  ///< [IN] The profile to use
    char*  interfaceNameStr,                ///< [OUT] The name of the network interface
    size_t interfaceNameStrSize             ///< [IN] The size in bytes of the name buffer
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the IP address for the given profile, if the data session is connected.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the IP address would not fit in gatewayAddrStr
 *      - LE_UNSUPPORTED if the IP version is unsupported
 *      - LE_FAULT for all other errors
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_mdc_GetIPAddress
(
    uint32_t profileIndex,             ///< [IN] The profile to use
    le_mdmDefs_IpVersion_t ipVersion,          ///< [IN] IP Version
    char*  ipAddrStr,                  ///< [OUT] The IP address in dotted format
    size_t ipAddrStrSize               ///< [IN] The size in bytes of the address buffer
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the gateway IP address for the given profile, if the data session is connected.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the IP address would not fit in gatewayAddrStr
 *      - LE_UNSUPPORTED if the IP version is unsupported
 *      - LE_FAULT for all other errors
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_mdc_GetGatewayAddress
(
    uint32_t profileIndex,                  ///< [IN] The profile to use
    le_mdmDefs_IpVersion_t ipVersion,               ///< [IN] IP Version
    char*  gatewayAddrStr,                  ///< [OUT] The gateway IP address in dotted format
    size_t gatewayAddrStrSize               ///< [IN] The size in bytes of the address buffer
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the primary/secondary DNS addresses for the given profile, if the data session is connected.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the IP address would not fit in buffer
 *      - LE_UNSUPPORTED if the IP version is unsupported
 *      - LE_FAULT for all other errors
 *
 * @note
 *      If only one DNS address is available, then it will be returned, and an empty string will
 *      be returned for the unavailable address
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_mdc_GetDNSAddresses
(
    uint32_t profileIndex,                  ///< [IN] The profile to use
    le_mdmDefs_IpVersion_t ipVersion,               ///< [IN] IP Version
    char*  dns1AddrStr,                     ///< [OUT] The primary DNS IP address in dotted format
    size_t dns1AddrStrSize,                 ///< [IN] The size in bytes of the dns1AddrStr buffer
    char*  dns2AddrStr,                     ///< [OUT] The secondary DNS IP address in dotted format
    size_t dns2AddrStrSize                  ///< [IN] The size in bytes of the dns2AddrStr buffer
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the Access Point Name for the given profile, if the data session is connected.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the Access Point Name would not fit in apnNameStr
 *      - LE_FAULT for all other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_GetAccessPointName
(
    uint32_t profileIndex,             ///< [IN] The profile to use
    char*  apnNameStr,                 ///< [OUT] The Access Point Name
    size_t apnNameStrSize              ///< [IN] The size in bytes of the address buffer
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the Data Bearer Technology for the given profile, if the data session is connected.
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT for all other errors
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_mdc_GetDataBearerTechnology
(
    uint32_t                       profileIndex,              ///< [IN] The profile to use
    le_mdc_DataBearerTechnology_t* downlinkDataBearerTechPtr, ///< [OUT] downlink data bearer technology
    le_mdc_DataBearerTechnology_t* uplinkDataBearerTechPtr    ///< [OUT] uplink data bearer technology
);

//--------------------------------------------------------------------------------------------------
/**
 * Get data flow statistics since the last reset.
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT for all other errors
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_mdc_GetDataFlowStatistics
(
    pa_mdc_PktStatistics_t *dataStatisticsPtr ///< [OUT] Statistics data
);

//--------------------------------------------------------------------------------------------------
/**
 * Reset data flow statistics
 *
 * * @return
 *      - LE_OK on success
 *      - LE_FAULT for all other errors
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_mdc_ResetDataFlowStatistics
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Stop collecting data flow statistics
 *
 * * @return
 *      - LE_OK on success
 *      - LE_FAULT for all other errors
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_mdc_StopDataFlowStatistics
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Start collecting data flow statistics
 *
 * * @return
 *      - LE_OK on success
 *      - LE_FAULT for all other errors
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_mdc_StartDataFlowStatistics
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Map a profile on a network interface
 *
 * * @return
 *      - LE_OK on success
 *      - LE_UNSUPPORTED if not supported by the target
 *      - LE_FAULT for all other errors
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_mdc_MapProfileOnNetworkInterface
(
    uint32_t         profileIndex,         ///< [IN] The profile to use
    const char*      interfaceNamePtr      ///< [IN] Network interface name
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the list of all profiles
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on failure
 *
 * ToDo: The output argument will soon be replaced after code integration with the new struct type
 *       support in .api
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_mdc_GetProfileList
(
    le_mdc_ProfileInfo_t *profileList, ///< [OUT] list of available profiles
    size_t *listSize                   ///< [INOUT] list size
);

#endif // LEGATO_PA_MDC_INCLUDE_GUARD

