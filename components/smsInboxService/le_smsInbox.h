// -------------------------------------------------------------------------------------------------
/**
 *  SMS Inbox Server
 *
 * Wrapper.
 *
 *  Copyright (C) Sierra Wireless Inc.
 */
// -------------------------------------------------------------------------------------------------
#ifndef LE_SMSINBOX_H_INCLUDE_GUARD
#define LE_SMSINBOX_H_INCLUDE_GUARD

#include "smsInbox.h"

// ifgen will generate some set of functions prefixed with the message box name (one set per .api
// provided into the Components.cdef). All these functions are wrapped on the sames functions, only
// the prefix is different.
// Instead of re-write the code for all functions, this macro does the job.
#define DEFINE_MBX(LE_NAME) \
\
LE_NAME##_SessionRef_t LE_NAME##Ref; \
\
/** \
 * Open a message box. \
 * \
 * @return \
 * Reference on the opened message box \
 */ \
LE_NAME##_SessionRef_t LE_NAME##_Open \
( \
    void \
) \
{ \
    le_msg_ServiceRef_t msgServiceRef = LE_NAME##_GetServiceRef(); \
    le_msg_SessionRef_t msgSession = LE_NAME##_GetClientSessionRef(); \
    static le_msg_SessionEventHandlerRef_t msgSessionRef = NULL; \
    if (msgSessionRef == NULL) \
    { \
        /* Register CloseSessionEventHandler for smsInbox service */ \
        msgSessionRef = le_msg_AddServiceCloseHandler(msgServiceRef, \
                        SmsInbox_CloseSessionEventHandler, NULL); \
    } \
    LE_NAME##Ref = (LE_NAME##_SessionRef_t) SmsInbox_Open(#LE_NAME,msgServiceRef,msgSession); \
    return LE_NAME##Ref; \
} \
\
/** \
 * Close a previously open message box. \
 * \
 */ \
void LE_NAME##_Close \
( \
    LE_NAME##_SessionRef_t  sessionRef \
) \
{ \
    SmsInbox_Close( (SmsInbox_SessionRef_t) sessionRef); \
} \
\
/** \
 * Add handler function for EVENT 'SmsInbox_RxMessage' \
 * \
 * This event provides information on new received messages. \
 */ \
LE_NAME##_RxMessageHandlerRef_t LE_NAME##_AddRxMessageHandler \
( \
    LE_NAME##_RxMessageHandlerFunc_t handlerPtr, \
    void* contextPtr \
) \
{ \
    return (LE_NAME##_RxMessageHandlerRef_t ) SmsInbox_AddRxMessageHandler( \
                                            (SmsInbox_SessionRef_t)LE_NAME##Ref, \
                                            (SmsInbox_RxMessageHandlerFunc_t) handlerPtr, \
                                             contextPtr ); \
} \
\
/** \
 * Remove handler function for EVENT 'SmsInbox_RxMessage' \
 */ \
void LE_NAME##_RemoveRxMessageHandler \
( \
    LE_NAME##_RxMessageHandlerRef_t addHandlerRef \
) \
{ \
    SmsInbox_RemoveRxMessageHandler ((SmsInbox_RxMessageHandlerRef_t) addHandlerRef);\
} \
\
/** \
 * Delete a Message. \
 * \
 * @note It is valid to delete a non-existent message. \
 */ \
void LE_NAME##_DeleteMsg \
( \
    uint32_t msgId \
) \
{ \
    return SmsInbox_DeleteMsg( \
        (SmsInbox_SessionRef_t)LE_NAME##Ref, \
        msgId \
        ); \
} \
\
/** \
 * Retrieves the IMSI of the message receiver SIM if it applies. \
 * \
 * @return \
 *  - LE_NOT_FOUND     The message item is not tied to a SIM card, the imsi string is empty. \
 *  - LE_OVERFLOW      The imsiPtr buffer was too small for the IMSI. \
 *  - LE_BAD_PARAMETER The message reference is invalid. \
 *  - LE_FAULT         The function failed. \
 *  - LE_OK            The function succeeded. \
 * \
 */ \
