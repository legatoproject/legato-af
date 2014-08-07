/** @file le_sms.h
 *
 * Legato @ref c_sms include file.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved.
 */

#ifndef LEGATO_SMS_OPS_INCLUDE_GUARD
#define LEGATO_SMS_OPS_INCLUDE_GUARD

#include "legato.h"
#include "le_mdm_defs.h"

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Time stamp string length.
 * The string format is "yy/MM/dd,hh:mm:ss+/-zz" (Year/Month/Day,Hour:Min:Seconds+/-TimeZone).
 * One extra byte is added for the null character.
 */
//--------------------------------------------------------------------------------------------------
#define LE_SMS_TIMESTAMP_MAX_LEN     (20+1)

//--------------------------------------------------------------------------------------------------
/**
 * The text message can be up to 160 characters long.
 * One extra byte is added for the null character.
 */
//--------------------------------------------------------------------------------------------------
#define LE_SMS_TEXT_MAX_LEN           (160+1)

//--------------------------------------------------------------------------------------------------
/**
 * The raw binary message can be up to 140 bytes long.
 *
 */
//--------------------------------------------------------------------------------------------------
#define LE_SMS_BINARY_MAX_LEN           (140)

//--------------------------------------------------------------------------------------------------
/**
 * The PDU payload bytes long.
 *
 */
//--------------------------------------------------------------------------------------------------
#define LE_SMS_PDU_MAX_PAYLOAD      (140)

//--------------------------------------------------------------------------------------------------
/**
 * The PDU message can be up to 36 (header) + 140 (payload) bytes long.
 *
 */
//--------------------------------------------------------------------------------------------------
#define LE_SMS_PDU_MAX_LEN           (36+LE_SMS_PDU_MAX_PAYLOAD)

//--------------------------------------------------------------------------------------------------
/**
 * Message Format.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_SMS_FORMAT_PDU     = 0,  ///< PDU message format.
    LE_SMS_FORMAT_TEXT    = 1,  ///< Text message format.
    LE_SMS_FORMAT_BINARY  = 2,  ///< Binary message format.
    LE_SMS_FORMAT_UNKNOWN = 3,  ///< Unknown message format.
}
le_sms_Format_t;

//--------------------------------------------------------------------------------------------------
/**
 * Message Status.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_SMS_RX_READ        = 0,  ///< Message present in the message storage has been read.
    LE_SMS_RX_UNREAD      = 1,  ///< Message present in the message storage has not been read.
    LE_SMS_STORED_SENT    = 2,  ///< Message saved in the message storage has been sent.
    LE_SMS_STORED_UNSENT  = 3,  ///< Message saved in the message storage has not been sent.
    LE_SMS_SENT           = 4,  ///< Message has been sent.
    LE_SMS_UNSENT         = 5,  ///< Message has not been sent.
    LE_SMS_STATUS_UNKNOWN = 6,  ///< Unknown message status.
}
le_sms_Status_t;

//--------------------------------------------------------------------------------------------------
// Other new type definitions.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Declare a reference type for referring to SMS Message objects.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_sms_Msg* le_sms_MsgRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * Opaque type for SMS Message Listing.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_sms_MsgList* le_sms_MsgListRef_t;

//--------------------------------------------------------------------------------------------------
/**
 *  Reference type for New Message Handler references.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_sms_RxMessageHandler* le_sms_RxMessageHandlerRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * Prototype for handler functions used to report that a new message has been received.
 *
 * @param msgRef      Message reference.
 * @param contextPtr  Whatever context information that the event handler may require.
 */
//--------------------------------------------------------------------------------------------------
typedef void(*le_sms_RxMessageHandlerFunc_t)
(
    le_sms_MsgRef_t  msgRef,
    void*            contextPtr
);


//--------------------------------------------------------------------------------------------------
// APIs.
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
/**
 * Create an SMS Message data structure.
 *
 * @return Reference to the new Message object.
 *
 * @note
 *      On failure, the process exits, so you don't have to worry about checking the returned
 *      reference for validity.
 */
//--------------------------------------------------------------------------------------------------
le_sms_MsgRef_t le_sms_Create
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Delete a Message data structure.
 *
 * It deletes the Message data structure and all the allocated memory is freed. If several
 * users own the Message object (e.g., several handler functions registered for
 * SMS message reception), the Message object will only be deleted if one User
 * owns the Message object.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_sms_Delete
