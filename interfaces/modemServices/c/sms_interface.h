/*
 * ====================== WARNING ======================
 *
 * THE CONTENTS OF THIS FILE HAVE BEEN AUTO-GENERATED.
 * DO NOT MODIFY IN ANY WAY.
 *
 * ====================== WARNING ======================
 */

/**
 * @page c_sms SMS Services
 *
 * @ref sms_interface.h "Click here for the API reference documentation."
 *
 * <HR>
 *
 * This file contains data structures and prototypes definitions for high level SMS APIs.
 *
 *
 * @ref le_sms_ops_creating_msg <br>
 * @ref le_sms_ops_deleting_msg <br>
 * @ref le_sms_ops_sending <br>
 * @ref le_sms_ops_receiving <br>
 * @ref le_sms_ops_listing <br>
 * @ref le_sms_ops_deleting <br>
 * @ref le_sms_configdb <br>
 * @ref le_sms_ops_samples <br>
 *
 * SMS is a common way to communicate in the M2M world.
 *
 * It's an easy, fast way to send a small amount of data (e.g., sensor values for gas telemetry).
 * Usually, the radio module requests small power resources to send or receive a message.
 * It's often a good way to wake-up a device that was disconnected from the network or that was
 * operating in low power mode.
 *
 * @section le_sms_ops_creating_msg Creating a Message object
 * There are 3 kinds of supported messages: text messages, binary messages, and PDU messages.
 *
 * You must create a Message object by calling @c le_sms_Create() before using the message
 * APIs. It automatically allocates needed resources for the Message object, which is referenced by @c le_sms_MsgRef_t type.
 *
 * When the Message object is no longer needed, call @c le_sms_Delete() to free all
 *  allocated resources associated with the object.
 *
 * @section le_sms_ops_deleting_msg Deleting a Message object
 * To delete a Message object, call le_sms_Delete(). This frees all the
 * resources allocated for the Message object. If several users own the Message object
 * (e.g., several handler functions registered for SMS message reception), the
 * Message object will be deleted only after the last user deletes the Message object.
 *
 * @section le_sms_ops_sending Sending a message
 * To send a message, create an @c le_sms_MsgRef_t object by calling the
 * @c le_sms_Create() function. Then, set all the needed parameters for the message:
 * - Destination telephone number with le_sms_SetDestination();
 * -  Text content with le_sms_SetText(), the total length are set as well with this API, maximum
 * 160 characters as only the 7-bit alphabet is supported.
 * - Binary content with le_sms_SetBinary(), total length is set with this API, maximum 140 bytes.
 * - PDU content with le_sms_SetPDU(), total length is set with this API, max 36 (header) + 140 (payload) bytes long.
 *
 * After the le_sms_MsgRef_t object is ready, call @c le_sms_Send().
 *
 * @c le_sms_Send() is a blocking function, it will return once the Modem has given back a
 * positive or negative answer to the sending operation. The return of @c le_sms_Send() API
 * provides definitive status of the sending operation.
 *
 * Message object is never deleted regardless of the sending result. Caller has to
 * delete it.
 *
 * @code
 *
 * [...]
 *
 * le_sms_MsgRef_t myTextMsg;
 *
 * // Create a Message Object
 * myTextMsg = le_sms_Create();
 *
 * // Set the telephone number
 * le_sms_SetDestination(myTextMsg, "+33606060708");
 *
 * // Set the message text
 * le_sms_SetText(myTextMsg, "Hello, this is a Legato SMS message");
 *
 * // Send the message
 * le_sms_Send(myTextMsg);
 *
 * // Delete the message
 * le_sms_Delete(myTextMsg);
 *
 * @endcode
 *
 * @section le_sms_ops_receiving Receiving a message
 * To receive SMS messages, register a handler function to obtain incoming
 * messages. Use @c le_sms_AddRxMessageHandler() to register that handler.
 *
 * The handler must satisfy the following prototype:
 * @c typedef void (*le_sms_RxMessageHandlerFunc_t)(le_sms_MsgRef_t msg).
 *
 * When a new incoming message is received, a Message object is automatically created and the
 * handler is called. This Message object is Read-Only, any calls of a le_sms_SetXXX API will
 * return a LE_NOT_PERMITTED error.
 *
 * Use the following APIs to retrieve message information and data from the Message
 * object:
 * - le_sms_GetFormat() - determine if it is a PDU, a binary or a text message.
 * - le_sms_GetSenderTel() - get the sender's Telephone number.
 * - le_sms_GetTimeStamp() - get the timestamp sets by the Service Center.
 * - le_sms_GetUserdataLen() - get the message content (text or binary) length.
 * - le_sms_GetPDULen() - get the PDU message .
 * - le_sms_GetText() - get the message text.
 * - le_sms_GetBinary() - get the message binary content.
 * - le_sms_GetPDU() - get the message PDU data.
 *
 * @note If two (or more) registered handler functions exist, they are
 * all called and get the same message object reference.
 *
 * If a succession of messages is received, a new Message object is created for each, and
 * the handler is called for each new message.
 *
 * Uninstall the handler function by calling @c le_sms_RemoveRxMessageHandler().
 * @note @c le_sms_RemoveRxMessageHandler() API does not delete the Message Object. The caller has to
 *       delete it.
 *
 * @code
 *
 *
 * [...]
 *
 * // Handler function for message reception
 * static void myMsgHandler
 * (
 *      le_sms_MsgRef_t msgRef,
 *      void*            contextPtr*
 * )
 * {
 *     char   tel[LE_SMS_TEL_NMBR_MAX_LEN];
 *     char   text[LE_SMS_TEXT_MAX_LEN];
 *
 *     if (le_sms_GetFormat(msgRef) == LE_SMS_FORMAT_TEXT)
 *     {
 *         le_sms_GetSenderTel(msgRef, tel, sizeof(tel));
 *         le_sms_GetText(msgRef, text, sizeof(text));
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
 * le_sms_RxMessageHandlerRef_t HdlrRef;
 *
 * // Add an handler function to handle message reception
 * HdlrRef=le_sms_AddRxMessageHandler(myMsgHandler);
 *
 * [...]
 *
 * // Remove Handler entry
 * le_sms_RemoveRxMessageHandler(HdlrRef);
 *
 * [...]
 *
 * @endcode
 *
 * @section le_sms_ops_listing Listing  messages recorded in storage area
 * @note The default SMS message storage area is the SIM card. Change the storage setting through the
 *       SMS configuration APIs (available in a future version of API specification).
 *
 * Call @c le_sms_CreateRxMsgList() to create a List object that lists the received
 * messages present in the storage area, which is referenced by le_sms_MsgListRef_t
 * type.
 *
 * If messages are not present, the le_sms_CreateRxMsgList() returns NULL.
 *
 * Once the list is available, call @c le_sms_GetFirst() to get the first
 * message from the list, and then call @c le_sms_GetNext() API to get the next message.
 *
 * Call @c le_sms_DeleteList() to free all allocated
 *  resources associated with the List object.
 *
 * Call @c le_sms_GetStatus() to read the status of a message (Received
 * Read, Received Unread).
 *
 * To finish, you can also modify the received status of a message with
 * @c le_sms_MarkRead() and @c le_sms_MarkUnread().
 *
 * @section le_sms_ops_deleting Deleting a message from the storage area
 * @note The default SMS message storage area is the SIM card. Change this setting through the
 *       SMS configuration APIs (available in a future version of API specification).
 *
 * @c le_sms_DeleteFromStorage() deletes the message from the storage area. Message is
 * identified with @c le_sms_MsgRef_t object. The API returns an error if the message is not found
 * in the storage area.
 *
 *
 * @section le_sms_configdb SMS configuration tree
 * @copydoc le_sms_configdbPage_Hide
 *
 *
 * @section le_sms_ops_samples Sample codes
 * A sample code that implements a function for Mobile Originated SMS message can be found in
 * \b smsMO.c file (please refer to @ref c_smsSampleMO page).
 *
 * A sample code that implements a function for Mobile Terminated SMS message can be found in
 * \b smsMT.c file (please refer to @ref c_smsSampleMT page).
 *
 * These two samples can be easily compiled and run into the \b sms app, to install and use
 * this app:
 *
   @verbatim
   $ make ar7
   $ bin/instapp  build/ar7/bin/samples/sms.ar7 <ipaddress>
   @endverbatim
 * where ipaddress is the address of your target device.
 *
 * Then on your target, just run:
   @verbatim
   $ app start sms
   @endverbatim
 *
 * The sample replies to the sender by the message "Message from <phone number> received". Where
 * "phone number" is the sender's phone number.
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless, Inc. 2014. All rights reserved.
 */
