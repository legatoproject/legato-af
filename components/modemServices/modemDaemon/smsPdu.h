/** @file smsPdu.h
 *
 * Functions to interact with SMS PDU data.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#include "legato.h"
#include "pa_sms.h"

#ifndef SMSPDU_H_
#define SMSPDU_H_

typedef enum {
    SMSPDU_7_BITS = 0x0,        ///< Characters are encoded on 7 bits
                                ///<  (for GSM (GSM 03.38), for CDMA 7bits Ascii)
    SMSPDU_8_BITS = 0x1,        ///< Informations are treated as raw data on 8 bits
    SMSPDU_UCS_2  = 0x2,        ///< Characters are encoded using UCS-2 on 16 bits
    SMSPDU_ENCODING_UNKNOWN,    ///< Unknown encoding format
}
smsPdu_Encoding_t;

//--------------------------------------------------------------------------------------------------
/**
 * Message Type Indicator.
 * It is used for the message service configuration.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    SMSPDU_MSGTYPE_DELIVER       = 0, ///< SMS-DELIVER (in the direction Service Center to Mobile station).
    SMSPDU_MSGTYPE_SUBMIT        = 1, ///< SMS-SUBMIT (in the direction Mobile station to Service Center).
    SMSPDU_MSGTYPE_STATUS_REPORT = 2, ///< SMS-STATUS-REPORT.
}
smsPdu_MsgType_t;

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the module.
 *
 * @return LE_OK
 */
//--------------------------------------------------------------------------------------------------
le_result_t smsPdu_Initialize
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Decode the content of dataPtr.
 *
 * @return LE_OK            Function succeed
 * @return LE_NOT_POSSIBLE  Protocol is not supported
 * @return LE_FAULT         Function failed
 */
//--------------------------------------------------------------------------------------------------
le_result_t smsPdu_Decode
(
    pa_sms_Protocol_t protocol, ///< [IN] decoding protocol
    const uint8_t*    dataPtr,  ///< [IN] PDU data to decode
    size_t            dataSize, ///< [IN] PDU data size
    pa_sms_Message_t* smsPtr    ///< [OUT] Buffer to store decoded data
);

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
/**
 * Encode the content of messagePtr in PDU format.
 *
 * @return LE_OK            Function succeed
 * @return LE_NOT_POSSIBLE  Protocol is not supported
 * @return LE_FAULT         Function failed
 */
//--------------------------------------------------------------------------------------------------
le_result_t smsPdu_Encode
(
    pa_sms_Protocol_t   protocol,     ///< [IN] Encoding protocol
    const uint8_t*      messagePtr,   ///< [IN] Data to encode
    size_t              length,       ///< [IN] Length of data
    const char*         addressPtr,   ///< [IN] Phone Number
    smsPdu_Encoding_t   encoding,     ///< [IN] Type of encoding to be used
    pa_sms_MsgType_t    messageType,  ///< [IN] Message Type
    pa_sms_Pdu_t*       pduPtr        ///< [OUT] Buffer for the encoded PDU
);

#endif /* SMSPDU_H_ */
