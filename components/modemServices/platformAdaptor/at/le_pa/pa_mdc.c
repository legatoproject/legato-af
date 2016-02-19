/**
 * @file pa_mdc.c
 *
 * AT implementation of c_pa_mdc API.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 *
 */

#include "legato.h"

#include "pa.h"
#include "pa_mdc.h"
#include "pa_common_local.h"

#include "atManager/inc/atCmdSync.h"
#include "atManager/inc/atPorts.h"

#include <sys/ioctl.h>
#include <termios.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <arpa/inet.h>
#include <resolv.h>

static le_event_Id_t          InternalEventCall;

//--------------------------------------------------------------------------------------------------
/**
 * Define an invalid profile index. Since profile indices start at 1, 0 is an invalid index.
 */
//--------------------------------------------------------------------------------------------------
#define INVALID_PROFILE_INDEX 0

//--------------------------------------------------------------------------------------------------
/**
 * This event is reported when a data session state change is received from the modem.  The
 * report data is allocated from the associated pool.   Only one event handler is allowed to be
 * registered at a time, so its reference is stored, in case it needs to be removed later.
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t            UnsolicitedEvent;
static le_event_Id_t            NewSessionStateEvent;
static le_mem_PoolRef_t         NewSessionStatePool;
static le_event_HandlerRef_t    NewSessionStateHandlerRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * The modem currently only supports one data session at a time, but the API provides support for
 * more. Therefore, the profile index of the current data session needs to be stored.  This
 * would normally be set when the data session is started, and cleared when it is stopped.  This
 * profile could be either connected or disconnected; all other profiles are always disconnected.
 */
//--------------------------------------------------------------------------------------------------
static uint32_t CurrentDataSessionIndex=INVALID_PROFILE_INDEX;

//--------------------------------------------------------------------------------------------------
/**
 * Mutex used to protect access to CurrentDataSessionIndex.
 */
//--------------------------------------------------------------------------------------------------
static pthread_mutex_t Mutex = PTHREAD_MUTEX_INITIALIZER;   // POSIX "Fast" mutex.

/// Locks the mutex.
#define LOCK    LE_ASSERT(pthread_mutex_lock(&Mutex) == 0)

/// Unlocks the mutex.
#define UNLOCK  LE_ASSERT(pthread_mutex_unlock(&Mutex) == 0)

//--------------------------------------------------------------------------------------------------
/**
 * Get the current data session index, ensuring that access is protected by a mutex
 *
 * @return
 *      The index value
 */
