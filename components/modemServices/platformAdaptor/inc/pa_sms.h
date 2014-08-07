/**
 * @page c_pa_sms Modem SMS Platform Adapter API
 *
 * @ref pa_sms.h "Click here for the API reference documentation."
 *
 * <HR>
 *
 * @section pa_sms_toc Table of Contents
 *
 *  - @ref pa_sms_intro
 *  - @ref pa_sms_rational
 *
 *
 * @section pa_sms_intro Introduction
 * These APIs are on the top of the platform-dependent adapter layer. They are independent of the
 * implementation. They guarantee the portability on different kind of platform without any changes
 * for the components developped upon these APIs.
 *
 *
 * @section pa_sms_rational Rational
 * These functions are all blocking functions, so that they return when the modem has answered or
 * when a timeout has occured due to an interrupted communication with the modem.
 *
 * They all verify the validity and the range of the input parameters before performing the modem
 * operation.
 *
 * Some functions are used to get some information with a fixed pattern string (i.e.
 * pa_sms_RdMsgFromMem), in this case no buffer overflow will occur has they always get a
 * fixed string length.
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */


/** @file pa_sms.h
 *
 * Legato @ref c_pa_sms include file.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#ifndef LEGATO_PASMS_INCLUDE_GUARD
#define LEGATO_PASMS_INCLUDE_GUARD


#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
/**
 * Message Type Indicator.
 * It is used for the message service configuration.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    PA_SMS_SMS_DELIVER       = 0, ///< SMS-DELIVER (in the direction Service Center to Mobile station).
    PA_SMS_SMS_SUBMIT        = 1, ///< SMS-SUBMIT (in the direction Mobile station to Service Center).
    PA_SMS_SMS_STATUS_REPORT = 2, ///< SMS-STATUS-REPORT.
    PA_SMS_PDU               = 3  ///< PDU message.
}
pa_sms_MsgType_t;


//--------------------------------------------------------------------------------------------------
// APIs.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * SMS-DELIVER message type structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct {
    le_sms_Status_t     status;                           ///< mandatory, status of msg in memory
    char                oa[LE_MDMDEFS_PHONE_NUM_MAX_LEN];          ///< mandatory, originator address
    char                scts[LE_SMS_TIMESTAMP_MAX_LEN];   ///< mandatory, service center timestamp
    le_sms_Format_t     format;                           ///< mandatory, SMS user data format
    uint8_t             data[LE_SMS_TEXT_MAX_LEN];        ///< mandatory, SMS user data
    uint32_t            dataLen;                          ///< mandatory, SMS user data length
}
pa_sms_SmsDeliver_t;

//--------------------------------------------------------------------------------------------------
/**
 * SMS-SUBMIT message type structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct {
    le_sms_Status_t     status;                           ///< mandatory, status of msg in memory
    char                da[LE_MDMDEFS_PHONE_NUM_MAX_LEN]; ///< mandatory, destination address
    le_sms_Format_t     format;                           ///< mandatory, SMS user data format
    uint8_t             data[LE_SMS_TEXT_MAX_LEN];        ///< mandatory, SMS user data
    uint32_t            dataLen;                          ///< mandatory, SMS user data length
}
pa_sms_SmsSubmit_t;

//--------------------------------------------------------------------------------------------------
/**
 * PDU message type structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct {
    le_sms_Status_t     status;                           ///< mandatory, status of msg in memory
    uint8_t             data[LE_SMS_PDU_MAX_LEN];         ///< mandatory, SMS user data (in HEX)
    uint32_t            dataLen;                          ///< mandatory, number of characters
}
pa_sms_Pdu_t;

//--------------------------------------------------------------------------------------------------
/**
 * Generic Message structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct {
    pa_sms_MsgType_t  type;                       ///< Message Type
    union {                                       ///< Associated data and informations
        pa_sms_SmsDeliver_t       smsDeliver;
        pa_sms_SmsSubmit_t        smsSubmit;
        pa_sms_Pdu_t              pdu;
    };
}
pa_sms_Message_t;

//--------------------------------------------------------------------------------------------------
/**
 * Prototype for handler functions used to report that a new message has been received.
 *
 * @param msgRef The message reference in message storage.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*pa_sms_NewMsgHdlrFunc_t)
(
    uint32_t* msgRef
);


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register a handler for a new message reception handling.
 *
 * @return LE_NOT_POSSIBLE  The function failed to register a new handler.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_SetNewMsgHandler
(
    pa_sms_NewMsgHdlrFunc_t msgHandler   ///< [IN] The handler function to handle a new message
                                         ///       reception.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to unregister the handler for a new message reception handling.
 *
 * @return LE_NOT_POSSIBLE  The function failed to unregister the handler.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_ClearNewMsgHandler
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * This function sends a message in PDU mode.
 *
 * @return LE_NOT_POSSIBLE    The function failed to send a message in PDU mode.
 * @return LE_TIMEOUT         No response was received from the Modem.
 * @return a positive value   The function succeeded. The value represents the message reference.
 */
