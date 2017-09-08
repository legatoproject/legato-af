/** @file smsPdu.h
 *
 * Functions to interact with SMS PDU data.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "pa_sms.h"

#ifndef SMSPDU_H_
#define SMSPDU_H_

//--------------------------------------------------------------------------------------------------
/**
 * Encoding type to use for the PDU.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    SMSPDU_7_BITS           = 0x0,      ///< Characters are encoded on 7 bits
                                        ///< (for GSM (GSM 03.38), for CDMA 7bits Ascii)
    SMSPDU_8_BITS           = 0x1,      ///< Informations are treated as raw data on 8 bits
    SMSPDU_UCS2_16_BITS     = 0x2,      ///< Characters are encoded using UCS-2 on 16 bits
    SMSPDU_ENCODING_UNKNOWN = 0x3,      ///< Unknown encoding format
}
smsPdu_Encoding_t;

//--------------------------------------------------------------------------------------------------
/**
 * Data used to encode the PDU.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    pa_sms_Protocol_t   protocol;       ///< Message protocol
    const uint8_t*      messagePtr;     ///< Data to encode
    size_t              length;         ///< Length of data
    const char*         addressPtr;     ///< Phone Number
    smsPdu_Encoding_t   encoding;       ///< Type of encoding to be used
    pa_sms_MsgType_t    messageType;    ///< Message Type
    bool                statusReport;   ///< Indicates if SMS Status Report is requested
}
smsPdu_DataToEncode_t;

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
 * @return LE_UNSUPPORTED   Protocol is not supported
 * @return LE_FAULT         Function failed
 */
//--------------------------------------------------------------------------------------------------
le_result_t smsPdu_Decode
(
    pa_sms_Protocol_t protocol, ///< [IN] decoding protocol
    const uint8_t*    dataPtr,  ///< [IN] PDU data to decode
    size_t            dataSize, ///< [IN] PDU data size
    bool              smscInfo, ///< [IN] indicates if PDU starts with SMSC information
    pa_sms_Message_t* smsPtr    ///< [OUT] Buffer to store decoded data
);

//--------------------------------------------------------------------------------------------------
/**
 * Encode the content of messagePtr in PDU format.
 *
 * @return LE_OK            Function succeed
 * @return LE_UNSUPPORTED   Protocol is not supported
 * @return LE_FAULT         Function failed
 */
//--------------------------------------------------------------------------------------------------
le_result_t smsPdu_Encode
(
    smsPdu_DataToEncode_t*  dataPtr,    ///< [IN]  Data to use for encoding the PDU
    pa_sms_Pdu_t*           pduPtr      ///< [OUT] Buffer for the encoded PDU
);

#endif /* SMSPDU_H_ */
