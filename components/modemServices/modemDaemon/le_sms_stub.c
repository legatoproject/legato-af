/** @file le_sms_stub.c
 *
 * This file implements some stubs for the sms service.
 *
 * Copyright (C) Sierra Wireless Inc.
 */


#include "legato.h"
#include "interfaces.h"
#include "pa_sms.h"
#include "pa_sim.h"
#include "smsPdu.h"
#include "time.h"
#include "mdmCfgEntries.h"
#include "le_ms_local.h"

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to create an SMS Message data structure.
 *
 * @return A reference to the new Message object.
 *
 * @note
 *      On failure, the process exits, so you don't have to worry about checking the returned
 *      reference for validity.
 */
//--------------------------------------------------------------------------------------------------
le_sms_MsgRef_t le_sms_Create
(
    void
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the timeout to send a SMS Message.
 *
 * @return
 * - LE_FAULT         Message is not in UNSENT state or is Read-Only.
 * - LE_OK            Function succeeded.
 *
 * @note
 *      On failure, the process exits, so you don't have to worry about checking the returned
 *      reference for validity.
 *
 * @deprecated
 *      This API should not be used for new applications and will be removed in a future version
 *      of Legato.
 */
 //--------------------------------------------------------------------------------------------------
le_result_t le_sms_SetTimeout
(
    le_sms_MsgRef_t msgRef,
        ///< [IN]
        ///< Reference to the message object.

    uint32_t timeout
        ///< [IN]
        ///< Timeout in seconds.
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to delete a Message data structure.
 *
 * It deletes the Message data structure, all the allocated memory is freed. However if several
 * Users own the Message object (for example in the case of several handler functions registered for
 * SMS message reception) the Message object will be actually deleted only if it remains one User
 * owning the Message object.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_sms_Delete
(
    le_sms_MsgRef_t  msgRef    ///< [IN] The pointer to the message data structure.
)
{
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the message format.
 *
 * @return The message format.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_sms_Format_t le_sms_GetFormat
(
    le_sms_MsgRef_t    msgRef   ///< [IN] The pointer to the message data structure.
)
{
    return LE_SMS_FORMAT_UNKNOWN;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the message type.
 *
 * @return
 *  - Message type.
 *  - LE_BAD_PARAMETER Invalid reference provided.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_sms_Type_t le_sms_GetType
(
    le_sms_MsgRef_t msgRef
        ///< [IN]
        ///< Reference to the message object.
)
{
    return LE_SMS_TYPE_STATUS_REPORT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the Cell Broadcast Message Identifier.
 *
 * @return
 * - LE_FAULT       Message is not a cell broadcast type.
 * - LE_OK          Function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_GetCellBroadcastId
(
    le_sms_MsgRef_t msgRef,
        ///< [IN]
        ///< Reference to the message object.

    uint16_t* messageIdPtr
        ///< [OUT]
        ///< Cell Broadcast Message Identifier.
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the Cell Broadcast Message Serial Number.
 *
 * @return
 * - LE_FAULT       Message is not a cell broadcast type.
 * - LE_OK          Function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_GetCellBroadcastSerialNumber
(
    le_sms_MsgRef_t msgRef,
        ///< [IN]
        ///< Reference to the message object.

    uint16_t* serialNumberPtr
        ///< [OUT]
        ///< Cell Broadcast Serial Number.
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the Telephone destination number.
 *
 * The Telephone number is defined in ITU-T recommendations E.164/E.163.
 * E.164 numbers can have a maximum of fifteen digits and are usually written with a '+' prefix.
 *
 * @return LE_NOT_PERMITTED The message is Read-Only.
 * @return LE_BAD_PARAMETER The Telephone destination number length is equal to zero.
 * @return LE_OK            The function succeeded.
 *
 * @note If telephone destination number is too long is too long (max LE_MDMDEFS_PHONE_NUM_MAX_LEN
 *       digits), it is a fatal error, the function will not return.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_SetDestination
(
    le_sms_MsgRef_t   msgRef,  ///< [IN] The pointer to the message data structure.
    const char*       destPtr  ///< [IN] The telephone number string.
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the Sender Telephone number.
 *
 * The output parameter is updated with the Telephone number. If the Telephone number string exceed
 * the value of 'len' parameter, a LE_OVERFLOW error code is returned and 'tel' is filled until
 * 'len-1' characters and a null-character is implicitly appended at the end of 'tel'.
 *
 * @return LE_NOT_PERMITTED The message is not a received message.
 * @return LE_OVERFLOW      The Telephone number length exceed the maximum length.
 * @return LE_OK            The function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_GetSenderTel
(
    le_sms_MsgRef_t  msgRef,  ///< [IN] The pointer to the message data structure.
    char*            telPtr,  ///< [OUT] The telephone number string.
    size_t           len      ///< [IN] The length of telephone number string.
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the Service Center Time Stamp string.
 *
 * The output parameter is updated with the Time Stamp string. If the Time Stamp string exceed the
 * value of 'len' parameter, a LE_OVERFLOW error code is returned and 'timestamp' is filled until
 * 'len-1' characters and a null-character is implicitly appended at the end of 'timestamp'.
 *
 * @return LE_NOT_PERMITTED The message is not a received message.
 * @return LE_OVERFLOW      The Timestamp number length exceed the maximum length.
 * @return LE_OK            The function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_GetTimeStamp
(
    le_sms_MsgRef_t  msgRef,       ///< [IN] The pointer to the message data structure.
    char*            timestampPtr, ///< [OUT] The message time stamp (in text mode).
    ///        string format: "yy/MM/dd,hh:mm:ss+/-zz"
    ///        (Year/Month/Day,Hour:Min:Seconds+/-TimeZone)
    /// The time zone indicates the difference, expressed in quarters of an hours between the
    ///  local time and GMT.
    size_t           len           ///< [IN] The length of timestamp string.
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the message Length value.
 *
 * @return The number of characters for text messages, or the length of the data in bytes for raw
 *         binary messages.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
size_t le_sms_GetUserdataLen
(
    le_sms_MsgRef_t msgRef ///< [IN] The pointer to the message data structure.
)
{
    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the message Length value.
 *
 * @return The length of the data in bytes of the PDU message.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
size_t le_sms_GetPDULen
(
    le_sms_MsgRef_t msgRef ///< [IN] The pointer to the message data structure.
)
{
    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the UCS2 Message (16-bit format).
 *
 * Output parameters are updated with the UCS2 message content and the number of characters. If
 * the UCS2 data exceed the value of the length input parameter, a LE_OVERFLOW error
 * code is returned and 'ucs2Ptr' is filled until of the number of chars specified.
 *
 * @return
 *  - LE_FORMAT_ERROR  Message is not in binary format.
 *  - LE_OVERFLOW      Message length exceed the maximum length.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_GetUCS2
(
    le_sms_MsgRef_t msgRef,
        ///< [IN]
        ///< Reference to the message object.

    uint16_t* ucs2Ptr,
        ///< [OUT]
        ///< UCS2 message.

    size_t* ucs2NumElementsPtr
        ///< [INOUT]
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Create and asynchronously send a text message.
 *
 * @return
 *  - le_sms_Msg Reference to the new Message object pooled.
 *  - NULL Not possible to pool a new message.
 *
 * @note If telephone destination number is too long is too long (max LE_MDMDEFS_PHONE_NUM_MAX_LEN
 *       digits), it is a fatal error, the function will not return.
 * @note If message is too long (max LE_SMS_TEXT_MAX_LEN digits), it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_sms_MsgRef_t le_sms_SendText
(
    const char* destStr,
    ///< [IN]
    ///< Telephone number string.

    const char* textStr,
    ///< [IN]
    ///< SMS text.

    le_sms_CallbackResultFunc_t handlerPtr,
    ///< [IN]

    void* contextPtr
    ///< [IN]
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Create and asynchronously send a PDU message.
 *
 * @return
 *  - le_sms_Msg Reference to the new Message object pooled.
 *  - NULL Not possible to pool a new message.
 *
 */
//--------------------------------------------------------------------------------------------------
le_sms_MsgRef_t le_sms_SendPdu
(
    const uint8_t* pduPtr,
    ///< [IN]
    ///< PDU message.

    size_t pduNumElements,
    ///< [IN]

    le_sms_CallbackResultFunc_t handlerPtr,
    ///< [IN]

    void* contextPtr
    ///< [IN]
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the Text Message content.
 *
 * @return LE_NOT_PERMITTED The message is Read-Only.
 * @return LE_BAD_PARAMETER The text message length is equal to zero.
 * @return LE_OK            The function succeeded.
 *
 * @note Text Message is encoded in ASCII format (ISO8859-15) and characters have to exist in
 *  the GSM 23.038 7 bit alphabet.
 *
 * @note If message is too long (max LE_SMS_TEXT_MAX_LEN digits), it is a fatal error, the
 *       function will not return.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_SetText
(
    le_sms_MsgRef_t       msgRef, ///< [IN] The pointer to the message data structure.
    const char*           textPtr ///< [IN] The SMS text.
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the binary message content.
 *
 * @return LE_NOT_PERMITTED The message is Read-Only.
 * @return LE_BAD_PARAMETER The length of the data is equal to zero.
 * @return LE_OK            The function succeeded.
 *
 * @note If length of the data is too long (max LE_SMS_BINARY_MAX_BYTES bytes), it is a fatal
 *       error, the function will not return.

 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_SetBinary
(
    le_sms_MsgRef_t   msgRef, ///< [IN] The pointer to the message data structure.
    const uint8_t*    binPtr, ///< [IN] The binary data.
    size_t            len     ///< [IN] The length of the data in bytes.
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the PDU message content.
 *
 * @return LE_NOT_PERMITTED The message is Read-Only.
 * @return LE_BAD_PARAMETER The length of the data is equal to zero.
 * @return LE_OK            The function succeeded.
 *
 * @note If length of the data is too long (max LE_SMS_PDU_MAX_BYTES bytes), it is a fatal error,
 *       the function will not return.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_SetPDU
(
    le_sms_MsgRef_t   msgRef, ///< [IN] The pointer to the message data structure.
    const uint8_t*    pduPtr, ///< [IN] The PDU message.
    size_t            len     ///< [IN] The length of the data in bytes.
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the UCS2 message content (16 bit format).
 *
 * @return
 *  - LE_NOT_PERMITTED Message is Read-Only.
 *  - LE_BAD_PARAMETER Length of the data is equal to zero.
 *  - LE_OK            Function succeeded.
 *
 * @note If length of the data is too long (max LE_SMS_UCS2_MAX_CHARS), it is a fatal
 *       error, the function will not return.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_SetUCS2
(
    le_sms_MsgRef_t msgRef,
        ///< [IN]
        ///< Reference to the message object.

    const uint16_t* ucs2Ptr,
        ///< [IN]
        ///< UCS2 message.

    size_t ucs2NumElements
        ///< [IN]
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the text Message.
 *
 * Output parameter is updated with the text string encoded in ASCII format. If the text string
 * exceeds the value of 'len' parameter, LE_OVERFLOW error code is returned and 'text' is filled
 * until 'len-1' characters and a null-character is implicitly appended at the end of 'text'.
 *
 * @return LE_OVERFLOW      The message length exceed the maximum length.
 * @return LE_OK            The function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_GetText
(
    le_sms_MsgRef_t  msgRef,  ///< [IN]  The pointer to the message data structure.
    char*            textPtr, ///< [OUT] The SMS text.
    size_t           len      ///< [IN] The maximum length of the text message.
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the binary Message.
 *
 * The output parameters are updated with the binary message content and the length of the raw
 * binary message in bytes. If the binary data exceed the value of 'len' input parameter, a
 * LE_OVERFLOW error code is returned and 'raw' is filled until 'len' bytes.
 *
 * @return LE_FORMAT_ERROR  Message is not in binary format.
 * @return LE_OVERFLOW      The message length exceed the maximum length.
 * @return LE_OK            The function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_GetBinary
(
    le_sms_MsgRef_t  msgRef, ///< [IN]  The pointer to the message data structure.
    uint8_t*         binPtr, ///< [OUT] The binary message.
    size_t*          lenPtr  ///< [IN,OUT] The length of the binary message in bytes.
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the PDU message.
 *
 * The output parameters are updated with the PDU message content and the length of the PDU message
 * in bytes. If the PDU data exceed the value of 'len' input parameter, a LE_OVERFLOW error code is
 * returned and 'pdu' is filled until 'len' bytes.
 *
 * @return LE_FORMAT_ERROR  Unable to encode the message in PDU (only for outgoing messages).
 * @return LE_OVERFLOW      The message length exceed the maximum length.
 * @return LE_OK            The function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_GetPDU
(
    le_sms_MsgRef_t  msgRef, ///< [IN]  The pointer to the message data structure.
    uint8_t*         pduPtr, ///< [OUT] The PDU message.
    size_t*          lenPtr  ///< [IN,OUT] The length of the PDU message in bytes.
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register a handler function for SMS full storage
 * message reception.
 *
 * @return A handler reference, which is only needed for later removal of the handler.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_sms_FullStorageEventHandlerRef_t le_sms_AddFullStorageEventHandler
(
    le_sms_FullStorageHandlerFunc_t handlerFuncPtr, ///< [IN] The handler function for SMS
                                                      ///  full storage message indication.
    void*                              contextPtr     ///< [IN] The handler's context.
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to unregister a handler function.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
void le_sms_RemoveFullStorageEventHandler
(
    le_sms_FullStorageEventHandlerRef_t   handlerRef ///< [IN] The handler reference.
)
{
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to send an SMS message.
 *
 * It verifies first if the parameters are valid, then it checks that the modem state can support
 * message sending.
 *
 * @return LE_FORMAT_ERROR  The message content is invalid.
 * @return LE_FAULT         The function failed to send the message.
 * @return LE_OK            The function succeeded.
 * @return LE_TIMEOUT       Timeout before the complete sending.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_Send
(
    le_sms_MsgRef_t    msgRef         ///< [IN] The message(s) to send.
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Send an asynchronous SMS message.
 *
 * Verifies first if the parameters are valid, then it checks the modem state can support
 * message sending.
 *
 * @return LE_FORMAT_ERROR  Message content is invalid.
 * @return LE_FAULT         Function failed to send the message.
 * @return LE_OK            Function succeeded.
 * @return LE_TIMEOUT       Timeout before the complete sending.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_SendAsync
(
    le_sms_MsgRef_t msgRef,
        ///< [IN]
        ///< Reference to the message object.

    le_sms_CallbackResultFunc_t handlerPtr,
        ///< [IN]

    void* contextPtr
        ///< [IN]
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the error code when a 3GPP2 message sending has Failed.
 *
 * @return
 *  - The error code
 *  - LE_BAD_PARAMETER Invalid reference provided.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 *
 * @note It is only applicable for 3GPP2 message sending failure, otherwise
 *       LE_SMS_ERROR_3GPP2_MAX is returned.
 */
//--------------------------------------------------------------------------------------------------
le_sms_ErrorCode3GPP2_t le_sms_Get3GPP2ErrorCode
(
    le_sms_MsgRef_t msgRef
        ///< [IN]
        ///< Reference to the message object.
)
{
    return LE_SMS_ERROR_3GPP2_MAX;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the Radio Protocol and the Transfer Protocol error code when a 3GPP message sending has
 * failed.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 *
 * @note It is only applicable for 3GPP message sending failure, otherwise
 *       LE_SMS_ERROR_3GPP_MAX is returned.
 */
//--------------------------------------------------------------------------------------------------
void le_sms_GetErrorCode
(
    le_sms_MsgRef_t msgRef,
        ///< [IN]
        ///< Reference to the message object.

    le_sms_ErrorCode_t* rpCausePtr,
        ///< [OUT]
        ///< Radio Protocol cause code.

    le_sms_ErrorCode_t* tpCausePtr
        ///< [OUT]
        ///< Transfer Protocol cause code.
)
{
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Called to get the platform specific error code.
 *
 * Refer to @ref platformConstraintsSpecificErrorCodes for platform specific error code description.
 *
 * @return
 *  - The platform specific error code.
 *  - LE_BAD_PARAMETER Invalid reference provided.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
int32_t le_sms_GetPlatformSpecificErrorCode
(
    le_sms_MsgRef_t msgRef
        ///< [IN]
        ///< Reference to the message object.
)
{
    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to delete an SMS message from the storage area.
 *
 * It verifies first if the parameter is valid, then it checks that the modem state can support
 * message deleting.
 *
 * @return LE_FAULT         The function failed to perform the deletion.
 * @return LE_NO_MEMORY     The message is not present in storage area.
 * @return LE_OK            The function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_DeleteFromStorage
(
    le_sms_MsgRef_t msgRef   ///< [IN] The message to delete.
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to create an object's reference of the list of received messages
 * saved in the SMS message storage area.
 *
 * @return
 *      A reference to the List object. Null pointer if no messages have been retrieved.
 */
//--------------------------------------------------------------------------------------------------
le_sms_MsgListRef_t le_sms_CreateRxMsgList
(
    void
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to delete the list of the Messages retrieved from the message
 * storage.
 *
 * @note
 *      On failure, the process exits, so you don't have to worry about checking the returned
 *      reference for validity.
 */
//--------------------------------------------------------------------------------------------------
void le_sms_DeleteList
(
    le_sms_MsgListRef_t     msgListRef   ///< [IN] The list of the Messages.
)
{
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the first Message object reference in the list of messages
 * retrieved with le_sms_CreateRxMsgList().
 *
 * @return NULL              No message found.
 * @return le_sms_MsgRef_t  The Message object reference.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_sms_MsgRef_t le_sms_GetFirst
(
    le_sms_MsgListRef_t        msgListRef ///< [IN] The list of the messages.
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the next Message object reference in the list of messages
 * retrieved with le_sms_CreateRxMsgList().
 *
 * @return NULL              No message found.
 * @return le_sms_MsgRef_t  The Message object reference.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_sms_MsgRef_t le_sms_GetNext
(
    le_sms_MsgListRef_t        msgListRef ///< [IN] The list of the messages.
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to read the Message status (Received Read, Received Unread, Stored
 * Sent, Stored Unsent).
 *
 * @return The status of the message.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_sms_Status_t le_sms_GetStatus
(
    le_sms_MsgRef_t      msgRef        ///< [IN] The message.
)
{
    return LE_SMS_STATUS_UNKNOWN;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to mark a message as 'read'.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_sms_MarkRead
(
    le_sms_MsgRef_t    msgRef         ///< [IN] The message.
)
{
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to mark a message as 'unread'.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_sms_MarkUnread
(
    le_sms_MsgRef_t    msgRef         ///< [IN] The message.
)
{
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the SMS center address.
 *
 * Output parameter is updated with the SMS Service center address. If the Telephone number string
 *  exceeds the value of 'len' parameter,  LE_OVERFLOW error code is returned and 'tel' is filled
 *  until 'len-1' characters and a null-character is implicitly appended at the end of 'tel'.
 *
 * @return
 *  - LE_FAULT         Service is not available.
 *  - LE_OVERFLOW      Telephone number length exceed the maximum length.
 *  - LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_GetSmsCenterAddress
(
    char*            telPtr,  ///< [OUT] SMS center address number string.
    size_t           len      ///< [IN] The length of SMS center address number string.
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the SMS center address.
 *
 * SMS center address number is defined in ITU-T recommendations E.164/E.163.
 * E.164 numbers can have a maximum of fifteen digits and are usually written with a '+' prefix.
 *
 * @return
 *  - LE_FAULT         Service is not available.
 *  - LE_OK            Function succeeded.
 *
 * @note If the SMS center address number is too long (max LE_MDMDEFS_PHONE_NUM_MAX_LEN digits), it
 *       is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_SetSmsCenterAddress
(
    const char*            telPtr  ///< [IN] SMS center address number string.
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the preferred SMS storage for incoming messages.
 * @return
 *  - LE_FAULT         Function failed.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_SetPreferredStorage
(
    le_sms_Storage_t  prefStorage  ///< IN storage parameter.
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the preferred SMS storage for incoming messages.
 * @return
 *  - LE_FAULT         Function failed.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_GetPreferredStorage
(
    le_sms_Storage_t*  prefStorage  ///< OUT storage parameter.
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Activate Cell Broadcast message notification.
 *
 * @return
 *  - LE_FAULT         Function failed.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_ActivateCellBroadcast
(
    void
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Deactivate Cell Broadcast message notification.
 *
 * @return
 *  - LE_FAULT         Function failed.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_DeactivateCellBroadcast
(
    void
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Activate CDMA Cell Broadcast message notification.
 *
 * @return
 *  - LE_FAULT         Function failed.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_ActivateCdmaCellBroadcast
(
    void
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Deactivate CDMA Cell Broadcast message notification.
 *
 * @return
 *  - LE_FAULT         Function failed.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_DeactivateCdmaCellBroadcast
(
    void
)
{
    return LE_OK;
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
le_result_t le_sms_AddCellBroadcastIds
(
    uint16_t fromId,
        ///< [IN]
        ///< Starting point of the range of cell broadcast message identifier.

    uint16_t toId
        ///< [IN]
        ///< Ending point of the range of cell broadcast message identifier.
)
{
    return LE_OK;
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
le_result_t le_sms_RemoveCellBroadcastIds
(
    uint16_t fromId,
        ///< [IN]
        ///< Starting point of the range of cell broadcast message identifier.

    uint16_t toId
        ///< [IN]
        ///< Ending point of the range of cell broadcast message identifier.
)
{
    return LE_OK;
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
le_result_t le_sms_ClearCellBroadcastIds
(
    void
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Add CDMA Cell Broadcast category services.
 *
 * @return
 *  - LE_FAULT         Function failed.
 *  - LE_BAD_PARAMETER Parameter is invalid.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_AddCdmaCellBroadcastServices
(
    le_sms_CdmaServiceCat_t serviceCat,
        ///< [IN]
        ///< Service category assignment. Reference to 3GPP2 C.R1001-D
        ///< v1.0 Section 9.3.1 Standard Service Category Assignments.

    le_sms_Languages_t language
        ///< [IN]
        ///< Language Indicator. Reference to
        ///< 3GPP2 C.R1001-D v1.0 Section 9.2.1 Language Indicator
        ///< Value Assignments
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove CDMA Cell Broadcast category services.
 *
 * @return
 *  - LE_FAULT         Function failed.
 *  - LE_BAD_PARAMETER Parameter is invalid.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_RemoveCdmaCellBroadcastServices
(
    le_sms_CdmaServiceCat_t serviceCat,
        ///< [IN]
        ///< Service category assignment. Reference to 3GPP2 C.R1001-D
        ///< v1.0 Section 9.3.1 Standard Service Category Assignments.

    le_sms_Languages_t language
        ///< [IN]
        ///< Language Indicator. Reference to
        ///< 3GPP2 C.R1001-D v1.0 Section 9.2.1 Language Indicator
        ///< Value Assignments.
)
{
    return LE_OK;
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
le_result_t le_sms_ClearCdmaCellBroadcastServices
(
    void
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the number of messages successfully received or sent since last counter reset.
 *
 * @return
 *  - LE_OK             Function succeeded.
 *  - LE_BAD_PARAMETER  A parameter is invalid.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_GetCount
(
    le_sms_Type_t messageType,      ///< Message type.
    int32_t*      messageCountPtr   ///< Number of messages.
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Start to count the messages successfully received and sent.
 */
//--------------------------------------------------------------------------------------------------
void le_sms_StartCount
(
    void
)
{
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Stop to count the messages successfully received and sent.
 */
//--------------------------------------------------------------------------------------------------
void le_sms_StopCount
(
    void
)
{
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Reset the count of messages successfully received and sent.
 */
//--------------------------------------------------------------------------------------------------
void le_sms_ResetCount
(
    void
)
{
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Enable SMS Status Report for outgoing messages.
 *
 * @return
 *  - LE_OK             Function succeeded.
 *  - LE_FAULT          Function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_EnableStatusReport
(
    void
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Disable SMS Status Report for outgoing messages.
 *
 * @return
 *  - LE_OK             Function succeeded.
 *  - LE_FAULT          Function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_DisableStatusReport
(
    void
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get SMS Status Report activation state.
 *
 * @return
 *  - LE_OK             Function succeeded.
 *  - LE_BAD_PARAMETER  Parameter is invalid.
 *  - LE_FAULT          Function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_IsStatusReportEnabled
(
    bool* enabledPtr    ///< [OUT] True when SMS Status Report is enabled, false otherwise.
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get TP-Message-Reference of a message. Message type should be either a SMS Status Report or an
 * outgoing SMS.
 * TP-Message-Reference is defined in 3GPP TS 23.040 section 9.2.3.6.
 *
 * @return
 *  - LE_OK             Function succeeded.
 *  - LE_BAD_PARAMETER  A parameter is invalid.
 *  - LE_FAULT          Function failed.
 *  - LE_UNAVAILABLE    Outgoing SMS message is not sent.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_GetTpMr
(
    le_sms_MsgRef_t msgRef, ///< [IN]  Reference to the message object.
    uint8_t* tpMrPtr        ///< [OUT] 3GPP TS 23.040 TP-Message-Reference.
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get TP-Recipient-Address of SMS Status Report.
 * TP-Recipient-Address is defined in 3GPP TS 23.040 section 9.2.3.14.
 * TP-Recipient-Address Type-of-Address is defined in 3GPP TS 24.011 section 8.2.5.2.
 *
 * @return
 *  - LE_OK             Function succeeded.
 *  - LE_BAD_PARAMETER  A parameter is invalid.
 *  - LE_OVERFLOW       The Recipient Address length exceeds the length of the provided buffer.
 *  - LE_FAULT          Function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_GetTpRa
(
    le_sms_MsgRef_t msgRef, ///< [IN]  Reference to the message object.
    uint8_t* toraPtr,       ///< [OUT] 3GPP TS 24.011 TP-Recipient-Address Type-of-Address.
    char* raPtr,            ///< [OUT] 3GPP TS 23.040 TP-Recipient-Address.
    size_t raSize           ///< [IN]  3GPP TS 23.040 TP-Recipient-Address buffer length.
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get TP-Service-Centre-Time-Stamp of SMS Status Report.
 * TP-Service-Centre-Time-Stamp is defined in 3GPP TS 23.040 section 9.2.3.11.
 *
 * @return
 *  - LE_OK             Function succeeded.
 *  - LE_BAD_PARAMETER  A parameter is invalid.
 *  - LE_OVERFLOW       The SC Timestamp length exceeds the length of the provided buffer.
 *  - LE_FAULT          Function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_GetTpScTs
(
    le_sms_MsgRef_t msgRef, ///< [IN]  Reference to the message object.
    char* sctsPtr,          ///< [OUT] 3GPP TS 23.040 TP-Service-Centre-Time-Stamp.
    size_t sctsSize         ///< [IN]  3GPP TS 23.040 TP-Service-Centre-Time-Stamp buffer length.
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get TP-Discharge-Time of SMS Status Report.
 * TP-Discharge-Time is defined in 3GPP TS 23.040 section 9.2.3.13.
 *
 * @return
 *  - LE_OK             Function succeeded.
 *  - LE_BAD_PARAMETER  A parameter is invalid.
 *  - LE_OVERFLOW       The Discharge Time length exceeds the length of the provided buffer.
 *  - LE_FAULT          Function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_GetTpDt
(
    le_sms_MsgRef_t msgRef, ///< [IN]  Reference to the message object.
    char* dtPtr,            ///< [OUT] 3GPP TS 23.040 TP-Discharge-Time.
    size_t dtSize           ///< [IN]  3GPP TS 23.040 TP-Discharge-Time buffer length.
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get TP-Status of SMS Status Report.
 * TP-Status is defined in 3GPP TS 23.040 section 9.2.3.15.
 *
 * @return
 *  - LE_OK             Function succeeded.
 *  - LE_BAD_PARAMETER  A parameter is invalid.
 *  - LE_FAULT          Function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_GetTpSt
(
    le_sms_MsgRef_t msgRef, ///< [IN]  Reference to the message object.
    uint8_t* stPtr          ///< [OUT] 3GPP TS 23.040 TP-Status.
)
{
    return LE_OK;
}
