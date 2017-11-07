// -------------------------------------------------------------------------------------------------
/**
 *  SMS Inbox Server
 *
 * Declaration of the smsInbox.
 *
 *  Copyright (C) Sierra Wireless Inc.
 */
// -------------------------------------------------------------------------------------------------

#ifndef SMSINBOX_H_INCLUDE_GUARD
#define SMSINBOX_H_INCLUDE_GUARD


#include "legato.h"

// Interface specific includes
#include "interfaces.h"


//--------------------------------------------------------------------------------------------------
/**
 * Get the server service reference
 */
//--------------------------------------------------------------------------------------------------
le_msg_ServiceRef_t SmsInbox_GetServiceRef
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the client session reference for the current message
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionRef_t SmsInbox_GetClientSessionRef
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the server and advertise the service.
 */
//--------------------------------------------------------------------------------------------------
void SmsInbox_AdvertiseService
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Declare a reference type for referring to Message objects.
 */
//--------------------------------------------------------------------------------------------------
typedef struct SmsInbox_Session* SmsInbox_SessionRef_t;


//--------------------------------------------------------------------------------------------------
/**
 * Define unknown format
 */
//--------------------------------------------------------------------------------------------------
#define LE_SMSINBOX_FORMAT_UNKNOWN -1


//--------------------------------------------------------------------------------------------------
/**
 * Define the name of length of the mailbox name
 */
//--------------------------------------------------------------------------------------------------
#define LE_SMSINBOX_MAILBOX_LEN 12


//--------------------------------------------------------------------------------------------------
/**
 * Reference type used by Add/Remove functions for EVENT 'SmsInbox_RxMessage'
 */
//--------------------------------------------------------------------------------------------------
typedef struct SmsInbox_RxMessageHandler* SmsInbox_RxMessageHandlerRef_t;


//--------------------------------------------------------------------------------------------------
/**
 * Handler for New Message.
 *
 *
 * @param msgId
 *        Message identifier.
 * @param contextPtr
 */