//--------------------------------------------------------------------------------------------------
static inline uint32_t GetCurrentDataSessionIndex(void)
{
    uint32_t index;

    LOCK;
    index = CurrentDataSessionIndex;
    UNLOCK;

    return index;
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the current data session index, ensuring that access is protected by a mutex
 */
//--------------------------------------------------------------------------------------------------
static inline void SetCurrentDataSessionIndex(uint32_t index)
{
    LOCK;
    CurrentDataSessionIndex = index;
    UNLOCK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Attach or detach the GPRS service
 *
 * @return LE_OK            GPRS is attached
 * @return LE_FAULT         modem could not attach the GPRS
 */
//--------------------------------------------------------------------------------------------------
static le_result_t AttachGPRS
(
    bool toAttach   ///< [IN] boolean value
)
{
    char atcommand[ATCOMMAND_SIZE] ;

    atcmdsync_PrepareString(atcommand,ATCOMMAND_SIZE,"at+cgatt=%d",toAttach);

    return atcmdsync_SendStandard(atports_GetInterface(ATPORT_COMMAND),
                                  atcommand,
                                  NULL,
                                  NULL,
                                  30000);
}

//--------------------------------------------------------------------------------------------------
/**
 * Activate or Desactivate the profile regarding toActivate value
 *
 * @return LE_OK            Modem succeded to activate/desactivate the profile
 * @return LE_FAULT         Modem could not proceed to activate/desactivate the profile
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ActivateContext
(
    uint32_t profileIndex,    ///< [IN] The profile to read
    bool toActivate           ///< [IN] activation boolean
)
{
    char atcommand[ATCOMMAND_SIZE] ;

    atcmdsync_PrepareString(atcommand,ATCOMMAND_SIZE,"at+cgact=%d,%d",toActivate,profileIndex);

    return atcmdsync_SendStandard(atports_GetInterface(ATPORT_COMMAND),
                                  atcommand,
                                  NULL,
                                  NULL,
                                  30000);
}

//--------------------------------------------------------------------------------------------------
/**
 * Enable or disable GPRS Event reporting.
 *
 * @return LE_OK            GPRS event reporting is enable/disable
 * @return LE_FAULT         Modem could not enable/disable the GPRS Event reporting
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SetIndicationHandler
(
    uint32_t  mode  ///< Unsolicited result mode
)
{
    char atcommand[ATCOMMAND_SIZE] ;

    atcmdsync_PrepareString(atcommand,ATCOMMAND_SIZE,"at+cgerep=%d",mode);

    le_result_t result = atcmdsync_SendStandard(atports_GetInterface(ATPORT_COMMAND),
                                                atcommand,
                                                NULL,
                                                NULL,
                                                30000);
    if ( result == LE_OK )
    {
    if (mode) {
        atmgr_SubscribeUnsolReq(atports_GetInterface(ATPORT_COMMAND),
                                   UnsolicitedEvent,
                                   "+CGEV:",
                                   false);
    } else {
            atmgr_UnSubscribeUnsolReq(atports_GetInterface(ATPORT_COMMAND),UnsolicitedEvent,"+CGEV:");
        }
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * The handler for GPRS Event Notification.
 *
 */
//--------------------------------------------------------------------------------------------------
static void CGEVUnsolHandler
(
    void* reportPtr
)
{
    atmgr_UnsolResponse_t* unsolPtr = reportPtr;
    uint32_t  numParam=0;
    pa_mdc_SessionStateData_t *sessionStatePtr=NULL;

    LE_DEBUG("Handler received -%s-",unsolPtr->line);

    if ( ( FIND_STRING("+CGEV: NW DEACT", unsolPtr->line) )
        ||
         ( FIND_STRING("+CGEV: ME DEACT", unsolPtr->line) )
       )
    {
        numParam = atcmd_CountLineParameter(unsolPtr->line);

        if (numParam == 4) {
            sessionStatePtr = le_mem_ForceAlloc(NewSessionStatePool);
            sessionStatePtr->profileIndex = atoi(atcmd_GetLineParameter(unsolPtr->line,4));
            sessionStatePtr->newState = LE_MDC_DISCONNECTED;

            SetCurrentDataSessionIndex(INVALID_PROFILE_INDEX);

            LE_DEBUG("Send Event for %d with state %d",
                        sessionStatePtr->profileIndex,sessionStatePtr->newState);
            le_event_ReportWithRefCounting(NewSessionStateEvent,sessionStatePtr);
        } else {
            LE_WARN("this Response pattern is not expected -%s-",unsolPtr->line);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Set PPP port for ppp connection.
 *
 * @return LE_OK            Enable/disable the PPP port
 * @return LE_FAULT         Could not Enable/disable the PPP port
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SetPPPPort
(
    bool enable
)
{
    LE_DEBUG("PPP Port %s",(enable==true)?"Enable":"Disable");
    if ( enable ) {
        atmgr_StartInterface(atports_GetInterface(ATPORT_PPP));
    } else {
        atmgr_StopInterface(atports_GetInterface(ATPORT_PPP));
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Start the PDP Modem connection.
 *
 * @return LE_OK            Activate the profile in the modem
 * @return LE_FAULT         Could not activate the profile in the modem
 */
//--------------------------------------------------------------------------------------------------
static le_result_t StartPDPConnection
(
    uint32_t profileIndex    ///< [IN] The profile identifier
)
{
    char atcommand[ATCOMMAND_SIZE] ;
    atcmd_Ref_t atReqRef=NULL;
    atcmdsync_ResultRef_t  atresRef;
    const char* finalRespOkPtr[] = { "CONNECT" , NULL };
    const char* finalRespKoPtr[] = { "NO CARRIER", "TIMEOUT", NULL};

    atcmdsync_PrepareString(atcommand,ATCOMMAND_SIZE,"ATD*99***%d#",profileIndex);

    atReqRef = atcmdsync_PrepareStandardCommand(atcommand,
                                                    NULL,
                                                    finalRespOkPtr,
                                                    finalRespKoPtr,
                                                    30000);
    atresRef = atcmdsync_SendCommand(atports_GetInterface(ATPORT_PPP),atReqRef);
    le_result_t result = atcmdsync_CheckCommandResult(atresRef,finalRespOkPtr,finalRespKoPtr);

    le_mem_Release(atReqRef);
    le_mem_Release(atresRef);

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Hang up the PDP Modem connection.
 *
 * @return LE_OK            Activate the profile in the modem
 * @return LE_FAULT         Could not hang up the PDP connection
 * @return LE_TIMEOUT       Command failed with timeout
 */
//--------------------------------------------------------------------------------------------------
static le_result_t StopPDPConnection
(
)
{
    return atcmdsync_SendStandard(atports_GetInterface(ATPORT_COMMAND),
                                  "ATGH",
                                  NULL,
                                  NULL,
                                  30000);
}

//--------------------------------------------------------------------------------------------------
/**
 * Start the ppp interface.
 *
 * @return LE_OK            Activate the ppp interface with linux
 * @return LE_FAULT         Failed to activate the ppp interface with linux
 */
//--------------------------------------------------------------------------------------------------
static le_result_t StartPPPInterface
(
    void
)
{
    pid_t pid = fork();
    if ( pid == -1)
    {
        return LE_FAULT;
    } else if ( pid == 0) // child process
    {
        char* args[] = {
            "pppd",     /* argv[0], programme name. */
            "usepeerdns",
            "updetach",
            AT_PPP,
            NULL      /* list of argument must finished by NULL.  */
        };

        // warn: pppd 2.4.5 with this patch:
        // http://code.google.com/p/wl500g/source/browse/trunk/ppp/115-debian-always_setsid.patch
        // does not work.
        // the modem hangup when the process is forked.
        // So need to use a pppd version without this patch.
        execvp("pppd", args);
        return 99;

    } else {
        int statchild;
        wait(&statchild); // wait for child to finished
        if (WIFEXITED(statchild)) {
            LE_DEBUG("Child's exit code %d\n", WEXITSTATUS(statchild));
            if (WEXITSTATUS(statchild) == 0) {
                // Remove the NO CARRIER unsolicited
                atmgr_UnSubscribeUnsolReq (atports_GetInterface(ATPORT_COMMAND),InternalEventCall,"NO CARRIER");

                return LE_OK;
            } else
            {
                return LE_FAULT;
            }
        }
        else {
            LE_WARN("Child did not terminate with exit\n");
            return LE_FAULT;
        }

    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Establish the connection.
 *  - ask the PDP connection to start on Modem
 *  - start a PPP connection to link with the Modem PPP Server
 *
 * @return LE_OK            The connection is established
 * @return LE_FAULT         could not establish connection for the profile
 */
//--------------------------------------------------------------------------------------------------
static le_result_t EstablishConnection
(
    uint32_t profileIndex    ///< [IN] The profile identifier
)
{
    if ( SetPPPPort(true) != LE_OK )
    {
        return LE_FAULT;
    }

    // Start the PDP connection on Modem side
    if ( StartPDPConnection(profileIndex) != LE_OK )
    {
        return LE_FAULT;
    }

    // Start the PPP connection on application side
    if ( StartPPPInterface() != LE_OK)
    {
        return LE_FAULT;
    }

    if ( SetPPPPort(false) != LE_OK )
    {
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register a handler for a ppp call.
 *
 *
 */
//--------------------------------------------------------------------------------------------------
static void MDCInternalHandler
(
    void* reportPtr
)
{
    atmgr_UnsolResponse_t* unsolPtr = reportPtr;

    LE_DEBUG("Handler received -%s-",unsolPtr->line);

    if ( FIND_STRING("NO CARRIER", unsolPtr->line) )
    {
        SetCurrentDataSessionIndex(INVALID_PROFILE_INDEX);
        atmgr_UnSubscribeUnsolReq(atports_GetInterface(ATPORT_COMMAND),InternalEventCall,"NO CARRIER");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the mdc module
 *
 *
 * @return LE_FAULT         The function failed to initialize the module.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_Init
(
    void
)
{
    if (atports_GetInterface(ATPORT_COMMAND)==NULL) {
        LE_WARN("DATA Module is not initialize in this session");
        return LE_FAULT;
    }

    if (atports_GetInterface(ATPORT_PPP)==false) {
        LE_WARN("PPP Module is not initialize in this session");
        return LE_FAULT;
    }

    NewSessionStateEvent = le_event_CreateIdWithRefCounting("NewSessionStateEvent");
    UnsolicitedEvent     = le_event_CreateId("SessionUnsolicitedEvent",sizeof(atmgr_UnsolResponse_t));
    NewSessionStatePool  = le_mem_CreatePool("NewSessionStatePool", sizeof(pa_mdc_SessionStateData_t));

    InternalEventCall = le_event_CreateId("MDCInternalEventCall",sizeof(atmgr_UnsolResponse_t));
    le_event_AddHandler("MDCUnsolHandler",InternalEventCall  ,MDCInternalHandler);

    // set unsolicited +CGEV to Register our own handler.
    SetIndicationHandler(2);

    le_event_AddHandler("MDCUnsolHandler",UnsolicitedEvent  ,CGEVUnsolHandler);

    return LE_OK;
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
 * Get the index of the default profile for Bearer Independent Protocol (link to the platform)
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_GetBipDefaultProfileIndex
(
    uint32_t* profileIndexPtr   ///< [OUT] index of the profile.
)
{
    *profileIndexPtr = 2;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the profile data for the given profile
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_ReadProfile
(
    uint32_t profileIndex,                    ///< [IN] The profile to read
    pa_mdc_ProfileData_t* profileDataPtr    ///< [OUT] The profile data
)
{
    le_result_t result = LE_FAULT;
    char atintermediate[ATCOMMAND_SIZE];

    atcmdsync_PrepareString(atintermediate,ATCOMMAND_SIZE,"+CGDCONT: %d,",profileIndex);

    const char* interRespPtr[] = {atintermediate,NULL};
    atcmdsync_ResultRef_t  atRespPtr = NULL;

    result = atcmdsync_SendStandard(atports_GetInterface(ATPORT_COMMAND),
                                    "at+cgdcont?",
                                    &atRespPtr,
                                    interRespPtr,
                                    30000);

    if ( result != LE_OK )
    {
        le_mem_Release(atRespPtr);     // Release atcmdsync_SendCommandDefaultExt
        return result;
    }

    // If there is more than one line then it mean that the command is OK so the first line is
    // the intermediate one
    if (atcmdsync_GetNumLines(atRespPtr) == 2)
    {
        // it parse just the first line because of '\0'
        char* line = atcmdsync_GetLine(atRespPtr,0);
        uint32_t numParam = atcmd_CountLineParameter(line);
        // it parse just the first line because of '\0'

        if ( FIND_STRING("+CGDCONT:",atcmd_GetLineParameter(line,1)))
        {
            if (numParam==7)
            {
                if(atoi(atcmd_GetLineParameter(line,2)) == profileIndex)
                {
                    strncpy(profileDataPtr->apn,
                            atcmd_GetLineParameter(line,4),
                            PA_MDC_APN_MAX_BYTES);
                    result = LE_OK;
                } else
                {
                    LE_WARN("This is not the good profile %d",
                            atoi(atcmd_GetLineParameter(line,2)));
                    result = LE_FAULT;
                }
            } else {
                LE_WARN("this pattern is not expected");
                result=LE_FAULT;
            }
        } else {
            LE_WARN("this pattern is not expected");
            result=LE_FAULT;
        }
    }

    le_mem_Release(atRespPtr);     // Release atcmdsync_SendCommandDefaultExt

    return result;
}

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
le_result_t pa_mdc_InitializeProfile
(
    uint32_t   profileIndex     ///< [IN] The profile to write
)
{
    // TODO:Implement this one
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Write the profile data for the given profile
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_WriteProfile
(
    uint32_t profileIndex,                    ///< [IN] The profile to write
    pa_mdc_ProfileData_t* profileDataPtr    ///< [IN] The profile data
)
{
    char atcommand[ATCOMMAND_SIZE] ;

    atcmdsync_PrepareString(atcommand,ATCOMMAND_SIZE,
                         "at+cgdcont=%d,\"%s\",\"%s\"",profileIndex, "IP", profileDataPtr->apn);

    return atcmdsync_SendStandard(atports_GetInterface(ATPORT_COMMAND),
                                  atcommand,
                                  NULL,
                                  NULL,
                                  30000);
}


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
le_result_t pa_mdc_StartSessionIPV4
(
    uint32_t profileIndex        ///< [IN] The profile to use
)
{
    le_result_t result=LE_FAULT;

    if ( GetCurrentDataSessionIndex() != INVALID_PROFILE_INDEX )
    {
        return LE_DUPLICATE;
    }

    // Always executed because:
    //   - if GPRS is already attached it doesn't do anything and then it returns OK
    //   - if GPRS is not attached it will attach it and then it returns OK on success
    if ( AttachGPRS(true) != LE_OK )
    {
        return LE_FAULT;
    }

    // Always executed because:
    //   - if the context is already activated it doesn't do anything and then it returns OK
    //   - if the context is not activated it will attach it and then it returns OK on success
    if ( ActivateContext(profileIndex,true) != LE_OK )
    {
        return LE_FAULT;
    }

    if ( (result = EstablishConnection(profileIndex)) != LE_OK )
    {
        SetCurrentDataSessionIndex(INVALID_PROFILE_INDEX);
        return LE_FAULT;
    }

    SetCurrentDataSessionIndex(profileIndex);

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Start a data session with the given profile using IPV6
 *
 * @return
 *      - LE_OK on success
 *      - LE_DUPLICATE if the data session is already connected
 *      - LE_FAULT for other failures
 *
 * @TODO    Implementation
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_StartSessionIPV6
(
    uint32_t profileIndex        ///< [IN] The profile to use
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Start a data session with the given profile using IPV4-V6
 *
 * @return
 *      - LE_OK on success
 *      - LE_DUPLICATE if the data session is already connected
 *      - LE_FAULT for other failures
 *
 * @TODO    implementation
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_StartSessionIPV4V6
(
    uint32_t profileIndex        ///< [IN] The profile to use
)
{
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get session type for the given profile ( IP V4 or V6 )
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT for other failures
 *
 * @TODO    implementation
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_GetSessionType
(
    uint32_t profileIndex,              ///< [IN] The profile to use
    pa_mdc_SessionType_t* sessionIpPtr  ///< [OUT] IP family session
)
{
    return LE_OK;
}


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
le_result_t pa_mdc_StopSession
(
    uint32_t profileIndex        ///< [IN] The profile to use
)
{
    if ( GetCurrentDataSessionIndex() == INVALID_PROFILE_INDEX )
    {
        return LE_FAULT;
    }

    // Stop the PDP connection on modem side
    if (StopPDPConnection() != LE_OK)
    {
        return LE_FAULT;
    }

    SetCurrentDataSessionIndex(INVALID_PROFILE_INDEX);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the session state for the given profile
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on error
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_GetSessionState
(
    uint32_t profileIndex,                    ///< [IN] The profile to use
    le_mdc_ConState_t* sessionStatePtr        ///< [OUT] The data session state
)
{
    // All other profiles are always disconnected
    if ( profileIndex != GetCurrentDataSessionIndex() )
    {
        *sessionStatePtr = LE_MDC_DISCONNECTED;
    } else {
        *sessionStatePtr = LE_MDC_CONNECTED;
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
le_event_HandlerRef_t pa_mdc_AddSessionStateHandler
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
    }

    return NewSessionStateHandlerRef;
}


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
le_result_t pa_mdc_GetInterfaceName
(
    uint32_t profileIndex,                    ///< [IN] The profile to use
    char*  interfaceNameStr,                ///< [OUT] The name of the network interface
    size_t interfaceNameStrSize             ///< [IN] The size in bytes of the name buffer
)
{
    // The interface name will always be of the form pppX, where X is an integer starting at zero.
    // Only one network interface is currently supported, thus X is 0, so hard-code the name.
    const char pppInterfaceNameStr[] = "ppp0";

    le_mdc_ConState_t sessionState;
    le_result_t result;

    result = pa_mdc_GetSessionState(profileIndex, &sessionState);
    if ( (result != LE_OK) || (sessionState != LE_MDC_CONNECTED) )
    {
        return LE_FAULT;
    }

    if (le_utf8_Copy(interfaceNameStr,
                        pppInterfaceNameStr, interfaceNameStrSize, NULL) == LE_OVERFLOW )
    {
        LE_ERROR("Interface name '%s' is too long", pppInterfaceNameStr);
        return LE_OVERFLOW;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the IP address for the given profile, if the data session is connected.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the IP address would not fit in gatewayAddrStr
 *      - LE_FAULT for all other errors
 *
 * @TODO    implementation
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
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the gateway IP address for the given profile, if the data session is connected.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the IP address would not fit in gatewayAddrStr
 *      - LE_FAULT for all other errors
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
    le_result_t result = LE_FAULT;
    char atcommand[ATCOMMAND_SIZE] ;
    char atintermediate[ATCOMMAND_SIZE];

    atcmdsync_PrepareString(atintermediate,ATCOMMAND_SIZE,"+CGPADDR: %d,",profileIndex);

    const char* interRespPtr[] = {atintermediate,NULL};
    atcmdsync_ResultRef_t  atRespPtr = NULL;

    atcmdsync_PrepareString(atcommand,ATCOMMAND_SIZE,"at+cgpaddr=%d",profileIndex);

    result = atcmdsync_SendStandard(atports_GetInterface(ATPORT_COMMAND),
                                    atcommand,
                                    &atRespPtr,
                                    interRespPtr,
                                    30000);

    if ( result != LE_OK ) {
        le_mem_Release(atRespPtr);
        return result;
    }
    // If there is more than one line then it mean that the command is OK so the first line is
    // the intermediate one
    if (atcmdsync_GetNumLines(atRespPtr) == 2)
    {
        // it parse just the first line because of '\0'
        char* line = atcmdsync_GetLine(atRespPtr,0);
        uint32_t  numParam = atcmd_CountLineParameter(line);
        // it parse just the first line because of '\0'

        if (FIND_STRING("+CGPADDR:",atcmd_GetLineParameter(line,1)))
        {
            if (numParam==3)
            {
                if(atoi(atcmd_GetLineParameter(line,2)) == profileIndex)
                {
                    const char* pAddr = atcmd_GetLineParameter(line,3);
                    size_t length = strlen(pAddr);
                    if (length-2 < gatewayAddrStrSize) {
                        atcmd_CopyStringWithoutQuote(gatewayAddrStr,pAddr,gatewayAddrStrSize);
                        result = LE_OK;
                    } else {
                        result = LE_OVERFLOW;
                    }
                } else
                {
                    LE_WARN("This is not the good profile %d",
                            atoi(atcmd_GetLineParameter(line,2)));
                    result = LE_FAULT;
                }
            } else {
                LE_WARN("this pattern is not expected");
                result = LE_FAULT;
            }
        } else {
            LE_WARN("this pattern is not expected");
            result = LE_FAULT;
        }
    }

    le_mem_Release(atRespPtr);     // Release atcmdsync_SendCommandDefaultExt

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Reject a MT-PDP data session for the given profile
 *
 * @return
 *      - LE_OK on success
 *      - LE_BAD_PARAMETER if the input parameter is not valid
 *      - LE_FAULT for other failures
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_RejectMtPdpSession
(
    uint32_t profileIndex
)
{
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the primary/secondary DNS addresses for the given profile, if the data session is connected.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the IP address would not fit in buffer
 *      - LE_FAULT for all other errors
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
    struct sockaddr_in addr;
    struct __res_state res;

    le_mdc_ConState_t sessionState;
    le_result_t result;

    result = pa_mdc_GetSessionState(profileIndex, &sessionState);
    if ( (result != LE_OK) || (sessionState != LE_MDC_CONNECTED) )
    {
        return LE_FAULT;
    }

    memset(dns1AddrStr,0,dns1AddrStrSize);
    memset(dns2AddrStr,0,dns1AddrStrSize);

    res.options &= ~ (RES_INIT);
    if (res_ninit(&res)==-1) {
        return LE_FAULT;
    }

    if (res.nscount > 0) {
        addr = res.nsaddr_list[0];

        if ( dns1AddrStrSize < INET_ADDRSTRLEN) {
            return LE_OVERFLOW;
        }
        // now get it back and save it
        if ( inet_ntop(AF_INET, &(addr.sin_addr), dns1AddrStr, INET_ADDRSTRLEN) == NULL) {
            return LE_FAULT;
        }
    }

    if (res.nscount > 1) {
        addr = res.nsaddr_list[1];

        if ( dns2AddrStrSize < INET_ADDRSTRLEN) {
            return LE_OVERFLOW;
        }
        // now get it back and save it
        if ( inet_ntop(AF_INET, &(addr.sin_addr), dns2AddrStr, INET_ADDRSTRLEN) == NULL) {
            return LE_FAULT;
        }
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the Access Point Name for the given profile, if the data session is connected.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the Access Point Name would not fit in apnNameStr
 *      - LE_FAULT for all other errors
 * @TODO
 *      implementation
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_GetAccessPointName
(
    uint32_t profileIndex,             ///< [IN] The profile to use
    char*  apnNameStr,                 ///< [OUT] The Access Point Name
    size_t apnNameStrSize              ///< [IN] The size in bytes of the address buffer
)
{
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the Data Bearer Technology for the given profile, if the data session is connected.
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT for all other errors
 * @TODO
 *      implementation
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_GetDataBearerTechnology
(
    uint32_t                       profileIndex,              ///< [IN] The profile to use
    le_mdc_DataBearerTechnology_t* downlinkDataBearerTechPtr, ///< [OUT] downlink data bearer technology
    le_mdc_DataBearerTechnology_t* uplinkDataBearerTechPtr    ///< [OUT] uplink data bearer technology
)
{
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get data flow statistics since the last reset.
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT for all other errors
 *
 * @TODO Implementation
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_GetDataFlowStatistics
(
    pa_mdc_PktStatistics_t *dataStatisticsPtr ///< [OUT] Statistics data
)
{
    memset(dataStatisticsPtr,0,sizeof(*dataStatisticsPtr));
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Reset data flow statistics
 *
 * * @return
 *      - LE_OK on success
 *      - LE_FAULT for all other errors
 *
 * @TODO Implementation
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_ResetDataFlowStatistics
(
    void
)
{
    return LE_OK;
}

