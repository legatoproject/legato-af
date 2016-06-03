/** @file pa_sms.c
 *
 * AT implementation of c_pa_sms API.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "legato.h"
#include "le_atClient.h"

#include "pa_sms.h"
#include "pa_sms_local.h"
#include "pa_utils_local.h"


//--------------------------------------------------------------------------------------------------
/**
 * Sms memory pool default value
 */
//--------------------------------------------------------------------------------------------------
#define DEFAULT_SMS_POOL_SIZE 1

//--------------------------------------------------------------------------------------------------
/**
 * Sms memory pool
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t       SmsPoolRef;

//--------------------------------------------------------------------------------------------------
/**
 * Unsolicited event identifier
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t          UnsolicitedEventId;

//--------------------------------------------------------------------------------------------------
/**
 * New sms event identifier
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t          NewSmsEventId;

//--------------------------------------------------------------------------------------------------
/**
 * New sms reference
 */
//--------------------------------------------------------------------------------------------------
static le_event_HandlerRef_t  SmsHandlerRef = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to check the CMTI unsolicited.
 *  parsing is "+CMTI: mem,index"
 *  parsing is "+CBMI: mem,index"
 *  parsing is "+CDSI: mem,index"
 *
 * @return true and index is filled.
 * @return false.
 */
//--------------------------------------------------------------------------------------------------
static bool GetSmsIndex
(
    char*     linePtr,    ///<  [IN] Line to parse
    uint32_t* indexPtr    ///< [OUT] Message Reference
)
{
    if (pa_utils_CountAndIsolateLineParameters(linePtr))
    {
        *indexPtr = atoi(pa_utils_IsolateLineParameter(linePtr,3));

        LE_DEBUG("SMS message index %d",*indexPtr);
        return true;
    }
    else
    {
        LE_WARN("SMS message index cannot be decoded %s",linePtr);
        return false;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to check SMS reception.
 *
 * @return true and index is filled.
 * @return false.
 */
//--------------------------------------------------------------------------------------------------
static bool CheckSmsUnsolicited
(
    char*     linePtr,      ///<  [IN] Line to parse
    uint32_t* indexPtr      ///< [OUT] Message reference in memory
)
{
    bool res = false;

    if (FIND_STRING("+CMTI:",linePtr))
    {
        res = GetSmsIndex(linePtr,indexPtr);
    }
    else if (FIND_STRING("+CBMI:",linePtr))
    {
        res = GetSmsIndex(linePtr,indexPtr);
    }
    else if (FIND_STRING("+CDSI:",linePtr))
    {
        res = GetSmsIndex(linePtr,indexPtr);
    }
    else
    {
        LE_DEBUG("this pattern is not expected -%s-",linePtr);
        res = false;
    }

    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to send event to all registered handler
 *
 */
//--------------------------------------------------------------------------------------------------
static void ReportMsgIndex
(
    uint32_t  index  ///< [IN] message reference
)
{
    pa_sms_NewMessageIndication_t messageIndication = {0};

    messageIndication.msgIndex = index;
    messageIndication.protocol = PA_SMS_PROTOCOL_GSM; // @TODO Hard-coded

    LE_DEBUG("Send new SMS Event with index %d in memory and protocol %d",
             messageIndication.msgIndex,
             messageIndication.protocol);
    le_event_Report(NewSmsEventId,&messageIndication, sizeof(messageIndication));
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register a handler for a new message reception handling.
 *
 */
//--------------------------------------------------------------------------------------------------
static void UnsolicitedSmsHandler
(
    void* reportPtr
)
{
    char*    unsolPtr = reportPtr;
    uint32_t msgIdx;

    if (CheckSmsUnsolicited(unsolPtr,&msgIdx))
    {
        ReportMsgIndex(msgIdx);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the sms module.
 *
 * @return LE_FAULT         The function failed to initialize the module.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_Init
(
    void
)
{
    UnsolicitedEventId = le_event_CreateId("UnsolicitedEventId",LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES);
    NewSmsEventId = le_event_CreateId("NewSmsEventId",sizeof(pa_sms_NewMessageIndication_t));

    le_event_AddHandler("UnsolicitedSmsHandler",UnsolicitedEventId  ,UnsolicitedSmsHandler);

    SmsPoolRef = le_mem_CreatePool("SmsPoolRef",sizeof(uint32_t));
    SmsPoolRef = le_mem_ExpandPool(SmsPoolRef,DEFAULT_SMS_POOL_SIZE);

    SmsHandlerRef = NULL;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register a handler for a new message reception handling.
 *
 * @return LE_BAD_PARAMETER The parameter is invalid.
 * @return LE_FAULT         The function failed to register a new handler.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_SetNewMsgHandler
(
    pa_sms_NewMsgHdlrFunc_t msgHandler ///< [IN] The handler function to handle a new message reception.
)
{
    LE_DEBUG("Set new SMS message handler");

    if (msgHandler == NULL)
    {
        LE_WARN("new SMS message handler is NULL");
        return LE_BAD_PARAMETER;
    }

    if (SmsHandlerRef != NULL)
    {
        LE_WARN("new SMS message handler has already been set");
        return LE_FAULT;
    }

    SmsHandlerRef = le_event_AddHandler("NewSMSHandler",
                                           NewSmsEventId,
                                           (le_event_HandlerFunc_t) msgHandler);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to unregister the handler for a new message reception handling.
 *
 * @return LE_FAULT         The function failed to unregister the handler.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_ClearNewMsgHandler
(
    void
)
{
    le_event_RemoveHandler(SmsHandlerRef);
    SmsHandlerRef = NULL;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize pattern matching for unsolicited message indicator.
 *
 */
//--------------------------------------------------------------------------------------------------
static void SetNewMsgIndicLocal
(
    pa_sms_NmiMt_t   mt,     ///< [IN] Result code indication routing for SMS-DELIVER indications.
    pa_sms_NmiBm_t   bm,     ///< [IN] Rules for storing the received CBMs (Cell Broadcast Message) types.
    pa_sms_NmiDs_t   ds      ///< [IN] SMS-STATUS-REPORTs routing.
)
{
    le_atClient_RemoveUnsolicitedResponseHandler(UnsolicitedEventId,"+CMTI:");
    le_atClient_RemoveUnsolicitedResponseHandler(UnsolicitedEventId,"+CMT:");
    le_atClient_RemoveUnsolicitedResponseHandler(UnsolicitedEventId,"+CBMI:");
    le_atClient_RemoveUnsolicitedResponseHandler(UnsolicitedEventId,"+CBM:");
    le_atClient_RemoveUnsolicitedResponseHandler(UnsolicitedEventId,"+CDS:");
    le_atClient_RemoveUnsolicitedResponseHandler(UnsolicitedEventId,"+CDSI:");

    switch (mt)
    {
        case PA_SMS_MT_0:
        {
            break;
        }
        case PA_SMS_MT_1:
        {
            le_atClient_AddUnsolicitedResponseHandler(UnsolicitedEventId,"+CMTI:",false);
            break;
        }
        case PA_SMS_MT_2:
        {
            le_atClient_AddUnsolicitedResponseHandler(UnsolicitedEventId,"+CMT:",true);
            break;
        }
        case PA_SMS_MT_3:
        {
            le_atClient_AddUnsolicitedResponseHandler(UnsolicitedEventId,"+CMTI:",false);
            le_atClient_AddUnsolicitedResponseHandler(UnsolicitedEventId,"+CMT:",true);
            break;
        }
        default:
        {
            LE_WARN("mt %d does not exist",mt);
        }
    }

    switch (bm)
    {
        case PA_SMS_BM_0:
        {
            break;
        }
        case PA_SMS_BM_1:
        {
            le_atClient_AddUnsolicitedResponseHandler(UnsolicitedEventId,"+CBMI:",false);
            break;
        }
        case PA_SMS_BM_2:
        {
            le_atClient_AddUnsolicitedResponseHandler(UnsolicitedEventId,"+CBM:",true);
            break;
        }
        case PA_SMS_BM_3:
        {
            le_atClient_AddUnsolicitedResponseHandler(UnsolicitedEventId,"+CBMI:",false);
            le_atClient_AddUnsolicitedResponseHandler(UnsolicitedEventId,"+CBM:",true);
            break;
        }
        default:
        {
            LE_WARN("bm %d does not exist",bm);
        }
    }

    switch (ds)
    {
        case PA_SMS_DS_0:
        {
            break;
        }
        case PA_SMS_DS_1:
        {
            le_atClient_AddUnsolicitedResponseHandler(UnsolicitedEventId,"+CDS:",true);
            break;
        }
        case PA_SMS_DS_2:
        {
            le_atClient_AddUnsolicitedResponseHandler(UnsolicitedEventId,"+CDSI:",false);
            break;
        }
        default:
        {
            LE_WARN("ds %d does not exist",ds);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function selects the procedure for message reception from the network (New Message
 * Indication settings).
 *
 * @return LE_FAULT         The function failed to select the procedure for message reception.
 * @return LE_TIMEOUT       No response was received from the Modem.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_SetNewMsgIndic
(
    pa_sms_NmiMode_t mode, ///< [IN] Processing of unsolicited result codes.
    pa_sms_NmiMt_t   mt,   ///< [IN] Result code indication routing for SMS-DELIVER indications.
    pa_sms_NmiBm_t   bm,   ///< [IN] Rules for storing the received Cell Broadcast Message types.
    pa_sms_NmiDs_t   ds,   ///< [IN] SMS-STATUS-REPORTs routing.
    pa_sms_NmiBfr_t  bfr   ///< [IN] TA buffer of unsolicited result codes mode.
)
{
    char                 command[LE_ATCLIENT_CMD_SIZE_MAX_LEN];
    le_atClient_CmdRef_t cmdRef = NULL;
    le_result_t          res    = LE_FAULT;

    SetNewMsgIndicLocal(mt,bm,ds);

    snprintf(command,LE_ATCLIENT_CMD_SIZE_MAX_LEN,"AT+CNMI=%d,%d,%d,%d,%d",mode,mt,bm,ds,bfr);

    res = le_atClient_SetCommandAndSend(&cmdRef,
                                        command,
                                        "",
                                        DEFAULT_AT_RESPONSE,
                                        DEFAULT_AT_CMD_TIMEOUT);
    if (res == LE_OK)
    {
        le_atClient_Delete(cmdRef);
    }
    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the New Message Indication settings.
 *
 * @return LE_FAULT         The function failed to get the New Message Indication settings.
 * @return LE_BAD_PARAMETER Bad parameter, one is NULL.
 * @return LE_TIMEOUT       No response was received from the Modem.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_GetNewMsgIndic
(
    pa_sms_NmiMode_t* modePtr, ///< [OUT] Processing of unsolicited result codes.
    pa_sms_NmiMt_t*   mtPtr,   ///< [OUT] Result code indication routing for SMS-DELIVER indications.
    pa_sms_NmiBm_t*   bmPtr,   ///< [OUT] Rules for storing the received Cell Broadcast Message types.
    pa_sms_NmiDs_t*   dsPtr,   ///< [OUT] SMS-STATUS-REPORTs routing.
    pa_sms_NmiBfr_t*  bfrPtr   ///< [OUT] Terminal Adaptor buffer of unsolicited result codes mode.
)
{
    char*                tokenPtr = NULL;
    char*                savePtr  = NULL;
    le_atClient_CmdRef_t cmdRef   = NULL;
    le_result_t          res      = LE_FAULT;
    char                 intermediateResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];
    char                 finalResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];

    if (!modePtr && !mtPtr && !bmPtr && !dsPtr && !bfrPtr)
    {
        LE_WARN("One parameter is NULL");
        return LE_BAD_PARAMETER;
    }

    res = le_atClient_SetCommandAndSend(&cmdRef,
                                        "AT+CNMI?",
                                        "+CNMI:",
                                        DEFAULT_AT_RESPONSE,
                                        DEFAULT_AT_CMD_TIMEOUT);
    if (res != LE_OK)
    {
        LE_ERROR("Function failed !");
        return res;
    }

    res = le_atClient_GetFinalResponse(cmdRef,
                                       finalResponse,
                                       LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES);

    if ((res != LE_OK) || (strcmp(finalResponse, "OK") != 0))
    {
        LE_ERROR("Function failed !");
        le_atClient_Delete(cmdRef);
        return res;
    }

    res = le_atClient_GetFirstIntermediateResponse(cmdRef,
                                                   intermediateResponse,
                                                   LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES);
    if (res != LE_OK)
    {
        LE_ERROR("Failed to get the intermediateResponse");
    }
    else
    {
        tokenPtr = strtok_r(intermediateResponse+strlen("+CNMI: "), ",", &savePtr);
        *modePtr = (pa_sms_NmiMode_t)atoi(tokenPtr);

        tokenPtr = strtok_r(NULL, ",", &savePtr);
        *mtPtr   = (pa_sms_NmiMt_t)  atoi(tokenPtr);

        tokenPtr = strtok_r(NULL, ",", &savePtr);
        *bmPtr   = (pa_sms_NmiBm_t)  atoi(tokenPtr);

        tokenPtr = strtok_r(NULL, ",", &savePtr);
        *dsPtr   = (pa_sms_NmiDs_t)  atoi(tokenPtr);

        tokenPtr = strtok_r(NULL, ",", &savePtr);
        *bfrPtr  = (pa_sms_NmiBfr_t) atoi(tokenPtr);
    }
    le_atClient_Delete(cmdRef);
    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function sets the Preferred Message Format (PDU or Text mode).
 *
 * @return LE_FAULT         The function failed to sets the Preferred Message Format.
 * @return LE_TIMEOUT       No response was received from the Modem.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_SetMsgFormat
(
    le_sms_Format_t   format   ///< [IN] The preferred message format.
)
{
    char                 command[LE_ATCLIENT_CMD_SIZE_MAX_LEN];
    char                 finalResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];
    le_atClient_CmdRef_t cmdRef = NULL;
    le_result_t          res    = LE_FAULT;

    snprintf(command,LE_ATCLIENT_CMD_SIZE_MAX_LEN,"AT+CMGF=%d",format);

    res = le_atClient_SetCommandAndSend(&cmdRef,
                                        command,
                                        "",
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
        LE_ERROR("Failed to get the response");
    }

    le_atClient_Delete(cmdRef);
    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function sends a message in PDU mode.
 *
 * @return LE_FAULT           The function failed to send a message in PDU mode.
 * @return LE_BAD_PARAMETER   The parameters are invalid.
 * @return LE_TIMEOUT         No response was received from the Modem.
 * @return a positive value   The function succeeded. The value represents the message reference.
 */
//--------------------------------------------------------------------------------------------------
int32_t pa_sms_SendPduMsg
(
    pa_sms_Protocol_t        protocol,   ///< [IN] protocol to use
    uint32_t                 length,     ///< [IN] The length of the TP data unit in bytes.
    const uint8_t           *dataPtr,    ///< [IN] The message.
    uint32_t                 timeout,    ///< [IN] Timeout in seconds.
    pa_sms_SendingErrCode_t *errorCode   ///< [OUT] The error code.
)
{
    char                 command[LE_ATCLIENT_CMD_SIZE_MAX_LEN];
    char                 intermediateResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];
    char                 finalResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];
    char                 hexString[LE_SMS_PDU_MAX_BYTES*2+1] = {0};
    uint32_t             hexStringSize;
    le_atClient_CmdRef_t cmdRef = NULL;
    int32_t              res    = LE_FAULT;

    if (!dataPtr)
    {
        LE_WARN("One parameter is NULL");
        return LE_BAD_PARAMETER;
    }

    snprintf(command,LE_ATCLIENT_CMD_SIZE_MAX_LEN,"AT+CMGS=%d",length-1);

    hexStringSize = le_hex_BinaryToString(dataPtr,length,hexString,sizeof(hexString));
    LE_INFO("Pdu string: %s, size = %d", hexString, hexStringSize);

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
    res = le_atClient_SetData(cmdRef,hexString,hexStringSize);
    if (res != LE_OK)
    {
        le_atClient_Delete(cmdRef);
        LE_ERROR("Failed to set data !");
        return res;
    }
    res = le_atClient_SetIntermediateResponse(cmdRef,"+CMGS:");
    if (res != LE_OK)
    {
        le_atClient_Delete(cmdRef);
        LE_ERROR("Failed to set intermediate response !");
        return res;
    }
    res = le_atClient_SetFinalResponse(cmdRef,"OK|ERROR|+CME ERROR:|+CMS ERROR:");
    if (res != LE_OK)
    {
        le_atClient_Delete(cmdRef);
        LE_ERROR("Failed to set final response !");
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
        if ((res != LE_OK) || (strcmp(finalResponse,"OK") != 0))
        {
            LE_ERROR("Failed to get the finalResponse");
            le_atClient_Delete(cmdRef);
            return res;
        }

        res = le_atClient_GetFirstIntermediateResponse(cmdRef,
                                                       intermediateResponse,
                                                       LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES);
        if (res != LE_OK)
        {
            LE_ERROR("Failed to get the intermediateResponse");
            le_atClient_Delete(cmdRef);
            return res;
        }

        uint32_t numParam = pa_utils_CountAndIsolateLineParameters(intermediateResponse);

        if (FIND_STRING("+CMGS:",pa_utils_IsolateLineParameter(intermediateResponse,1)))
        {
            if (numParam == 2)
            {
                res = atoi(pa_utils_IsolateLineParameter(intermediateResponse,2));
            }
            else
            {
                LE_WARN("this pattern is not expected");
                res = LE_FAULT;
            }
        }
        else
        {
            LE_WARN("this pattern is not expected");
            res = LE_FAULT;
        }
    }

    le_atClient_Delete(cmdRef);
    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the message from the preferred message storage.
 *
 * @return LE_FAULT        The function failed to get the message from the preferred message
 *                         storage.
 * @return LE_TIMEOUT      No response was received from the Modem.
 * @return LE_OK           The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_RdPDUMsgFromMem
(
    uint32_t            index,      ///< [IN] The place of storage in memory.
    pa_sms_Protocol_t   protocol,   ///< [IN] The protocol used.
    pa_sms_Storage_t    storage,    ///< [IN] SMS Storage used.
    pa_sms_Pdu_t*       msgPtr      ///< [OUT] The message.
)
{
    char                 command[LE_ATCLIENT_CMD_SIZE_MAX_LEN];
    char*                tokenPtr = NULL;
    le_atClient_CmdRef_t cmdRef   = NULL;
    le_result_t          res      = LE_FAULT;
    char                 intermediateResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];
    char                 finalResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];

    if (!msgPtr)
    {
        LE_WARN("One parameter is NULL");
        return LE_BAD_PARAMETER;
    }

    snprintf(command,LE_ATCLIENT_CMD_SIZE_MAX_LEN,"AT+CMGR=%d",index);

    res = le_atClient_SetCommandAndSend(&cmdRef,
                                        command,
                                        "+CMGR:|0|1|2|3|4|5|6|7|8|9",
                                        "OK|ERROR|+CME ERROR:|+CMS ERROR:",
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
        LE_ERROR("Failed to get the response");
        le_atClient_Delete(cmdRef);
        return res;
    }

    res = le_atClient_GetFirstIntermediateResponse(cmdRef,
                                                   intermediateResponse,
                                                   LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES);
    if (res != LE_OK)
    {
        LE_ERROR("Failed to get the response");
        le_atClient_Delete(cmdRef);
        return res;
    }
    else
    {
        tokenPtr = strtok(intermediateResponse+strlen("+CMGR: "), ",");
        msgPtr->status=(le_sms_Status_t)atoi(tokenPtr);
        msgPtr->protocol = PA_SMS_PROTOCOL_GSM;
    }

    res = le_atClient_GetNextIntermediateResponse(cmdRef,
                                                  intermediateResponse,
                                                  LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES);
    if (res != LE_OK)
    {
        LE_ERROR("Failed to get the response");
    }
    else
    {
        int32_t dataSize = le_hex_StringToBinary(intermediateResponse,
                                                 strlen(intermediateResponse),
                                                 msgPtr->data,
                                                 LE_SMS_PDU_MAX_BYTES);
        if (dataSize < 0)
        {
            LE_ERROR("Message cannot be converted");
            res = LE_FAULT;
        }
        else
        {
            LE_DEBUG("Fill message in binary mode");
            msgPtr->dataLen = dataSize;
            res = LE_OK;
        }

        LE_INFO("Message PDU = %s",intermediateResponse);
    }

    le_atClient_Delete(cmdRef);
    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the indexes of messages stored in the preferred memory for a specific
 * status.
 *
 * @return LE_FAULT          The function failed to get the indexes of messages stored in the
 *                           preferred memory.
 * @return LE_BAD_PARAMETER  The parameters are invalid.
 * @return LE_TIMEOUT        No response was received from the Modem.
 * @return LE_OK             The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_ListMsgFromMem
(
    le_sms_Status_t     status,     ///< [IN] The status of message in memory.
    pa_sms_Protocol_t   protocol,   ///< [IN] The protocol to read.
    uint32_t           *numPtr,     ///< [OUT] The number of indexes retrieved.
    uint32_t           *idxPtr,     ///< [OUT] The pointer to an array of indexes.
                                    ///        The array is filled with 'num' index values.
    pa_sms_Storage_t    storage     ///< [IN] SMS Storage used.
)
{
    char                 command[LE_ATCLIENT_CMD_SIZE_MAX_LEN];
    char*                tokenPtr = NULL;
    le_atClient_CmdRef_t cmdRef   = NULL;
    le_result_t          res      = LE_OK;
    char                 intermediateResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];
    char                 finalResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];

    if (!numPtr && !idxPtr)
    {
        LE_WARN("One parameter is NULL");
        return LE_BAD_PARAMETER;
    }

    *numPtr = 0;

    if ((storage != PA_SMS_STORAGE_SIM) || (protocol != PA_SMS_PROTOCOL_GSM))
    {
        return res;
    }

    if (status == LE_SMS_RX_READ)
    {
        snprintf(command,LE_ATCLIENT_CMD_SIZE_MAX_LEN,"AT+CMGL=1");
    }
    else if (status == LE_SMS_RX_UNREAD)
    {
        snprintf(command,LE_ATCLIENT_CMD_SIZE_MAX_LEN,"AT+CMGL=0");
    }
    else
    {
        return LE_FAULT;
    }

    res = le_atClient_SetCommandAndSend(&cmdRef,
                                        command,
                                        "+CMGL:",
                                        "OK|ERROR|+CME ERROR:|+CMS ERROR:",
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
        LE_ERROR("Failed to get the response");
        le_atClient_Delete(cmdRef);
        return LE_FAULT;
    }

    res = le_atClient_GetFirstIntermediateResponse(cmdRef,
                                                   intermediateResponse,
                                                   LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES);
    if (res != LE_OK)
    {
        LE_ERROR("Failed to get the response");
    }
    else
    {
        uint32_t cpt = 0;

        while(strcmp(intermediateResponse,"OK") != 0)
        {
            tokenPtr    = strtok(intermediateResponse+strlen("+CMGL: "), ",");
            idxPtr[cpt] = atoi(tokenPtr);
            (*numPtr)++;
            cpt += 1;

            res = le_atClient_GetNextIntermediateResponse(cmdRef,
                                                        intermediateResponse,
                                                        LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES);
        }
    }
    le_atClient_Delete(cmdRef);
    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function deletes one specific Message from preferred message storage.
 *
 * @return LE_FAULT          The function failed to delete one specific Message from preferred
 *                           message storage.
 * @return LE_TIMEOUT        No response was received from the Modem.
 * @return LE_OK             The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_DelMsgFromMem
(
    uint32_t            index,    ///< [IN] Index of the message to be deleted.
    pa_sms_Protocol_t   protocol, ///< [IN] protocol.
    pa_sms_Storage_t    storage   ///< [IN] SMS Storage used..
)
{
    char                 command[LE_ATCLIENT_CMD_SIZE_MAX_LEN];
    char                 finalResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];
    le_atClient_CmdRef_t cmdRef = NULL;
    le_result_t          res    = LE_FAULT;

    snprintf(command,LE_ATCLIENT_CMD_SIZE_MAX_LEN,"AT+CMGD=%d,0",index);

    res = le_atClient_SetCommandAndSend(&cmdRef,
                                        command,
                                        "",
                                        "OK|ERROR|+CME ERROR:|+CMS ERROR:",
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
        LE_ERROR("Failed to get the response");
    }

    le_atClient_Delete(cmdRef);
    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function deletes all Messages from preferred message storage.
 *
 * @return LE_FAULT        The function failed to delete all Messages from preferred message storage.
 * @return LE_TIMEOUT      No response was received from the Modem.
 * @return LE_OK           The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_DelAllMsg
(
    void
)
{
    le_atClient_CmdRef_t cmdRef = NULL;
    le_result_t          res    = LE_FAULT;
    char                 finalResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];

    res = le_atClient_SetCommandAndSend(&cmdRef,
                                        "AT+CMGD=0,4",
                                        "",
                                        "OK|ERROR|+CME ERROR:|+CMS ERROR:",
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
        LE_ERROR("Failed to get the response");
    }

    le_atClient_Delete(cmdRef);
    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function saves the SMS Settings.
 *
 * @return LE_FAULT        The function failed to save the SMS Settings.
 * @return LE_TIMEOUT      No response was received from the Modem.
 * @return LE_OK           The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_SaveSettings
(
    void
)
{
    le_atClient_CmdRef_t cmdRef = NULL;
    le_result_t          res    = LE_FAULT;
    char                 finalResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];

    res = le_atClient_SetCommandAndSend(&cmdRef,
                                        "AT+CSAS",
                                        "",
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
        LE_ERROR("Failed to get the response");
    }

    le_atClient_Delete(cmdRef);
    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function restores the SMS Settings.
 *
 * @return LE_FAULT        The function failed to restore the SMS Settings.
 * @return LE_TIMEOUT      No response was received from the Modem.
 * @return LE_OK           The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_RestoreSettings
(
    void
)
{
    le_atClient_CmdRef_t cmdRef = NULL;
    le_result_t          res    = LE_FAULT;
    char                 finalResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];

    res = le_atClient_SetCommandAndSend(&cmdRef,
                                        "AT+CRES",
                                        "",
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
        LE_ERROR("Failed to get the response");
    }

    le_atClient_Delete(cmdRef);
    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function changes the message status.
 *
 * @return LE_FAULT        The function failed to change the message status.
 * @return LE_TIMEOUT      No response was received from the Modem.
 * @return LE_OK           The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_ChangeMessageStatus
(
    uint32_t            index,    ///< [IN] Index of the message to be deleted.
    pa_sms_Protocol_t   protocol, ///< [IN] protocol.
    le_sms_Status_t     status,   ///< [IN] The status of message in memory.
    pa_sms_Storage_t    storage   ///< [IN] SMS Storage used.
)
{
    LE_ERROR("Impossible to change the SMS status !");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the SMS center.
 *
 * @return
 *   - LE_FAULT        The function failed.
 *   - LE_TIMEOUT      No response was received from the Modem.
 *   - LE_OK           The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_GetSmsc
(
    char*        smscPtr,  ///< [OUT] The Short message service center string.
    size_t       len       ///< [IN] The length of SMSC string.
)
{
    le_result_t          res      = LE_FAULT;
    le_atClient_CmdRef_t cmdRef   = NULL;
    char*                tokenPtr = NULL;
    char*                savePtr  = NULL;
    char                 intermediateResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];
    char                 finalResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];

    if (!smscPtr)
    {
        LE_DEBUG("One parameter is NULL");
        return LE_BAD_PARAMETER;
    }

    res = le_atClient_SetCommandAndSend(&cmdRef,
                                        "AT+CSCA?",
                                        "+CSCA:",
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
        LE_ERROR("Failed to get the response");
        le_atClient_Delete(cmdRef);
        return res;
    }

    res = le_atClient_GetFirstIntermediateResponse(cmdRef,
                                                   intermediateResponse,
                                                   LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES);
    if (res != LE_OK)
    {
        LE_ERROR("Failed to get the response");
        le_atClient_Delete(cmdRef);
        return res;
    }

    // Cut the string for keep just the phone number
    tokenPtr = strtok_r(intermediateResponse, "\"", &savePtr);
    tokenPtr = strtok_r(NULL, "\"", &savePtr);
    strncpy(smscPtr, tokenPtr, len);

    le_atClient_Delete(cmdRef);
    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the SMS center.
 *
 * @return
 *   - LE_FAULT        The function failed.
 *   - LE_TIMEOUT      No response was received from the Modem.
 *   - LE_OK           The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_SetSmsc
(
    const char*    smscPtr  ///< [IN] The Short message service center.
)
{
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Activate Cell Broadcast message notification
 *
 * @return
 *  - LE_FAULT         Function failed.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_ActivateCellBroadcast
(
    pa_sms_Protocol_t protocol
)
{
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Deactivate Cell Broadcast message notification
 *
 * @return
 *  - LE_FAULT         Function failed.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_DeactivateCellBroadcast
(
    pa_sms_Protocol_t protocol
)
{
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Add Cell Broadcast message Identifiers range.
 *
 * @return
 *  - LE_FAULT         Function failed.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_AddCellBroadcastIds
(
    uint16_t fromId,    ///< [IN] Starting point of the range of cell broadcast message identifier.
    uint16_t toId       ///< [IN] Ending point of the range of cell broadcast message identifier.
)
{
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove Cell Broadcast message Identifiers range.
 *
 * @return
 *  - LE_FAULT         Function failed.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_RemoveCellBroadcastIds
(
    uint16_t fromId,    ///< [IN] Starting point of the range of cell broadcast message identifier.
    uint16_t toId       ///< [IN] Ending point of the range of cell broadcast message identifier.
)
{
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Add CDMA Cell Broadcast category services.
 *
 * @return
 *  - LE_FAULT         Function failed.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_AddCdmaCellBroadcastServices
(
    le_sms_CdmaServiceCat_t serviceCat, ///< [IN] Service category assignment. Reference to 3GPP2 C.R1001-D
                                        ///       v1.0 Section 9.3.1 Standard Service Category Assignments.
    le_sms_Languages_t language         ///< [IN] Language Indicator. Reference to
                                        ///       3GPP2 C.R1001-D v1.0 Section 9.2.1 Language Indicator
                                        ///       Value Assignments
)
{
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove CDMA Cell Broadcast category services.
 *
 * @return
 *  - LE_FAULT         Function failed.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_RemoveCdmaCellBroadcastServices
(
    le_sms_CdmaServiceCat_t serviceCat, ///< [IN] Service category assignment. Reference to 3GPP2 C.R1001-D
                                        /// v1.0 Section 9.3.1 Standard Service Category Assignments.
    le_sms_Languages_t language         ///< [IN] Language Indicator. Reference to
                                        ///       3GPP2 C.R1001-D v1.0 Section 9.2.1 Language Indicator
                                        ///       Value Assignments
)
{
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Clear Cell Broadcast message Identifiers range.
 *
 * @return
 *  - LE_FAULT         Function failed.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_ClearCellBroadcastIds
(
    void
)
{
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Clear CDMA Cell Broadcast category services.
 *
 * @return
 *  - LE_FAULT         Function failed.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_ClearCdmaCellBroadcastServices
(
    void
)
{
    return LE_FAULT;
}