(
    le_sms_MsgRef_t  msgRef    ///< [IN] Reference to the message object.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the message format (text or PDU).
 *
 * @return Message format.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_sms_Format_t le_sms_GetFormat
(
    le_sms_MsgRef_t   msgRef   ///< [IN] Reference to the message object.
);

//--------------------------------------------------------------------------------------------------
/**
 * Set the Telephone destination number.
 *
 * Telephone number is defined in ITU-T recommendations E.164/E.163.
 * E.164 numbers can have a maximum of fifteen digits and are usually written with a ‘+’ prefix.
 *
 * @return LE_NOT_PERMITTED Message is Read-Only.
 * @return LE_BAD_PARAMETER  Telephone destination number length is equal to zero.
 * @return LE_OK            Function succeeded.
 *
 * @note If telephone destination number is too long (max 17 digits), it is a fatal
 *       error, the function will not return.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_SetDestination
(
    le_sms_MsgRef_t   msgRef,  ///< [IN] Reference to the message object.
    const char*       destPtr  ///< [IN] Telephone number string.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the Sender Telephone number.
 *
 * Output parameter is updated with the Telephone number. If the Telephone number string exceeds
 * the value of 'len' parameter,  LE_OVERFLOW error code is returned and 'tel' is filled until
 * 'len-1' characters and a null-character is implicitly appended at the end of 'tel'.
 *
 * @return LE_NOT_PERMITTED Message is not a received message.
 * @return LE_OVERFLOW      Telephone number length exceed the maximum length.
 * @return LE_OK            Function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_GetSenderTel
(
    le_sms_MsgRef_t  msgRef,  ///< [IN] Reference to the message object.
    char*            telPtr,  ///< [OUT] Telephone number string.
    size_t           len      ///< [IN] Telephone number stringlength.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the Service Center Time Stamp string.
 *
 * Output parameter is updated with the Time Stamp string. If the Time Stamp string exceeds the
 * value of 'len' parameter, a LE_OVERFLOW error code is returned and 'timestamp' is filled until
 * 'len-1' characters and a null-character is implicitly appended at the end of 'timestamp'.
 *
 * @return LE_NOT_PERMITTED Message is not a received message.
 * @return LE_OVERFLOW      Timestamp number length exceed the maximum length.
 * @return LE_OK            Function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_GetTimeStamp
(
    le_sms_MsgRef_t  msgRef,       ///< [IN] Reference to the message object.
    char*            timestampPtr, ///< [OUT] Message time stamp (in text mode).
                                   ///        string format: "yy/MM/dd,hh:mm:ss+/-zz"
                                   ///        (Year/Month/Day,Hour:Min:Seconds+/-TimeZone)
    size_t           len           ///< [IN] Timestamp string length.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the message Length value.
 *
 * @return Number of characters for text messages, or the length of the data in bytes for raw
 *         binary messages.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
size_t le_sms_GetUserdataLen
(
    le_sms_MsgRef_t msgRef ///< [IN] Reference to the message object.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the message Length value.
 *
 * @return Length of the data in bytes of the PDU message.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
size_t le_sms_GetPDULen
(
    le_sms_MsgRef_t msgRef ///< [IN] Reference to the message object.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the Text Message content.
 *
 * @return LE_NOT_PERMITTED Message is Read-Only.
 * @return LE_BAD_PARAMETER Text message length is equal to zero.
 * @return LE_OK            Function succeeded.
 *
 * @note If message is too long (max 160 digits), it is a fatal error, the
 *       function will not return.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_SetText
(
    le_sms_MsgRef_t       msgRef, ///< [IN] Reference to the message object.
    const char*           textPtr ///< [IN] SMS text.
);

//--------------------------------------------------------------------------------------------------
/**
 * Set the binary message content.
 *
 * @return LE_NOT_PERMITTED Message is Read-Only.
 * @return LE_BAD_PARAMETER Length of the data is equal to zero.
 * @return LE_OK            Function succeeded.
 *
 * @note If len is too long (max 140 bytes), it is a fatal error, the
 *       function will not return.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_SetBinary
(
    le_sms_MsgRef_t   msg,    ///< [IN] Pointer to the message data structure.
    const uint8_t*    binPtr, ///< [IN] Binary data.
    size_t            len     ///< [IN] Length of the data in bytes.
);

//--------------------------------------------------------------------------------------------------
/**
 * Set the PDU message content.
 *
 * @return LE_NOT_PERMITTED Message is Read-Only.
 * @return LE_BAD_PARAMETER Length of the data is equal to zero.
 * @return LE_OK            Function succeeded.
 *
 * @note If len is too long (max 176 bytes), it is a fatal error, the
 *       function will not return.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_SetPDU
(
    le_sms_MsgRef_t   msgRef, ///< [IN] Reference to the message object.
    const uint8_t*    pduPtr, ///< [IN] PDU message.
    size_t            len     ///< [IN] Length of the data in bytes.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the text Message.
 *
 * Output parameter is updated with the text string. If the text string exceedS the
 * value of 'len' parameter,  LE_OVERFLOW error code is returned and 'text' is filled until
 * 'len-1' characters and a null-character is implicitly appended at the end of 'text'.
 *
 * @return LE_OVERFLOW      Message length exceed the maximum length.
 * @return LE_OK            Function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_GetText
(
    le_sms_MsgRef_t  msgRef,  ///< [IN]  Reference to the message object.
    char*            textPtr, ///< [OUT] SMS text.
    size_t           len      ///< [IN] The maximum length of the text message.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the binary Message.
 *
 * Output parameters are updated with the binary message content and the length of the raw
 * binary message in bytes. If the binary data exceed the value of 'len' input parameter, a
 * LE_OVERFLOW error code is returned and 'raw' is filled until 'len' bytes.
 *
 * @return LE_FORMAT_ERROR  Message is not in binary format
 * @return LE_OVERFLOW      Message length exceed the maximum length.
 * @return LE_OK            Function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_GetBinary
(
    le_sms_MsgRef_t  msgRef, ///< [IN]  Reference to the message object.
    uint8_t*         binPtr, ///< [OUT] Binary message.
    size_t*          lenPtr  ///< [IN,OUT] Length of the binary message in bytes.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the PDU message.
 *
 * Output parameters are updated with the PDU message content and the length of the PDU message
 * in bytes. If the PDU data exceed the value of 'len' input parameter, a LE_OVERFLOW error code is
 * returned and 'pdu' is filled until 'len' bytes.
 *
 * @return LE_FORMAT_ERROR  Unable to encode the message in PDU.
 * @return LE_OVERFLOW      Message length exceed the maximum length.
 * @return LE_OK            Function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_GetPDU
(
    le_sms_MsgRef_t  msgRef, ///< [IN]  Reference to the message object.
    uint8_t*         pduPtr, ///< [OUT] PDU message.
    size_t*          lenPtr  ///< [IN,OUT] Length of the PDU message in bytes.
);

//--------------------------------------------------------------------------------------------------
/**
 * Register an handler function for SMS message reception.
 *
 * @return Handler reference, which is only needed for later removal of the handler.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_sms_RxMessageHandlerRef_t le_sms_AddRxMessageHandler
(
    le_sms_RxMessageHandlerFunc_t handlerFuncPtr, ///< [IN] handler function for message.
                                                       ///  reception.
    void*                              contextPtr      ///< [IN] Handler's context.
);

//--------------------------------------------------------------------------------------------------
/**
 * Unregister a handler function
 *
 * @note Doesn't return on failure, no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
void le_sms_RemoveRxMessageHandler
(
    le_sms_RxMessageHandlerRef_t   handlerRef ///< [IN] Handler reference.
);

//--------------------------------------------------------------------------------------------------
/**
 * Send an SMS message.
 *
 * Verifies first if the parameters are valid, then it checks the modem state can support
 * message sending.
 *
 * @return LE_NOT_POSSIBLE  Current modem state does not support message sending.
 * @return LE_FORMAT_ERROR  Message content is invalid.
 * @return LE_FAULT         Function failed to send the message.
 * @return LE_OK            Function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_Send
(
    le_sms_MsgRef_t    msgRef         ///< [IN] Message(s) to send.
);

//--------------------------------------------------------------------------------------------------
/**
 * Delete an SMS message from the storage area.
 *
 * Verifies first if the parameter is valid, then it checks the modem state can support
 * message deleting.
 *
 * @return LE_NOT_POSSIBLE  Current modem state does not support message deleting.
 * @return LE_FAULT         Function failed to perform the deletion.
 * @return LE_NO_MEMORY     Message storage is not available.
 * @return LE_OK            Function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_DeleteFromStorage
(
    le_sms_MsgRef_t msgRef   ///< [IN] Message to delete.
);

//--------------------------------------------------------------------------------------------------
/**
 * Create an object's reference of the list of received messages
 * saved in the SMS message storage area.
 *
 * @return
 *      Reference to the List object. Null pointer if no messages have been retrieved.
 */
//--------------------------------------------------------------------------------------------------
le_sms_MsgListRef_t le_sms_CreateRxMsgList
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Delete the list of the Messages retrieved from the message
 * storage.
 *
 * @note
 *      On failure, the process exits, so you don't have to worry about checking the returned
 *      reference for validity.
 */
//--------------------------------------------------------------------------------------------------
void le_sms_DeleteList
(
    le_sms_MsgListRef_t     msgListRef   ///< [IN] Messages list.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the first Message object reference in the list of messages
 * retrieved with le_sms_CreateRxMsgList().
 *
 * @return NULL              No message found.
 * @return le_sms_MsgRef_t  Message object reference.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_sms_MsgRef_t le_sms_GetFirst
(
    le_sms_MsgListRef_t        msgListRef ///< [IN] Messages list.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the next Message object reference in the list of messages
 * retrieved with le_sms_CreateRxMsgList().
 *
 * @return NULL              No message found.
 * @return le_sms_MsgRef_t  Message object reference.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_sms_MsgRef_t le_sms_GetNext
(
    le_sms_MsgListRef_t        msgListRef ///< [IN] Messages list.
);

//--------------------------------------------------------------------------------------------------
/**
 * Read the Message status (Received Read, Received Unread, Stored
 * Sent, Stored Unsent).
 *
 * @return Status of the message.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_sms_Status_t le_sms_GetStatus
(
    le_sms_MsgRef_t      msgRef        ///< [IN] Reference to the message object.
);

//--------------------------------------------------------------------------------------------------
/**
 * Mark a message as 'read'.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_sms_MarkRead
(
    le_sms_MsgRef_t    msgRef         ///< [IN] Reference to the message object.
);

//--------------------------------------------------------------------------------------------------
/**
 * Mark a message as 'unread'.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_sms_MarkUnread
(
    le_sms_MsgRef_t    msgRef         ///< [IN] Reference to the message object.
);



#endif // LEGATO_SMS_OPS_INCLUDE_GUARD