//--------------------------------------------------------------------------------------------------
typedef void (*SmsInbox_RxMessageHandlerFunc_t)
(
    uint32_t msgId,
    void* contextPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Open a mailbox.
 *
 * @return
 * Reference on the opened mailbox
 */
//--------------------------------------------------------------------------------------------------
SmsInbox_SessionRef_t SmsInbox_Open
(
    const char* mailboxName,
        ///< [IN]
        ///< Session name.

    le_msg_ServiceRef_t msgServiceRef,
        ///< [IN]
        ///< Message service reference

    le_msg_SessionRef_t msgSession
        ///< [IN]
        ///< Client session reference
);

//--------------------------------------------------------------------------------------------------
/**
 * Close a previously open mailbox.
 *
 */
//--------------------------------------------------------------------------------------------------
void SmsInbox_Close
(
    SmsInbox_SessionRef_t sessionRef
        ///< [IN]
        ///< Session reference.
);

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'SmsInbox_RxMessage'
 *
 * This event provides information on new received messages.
 */
//--------------------------------------------------------------------------------------------------
SmsInbox_RxMessageHandlerRef_t SmsInbox_AddRxMessageHandler
(
    SmsInbox_SessionRef_t sessionRef,
        ///< [IN]
        ///< Session reference.

    SmsInbox_RxMessageHandlerFunc_t handlerPtr,
        ///< [IN]

    void* contextPtr
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'SmsInbox_RxMessage'
 */
//--------------------------------------------------------------------------------------------------
void SmsInbox_RemoveRxMessageHandler
(
    SmsInbox_RxMessageHandlerRef_t addHandlerRef
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * Delete a Message.
 *
 * @note It is valid to delete a non-existent message.
 */
//--------------------------------------------------------------------------------------------------
void SmsInbox_DeleteMsg
(
    SmsInbox_SessionRef_t sessionRef,
        ///< [IN]
        ///< Session reference.

    uint32_t msgId
        ///< [IN]
        ///< Message identifier.
);

//--------------------------------------------------------------------------------------------------
/**
 * Retrieves the IMSI of the message receiver SIM if it applies.
 *
 * @return
 *  - LE_NOT_FOUND     The message item is not tied to a SIM card, the imsi string is empty.
 *  - LE_OVERFLOW      The imsiPtr buffer was too small for the IMSI.
 *  - LE_BAD_PARAMETER The message reference is invalid.
 *  - LE_FAULT         The function failed.
 *  - LE_OK            The function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t SmsInbox_GetImsi
(
    SmsInbox_SessionRef_t sessionRef,
        ///< [IN]
        ///< Session reference.

    uint32_t msgId,
        ///< [IN]
        ///< Message identifier.

    char* imsi,
        ///< [OUT]
        ///< IMSI.

    size_t imsiNumElements
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the message format (text, binary or PDU).
 *
 * @return
 *  - Message format.
 *  - FORMAT_UNKNOWN when the message format cannot be identified or the message reference is
 *    invalid.
 */
//--------------------------------------------------------------------------------------------------
le_sms_Format_t SmsInbox_GetFormat
(
    SmsInbox_SessionRef_t sessionRef,
        ///< [IN]
        ///< Session reference.

    uint32_t msgId
        ///< [IN]
        ///< Message identifier.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the Sender Identifier.
 *
 * @return
 *  - LE_BAD_PARAMETER The message reference is invalid.
 *  - LE_OVERFLOW      Identifier length exceed the maximum length.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t SmsInbox_GetSenderTel
(
    SmsInbox_SessionRef_t sessionRef,
        ///< [IN]
        ///< Session reference.

    uint32_t msgId,
        ///< [IN]
        ///< Message identifier.

    char* tel,
        ///< [OUT]
        ///< Identifier string.

    size_t telNumElements
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the Message Time Stamp string (it does not apply for PDU message).
 *
 * @return
 *  - LE_BAD_PARAMETER The message reference is invalid.
 *  - LE_NOT_FOUND     The message is a PDU message.
 *  - LE_OVERFLOW      Timestamp number length exceed the maximum length.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t SmsInbox_GetTimeStamp
(
    SmsInbox_SessionRef_t sessionRef,
        ///< [IN]
        ///< Session reference.

    uint32_t msgId,
        ///< [IN]
        ///< Message identifier.

    char* timestamp,
        ///< [OUT]
        ///< Message time stamp (for text or binary
        ///<  messages). String format:
        ///< "yy/MM/dd,hh:mm:ss+/-zz"
        ///< (Year/Month/Day,Hour:Min:Seconds+/-TimeZone)

    size_t timestampNumElements
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the message Length value.
 *
 * @return Number of characters for text messages, or the length of the data in bytes for raw
 *         binary and PDU messages.
 */
//--------------------------------------------------------------------------------------------------
size_t SmsInbox_GetMsgLen
(
    SmsInbox_SessionRef_t sessionRef,
        ///< [IN]
        ///< Session reference.

    uint32_t msgId
        ///< [IN]
        ///< Message identifier.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the text Message.
 *
 *
 * @return
 *  - LE_BAD_PARAMETER The message reference is invalid.
 *  - LE_FORMAT_ERROR  Message is not in text format.
 *  - LE_OVERFLOW      Message length exceed the maximum length.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t SmsInbox_GetText
(
    SmsInbox_SessionRef_t sessionRef,
        ///< [IN]
        ///< Session reference.

    uint32_t msgId,
        ///< [IN]
        ///< Message identifier.

    char* text,
        ///< [OUT]
        ///< Message text.

    size_t textNumElements
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the binary Message.
 *
 *
 * @return
 *  - LE_BAD_PARAMETER The message reference is invalid.
 *  - LE_FORMAT_ERROR  Message is not in binary format.
 *  - LE_OVERFLOW      Message length exceed the maximum length.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t SmsInbox_GetBinary
(
    SmsInbox_SessionRef_t sessionRef,
        ///< [IN]
        ///< Session reference.

    uint32_t msgId,
        ///< [IN]
        ///< Message identifier.

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
 * in bytes.
 *
 * @return
 *  - LE_BAD_PARAMETER The message reference is invalid.
 *  - LE_FORMAT_ERROR  Unable to encode the message in PDU.
 *  - LE_OVERFLOW      Message length exceed the maximum length.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t SmsInbox_GetPdu
(
    SmsInbox_SessionRef_t sessionRef,
        ///< [IN]
        ///< Session reference.

    uint32_t msgId,
        ///< [IN]
        ///< Message identifier.

    uint8_t* pduPtr,
        ///< [OUT]
        ///< PDU message.

    size_t* pduNumElementsPtr
        ///< [INOUT]
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the first Message object reference in the inbox message.
 *
 * @return
 *  - 0 No message found (mailbox parsing is over).
 *  - Message identifier.
 */
//--------------------------------------------------------------------------------------------------
uint32_t SmsInbox_GetFirst
(
    SmsInbox_SessionRef_t sessionRef
        ///< [IN]
        ///< mailbox reference to browse.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the next Message object reference in the inbox message.
 *
 * @return
 *  - 0 No message found (mailbox parsing is over).
 *  - Message identifier.
 */
//--------------------------------------------------------------------------------------------------
uint32_t SmsInbox_GetNext
(
    SmsInbox_SessionRef_t sessionRef
        ///< [IN]
        ///< mailbox reference to browse.
);

//--------------------------------------------------------------------------------------------------
/**
 * allow to know whether the message has been read or not. The message status is tied to the client
 * app.
 *
 * @return True if the message is unread, false otherwise.
 *
 * @note If the caller is passing a bad message reference into this function, it is a fatal error,
 *        the function will not return.
 */
//--------------------------------------------------------------------------------------------------
bool SmsInbox_IsUnread
(
    SmsInbox_SessionRef_t sessionRef,
        ///< [IN]
        ///< mailbox reference.

    uint32_t msgId
        ///< [IN]
        ///< Message identifier.
);

//--------------------------------------------------------------------------------------------------
/**
 * Mark a message as 'read'.
 *
 * @note If the caller is passing a bad message reference into this function, it is a fatal error,
 *        the function will not return.
 */
//--------------------------------------------------------------------------------------------------
void SmsInbox_MarkRead
(
    SmsInbox_SessionRef_t sessionRef,
        ///< [IN]
        ///< mailbox reference.

    uint32_t msgId
        ///< [IN]
        ///< Message identifier.
);

//--------------------------------------------------------------------------------------------------
/**
 * Mark a message as 'unread'.
 *
 * @note If the caller is passing a bad message reference into this function, it is a fatal error,
 *        the function will not return.
 */
//--------------------------------------------------------------------------------------------------
void SmsInbox_MarkUnread
(
    SmsInbox_SessionRef_t sessionRef,
        ///< [IN]
        ///< mailbox reference.

    uint32_t msgId
        ///< [IN]
        ///< Message identifier.
);

//--------------------------------------------------------------------------------------------------
/**
 * handler function to release smsInbox service
 */
//--------------------------------------------------------------------------------------------------
void SmsInbox_CloseSessionEventHandler
(
    le_msg_SessionRef_t sessionRef,
        ///< [IN]
        ///< message session reference.

    void* contextPtr
        ///< [IN]
        ///< context pointer.
);

//--------------------------------------------------------------------------------------------------
/**
 * Set the maximum number of messages for message box.
 *
 * @return
 *  - LE_BAD_PARAMETER The message box name is invalid.
 *  - LE_OVERFLOW      Message count exceed the maximum limit.
 *  - LE_OK            Function succeeded.
 *  - LE_FAULT         Function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t SmsInbox_SetMaxMessages
(
    const char* mboxNamePtr,
        ///< [IN]
        ///< Message box name

    uint32_t maxMessageCount
        ///< [IN]
        ///< Maximum number of messages
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the maximum number of messages for message box.
 *
 * @return
 *  - LE_BAD_PARAMETER Invalid parameters.
 *  - LE_OK            Function succeeded.
 *  - LE_FAULT         Function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t SmsInbox_GetMaxMessages
(
    const char* mboxNamePtr,
        ///< [IN]
        ///< Message box name

    uint32_t* maxMessageCountPtr
        ///< [OUT]
        ///< Maximum number of messages
);
#endif // SMSINBOX_H_INCLUDE_GUARD

