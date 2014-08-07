/** @file asn1Msd.c
 *
 * Source code of functions to build the MSD needed by eCall.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. Use of this work is subject to license.
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
   uint8_t n;
   uint16_t setBit[16] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80,
                           0x100, 0x200, 0x400, 0x800, 0x1000, 0x2000, 0x4000, 0x8000 };

   for( n = 0; n < elmtLen; n++ )
   {
      if( *(elmtPtr) & setBit[elmtLen-1-n] )
      {
         msgPtr[ (msgOffset>>3) ] |= 0x01<<(7-( msgOffset&0x07 ));
      }
      else
      {
         msgPtr[ (msgOffset>>3) ] &=~( 0x01<<(7-( msgOffset&0x07 )));
      }
      msgOffset++;
   }

   return (msgOffset + elmtLen);
}


//--------------------------------------------------------------------------------------------------
/**
 * This function encodes the MSD message from the elements of the MSD data structure
 *
 * @return the MSD message length in bytes on success
 * @return LE_FAULT on failure
 *
 * @note Only MSD version 1 is supported.
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

    /* MSD Format */
    if(msdDataPtr->version != 1)
    {
        LE_ERROR("MSD version is %d", msdDataPtr->version);
        return LE_FAULT;
    }
    offset = PutBits(offset, 8, &msdDataPtr->version, outDataPtr);

    /* Extension bit */
    offset = PutBits(offset, 1, &off, outDataPtr);

    /* Optional Data Presence */
    offset = PutBits(offset, 1, (uint8_t*)&msdDataPtr->msdMsg.optionalDataPres, outDataPtr);

    /* ** MSD structure ** */
    /* Extension bit */
    offset = PutBits(offset, 1, &off, outDataPtr);

    /* Optional field presence indication */
    offset = PutBits(offset, 1, (uint8_t*)&msdDataPtr->msdMsg.msdStruct.recentVehLocationN1Pres, outDataPtr);
    offset = PutBits(offset, 1, (uint8_t*)&msdDataPtr->msdMsg.msdStruct.recentVehLocationN2Pres, outDataPtr);
    offset = PutBits(offset, 1, (uint8_t*)&msdDataPtr->msdMsg.msdStruct.numberOfPassengersPres, outDataPtr);

    /* Message Identifier */
    offset = PutBits(offset, 8, &msdDataPtr->msdMsg.msdStruct.messageIdentifier, outDataPtr);

    /* Control Type */
    offset = PutBits(offset, 1, (uint8_t*)&msdDataPtr->msdMsg.msdStruct.control.automaticActivation, outDataPtr);
    offset = PutBits(offset, 1, (uint8_t*)&msdDataPtr->msdMsg.msdStruct.control.testCall, outDataPtr);
    offset = PutBits(offset, 1, (uint8_t*)&msdDataPtr->msdMsg.msdStruct.control.positionCanBeTrusted, outDataPtr);
    /* vehType : Only enumerated values are supported : no extension*/
    offset = PutBits(offset, 1, &off, outDataPtr); /* extension bit */
    {
        uint8_t tmp = msdDataPtr->msdMsg.msdStruct.control.vehType;
        tmp -= 1;

        offset = PutBits(offset, 4, &tmp , outDataPtr);
    }

    /* Vehicle identification Number */
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
        uint8_t* ptr = (uint8_t*) msdDataPtr->msdMsg.msdStruct.vehIdentificationNumber.isovisModelyear;
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
        uint8_t* ptr = (uint8_t*) msdDataPtr->msdMsg.msdStruct.vehIdentificationNumber.isovisSeqPlant;
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

    offset = PutBits(offset, 1, (uint8_t*)&msdDataPtr->msdMsg.msdStruct.vehPropulsionStorageType.gasolineTankPresent, outDataPtr);
    offset = PutBits(offset, 1, (uint8_t*)&msdDataPtr->msdMsg.msdStruct.vehPropulsionStorageType.dieselTankPresent, outDataPtr);
    offset = PutBits(offset, 1, (uint8_t*)&msdDataPtr->msdMsg.msdStruct.vehPropulsionStorageType.compressedNaturalGas, outDataPtr);
    offset = PutBits(offset, 1, (uint8_t*)&msdDataPtr->msdMsg.msdStruct.vehPropulsionStorageType.liquidPropaneGas, outDataPtr);
    offset = PutBits(offset, 1, (uint8_t*)&msdDataPtr->msdMsg.msdStruct.vehPropulsionStorageType.electricEnergyStorage, outDataPtr);
    offset = PutBits(offset, 1, (uint8_t*)&msdDataPtr->msdMsg.msdStruct.vehPropulsionStorageType.hydrogenStorage, outDataPtr);
    if ( msdDataPtr->msdMsg.msdStruct.vehPropulsionStorageType.gasolineTankPresent )
    {
        offset = PutBits(offset, 1, (uint8_t*)&msdDataPtr->msdMsg.msdStruct.vehPropulsionStorageType.gasolineTankPresent, outDataPtr);
    }
    if ( msdDataPtr->msdMsg.msdStruct.vehPropulsionStorageType.dieselTankPresent )
    {
        offset = PutBits(offset, 1, (uint8_t*)&msdDataPtr->msdMsg.msdStruct.vehPropulsionStorageType.dieselTankPresent, outDataPtr);
    }
    if ( msdDataPtr->msdMsg.msdStruct.vehPropulsionStorageType.compressedNaturalGas )
    {
        offset = PutBits(offset, 1, (uint8_t*)&msdDataPtr->msdMsg.msdStruct.vehPropulsionStorageType.compressedNaturalGas, outDataPtr);
    }
    if ( msdDataPtr->msdMsg.msdStruct.vehPropulsionStorageType.liquidPropaneGas )
    {
        offset = PutBits(offset, 1, (uint8_t*)&msdDataPtr->msdMsg.msdStruct.vehPropulsionStorageType.liquidPropaneGas, outDataPtr);
    }
    if ( msdDataPtr->msdMsg.msdStruct.vehPropulsionStorageType.electricEnergyStorage )
    {
        offset = PutBits(offset, 1, (uint8_t*)&msdDataPtr->msdMsg.msdStruct.vehPropulsionStorageType.electricEnergyStorage, outDataPtr);
    }
    if ( msdDataPtr->msdMsg.msdStruct.vehPropulsionStorageType.hydrogenStorage )
    {
        offset = PutBits(offset, 1, (uint8_t*)&msdDataPtr->msdMsg.msdStruct.vehPropulsionStorageType.hydrogenStorage, outDataPtr);
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
                     msdDataPtr->msdMsg.msdStruct.vehDirection)
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
        if((latitudeDeltaTmp < -512) || (latitudeDeltaTmp > 511))
        {
            LE_ERROR("Bad latitude delta 1 value.%d", latitudeDeltaTmp);
            return LE_FAULT;
        }
        latitudeDeltaTmp += 512;
        offset = PutTwoBytes(offset, 10, (uint16_t*)&latitudeDeltaTmp, outDataPtr);

        /* longitudeDelta */
        longitudeDeltaTmp = msdDataPtr->msdMsg.msdStruct.recentVehLocationN1.longitudeDelta;
        if((longitudeDeltaTmp < -512) || (longitudeDeltaTmp > 511))
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
        if((latitudeDeltaTmp < -512) || (latitudeDeltaTmp > 511))
        {
            LE_ERROR("Bad latitude delta 2 value.%d", latitudeDeltaTmp);
            return LE_FAULT;
        }

        latitudeDeltaTmp += 512;
        offset = PutTwoBytes(offset, 10, (uint16_t*)&latitudeDeltaTmp, outDataPtr);

        /* longitudeDelta */
        longitudeDeltaTmp = msdDataPtr->msdMsg.msdStruct.recentVehLocationN2.longitudeDelta;
        if((longitudeDeltaTmp < -512) || (longitudeDeltaTmp > 511))
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
        uint8_t length = msdDataPtr->msdMsg.optionalData.oidlen;
        uint8_t* ptr= (uint8_t*) msdDataPtr->msdMsg.optionalData.data;
        /* OID */
        offset = PutBits(offset, 8, &length, outDataPtr);
        for(i=0; i<length; i++)
        {
            offset = PutBits(offset, 8, &msdDataPtr->msdMsg.optionalData.oid[i], outDataPtr);
        }

        /* data */
        length = msdDataPtr->msdMsg.optionalData.dataLen;
        offset = PutBits(offset, 8, &length, outDataPtr);
        for(i=0; i<length; i++)
        {
            offset = PutBits(offset, 8, &ptr[i], outDataPtr);
        }
    }

    /* Check the offset value, if > 1120 (MSD_MAX_SIZE (140) * 8 */
    if(offset > 1120)
    {
        LE_ERROR("Bad offset value.%d bits", offset);
        return LE_FAULT;
    }
    else
    {
        return (offset/8);
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
    uint8_t  dataLen=0;
    uint8_t  id=1;
    uint8_t off=0;
    int offset=0;

    if(outDataPtr)
    {
        // TODO: REMAINS TO BE BETTER EXPLAINED by glonassunion
        // "In the meantime it is set to 192 just for test purposes."

        /* 12-A0 OID Integer  1 byte   M  Identifier of additional data block, containing parameters,
         * introduced additionally for the "ERA-GLONASS" system set to value 11000000(bin) = 192(dec)*/
        //p_OID[oidlen++] = 192;

        /* 12-A1 id Integer  1 byte   M  Version of format of additional data of MSD is set to "1".
         * Subsequent ids must be compatible with earlier ids. */
        offset = PutBits(offset, 8, &id , outDataPtr);

        /* Extension bit should PROBABLY be here since there is OPTIONAL parameters in the tree*/
        offset = PutBits(offset, 1, &off, outDataPtr);

        /* Optional Data Presence */

        /* We do not support <mobileDef> BUT put presence bit to 0 (false) */
        offset = PutBits(offset, 1,0, outDataPtr);

        offset = PutBits(offset, 1, (uint8_t*)&eraGlonassDataPtr->testResultsDefPres, outDataPtr);
        offset = PutBits(offset, 1, (uint8_t*)&eraGlonassDataPtr->crashTypePres, outDataPtr);

        offset = PutBits(offset, 1, (uint8_t*)&eraGlonassDataPtr->SevereCrashEstimation, outDataPtr);

        if ( eraGlonassDataPtr->testResultsDefPres )
        {
            offset = PutBits(offset, 1, (uint8_t*)&eraGlonassDataPtr->testResultsDef.micConnectionFailure, outDataPtr);
            offset = PutBits(offset, 1, (uint8_t*)&eraGlonassDataPtr->testResultsDef.micFailure, outDataPtr);
            offset = PutBits(offset, 1, (uint8_t*)&eraGlonassDataPtr->testResultsDef.rightSpeakerFailure, outDataPtr);
            offset = PutBits(offset, 1, (uint8_t*)&eraGlonassDataPtr->testResultsDef.leftSpeakerFailure, outDataPtr);
            offset = PutBits(offset, 1, (uint8_t*)&eraGlonassDataPtr->testResultsDef.speakersFailure, outDataPtr);
            offset = PutBits(offset, 1, (uint8_t*)&eraGlonassDataPtr->testResultsDef.ignitionLineFailure, outDataPtr);
            offset = PutBits(offset, 1, (uint8_t*)&eraGlonassDataPtr->testResultsDef.uimFailure, outDataPtr);
            offset = PutBits(offset, 1, (uint8_t*)&eraGlonassDataPtr->testResultsDef.statusIndicatorFailure, outDataPtr);
            offset = PutBits(offset, 1, (uint8_t*)&eraGlonassDataPtr->testResultsDef.batteryFailure, outDataPtr);
            offset = PutBits(offset, 1, (uint8_t*)&eraGlonassDataPtr->testResultsDef.batteryVoltageLow, outDataPtr);
            offset = PutBits(offset, 1, (uint8_t*)&eraGlonassDataPtr->testResultsDef.crashSensorFailure, outDataPtr);
            offset = PutBits(offset, 1, (uint8_t*)&eraGlonassDataPtr->testResultsDef.swImageCorruption, outDataPtr);
            offset = PutBits(offset, 1, (uint8_t*)&eraGlonassDataPtr->testResultsDef.commModuleInterfaceFailure, outDataPtr);
            offset = PutBits(offset, 1, (uint8_t*)&eraGlonassDataPtr->testResultsDef.gnssReceiverFailure, outDataPtr);
            offset = PutBits(offset, 1, (uint8_t*)&eraGlonassDataPtr->testResultsDef.raimProblem, outDataPtr);
            offset = PutBits(offset, 1, (uint8_t*)&eraGlonassDataPtr->testResultsDef.gnssAntennaFailure, outDataPtr);
            offset = PutBits(offset, 1, (uint8_t*)&eraGlonassDataPtr->testResultsDef.commModuleFailure, outDataPtr);
            offset = PutBits(offset, 1, (uint8_t*)&eraGlonassDataPtr->testResultsDef.eventsMemoryOverflow, outDataPtr);
            offset = PutBits(offset, 1, (uint8_t*)&eraGlonassDataPtr->testResultsDef.crashProfileMemoryOverflow, outDataPtr);
            offset = PutBits(offset, 1, (uint8_t*)&eraGlonassDataPtr->testResultsDef.otherCriticalFailires, outDataPtr);
            offset = PutBits(offset, 1, (uint8_t*)&eraGlonassDataPtr->testResultsDef.otherNotCriticalFailures, outDataPtr);
        }

        if ( eraGlonassDataPtr->crashTypePres )
        {
            offset = PutBits(offset, 1, (uint8_t*)&eraGlonassDataPtr->crashType.crashFront, outDataPtr);
            offset = PutBits(offset, 1, (uint8_t*)&eraGlonassDataPtr->crashType.crashSide, outDataPtr);
            offset = PutBits(offset, 1, (uint8_t*)&eraGlonassDataPtr->crashType.crashFrontOrSide, outDataPtr);
            offset = PutBits(offset, 1, (uint8_t*)&eraGlonassDataPtr->crashType.crashRear, outDataPtr);
            offset = PutBits(offset, 1, (uint8_t*)&eraGlonassDataPtr->crashType.crashRollover, outDataPtr);
            offset = PutBits(offset, 1, (uint8_t*)&eraGlonassDataPtr->crashType.crashAnotherType, outDataPtr);
        }

        LE_DEBUG("ans1_EraGlonass_EncodeoptionalData dataLen %d. Offset %d", dataLen, offset);
   }
   return offset; // nbr of bits.
}

