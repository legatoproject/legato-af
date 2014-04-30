/*
 * ====================== WARNING ======================
 *
 * THE CONTENTS OF THIS FILE HAVE BEEN AUTO-GENERATED.
 * DO NOT MODIFY IN ANY WAY.
 *
 * ====================== WARNING ======================
 */


#ifndef MDC_INTERFACE_H_INCLUDE_GUARD
#define MDC_INTERFACE_H_INCLUDE_GUARD


#include "legato.h"

// User customizable include file
#include "userInclude.h"


//--------------------------------------------------------------------------------------------------
/**
 * Start the service for the client main thread
 */
//--------------------------------------------------------------------------------------------------
void le_mdc_StartClient
(
    const char* serviceInstanceName
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * Stop the service for the current client thread
 */
//--------------------------------------------------------------------------------------------------
void le_mdc_StopClient
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Reference type for le_mdc_SessionStateHandler handler ADD/REMOVE functions
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_mdc_SessionStateHandler* le_mdc_SessionStateHandlerRef_t;


//--------------------------------------------------------------------------------------------------
/**
 * This handler ...
 *
 * @param isConnected
 * @param contextPtr
 */
//--------------------------------------------------------------------------------------------------
typedef void (*le_mdc_SessionStateHandlerFunc_t)
(
    bool isConnected,
    void* contextPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * le_mdc_SessionStateHandler handler ADD function
 */
//--------------------------------------------------------------------------------------------------
le_mdc_SessionStateHandlerRef_t le_mdc_AddSessionStateHandler
(
    le_mdc_Profile_Ref_t profileRef,
        ///< [IN]
        ///< The profile object of interest

    le_mdc_SessionStateHandlerFunc_t handlerPtr,
        ///< [IN]

    void* contextPtr
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * le_mdc_SessionStateHandler handler REMOVE function
 */
//--------------------------------------------------------------------------------------------------
void le_mdc_RemoveSessionStateHandler
(
    le_mdc_SessionStateHandlerRef_t addHandlerRef
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
le_mdc_Profile_Ref_t le_mdc_LoadProfile
(
    const char* name
        ///< [IN]
        ///< Name of the profile.
);

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
le_result_t le_mdc_GetProfileName
(
    le_mdc_Profile_Ref_t profileRef,
        ///< [IN]
        ///< Query this profile object

    char* name,
        ///< [OUT]
        ///< The name of the profile

    size_t nameNumElements
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
le_result_t le_mdc_StartSession
(
    le_mdc_Profile_Ref_t profileRef
        ///< [IN]
        ///< Start data session for this profile object
);

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
le_result_t le_mdc_StopSession
(
    le_mdc_Profile_Ref_t profileRef
        ///< [IN]
        ///< Stop data session for this profile object
);

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
le_result_t le_mdc_GetSessionState
(
    le_mdc_Profile_Ref_t profileRef,
        ///< [IN]
        ///< Query this profile object

    bool* isConnectedPtr
        ///< [OUT]
        ///< The data session state
);

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
le_result_t le_mdc_GetInterfaceName
(
    le_mdc_Profile_Ref_t profileRef,
        ///< [IN]
        ///< Query this profile object

    char* interfaceName,
        ///< [OUT]
        ///< The name of the network interface

    size_t interfaceNameNumElements
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
le_result_t le_mdc_GetIPAddress
(
    le_mdc_Profile_Ref_t profileRef,
        ///< [IN]
        ///< Query this profile object

    char* ipAddr,
        ///< [OUT]
        ///< [OUT] The IP address in dotted format

    size_t ipAddrNumElements
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
le_result_t le_mdc_GetGatewayAddress
(
    le_mdc_Profile_Ref_t profileRef,
        ///< [IN]
        ///< Query this profile object

    char* gatewayAddr,
        ///< [OUT]
        ///< The gateway IP address in dotted format

    size_t gatewayAddrNumElements
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
le_result_t le_mdc_GetDNSAddresses
(
    le_mdc_Profile_Ref_t profileRef,
        ///< [IN]
        ///< Query this profile object

    char* dns1AddrStr,
        ///< [OUT]
        ///< The primary DNS IP address in dotted format

    size_t dns1AddrStrNumElements,
        ///< [IN]

    char* dns2AddrStr,
        ///< [OUT]
        ///< The secondary DNS IP address in dotted format

    size_t dns2AddrStrNumElements
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
bool le_mdc_IsIPV4
(
    le_mdc_Profile_Ref_t profileRef
        ///< [IN]
        ///< Query this profile object
);

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
bool le_mdc_IsIPV6
(
    le_mdc_Profile_Ref_t profileRef
        ///< [IN]
        ///< Query this profile object
);

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
le_result_t le_mdc_GetAccessPointName
(
    le_mdc_Profile_Ref_t profileRef,
        ///< [IN]
        ///< Query this profile object

    char* apnNameStr,
        ///< [OUT]
        ///< The Access Point Name

    size_t apnNameStrNumElements
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
le_result_t le_mdc_GetDataBearerTechnology
(
    le_mdc_Profile_Ref_t profileRef,
        ///< [IN]
        ///< Query this profile object

    le_mdc_dataBearerTechnology_t* dataBearerTechnologyPtrPtr
        ///< [OUT]
        ///< The data bearer technology
);

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
le_result_t le_mdc_GetBytesCounters
(
    uint64_t* rxBytesPtr,
        ///< [OUT]
        ///< bytes amount received since the last counter reset

    uint64_t* txBytesPtr
        ///< [OUT]
        ///< bytes amount transmitted since the last counter reset
);

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
le_result_t le_mdc_ResetBytesCounter
(
    void
);


#endif // MDC_INTERFACE_H_INCLUDE_GUARD

