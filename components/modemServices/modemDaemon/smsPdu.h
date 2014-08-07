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
    SMSPDU_GSM_7_BITS = 0x0, ///< Characters are encoded on 7 bits (GSM 03.38)
    SMSPDU_8_BITS     = 0x1, ///< Informations are treated as raw data on 8 bits
    SMSPDU_UCS_2      = 0x2, ///< Characters are encoded using UCS-2 on 16 bits
    SMSPDU_ENCODING_UNKNOWN, ///< Unknown encoding format
}
smsPdu_Encoding_t;

//--------------------------------------------------------------------------------------------------
/**
 * Decode the content of dataPtr.
 */
//--------------------------------------------------------------------------------------------------
le_result_t smsPdu_Decode
(
    const uint8_t*    dataPtr,  ///< [IN] PDU data to decode
    pa_sms_Message_t* smsPtr    ///< [OUT] Buffer to store decoded data
);

//--------------------------------------------------------------------------------------------------
/**
 * Encode the content of messagePtr in PDU format.
 */
//--------------------------------------------------------------------------------------------------
le_result_t smsPdu_Encode
(
    const uint8_t*      messagePtr,   ///< [IN] Data to encode
    size_t              length,       ///< [IN] Length of data
    const char*         addressPtr,   ///< [IN] Phone Number
    smsPdu_Encoding_t   encoding,     ///< [IN] Type of encoding to be used
    pa_sms_MsgType_t    messageType,  ///< [IN] Message Type
    pa_sms_Pdu_t*       pduPtr        ///< [OUT] Buffer for the encoded PDU
);

#endif /* SMSPDU_H_ */
