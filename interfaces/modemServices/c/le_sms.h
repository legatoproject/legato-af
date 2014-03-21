/**
 * @page c_sms SMS Services
 *
 * @ref le_sms.h "Click here for the API reference documentation."
 *
 * <HR>
 *
 * This file contains data structures and prototypes definitions for high level SMS APIs.
 *
 *
 *@ref le_sms_msg_ops_creating_msg <br>
 *@ref le_sms_msg_ops_deleting_msg <br>
 *@ref le_sms_msg_ops_sending <br>
 *@ref le_sms_msg_ops_receiving <br>
 *@ref le_sms_msg_ops_listing <br>
 *@ref le_sms_msg_ops_deleting <br>
 *
 * SMS is a common way to communicate in the M2M world.
 *
 * It's an easy, fast way to send a small amount of data (e.g., sensor values for gas telemetry).
 * Usually, the radio module requests small power resources to send or receive a message.
 * It's often a good way to wake-up a device that was disconnected from the network or that was
 * operating in low power mode.
 *
 * @section le_sms_msg_ops_creating_msg Creating a Message object
 * There are 3 kinds of supported messages: text messages, binary messages, and PDU messages.
 *
 * You must create a Message object by calling @c le_sms_msg_Create() before using the message
 * APIs. It automatically allocates needed resources for the Message object, which is referenced by @c le_sms_msg_Ref_t type.
 *
 * When the Message object is no longer needed, call @c le_sms_msg_Delete() to free all
 *  allocated resources associated with the object.
 *
 * @section le_sms_msg_ops_deleting_msg Deleting a Message object
 * To delete a Message object, call le_sms_msg_Delete(). This frees all the
 * resources allocated for the Message object. If several users own the Message object 
 * (e.g., several handler functions registered for SMS message reception), the
 * Message object will be deleted only after the last user deletes the Message object.
 *
 * @section le_sms_msg_ops_sending Sending a message
 * To send a message, create an @c le_sms_msg_Ref_t object by calling the
 * @c le_sms_msg_Create() function. Then, set all the needed parameters for the message:
 * - Destination telephone number with le_sms_msg_SetDestination();
 * -  Text content with le_sms_msg_SetText(), the total length are set as well with this API, maximum 
 * 160 characters as only the 7-bit alphabet is supported.
 * - Binary content with le_sms_msg_SetBinary(), total length is set with this API, maximum 140 bytes.
 * - PDU content with le_sms_msg_SetPDU(), total length is set with this API, max 36 (header) + 140 (payload) bytes long.
 *
 * After the le_sms_msg_Ref_t object is ready, call @c le_sms_msg_Send().
 *
 * @c le_sms_msg_Send() is a blocking function, it will return once the Modem has given back a
 * positive or negative answer to the sending operation. The return of @c le_sms_msg_Send() API 
 * provides definitive status of the sending operation.
 *
 * Message object is never deleted regardless of the sending result. Caller has to
 * delete it.
 *
 * @code
 *
 * [...]
 *
 * le_sms_msg_Ref_t myTextMsg;
 *
 * // Create a Message Object
 * myTextMsg = le_sms_msg_Create();
 *
 * // Set the telephone number
 * le_sms_msg_SetDestination(myTextMsg, "+33606060708");
 *
 * // Set the message text
 * le_sms_msg_SetText(myTextMsg, "Hello, this is a Legato SMS message");
 *
 * // Send the message
 * le_sms_msg_Send(myTextMsg);
 *
 * // Delete the message
 * le_sms_msg_Delete(myTextMsg);
 *
 * @endcode
 *
 * @section le_sms_msg_ops_receiving Receiving a message
 * To receive SMS messages, register a handler function to obtain incoming
 * messages. Use @c le_sms_msg_AddRxMessageHandler() to register that handler.
 *
 * The handler must satisfy the following prototype:
 * @c typedef void (*le_sms_msg_RxMessageHandlerFunc_t)(le_sms_msg_Ref_t msg).
 *
 * When a new incoming message is received, a Message object is automatically created and the
 * handler is called. This Message object is Read-Only, any calls of a le_sms_msg_SetXXX API will
 * return a LE_NOT_PERMITTED error.
 *
 * Use the following APIs to retrieve message information and data from the Message
 * object:
 * - le_sms_msg_GetFormat() - determine if it is a PDU, a binary or a text message.
 * - le_sms_msg_GetSenderTel() - get the sender's Telephone number.
 * - le_sms_msg_GetTimeStamp() - get the timestamp sets by the Service Center.
 * - le_sms_msg_GetUserdataLen() - get the message content (text or binary) length.
 * - le_sms_msg_GetPDULen() - get the PDU message .
 * - le_sms_msg_GetText() - get the message text.
 * - le_sms_msg_GetBinary() - get the message binary content.
 * - le_sms_msg_GetPDU() - get the message PDU data.
 *
 * @note If two (or more) registered handler functions exist, they are
 * all called and get the same message object reference.
 *
 * If a succession of messages is received, a new Message object is created for each, and
 * the handler is called for each new message.
 *
 * Uninstall the handler function by calling @c le_sms_msg_RemoveRxMessageHandler().
 * @note @c le_sms_msg_RemoveRxMessageHandler() API does not delete the Message Object. The caller has to
 *       delete it.
 *
 * @code
 *
 *
 * [...]
 *
 * // Handler function for message reception
 * static void myMsgHandler(le_sms_msg_Ref_t msg)
 * {
 *     char                tel[LE_SMS_TEL_NMBR_MAX_LEN];
 *     char                text[LE_SMS_TEXT_MAX_LEN];
 *     le_sms_msg_Format_t format;
 *     uint32_t len=0;
 *
 *     le_sms_msg_GetFormat(msg, &format);
 *     if (format == LE_SMS_TEXT)
 *     {
 *         le_sms_msg_GetSenderTel(msg, tel, &len);
 *         le_sms_msg_GetText(msg, text, &len);
 *
 *         LE_INFO(" A new text message has been received !");
 *         LE_INFO(" From tel.%s, text: \"%s\"", tel, text);
 *     }
 *     else
 *     {
 *         LE_INFO(" I support only text messages !");
 *     }
 * }
 *
 *
 * [...]
 * // In the main function:
 *
 * int32_t hdlrId=-1;
 *
 * // Add an handler function to handle message reception
 * hdlrId=le_sms_msg_AddRxMessageHandler(myMsgHandler);
 *
 * [...]
 *
 * // Remove Handler entry
 * le_sms_msg_RemoveRxMessageHandler(hdlrId);
 *
 * [...]
 *
 * @endcode
 *
 * @section le_sms_msg_ops_listing Listing  messages recorded in storage area
 * @note The default SMS message storage area is the SIM card. Change the storage setting through the
 *       SMS configuration APIs (available in a future version of API specification).
 *
 * Call @c le_sms_msg_CreateRxMsgList() to create a List object that lists the received
 * messages present in the storage area, which is referenced by le_sms_msg_ListRef_t
 * type.
 *
 * If messages are not present, the le_sms_msg_CreateRxMsgList() returns NULL.
 *
 * Once the list is available, call @c le_sms_msg_GetFirst() to get the first
 * message from the list, and then call @c le_sms_msg_GetNext() API to get the next message.
 *
 * Call @c le_sms_msg_DeleteList() to free all allocated
 *  resources associated with the List object.
 *
 * Call @c le_sms_msg_GetStatus() to read the status of a message (Received
 * Read, Received Unread).
 *
 * To finish, you can also modify the received status of a message with
 * @c le_sms_msg_MarkRead() and @c le_sms_msg_MarkUnread().
 *
 * @section le_sms_msg_ops_deleting Deleting a message from the storage area
 * @note The default SMS message storage area is the SIM card. Change this setting through the
 *       SMS configuration APIs (available in a future version of API specification).
 *
 * @c le_sms_msg_DeleteFromStorage() deletes the message from the storage area. Message is
 * identified with @c le_sms_msg_Ref_t object. The API returns an error if the message is not found
 * in the storage area.
 *
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */


/** @file le_sms.h
 *
 * Legato @ref c_sms include file.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#ifndef LEGATO_SMS_OPS_INCLUDE_GUARD
#define LEGATO_SMS_OPS_INCLUDE_GUARD

#include "legato.h"
#include "le_mdm_defs.h"


//--------------------------------------------------------------------------------------------------
// Other new type definitions.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Declare a reference type for referring to SMS Message objects.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_sms_Msg* le_sms_msg_Ref_t;

//--------------------------------------------------------------------------------------------------
/**
 * Opaque type for SMS Message Listing.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_sms_msg_List* le_sms_msg_ListRef_t;

//--------------------------------------------------------------------------------------------------
/**
 *  Reference type for New Message Handler references.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_sms_msg_RxMessageHandler* le_sms_msg_RxMessageHandlerRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * Prototype for handler functions used to report that a new message has been received.
 *
 * @param msgRef      Message reference.
 * @param contextPtr  Whatever context information that the event handler may require.
 */
//--------------------------------------------------------------------------------------------------
typedef void(*le_sms_msg_RxMessageHandlerFunc_t)
(
    le_sms_msg_Ref_t  msgRef,
    void*             contextPtr
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
le_sms_msg_Ref_t le_sms_msg_Create
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
void le_sms_msg_Delete
(
    le_sms_msg_Ref_t  msgRef    ///< [IN] Reference to the message object.
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
le_sms_msg_Format_t le_sms_msg_GetFormat
(
    le_sms_msg_Ref_t   msgRef   ///< [IN] Reference to the message object.
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
 * @return LE_OVERFLOW       Telephone destination number is too long.
 * @return LE_OK            Function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_msg_SetDestination
(
    le_sms_msg_Ref_t  msgRef,  ///< [IN] Reference to the message object.
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
le_result_t le_sms_msg_GetSenderTel
(
    le_sms_msg_Ref_t msgRef,  ///< [IN] Reference to the message object.
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
le_result_t le_sms_msg_GetTimeStamp
(
    le_sms_msg_Ref_t msgRef,       ///< [IN] Reference to the message object.
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
size_t le_sms_msg_GetUserdataLen
(
    le_sms_msg_Ref_t msgRef ///< [IN] Reference to the message object.
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
size_t le_sms_msg_GetPDULen
(
    le_sms_msg_Ref_t msgRef ///< [IN] Reference to the message object.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the Text Message content.
 *
 * @return LE_NOT_PERMITTED Message is Read-Only.
 * @return LE_BAD_PARAMETER Text message length is equal to zero.
 * @return LE_OUT_OF_RANGE  Message is too long.
 * @return LE_OK            Function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_msg_SetText
(
    le_sms_msg_Ref_t      msgRef, ///< [IN] Reference to the message object.
    const char*           textPtr ///< [IN] SMS text.
);

//--------------------------------------------------------------------------------------------------
/**
 * Set the binary message content.
 *
 * @return LE_NOT_PERMITTED Message is Read-Only.
 * @return LE_BAD_PARAMETER Length of the data is equal to zero.
 * @return LE_OUT_OF_RANGE  Message is too long.
 * @return LE_OK            Function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_msg_SetBinary
(
    le_sms_msg_Ref_t  msg,    ///< [IN] Pointer to the message data structure.
    const uint8_t*    binPtr, ///< [IN] Binary data.
    size_t            len     ///< [IN] Length of the data in bytes.
);

//--------------------------------------------------------------------------------------------------
/**
 * Set the PDU message content.
 *
 * @return LE_NOT_PERMITTED Message is Read-Only.
 * @return LE_BAD_PARAMETER Length of the data is equal to zero.
 * @return LE_OUT_OF_RANGE  Message is too long.
 * @return LE_OK            Function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_msg_SetPDU
(
    le_sms_msg_Ref_t  msgRef, ///< [IN] Reference to the message object.
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
le_result_t le_sms_msg_GetText
(
    le_sms_msg_Ref_t msgRef,  ///< [IN]  Reference to the message object.
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
le_result_t le_sms_msg_GetBinary
(
    le_sms_msg_Ref_t msgRef, ///< [IN]  Reference to the message object.
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
le_result_t le_sms_msg_GetPDU
(
    le_sms_msg_Ref_t msgRef, ///< [IN]  Reference to the message object.
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
le_sms_msg_RxMessageHandlerRef_t le_sms_msg_AddRxMessageHandler
(
    le_sms_msg_RxMessageHandlerFunc_t handlerFuncPtr, ///< [IN] handler function for message.
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
void le_sms_msg_RemoveRxMessageHandler
(
    le_sms_msg_RxMessageHandlerRef_t   handlerRef ///< [IN] Handler reference.
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
le_result_t le_sms_msg_Send
(
    le_sms_msg_Ref_t    msgRef         ///< [IN] Message(s) to send.
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
le_result_t le_sms_msg_DeleteFromStorage
(
    le_sms_msg_Ref_t msgRef   ///< [IN] Message to delete.
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
le_sms_msg_ListRef_t le_sms_msg_CreateRxMsgList
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
void le_sms_msg_DeleteList
(
    le_sms_msg_ListRef_t     msgListRef   ///< [IN] Messages list.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the first Message object reference in the list of messages
 * retrieved with le_sms_msg_CreateRxMsgList().
 *
 * @return NULL              No message found.
 * @return le_sms_msg_Ref_t  Message object reference.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_sms_msg_Ref_t le_sms_msg_GetFirst
(
    le_sms_msg_ListRef_t        msgListRef ///< [IN] Messages list.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the next Message object reference in the list of messages
 * retrieved with le_sms_msg_CreateRxMsgList().
 *
 * @return NULL              No message found.
 * @return le_sms_msg_Ref_t  Message object reference.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_sms_msg_Ref_t le_sms_msg_GetNext
(
    le_sms_msg_ListRef_t        msgListRef ///< [IN] Messages list.
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
le_sms_msg_Status_t le_sms_msg_GetStatus
(
    le_sms_msg_Ref_t      msgRef        ///< [IN] Reference to the message object.
);

//--------------------------------------------------------------------------------------------------
/**
 * Mark a message as 'read'.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_sms_msg_MarkRead
(
    le_sms_msg_Ref_t    msgRef         ///< [IN] Reference to the message object.
);

//--------------------------------------------------------------------------------------------------
/**
 * Mark a message as 'unread'.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_sms_msg_MarkUnread
(
    le_sms_msg_Ref_t    msgRef         ///< [IN] Reference to the message object.
);



#endif // LEGATO_SMS_OPS_INCLUDE_GUARD