/** @page c_sampleCodes_modem_sms Sample codes for SMS API
 *bin
 * @subpage c_smsSampleMO
 *
 * @subpage c_smsSampleMT
 *
 */
/** @page c_smsSampleMO Sample code for Mobile Originated SMS message
 *
 * @include "apps/sample/sms/smsMO.c"
 *
 */
/** @page c_smsSampleMT Sample code for Mobile Terminated SMS message
 *
 * @include "apps/sample/sms/smsMT.c"
 *
 */
/**
 * @interface le_sms_configdbPage_Hide
 *
 * The configuration database path for the SMS is:
 * @verbatim
   /
       modemServices/
           sms/
               smsc<string> = <SMS Center Address>
  @endverbatim
 *
 *  Where 'smsc' is the SMS Center Address.
 *
 */
/** @file sms_interface.h
 *
 * Legato @ref c_sms include file.
 *
 * Copyright (C) Sierra Wireless, Inc. 2014. All rights reserved.
 */

#ifndef SMS_INTERFACE_H_INCLUDE_GUARD
#define SMS_INTERFACE_H_INCLUDE_GUARD


#include "legato.h"

// User customizable include file
#include "userInclude.h"


//--------------------------------------------------------------------------------------------------
/**
 * Start the service for the client main thread
 */
