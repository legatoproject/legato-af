/** @file cdmaPdu.c
 *
 * Source code of functions to interact with CDMA PDU data.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "cdmaPdu.h"

// Include macros for printing out values
#include "le_print.h"

typedef struct
{
    const uint8_t  *bufferPtr;  ///< buffer
    uint32_t        index;      ///< current index in buffer
    uint32_t        readCache;
    uint8_t         readCacheSize;
}
readBitsBuffer_t;

typedef struct
{
    uint8_t    *bufferPtr;     ///< buffer
    uint32_t    bufferSize;    ///< buffer size
    uint32_t    index;         ///< current index in buffer
    uint32_t    writeCache;
    uint8_t     writeCacheSize;
}
writeBitsBuffer_t;

//--------------------------------------------------------------------------------------------------
/**
 * Initialize readBitsBuffer_t
 */
//--------------------------------------------------------------------------------------------------
static void InitializeReadBuffer
(
    readBitsBuffer_t *bufferPtr,
    const uint8_t    *dataBufferPtr
)
{
    bufferPtr->bufferPtr = dataBufferPtr;
    bufferPtr->index = 0;
    bufferPtr->readCache = 0;
    bufferPtr->readCacheSize = 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialize writeBitsBuffer_t
 */
//--------------------------------------------------------------------------------------------------
static void InitializeWriteBuffer
(
    writeBitsBuffer_t *bufferPtr,
    uint8_t           *dataBufferPtr,
    uint32_t           dataBufferSize
)
{
    bufferPtr->bufferPtr = dataBufferPtr;
    bufferPtr->bufferSize = dataBufferSize;
    bufferPtr->index = 0;
    bufferPtr->writeCache = 0;
    bufferPtr->writeCacheSize = 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Skip unused bits from cache.
 */
//--------------------------------------------------------------------------------------------------
static void SkipBits
(
    readBitsBuffer_t *bufferPtr
)
{
    bufferPtr->readCache = 0;
    bufferPtr->readCacheSize = 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Return value that corresponds to the next number of bits specified by length
 *
 */
//--------------------------------------------------------------------------------------------------
static uint32_t ReadBits
(
    readBitsBuffer_t* bufferPtr,
    uint8_t           length
)
{
    if (length <= 0 || length > 32)
    {
        LE_WARN("Should not read more that 32 bits");
        return 0;
    }

    if (length > bufferPtr->readCacheSize)
    {
        uint8_t i;
        uint8_t bytesToRead;
        if ( (length - bufferPtr->readCacheSize) % 8 )
        {
            bytesToRead = ((length - bufferPtr->readCacheSize) / 8)+1;
        }
        else
        {
            bytesToRead = ((length - bufferPtr->readCacheSize) / 8);
        }
        for(i = 0; i < bytesToRead; i++)
        {
            bufferPtr->readCache =   (bufferPtr->readCache << 8)
                                | (bufferPtr->bufferPtr[bufferPtr->index++] & 0xFF);
            bufferPtr->readCacheSize += 8;
        }
    }

    uint32_t bitOffset = (bufferPtr->readCacheSize - length);
    uint32_t resultMask = ((uint64_t)1 << length) - 1;
    uint32_t result = 0;

    result = (bufferPtr->readCache >> bitOffset) & resultMask;
    bufferPtr->readCacheSize -= length;

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Flush current write cache with padding 0s.
 */
//--------------------------------------------------------------------------------------------------
static void WritePadding
(
    writeBitsBuffer_t *bufferPtr
)
{
    if (bufferPtr->index>bufferPtr->bufferSize)
    {
        LE_ERROR("Internal buffer overflow [%d]",bufferPtr->bufferSize);
        return;
    }

    if (bufferPtr->writeCacheSize)
    {
      bufferPtr->bufferPtr[bufferPtr->index++] = (bufferPtr->writeCache << (8 - bufferPtr->writeCacheSize));
    }
    bufferPtr->writeCache = 0;
    bufferPtr->writeCacheSize = 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Write the number of bits specified by length from value into the buffer
 *
 */
//--------------------------------------------------------------------------------------------------
static void WriteBits
(
    writeBitsBuffer_t* bufferPtr,
    uint32_t           value,
    uint8_t            length
)
{
    if (length <= 0 || length > 32)
    {
        LE_WARN("Should not write more that 32 bits");
        return;
    }

    if ((bufferPtr->index + (length>>3)) > bufferPtr->bufferSize)
    {
        LE_ERROR("Internal buffer overflow [%d]",bufferPtr->bufferSize);
        return;
    }

    uint8_t totalLength = length + bufferPtr->writeCacheSize;

    // 8-byte cache not full
    if (totalLength < 8) {
        uint32_t valueMask = ((uint64_t)1 << length) - 1;
        bufferPtr->writeCache = (bufferPtr->writeCache << length) | (value & valueMask);
        bufferPtr->writeCacheSize += length;
        return;
    }

    // Deal with unaligned part
    if (bufferPtr->writeCacheSize) {
        uint8_t mergeLength = 8 - bufferPtr->writeCacheSize;
        uint8_t valueMask = (1 << mergeLength) - 1;

        bufferPtr->writeCache =   (bufferPtr->writeCache << mergeLength)
                             | ((value >> (length - mergeLength)) & valueMask);
        bufferPtr->bufferPtr[bufferPtr->index++] = bufferPtr->writeCache & 0xFF;
        length -= mergeLength;
    }

    // Aligned part, just copy
    bufferPtr->writeCache = 0;
    bufferPtr->writeCacheSize = 0;
    while (length >= 8) {
        length -= 8;
        bufferPtr->bufferPtr[bufferPtr->index++] = (value >> length) & 0xFF;
    }

    // Rest part is saved into cache
    bufferPtr->writeCacheSize = length;
    bufferPtr->writeCache = value & (((uint64_t)1 << length) - 1);

    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the Teleservice Identifier structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ReadParameterTeleserviceId
(
    readBitsBuffer_t   *decoderPtr,    ///< [IN/OUT] decoder
    cdmaPdu_t          *cdmaSmsPtr     ///< [OUT] Buffer to store decoded data
)
{
    // Read the parameter Id value
    cdmaSmsPtr->message.teleServiceId = ReadBits(decoderPtr,16);

    // Teleservice Identifier is available
    cdmaSmsPtr->message.parameterMask |= CDMAPDU_PARAMETERMASK_TELESERVICE_ID;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the Service Category structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ReadParameterServiceCategory
(
    readBitsBuffer_t   *decoderPtr,    ///< [IN/OUT] decoder
    cdmaPdu_t          *cdmaSmsPtr     ///< [OUT] Buffer to store decoded data
)
{
    // Read the parameter value
    cdmaSmsPtr->message.serviceCategory = ReadBits(decoderPtr,16);

    // Service Category is available
    cdmaSmsPtr->message.parameterMask |= CDMAPDU_PARAMETERMASK_SERVICE_CATEGORY;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the address parameter structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static bool ReadParameterAddress
(
    readBitsBuffer_t            *decoderPtr,    ///< [IN/OUT] decoder
    cdmaPdu_AddressParameter_t  *addrPtr     ///< [OUT] Buffer to store decoded data
)
{
    // Read the digit mode value
    addrPtr->digitMode = ReadBits(decoderPtr,1);

    // Read the number mode value
    addrPtr->numberMode = ReadBits(decoderPtr,1);

    // Read number type when digit mode is true
    if (addrPtr->digitMode)
    {
        addrPtr->numberType = ReadBits(decoderPtr,3);

        // Read number plan type when number mode is true
        if (addrPtr->numberMode)
        {
            addrPtr->numberPlan = ReadBits(decoderPtr,4);
        }
    }

    // Read the length of CHARi
    uint8_t fieldsNumber = ReadBits(decoderPtr,8);
    addrPtr->fieldsNumber = fieldsNumber;

    // Determine size for copy
    uint32_t sizeChar;
    if (addrPtr->digitMode)
    {
        sizeChar = 8;
    }
    else
    {
        sizeChar = 4;
    }

    // check size of receiving buffer
    if ( sizeChar*fieldsNumber >= sizeof(addrPtr->chari))
    {
        LE_WARN("Internal buffer of originating address is too small %d",sizeChar*fieldsNumber);
        return false;
    }

    // Save each char into the buffer
    writeBitsBuffer_t buffer;
    uint32_t i;

    InitializeWriteBuffer(&buffer,addrPtr->chari,sizeof(addrPtr->chari));
    for (i=0;i<fieldsNumber;i++)
    {
        WriteBits(&buffer,ReadBits(decoderPtr,sizeChar),sizeChar);
    }
    WritePadding(&buffer);

    return true;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the originating address parameter structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ReadParameterOriginatingAddress
(
    readBitsBuffer_t   *decoderPtr,    ///< [IN/OUT] decoder
    cdmaPdu_t          *cdmaSmsPtr     ///< [OUT] Buffer to store decoded data
)
{
    if (ReadParameterAddress(decoderPtr,&cdmaSmsPtr->message.originatingAddr))
    {
        // Originating Address is available
        cdmaSmsPtr->message.parameterMask |= CDMAPDU_PARAMETERMASK_ORIGINATING_ADDR;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the Destination address parameter structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ReadParameterDestinationAddress
(
    readBitsBuffer_t  *decoderPtr,    ///< [IN/OUT] decoder
    cdmaPdu_t         *cdmaSmsPtr     ///< [OUT] Buffer to store decoded data
)
{
    if (ReadParameterAddress(decoderPtr,&cdmaSmsPtr->message.destinationAddr))
    {
        // Destination Address is available
        cdmaSmsPtr->message.parameterMask |= CDMAPDU_PARAMETERMASK_DESTINATION_ADDR;
    }

}

//--------------------------------------------------------------------------------------------------
/**
 * Read the subaddress parameter structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static bool ReadParameterSubAddress
(
    readBitsBuffer_t        *decoderPtr,    ///< [IN/OUT] decoder
    cdmaPdu_SubAddress_t    *subAddrPtr     ///< [OUT] Buffer to store decoded data
)
{
    // Read the type value
    subAddrPtr->type = ReadBits(decoderPtr,3);

    // Read the odd value
    subAddrPtr->odd = ReadBits(decoderPtr,1);

    // Read the length of CHARi
    uint8_t fieldsNumber = ReadBits(decoderPtr,8);
    subAddrPtr->fieldsNumber = fieldsNumber;

    // check size of receiving buffer
    if ( 8*fieldsNumber >= sizeof(subAddrPtr->chari))
    {
        LE_WARN("Internal buffer of originating subaddress is too small %d",8*fieldsNumber);
        return false;
    }

    // Save each char into the buffer
    writeBitsBuffer_t buffer;
    uint32_t i;

    InitializeWriteBuffer(&buffer,subAddrPtr->chari,sizeof(subAddrPtr->chari));
    for (i=0;i<fieldsNumber;i++)
    {
        WriteBits(&buffer,ReadBits(decoderPtr,8),8);
    }
    WritePadding(&buffer);

    return true;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the Originating subaddress parameter structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ReadParameterOriginationSubAddress
(
    readBitsBuffer_t  *decoderPtr,    ///< [IN/OUT] decoder
    cdmaPdu_t         *cdmaSmsPtr     ///< [OUT] Buffer to store decoded data
)
{
    if (ReadParameterSubAddress(decoderPtr,&cdmaSmsPtr->message.originatingSubaddress))
    {
        // Origination SubAddress is available
        cdmaSmsPtr->message.parameterMask |= CDMAPDU_PARAMETERMASK_ORIGINATING_SUBADDR;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the Destination subaddress parameter structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ReadParameterDestinationSubAddress
(
    readBitsBuffer_t   *decoderPtr,    ///< [IN/OUT] decoder
    cdmaPdu_t          *cdmaSmsPtr     ///< [OUT] Buffer to store decoded data
)
{
    if (ReadParameterSubAddress(decoderPtr,&cdmaSmsPtr->message.destinationSubaddress))
    {
        // Destination SubAddress is available
        cdmaSmsPtr->message.parameterMask |= CDMAPDU_PARAMETERMASK_DESTINATION_SUBADDR;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the Bearer Reply Option parameter structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ReadParameterBearerReplyOption
(
    readBitsBuffer_t   *decoderPtr,    ///< [IN/OUT] decoder
    cdmaPdu_t          *cdmaSmsPtr     ///< [OUT] Buffer to store decoded data
)
{
    // Read the type value
    cdmaSmsPtr->message.bearerReplyOption.replySeq = ReadBits(decoderPtr,6);

    // Skip the reserved bits
    SkipBits(decoderPtr);

    // Bearer Reply Option is available
    cdmaSmsPtr->message.parameterMask |= CDMAPDU_PARAMETERMASK_BEARER_REPLY_OPTION;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the Cause Codes parameter structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ReadParameterCauseCodes
(
    readBitsBuffer_t   *decoderPtr,    ///< [IN/OUT] decoder
    cdmaPdu_t          *cdmaSmsPtr     ///< [OUT] Buffer to store decoded data
)
{
    // Read the reply seq value
    cdmaSmsPtr->message.causeCodes.replySeq = ReadBits(decoderPtr,6);

    // Read the error class value
    cdmaSmsPtr->message.causeCodes.errorClass = ReadBits(decoderPtr,2);

    // Read cause code value
    if (cdmaSmsPtr->message.causeCodes.errorClass!=CDMAPDU_ERRORCLASS_NO_ERROR)
    {
        cdmaSmsPtr->message.causeCodes.errorCause = ReadBits(decoderPtr,8);
    }

    // Destination Address is available
    cdmaSmsPtr->message.parameterMask |= CDMAPDU_PARAMETERMASK_CAUSE_CODES;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the Message Identifier subparameter structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ReadSubParameterMessageIdentifier
(
    readBitsBuffer_t   *decoderPtr,    ///< [IN/OUT] decoder
    cdmaPdu_t          *cdmaSmsPtr     ///< [OUT] Buffer to store decoded data
)
{
    // Read the message type value
    cdmaSmsPtr->message.bearerData.messageIdentifier.messageType = ReadBits(decoderPtr,4);

    // Read the message id value
    cdmaSmsPtr->message.bearerData.messageIdentifier.messageIdentifier = ReadBits(decoderPtr,16);

    // Read the header indication value
    cdmaSmsPtr->message.bearerData.messageIdentifier.headerIndication = ReadBits(decoderPtr,1);

    // Skip reserved bits
    SkipBits(decoderPtr);

    // Message Identifier is available
    cdmaSmsPtr->message.bearerData.subParameterMask |= CDMAPDU_SUBPARAMETERMASK_MESSAGE_IDENTIFIER;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the User Data subparameter structure.
 *
 * @TODO: Support all encoding
 */
//--------------------------------------------------------------------------------------------------
static void ReadSubParameterUserData
(
    readBitsBuffer_t   *decoderPtr,    ///< [IN/OUT] decoder
    cdmaPdu_t          *cdmaSmsPtr     ///< [OUT] Buffer to store decoded data
)
{
    // Read the encoding value
    cdmaSmsPtr->message.bearerData.userData.messageEncoding = ReadBits(decoderPtr,5);

    cdmaPdu_Encoding_t encoding = cdmaSmsPtr->message.bearerData.userData.messageEncoding;
    // Read the message type value
    if (   (encoding == CDMAPDU_ENCODING_EXTENDED_PROTOCOL_MESSAGE)
        ||
           (encoding == 10 ) // GSM Data-Coding-Scheme
       )
    {
        cdmaSmsPtr->message.bearerData.userData.messageType = ReadBits(decoderPtr,8);
    }

    // Read the length of CHARi
    uint8_t fieldsNumber = ReadBits(decoderPtr,8);
    cdmaSmsPtr->message.bearerData.userData.fieldsNumber = fieldsNumber;

    uint8_t charBitSize;
    uint8_t index;
    switch (encoding)
    {
        case CDMAPDU_ENCODING_7BIT_ASCII: charBitSize = 7; break;
        case CDMAPDU_ENCODING_OCTET: charBitSize = 8; break;
        case CDMAPDU_ENCODING_UNICODE: charBitSize = 16; break;
        default :
        {
            LE_WARN("encoding %d not supported",encoding);
            return;
        }
    }

    // check size of receiving buffer
    if ( fieldsNumber*charBitSize > sizeof(cdmaSmsPtr->message.bearerData.userData.chari)*8)
    {
        LE_WARN("Internal buffer of userData is too small %d",fieldsNumber);
        return;
    }

    int32_t totalBitSize = fieldsNumber*charBitSize;

    writeBitsBuffer_t buffer;
    InitializeWriteBuffer(&buffer,
        cdmaSmsPtr->message.bearerData.userData.chari,
        sizeof(cdmaSmsPtr->message.bearerData.userData.chari));

    for (index = 0;
         totalBitSize>0;
         totalBitSize-=charBitSize, index++)
    {
        WriteBits(&buffer,ReadBits(decoderPtr,charBitSize),charBitSize);
    }
    WritePadding(&buffer);

    // User data is available
    cdmaSmsPtr->message.bearerData.subParameterMask |= CDMAPDU_SUBPARAMETERMASK_USER_DATA;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the User Response Code subparameter structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ReadSubParameterUserResponseCode
(
    readBitsBuffer_t   *decoderPtr,    ///< [IN/OUT] decoder
    cdmaPdu_t          *cdmaSmsPtr     ///< [OUT] Buffer to store decoded data
)
{
    // Read the response code value
    cdmaSmsPtr->message.bearerData.userResponseCode = ReadBits(decoderPtr,8);

    // User response code is available
    cdmaSmsPtr->message.bearerData.subParameterMask |= CDMAPDU_SUBPARAMETERMASK_USER_RESPONSE_CODE;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the Date subparameter structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ReadSubParameterDate
(
    readBitsBuffer_t   *decoderPtr,  ///< [IN/OUT] decoder
    cdmaPdu_Date_t     *datePtr      ///< [OUT] Buffer to store decoded data
)
{
    // Read the year value
    datePtr->year = ReadBits(decoderPtr,8);

    // Read the month value
    datePtr->month = ReadBits(decoderPtr,8);

    // Read the day value
    datePtr->day = ReadBits(decoderPtr,8);

    // Read the hours value
    datePtr->hours = ReadBits(decoderPtr,8);

    // Read the minutes value
    datePtr->minutes = ReadBits(decoderPtr,8);

    // Read the seconds value
    datePtr->seconds = ReadBits(decoderPtr,8);
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the Message Center Time Stamp subparameter structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ReadSubParameterMessageCenterTimeStamp
(
    readBitsBuffer_t   *decoderPtr,    ///< [IN/OUT] decoder
    cdmaPdu_t          *cdmaSmsPtr     ///< [OUT] Buffer to store decoded data
)
{
    ReadSubParameterDate(decoderPtr,&cdmaSmsPtr->message.bearerData.messageCenterTimeStamp);

    // Message center time stamp is available
    cdmaSmsPtr->message.bearerData.subParameterMask |= CDMAPDU_SUBPARAMETERMASK_MESSAGE_CENTER_TIME_STAMP;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the Validity period absolute subparameter structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ReadSubParameterValidityPeriodAbsolute
(
    readBitsBuffer_t   *decoderPtr,    ///< [IN/OUT] decoder
    cdmaPdu_t          *cdmaSmsPtr     ///< [OUT] Buffer to store decoded data
)
{
    ReadSubParameterDate(decoderPtr,&cdmaSmsPtr->message.bearerData.validityPeriodAbsolute);

    // Validity period absolute is available
    cdmaSmsPtr->message.bearerData.subParameterMask
                |= CDMAPDU_SUBPARAMETERMASK_VALIDITY_PERIOD_ABSOLUTE;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the Validity period relative subparameter structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ReadSubParameterValidityPeriodRelative
(
    readBitsBuffer_t   *decoderPtr,    ///< [IN/OUT] decoder
    cdmaPdu_t          *cdmaSmsPtr     ///< [OUT] Buffer to store decoded data
)
{
    // Read the validity time value
    cdmaSmsPtr->message.bearerData.validityPeriodRelative = ReadBits(decoderPtr,8);

    // Validity period absolute is available
    cdmaSmsPtr->message.bearerData.subParameterMask
                |= CDMAPDU_SUBPARAMETERMASK_VALIDITY_PERIOD_RELATIVE;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the defered delivery time absolute subparameter structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ReadSubParameterDeferedDeliveryTimeAbsolute
(
    readBitsBuffer_t   *decoderPtr,    ///< [IN/OUT] decoder
    cdmaPdu_t          *cdmaSmsPtr     ///< [OUT] Buffer to store decoded data
)
{
    ReadSubParameterDate(decoderPtr,&cdmaSmsPtr->message.bearerData.deferredDeliveryTimeAbsolute);

    // Validity period absolute is available
    cdmaSmsPtr->message.bearerData.subParameterMask
                |= CDMAPDU_SUBPARAMETERMASK_DEFERRED_DELIVERY_TIME_ABSOLUTE;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the defered delivery time relative subparameter structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ReadSubParameterDeferedDeliveryTimeRelative
(
    readBitsBuffer_t   *decoderPtr,    ///< [IN/OUT] decoder
    cdmaPdu_t          *cdmaSmsPtr     ///< [OUT] Buffer to store decoded data
)
{
    // Read the deferred delivety time value
    cdmaSmsPtr->message.bearerData.deferredDeliveryTimeRelative = ReadBits(decoderPtr,8);

    // Validity period absolute is available
    cdmaSmsPtr->message.bearerData.subParameterMask
                |= CDMAPDU_SUBPARAMETERMASK_DEFERRED_DELIVERY_TIME_RELATIVE;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the priority indicator subparameter structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ReadSubParameterPriorityIndicator
(
    readBitsBuffer_t   *decoderPtr,    ///< [IN/OUT] decoder
    cdmaPdu_t          *cdmaSmsPtr     ///< [OUT] Buffer to store decoded data
)
{
    // Read the priority value
    cdmaSmsPtr->message.bearerData.priority = ReadBits(decoderPtr,2);

    // Skip the reserved bits
    SkipBits(decoderPtr);

    // Priority indicator is available
    cdmaSmsPtr->message.bearerData.subParameterMask |= CDMAPDU_SUBPARAMETERMASK_PRIORITY;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the privacy indicator subparameter structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ReadSubParameterPrivacyIndicator
(
    readBitsBuffer_t   *decoderPtr,    ///< [IN/OUT] decoder
    cdmaPdu_t          *cdmaSmsPtr     ///< [OUT] Buffer to store decoded data
)
{
    // Read the privacy value
    cdmaSmsPtr->message.bearerData.privacy = ReadBits(decoderPtr,2);

    // Skip the reserved bits
    SkipBits(decoderPtr);

    // Priority indicator is available
    cdmaSmsPtr->message.bearerData.subParameterMask |= CDMAPDU_SUBPARAMETERMASK_PRIVACY;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the reply option subparameter structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ReadSubParameterReplyOption
(
    readBitsBuffer_t   *decoderPtr,    ///< [IN/OUT] decoder
    cdmaPdu_t          *cdmaSmsPtr     ///< [OUT] Buffer to store decoded data
)
{
    // Read the user ack value
    cdmaSmsPtr->message.bearerData.replyOption.userAck = ReadBits(decoderPtr,1);

    // Read the dak value
    cdmaSmsPtr->message.bearerData.replyOption.deliveryAck = ReadBits(decoderPtr,1);

    // Read the read ack value
    cdmaSmsPtr->message.bearerData.replyOption.readAck = ReadBits(decoderPtr,1);

    // Read the report value
    cdmaSmsPtr->message.bearerData.replyOption.deliveryReport = ReadBits(decoderPtr,1);

    // Skip the reserved bits
    SkipBits(decoderPtr);

    // Priority indicator is available
    cdmaSmsPtr->message.bearerData.subParameterMask |= CDMAPDU_SUBPARAMETERMASK_REPLY_OPTION;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the number of message subparameter structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ReadSubParameterNumberOfMessage
(
    readBitsBuffer_t   *decoderPtr,    ///< [IN/OUT] decoder
    cdmaPdu_t          *cdmaSmsPtr     ///< [OUT] Buffer to store decoded data
)
{
    // Read the Message count value
    cdmaSmsPtr->message.bearerData.messageCount = ReadBits(decoderPtr,8);

    // Number of message is available
    cdmaSmsPtr->message.bearerData.subParameterMask |= CDMAPDU_SUBPARAMETERMASK_MESSAGE_COUNT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the alert on message delivery subparameter structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ReadSubParameterAlertOnMessageDelivery
(
    readBitsBuffer_t   *decoderPtr,    ///< [IN/OUT] decoder
    cdmaPdu_t          *cdmaSmsPtr     ///< [OUT] Buffer to store decoded data
)
{
    // Read the alert priority value
    cdmaSmsPtr->message.bearerData.alertOnMessageDelivery = ReadBits(decoderPtr,2);

    // Skip the reserved bits
    SkipBits(decoderPtr);

    // Alert on message delivery is available
    cdmaSmsPtr->message.bearerData.subParameterMask
                |= CDMAPDU_SUBPARAMETERMASK_ALERT_ON_MESSAGE_DELIVERY;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the language indicator subparameter structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ReadSubParameterLanguageIndicator
(
    readBitsBuffer_t   *decoderPtr,    ///< [IN/OUT] decoder
    cdmaPdu_t          *cdmaSmsPtr     ///< [OUT] Buffer to store decoded data
)
{
    // Read the language value
    cdmaSmsPtr->message.bearerData.alertOnMessageDelivery = ReadBits(decoderPtr,8);

    // Language indicator is available
    cdmaSmsPtr->message.bearerData.subParameterMask |= CDMAPDU_SUBPARAMETERMASK_LANGUAGE;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the call back number subparameter structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ReadSubParameterCallBackNumber
(
    readBitsBuffer_t   *decoderPtr,    ///< [IN/OUT] decoder
    cdmaPdu_t          *cdmaSmsPtr     ///< [OUT] Buffer to store decoded data
)
{
    if (ReadParameterAddress(decoderPtr,&cdmaSmsPtr->message.bearerData.callBackNumber))
    {
        // Originating Address is available
        cdmaSmsPtr->message.parameterMask |= CDMAPDU_SUBPARAMETERMASK_CALL_BACK_NUMBER;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the message display mode subparameter structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ReadSubParameterMessageDisplayMode
(
    readBitsBuffer_t   *decoderPtr,    ///< [IN/OUT] decoder
    cdmaPdu_t          *cdmaSmsPtr     ///< [OUT] Buffer to store decoded data
)
{
    // Read the message display mode value
    cdmaSmsPtr->message.bearerData.messageDisplayMode = ReadBits(decoderPtr,2);

    // Skip the reserved bits
    SkipBits(decoderPtr);

    // Message display mode is available
    cdmaSmsPtr->message.bearerData.subParameterMask
                |= CDMAPDU_SUBPARAMETERMASK_MESSAGE_DISPLAY_MODE;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the message deposit index subparameter structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ReadSubParameterMessageDepositIndex
(
    readBitsBuffer_t   *decoderPtr,    ///< [IN/OUT] decoder
    cdmaPdu_t          *cdmaSmsPtr     ///< [OUT] Buffer to store decoded data
)
{
    // Read the message deposit index value
    cdmaSmsPtr->message.bearerData.messageDepositIndex = ReadBits(decoderPtr,16);

    // Message deposit index is available
    cdmaSmsPtr->message.bearerData.subParameterMask
                |= CDMAPDU_SUBPARAMETERMASK_MESSAGE_DEPOSIT_INDEX;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the message status subparameter structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ReadSubParameterMessageStatus
(
    readBitsBuffer_t   *decoderPtr,    ///< [IN/OUT] decoder
    cdmaPdu_t          *cdmaSmsPtr     ///< [OUT] Buffer to store decoded data
)
{
    // Read the error class value
    cdmaSmsPtr->message.bearerData.messageStatus.errorClass = ReadBits(decoderPtr,2);

    // Read the message status mode value
    cdmaSmsPtr->message.bearerData.messageStatus.messageStatusCode = ReadBits(decoderPtr,6);

    // Message status is available
    cdmaSmsPtr->message.bearerData.subParameterMask
                |= CDMAPDU_SUBPARAMETERMASK_MESSAGE_STATUS;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the TP-Failure cause subparameter structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ReadSubParameterTPFailureCause
(
    readBitsBuffer_t   *decoderPtr,    ///< [IN/OUT] decoder
    cdmaPdu_t          *cdmaSmsPtr     ///< [OUT] Buffer to store decoded data
)
{
    // Read the value
    cdmaSmsPtr->message.bearerData.tpFailureCause = ReadBits(decoderPtr,8);

    // TPFailure cause is available
    cdmaSmsPtr->message.bearerData.subParameterMask |= CDMAPDU_SUBPARAMETERMASK_TP_FAILURE_CAUSE;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the content of SubParameter TLV.
 *
 * return: the length of this SubParameter tlv
 *
 * From Reference : C.S0015-B
 */
//--------------------------------------------------------------------------------------------------
static uint32_t ReadSubParameters
(
    readBitsBuffer_t   *decoderPtr,    ///< [IN/OUT] decoder
    cdmaPdu_t          *cdmaSmsPtr     ///< [OUT] Buffer to store decoded data
)
{
    uint8_t subParameterIndex;
    uint8_t subParameterId;
    uint8_t subParameterLen;

    // Read the parameter Id
    subParameterId = ReadBits(decoderPtr,8);

    // Read the length value
    subParameterLen = ReadBits(decoderPtr,8);
    subParameterIndex = decoderPtr->index;

    switch (subParameterId)
    {
        /* 4.5.1 Message Identifier */
        case 0:
        {
            if (subParameterLen!=3)
            {
                LE_ERROR("%d: Subparameter length should be 3",subParameterId);
            }
            else
            {
                ReadSubParameterMessageIdentifier(decoderPtr,cdmaSmsPtr);
            }
            break;
        }
        /* 4.5.2 User Data */
        case 1:
        {
            ReadSubParameterUserData(decoderPtr,cdmaSmsPtr);
            break;
        }
        /* 4.5.3 User Response Code */
        case 2:
        {
            if (subParameterLen!=1)
            {
                LE_ERROR("%d: Subparameter length should be 1",subParameterId);
            }
            else
            {
                ReadSubParameterUserResponseCode(decoderPtr,cdmaSmsPtr);
            }
            break;
        }
        /* 4.5.4 Message Center Time Stamp */
        case 3:
        {
            if (subParameterLen!=6)
            {
                LE_ERROR("%d: Subparameter length should be 6",subParameterId);
            }
            else
            {
                ReadSubParameterMessageCenterTimeStamp(decoderPtr,cdmaSmsPtr);
            }
            break;
        }
        /* 4.5.5 Validity Period - Absolute */
        case 4:
        {
            if (subParameterLen!=6)
            {
                LE_ERROR("%d: Subparameter length should be 6",subParameterId);
            }
            else
            {
                ReadSubParameterValidityPeriodAbsolute(decoderPtr,cdmaSmsPtr);
            }
            break;
        }
        /* 4.5.6 Validity Period - Relative */
        case 5:
        {
            if (subParameterLen!=1)
            {
                LE_ERROR("%d: Subparameter length should be 1",subParameterId);
            }
            else
            {
                ReadSubParameterValidityPeriodRelative(decoderPtr,cdmaSmsPtr);
            }
            break;
        }
        /* 4.5.7 Deferred Delivery Time - Absolute */
        case 6:
        {
            if (subParameterLen!=6)
            {
                LE_ERROR("%d: Subparameter length should be 6",subParameterId);
            }
            else
            {
                ReadSubParameterDeferedDeliveryTimeAbsolute(decoderPtr,cdmaSmsPtr);
            }
            break;
        }
        /* 4.5.8 Deferred Delivery Time - Relative */
        case 7:
        {
            if (subParameterLen!=1)
            {
                LE_ERROR("%d: Subparameter length should be 1",subParameterId);
            }
            else
            {
                ReadSubParameterDeferedDeliveryTimeRelative(decoderPtr,cdmaSmsPtr);
            }
            break;
        }
        /* 4.5.9 Priority Indicator */
        case 8:
        {
            if (subParameterLen!=1)
            {
                LE_ERROR("%d: Subparameter length should be 1",subParameterId);
            }
            else
            {
                ReadSubParameterPriorityIndicator(decoderPtr,cdmaSmsPtr);
            }
            break;
        }
        /* 4.5.10 Privacy Indicator */
        case 9:
        {
            if (subParameterLen!=1)
            {
                LE_ERROR("%d: Subparameter length should be 1",subParameterId);
            }
            else
            {
                ReadSubParameterPrivacyIndicator(decoderPtr,cdmaSmsPtr);
            }
            break;
        }
        /* 4.5.11 Reply Option */
        case 10:
        {
            if (subParameterLen!=1)
            {
                LE_ERROR("%d: Subparameter length should be 1",subParameterId);
            }
            else
            {
                ReadSubParameterReplyOption(decoderPtr,cdmaSmsPtr);
            }
            break;
        }
        /* 4.5.12 Number of Messages */
        case 11:
        {
            if (subParameterLen!=1)
            {
                LE_ERROR("%d: Subparameter length should be 1",subParameterId);
            }
            else
            {
                ReadSubParameterNumberOfMessage(decoderPtr,cdmaSmsPtr);
            }
            break;
        }
        /* 4.5.13 Alert on Message Delivery */
        case 12:
        {
            if (subParameterLen!=1)
            {
                LE_ERROR("%d: Subparameter length should be 1",subParameterId);
            }
            else
            {
                ReadSubParameterAlertOnMessageDelivery(decoderPtr,cdmaSmsPtr);
            }
            break;
        }
        /* 4.5.14 Language Indicator */
        case 13:
        {
            if (subParameterLen!=1)
            {
                LE_ERROR("%d: Subparameter length should be 1",subParameterId);
            }
            else
            {
                ReadSubParameterLanguageIndicator(decoderPtr,cdmaSmsPtr);
            }
            break;
        }
        /* 4.5.15 Call-Back Number */
        case 14:
        {
            ReadSubParameterCallBackNumber(decoderPtr,cdmaSmsPtr);
            break;
        }
        /* 4.5.16 Message Display Mode */
        case 15:
        {
            if (subParameterLen!=1)
            {
                LE_ERROR("%d: Subparameter length should be 1",subParameterId);
            }
            else
            {
                ReadSubParameterMessageDisplayMode(decoderPtr,cdmaSmsPtr);
            }
            break;
        }
        /* 4.5.18 Message Deposit Index */
        case 17:
        {
            if (subParameterLen!=2)
            {
                LE_ERROR("%d: Subparameter length should be 2",subParameterId);
            }
            else
            {
                ReadSubParameterMessageDepositIndex(decoderPtr,cdmaSmsPtr);
            }
            break;
        }
        /* 4.5.21 Message Status */
        case 20:
        {
            if (subParameterLen!=1)
            {
                LE_ERROR("%d: Subparameter length should be 1",subParameterId);
            }
            else
            {
                ReadSubParameterMessageStatus(decoderPtr,cdmaSmsPtr);
            }
            break;
        }
        /* 4.5.22 TP-Failure Cause */
        case 21:
        {
            if (subParameterLen!=1)
            {
                LE_ERROR("%d: Subparameter length should be 1",subParameterId);
            }
            else
            {
                ReadSubParameterTPFailureCause(decoderPtr,cdmaSmsPtr);
            }
            break;
        }

        /* 4.5.17 Multiple Encoding User Data */
        case 16:
        /* 4.5.19 Service Category Program Data */
        case 18:
        /* 4.5.20 Service Category Program Results */
        case 19:
        /* Enhanced VMN (4.5.23) */
        case 22:
        /* not implemented: Enhanced VMN Ack (4.5.24) */
        case 23:
        default:
        {
            LE_WARN("Do not support this subparameter Id: %d", subParameterId);
        }
    }
    SkipBits(decoderPtr);

    // Reset the index if a read did not work correctly
    decoderPtr->index = subParameterIndex + subParameterLen;

    return (subParameterLen + 2);
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the Bearer Data parameter structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ReadParameterBearerData
(
    readBitsBuffer_t   *decoderPtr,    ///< [IN/OUT] decoder
    uint8_t             length,        ///< [IN] Size of the parameter
    cdmaPdu_t          *cdmaSmsPtr     ///< [OUT] Buffer to store decoded data
)
{
    uint8_t parameterSize = length;

    while (parameterSize)
    {
        // Decode the subparameter
        parameterSize -= ( ReadSubParameters(decoderPtr,cdmaSmsPtr) );
    };

    // Data Bearer is available
    cdmaSmsPtr->message.parameterMask |= CDMAPDU_PARAMETERMASK_BEARER_DATA;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the content of Parameter TLV.
 *
 * return: the length of this Parameter tlv
 *
 * From Reference : C.S0015-B
 */
//--------------------------------------------------------------------------------------------------
static uint32_t ReadParameters
(
    readBitsBuffer_t   *decoderPtr,    ///< [IN/OUT] decoder
    cdmaPdu_t          *cdmaSmsPtr     ///< [OUT] Buffer to store decoded data
)
{
    uint8_t parameterIndex;
    uint8_t parameterId;
    uint8_t parameterLen;

    // Read the parameter Id
    parameterId = ReadBits(decoderPtr,8);

    // Read the length value
    parameterLen = ReadBits(decoderPtr,8);
    parameterIndex = decoderPtr->index;

    switch (parameterId)
    {
        /* 3.4.3.1 Teleservice Identifier */
        case 0:
        {
            if (parameterLen != 2)
            {
                LE_ERROR("%d: SMS message parameter length should be 2",parameterId);
            }
            else
            {
                ReadParameterTeleserviceId(decoderPtr,cdmaSmsPtr);
            }
            break;
        }
        /* 3.4.3.2 Service Category */
        case 1:
        {
            if (parameterLen != 2)
            {
                LE_ERROR("%d: SMS message parameter length should be 2",parameterId);
            }
            else
            {
                ReadParameterServiceCategory(decoderPtr,cdmaSmsPtr);
            }
            break;
        }
        /* 3.4.3.3 Address Parameters */
        case 2:
        {
            ReadParameterOriginatingAddress(decoderPtr,cdmaSmsPtr);
            break;
        }
        /* 3.4.3.4 Subaddress */
        case 3:
        {
            ReadParameterOriginationSubAddress(decoderPtr,cdmaSmsPtr);
            break;
        }
        /* 3.4.3.3 Address Parameters */
        case 4:
        {
            ReadParameterDestinationAddress(decoderPtr,cdmaSmsPtr);
            break;
        }
        /* 3.4.3.4 Subaddress */
        case 5:
        {
            ReadParameterDestinationSubAddress(decoderPtr,cdmaSmsPtr);
            break;
        }
        /* 3.4.3.5 Bearer Reply Option */
        case 6:
        {
            if (parameterLen != 1)
            {
                LE_ERROR("%d: SMS message parameter length should be 1",parameterId);
            }
            else
            {
                ReadParameterBearerReplyOption(decoderPtr,cdmaSmsPtr);
            }
            break;
        }
        /* 3.4.3.6 Cause Codes */
        case 7:
        {
            ReadParameterCauseCodes(decoderPtr,cdmaSmsPtr);
            break;
        }
        /* 3.4.3.7 Bearer Data */
        case 8:
        {
            ReadParameterBearerData(decoderPtr,parameterLen,cdmaSmsPtr);
            break;
        }
        default:
        {
            LE_WARN("Do not support this Parameter Id: %d", parameterId);
        }
    }
    SkipBits(decoderPtr);

    // Reset the index if a read did not work correctly
    decoderPtr->index = parameterIndex + parameterLen;

    return (parameterLen + 2);
}

//--------------------------------------------------------------------------------------------------
/**
 * Write the Teleservice Identifier structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static void WriteParameterTeleserviceId
(
    const cdmaPdu_t     *cdmaSmsPtr,    ///< [IN] Buffer to store decoded data
    writeBitsBuffer_t  *encoderPtr     ///< [IN/OUT] encoder
)
{
    // Write the TVL Id value
    WriteBits(encoderPtr,0,8);

    // Write the TLV Length value
    WriteBits(encoderPtr,2,8);

    // Write the parameter Id value
    WriteBits(encoderPtr,cdmaSmsPtr->message.teleServiceId,16);
}

//--------------------------------------------------------------------------------------------------
/**
 * Write the service category structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static void WriteParameterServiceCategory
(
    const cdmaPdu_t     *cdmaSmsPtr,    ///< [IN] Buffer to store decoded data
    writeBitsBuffer_t  *encoderPtr     ///< [IN/OUT] encoder
)
{
    // Write the TVL Id value
    WriteBits(encoderPtr,1,8);

    // Write the TLV Length value
    WriteBits(encoderPtr,2,8);

    // Write the service category value
    WriteBits(encoderPtr,cdmaSmsPtr->message.serviceCategory,16);
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the address parameter structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static bool WriteParameterAddress
(
    const cdmaPdu_AddressParameter_t *addrPtr,     ///< [IN] Buffer to store decoded data
    writeBitsBuffer_t               *encoderPtr   ///< [IN/OUT] encoder
)
{
    // Reserved the TLV Length value
    uint32_t lengthPosition = encoderPtr->index;
    WriteBits(encoderPtr,0,8);

    // write the digit mode value
    WriteBits(encoderPtr,addrPtr->digitMode,1);

    // write the number mode value
    WriteBits(encoderPtr,addrPtr->numberMode,1);

    // Write number type when digit mode is true
    if (addrPtr->digitMode)
    {
        WriteBits(encoderPtr,addrPtr->numberType,3);

        // Write number plan type when number mode is false
        if (addrPtr->numberMode==false)
        {
            WriteBits(encoderPtr,addrPtr->numberPlan,4);
        }
    }

    // Write the length of CHARi
    WriteBits(encoderPtr,addrPtr->fieldsNumber,8);

    // Determine size for copy
    uint32_t sizeChar;
    if (addrPtr->digitMode)
    {
        sizeChar = 8;
    }
    else
    {
        sizeChar = 4;
    }

    // Save each char into the buffer
    readBitsBuffer_t buffer;
    uint32_t i;

    InitializeReadBuffer(&buffer,addrPtr->chari);
    for (i=0;i<addrPtr->fieldsNumber;i++)
    {
        WriteBits(encoderPtr,ReadBits(&buffer,sizeChar),sizeChar);
    }
    WritePadding(encoderPtr);

    // update the TLV Length value
    uint32_t endPosition = encoderPtr->index-1;
    encoderPtr->bufferPtr[lengthPosition] = ((endPosition-lengthPosition)&0xFF);

    return true;
}

//--------------------------------------------------------------------------------------------------
/**
 * Write the origination address structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static void WriteParameterOriginatingAddress
(
    const cdmaPdu_t     *cdmaSmsPtr,    ///< [IN] Buffer to store decoded data
    writeBitsBuffer_t  *encoderPtr     ///< [IN/OUT] encoderPtr
)
{
    // Write the TVL Id value
    WriteBits(encoderPtr,2,8);

    // Write the origination address value
    WriteParameterAddress(&cdmaSmsPtr->message.originatingAddr, encoderPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Write the destination address structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static void WriteParameterDestinationAddress
(
    const cdmaPdu_t     *cdmaSmsPtr,    ///< [IN] Buffer to store decoded data
    writeBitsBuffer_t  *encoderPtr     ///< [IN/OUT] encoderPtr
)
{
    // Write the TVL Id value
    WriteBits(encoderPtr,4,8);

    // Write the destination address value
    WriteParameterAddress(&cdmaSmsPtr->message.destinationAddr, encoderPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Write the subaddress parameter structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static bool WriteParameterSubAddress
(
    const cdmaPdu_SubAddress_t  *subAddrPtr,   ///< [IN] Buffer to store decoded data
    writeBitsBuffer_t          *encoderPtr    ///< [IN/OUT] encoder
)
{
    // Reserved the TLV Length value
    uint32_t lengthPosition = encoderPtr->index;
    WriteBits(encoderPtr,0,8);

    // Write the type value
    WriteBits(encoderPtr,subAddrPtr->type,3);

    // Write the odd value
    WriteBits(encoderPtr,subAddrPtr->odd,1);

    // Read the length of CHARi
    WriteBits(encoderPtr,subAddrPtr->fieldsNumber,8);

    // Save each char into the buffer
    readBitsBuffer_t buffer;
    uint32_t i;

    InitializeReadBuffer(&buffer,subAddrPtr->chari);
    for (i=0;i<subAddrPtr->fieldsNumber;i++)
    {
        WriteBits(encoderPtr,ReadBits(&buffer,8),8);
    }
    WritePadding(encoderPtr);

    // update the TLV Length value
    uint32_t endPosition = encoderPtr->index-1;
    encoderPtr->bufferPtr[lengthPosition] = ((endPosition-lengthPosition)&0xFF);

    return true;
}

//--------------------------------------------------------------------------------------------------
/**
 * Write the origination subaddress structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static void WriteParameterOriginatingSubAddress
(
    const cdmaPdu_t     *cdmaSmsPtr,    ///< [IN] Buffer to store decoded data
    writeBitsBuffer_t  *encoderPtr     ///< [IN/OUT] encoderPtr
)
{
    // Write the TVL Id value
    WriteBits(encoderPtr,3,8);

    // Write the origination subaddress value
    WriteParameterSubAddress(&cdmaSmsPtr->message.originatingSubaddress, encoderPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Write the desitnation subaddress structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static void WriteParameterDestinationSubAddress
(
    const cdmaPdu_t     *cdmaSmsPtr,    ///< [IN] Buffer to store decoded data
    writeBitsBuffer_t  *encoderPtr     ///< [IN/OUT] encoderPtr
)
{
    // Write the TVL Id value
    WriteBits(encoderPtr,5,8);

    // Write the destination subaddress value
    WriteParameterSubAddress(&cdmaSmsPtr->message.destinationSubaddress, encoderPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Write the Bearer Reply Option parameter structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static void WriteParameterBearerReplyOption
(
    const cdmaPdu_t     *cdmaSmsPtr,    ///< [IN] Buffer to store decoded data
    writeBitsBuffer_t  *encoderPtr     ///< [IN/OUT] encoderPtr
)
{
    // Write the TVL Id value
    WriteBits(encoderPtr,6,8);

    // Write the TLV Length value
    WriteBits(encoderPtr,1,8);

    // Write the reply seq value
    WriteBits(encoderPtr,cdmaSmsPtr->message.bearerReplyOption.replySeq,6);

    // Skip the reserved bits
    WritePadding(encoderPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Write the Cause codes parameter structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static void WriteParameterCauseCodes
(
    const cdmaPdu_t     *cdmaSmsPtr,    ///< [IN] Buffer to store decoded data
    writeBitsBuffer_t  *encoderPtr     ///< [IN/OUT] encoderPtr
)
{
    // Write the TVL Id value
    WriteBits(encoderPtr,7,8);

    // Reserved the TLV Length value
    uint32_t lengthPosition = encoderPtr->index;
    WriteBits(encoderPtr,0,8);

    // Write the reply seq value
    WriteBits(encoderPtr,cdmaSmsPtr->message.causeCodes.replySeq,6);

    // Write the error class value
    WriteBits(encoderPtr,cdmaSmsPtr->message.causeCodes.errorClass,2);

    if (cdmaSmsPtr->message.causeCodes.errorClass!=CDMAPDU_ERRORCLASS_NO_ERROR)
    {
        WriteBits(encoderPtr,cdmaSmsPtr->message.causeCodes.errorCause,8);
    }

    // update the TLV Length value
    uint32_t endPosition = encoderPtr->index-1;
    encoderPtr->bufferPtr[lengthPosition] = ((endPosition-lengthPosition)&0xFF);
}

//--------------------------------------------------------------------------------------------------
/**
 * Write the Message Identifier subparameter structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static void WriteSubParameterMessageIdentifier
(
    const cdmaPdu_t     *cdmaSmsPtr,    ///< [IN] Buffer to store decoded data
    writeBitsBuffer_t  *encoderPtr     ///< [IN/OUT] encoderPtr
)
{
    // Write the TVL Id value
    WriteBits(encoderPtr,0,8);

    // Write the TLV Length value
    WriteBits(encoderPtr,3,8);

    // Write the message type value
    WriteBits(encoderPtr,cdmaSmsPtr->message.bearerData.messageIdentifier.messageType,4);

    // Write the message id value
    WriteBits(encoderPtr,cdmaSmsPtr->message.bearerData.messageIdentifier.messageIdentifier,16);

    // Write the header indication value
    WriteBits(encoderPtr,cdmaSmsPtr->message.bearerData.messageIdentifier.headerIndication,1);

    // Skip reserved bits
    WritePadding(encoderPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Write the User Data subparameter structure.
 *
 * @TODO: Support all encoding
 */
//--------------------------------------------------------------------------------------------------
static void WriteSubParameterUserData
(
    const cdmaPdu_t     *cdmaSmsPtr,    ///< [IN] Buffer to store decoded data
    writeBitsBuffer_t  *encoderPtr     ///< [IN/OUT] encoderPtr
)
{
    // Write the TVL Id value
    WriteBits(encoderPtr,1,8);

    // Reserved the TLV Length value
    uint32_t lengthPosition = encoderPtr->index;
    WriteBits(encoderPtr,0,8);

    // Write the encoding value
    WriteBits(encoderPtr,cdmaSmsPtr->message.bearerData.userData.messageEncoding,5);

    cdmaPdu_Encoding_t encoding = cdmaSmsPtr->message.bearerData.userData.messageEncoding;
    // Write the message type value
    if (   (encoding == CDMAPDU_ENCODING_EXTENDED_PROTOCOL_MESSAGE)
        ||
           (encoding == 10 ) // GSM Data-Coding-Scheme
       )
    {
        LE_WARN("WRITE MESSAGE TYPE");
         WriteBits(encoderPtr,cdmaSmsPtr->message.bearerData.userData.messageType,8);
    }

    // Write the length of CHARi
    WriteBits(encoderPtr,cdmaSmsPtr->message.bearerData.userData.fieldsNumber,8);

    uint8_t charBitSize;
    uint8_t index;
    switch (encoding)
    {
        case CDMAPDU_ENCODING_7BIT_ASCII: charBitSize = 7; break;
        case CDMAPDU_ENCODING_OCTET: charBitSize = 8; break;
        case CDMAPDU_ENCODING_UNICODE: charBitSize = 16; break;
        default :
        {
            LE_WARN("encoding %d not supported",encoding);
            return;
        }
    }

    int32_t totalBitSize = cdmaSmsPtr->message.bearerData.userData.fieldsNumber*charBitSize;

    readBitsBuffer_t buffer;
    InitializeReadBuffer(&buffer,cdmaSmsPtr->message.bearerData.userData.chari);

    for (index = 0;
         totalBitSize>0;
         totalBitSize-=charBitSize, index++)
    {
        WriteBits(encoderPtr,ReadBits(&buffer,charBitSize),charBitSize);
    }
    WritePadding(encoderPtr);

    // update the TLV Length value
    uint32_t endPosition = encoderPtr->index-1;
    encoderPtr->bufferPtr[lengthPosition] = ((endPosition-lengthPosition)&0xFF);
}

//--------------------------------------------------------------------------------------------------
/**
 * Write the User Response Code subparameter structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static void WriteSubParameterUserResponseCode
(
    const cdmaPdu_t     *cdmaSmsPtr,    ///< [IN] Buffer to store decoded data
    writeBitsBuffer_t  *encoderPtr     ///< [IN/OUT] encoderPtr
)
{
    // Write the TVL Id value
    WriteBits(encoderPtr,2,8);

    // Write the TLV Length value
    WriteBits(encoderPtr,1,8);

    // write the response code value
    WriteBits(encoderPtr,cdmaSmsPtr->message.bearerData.userResponseCode,8);
}

//--------------------------------------------------------------------------------------------------
/**
 * Write the Date subparameter structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static void WriteSubParameterDate
(
    const cdmaPdu_Date_t *datePtr,    ///< [IN] Buffer to store decoded data
    writeBitsBuffer_t   *encoderPtr  ///< [IN/OUT] decoder
)
{
    // Write the year value
    WriteBits(encoderPtr,datePtr->year,8);

    // Write the month value
    WriteBits(encoderPtr,datePtr->month,8);

    // Write the day value
    WriteBits(encoderPtr,datePtr->day,8);

    // Write the hours value
    WriteBits(encoderPtr,datePtr->hours,8);

    // Write the minutes value
    WriteBits(encoderPtr,datePtr->minutes,8);

    // Write the seconds value
    WriteBits(encoderPtr,datePtr->seconds,8);
}

//--------------------------------------------------------------------------------------------------
/**
 * Write the Message Center Time Stamp subparameter structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static void WriteSubParameterMessageCenterTimeStamp
(
    const cdmaPdu_t     *cdmaSmsPtr,    ///< [IN] Buffer to store decoded data
    writeBitsBuffer_t  *encoderPtr     ///< [IN/OUT] encoderPtr
)
{
    // Write the TVL Id value
    WriteBits(encoderPtr,3,8);

    // Write the TLV Length value
    WriteBits(encoderPtr,6,8);

    // Write the time stamp
    WriteSubParameterDate(&cdmaSmsPtr->message.bearerData.messageCenterTimeStamp,encoderPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Write the Validity period absolute subparameter structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static void WriteSubParameterValidityPeriodAbsolute
(
    const cdmaPdu_t     *cdmaSmsPtr,    ///< [IN] Buffer to store decoded data
    writeBitsBuffer_t  *encoderPtr     ///< [IN/OUT] encoderPtr
)
{
    // Write the TVL Id value
    WriteBits(encoderPtr,4,8);

    // Write the TLV Length value
    WriteBits(encoderPtr,6,8);

    // Write the validity period
    WriteSubParameterDate(&cdmaSmsPtr->message.bearerData.validityPeriodAbsolute,encoderPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Write the Validity period relative subparameter structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static void WriteSubParameterValidityPeriodRelative
(
    const cdmaPdu_t     *cdmaSmsPtr,    ///< [IN] Buffer to store decoded data
    writeBitsBuffer_t  *encoderPtr     ///< [IN/OUT] encoderPtr
)
{
    // Write the TVL Id value
    WriteBits(encoderPtr,5,8);

    // Write the TLV Length value
    WriteBits(encoderPtr,1,8);

    // Write the validity period value
    WriteBits(encoderPtr,cdmaSmsPtr->message.bearerData.validityPeriodRelative,8);
}

//--------------------------------------------------------------------------------------------------
/**
 * Write the Deferred delivery time absolute subparameter structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static void WriteSubParameterDeferredDeliveyTimeAbsolute
(
    const cdmaPdu_t     *cdmaSmsPtr,    ///< [IN] Buffer to store decoded data
    writeBitsBuffer_t  *encoderPtr     ///< [IN/OUT] encoderPtr
)
{
    // Write the TVL Id value
    WriteBits(encoderPtr,6,8);

    // Write the TLV Length value
    WriteBits(encoderPtr,6,8);

    // Write the deferred delivery time
    WriteSubParameterDate(&cdmaSmsPtr->message.bearerData.deferredDeliveryTimeAbsolute,encoderPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Write the Deferred delivery time relative subparameter structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static void WriteSubParameterDeferredDeliveyTimeRelative
(
    const cdmaPdu_t     *cdmaSmsPtr,    ///< [IN] Buffer to store decoded data
    writeBitsBuffer_t  *encoderPtr     ///< [IN/OUT] encoderPtr
)
{
    // Write the TVL Id value
    WriteBits(encoderPtr,7,8);

    // Write the TLV Length value
    WriteBits(encoderPtr,1,8);

    // Write the deferred delivery time
    WriteBits(encoderPtr,cdmaSmsPtr->message.bearerData.deferredDeliveryTimeRelative,8);
}


//--------------------------------------------------------------------------------------------------
/**
 * Write the priority indicator subparameter structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static void WriteSubParameterPriorityIndicator
(
    const cdmaPdu_t     *cdmaSmsPtr,    ///< [IN] Buffer to store decoded data
    writeBitsBuffer_t  *encoderPtr     ///< [IN/OUT] encoderPtr
)
{
    // Write the TVL Id value
    WriteBits(encoderPtr,8,8);

    // Write the TLV Length value
    WriteBits(encoderPtr,1,8);

    // Read the priority value
    WriteBits(encoderPtr,cdmaSmsPtr->message.bearerData.priority,2);

    // Skip the reserved bits
    WritePadding(encoderPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Write the privacy indicator subparameter structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static void WriteSubParameterPrivacyIndicator
(
    const cdmaPdu_t     *cdmaSmsPtr,    ///< [IN] Buffer to store decoded data
    writeBitsBuffer_t  *encoderPtr     ///< [IN/OUT] encoderPtr
)
{
    // Write the TVL Id value
    WriteBits(encoderPtr,9,8);

    // Write the TLV Length value
    WriteBits(encoderPtr,1,8);

    // Read the priority value
    WriteBits(encoderPtr,cdmaSmsPtr->message.bearerData.privacy,2);

    // Skip the reserved bits
    WritePadding(encoderPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Write the reply option subparameter structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static void WriteSubParameterReplyOption
(
    const cdmaPdu_t     *cdmaSmsPtr,    ///< [IN] Buffer to store decoded data
    writeBitsBuffer_t  *encoderPtr     ///< [IN/OUT] encoderPtr
)
{
    // Write the TVL Id value
    WriteBits(encoderPtr,10,8);

    // Write the TLV Length value
    WriteBits(encoderPtr,1,8);

    // Write the user ack value
    WriteBits(encoderPtr,cdmaSmsPtr->message.bearerData.replyOption.userAck,1);

    // Write the dak value
    WriteBits(encoderPtr,cdmaSmsPtr->message.bearerData.replyOption.deliveryAck,1);

    // Write the read ack value
    WriteBits(encoderPtr,cdmaSmsPtr->message.bearerData.replyOption.readAck,1);

    // Write the report value
    WriteBits(encoderPtr,cdmaSmsPtr->message.bearerData.replyOption.deliveryReport,1);

    // Skip the reserved bits
    WritePadding(encoderPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Write the number of message subparameter structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static void WriteSubParameterNumberOfMessage
(
    const cdmaPdu_t     *cdmaSmsPtr,    ///< [IN] Buffer to store decoded data
    writeBitsBuffer_t  *encoderPtr     ///< [IN/OUT] encoderPtr
)
{
    // Write the TVL Id value
    WriteBits(encoderPtr,11,8);

    // Write the TLV Length value
    WriteBits(encoderPtr,1,8);

    // Write the Message count value
    WriteBits(encoderPtr,cdmaSmsPtr->message.bearerData.messageCount,8);
}

//--------------------------------------------------------------------------------------------------
/**
 * Write the alert on message delivery subparameter structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static void WriteSubParameterAlertOnMessageDelivery
(
    const cdmaPdu_t     *cdmaSmsPtr,    ///< [IN] Buffer to store decoded data
    writeBitsBuffer_t  *encoderPtr     ///< [IN/OUT] encoderPtr
)
{
    // Write the TVL Id value
    WriteBits(encoderPtr,12,8);

    // Write the TLV Length value
    WriteBits(encoderPtr,1,8);

    // Write the alert priority value
    WriteBits(encoderPtr,cdmaSmsPtr->message.bearerData.alertOnMessageDelivery,2);

    // Skip the reserved bits
    WritePadding(encoderPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Write the language indicator subparameter structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static void WriteSubParameterLanguageIndicator
(
    const cdmaPdu_t     *cdmaSmsPtr,    ///< [IN] Buffer to store decoded data
    writeBitsBuffer_t  *encoderPtr     ///< [IN/OUT] encoderPtr
)
{
    // Write the TVL Id value
    WriteBits(encoderPtr,13,8);

    // Write the TLV Length value
    WriteBits(encoderPtr,1,8);

    // Write the language value
    WriteBits(encoderPtr,cdmaSmsPtr->message.bearerData.alertOnMessageDelivery,8);
}

//--------------------------------------------------------------------------------------------------
/**
 * Write the call back number subparameter structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static void WriteSubParameterCallBackNumber
(
    const cdmaPdu_t     *cdmaSmsPtr,    ///< [IN] Buffer to store decoded data
    writeBitsBuffer_t  *encoderPtr     ///< [IN/OUT] encoderPtr
)
{
    // Write the TVL Id value
    WriteBits(encoderPtr,14,8);

    // Write the call back number
    WriteParameterAddress(&cdmaSmsPtr->message.bearerData.callBackNumber,encoderPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Write the message display mode subparameter structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static void WriteSubParameterMessageDisplayMode
(
    const cdmaPdu_t     *cdmaSmsPtr,    ///< [IN] Buffer to store decoded data
    writeBitsBuffer_t  *encoderPtr     ///< [IN/OUT] encoderPtr
)
{
    // Write the TVL Id value
    WriteBits(encoderPtr,15,8);

    // Write the TLV Length value
    WriteBits(encoderPtr,1,8);

    // Write the message display mode value
    WriteBits(encoderPtr,cdmaSmsPtr->message.bearerData.messageDisplayMode,2);

    // Skip the reserved bits
    WritePadding(encoderPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Write the message deposit index subparameter structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static void WriteSubParameterMessageDepositIndex
(
    const cdmaPdu_t     *cdmaSmsPtr,    ///< [IN] Buffer to store decoded data
    writeBitsBuffer_t  *encoderPtr     ///< [IN/OUT] encoderPtr
)
{
    // Write the TVL Id value
    WriteBits(encoderPtr,17,8);

    // Write the TLV Length value
    WriteBits(encoderPtr,2,8);

    // Write the message deposit index value
    WriteBits(encoderPtr,cdmaSmsPtr->message.bearerData.messageDepositIndex,16);
}

//--------------------------------------------------------------------------------------------------
/**
 * Write the message status subparameter structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static void WriteSubParameterMessageStatus
(
    const cdmaPdu_t     *cdmaSmsPtr,    ///< [IN] Buffer to store decoded data
    writeBitsBuffer_t  *encoderPtr     ///< [IN/OUT] encoderPtr
)
{
    // Write the TVL Id value
    WriteBits(encoderPtr,20,8);

    // Write the TLV Length value
    WriteBits(encoderPtr,1,8);

    // Write the error class value
    WriteBits(encoderPtr,cdmaSmsPtr->message.bearerData.messageStatus.errorClass,2);

    // Write the message status mode value
    WriteBits(encoderPtr,cdmaSmsPtr->message.bearerData.messageStatus.messageStatusCode,6);
}

//--------------------------------------------------------------------------------------------------
/**
 * Write the TP-Failure cause subparameter structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static void WriteSubParameterTPFailureCause
(
    const cdmaPdu_t     *cdmaSmsPtr,    ///< [IN] Buffer to store decoded data
    writeBitsBuffer_t  *encoderPtr     ///< [IN/OUT] encoderPtr
)
{
    // Write the TVL Id value
    WriteBits(encoderPtr,21,8);

    // Write the TLV Length value
    WriteBits(encoderPtr,1,8);

    // Read the value
    WriteBits(encoderPtr,cdmaSmsPtr->message.bearerData.tpFailureCause,8);
}

//--------------------------------------------------------------------------------------------------
/**
 * Write the Bearer data parameter structure.
 *
 */
//--------------------------------------------------------------------------------------------------
static void WriteParameterBearerData
(
    const cdmaPdu_t     *cdmaSmsPtr,    ///< [IN] Buffer to store decoded data
    writeBitsBuffer_t   *encoderPtr     ///< [IN/OUT] encoderPtr
)
{
    // Write the TVL Id value
    WriteBits(encoderPtr,8,8);

    // Reserved the TLV Length value
    uint32_t lengthPosition = encoderPtr->index;
    WriteBits(encoderPtr,0,8);

    // write Message Identifer
    if (   cdmaSmsPtr->message.bearerData.subParameterMask
         & CDMAPDU_SUBPARAMETERMASK_MESSAGE_IDENTIFIER)
    {
        WriteSubParameterMessageIdentifier(cdmaSmsPtr,encoderPtr);
    }

    // write User data
    if ( cdmaSmsPtr->message.bearerData.subParameterMask & CDMAPDU_SUBPARAMETERMASK_USER_DATA)
    {
        WriteSubParameterUserData(cdmaSmsPtr,encoderPtr);
    }

    // write User data code response
    if (   cdmaSmsPtr->message.bearerData.subParameterMask
         & CDMAPDU_SUBPARAMETERMASK_USER_RESPONSE_CODE)
    {
        WriteSubParameterUserResponseCode(cdmaSmsPtr,encoderPtr);
    }

    // write Message center time stamp
    if (   cdmaSmsPtr->message.bearerData.subParameterMask
         & CDMAPDU_SUBPARAMETERMASK_MESSAGE_CENTER_TIME_STAMP)
    {
        WriteSubParameterMessageCenterTimeStamp(cdmaSmsPtr,encoderPtr);
    }

    // write Validity period - Absolute
    if (   cdmaSmsPtr->message.bearerData.subParameterMask
         & CDMAPDU_SUBPARAMETERMASK_VALIDITY_PERIOD_ABSOLUTE)
    {
        WriteSubParameterValidityPeriodAbsolute(cdmaSmsPtr,encoderPtr);
    }

    // write Validity period - Relative
    if (   cdmaSmsPtr->message.bearerData.subParameterMask
         & CDMAPDU_SUBPARAMETERMASK_VALIDITY_PERIOD_RELATIVE)
    {
        WriteSubParameterValidityPeriodRelative(cdmaSmsPtr,encoderPtr);
    }

    // write Deferred delivery time - Absolute
    if (   cdmaSmsPtr->message.bearerData.subParameterMask
         & CDMAPDU_SUBPARAMETERMASK_DEFERRED_DELIVERY_TIME_ABSOLUTE)
    {
        WriteSubParameterDeferredDeliveyTimeAbsolute(cdmaSmsPtr,encoderPtr);
    }

    // write Deferred delivery time - Relative
    if (   cdmaSmsPtr->message.bearerData.subParameterMask
         & CDMAPDU_SUBPARAMETERMASK_DEFERRED_DELIVERY_TIME_RELATIVE)
    {
        WriteSubParameterDeferredDeliveyTimeRelative(cdmaSmsPtr,encoderPtr);
    }

    // write priority
    if ( cdmaSmsPtr->message.bearerData.subParameterMask & CDMAPDU_SUBPARAMETERMASK_PRIORITY)
    {
        WriteSubParameterPriorityIndicator(cdmaSmsPtr,encoderPtr);
    }

    // write privacy
    if ( cdmaSmsPtr->message.bearerData.subParameterMask & CDMAPDU_SUBPARAMETERMASK_PRIVACY)
    {
        WriteSubParameterPrivacyIndicator(cdmaSmsPtr,encoderPtr);
    }

    // write reply option
    if ( cdmaSmsPtr->message.bearerData.subParameterMask & CDMAPDU_SUBPARAMETERMASK_REPLY_OPTION)
    {
        WriteSubParameterReplyOption(cdmaSmsPtr,encoderPtr);
    }

    // write number of message
    if ( cdmaSmsPtr->message.bearerData.subParameterMask & CDMAPDU_SUBPARAMETERMASK_MESSAGE_COUNT)
    {
        WriteSubParameterNumberOfMessage(cdmaSmsPtr,encoderPtr);
    }

    // write Alert on Message delivery
    if (   cdmaSmsPtr->message.bearerData.subParameterMask
         & CDMAPDU_SUBPARAMETERMASK_ALERT_ON_MESSAGE_DELIVERY)
    {
        WriteSubParameterAlertOnMessageDelivery(cdmaSmsPtr,encoderPtr);
    }

    // write language indicator
    if ( cdmaSmsPtr->message.bearerData.subParameterMask & CDMAPDU_SUBPARAMETERMASK_LANGUAGE)
    {
        WriteSubParameterLanguageIndicator(cdmaSmsPtr,encoderPtr);
    }

    // write call back number
    if (   cdmaSmsPtr->message.bearerData.subParameterMask
         & CDMAPDU_SUBPARAMETERMASK_CALL_BACK_NUMBER)
    {
        WriteSubParameterCallBackNumber(cdmaSmsPtr,encoderPtr);
    }

    // write message display mode
    if (   cdmaSmsPtr->message.bearerData.subParameterMask
         & CDMAPDU_SUBPARAMETERMASK_MESSAGE_DISPLAY_MODE)
    {
        WriteSubParameterMessageDisplayMode(cdmaSmsPtr,encoderPtr);
    }

    // write multiple ending user data
    // Not supported

    // write message deposit index
    if (   cdmaSmsPtr->message.bearerData.subParameterMask
         & CDMAPDU_SUBPARAMETERMASK_MESSAGE_DEPOSIT_INDEX)
    {
        WriteSubParameterMessageDepositIndex(cdmaSmsPtr,encoderPtr);
    }

    // write Service Category Program Data
    // Not supported

    // write Service Category Program Results
    // Not supported

    // write message status
    if ( cdmaSmsPtr->message.bearerData.subParameterMask & CDMAPDU_SUBPARAMETERMASK_MESSAGE_STATUS)
    {
        WriteSubParameterMessageStatus(cdmaSmsPtr,encoderPtr);
    }

    // write TP-Failure cause
    if (   cdmaSmsPtr->message.bearerData.subParameterMask
         & CDMAPDU_SUBPARAMETERMASK_TP_FAILURE_CAUSE)
    {
        WriteSubParameterTPFailureCause(cdmaSmsPtr,encoderPtr);
    }

    // write Enhanced VMN
    // Not supported

    // write Enhanced VMN Ack
    // Not supported

    // update the TLV Length value
    uint32_t endPosition = encoderPtr->index-1;
    encoderPtr->bufferPtr[lengthPosition] = ((endPosition-lengthPosition)&0xFF);
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the content of dataPtr.
 *
 * return LE_OK function succeed
 */
//--------------------------------------------------------------------------------------------------
le_result_t cdmaPdu_Decode
(
    const uint8_t   *dataPtr,       ///< [IN] PDU data to decode
    size_t           dataPtrSize,   ///< [IN] Size of the dataPtr
    cdmaPdu_t       *cdmaSmsPtr     ///< [OUT] Buffer to store decoded data
)
{
    readBitsBuffer_t pduBuffer;
    size_t pduSize = dataPtrSize;

    // Reset the output.
    memset(cdmaSmsPtr,0,sizeof(*cdmaSmsPtr));

    // Initialize the decoder
    InitializeReadBuffer(&pduBuffer, dataPtr);

    // Read message format
    cdmaSmsPtr->messageFormat = ReadBits(&pduBuffer,8);
    pduSize--;

    while (pduSize>0)
    {
        // Decode the parameter
        pduSize -= ( ReadParameters(&pduBuffer,cdmaSmsPtr) );
    };

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Encode the content of dataPtr.
 *
 * @return LE_OK        function succeed
 * @return LE_OVERFLOW  dataPtr is too small
 * @return LE_FAULT     function failed
 *
 * @TODO: improvement Add checking on the teleservice Identifier to know which optional TLV to add.
 */
//--------------------------------------------------------------------------------------------------
le_result_t cdmaPdu_Encode
(
    const cdmaPdu_t *cdmaSmsPtr,    ///< [IN] Buffer to store decoded data
    uint8_t         *dataPtr,       ///< [OUT] PDU data to decode
    size_t           dataPtrSize,   ///< [IN] Size of the dataPtr
    uint32_t        *pduByteSize    ///< [OUT] size of the encoded pdu in bytes
)
{
    writeBitsBuffer_t pduBuffer;

    // Reset the output.
    memset(dataPtr,0,dataPtrSize);

    // Initialize the encoder
    InitializeWriteBuffer(&pduBuffer, dataPtr, dataPtrSize);

    // Write message format
    WriteBits(&pduBuffer,cdmaSmsPtr->messageFormat,8);

    // write teleservice Id
    if ( cdmaSmsPtr->message.parameterMask & CDMAPDU_PARAMETERMASK_TELESERVICE_ID)
    {
        WriteParameterTeleserviceId(cdmaSmsPtr,&pduBuffer);
    }

    // write Service category
    if ( cdmaSmsPtr->message.parameterMask & CDMAPDU_PARAMETERMASK_SERVICE_CATEGORY)
    {
        WriteParameterServiceCategory(cdmaSmsPtr,&pduBuffer);
    }

    // write originating Address Parameters
    if ( cdmaSmsPtr->message.parameterMask & CDMAPDU_PARAMETERMASK_ORIGINATING_ADDR)
    {
        WriteParameterOriginatingAddress(cdmaSmsPtr,&pduBuffer);
    }

    // write originating SubAddress Parameters
    if ( cdmaSmsPtr->message.parameterMask & CDMAPDU_PARAMETERMASK_ORIGINATING_SUBADDR)
    {
        WriteParameterOriginatingSubAddress(cdmaSmsPtr,&pduBuffer);
    }

    // write destination Address Parameters
    if ( cdmaSmsPtr->message.parameterMask & CDMAPDU_PARAMETERMASK_DESTINATION_ADDR)
    {
        WriteParameterDestinationAddress(cdmaSmsPtr,&pduBuffer);
    }

    // write destination SubAddress Parameters
    if ( cdmaSmsPtr->message.parameterMask & CDMAPDU_PARAMETERMASK_DESTINATION_SUBADDR)
    {
        WriteParameterDestinationSubAddress(cdmaSmsPtr,&pduBuffer);
    }

    // write bearer reply option Parameters
    if ( cdmaSmsPtr->message.parameterMask & CDMAPDU_PARAMETERMASK_BEARER_REPLY_OPTION)
    {
        WriteParameterBearerReplyOption(cdmaSmsPtr,&pduBuffer);
    }

    // write Cause Codes Parameters
    if ( cdmaSmsPtr->message.parameterMask & CDMAPDU_PARAMETERMASK_CAUSE_CODES)
    {
        WriteParameterCauseCodes(cdmaSmsPtr,&pduBuffer);
    }

    // write bearer data Parameters
    if ( cdmaSmsPtr->message.parameterMask & CDMAPDU_PARAMETERMASK_BEARER_DATA)
    {
        WriteParameterBearerData(cdmaSmsPtr,&pduBuffer);
    }

    // Save the PDU encoded size
    *pduByteSize = pduBuffer.index;

    // Check overflow
    if (pduBuffer.index>pduBuffer.bufferSize)
    {
        return LE_OVERFLOW;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Print the content of the structure cdmaPdu_t
 */
//--------------------------------------------------------------------------------------------------
void cdmaPdu_Dump
(
    const cdmaPdu_t *cdmaSmsPtr     ///< [IN] Buffer to store decoded data
)
{
    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->messageFormat);

    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.parameterMask);

    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.teleServiceId);

    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.originatingAddr.digitMode);
    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.originatingAddr.numberMode);
    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.originatingAddr.numberType);
    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.originatingAddr.numberPlan);
    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.originatingAddr.fieldsNumber);
    if (cdmaSmsPtr->message.originatingAddr.digitMode)
    {
        LE_PRINT_ARRAY("%2x",cdmaSmsPtr->message.originatingAddr.fieldsNumber,
                             cdmaSmsPtr->message.originatingAddr.chari);
    }
    else
    {
        LE_PRINT_ARRAY("%2x",cdmaSmsPtr->message.originatingAddr.fieldsNumber/2,
                             cdmaSmsPtr->message.originatingAddr.chari);
    }

    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.destinationAddr.digitMode);
    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.destinationAddr.numberMode);
    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.destinationAddr.numberType);
    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.destinationAddr.numberPlan);
    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.destinationAddr.fieldsNumber);
    if (cdmaSmsPtr->message.destinationAddr.digitMode)
    {
        LE_PRINT_ARRAY("0x%2x",cdmaSmsPtr->message.destinationAddr.fieldsNumber,
                               cdmaSmsPtr->message.destinationAddr.chari);
    }
    else
    {
        LE_PRINT_ARRAY("0x%2x",cdmaSmsPtr->message.destinationAddr.fieldsNumber/2,
                               cdmaSmsPtr->message.destinationAddr.chari);
    }

    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.serviceCategory);

    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.originatingSubaddress.type);
    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.originatingSubaddress.odd);
    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.originatingSubaddress.fieldsNumber);
    LE_PRINT_ARRAY("0x%2x",cdmaSmsPtr->message.originatingSubaddress.fieldsNumber,
                           cdmaSmsPtr->message.originatingSubaddress.chari);

    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.destinationSubaddress.type);
    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.destinationSubaddress.odd);
    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.destinationSubaddress.fieldsNumber);
    LE_PRINT_ARRAY("0x%x",cdmaSmsPtr->message.destinationSubaddress.fieldsNumber,
                          cdmaSmsPtr->message.originatingSubaddress.chari);

    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.bearerReplyOption.replySeq);

    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.causeCodes.replySeq);
    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.causeCodes.errorClass);
    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.causeCodes.errorCause);

    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.bearerData.subParameterMask);

    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.bearerData.messageIdentifier.messageType);
    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.bearerData.messageIdentifier.messageIdentifier);
    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.bearerData.messageIdentifier.headerIndication);

    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.bearerData.userData.messageEncoding);
    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.bearerData.userData.messageType);
    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.bearerData.userData.fieldsNumber);
    LE_PRINT_ARRAY("0x%2x",cdmaSmsPtr->message.bearerData.userData.fieldsNumber,
                           cdmaSmsPtr->message.bearerData.userData.chari);

    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.bearerData.userResponseCode);

    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.bearerData.messageCenterTimeStamp.year);
    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.bearerData.messageCenterTimeStamp.month);
    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.bearerData.messageCenterTimeStamp.day);
    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.bearerData.messageCenterTimeStamp.hours);
    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.bearerData.messageCenterTimeStamp.minutes);
    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.bearerData.messageCenterTimeStamp.seconds);

    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.bearerData.validityPeriodAbsolute.year);
    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.bearerData.validityPeriodAbsolute.month);
    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.bearerData.validityPeriodAbsolute.day);
    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.bearerData.validityPeriodAbsolute.hours);
    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.bearerData.validityPeriodAbsolute.minutes);
    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.bearerData.validityPeriodAbsolute.seconds);

    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.bearerData.validityPeriodRelative);

    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.bearerData.deferredDeliveryTimeAbsolute.year);
    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.bearerData.deferredDeliveryTimeAbsolute.month);
    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.bearerData.deferredDeliveryTimeAbsolute.day);
    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.bearerData.deferredDeliveryTimeAbsolute.hours);
    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.bearerData.deferredDeliveryTimeAbsolute.minutes);
    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.bearerData.deferredDeliveryTimeAbsolute.seconds);

    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.bearerData.deferredDeliveryTimeRelative);

    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.bearerData.priority);

    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.bearerData.privacy);

    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.bearerData.replyOption.userAck);
    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.bearerData.replyOption.deliveryAck);
    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.bearerData.replyOption.readAck);
    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.bearerData.replyOption.deliveryReport);

    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.bearerData.messageCount);

    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.bearerData.alertOnMessageDelivery);

    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.bearerData.language);

    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.bearerData.callBackNumber.digitMode);
    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.bearerData.callBackNumber.numberMode);
    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.bearerData.callBackNumber.numberType);
    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.bearerData.callBackNumber.numberPlan);
    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.bearerData.callBackNumber.fieldsNumber);
    if (cdmaSmsPtr->message.bearerData.callBackNumber.digitMode)
    {
        LE_PRINT_ARRAY("0x%2x",cdmaSmsPtr->message.bearerData.callBackNumber.fieldsNumber,
                               cdmaSmsPtr->message.bearerData.callBackNumber.chari);
    }
    else
    {
        LE_PRINT_ARRAY("0x%2x",cdmaSmsPtr->message.bearerData.callBackNumber.fieldsNumber/2,
                               cdmaSmsPtr->message.bearerData.callBackNumber.chari);
    }

    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.bearerData.messageDisplayMode);

    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.bearerData.messageStatus.errorClass);
    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.bearerData.messageStatus.messageStatusCode);

    LE_PRINT_VALUE("0x%x",cdmaSmsPtr->message.bearerData.tpFailureCause);
}