le_result_t LE_NAME##_GetImsi \
( \
    uint32_t msgId, \
    char* imsi, \
    size_t imsiNumElements \
) \
{ \
    return SmsInbox_GetImsi( \
            (SmsInbox_SessionRef_t)LE_NAME##Ref, \
            msgId, \
            imsi, \
            imsiNumElements); \
} \
\
/** \
 * Get the message format (text, binary or PDU). \
 * \
 * @return \
 *  - Message format. \
 *  - FORMAT_UNKNOWN when the message format cannot be identified or the message reference is \
 *    invalid. \
 */ \
le_sms_Format_t LE_NAME##_GetFormat \
( \
    uint32_t msgId \
) \
{ \
    return SmsInbox_GetFormat( \
            (SmsInbox_SessionRef_t)LE_NAME##Ref, \
            msgId); \
} \
\
/** \
 * Get the Sender Identifier. \
 * \
 * @return \
 *  - LE_BAD_PARAMETER The message reference is invalid. \
 *  - LE_OVERFLOW      Identifier length exceed the maximum length. \
 *  - LE_OK            Function succeeded. \
 */ \
le_result_t LE_NAME##_GetSenderTel \
( \
    uint32_t msgId, \
    char* tel, \
    size_t telNumElements \
) \
{ \
    return SmsInbox_GetSenderTel( \
        (SmsInbox_SessionRef_t)LE_NAME##Ref, \
        msgId,\
        tel,\
        telNumElements ); \
} \
\
/** \
 * Get the Message Time Stamp string (it does not apply for PDU message). \
 * \
 * @return \
 *  - LE_BAD_PARAMETER The message reference is invalid. \
 *  - LE_NOT_FOUND     The message is a PDU message. \
 *  - LE_OVERFLOW      Timestamp number length exceed the maximum length. \
 *  - LE_OK            Function succeeded. \
 */ \
le_result_t LE_NAME##_GetTimeStamp \
( \
    uint32_t msgId, \
    char* timestamp, \
    size_t timestampNumElements \
) \
{ \
    return SmsInbox_GetTimeStamp( \
        (SmsInbox_SessionRef_t)LE_NAME##Ref, \
        msgId, \
        timestamp, \
        timestampNumElements); \
} \
\
/** \
 * Get the message Length value. \
 * \
 * @return Number of characters for text messages, or the length of the data in bytes for raw \
 *         binary and PDU messages. \
 */ \
