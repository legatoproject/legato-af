/**
 * @file pa_mdc.c
 *
 * AT implementation of c_pa_mdc API.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 *
 */

#include "legato.h"

#include "pa_mdc.h"
#include "pa_utils_local.h"

#include "le_atClient.h"

#include <sys/ioctl.h>
#include <termios.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <arpa/inet.h>
#include <resolv.h>


//--------------------------------------------------------------------------------------------------
/**
 * Call event ID
 */
//--------------------------------------------------------------------------------------------------

static le_event_Id_t CallEventId;

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
static le_event_Id_t         UnsolicitedEventId;
static le_event_Id_t         SessionStateEventId;
static le_mem_PoolRef_t      SessionStatePool;
static le_event_HandlerRef_t NewSessionStateHandlerRef = NULL;

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
    char                 command[LE_ATCLIENT_CMD_SIZE_MAX_LEN];
    le_atClient_CmdRef_t cmdRef = NULL;
    le_result_t          res    = LE_FAULT;

    snprintf(command,LE_ATCLIENT_CMD_SIZE_MAX_LEN,"AT+CGATT=%d",toAttach);

    res = le_atClient_SetCommandAndSend(&cmdRef,
                                        command,
                                        "",
                                        DEFAULT_AT_RESPONSE,
                                        DEFAULT_AT_CMD_TIMEOUT);

    le_atClient_Delete(cmdRef);
    return res;
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
    bool     toActivate       ///< [IN] activation boolean
)
{
    char                 command[LE_ATCLIENT_CMD_SIZE_MAX_LEN];
    le_atClient_CmdRef_t cmdRef = NULL;
    le_result_t          res    = LE_FAULT;

    snprintf(command,LE_ATCLIENT_CMD_SIZE_MAX_LEN,"AT+CGACT=%d,%d",toActivate,profileIndex);

    res = le_atClient_SetCommandAndSend(&cmdRef,
                                        command,
                                        "",
                                        DEFAULT_AT_RESPONSE,
                                        DEFAULT_AT_CMD_TIMEOUT);

    le_atClient_Delete(cmdRef);
    return res;
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
    char                 command[LE_ATCLIENT_CMD_SIZE_MAX_LEN];
    le_atClient_CmdRef_t cmdRef = NULL;
    le_result_t          res    = LE_FAULT;

    snprintf(command,LE_ATCLIENT_CMD_SIZE_MAX_LEN,"AT+CGEREP=%d",mode);

    res = le_atClient_SetCommandAndSend(&cmdRef,
                                        command,
                                        "",
                                        DEFAULT_AT_RESPONSE,
                                        DEFAULT_AT_CMD_TIMEOUT);
    if (res == LE_OK)
    {
        if (mode)
        {
            le_atClient_AddUnsolicitedResponseHandler(UnsolicitedEventId,"+CGEV:",false);
        }
        else
        {
            le_atClient_RemoveUnsolicitedResponseHandler(UnsolicitedEventId,"+CGEV:");
        }
    }

    le_atClient_Delete(cmdRef);
    return res;
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
    char*                      unsolPtr        = reportPtr;
    uint32_t                   numParam        = 0;
    pa_mdc_SessionStateData_t* sessionStatePtr = NULL;

    if ( ( FIND_STRING("+CGEV: NW DEACT", unsolPtr) )
        ||
         ( FIND_STRING("+CGEV: ME DEACT", unsolPtr) )
       )
    {
        numParam = pa_utils_CountAndIsolateLineParameters(unsolPtr);

        if (numParam == 4)
        {
            sessionStatePtr = le_mem_ForceAlloc(SessionStatePool);
            sessionStatePtr->profileIndex = atoi(pa_utils_IsolateLineParameter(unsolPtr,4));
            sessionStatePtr->newState = LE_MDC_DISCONNECTED;

            SetCurrentDataSessionIndex(INVALID_PROFILE_INDEX);

            LE_DEBUG("Send Event for %d with state %d",
                     sessionStatePtr->profileIndex,
                     sessionStatePtr->newState);
            le_event_ReportWithRefCounting(SessionStateEventId,sessionStatePtr);
        }
        else
        {
            LE_WARN("this Response pattern is not expected -%s-",unsolPtr);
        }
    }
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
    le_atClient_CmdRef_t cmdRef = NULL;
    le_result_t          res    = LE_FAULT;
    le_atClient_Ports_t  port   = LE_ATCLIENT_PORT_PPP;
    char                 command[LE_ATCLIENT_CMD_SIZE_MAX_LEN] ;
    char                 finalResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];

    if (!profileIndex)
    {
        LE_DEBUG("One parameter is NULL");
        return LE_BAD_PARAMETER;
    }

    snprintf(command,LE_ATCLIENT_CMD_SIZE_MAX_LEN,"ATD*99***%d#",profileIndex);

    cmdRef = le_atClient_Create();
    LE_DEBUG("New command ref (%p) created",cmdRef);
    if(cmdRef == NULL)
    {
        return res;
    }
    res = le_atClient_SetCommand(cmdRef,command);
    if (res != LE_OK)
    {
        le_atClient_Delete(cmdRef);
        LE_ERROR("Failed to set the command !");
        return res;
    }
    res = le_atClient_SetFinalResponse(cmdRef,"CONNECT|NO CARRIER|TIMEOUT|ERROR");
    if (res != LE_OK)
    {
        le_atClient_Delete(cmdRef);
        LE_ERROR("Failed to set final response !");
        return res;
    }
    res = le_atClient_SetPort(cmdRef, port);
    if (res != LE_OK)
    {
        le_atClient_Delete(cmdRef);
        LE_ERROR("Failed to set the command !");
        return res;
    }
    res = le_atClient_Send(cmdRef);
    if (res != LE_OK)
    {
        le_atClient_Delete(cmdRef);
        LE_ERROR("Failed to send !");
        return res;
    }
    else
    {
        res = le_atClient_GetFinalResponse(cmdRef,
                                        finalResponse,
                                        LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES);

        if ((res != LE_OK) || (strcmp(finalResponse,"CONNECT") != 0))
        {
            LE_ERROR("Failed to establish the connection");
        }
        else
        {
            LE_INFO("CONNECT !");
        }
    }
    le_atClient_Delete(cmdRef);
    return res;
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
    le_atClient_CmdRef_t cmdRef = NULL;
    le_result_t          res    = LE_FAULT;

    res = le_atClient_SetCommandAndSend(&cmdRef,
                                        "ATGH",
                                        "",
                                        DEFAULT_AT_RESPONSE,
                                        DEFAULT_AT_CMD_TIMEOUT);

    le_atClient_Delete(cmdRef);
    return res;
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
    if (pid == -1)
    {
        return LE_FAULT;
    }
    else if ( pid == 0) // child process
    {
        char* args[] = {
            "pppd",     /* argv[0], programme name. */
            "noauth",
            "nolock",
            "debug",
            "/dev/ttyACM0",
            "115200",
            "defaultroute",
            "noipdefault",
            "replacedefaultroute",
            "dump",
            "noccp",
            "usepeerdns",
            "updetach",
            "ipcp-accept-local",
            "ipcp-accept-remote",
            "0.0.0.0:0.0.0.0",
            "novj",
            "nomagic",
            "noaccomp",
            "nopcomp",
            NULL      /* list of argument must finished by NULL.  */
        };

        if (execvp("/usr/sbin/pppd", args) < 0)
        {
            LE_INFO("Please install PPP daemon ($ sudo apt-get install ppp)");
            return LE_FAULT;
        }
        else
        {
            LE_INFO("PPP daemon launched");
            return 99;
        }
    }
    else
    {
        int statchild;
        wait(&statchild); // wait for child to finished

        if (WIFEXITED(statchild))
        {
            LE_DEBUG("Child's exit code %d\n", WEXITSTATUS(statchild));

            if (WEXITSTATUS(statchild) == 0)
            {
                // Remove the NO CARRIER unsolicited
                le_atClient_RemoveUnsolicitedResponseHandler(CallEventId,"NO CARRIER");

                return LE_OK;
            }
            else
            {
                return LE_FAULT;
            }
        }
        else
        {
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
    // Start the PDP connection on Modem side
    if (StartPDPConnection(profileIndex) != LE_OK)
    {
        return LE_FAULT;
    }

    // Start the PPP connection on application side
    if (StartPPPInterface() != LE_OK)
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
static void PppCallHandler
(
    void* reportPtr
)
{
    char* unsolPtr = reportPtr;

    if (FIND_STRING("NO CARRIER", unsolPtr))
    {
        SetCurrentDataSessionIndex(INVALID_PROFILE_INDEX);
        le_atClient_RemoveUnsolicitedResponseHandler(CallEventId,"NO CARRIER");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the mdc module
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
    SessionStateEventId = le_event_CreateIdWithRefCounting("SessionStateEventId");
    UnsolicitedEventId  = le_event_CreateId("UnsolicitedEventId",LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES);
    SessionStatePool = le_mem_CreatePool("SessionStatePool", sizeof(pa_mdc_SessionStateData_t));
    CallEventId = le_event_CreateId("CallEventId",LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES);
    le_event_AddHandler("PppCallHandler",CallEventId,PppCallHandler);

    // set unsolicited +CGEV to Register our own handler.
    SetIndicationHandler(2);

    le_event_AddHandler("CGEVUnsolHandler",UnsolicitedEventId,CGEVUnsolHandler);

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
    uint32_t              profileIndex,     ///< [IN] The profile to read
    pa_mdc_ProfileData_t* profileDataPtr    ///< [OUT] The profile data
)
{
    le_result_t          res    = LE_FAULT;

    if (!profileIndex)
    {
        LE_DEBUG("One parameter is NULL");
        return LE_BAD_PARAMETER;
    }

    res = pa_mdc_GetAccessPointName(profileIndex, profileDataPtr->apn, PA_MDC_APN_MAX_BYTES);

    profileDataPtr->pdp = LE_MDC_PDP_IPV4;
    profileDataPtr->authentication.type = LE_MDC_AUTH_NONE;
    return res;
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
    uint32_t profileIndex     ///< [IN] The profile to write
)
{
    pa_mdc_ProfileData_t profileData;
    le_result_t          res          = LE_FAULT;
    char                 defaultAPN[] = "orange.fr";

    res = pa_mdc_ReadProfile(profileIndex, &profileData);

    if (res != LE_OK)
    {
        memset(&profileData,0,sizeof(pa_mdc_ProfileData_t));
        strcpy(&profileData.apn[0],defaultAPN);
        LE_INFO("Initialize");
        res = pa_mdc_WriteProfile(profileIndex, &profileData);
    }

    return res;
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
    uint32_t              profileIndex,     ///< [IN] The profile to write
    pa_mdc_ProfileData_t* profileDataPtr    ///< [IN] The profile data
)
{
    char                 command[LE_ATCLIENT_CMD_SIZE_MAX_LEN];
    le_result_t          res    = LE_FAULT;
    le_atClient_CmdRef_t cmdRef = NULL;

    snprintf(command,
             LE_ATCLIENT_CMD_SIZE_MAX_LEN,
             "AT+CGQREQ=%d,0,0,0,0,0",
             profileIndex);

    res = le_atClient_SetCommandAndSend(&cmdRef,
                                        command,
                                        "",
                                        DEFAULT_AT_RESPONSE,
                                        DEFAULT_AT_CMD_TIMEOUT);
    if (res == LE_OK)
    {
        le_atClient_Delete(cmdRef);
    }


    snprintf(command,
             LE_ATCLIENT_CMD_SIZE_MAX_LEN,
             "AT+CGQMIN=%d,0,0,0,0,0",
             profileIndex);
    cmdRef = NULL;
    res    = le_atClient_SetCommandAndSend(&cmdRef,
                                           command,
                                           "",
                                           DEFAULT_AT_RESPONSE,
                                           DEFAULT_AT_CMD_TIMEOUT);
    if (res == LE_OK)
    {
        le_atClient_Delete(cmdRef);
    }


    snprintf(command,
             LE_ATCLIENT_CMD_SIZE_MAX_LEN,
             "AT+CGDCONT=%d,\"%s\",\"%s\"",
             profileIndex,
             "IP",
             profileDataPtr->apn);
    cmdRef = NULL;
    res    = le_atClient_SetCommandAndSend(&cmdRef,
                                           command,
                                           "",
                                           DEFAULT_AT_RESPONSE,
                                           DEFAULT_AT_CMD_TIMEOUT);
    if (res != LE_OK)
    {
        LE_ERROR("Write profile failed !");
    }
    else
    {
        le_atClient_Delete(cmdRef);
    }
    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the connection failure reason
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_mdc_GetConnectionFailureReason
(
    uint32_t profileIndex,              ///< [IN] The profile to use
    pa_mdc_ConnectionFailureCode_t* failureCodesPtr  ///< [OUT] The specific Failure Reason codes
)
{
    memset(failureCodesPtr, 0, sizeof(pa_mdc_ConnectionFailureCode_t));
    failureCodesPtr->callEndFailure = LE_MDC_DISC_UNDEFINED;
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
    le_result_t res = LE_FAULT;

    if (GetCurrentDataSessionIndex() != INVALID_PROFILE_INDEX)
    {
        return LE_DUPLICATE;
    }

    // Always executed because:
    //   - if GPRS is already attached it doesn't do anything and then it returns OK
    //   - if GPRS is not attached it will attach it and then it returns OK on success
    if (AttachGPRS(true) != LE_OK)
    {
        return LE_FAULT;
    }

    // Always executed because:
    //   - if the context is already activated it doesn't do anything and then it returns OK
    //   - if the context is not activated it will attach it and then it returns OK on success
    if (ActivateContext(profileIndex,true) != LE_OK)
    {
        return LE_FAULT;
    }

    if ((res = EstablishConnection(profileIndex)) != LE_OK)
    {
        SetCurrentDataSessionIndex(INVALID_PROFILE_INDEX);
        return LE_FAULT;
    }

    SetCurrentDataSessionIndex(profileIndex);

    return res;
}


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
le_result_t pa_mdc_StartSessionIPV6
(
    uint32_t profileIndex        ///< [IN] The profile to use
)
{
    return LE_FAULT;
}

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
le_result_t pa_mdc_StartSessionIPV4V6
(
    uint32_t profileIndex        ///< [IN] The profile to use
)
{
    return LE_FAULT;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get session type for the given profile (IPV4 or IPV6)
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT for other failures
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_GetSessionType
(
    uint32_t              profileIndex,     ///< [IN] The profile to use
    pa_mdc_SessionType_t* sessionIpPtr      ///< [OUT] IP family session
)
{
    *sessionIpPtr = PA_MDC_SESSION_IPV4;
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
    if (GetCurrentDataSessionIndex() == INVALID_PROFILE_INDEX)
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
    uint32_t           profileIndex,          ///< [IN] The profile to use
    le_mdc_ConState_t* sessionStatePtr        ///< [OUT] The data session state
)
{
    // All other profiles are always disconnected
    if (profileIndex != GetCurrentDataSessionIndex())
    {
        *sessionStatePtr = LE_MDC_DISCONNECTED;
    }
    else
    {
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
    if ((NewSessionStateHandlerRef != NULL) || (handlerRef == NULL))
    {
        LE_INFO("Clearing old handler");
        le_event_RemoveHandler(NewSessionStateHandlerRef);
        NewSessionStateHandlerRef = NULL;
    }

    // Check if new handler is being added
    if (handlerRef != NULL)
    {
        NewSessionStateHandlerRef = le_event_AddHandler("NewSessionStateHandler",
                                                        SessionStateEventId,
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
    char*    interfaceNameStr,                ///< [OUT] The name of the network interface
    size_t   interfaceNameStrSize             ///< [IN] The size in bytes of the name buffer
)
{
    // The interface name will always be of the form pppX, where X is an integer starting at zero.
    // Only one network interface is currently supported, thus X is 0, so hard-code the name.
    const char pppInterfaceNameStr[] = "ppp0";

    le_mdc_ConState_t sessionState;
    le_result_t res;

    res = pa_mdc_GetSessionState(profileIndex, &sessionState);
    if ((res != LE_OK) || (sessionState != LE_MDC_CONNECTED))
    {
        return LE_FAULT;
    }

    if (le_utf8_Copy(interfaceNameStr,pppInterfaceNameStr,interfaceNameStrSize, NULL) == LE_OVERFLOW)
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
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_GetIPAddress
(
    uint32_t               profileIndex,       ///< [IN] The profile to use
    le_mdmDefs_IpVersion_t ipVersion,          ///< [IN] IP Version
    char*                  ipAddrStr,          ///< [OUT] The IP address in dotted format
    size_t                 ipAddrStrSize       ///< [IN] The size in bytes of the address buffer
)
{
    le_result_t res = LE_FAULT;

    if(ipVersion != LE_MDMDEFS_IPV4)
    {
        LE_ERROR("Only IPv4 is supported");
    }
    else
    {
        le_atClient_CmdRef_t cmdRef   = NULL;
        char*                tokenPtr = NULL;
        char*                savePtr  = NULL;
        char                 intermediate[LE_ATCLIENT_CMD_SIZE_MAX_LEN];
        char                 intermediateResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];
        char                 finalResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];

        if (!profileIndex)
        {
            LE_DEBUG("One parameter is NULL");
            return LE_BAD_PARAMETER;
        }

        snprintf(intermediate,LE_ATCLIENT_CMD_SIZE_MAX_LEN,"+CGDCONT: %d,",profileIndex);

        res = le_atClient_SetCommandAndSend(&cmdRef,
                                            "AT+CGDCONT?",
                                            intermediate,
                                            DEFAULT_AT_RESPONSE,
                                            DEFAULT_AT_CMD_TIMEOUT);
        if (res != LE_OK)
        {
            LE_ERROR("Failed to send the command");
            return res;
        }

        res = le_atClient_GetFinalResponse(cmdRef,
                                        finalResponse,
                                        LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES);

        if ((res != LE_OK) || (strcmp(finalResponse,"OK") != 0))
        {
            LE_ERROR("Failed to get the final response");
            le_atClient_Delete(cmdRef);
            return res;
        }

        res = le_atClient_GetFirstIntermediateResponse(cmdRef,
                                                    intermediateResponse,
                                                    LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES);
        if (res != LE_OK)
        {
            LE_ERROR("Failed to get the intermediate response");
        }
        else
        {
            tokenPtr = strtok_r(intermediateResponse, "\"", &savePtr);
            int cpt;
            for (cpt = 0; cpt < 5; cpt++)
            {
                tokenPtr = strtok_r(NULL, "\"", &savePtr);
            }
            strncpy(ipAddrStr, tokenPtr, ipAddrStrSize);
        }
        le_atClient_Delete(cmdRef);
    }
    return res;
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
    uint32_t                profileIndex,      ///< [IN]  The profile to use
    le_mdmDefs_IpVersion_t  ipVersion,         ///< [IN]  IP Version
    char*                   gatewayAddrStr,    ///< [OUT] The gateway IP address in dotted format
    size_t                  gatewayAddrStrSize ///< [IN]  The size in bytes of the address buffer
)
{
    le_atClient_CmdRef_t cmdRef   = NULL;
    le_result_t          res      = LE_FAULT;
    char*                tokenPtr = NULL;
    char*                savePtr  = NULL;
    char                 command[LE_ATCLIENT_CMD_SIZE_MAX_LEN] ;
    char                 intermediate[LE_ATCLIENT_CMD_SIZE_MAX_LEN];
    char                 intermediateResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];
    char                 finalResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];

    if (!profileIndex)
    {
        LE_DEBUG("One parameter is NULL");
        return LE_BAD_PARAMETER;
    }

    snprintf(command,LE_ATCLIENT_CMD_SIZE_MAX_LEN,"AT+CGPADDR=%d",profileIndex);
    snprintf(intermediate,LE_ATCLIENT_CMD_SIZE_MAX_LEN,"+CGPADDR: %d,",profileIndex);

    res = le_atClient_SetCommandAndSend(&cmdRef,
                                        command,
                                        intermediate,
                                        DEFAULT_AT_RESPONSE,
                                        DEFAULT_AT_CMD_TIMEOUT);
    if (res != LE_OK)
    {
        LE_ERROR("Failed to send the command");
        return res;
    }

    res = le_atClient_GetFinalResponse(cmdRef,
                                       finalResponse,
                                       LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES);

    if ((res != LE_OK) || (strcmp(finalResponse,"OK") != 0))
    {
        LE_ERROR("Failed to get the final response");
        le_atClient_Delete(cmdRef);
        return res;
    }

    res = le_atClient_GetFirstIntermediateResponse(cmdRef,
                                                   intermediateResponse,
                                                   LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES);
    if (res != LE_OK)
    {
        LE_ERROR("Failed to get the intermediate response");
    }
    else
    {
        tokenPtr = strtok_r(intermediateResponse, "\"", &savePtr);
        tokenPtr = strtok_r(NULL, "\"", &savePtr);
        strncpy(gatewayAddrStr, tokenPtr, gatewayAddrStrSize);
    }
    le_atClient_Delete(cmdRef);
    return res;
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
    uint32_t               profileIndex,    ///< [IN]  The profile to use
    le_mdmDefs_IpVersion_t ipVersion,       ///< [IN]  IP Version
    char*                  dns1AddrStr,     ///< [OUT] The primary DNS IP address in dotted format
    size_t                 dns1AddrStrSize, ///< [IN]  The size in bytes of the dns1AddrStr buffer
    char*                  dns2AddrStr,     ///< [OUT] The secondary DNS IP address in dotted format
    size_t                 dns2AddrStrSize  ///< [IN]  The size in bytes of the dns2AddrStr buffer
)
{
    struct sockaddr_in addr;
    struct __res_state res;
    le_mdc_ConState_t  sessionState;
    le_result_t        result = LE_OK;

    result = pa_mdc_GetSessionState(profileIndex, &sessionState);
    if ((result != LE_OK) || (sessionState != LE_MDC_CONNECTED))
    {
        return LE_FAULT;
    }

    memset(dns1AddrStr,0,dns1AddrStrSize);
    memset(dns2AddrStr,0,dns1AddrStrSize);

    res.options &= ~ (RES_INIT);
    if (res_ninit(&res)==-1)
    {
        return LE_FAULT;
    }

    if (res.nscount > 0)
    {
        addr = res.nsaddr_list[0];

        if ( dns1AddrStrSize < INET_ADDRSTRLEN)
        {
            return LE_OVERFLOW;
        }

        // Now get it back and save it
        if ( inet_ntop(AF_INET, &(addr.sin_addr), dns1AddrStr, INET_ADDRSTRLEN) == NULL)
        {
            return LE_FAULT;
        }
    }

    if (res.nscount > 1)
    {
        addr = res.nsaddr_list[1];

        if ( dns2AddrStrSize < INET_ADDRSTRLEN)
        {
            return LE_OVERFLOW;
        }

        // Now get it back and save it
        if ( inet_ntop(AF_INET, &(addr.sin_addr), dns2AddrStr, INET_ADDRSTRLEN) == NULL)
        {
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
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_GetAccessPointName
(
    uint32_t profileIndex,               ///< [IN]  The profile to use
    char*    apnNameStr,                 ///< [OUT] The Access Point Name
    size_t   apnNameStrSize              ///< [IN]  The size in bytes of the address buffer
)
{
    le_atClient_CmdRef_t cmdRef   = NULL;
    le_result_t          res      = LE_FAULT;
    char*                tokenPtr = NULL;
    char*                savePtr  = NULL;
    char                 intermediate[LE_ATCLIENT_CMD_SIZE_MAX_LEN];
    char                 intermediateResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];
    char                 finalResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];

    if (!profileIndex)
    {
        LE_DEBUG("One parameter is NULL");
        return LE_BAD_PARAMETER;
    }

    snprintf(intermediate,LE_ATCLIENT_CMD_SIZE_MAX_LEN,"+CGDCONT: %d,",profileIndex);

    res = le_atClient_SetCommandAndSend(&cmdRef,
                                        "AT+CGDCONT?",
                                        intermediate,
                                        DEFAULT_AT_RESPONSE,
                                        DEFAULT_AT_CMD_TIMEOUT);
    if (res != LE_OK)
    {
        LE_ERROR("Failed to send the command");
        return res;
    }

    res = le_atClient_GetFinalResponse(cmdRef,
                                       finalResponse,
                                       LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES);

    if ((res != LE_OK) || (strcmp(finalResponse,"OK") != 0))
    {
        LE_ERROR("Failed to get the final response");
        le_atClient_Delete(cmdRef);
        return res;
    }

    res = le_atClient_GetFirstIntermediateResponse(cmdRef,
                                                   intermediateResponse,
                                                   LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES);
    if (res != LE_OK)
    {
        LE_ERROR("Failed to get the intermediate response");
    }
    else
    {
        tokenPtr = strtok_r(intermediateResponse, "\"", &savePtr);
        int cpt;
        for (cpt = 0; cpt < 3; cpt++)
        {
            tokenPtr = strtok_r(NULL, "\"", &savePtr);
        }
        strncpy(apnNameStr, tokenPtr, apnNameStrSize);
    }
    le_atClient_Delete(cmdRef);
    return res;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the Data Bearer Technology for the given profile, if the data session is connected.
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT for all other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_GetDataBearerTechnology
(
    uint32_t                       profileIndex,              ///< [IN]  The profile to use
    le_mdc_DataBearerTechnology_t* downlinkDataBearerTechPtr, ///< [OUT] Downlink data bearer technology
    le_mdc_DataBearerTechnology_t* uplinkDataBearerTechPtr    ///< [OUT] Uplink data bearer technology
)
{
    return LE_FAULT;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get data flow statistics since the last reset.
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT for all other errors
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
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_ResetDataFlowStatistics
(
    void
)
{
    return LE_FAULT;
}