//--------------------------------------------------------------------------------------------------
void le_sms_StartClient
(
    const char* serviceInstanceName
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * Stop the service for the current client thread
 */
//--------------------------------------------------------------------------------------------------
void le_sms_StopClient
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Reference type for le_sms_RxMessageHandler handler ADD/REMOVE functions
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_sms_RxMessageHandler* le_sms_RxMessageHandlerRef_t;


//--------------------------------------------------------------------------------------------------
/**
 * Handler for New Message.
 *
 *
 * @param msgRef
 *        Message reference.
 * @param contextPtr
 */
//--------------------------------------------------------------------------------------------------
typedef void (*le_sms_RxMessageHandlerFunc_t)
(
    le_sms_MsgRef_t msgRef,
    void* contextPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * le_sms_RxMessageHandler handler ADD function
 */
//--------------------------------------------------------------------------------------------------
le_sms_RxMessageHandlerRef_t le_sms_AddRxMessageHandler
(
    le_sms_RxMessageHandlerFunc_t handlerPtr,
        ///< [IN]

    void* contextPtr
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * le_sms_RxMessageHandler handler REMOVE function
 */
//--------------------------------------------------------------------------------------------------
void le_sms_RemoveRxMessageHandler
(
    le_sms_RxMessageHandlerRef_t addHandlerRef
        ///< [IN]
);

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
 * Set the Telephone destination number.
 *
 * Telephone number is defined in ITU-T recommendations E.164/E.163.
 * E.164 numbers can have a maximum of fifteen digits and are usually written with a '+' prefix.
 *
 * @return LE_NOT_PERMITTED Message is Read-Only.
 * @return LE_BAD_PARAMETER Telephone destination number length is equal to zero.
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
    le_sms_MsgRef_t msgRef,
        ///< [IN]
        ///< Reference to the message object.

    const char* dest
        ///< [IN]
        ///< Telephone number string.
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
    le_sms_MsgRef_t msgRef,
        ///< [IN]
        ///< Reference to the message object.

    const char* text
        ///< [IN]
        ///< SMS text.
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
    le_sms_MsgRef_t msgRef,
        ///< [IN]
        ///< Reference to the message object.

    const uint8_t* binPtr,
        ///< [IN]
        ///< Binary data.

    size_t binNumElements
        ///< [IN]
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
    le_sms_MsgRef_t msgRef,
        ///< [IN]
        ///< Reference to the message object.

    const uint8_t* pduPtr,
        ///< [IN]
        ///< PDU message.

    size_t pduNumElements
        ///< [IN]
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
    le_sms_MsgRef_t msgRef
        ///< [IN]
        ///< Reference to the message object.
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
    le_sms_MsgRef_t msgRef
        ///< [IN]
        ///< Reference to the message object.
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
    le_sms_MsgRef_t msgRef
        ///< [IN]
        ///< Reference to the message object.
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
    le_sms_MsgRef_t msgRef,
        ///< [IN]
        ///< Reference to the message object.

    char* tel,
        ///< [OUT]
        ///< Telephone number string.

    size_t telNumElements
        ///< [IN]
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
    le_sms_MsgRef_t msgRef,
        ///< [IN]
        ///< Reference to the message object.

    char* timestamp,
        ///< [OUT]
        ///< Message time stamp (in text mode).
        ///<        string format: "yy/MM/dd,hh:mm:ss+/-zz"
        ///<        (Year/Month/Day,Hour:Min:Seconds+/-TimeZone)

    size_t timestampNumElements
        ///< [IN]
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
    le_sms_MsgRef_t msgRef
        ///< [IN]
        ///< Reference to the message object.
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
    le_sms_MsgRef_t msgRef,
        ///< [IN]
        ///< Reference to the message object.

    char* text,
        ///< [OUT]
        ///< SMS text.

    size_t textNumElements
        ///< [IN]
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
    le_sms_MsgRef_t msgRef,
        ///< [IN]
        ///< Reference to the message object.

    uint8_t* binPtr,
        ///< [OUT]
        ///< Binary message.

    size_t* binNumElementsPtr
        ///< [INOUT]
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
    le_sms_MsgRef_t msgRef,
        ///< [IN]
        ///< Reference to the message object.

    uint8_t* pduPtr,
        ///< [OUT]
        ///< PDU message.

    size_t* pduNumElementsPtr
        ///< [INOUT]
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
    le_sms_MsgRef_t msgRef
        ///< [IN]
        ///< Reference to the message object.
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
    le_sms_MsgRef_t msgRef
        ///< [IN]
        ///< Reference to the message object.
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
    le_sms_MsgListRef_t msgListRef
        ///< [IN]
        ///< Messages list.
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
    le_sms_MsgListRef_t msgListRef
        ///< [IN]
        ///< Messages list.
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
    le_sms_MsgListRef_t msgListRef
        ///< [IN]
        ///< Messages list.
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
    le_sms_MsgRef_t msgRef
        ///< [IN]
        ///< Messages list.
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
    le_sms_MsgRef_t msgRef
        ///< [IN]
        ///< Messages list.
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
    le_sms_MsgRef_t msgRef
        ///< [IN]
        ///< Messages list.
);


#endif // SMS_INTERFACE_H_INCLUDE_GUARD