//--------------------------------------------------------------------------------------------------
int32_t pa_sms_SendPduMsg
(
    uint32_t       length, ///< [IN] The length of the TP data unit in bytes.
    const uint8_t* dataPtr ///< [IN] The message.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the message from the preferred message storage.
 *
 * @return LE_NOT_POSSIBLE The function failed to get the message from the preferred message
 *                         storage.
 * @return LE_TIMEOUT      No response was received from the Modem.
 * @return LE_OK           The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_RdPDUMsgFromMem
(
    uint32_t        index,        ///< [IN]  The place of storage in memory.
    pa_sms_Pdu_t*   msgPtr        ///< [OUT] The message.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the indexes of messages stored in the preferred memory for a specific
 * status.
 *
 * @return LE_NOT_POSSIBLE   The function failed to get the indexes of messages stored in the
 *                           preferred memory.
 * @return LE_TIMEOUT        No response was received from the Modem.
 * @return LE_OK             The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_ListMsgFromMem
(
    le_sms_Status_t        status, ///< [IN] The status of message in memory.
    uint32_t*              numPtr, ///< [OUT] The number of indexes retrieved.
    uint32_t*              idxPtr  ///< [OUT] The pointer to an array of indexes. The array is filled
                                   ///        with 'num' index values.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function deletes one specific Message from preferred message storage.
 *
 * @return LE_NOT_POSSIBLE   The function failed to delete one specific Message from preferred
 *                           message storage.
 * @return LE_TIMEOUT        No response was received from the Modem.
 * @return LE_OK             The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_DelMsgFromMem
(
    uint32_t  index   ///< [IN] Index of the message to be deleted.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function deletes all Messages from preferred message storage.
 *
 * @return LE_NOT_POSSIBLE The function failed to delete all Messages from preferred message storage.
 * @return LE_TIMEOUT      No response was received from the Modem.
 * @return LE_OK           The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_DelAllMsg
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * This function changes the message status.
 *
 * @return LE_NOT_POSSIBLE The function failed.
 * @return LE_TIMEOUT      No response was received from the Modem.
 * @return LE_OK           The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_ChangeMessageStatus
(
    uint32_t            index, ///< [IN] Index of the message to be deleted.
    le_sms_Status_t     status ///< [IN] The status of message in memory.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the SMS center.
 *
 * @return LE_NOT_POSSIBLE The function failed.
 * @return LE_TIMEOUT      No response was received from the Modem.
 * @return LE_OK           The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_GetSmsc
(
    char*        smscPtr,  ///< [OUT] The Short message service center string.
    size_t       len       ///< [IN] The length of SMSC string.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the SMS center.
 *
 * @return LE_NOT_POSSIBLE The function failed.
 * @return LE_TIMEOUT      No response was received from the Modem.
 * @return LE_OK           The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_SetSmsc
(
    const char*    smscPtr  ///< [IN] The Short message service center.
);



#endif // LEGATO_PASMS_INCLUDE_GUARD
