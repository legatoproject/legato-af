
#include <arpa/inet.h>

/* Legato Framework */
#include "legato.h"

/* IPC APIs */
#include "interfaces.h"

// -------------------------------------------------------------------------------------------------
/**
 *  Sierra Wireless server IP address used for data connection testing.
 */
// -------------------------------------------------------------------------------------------------
#define SERVER_ADDR_V4 "69.10.131.102"
#define SERVER_ADDR_V6 "2a01:cd00:ff:ffff::450a:8366"


// -------------------------------------------------------------------------------------------------
/**
 *  Is the destination number valid.
 */
// -------------------------------------------------------------------------------------------------
static char DestNumValid = false;

// -------------------------------------------------------------------------------------------------
/**
 *  The destination phone number we report to on events.
 */
// -------------------------------------------------------------------------------------------------
static char DestNum[LE_MDMDEFS_PHONE_NUM_MAX_BYTES] = { 0 };

// -------------------------------------------------------------------------------------------------
/**
 *  File pointer to the chat log maintained by this application.  All incoming and outgoing texts go
 *  here.
 */
// -------------------------------------------------------------------------------------------------
static FILE* OutputFilePtr = NULL;

// -------------------------------------------------------------------------------------------------
/**
 *  The data connection reference
 */
// -------------------------------------------------------------------------------------------------
static le_data_RequestObjRef_t RequestRef = NULL;

// -------------------------------------------------------------------------------------------------
/**
 *  Get a string for a given network state enumeration.
 *
 *  @return
 *       A string that best represents the given enumerated network state.
 *
 *  @note
 *       Do *not* free the returned string.
 */
// -------------------------------------------------------------------------------------------------
static const char* GetNetStateString
(
    le_mrc_NetRegState_t state   ///< [IN] Network state to translate into a string.
)
{
    char* string = NULL;

    switch (state)
    {
        case LE_MRC_REG_NONE:
            string = "off of the network";
            break;

        case LE_MRC_REG_HOME:
            string = "registered on its home network";
            break;

        case LE_MRC_REG_SEARCHING:
            string = "searching for network";
            break;

        case LE_MRC_REG_DENIED:
            string = "denied access to the network";
            break;

        case LE_MRC_REG_ROAMING:
            string = "registered on a roaming network";
            break;

        case LE_MRC_REG_UNKNOWN:
        default:
            string = "in an unknown state";
            break;
    }

    return string;
}



// -------------------------------------------------------------------------------------------------
/**
 *  Get a string for a given SIM state enumeration.
 *
 *  @return
 *       A string that best represents the given enumerated SIM state.
 *
 *  @note
 *       Do *not* free the returned string.
 */
// -------------------------------------------------------------------------------------------------
static const char* GetSimStateString
(
    le_sim_States_t simState   ///< [IN] SIM state to translate into a string.
)
{
    char* string = NULL;

    switch (simState)
    {
        case LE_SIM_INSERTED:
            string = "inserted and locked";
            break;

        case LE_SIM_ABSENT:
            string = "absent";
            break;

        case LE_SIM_READY:
            string = "inserted and unlocked";
            break;

        case LE_SIM_BLOCKED:
            string = "blocked";
            break;

        case LE_SIM_BUSY:
            string = "busy";
            break;

        case LE_SIM_STATE_UNKNOWN:
        default:
            string = "in an unknown state";
            break;
    }

    return string;
}



// -------------------------------------------------------------------------------------------------
/**
 *  Get a string for a given radio signal strength enumeration.
 *
 *  @return
 *       A string that best represents the given enumerated radio signal strength.
 *
 *  @note
 *       Do *not* free the returned string.
 */
