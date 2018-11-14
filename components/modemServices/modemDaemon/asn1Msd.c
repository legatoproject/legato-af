/** @file asn1Msd.c
 *
 * Source code of functions to build the MSD needed by eCall.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "asn1Msd.h"

//--------------------------------------------------------------------------------------------------
//                                       Static declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * ...
 *
 */

//--------------------------------------------------------------------------------------------------
static const uint8_t PrintableAsciiCodes[33] =
{
    0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,
    0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x4A,0x4B,0x4C,
    0x4D,0x4E,0x50,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A
};

//--------------------------------------------------------------------------------------------------
/**
 * This function checks the validity of the Vehicle Identification Number.
 *
 * @return true if the VIN is valid, false otherwise.
 */
//--------------------------------------------------------------------------------------------------
static bool IsVinValid
(
    msd_Vin_t vin
)
{
    char vinstr[sizeof(vin) + 1] = {'0'};
    int  i=0;

    memcpy(vinstr, (void*)&vin, sizeof(vin));

    for(i=0 ; (i<sizeof(vin)) &&
              (vinstr[i]) &&
              (NULL!=strchr("0123456789ABCDEFGHJKLMNPRSTUVWXYZ", vinstr[i])) ; i++)
    {
    }

    if (i != sizeof(vin))
    {
        LE_ERROR("Invalid Vehicle Identification Number (%s), it must be a %zd-character string, "
                 "upper-case and it can't contain 'I', 'O' or 'Q' letters",
                 vinstr,
                 sizeof(vin));
        return false;
    }
    else
    {
        return true;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function returns tha ASCII code of a character.
 * The supported characters are 0-9 and A-Z.
 *
 * @return The ASCII code
 */
//--------------------------------------------------------------------------------------------------
static int8_t GetAsciiCode
(
    uint8_t character
)
{
    int8_t index = 0;

    while ( (index < 33) && (character!=PrintableAsciiCodes[index]))
    {
        index++;
    }

    if (index == sizeof(PrintableAsciiCodes))
    {
        index = -1;
    }

    return index;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function insert an 'up to 8-bit' element in the MSD message
 *
 * @return the updated offset after the element insertion
 */
//--------------------------------------------------------------------------------------------------
static uint16_t PutBits
(
    uint16_t msgOffset,   ///< [IN] Element position in the MSD message
    uint16_t elmtLen,     ///< [IN] Element length in bits
    uint8_t* elmtPtr,     ///< [IN] Pointer to the element to insert
    uint8_t* msgPtr       ///< [OUT] updated MSD message with the new element
)
{
    register uint8_t elmtPos;
    register uint8_t msgPos;
    register uint8_t val;
    register int8_t shift;
    uint8_t mask;
    uint16_t i ;
    uint8_t bitMask [8] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};

    msgPos = (msgOffset & 0x07);      /* Modulus 8      */
    msgPtr += msgOffset >> 3;         /* integral div 8 */
    elmtPos  = 8 - ((elmtLen & 0x07) ? elmtLen & 0x07 : (elmtLen & 0x07) + 8);

    for (i = 0 ; i < elmtLen ; i++)
    {
        val = (*elmtPtr) &  bitMask [elmtPos];
        mask = bitMask [elmtPos];
        shift = msgPos - elmtPos;
        if (shift >= 0)
        {
            val >>= shift;
            mask >>= shift;
        }
        else
        {
            val <<= (- shift);
            mask <<= (-shift);
        }
        *msgPtr &= ~mask;
        *msgPtr |= val;
        elmtPos++;
        msgPos++;
        if (elmtPos > 7) /* byte change in elmtPtr */
        {
            elmtPtr++;
            elmtPos = 0;
        }
        if (msgPos > 7) /* byte change in elmtPtr */
        {
            msgPtr++;
            msgPos = 0;
        }
    }

    return (msgOffset + elmtLen);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function insert an 'up to 2-byte' element in the MSD message
 *
 * @return the updated offset after the element insertion
 */
//--------------------------------------------------------------------------------------------------
static uint16_t PutTwoBytes
(
    uint16_t  msgOffset,   ///< [IN] Element position in the MSD message
    uint16_t  elmtLen,     ///< [IN] Element length in bits
    uint16_t* elmtPtr,     ///< [IN] Pointer to the element to insert
    uint8_t*  msgPtr       ///< [OUT] updated MSD message with the new element
)
{
   uint16_t  msgOffsetCurrent = msgOffset;
   uint8_t n;
   uint16_t setBit[16] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80,
                           0x100, 0x200, 0x400, 0x800, 0x1000, 0x2000, 0x4000, 0x8000 };

   for( n = 0; n < elmtLen; n++ )
   {
      if( *(elmtPtr) & setBit[elmtLen-1-n] )
      {
         msgPtr[ (msgOffsetCurrent>>3) ] |= 0x01<<(7-( msgOffsetCurrent&0x07 ));
      }
      else
      {
         msgPtr[ (msgOffsetCurrent>>3) ] &=~( 0x01<<(7-( msgOffsetCurrent&0x07 )));
      }
      msgOffsetCurrent++;
   }

   return (msgOffset + elmtLen);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function encodes the MSD message optional data from the elements of the MSD data structure
 *
 * @return the MSD message optional data length in bits on success
 * @return LE_FAULT on failure
 *
 * @note Only MSD version 1 is supported.
 */
//--------------------------------------------------------------------------------------------------
static int32_t msd_EncodeMsdMessageOptionalData
(
    uint16_t    offset,     ///< [IN] offset of encoded MSD message
    msd_t*      msdDataPtr, ///< [IN] MSD data
    uint8_t*    outDataPtr  ///< [OUT] encoded MSD message
)
{
    int i;
    uint8_t length = msdDataPtr->msdMsg.optionalData.oidlen;
    uint8_t oidLength = length;
    uint8_t* optionalDataPtr= (uint8_t*) msdDataPtr->msdMsg.optionalData.data;

    /* OID */
    // The OID digit value encoding must conform to ITU-T X.690 chapter 8.20.2:
    // The contents octets shall be an (ordered) list of encodings of sub-identifiers
    // (see 8.20.3 and 8.20.4) concatenated together. Each sub-identifier is represented
    // as a series of (one or more) octets. Bit 8 of each octet indicates whether it is
    // the last in the series: bit 8 of the last octet is zero; bit 8 of each preceding octet
    //is one. Bits 7-1 of the octets in the series collectively encode the sub-identifier.
    // Conceptually, these groups of bits are concatenated to form an unsigned binary number
    // whose most significant bit is bit 7 of the first octet and whose least significant bit
    // is bit 1 of the last octet. The sub-identifier shall be encoded in the fewest possible
    // octets, that is, the leading octet of the sub-identifier shall not have the value 0x80.

    // Update OID length (+1) with OID value(s) greater than 127
    for(i=0; i<length; i++)
    {
        if ( msdDataPtr->msdMsg.optionalData.oid[i] > 127 )
        {
            oidLength++;
        }
    }
    // Put updated OID length value
    offset = PutBits(offset, 8, &oidLength, outDataPtr);

    /* Put OID value */
    for(i=0; i<length; i++)
    {
        // OID encoded thru 7 bits: additional Bytes added for OID value(s) greater than 127
        if ( msdDataPtr->msdMsg.optionalData.oid[i] > 127 )
        {
            uint8_t oidFirstByte=0x00;
            uint8_t oidSecondByte=0x00;
            // OID b7 -> OID first Byte b0
            oidFirstByte = (msdDataPtr->msdMsg.optionalData.oid[i] >> 7)& 0x01;
            // OID first Byte: b7 set to 1
            oidFirstByte |= 0x80;
            // OID b6-b0 -> OID second Byte b6-b0
            oidSecondByte = msdDataPtr->msdMsg.optionalData.oid[i] & 0x7F;
            offset = PutBits(offset, 8, (uint8_t*)&oidFirstByte, outDataPtr);
            offset = PutBits(offset, 8, (uint8_t*)&oidSecondByte, outDataPtr);
        } else
        {
            offset = PutBits(offset, 8, &msdDataPtr->msdMsg.optionalData.oid[i], outDataPtr);
        }
    }

    /* Put optional data to MSD message */
    length = msdDataPtr->msdMsg.optionalData.dataLen;
    offset = PutBits(offset, 8, &length, outDataPtr);
    for(i=0; i<length; i++)
    {
        offset = PutBits(offset, 8, &optionalDataPtr[i], outDataPtr);
    }

    return offset;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function encodes the MSD message from the elements of the MSD data structure
 *
 * @return the MSD message length in bytes on success
 * @return LE_FAULT on failure
 *
 */
//--------------------------------------------------------------------------------------------------
int32_t msd_EncodeMsdMessage
(
    msd_t*      msdDataPtr, ///< [IN] MSD data
    uint8_t*    outDataPtr  ///< [OUT] encoded MSD message
)
{
    uint8_t off = 0;
    uint16_t offset=0;
    int i;
    uint16_t offsetV2=0;
    uint8_t *outDataV2Ptr;


    /* MSD Format */
    if ((msdDataPtr->version != 1)&&(msdDataPtr->version != 2))
    {
        LE_ERROR("MSD version %d not supported", msdDataPtr->version);
        return LE_FAULT;
    }
    offset = PutBits(offset, 8, &msdDataPtr->version, outDataPtr);

    /* MSD structure size field for MSD V2 coding (left empty and compute at the end) */
    offsetV2 = offset;
    outDataV2Ptr = outDataPtr;
    if (msdDataPtr->version == 2)
    {
       /* Length for MSD structure */
       offset = PutBits(offset, 8, &off, outDataPtr);
    }

    /* Extension bit */
    offset = PutBits(offset, 1, &off, outDataPtr);

    /* Optional Data Presence */
    offset = PutBits(offset, 1, (uint8_t*)&msdDataPtr->msdMsg.optionalDataPres, outDataPtr);

    /* ** MSD structure ** */
    /* Extension bit */
    offset = PutBits(offset, 1, &off, outDataPtr);

    /* Optional field presence indication */
    offset = PutBits(offset, 1, (uint8_t*)&msdDataPtr->msdMsg.msdStruct.recentVehLocationN1Pres
             , outDataPtr);
    offset = PutBits(offset, 1, (uint8_t*)&msdDataPtr->msdMsg.msdStruct.recentVehLocationN2Pres
             , outDataPtr);
    offset = PutBits(offset, 1, (uint8_t*)&msdDataPtr->msdMsg.msdStruct.numberOfPassengersPres
             , outDataPtr);

    /* Message Identifier */
    offset = PutBits(offset, 8, &msdDataPtr->msdMsg.msdStruct.messageIdentifier, outDataPtr);

    /* Control Type */
    offset = PutBits(offset, 1, (uint8_t*)&msdDataPtr->msdMsg.msdStruct.control.automaticActivation
             , outDataPtr);
    offset = PutBits(offset, 1, (uint8_t*)&msdDataPtr->msdMsg.msdStruct.control.testCall
             , outDataPtr);
    offset = PutBits(offset, 1, (uint8_t*)&msdDataPtr->msdMsg.msdStruct.control.positionCanBeTrusted
             , outDataPtr);
    /* vehType : Only enumerated values are supported : no extension*/
    offset = PutBits(offset, 1, &off, outDataPtr); /* extension bit */
    {
        uint8_t tmp = msdDataPtr->msdMsg.msdStruct.control.vehType;
        tmp -= 1;

        offset = PutBits(offset, 4, &tmp , outDataPtr);
    }

    /* Vehicle identification Number */
    if (!IsVinValid(msdDataPtr->msdMsg.msdStruct.vehIdentificationNumber))
    {
        LE_ERROR("Cannot encode Vehicle Identification Number!");
        return LE_FAULT;
    }

    /* Each character is coded within 6 bits according to translation table */
    /* isowmi */
    for (i=0;i<3;i++)
    {
        uint8_t* ptr = (uint8_t*) msdDataPtr->msdMsg.msdStruct.vehIdentificationNumber.isowmi;
        int8_t tmp = GetAsciiCode (ptr[i]);
        /* check if character is authorized */
        if (tmp <0)
        {
            LE_ERROR("Unable to get ASCII code for isowmi");
            return LE_FAULT;
        }
        offset = PutBits(offset, 6, (uint8_t*) &tmp, outDataPtr);
    }

    /* isovdsvds */
    for (i=0;i<6;i++)
    {
        uint8_t* ptr = (uint8_t*) msdDataPtr->msdMsg.msdStruct.vehIdentificationNumber.isovds;
        int8_t tmp = GetAsciiCode (ptr[i]);
        /* check if character is authorized */
        if (tmp <0)
        {
            LE_ERROR("Unable to get ASCII code for isovds");
            return LE_FAULT;
        }
        offset = PutBits(offset, 6, (uint8_t*) &tmp, outDataPtr);
    }

    /* isovisModelyear */
    for (i=0;i<1;i++)
    {
        uint8_t* ptr =
            (uint8_t*) msdDataPtr->msdMsg.msdStruct.vehIdentificationNumber.isovisModelyear;
        int8_t tmp = GetAsciiCode (ptr[i]);
        /* check if character is authorized */
        if (tmp <0)
        {
            LE_ERROR("Unable to get ASCII code for isovisModelyear");
            return LE_FAULT;
        }
        offset = PutBits(offset, 6, (uint8_t*) &tmp, outDataPtr);
    }

    /* isovisSeqPlant */
    for (i=0;i<7;i++)
    {
        uint8_t* ptr =
            (uint8_t*) msdDataPtr->msdMsg.msdStruct.vehIdentificationNumber.isovisSeqPlant;
        int8_t tmp = GetAsciiCode (ptr[i]);
        /* check if character is authorized */
        if (tmp <0)
        {
            LE_ERROR("Unable to get ASCII code for isovisSeqPlant");
            return LE_FAULT;
        }
        offset = PutBits(offset, 6, (uint8_t*) &tmp, outDataPtr);
    }

    /* VehiclePropulsionStorageType */
    /* Extension bit */
    offset = PutBits(offset, 1, &off, outDataPtr);

    offset = PutBits(offset, 1
             , (uint8_t*)&msdDataPtr->msdMsg.msdStruct.vehPropulsionStorageType.gasolineTankPresent
             , outDataPtr);
    offset = PutBits(offset, 1
             , (uint8_t*)&msdDataPtr->msdMsg.msdStruct.vehPropulsionStorageType.dieselTankPresent
             , outDataPtr);
    offset = PutBits(offset, 1
    , (uint8_t*)&msdDataPtr->msdMsg.msdStruct.vehPropulsionStorageType.compressedNaturalGas
    , outDataPtr);
    offset = PutBits(offset, 1
             , (uint8_t*)&msdDataPtr->msdMsg.msdStruct.vehPropulsionStorageType.liquidPropaneGas
             , outDataPtr);
    offset = PutBits(offset, 1
    , (uint8_t*)&msdDataPtr->msdMsg.msdStruct.vehPropulsionStorageType.electricEnergyStorage
    , outDataPtr);
    offset = PutBits(offset, 1
             , (uint8_t*)&msdDataPtr->msdMsg.msdStruct.vehPropulsionStorageType.hydrogenStorage
             , outDataPtr);
    if (msdDataPtr->version == 2)
    {
        offset = PutBits(offset, 1
                 , (uint8_t*)&msdDataPtr->msdMsg.msdStruct.vehPropulsionStorageType.otherStorage
                 , outDataPtr);
    }

    if ( msdDataPtr->msdMsg.msdStruct.vehPropulsionStorageType.gasolineTankPresent )
    {
        offset = PutBits(offset, 1
        , (uint8_t*)&msdDataPtr->msdMsg.msdStruct.vehPropulsionStorageType.gasolineTankPresent
        , outDataPtr);
    }
    if ( msdDataPtr->msdMsg.msdStruct.vehPropulsionStorageType.dieselTankPresent )
    {
        offset = PutBits(offset, 1
        , (uint8_t*)&msdDataPtr->msdMsg.msdStruct.vehPropulsionStorageType.dieselTankPresent
        , outDataPtr);
    }
    if ( msdDataPtr->msdMsg.msdStruct.vehPropulsionStorageType.compressedNaturalGas )
    {
        offset = PutBits(offset, 1
        , (uint8_t*)&msdDataPtr->msdMsg.msdStruct.vehPropulsionStorageType.compressedNaturalGas
        , outDataPtr);
    }
    if ( msdDataPtr->msdMsg.msdStruct.vehPropulsionStorageType.liquidPropaneGas )
    {
        offset = PutBits(offset, 1
        , (uint8_t*)&msdDataPtr->msdMsg.msdStruct.vehPropulsionStorageType.liquidPropaneGas
        , outDataPtr);
    }
    if ( msdDataPtr->msdMsg.msdStruct.vehPropulsionStorageType.electricEnergyStorage )
    {
        offset = PutBits(offset, 1
        , (uint8_t*)&msdDataPtr->msdMsg.msdStruct.vehPropulsionStorageType.electricEnergyStorage
        , outDataPtr);
    }
    if ( msdDataPtr->msdMsg.msdStruct.vehPropulsionStorageType.hydrogenStorage )
    {
        offset = PutBits(offset, 1
        , (uint8_t*)&msdDataPtr->msdMsg.msdStruct.vehPropulsionStorageType.hydrogenStorage
        , outDataPtr);
    }
    if (msdDataPtr->version == 2)
    {
       if ( msdDataPtr->msdMsg.msdStruct.vehPropulsionStorageType.otherStorage )
       {
           offset = PutBits(offset, 1
           , (uint8_t*)&msdDataPtr->msdMsg.msdStruct.vehPropulsionStorageType.otherStorage
           , outDataPtr);
       }
    }

    /* Timestamp ( 32 bits ==> 4 * 8 bits ==> to check order ) */
    {
        uint8_t* ptr = (uint8_t*) &msdDataPtr->msdMsg.msdStruct.timestamp;

        for (i=0;i<4;i++)
        {
            offset = PutBits(offset, 8, (uint8_t*) &ptr[3-i], outDataPtr);
        }
    }

    /* vehLocation idem on 32 bits for latitude and longitude */
    /* latitude */
    {
        int32_t latitudeTmp = msdDataPtr->msdMsg.msdStruct.vehLocation.latitude;
        uint8_t* ptr = (uint8_t*) &latitudeTmp;
        if((latitudeTmp < -324000000) || (latitudeTmp > 324000000))
        {
            if(latitudeTmp != 0x7FFFFFFF)
            {
                LE_ERROR("Bad latitude value.%d", latitudeTmp);
                return LE_FAULT;
            }
        }
        latitudeTmp += 0x80000000;
        for (i=0;i<4;i++)
        {
            offset = PutBits(offset, 8, (uint8_t*) &ptr[3-i], outDataPtr);
        }
    }

    /* longitude */
    {
        int32_t longitudeTmp = msdDataPtr->msdMsg.msdStruct.vehLocation.longitude;
        uint8_t* ptr = (uint8_t*) &longitudeTmp;
        if((longitudeTmp < -648000000) || (longitudeTmp > 648000000))
        {
            if(longitudeTmp != 0x7FFFFFFF)
            {
                LE_ERROR("Bad longitude value.%d", longitudeTmp);
                return LE_FAULT;
            }
        }
        longitudeTmp += 0x80000000;
        for (i=0;i<4;i++)
        {
            offset = PutBits(offset, 8, (uint8_t*) &ptr[3-i], outDataPtr);
        }
    }

    /* vehDirection */
    if(msdDataPtr->msdMsg.msdStruct.vehDirection > 179)
    {
        if(msdDataPtr->msdMsg.msdStruct.vehDirection != 0xFF)
        {
            LE_ERROR("Bad Vehicle direction.%d (> 179 degrees && != 255)",
                     msdDataPtr->msdMsg.msdStruct.vehDirection);
            return LE_FAULT;
        }
    }
    offset = PutBits(offset, 8 , &msdDataPtr->msdMsg.msdStruct.vehDirection, outDataPtr);

    /* Optional field */
    /* recentVehLocationN1 */
    if (msdDataPtr->msdMsg.msdStruct.recentVehLocationN1Pres)
    {
        int32_t latitudeDeltaTmp, longitudeDeltaTmp;
        /* latitudeDelta */
        latitudeDeltaTmp = msdDataPtr->msdMsg.msdStruct.recentVehLocationN1.latitudeDelta;
        if((latitudeDeltaTmp < ASN1_LATITUDE_DELTA_MIN) ||
           (latitudeDeltaTmp > ASN1_LATITUDE_DELTA_MAX))
        {
            LE_ERROR("Bad latitude delta 1 value.%d", latitudeDeltaTmp);
            return LE_FAULT;
        }
        latitudeDeltaTmp += 512;
        offset = PutTwoBytes(offset, 10, (uint16_t*)&latitudeDeltaTmp, outDataPtr);

        /* longitudeDelta */
        longitudeDeltaTmp = msdDataPtr->msdMsg.msdStruct.recentVehLocationN1.longitudeDelta;
        if((longitudeDeltaTmp < ASN1_LONGITUDE_DELTA_MIN) ||
           (longitudeDeltaTmp > ASN1_LONGITUDE_DELTA_MAX))
        {
            LE_ERROR("Bad longitude delta 1 value.%d", longitudeDeltaTmp);
            return LE_FAULT;
        }
        longitudeDeltaTmp += 512;
        offset = PutTwoBytes(offset, 10, (uint16_t*)&longitudeDeltaTmp, outDataPtr);
    }

    /* recentVehLocationN2 */
    if (msdDataPtr->msdMsg.msdStruct.recentVehLocationN2Pres)
    {
        int32_t latitudeDeltaTmp, longitudeDeltaTmp;
        /* latitudeDelta */
        latitudeDeltaTmp = msdDataPtr->msdMsg.msdStruct.recentVehLocationN2.latitudeDelta;
        if((latitudeDeltaTmp < ASN1_LATITUDE_DELTA_MIN) ||
           (latitudeDeltaTmp > ASN1_LATITUDE_DELTA_MAX))
        {
            LE_ERROR("Bad latitude delta 2 value.%d", latitudeDeltaTmp);
            return LE_FAULT;
        }

        latitudeDeltaTmp += 512;
        offset = PutTwoBytes(offset, 10, (uint16_t*)&latitudeDeltaTmp, outDataPtr);

        /* longitudeDelta */
        longitudeDeltaTmp = msdDataPtr->msdMsg.msdStruct.recentVehLocationN2.longitudeDelta;
        if((longitudeDeltaTmp < ASN1_LONGITUDE_DELTA_MIN) ||
           (longitudeDeltaTmp > ASN1_LONGITUDE_DELTA_MAX))
        {
            LE_ERROR("Bad longitude delta 2 value.%d", longitudeDeltaTmp);
            return LE_FAULT;
        }
        longitudeDeltaTmp += 512;
        offset = PutTwoBytes(offset, 10, (uint16_t*)&longitudeDeltaTmp, outDataPtr);
    }

    /* numberOfPassengers */
    if (msdDataPtr->msdMsg.msdStruct.numberOfPassengersPres)
    {
        offset = PutBits(offset, 8, &msdDataPtr->msdMsg.msdStruct.numberOfPassengers, outDataPtr);
    }

    /* optionalData */
    if (msdDataPtr->msdMsg.optionalDataPres)
    {
        offset = msd_EncodeMsdMessageOptionalData(offset, msdDataPtr, outDataPtr);
    }

    /* Check the offset value, if > 1120 (MSD_MAX_SIZE (140) * 8 */
    if(offset > 1120)
    {
        LE_ERROR("Bad offset value %d bits", offset);
        return LE_FAULT;
    }
    else
    {
        uint16_t msdMsgLen=0;
        /* Convert MSD message length in bits to length in Bytes */
        if(offset%8)
        {
            /* Modulo 8 bits requires a one Byte extension */
            msdMsgLen = (offset/8)+1;
        }
        else
        {
            msdMsgLen = (offset/8);
        }

        LE_DEBUG("MSD length %d Bytes for %d Bits", msdMsgLen, offset);

        if (msdDataPtr->version == 2)
        {
            /* MSD structure size for MSD V2 coding */
            if (msdMsgLen < 2)
            {
                LE_ERROR("Message length should be at least 2 bytes long");
                return LE_FAULT;
            }

            uint8_t msdV2StructLen = msdMsgLen-2;
            LE_DEBUG("MSD version 2: offset %d, MSD struct length %d", offsetV2, msdV2StructLen );

            if(msdV2StructLen < 128)
            {
                offset = PutBits(offsetV2, 8, &msdV2StructLen, outDataV2Ptr);
            }
            else
            {
                uint8_t tmpOutDataV2[140]={0};

                /* Check the offset value, if > 1112 ((140-1) * 8 */
                if(msdMsgLen > sizeof(tmpOutDataV2) - 1)
                {
                    LE_ERROR("Bad offset value %d bits for MSD version 2 long form", offset);
                    return LE_FAULT;
                }

                tmpOutDataV2[0]=0x02;
                tmpOutDataV2[1]=0x80; /* ITU-T X.690 chapter 8.1.3, short form and long form */
                tmpOutDataV2[2]=msdV2StructLen;
                memcpy(&tmpOutDataV2[3], &outDataV2Ptr[2], msdMsgLen - 2);
                memcpy(&outDataV2Ptr[0], &tmpOutDataV2[0], msdMsgLen + 1);
                msdMsgLen = msdMsgLen + 1;
            }
        }

        return msdMsgLen;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function encodes a data buffer from the elements of the ERA Glonass additional data
 * structure.
 *
 * @return the data buffer length in bits
 */
//--------------------------------------------------------------------------------------------------
int32_t msd_EncodeOptionalDataForEraGlonass
(
    msd_EraGlonassData_t* eraGlonassDataPtr,  ///< [IN] ERA Glonass additional data
    uint8_t*              outDataPtr          ///< [OUT] encoded data buffer (allocated by the
                                              ///  calling function must be minimum 140 Bytes)
)
{
    uint8_t off=0;
    int offset=0;
    uint16_t msdMsgLen=0;

    if (outDataPtr)
    {
        /* 12-A1 id Integer  1 byte   M  Version of format of additional data of MSD is set to "1".
         * Subsequent ids must be compatible with earlier ids. */
        /**
         * ERAAdditionalData ::= SEQUENCE {
         * crashSeverity INTEGER(0..2047) OPTIONAL,
         * diagnosticResult DiagnosticResult OPTIONAL,
         * crashInfo CrashInfo OPTIONAL,
         * ...
         * }
         */

        /* Extension bit should PROBABLY be here since there is OPTIONAL parameters in the tree*/
        offset = PutBits(offset, 1, &off, outDataPtr);

        /* Optional Data Presence */
        LE_DEBUG("Present: crashSeverity %d; diagno %d, crashInfo %d ",
              eraGlonassDataPtr->presentCrashSeverity,
              eraGlonassDataPtr->presentDiagnosticResult,
              eraGlonassDataPtr->presentCrashInfo);
        offset = PutBits(offset, 1 ,(uint8_t*)&eraGlonassDataPtr->presentCrashSeverity
                        , outDataPtr);
        offset = PutBits(offset, 1 ,(uint8_t*)&eraGlonassDataPtr->presentDiagnosticResult
                        , outDataPtr);
        offset = PutBits(offset, 1 ,(uint8_t*)&eraGlonassDataPtr->presentCrashInfo
                        , outDataPtr);

        if (eraGlonassDataPtr->msdVersion == 2)
        {
            offset = PutBits(offset, 1
                            , (uint8_t*)&eraGlonassDataPtr->presentCoordinateSystemTypeInfo
                            , outDataPtr);
        }

        if (eraGlonassDataPtr->presentCrashSeverity)
        {
            /* crashSeverity : INTEGER (0..2047) OPTIONAL*/
            /* Fits in 11 bits */
            offset = PutTwoBytes(offset, 11 ,(uint16_t*)&eraGlonassDataPtr->crashSeverity
                                 , outDataPtr);
        }

        if (eraGlonassDataPtr->presentDiagnosticResult)
        {
            /* first presence bits*/
            offset = PutBits(offset, 1
                     , (uint8_t*)&eraGlonassDataPtr->diagnosticResult.presentMicConnectionFailure
                     , outDataPtr);
            offset = PutBits(offset, 1
                     , (uint8_t*)&eraGlonassDataPtr->diagnosticResult.presentMicFailure
                     , outDataPtr);
            offset = PutBits(offset, 1
                     , (uint8_t*)&eraGlonassDataPtr->diagnosticResult.presentRightSpeakerFailure
                     , outDataPtr);
            offset = PutBits(offset, 1
                     , (uint8_t*)&eraGlonassDataPtr->diagnosticResult.presentLeftSpeakerFailure
                     , outDataPtr);
            offset = PutBits(offset, 1
                     , (uint8_t*)&eraGlonassDataPtr->diagnosticResult.presentSpeakersFailure
                     , outDataPtr);
            offset = PutBits(offset, 1
                     , (uint8_t*)&eraGlonassDataPtr->diagnosticResult.presentIgnitionLineFailure
                     , outDataPtr);
            offset = PutBits(offset, 1
                     , (uint8_t*)&eraGlonassDataPtr->diagnosticResult.presentUimFailure
                     , outDataPtr);
            offset = PutBits(offset, 1
                     , (uint8_t*)&eraGlonassDataPtr->diagnosticResult.presentStatusIndicatorFailure
                     , outDataPtr);
            offset = PutBits(offset, 1
                     , (uint8_t*)&eraGlonassDataPtr->diagnosticResult.presentBatteryFailure
                     , outDataPtr);
            offset = PutBits(offset, 1
                     , (uint8_t*)&eraGlonassDataPtr->diagnosticResult.presentBatteryVoltageLow
                     , outDataPtr);
            offset = PutBits(offset, 1
                     , (uint8_t*)&eraGlonassDataPtr->diagnosticResult.presentCrashSensorFailure
                     , outDataPtr);
            offset = PutBits(offset, 1
            , (uint8_t*)&eraGlonassDataPtr->diagnosticResult.presentFirmwareImageCorruption
            , outDataPtr);
            offset = PutBits(offset, 1
            , (uint8_t*)&eraGlonassDataPtr->diagnosticResult.presentCommModuleInterfaceFailure
            , outDataPtr);
            offset = PutBits(offset, 1
                     , (uint8_t*)&eraGlonassDataPtr->diagnosticResult.presentGnssReceiverFailure
                     , outDataPtr);
            offset = PutBits(offset, 1
                     , (uint8_t*)&eraGlonassDataPtr->diagnosticResult.presentRaimProblem
                     , outDataPtr);
            offset = PutBits(offset, 1
                     , (uint8_t*)&eraGlonassDataPtr->diagnosticResult.presentGnssAntennaFailure
                     , outDataPtr);
            offset = PutBits(offset, 1
                     , (uint8_t*)&eraGlonassDataPtr->diagnosticResult.presentCommModuleFailure
                     , outDataPtr);
            offset = PutBits(offset, 1
                     , (uint8_t*)&eraGlonassDataPtr->diagnosticResult.presentEventsMemoryOverflow
                     , outDataPtr);
            offset = PutBits(offset, 1
            , (uint8_t*)&eraGlonassDataPtr->diagnosticResult.presentCrashProfileMemoryOverflow
            , outDataPtr);
            offset = PutBits(offset, 1
                     , (uint8_t*)&eraGlonassDataPtr->diagnosticResult.presentOtherCriticalFailures
                     , outDataPtr);
            offset = PutBits(offset, 1
            , (uint8_t*)&eraGlonassDataPtr->diagnosticResult.presentOtherNotCriticalFailures
            , outDataPtr);

           if (eraGlonassDataPtr->diagnosticResult.presentMicConnectionFailure)
           {
               offset = PutBits(offset, 1
                        , (uint8_t*)&eraGlonassDataPtr->diagnosticResult.micConnectionFailure
                        , outDataPtr);
           }
           if (eraGlonassDataPtr->diagnosticResult.presentMicFailure)
           {
               offset = PutBits(offset, 1
                        , (uint8_t*)&eraGlonassDataPtr->diagnosticResult.micFailure
                        , outDataPtr);
           }
           if (eraGlonassDataPtr->diagnosticResult.presentRightSpeakerFailure)
           {
               offset = PutBits(offset, 1
                        , (uint8_t*)&eraGlonassDataPtr->diagnosticResult.rightSpeakerFailure
                        , outDataPtr);
           }
           if (eraGlonassDataPtr->diagnosticResult.presentLeftSpeakerFailure)
           {
               offset = PutBits(offset, 1
                        , (uint8_t*)&eraGlonassDataPtr->diagnosticResult.leftSpeakerFailure
                        , outDataPtr);
           }
           if (eraGlonassDataPtr->diagnosticResult.presentSpeakersFailure)
           {
               offset = PutBits(offset, 1
                        , (uint8_t*)&eraGlonassDataPtr->diagnosticResult.speakersFailure
                        , outDataPtr);
           }
           if (eraGlonassDataPtr->diagnosticResult.presentIgnitionLineFailure)
           {
               offset = PutBits(offset, 1
                        , (uint8_t*)&eraGlonassDataPtr->diagnosticResult.ignitionLineFailure
                        , outDataPtr);
           }
           if (eraGlonassDataPtr->diagnosticResult.presentUimFailure)
           {
               offset = PutBits(offset, 1
                        , (uint8_t*)&eraGlonassDataPtr->diagnosticResult.uimFailure
                        , outDataPtr);
           }
           if (eraGlonassDataPtr->diagnosticResult.presentStatusIndicatorFailure)
           {
               offset = PutBits(offset, 1
                        , (uint8_t*)&eraGlonassDataPtr->diagnosticResult.statusIndicatorFailure
                        , outDataPtr);
           }
           if (eraGlonassDataPtr->diagnosticResult.presentBatteryFailure)
           {
               offset = PutBits(offset, 1
                        , (uint8_t*)&eraGlonassDataPtr->diagnosticResult.batteryFailure
                        , outDataPtr);
           }
           if (eraGlonassDataPtr->diagnosticResult.presentBatteryVoltageLow)
           {
               offset = PutBits(offset, 1
                        , (uint8_t*)&eraGlonassDataPtr->diagnosticResult.batteryVoltageLow
                        , outDataPtr);
           }
           if (eraGlonassDataPtr->diagnosticResult.presentCrashSensorFailure)
           {
               offset = PutBits(offset, 1
                        , (uint8_t*)&eraGlonassDataPtr->diagnosticResult.crashSensorFailure
                        , outDataPtr);
           }
           if (eraGlonassDataPtr->diagnosticResult.presentFirmwareImageCorruption)
           {
               offset = PutBits(offset, 1
                        , (uint8_t*)&eraGlonassDataPtr->diagnosticResult.firmwareImageCorruption
                        , outDataPtr);
           }
           if (eraGlonassDataPtr->diagnosticResult.presentCommModuleInterfaceFailure)
           {
               offset = PutBits(offset, 1
                        , (uint8_t*)&eraGlonassDataPtr->diagnosticResult.commModuleInterfaceFailure
                        , outDataPtr);
           }
           if( eraGlonassDataPtr->diagnosticResult.presentGnssReceiverFailure)
           {
               offset = PutBits(offset, 1
                        , (uint8_t*)&eraGlonassDataPtr->diagnosticResult.gnssReceiverFailure
                        , outDataPtr);
           }
           if (eraGlonassDataPtr->diagnosticResult.presentRaimProblem)
           {
               offset = PutBits(offset, 1
                        , (uint8_t*)&eraGlonassDataPtr->diagnosticResult.raimProblem
                        , outDataPtr);
           }
           if (eraGlonassDataPtr->diagnosticResult.presentGnssAntennaFailure)
           {
               offset = PutBits(offset, 1
                        , (uint8_t*)&eraGlonassDataPtr->diagnosticResult.gnssAntennaFailure
                        , outDataPtr);
           }
           if (eraGlonassDataPtr->diagnosticResult.presentCommModuleFailure)
           {
               offset = PutBits(offset, 1
                        , (uint8_t*)&eraGlonassDataPtr->diagnosticResult.commModuleFailure
                        , outDataPtr);
           }
           if (eraGlonassDataPtr->diagnosticResult.presentEventsMemoryOverflow)
           {
               offset = PutBits(offset, 1
                        , (uint8_t*)&eraGlonassDataPtr->diagnosticResult.eventsMemoryOverflow
                        , outDataPtr);
           }
           if (eraGlonassDataPtr->diagnosticResult.presentCrashProfileMemoryOverflow)
           {
               offset = PutBits(offset, 1
                        , (uint8_t*)&eraGlonassDataPtr->diagnosticResult.crashProfileMemoryOverflow
                        , outDataPtr);
           }
           if (eraGlonassDataPtr->diagnosticResult.presentOtherCriticalFailures)
           {
               offset = PutBits(offset, 1
                        , (uint8_t*)&eraGlonassDataPtr->diagnosticResult.otherCriticalFailures
                        , outDataPtr);
           }
           if (eraGlonassDataPtr->diagnosticResult.presentOtherNotCriticalFailures)
           {
               offset = PutBits(offset, 1
                        , (uint8_t*)&eraGlonassDataPtr->diagnosticResult.otherNotCriticalFailures
                        , outDataPtr);
           }
        }


        if ( eraGlonassDataPtr->presentCrashInfo )
        {
            /* Presence bits*/
            offset = PutBits(offset, 1
                             , (uint8_t*)&eraGlonassDataPtr->crashType.presentCrashFront
                             , outDataPtr);
            offset = PutBits(offset, 1
                             , (uint8_t*)&eraGlonassDataPtr->crashType.presentCrashLeft
                             , outDataPtr);
            offset = PutBits(offset, 1
                             , (uint8_t*)&eraGlonassDataPtr->crashType.presentCrashRight
                             , outDataPtr);
            offset = PutBits(offset, 1
                             , (uint8_t*)&eraGlonassDataPtr->crashType.presentCrashRear
                             , outDataPtr);
            offset = PutBits(offset, 1
                             , (uint8_t*)&eraGlonassDataPtr->crashType.presentCrashRollover
                             , outDataPtr);
            offset = PutBits(offset, 1
                             , (uint8_t*)&eraGlonassDataPtr->crashType.presentCrashSide
                             , outDataPtr);
            offset = PutBits(offset, 1
                             , (uint8_t*)&eraGlonassDataPtr->crashType.presentCrashFrontOrSide
                             , outDataPtr);
            offset = PutBits(offset, 1
                             , (uint8_t*)&eraGlonassDataPtr->crashType.presentCrashAnotherType
                             , outDataPtr);

           /* Value bits*/
           if (eraGlonassDataPtr->crashType.presentCrashFront)
           {
               offset = PutBits(offset, 1
                                , (uint8_t*)&eraGlonassDataPtr->crashType.crashFront
                                , outDataPtr);
           }
           if (eraGlonassDataPtr->crashType.presentCrashLeft)
           {
               offset = PutBits(offset, 1
                                , (uint8_t*)&eraGlonassDataPtr->crashType.crashLeft
                                , outDataPtr);
           }
           if (eraGlonassDataPtr->crashType.presentCrashRight)
           {
               offset = PutBits(offset, 1
                                , (uint8_t*)&eraGlonassDataPtr->crashType.crashRight
                                , outDataPtr);
           }
           if (eraGlonassDataPtr->crashType.presentCrashRear)
           {
               offset = PutBits(offset, 1
                                , (uint8_t*)&eraGlonassDataPtr->crashType.crashRear
                                , outDataPtr);
           }
           if (eraGlonassDataPtr->crashType.presentCrashRollover)
           {
               offset = PutBits(offset, 1
                                , (uint8_t*)&eraGlonassDataPtr->crashType.crashRollover
                                , outDataPtr);
           }
           if (eraGlonassDataPtr->crashType.presentCrashSide)
           {
               offset = PutBits(offset, 1
                                , (uint8_t*)&eraGlonassDataPtr->crashType.crashSide
                                , outDataPtr);
           }
           if (eraGlonassDataPtr->crashType.presentCrashFrontOrSide)
           {
               offset = PutBits(offset, 1
                                , (uint8_t*)&eraGlonassDataPtr->crashType.crashFrontOrSide
                                , outDataPtr);
           }
           if (eraGlonassDataPtr->crashType.presentCrashAnotherType)
           {
               offset = PutBits(offset, 1
                                , (uint8_t*)&eraGlonassDataPtr->crashType.crashAnotherType
                                , outDataPtr);
           }
        }

        if ((eraGlonassDataPtr->msdVersion == 2) &&
            (eraGlonassDataPtr->presentCoordinateSystemTypeInfo))
        {
            offset = PutBits(offset, 2
                             , (uint8_t*)&eraGlonassDataPtr->coordinateSystemType
                             , outDataPtr);
        }

        /* Convert MSD message length in bits to length in Bytes */
        if (offset%8)
        {
            /* Modulo 8 bits requires a one Byte extension */
            msdMsgLen = (offset/8)+1;
        }
        else
        {
            msdMsgLen = (offset/8);
        }
    }

    LE_DEBUG("MSD Optional Data length %d Bytes for %d Bits", msdMsgLen, offset);

   return msdMsgLen; // number of Bytes.
}

