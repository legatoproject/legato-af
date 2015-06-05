/**
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */


/** @file hex.c
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
#include "legato.h"

static const int8_t hextable[] =
{
    [0 ... '0'-1] = -1,
    ['0'] = 0 , 1, 2, 3, 4, 5, 6, 7, 8, 9,
    ['0'+10 ... 'A'-1] = -1,
    ['A'] = 10, 11, 12, 13, 14, 15,
    ['A'+6 ... 'a'-1] = -1,
    ['a'] = 10, 11, 12, 13, 14, 15,
    ['a'+6 ... 255] = -1,
};

//--------------------------------------------------------------------------------------------------
/**
 * This function convert "0-F" char into uint8_t value
 *
 * @return
 *      \return return the value or -1 if not possible.
 */
//--------------------------------------------------------------------------------------------------
static int8_t HexToDec(unsigned const char* hex)
{
    int8_t ret = 0;

    while(*hex && ret>=0)
    {
        ret = ((ret<<4) | hextable[*hex++]);
    }

    return ret;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function convert the hexadecimal value
 * Should be between 0 and 16.
 *
 * @return
 *      \return the character code if exist, 0 instead.
 */
//--------------------------------------------------------------------------------------------------
static char DecToHex(uint8_t hex)
{
    if ( hex < 10) {
        return (char)('0'+hex);  // for number
    }
    else if (hex < 16) {
        return (char)('A'+hex-10);  // for A,B,C,D,E,F
    }
    else {
        LE_DEBUG("value %u cannot be converted in HEX string", hex);
        return 0;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function converts hex string to binary format.
 *
 * @return size of binary, if < 0 it has failed
 */
//--------------------------------------------------------------------------------------------------
int32_t le_hex_StringToBinary
(
    const char *stringPtr,     ///< [IN] string to convert, terminated with '\0'.
    uint32_t    stringLength,  ///< [IN] string length
    uint8_t    *binaryPtr,     ///< [OUT] binary result
    uint32_t    binarySize     ///< [IN] size of the binary table
)
{
    uint32_t idxString,idxBinary;

    if (2*binarySize+1 < stringLength)
    {
        LE_DEBUG("binary array (%u) is too small (%u)",binarySize, stringLength);
        return -1;
    }

    for(idxString=0,idxBinary=0;
        idxString<stringLength;
        idxString=idxString+2,idxBinary++)
    {
        char tmp[3];

        tmp[0] = stringPtr[idxString];
        tmp[1] = stringPtr[idxString+1];
        tmp[2] = '\0';

        binaryPtr[idxBinary] = HexToDec((unsigned const char*)tmp);
    }

    return idxBinary;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function converts binary to hex string format.
 *
 * @return size of hex string, if < 0 it has failed
 */
//--------------------------------------------------------------------------------------------------
int32_t le_hex_BinaryToString
(
    const uint8_t *binaryPtr,  ///< [IN] binary array to convert
    uint32_t       binarySize, ///< [IN] size of binary array
    char          *stringPtr,  ///< [OUT] hex string array, terminated with '\0'.
    uint32_t       stringSize  ///< [IN] size of string array
)
{
    int32_t idxString,idxBinary;

    if (stringSize < 2*binarySize+1)
    {
        LE_DEBUG("Hex string array (%u) is too small (%u)",stringSize, binarySize);
        return -1;
    }

    for(idxBinary=0,idxString=0;
        idxBinary<binarySize;
        idxBinary++,idxString=idxString+2)
    {
        stringPtr[idxString]   = DecToHex( (binaryPtr[idxBinary]>>4) & 0x0F);
        stringPtr[idxString+1] = DecToHex(  binaryPtr[idxBinary]     & 0x0F);
    }
    stringPtr[idxString] = '\0';

    return idxString;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function convert hexa string into integer
 *
 * @return
 *      \return return the value or -1 if not possible.
 */
//--------------------------------------------------------------------------------------------------
int le_hex_HexaToInteger(char s[])
{
  int   i;
  int n=0;

  if(s == NULL)
  {
    return (-1);
  }

  for(i=0; s[i]!= '\0'; i++)
  {
    if(s[i] <= '9' && s[i] >= '0' )
    {
      n=n*16+s[i]- '0';
    }
    else if (s[i]>= 'a' && s[i]<= 'f')
    {
      n=n*16+ (int)s[i] - 'a' + 10;
    }
    else if (s[i]>= 'A' && s[i]<= 'F')
    {
      n=n*16+ (int)s[i] - 'A' + 10;
    }
    else
    {
      return (-1);
    }
  }
  return (n);
}