// -------------------------------------------------------------------------------------------------
static const char* GetSignalString
(
    uint32_t signal   ///< [IN] Signal level to translate into a string.
)
{
    char* string = NULL;

    switch (signal)
    {
        case 0:
            string = "nonexistant";
            break;

        case 1:
            string = "very weak";
            break;

        case 2:
            string = "weak";
            break;

        case 3:
            string = "good";
            break;

        case 4:
            string = "strong";
            break;

        case 5:
            string = "very strong";
            break;

        default:
            string = "unknown";
            break;
    }

    return string;
}




// -------------------------------------------------------------------------------------------------
/**
 *  This function deals with the actual sending of a text message to a destination.
 *  If the message is longer than 160 char, then the message will be send by multiple SMS,
 *  the arrival order is not predictable.
 *
 *  @return
 *       - LE_OK            The function succeeded.
 *       - LE_BAD_PARAMETER The telephone destination number is invalid, or the text message is
 *                          NULL.
 *       - LE_OUT_OF_RANGE  The telephone number or the message is too long.
 *       - LE_FORMAT_ERROR  The message content is invalid.
 *       - LE_FAULT         The function failed to send the message.
 *
 *  @note
 *       If this function fails to allocate it's required send buffers, the appliation will
 *       terminate.
 */
// -------------------------------------------------------------------------------------------------
static le_result_t SendMessage
(
    const char* number,   ///< [IN] The destination phone number of this message.
    const char* message   ///< [IN] The message to be sent to the destination phone.
)
{
    le_sms_MsgRef_t messageRef = NULL;
    le_result_t result;

    uint32_t messageSize = strlen(message);
    uint32_t messagePart;
    uint32_t i;

    if (messageSize % LE_SMS_TEXT_MAX_LEN)
    {
        messagePart = (messageSize/LE_SMS_TEXT_MAX_LEN)+1;
    }
    else
    {
        messagePart = messageSize/LE_SMS_TEXT_MAX_LEN;
    }

    for (i=0; i < messagePart; i++)
    {
        uint32_t messageCopySize;
        char messageToSend[LE_SMS_TEXT_MAX_BYTES] = {0};

        if (i == (messagePart-1) )
        {
            messageCopySize = messageSize % LE_SMS_TEXT_MAX_LEN;
        }
        else
        {
            messageCopySize = LE_SMS_TEXT_MAX_LEN;
        }

        memcpy(messageToSend, &message[i*LE_SMS_TEXT_MAX_LEN], messageCopySize);
        messageToSend[messageCopySize] = '\0';

        // Allocate a message object from the SMS pool.  If this fails, the application will halt, so
        // there's no point in checking for a valid object.
        messageRef = le_sms_Create();

        // Populate the message parameters, and let the underlying API validate them.
        if ((result = le_sms_SetDestination(messageRef, number)) != LE_OK)
        {
            LE_ERROR("Failed to set message destination number.  Result: %d", result);
            goto done;
        }

        if ((result = le_sms_SetText(messageRef, messageToSend)) != LE_OK)
        {
            LE_ERROR("Failed to set the message text.  Result: %d", result);
            goto done;
        }

        // Now, attempt to send the message.
        if ((result = le_sms_Send(messageRef)) != LE_OK)
        {
            LE_ERROR("Message transmission failed.  Result: %d", result);
            goto done;
        }

        // If we got here, then the send was successful.  Record the message in the log.
        if (OutputFilePtr)
        {
            fprintf(OutputFilePtr, "send (%s): %s\n", number, messageToSend);
            fflush(OutputFilePtr);
        }

        le_sms_Delete(messageRef);
        messageRef = NULL;
    }
// Finally, clean up our memory and let the caller know what happened.
done:

    if (messageRef) { le_sms_Delete(messageRef); }
    return result;
}



// -------------------------------------------------------------------------------------------------
/**
 *  Command to test out the modem data control.  This function will request the default data connection
 */
// -------------------------------------------------------------------------------------------------
static void GoOnline
(
    void
)
{
    if(RequestRef)
    {
        LE_ERROR("A connection request already exist.");
        return;
    }

    RequestRef = le_data_Request();
    LE_INFO("Requesting the default data connection: %p.", RequestRef);
}



