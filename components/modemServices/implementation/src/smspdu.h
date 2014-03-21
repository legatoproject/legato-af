/** @file smspdu.h
 *
 * Functions to interact with SMS PDU data.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#include <legato.h>
#include <pa_sms.h>

#ifndef SMSPDU_H_
#define SMSPDU_H_

typedef enum {
    SMSPDU_GSM_7_BITS, ///< Characters are encoded on 7 bits (GSM 03.38)
    SMSPDU_8_BITS,     ///< Informations are treated as raw data on 8 bits
    SMSPDU_UCS_2,      ///< Characters are encoded using UCS-2 on 16 bits
} smspdu_Encoding_t;

//--------------------------------------------------------------------------------------------------
/**
 * Decode the content of dataPtr.
 */
//--------------------------------------------------------------------------------------------------
le_result_t smspdu_Decode
(
    const uint8_t*    dataPtr,  ///< [IN] PDU data to decode
    pa_sms_Message_t* smsPtr    ///< [OUT] Buffer to store decoded data
);

//--------------------------------------------------------------------------------------------------
/**
 * Encode the content of messagePtr as a SMS-DELIVER message (from mobile to service center).
 */
//--------------------------------------------------------------------------------------------------
le_result_t smspdu_Encode
(
    const uint8_t*    messagePtr,   ///< [IN] Data to encode
    size_t            length,       ///< [IN] Length of data
    const char*       addressPtr,   ///< [IN] Phone Number
    smspdu_Encoding_t encoding,     ///< [IN] Type of encoding to be used
    pa_sms_Pdu_t*     pduPtr        ///< [OUT] Buffer for the encoded PDU
);

#endif /* SMSPDU_H_ */
