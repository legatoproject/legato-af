/**
 * @page c_pa_mdc Modem Data Control Platform Adapter API
 *
 * @ref pa_mdc.h "Click here for the API reference documentation."
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
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

//--------------------------------------------------------------------------------------------------
/** @file pa_mdc.h
 *
 * Legato @ref c_pa_mdc include file.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#ifndef LEGATO_PA_MDC_INCLUDE_GUARD
#define LEGATO_PA_MDC_INCLUDE_GUARD

#include "legato.h"
#include "le_mdm_defs.h"



//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of profile objects supported
 */
//--------------------------------------------------------------------------------------------------
#define PA_MDC_MAX_PROFILE 5


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
#define PA_MDC_APN_MAX_LEN 100

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
#define PA_MDC_USERNAME_MAX_LEN 64

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
#define PA_MDC_PWD_MAX_LEN 100

//--------------------------------------------------------------------------------------------------
/**
 * Define the maximum length for an password null-ended string
 */
//--------------------------------------------------------------------------------------------------
#define PA_MDC_PWD_MAX_BYTES (PA_MDC_PWD_MAX_LEN+1)

//--------------------------------------------------------------------------------------------------
/**
 * Enumerates the possible values for the authentication type
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    PA_MDC_AUTH_NONE,    ///< no authentication
    PA_MDC_AUTH_PAP,     ///< PAP protocol
    PA_MDC_AUTH_CHAP,    ///< CHAP protocol
}
 pa_mdc_AuthType_t;

//--------------------------------------------------------------------------------------------------
/**
 * Data Control Profile structure that contains modem specific profile data
 */
//--------------------------------------------------------------------------------------------------
typedef struct {
    pa_mdc_AuthType_t type;                   ///< Authentication using PAP
    char userName[PA_MDC_USERNAME_MAX_BYTES]; ///< UserName used by authentication
    char password[PA_MDC_APN_MAX_BYTES]; ///< Password used by authentication
}
pa_mdc_Authentication_t;

//--------------------------------------------------------------------------------------------------
/**
 * Session IP family
 */
//--------------------------------------------------------------------------------------------------
typedef enum {
    PA_MDC_SESSION_IPV4=0,  ///< IP V4
    PA_MDC_SESSION_IPV6     ///< IP V6
}
pa_mdc_SessionType_t;

//--------------------------------------------------------------------------------------------------
/**
 * Data PDP Type
 */
//--------------------------------------------------------------------------------------------------
typedef enum {
    PA_MDC_PDP_UNKNOWN = 0,
    PA_MDC_PDP_IPV4,
    PA_MDC_PDP_IPV6,
    PA_MDC_PDP_IPV4V6
}
pa_mdc_PdpType_t;

//--------------------------------------------------------------------------------------------------
/**
 * Data Control Profile structure that contains modem specific profile data
 */
//--------------------------------------------------------------------------------------------------
typedef struct {
    char apn[PA_MDC_APN_MAX_BYTES]; ///< Access Point Name (APN)
    pa_mdc_Authentication_t authentication; ///< Authentication
    pa_mdc_PdpType_t pdp;           ///< PDP type
}
pa_mdc_ProfileData_t;


//--------------------------------------------------------------------------------------------------
/**
 * Enumerates the possible values for the data session state
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    PA_MDC_CONNECTED,       ///< Data session is connected
    PA_MDC_DISCONNECTED,    ///< Data session is disconnected
}
pa_mdc_SessionState_t;


//--------------------------------------------------------------------------------------------------
/**
 * Structure provided to session state handler
 */
//--------------------------------------------------------------------------------------------------
typedef struct {
    uint32_t profileIndex;              ///< Profile that had the state change
    pa_mdc_SessionState_t newState;     ///< New data session state
}
pa_mdc_SessionStateData_t;

//--------------------------------------------------------------------------------------------------
/**
 * Packet statistics structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct {
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
);


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
);


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
    uint32_t profileIndex,        ///< [IN] The profile to use
    uint32_t* callRefPtr          ///< [OUT] Reference used for stopping the data session
);

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
    uint32_t* callRefPtr          ///< [OUT] Reference used for stopping the data session
);

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
    uint32_t* callRefPtr          ///< [OUT] Reference used for stopping the data session
);


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
);

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
    uint32_t callRef         ///< [IN] The call reference returned when starting the sessions
);


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
void pa_mdc_SetSessionStateHandler
(
    pa_mdc_SessionStateHandler_t handlerRef ///< [IN] The session state handler function.
);


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
);


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
 *      - LE_NOT_POSSIBLE for all other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_GetGatewayAddress
(
    uint32_t profileIndex,                  ///< [IN] The profile to use
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
 *      - LE_NOT_POSSIBLE for all other errors
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
 *      - LE_NOT_POSSIBLE for all other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_GetDataBearerTechnology
(
    uint32_t profileIndex,                                 ///< [IN] The profile to use
    le_mdc_dataBearerTechnology_t* dataBearerTechnologyPtr ///< [OUT] The data bearer technology
);

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
);

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
);

#endif // LEGATO_PA_MDC_INCLUDE_GUARD