// -------------------------------------------------------------------------------------------------
/**
 *  The opposite of go online, this function will tear down the data connection.
 */
// -------------------------------------------------------------------------------------------------
static void GoOffline
(
    char* buffer   ///< [OUT] On success or failure, a message is written to this buffer.
)
{
    if(!RequestRef)
    {
        LE_ERROR("Not existing connection reference.");
        return;
    }

    le_data_Release(RequestRef);
    LE_INFO("Releasing the default data connection.");

    RequestRef = NULL;
}




// -------------------------------------------------------------------------------------------------
/**
 *  In order to test out the active data connection, we simply attempt to connect to Sierra's website and
 *  report either success or failure (through TCP connection).
 */
// -------------------------------------------------------------------------------------------------
static void TestDataConnectionV4
(
    char* buffer
)
{
    int sockFd = 0;
    struct sockaddr_in servAddr;

    if ((sockFd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        sprintf(buffer, "Failed to create socket");
        return;
    }

    LE_INFO("Connecting to %s (www.sierrawireless.com)\n", SERVER_ADDR_V4);

    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(80);
    servAddr.sin_addr.s_addr = inet_addr(SERVER_ADDR_V4);

    if (connect(sockFd, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0)
    {
        sprintf(buffer, "Failed to connect to www.sierrawireless.com.");
    }
    else
    {
        sprintf(buffer, "Connection to www.sierrawireless.com was successful.");
    }

    close(sockFd);
}

// -------------------------------------------------------------------------------------------------
/**
 *  In order to test out the active data connection, we simply attempt to connect to Sierra's website and
 *  report either success or failure (through TCP connection).
 */
// -------------------------------------------------------------------------------------------------
static void TestDataConnectionV6
(
    char* buffer
)
{
    int sockFd = 0;
    struct sockaddr_in6 servAddr;

    if ((sockFd = socket(AF_INET6, SOCK_STREAM, 0)) < 0)
    {
        sprintf(buffer, "Failed to create socket");
        return;
    }

    LE_INFO("Connecting to %s (www.sierrawireless.com)\n", SERVER_ADDR_V6);

    servAddr.sin6_family = AF_INET6;
    servAddr.sin6_port = htons(80);
    if ( inet_pton(AF_INET6,SERVER_ADDR_V6,&(servAddr.sin6_addr)) == 1 )
    {
        if (connect(sockFd, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0)
        {
            sprintf(buffer, "Failed to connect to www.sierrawireless.com.");
        }
        else
        {
            sprintf(buffer, "Connection to www.sierrawireless.com was successful.");
        }
    }
    else
    {
        sprintf(buffer, "Failed to convert %s ipv6.",SERVER_ADDR_V6);
    }

    close(sockFd);
}

// -------------------------------------------------------------------------------------------------
/**
 *  This function returns some useful information about the active data connection.
 */
// -------------------------------------------------------------------------------------------------
static void Netinfo
(
    char* buffer   ///< [OUT] On success or failure, a message is written to this buffer.
)
{
    uint32_t bufferIndex = 0;
    // hard coded, first profile.
    le_mdc_ProfileRef_t profileRef = le_mdc_GetProfile(1);

    if (profileRef == NULL)
    {
        bufferIndex += sprintf(&buffer[bufferIndex], "Failed to open profile.");
        return;
    }

    char interfaceName[100];
    char gatewayAddr[100];
    char dns1Addr[100];
    char dns2Addr[100];

    if (le_mdc_GetInterfaceName(profileRef, interfaceName, sizeof(interfaceName)) != LE_OK)
    {
        bufferIndex += sprintf(&buffer[bufferIndex], "Failed to get interface name.");
        interfaceName[0] = '\0';
        return;
    }

    if ( le_mdc_IsIPv4(profileRef) )
    {
        if (le_mdc_GetIPv4GatewayAddress(profileRef, gatewayAddr, sizeof(gatewayAddr)) != LE_OK)
        {
            bufferIndex += sprintf(&buffer[bufferIndex], "Failed to get gateway address.");
            gatewayAddr[0] = '\0';
            return;
        }

        if (le_mdc_GetIPv4DNSAddresses(profileRef,
                                   dns1Addr, sizeof(dns1Addr),
                                   dns2Addr, sizeof(dns2Addr)) != LE_OK)
        {
            bufferIndex += sprintf(&buffer[bufferIndex], "Failed to read DNS addresses.");
            dns1Addr[0] = '\0';
            dns2Addr[0] = '\0';
            return;
        }

        bufferIndex += sprintf(&buffer[bufferIndex],
                               "\nIPV4 GW: %s, DNS1: %s, DNS2: %s on %s",
                               gatewayAddr, dns1Addr, dns2Addr, interfaceName);
    }

    if ( le_mdc_IsIPv6(profileRef) )
    {
        if (le_mdc_GetIPv6GatewayAddress(profileRef, gatewayAddr, sizeof(gatewayAddr)) != LE_OK)
        {
            bufferIndex += sprintf(&buffer[bufferIndex], "Failed to get gateway address.");
            gatewayAddr[0] = '\0';
            return;
        }

        if (le_mdc_GetIPv6DNSAddresses(profileRef,
                                   dns1Addr, sizeof(dns1Addr),
                                   dns2Addr, sizeof(dns2Addr)) != LE_OK)
        {
            bufferIndex += sprintf(&buffer[bufferIndex], "Failed to read DNS addresses.");
            dns1Addr[0] = '\0';
            dns2Addr[0] = '\0';
            return;
        }

        bufferIndex += sprintf(&buffer[bufferIndex],
                               "\nIPV6 GW: %s, DNS1: %s, DNS2: %s on %s",
                               gatewayAddr, dns1Addr, dns2Addr, interfaceName);
    }

}

// -------------------------------------------------------------------------------------------------
/**
 *  This function returns some useful information about the data flow statistics.
 */
// -------------------------------------------------------------------------------------------------
static void Datainfo
(
    char* buffer   ///< [OUT] On success or failure, a message is written to this buffer.
)
{
    uint64_t rxBytes=0;
    uint64_t txBytes=0;

    if (le_mdc_GetBytesCounters(&rxBytes,&txBytes) != LE_OK)
    {
        sprintf(buffer, "Failed to get bytes statistics.");
        return;
    }

    sprintf(buffer, "Data bytes statistics: Received: %"PRIu64", Transmitted: %"PRIu64" ",
                    rxBytes, txBytes);
}

// -------------------------------------------------------------------------------------------------
/**
 *  This function reset bytes information of the data flow statistics.
 */
// -------------------------------------------------------------------------------------------------
static void DataReset
(
    char* buffer   ///< [OUT] On success or failure, a message is written to this buffer.
)
{
    uint64_t rxBytes=0;
    uint64_t txBytes=0;

    le_mdc_ResetBytesCounter();

    if (le_mdc_GetBytesCounters(&rxBytes,&txBytes) != LE_OK)
    {
        sprintf(buffer, "Failed to get bytes statistics.");
        return;
    }

    sprintf(buffer, "Reset Data bytes statistics: Received: %"PRIu64", Transmitted: %"PRIu64" ",
                    rxBytes, txBytes);
}

static const char* PrintNetworkName
(
    le_mrc_Rat_t technology
)
{
    switch (technology)
    {
    case LE_MRC_RAT_UNKNOWN:
        return "Undefined";
    case LE_MRC_RAT_GSM:
        return "GSM";
    case LE_MRC_RAT_UMTS:
        return "UMTS";
    case LE_MRC_RAT_LTE:
        return "LTE";
    case LE_MRC_RAT_CDMA:
        return "CDMA";
    }
    return "Undefined";
}

// -------------------------------------------------------------------------------------------------
/**
 *  This is a helper function to append strings to a buffer.
 *
 *  @return Number of bytes written in buffer.
 */
// -------------------------------------------------------------------------------------------------
static int AppendToBuffer
(
    char* bufferPtr,
    ssize_t bufferSz,
    const char * formatPtr,
    ...
)
{
    int bufferIdx;

    if (bufferSz <= 0)
    {
        return 0;
    }

    va_list args;
    va_start(args, formatPtr);
    bufferIdx = vsnprintf(bufferPtr, bufferSz, formatPtr, args);
    va_end(args);

    return bufferIdx;
}

// -------------------------------------------------------------------------------------------------
/**
 *  This function returns Network Scan.
 */
// -------------------------------------------------------------------------------------------------
static void PerformScan
(
    char* bufferPtr,         ///< [OUT] On success or failure, a message is written to this buffer.
    size_t bufferSz,         ///< [IN] Size of the output buffer.
    const char* requesterPtr ///< [IN] If not NULL, then any response text is SMSed to this target.
)
{
    int bufferIdx = 0;

    le_mrc_ScanInformationListRef_t scanInformationList = NULL;
    fprintf(OutputFilePtr, "Scan was asked");
    scanInformationList = le_mrc_PerformCellularNetworkScan(LE_MRC_BITMASK_RAT_ALL);
    if (!scanInformationList)
    {
        bufferIdx += AppendToBuffer(&bufferPtr[bufferIdx], (bufferSz - bufferIdx), "Could not perform scan\n");
        return;
    }

    le_mrc_ScanInformationRef_t cellRef;

    for (cellRef=le_mrc_GetFirstCellularNetworkScan(scanInformationList);
         cellRef!=NULL;
         cellRef=le_mrc_GetNextCellularNetworkScan(scanInformationList))
    {
        char mcc[4], mnc[4];
        char name[100];
        le_mrc_Rat_t rat;

        bufferIdx = 0;

        rat = le_mrc_GetCellularNetworkRat(cellRef);

        bufferIdx += AppendToBuffer(&bufferPtr[bufferIdx], (bufferSz - bufferIdx), " %s ", PrintNetworkName(rat));

        if (le_mrc_GetCellularNetworkMccMnc(cellRef, mcc, sizeof(mcc), mnc, sizeof(mnc)) != LE_OK)
        {
            bufferIdx += AppendToBuffer(&bufferPtr[bufferIdx], (bufferSz - bufferIdx), "Failed to get operator code.\n");
        }
        else
        {
            bufferIdx += AppendToBuffer(&bufferPtr[bufferIdx], (bufferSz - bufferIdx), "%s-%s ", mcc, mnc);
        }

        if (le_mrc_GetCellularNetworkName(cellRef, name, sizeof(name)) != LE_OK)
        {
            bufferIdx += AppendToBuffer(&bufferPtr[bufferIdx], (bufferSz - bufferIdx), "Failed to get operator name.\n");
        }
        else
        {
            bufferIdx += AppendToBuffer(&bufferPtr[bufferIdx], (bufferSz - bufferIdx), "%s", name);
        }

        bufferIdx += AppendToBuffer(&bufferPtr[bufferIdx], (bufferSz - bufferIdx), " - %s ", PrintNetworkName(rat));

        bufferIdx += AppendToBuffer(&bufferPtr[bufferIdx], (bufferSz - bufferIdx), "%s,",
                             le_mrc_IsCellularNetworkInUse(cellRef)?"In use":"Unused");

        bufferIdx += AppendToBuffer(&bufferPtr[bufferIdx], (bufferSz - bufferIdx), "%s,",
                             le_mrc_IsCellularNetworkAvailable(cellRef)?"Available":"Unavailable");

        bufferIdx += AppendToBuffer(&bufferPtr[bufferIdx], (bufferSz - bufferIdx), "%s,",
                             le_mrc_IsCellularNetworkHome(cellRef)?"Home":"Roaming");

        bufferIdx += AppendToBuffer(&bufferPtr[bufferIdx], (bufferSz - bufferIdx), "%s\n",
                             le_mrc_IsCellularNetworkForbidden(cellRef)?"Forbidden":"Allowed");

        // Check to see if any processing has occurred.  If so, check to see if the request came from a
        // local or remote requester.
        // If the requester was local, (requesterPtr == NULL) then simply log our response to the SMS log.
        // Otherwise, attempt to SMS the response string to the original caller.
        if (requesterPtr != NULL)
        {
            SendMessage(requesterPtr, bufferPtr);
        }
        else if (OutputFilePtr != NULL)
        {
            fprintf(OutputFilePtr, "## %s ##\n", bufferPtr);
            fflush(OutputFilePtr);
        }
    }

    le_mrc_DeleteCellularNetworkScan(scanInformationList);

    bufferIdx += snprintf(&bufferPtr[0], bufferSz, "Scan was Performed");
}


// -------------------------------------------------------------------------------------------------
/**
 *  Called to check to see if the given text is a valid command.  If the text is one of the
 *  supported commands, this function will carry out that command.
 *
 *  @return Whether the supplied text was a command (and therefore processed) or not.
 */
// -------------------------------------------------------------------------------------------------
static bool ProcessCommand
(
    const char* textPtr,      ///< [IN] Check this text to see if it's a valid command.
    const char* requesterPtr  ///< [IN] If not NULL, then any response text is SMSed to this target.
)
{
    char buffer[10240];

    // Start looking for a match...
    if (strcmp(textPtr, "Crash") == 0)
    {
        // As the name implies, we are going to be crashing the application.  So simply append to
        // the output log and crash the app.  This is done to allow demonstration of the supervisor
        // policies.
        int x = 10;
        int y = 0;

        LE_ERROR("Something wicked this way comes...");
        LE_ERROR("Data result: %d", x / y);
    }
    else if (strcmp(textPtr, "Status") == 0)
    {
        // The status command allows the caller to query the current state of the modem.  A friendly
        // version of this status is constructed and returned to the caller.
        le_onoff_t radioStatus;
        const char * radioStatusPtr;
        le_mrc_NetRegState_t netRegState;
        uint32_t signalQuality;

        if(le_mrc_GetRadioPower(&radioStatus) != LE_OK)
        {
            radioStatusPtr = "in an unknown state";
        }
        else
        {
            radioStatusPtr = (radioStatus == LE_OFF) ? "off" : "on";
        }

        if(le_mrc_GetNetRegState(&netRegState) != LE_OK)
        {
            netRegState = LE_MRC_REG_UNKNOWN;
        }

        if(le_mrc_GetSignalQual(&signalQuality) != LE_OK)
        {
            signalQuality = 0;
        }

        sprintf(buffer, "The radio is %s and is %s. The signal strength is %s.",
                radioStatusPtr,
                GetNetStateString(netRegState),
                GetSignalString(signalQuality));
    }
    else if (strcmp(textPtr, "Sim") == 0)
    {
        // Like the status command, this command queries the underling hardware for information.
        // This information is turned into a string that can then be returned to the caller.
        le_sim_Id_t simId;
        le_sim_States_t simState;

        char iccid[100];
        char imsi[100];
        int pos = 0;

        simId = le_sim_GetSelectedCard();
        simState = le_sim_GetState(simId);

        pos += snprintf(buffer + pos, sizeof(buffer) - pos,
                "SIM %u is %s.",
                simId,
                GetSimStateString(simState));

        if(le_sim_GetICCID(simId, iccid, sizeof(iccid)) == LE_OK)
        {
            pos += snprintf(buffer + pos, sizeof(buffer) - pos,
                    " ICCID=%s", iccid);
        }

        if(le_sim_GetIMSI(simId, imsi, sizeof(imsi)) == LE_OK)
        {
            pos += snprintf(buffer + pos, sizeof(buffer) - pos,
                    " IMSI=%s", imsi);
        }

    }
    else if (strcmp(textPtr, "Online") == 0)
    {
        le_utf8_Copy(buffer, "Requesting data connection.", sizeof(buffer), NULL);
        GoOnline();
    }
    else if (strcmp(textPtr, "Offline") == 0)
    {
        le_utf8_Copy(buffer, "Releasing data connection.", sizeof(buffer), NULL);
        GoOffline(buffer);
    }
    else if (strcmp(textPtr, "TestDataConnectionV4") == 0)
    {
        TestDataConnectionV4(buffer);
    }
    else if (strcmp(textPtr, "TestDataConnectionV6") == 0)
    {
        TestDataConnectionV6(buffer);
    }
    else if (strcmp(textPtr, "Netinfo") == 0)
    {
        Netinfo(buffer);
    }
    else if (strcmp(textPtr, "DataInfo") == 0)
    {
        Datainfo(buffer);
    }
    else if (strcmp(textPtr, "DataReset") == 0)
    {
        DataReset(buffer);
    }
    else if (strcmp(textPtr, "Scan") == 0)
    {
        PerformScan(buffer, sizeof(buffer), requesterPtr);
    }
    else
    {
        return false;
    }

    // Check to see if any processing has occurred.  If so, check to see if the request came from a
    // local or remote requester.
    // If the requester was local, (requesterPtr == NULL) then simply log our response to the SMS log.
    // Otherwise, attempt to SMS the response string to the original caller.
    if (requesterPtr != NULL)
    {
        SendMessage(requesterPtr, buffer);
    }
    else if (OutputFilePtr != NULL)
    {
        fprintf(OutputFilePtr, "## %s ##\n", buffer);
        fflush(OutputFilePtr);
    }

    // Let the calling function know if we did any processing or not.
    return true;
}




// -------------------------------------------------------------------------------------------------
/**
 *  The modem callback, this function is called whenever a SMS message is received from the modem.
 *  Our implementation here simply logs this as an informational message and appends the record to the
 *  chat log.
 *
 *  Also, if the incoming message is also a command, then this function will make sure it's handled.
 */
// -------------------------------------------------------------------------------------------------
static void SmsReceivedHandler
(
    le_sms_MsgRef_t messagePtr,   ///< [IN] Message object received from the modem.
    void*           contextPtr    ///< [IN] The handler's context.
)
{
    // First off, make sure this is a message that we can handle.
    LE_DEBUG("smsReceivedHandler called.");

    if (le_sms_GetFormat(messagePtr) != LE_SMS_FORMAT_TEXT)
    {
        LE_INFO("Non-text message received!");
        return;
    }

    // Now, extract the relavant information and record the message in the appropriate places.
    char tel[LE_MDMDEFS_PHONE_NUM_MAX_BYTES] = { 0 };
    char text[LE_SMS_TEXT_MAX_BYTES] = { 0 };

    le_sms_GetSenderTel(messagePtr, tel, LE_MDMDEFS_PHONE_NUM_MAX_BYTES);
    le_sms_GetText(messagePtr, text, LE_SMS_TEXT_MAX_BYTES);

    // We are now reporting to this person
    DestNumValid = true;
    strncpy(DestNum, tel, LE_MDMDEFS_PHONE_NUM_MAX_LEN);
    DestNum[LE_MDMDEFS_PHONE_NUM_MAX_LEN] = '\0';

    LE_INFO("Message: %s: %s", tel, text);

    le_sms_DeleteFromStorage(messagePtr);

    if (OutputFilePtr)
    {
        fprintf(OutputFilePtr, "recv (%s): %s\n", tel, text);
        fflush(OutputFilePtr);
    }

    // If this message was in fact a command, handle it now.
    ProcessCommand(text, tel);
}



// -------------------------------------------------------------------------------------------------
/**
 *  An event callback that is run every time the modem's state changes.  When the modem becomes
 *  attached to a network, a message is sent to a preconfigured target.  However the target and the
 *  message to be send have to be passed on program startup.
 */
// -------------------------------------------------------------------------------------------------
static void NetRegStateHandler
(
    le_mrc_NetRegState_t state,   ///< [IN] The new state of the modem.
    void*                contextPtr
)
{
    // Record the change of state to the chat log.
    if (OutputFilePtr)
    {
        fprintf(OutputFilePtr, "## %s ##\n", GetNetStateString(state));
        fflush(OutputFilePtr);
    }

    // For traceablity, make sure that this event is recorded.
    LE_DEBUG("%s", GetNetStateString(state));

    // If we are going back on net, and have been configured to do so, send our "on network" message
    // now.
    if (((state == LE_MRC_REG_HOME)
       || (state == LE_MRC_REG_ROAMING))
       && (DestNumValid))
    {
        LE_DEBUG("Sending On Network Message.");
        SendMessage(DestNum, "Getting back on network.");
    }
}



// -------------------------------------------------------------------------------------------------
/**
 *  Event callback for connection state changes.
 */
// -------------------------------------------------------------------------------------------------
static void ConnectionStateHandler
(
    const char *intfName,
    bool   isConnected,
    void*  contextPtr
)
{
    LE_INFO("Connection State Event: '%s' %s",
            intfName,
            (isConnected) ? "connected" : "not connected");

    if(!DestNumValid)
    {
        return;
    }

    if (!isConnected)
    {
        SendMessage(DestNum, "Data connection: not connected.");
    }
    else
    {
        SendMessage(DestNum, "Data connection: connected.");
    }
}



// -------------------------------------------------------------------------------------------------
/**
 *  Interface handler for dealing with SMS send requests.
 *
 *  If a request comes in, it is checked to see if it is a command.
 *  If the request is a command, it is processed and we are done.
 *  If the request isn't a command, it is sent as a SMS message to the requested recipient.
 */
// -------------------------------------------------------------------------------------------------
le_result_t send_SendMessage(const char* telPtr, const char* messagePtr)
{
    if (!ProcessCommand(messagePtr, NULL))
    {
        return SendMessage(telPtr, messagePtr);
    }

    return LE_OK;
}



// -------------------------------------------------------------------------------------------------
/**
 *  Our main function.
 *
 *  Start off by checking for any command line arguments, then go ahead and initialize the modem
 *  subsystem.  Once that is done, make sure that the chat log is good to go and that we are
 *  listening on our SMS request socket.
 */
// -------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    le_onoff_t power;
    le_result_t result;

    LE_INFO("Running modemDemo\n");

    // register network state handler
    le_mrc_AddNetRegStateEventHandler(NetRegStateHandler, NULL);

    // register sms handler
    le_sms_AddRxMessageHandler(SmsReceivedHandler, NULL);

    // Now, make sure that the radio has been turned on and is ready to go.
    if ((result = le_mrc_GetRadioPower(&power)) != LE_OK)
    {
        LE_WARN("Failed to get the radio power.  Result: %d", result);
    }
    else if (power == LE_OFF)
    {
        if ((result = le_mrc_SetRadioPower(LE_ON)) != LE_OK)
        {
            LE_FATAL("Failed to set the radio power.  Result: %d", result);
        }
    }

    // register handler for connection state change
    le_data_AddConnectionStateHandler(ConnectionStateHandler, NULL);

    // Open up our chat log.
    OutputFilePtr = fopen("smsChat.txt", "a");

    if (OutputFilePtr == NULL)
    {
        LE_ERROR("Output Open Failed: '%s'.", strerror(errno));
    }
    else
    {
        LE_DEBUG("Output Open Success.");
    }

}