size_t LE_NAME##_GetMsgLen \
( \
    uint32_t msgId \
) \
{ \
    return SmsInbox_GetMsgLen((SmsInbox_SessionRef_t)LE_NAME##Ref, msgId);\
} \
\
/** \
 * Get the text Message. \
 * \
 * \
 * @return \
 *  - LE_BAD_PARAMETER The message reference is invalid. \
 *  - LE_FORMAT_ERROR  Message is not in text format. \
 *  - LE_OVERFLOW      Message length exceed the maximum length. \
 *  - LE_OK            Function succeeded. \
 */ \
le_result_t LE_NAME##_GetText \
( \
    uint32_t msgId, \
    char* text, \
    size_t textNumElements \
) \
{ \
    return SmsInbox_GetText( \
            (SmsInbox_SessionRef_t)LE_NAME##Ref, \
            msgId,\
            text,\
            textNumElements);\
} \
\
/** \
 * Get the binary Message. \
 * \
 * \
 * @return \
 *  - LE_BAD_PARAMETER The message reference is invalid. \
 *  - LE_FORMAT_ERROR  Message is not in binary format. \
 *  - LE_OVERFLOW      Message length exceed the maximum length. \
 *  - LE_OK            Function succeeded. \
 */ \
le_result_t LE_NAME##_GetBinary \
( \
    uint32_t msgId, \
    uint8_t* binPtr, \
    size_t* binNumElementsPtr \
) \
{ \
    return SmsInbox_GetBinary (\
                (SmsInbox_SessionRef_t)LE_NAME##Ref, \
                msgId,\
                binPtr,\
                binNumElementsPtr);\
} \
\
/** \
 * Get the PDU message. \
 * \
 * Output parameters are updated with the PDU message content and the length of the PDU message \
 * in bytes. \
 * \
 * @return \
 *  - LE_BAD_PARAMETER The message reference is invalid. \
 *  - LE_FORMAT_ERROR  Unable to encode the message in PDU. \
 *  - LE_OVERFLOW      Message length exceed the maximum length. \
 *  - LE_OK            Function succeeded. \
 */ \
le_result_t LE_NAME##_GetPdu \
( \
    uint32_t msgId, \
    uint8_t* pduPtr, \
    size_t* pduNumElementsPtr \
) \
{ \
    return SmsInbox_GetPdu( (SmsInbox_SessionRef_t)LE_NAME##Ref, \
                                msgId, \
                                pduPtr, \
                                pduNumElementsPtr); \
} \
\
/** \
 * Get the first Message object reference in the inbox message. \
 * \
 * @return \
 *  - 0 No message found (message box parsing is over). \
 *  - Message identifier. \
 */ \
uint32_t LE_NAME##_GetFirst \
( \
    LE_NAME##_SessionRef_t  sessionRef \
) \
{ \
    return SmsInbox_GetFirst((SmsInbox_SessionRef_t) sessionRef); \
} \
\
/** \
 * Get the next Message object reference in the inbox message. \
 * \
 * @return \
 *  - 0 No message found (message box parsing is over). \
 *  - Message identifier. \
 */ \
uint32_t LE_NAME##_GetNext \
( \
    LE_NAME##_SessionRef_t  sessionRef \
) \
{ \
    return SmsInbox_GetNext((SmsInbox_SessionRef_t) sessionRef); \
} \
\
/** \
 * allow to know whether the message has been read or not. The message status is tied to the client \
 * app. \
 * \
 * @return True if the message is unread, false otherwise. \
 * \
 * @note If the caller is passing a bad message reference into this function, it is a fatal error, \
 *        the function will not return. \
 */ \
bool LE_NAME##_IsUnread \
( \
    uint32_t msgId \
) \
{ \
    return SmsInbox_IsUnread ((SmsInbox_SessionRef_t)LE_NAME##Ref, msgId ); \
} \
\
/** \
 * Mark a message as 'read'. \
 * \
 * @note If the caller is passing a bad message reference into this function, it is a fatal error, \
 *        the function will not return. \
 */ \
void LE_NAME##_MarkRead \
( \
    uint32_t msgId \
) \
{ \
    return SmsInbox_MarkRead((SmsInbox_SessionRef_t)LE_NAME##Ref, msgId); \
} \
\
/** \
 * Mark a message as 'unread'. \
 * \
 * @note If the caller is passing a bad message reference into this function, it is a fatal error, \
 *        the function will not return. \
 */ \
void LE_NAME##_MarkUnread \
( \
    uint32_t msgId \
) \
{ \
    return SmsInbox_MarkUnread((SmsInbox_SessionRef_t)LE_NAME##Ref, msgId); \
} \
\
/** \
 * Set the maximum number of messages for message box. \
 * \
 * @return \
 *  - LE_BAD_PARAMETER The message box name is invalid. \
 *  - LE_OVERFLOW      Message count exceed the maximum limit. \
 *  - LE_OK            Function succeeded. \
 *  - LE_FAULT         Function failed. \
 */ \
le_result_t LE_NAME##_SetMaxMessages \
( \
    uint32_t maxMessageCount \
) \
{ \
    return SmsInbox_SetMaxMessages(#LE_NAME, maxMessageCount); \
} \
/** \
 * Get the maximum number of messages for message box. \
 * \
 * @return \
 *  - LE_BAD_PARAMETER The message box name is invalid. \
 *  - LE_OK            Function succeeded. \
 *  - LE_FAULT         Function failed. \
 */ \
le_result_t LE_NAME##_GetMaxMessages \
( \
    uint32_t* maxMessageCountPtr \
) \
{ \
    return SmsInbox_GetMaxMessages(#LE_NAME, maxMessageCountPtr); \
} \

#endif
