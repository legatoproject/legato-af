
/**
 * @file cbor.c
 *
 * Implementation of CBOR encoding and decoding interface for Legato.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include <math.h>

//--------------------------------------------------------------------------------------------------
/**
 * Macros for cbor encoding/decoding
 */
//--------------------------------------------------------------------------------------------------

// CBOR major type
#define _LE_CBOR_POS_INTEGER        0
#define _LE_CBOR_NEG_INTEGER        1
#define _LE_CBOR_BYTE_STRING        2
#define _LE_CBOR_TEXT_STRING        3
#define _LE_CBOR_ITEM_ARRAY         4
#define _LE_CBOR_PAIR_MAP           5
#define _LE_CBOR_TAG                6
#define _LE_CBOR_PRIMITVE           7

#define _LE_CBOR_COMPLEX_THRESHOLD      24

// CBOR short count
#define _LE_CBOR_PRIMITIVE_FALSE        20
#define _LE_CBOR_PRIMITIVE_TRUE         21
#define _LE_CBOR_PRIMITIVE_NULL         22
#define _LE_CBOR_PRIMITIVE_FLOAT        26
#define _LE_CBOR_PRIMITIVE_DOUBLE       27
#define _LE_CBOR_PRIMITIVE_BREAK        31
#define _LE_CBOR_PRIMITIVE_INDEFINITE   31


// Write bytes
#define LE_CBOR_PACK_SIMPLE_BUFFER(desPtr, length)              \
    do {                                                        \
        memcpy(*bufferPtr, desPtr, length);                     \
        *bufferPtr += length;                                   \
    } while(0)


// Pack/encode tiny field
#define LE_CBOR_PACK_TINY_ITEM(major, additional)               \
    do {                                                        \
        *(*bufferPtr) = ((major & 0x7) << 5) | (additional);    \
        *bufferPtr += 1;                                        \
    } while(0)


// Unpack/decode tiny field
#define LE_CBOR_UNPACK_TINY_ITEM()                              \
    do {                                                        \
        uint8_t* buffer = *bufferPtr;                           \
        major = (buffer[0] >> 5) & 0x7;                         \
        additional = buffer[0] & 0x1F;                          \
        *bufferPtr += 1;                                        \
    } while(0)


// Read bytes
#define LE_CBOR_UNPACK_SIMPLE_BUFFER(srcPtr, length)            \
    do {                                                        \
        memcpy((void*)(srcPtr), *bufferPtr, length);            \
        *bufferPtr += length;                                   \
    } while(0)


//--------------------------------------------------------------------------------------------------
/**
 * Decode a half float from buffer, increment the buffer pointer if encoding is successful
 *
 * @return
 *      - true      if successfully encoded
 *      - false     otherwise
 * @note
 *      - As per http://tools.ietf.org/html/rfc7049#appendix-D
 */
//--------------------------------------------------------------------------------------------------
static bool DecodeHalfFloat
(
    uint8_t** bufferPtr,            ///< [IN/OUT] Pointer of buffer for decoding
    float* valuePtr                 ///< [OUT] Decoded value
)
{
    if (bufferPtr == NULL || *bufferPtr == NULL || valuePtr == NULL || **bufferPtr != 0xF9)
    {
        return false;
    }

    *bufferPtr += 1;
    uint8_t *halfp = *bufferPtr;

    int half = (halfp[0] << 8) + halfp[1];
    int exp = (half >> 10) & 0x1f;
    int mant = half & 0x3ff;
    double val;
    if (exp == 0)
    {
        val = ldexp(mant, -24);
    }
    else if (exp != 31)
    {
        val = ldexp(mant + 1024, exp - 25);
    }
    else
    {
        val = mant == 0 ? INFINITY : NAN;
    }
    *valuePtr = (float)(half & 0x8000 ? -val : val);

    *bufferPtr += 2;
    return true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Encode an integer into a buffer, increment the buffer pointer if encoding is successful
 *
 * @return
 *      - true      if successfully encoded
 *      - false     otherwise
 */
//--------------------------------------------------------------------------------------------------
static bool EncodeInteger
(
    uint8_t **bufferPtr,            ///< [OUT] Pointer of buffer to store the encoded data
    size_t* bufLen,                 ///< [IN/OUT] Size of the buffer available
    uint64_t value,                 ///< [IN] Value to be encoded
    unsigned int major              ///< [IN] CBOR major type
)
{
    if (bufferPtr == NULL || *bufferPtr == NULL || bufLen == NULL)
    {
        return false;
    }

    if (value <= UINT16_MAX)
    {
        if (value <= UINT8_MAX)
        {
            if (value < _LE_CBOR_COMPLEX_THRESHOLD)
            {
                if (*bufLen < 1)
                {
                    return false;
                }

                LE_CBOR_PACK_TINY_ITEM(major, value);
                *bufLen -= 1;

                return true;
            }
            else
            {
                if (*bufLen < LE_CBOR_UINT8_MAX_SIZE)
                {
                    return false;
                }

                uint8_t value2 = (uint8_t) value;
                LE_CBOR_PACK_TINY_ITEM(major, _LE_CBOR_COMPLEX_THRESHOLD);
                LE_CBOR_PACK_SIMPLE_BUFFER(&value2, 1);
                *bufLen -= LE_CBOR_UINT8_MAX_SIZE;

                return true;
            }
        }
        else
        {
            if (*bufLen < LE_CBOR_UINT16_MAX_SIZE)
            {
                return false;
            }

            uint16_t value2 = htobe16((uint16_t)value);
            LE_CBOR_PACK_TINY_ITEM(major, _LE_CBOR_COMPLEX_THRESHOLD + 1);
            LE_CBOR_PACK_SIMPLE_BUFFER(&value2, 2);
            *bufLen -= LE_CBOR_UINT16_MAX_SIZE;

            return true;
        }
    }
    else
    {
        if (value <= UINT32_MAX)
        {
            if (*bufLen < LE_CBOR_UINT32_MAX_SIZE)
            {
                return false;
            }

            uint32_t value2 = htobe32((uint32_t)value);
            LE_CBOR_PACK_TINY_ITEM(major, _LE_CBOR_COMPLEX_THRESHOLD + 2);
            LE_CBOR_PACK_SIMPLE_BUFFER(&value2, 4);
            *bufLen -= LE_CBOR_UINT32_MAX_SIZE;

            return true;
        }
        else
        {
            if (*bufLen < LE_CBOR_UINT64_MAX_SIZE)
            {
                return false;
            }

            uint64_t value2 = htobe64((uint64_t)value);
            LE_CBOR_PACK_TINY_ITEM(major, _LE_CBOR_COMPLEX_THRESHOLD + 3);
            LE_CBOR_PACK_SIMPLE_BUFFER(&value2, 8);
            *bufLen -= LE_CBOR_UINT64_MAX_SIZE;

            return true;
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Decode a positive integer from a buffer, increment the buffer pointer if decoding is successful
 *
 * @return
 *      - true      if successfully decoded
 *      - false     failed to decode
 */
//--------------------------------------------------------------------------------------------------
static bool DecodePositiveInteger
(
    uint8_t** bufferPtr,            ///< [IN/OUT] Pointer of buffer for decoding
    uint64_t* valuePtr,             ///< [OUT] Decoded value
    unsigned int expectedMajor      ///< [IN] Expected CBOR major type
)
{
    unsigned int additional;
    unsigned int major;

    if (bufferPtr == NULL || *bufferPtr == NULL || valuePtr == NULL)
    {
        return false;
    }

    LE_CBOR_UNPACK_TINY_ITEM();
    if (major != expectedMajor)
    {
        // Restore the pointer position
        *bufferPtr -= 1;
        return false;
    }
    else
    {
        if (additional < _LE_CBOR_COMPLEX_THRESHOLD)
        {
            *valuePtr = additional;
        }
        else
        {
            int length = (additional == _LE_CBOR_COMPLEX_THRESHOLD)?1:
                2 << (additional-_LE_CBOR_COMPLEX_THRESHOLD-1);
            LE_CBOR_UNPACK_SIMPLE_BUFFER(((uint8_t*)valuePtr) + sizeof(uint64_t)-length, length);
            *valuePtr = be64toh(*valuePtr);
        }
        return true;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Encode a tag ID into a buffer, increment the buffer pointer if encoding is successful
 *
 * @return
 *      - true      if successfully encoded
 *      - false     otherwise
 */
//--------------------------------------------------------------------------------------------------
bool le_cbor_EncodeTag
(
    uint8_t** bufferPtr,            ///< [OUT] Pointer of buffer to store the encoded data
    size_t* bufLen,                 ///< [IN/OUT] Size of the buffer available
    le_cbor_Tag_t value             ///< [IN] Tag to be encoded
)
{
    return EncodeInteger(bufferPtr, bufLen, value, _LE_CBOR_TAG);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get CBOR data type
 *
 * @return
 *      - an le_cbor_Type_t enum
 */
//--------------------------------------------------------------------------------------------------
le_cbor_Type_t le_cbor_GetType
(
    const uint8_t* buffer,          ///< [IN] buffer pointer
    ssize_t* additionalBytes        ///< [OUT] number of additional bytes
)
{
    if (buffer == NULL || additionalBytes == NULL)
    {
        return LE_CBOR_TYPE_INVALID_TYPE;
    }

    unsigned int major = (buffer[0] >> 5) & 0x7;
    unsigned int additional = buffer[0] & 0x1F;

    if (additional < _LE_CBOR_COMPLEX_THRESHOLD)
    {
        *additionalBytes = 0;
    }
    else if (additional < _LE_CBOR_PRIMITIVE_INDEFINITE)
    {
        *additionalBytes = (additional == _LE_CBOR_COMPLEX_THRESHOLD)?1:
            2 << (additional-_LE_CBOR_COMPLEX_THRESHOLD-1);
    }
    else
    {
        *additionalBytes = -1;
    }

    le_cbor_Type_t ret;

    switch(major)
    {
        case (_LE_CBOR_POS_INTEGER):  ret = LE_CBOR_TYPE_POS_INTEGER;  break;
        case (_LE_CBOR_NEG_INTEGER):  ret = LE_CBOR_TYPE_NEG_INTEGER;  break;
        case (_LE_CBOR_BYTE_STRING):  ret = LE_CBOR_TYPE_BYTE_STRING;  break;
        case (_LE_CBOR_TEXT_STRING):  ret = LE_CBOR_TYPE_TEXT_STRING;  break;
        case (_LE_CBOR_ITEM_ARRAY):   ret = LE_CBOR_TYPE_ITEM_ARRAY;   break;
        case (_LE_CBOR_TAG):          ret = LE_CBOR_TYPE_TAG;          break;
        case (_LE_CBOR_PRIMITVE):
        {
            if (additional == _LE_CBOR_PRIMITIVE_TRUE ||
                additional == _LE_CBOR_PRIMITIVE_FALSE)
            {
                ret = LE_CBOR_TYPE_BOOLEAN;
            }
            else if (additional == _LE_CBOR_PRIMITIVE_DOUBLE)
            {
                ret = LE_CBOR_TYPE_DOUBLE;
            }
            else if (additional == _LE_CBOR_PRIMITIVE_BREAK)
            {
                ret = LE_CBOR_TYPE_INDEF_END;
            }
            else if (additional == _LE_CBOR_PRIMITIVE_NULL)
            {
                ret = LE_CBOR_TYPE_NULL;
            }
            else
            {
                ret = LE_CBOR_TYPE_INVALID_TYPE;
            }
            break;
        }
        default:
            ret = LE_CBOR_TYPE_INVALID_TYPE;
    }

    return ret;
}


//--------------------------------------------------------------------------------------------------
/**
 * Encode a negative integer into a buffer, increment the buffer pointer if encoding is successful
 *
 * This function takes uint64_t type value, which shall be a 2's compliment of the to-be-encoded
 * negative number. That means, to encode a negative number x, call this function with value:
 * (-1 - x), e.g., to encode -5, provide (-1 - 5). Another approach to encode a negative value is
 * directly call le_cbor_EncodeInteger() which handles the conversion internally.
 *
 * @return
 *      - true      if successfully encoded
 *      - false     otherwise
 */
//--------------------------------------------------------------------------------------------------
bool le_cbor_EncodeNegativeInteger
(
    uint8_t **bufferPtr,            ///< [OUT] Pointer of buffer to store the encoded data
    size_t* bufLen,                 ///< [IN/OUT] Size of the buffer available
    uint64_t value                  ///< [IN] Value to be encoded, this value is uint64_t type, and
                                    ///       must be the 2's compliment of the to-be-encoded
                                    ///       negative number
)
{
    return EncodeInteger(bufferPtr, bufLen, value, _LE_CBOR_NEG_INTEGER);
}


//--------------------------------------------------------------------------------------------------
/**
 * Encode a positive integer into a buffer, increment the buffer pointer if encoding is successful
 *
 * @return
 *      - true      if successfully encoded
 *      - false     otherwise
 */
//--------------------------------------------------------------------------------------------------
bool le_cbor_EncodePositiveInteger
(
    uint8_t **bufferPtr,            ///< [OUT] Pointer of buffer to store the encoded data
    size_t* bufLen,                 ///< [IN/OUT] Size of the buffer available
    uint64_t value                  ///< [IN] Value to be encoded
)
{
    return EncodeInteger(bufferPtr, bufLen, value, _LE_CBOR_POS_INTEGER);
}


//--------------------------------------------------------------------------------------------------
/**
 * Encode an integer into a buffer, increment the buffer pointer if encoding is successful
 * the integer to be encouded
 *
 * @return
 *      - true      if successfully encoded
 *      - false     otherwise
 */
//--------------------------------------------------------------------------------------------------
bool le_cbor_EncodeInteger
(
    uint8_t **bufferPtr,            ///< [OUT] Pointer of buffer to store the encoded data
    size_t* bufLen,                 ///< [IN/OUT] Size of the buffer available
    int64_t value                   ///< [IN] Value to be encoded
)
{
    return le_cbor_EncodeInt64(bufferPtr, bufLen, value);
}


//--------------------------------------------------------------------------------------------------
/**
 * Encode a int8_t value into a buffer, increment the buffer pointer if encoding is successful
 *
 * @return
 *      - true      if successfully encoded
 *      - false     otherwise
 */
//--------------------------------------------------------------------------------------------------
bool le_cbor_EncodeInt8
(
    uint8_t **bufferPtr,            ///< [OUT] Pointer of buffer to store the encoded data
    size_t* bufLen,                 ///< [IN/OUT] Size of the buffer available
    int8_t value                    ///< [IN] Value to be encoded
)
{
    if (value >= 0)
    {
        return le_cbor_EncodePositiveInteger(bufferPtr, bufLen, value);
    }

    return le_cbor_EncodeNegativeInteger(bufferPtr, bufLen, -1 - value);
}


//--------------------------------------------------------------------------------------------------
/**
 * Encode a int16_t value into a buffer, increment the buffer pointer if encoding is successful
 *
 * @return
 *      - true      if successfully encoded
 *      - false     otherwise
 */
//--------------------------------------------------------------------------------------------------
bool le_cbor_EncodeInt16
(
    uint8_t **bufferPtr,            ///< [OUT] Pointer of buffer to store the encoded data
    size_t* bufLen,                 ///< [IN/OUT] Size of the buffer available
    int16_t value                   ///< [IN] Value to be encoded
)
{
    if (value >= 0)
    {
        return le_cbor_EncodePositiveInteger(bufferPtr, bufLen, value);
    }

    return le_cbor_EncodeNegativeInteger(bufferPtr, bufLen, -1 - value);
}


//--------------------------------------------------------------------------------------------------
/**
 * Encode a int32_t value into a buffer, increment the buffer pointer if encoding is successful
 *
 * @return
 *      - true      if successfully encoded
 *      - false     otherwise
 */
//--------------------------------------------------------------------------------------------------
bool le_cbor_EncodeInt32
(
    uint8_t **bufferPtr,            ///< [OUT] Pointer of buffer to store the encoded data
    size_t* bufLen,                 ///< [IN/OUT] Size of the buffer available
    int32_t value                   ///< [IN] Value to be encoded
)
{
    if (value >= 0)
    {
        return le_cbor_EncodePositiveInteger(bufferPtr, bufLen, value);
    }

    return le_cbor_EncodeNegativeInteger(bufferPtr, bufLen, -1 - value);
}


//--------------------------------------------------------------------------------------------------
/**
 * Encode a int64_t value into a buffer, increment the buffer pointer if encoding is successful
 *
 * @return
 *      - true      if successfully encoded
 *      - false     otherwise
 */
//--------------------------------------------------------------------------------------------------
bool le_cbor_EncodeInt64
(
    uint8_t **bufferPtr,            ///< [OUT] Pointer of buffer to store the encoded data
    size_t* bufLen,                 ///< [IN/OUT] Size of the buffer available
    int64_t value                   ///< [IN] Value to be encoded
)
{
    if (value >= 0)
    {
        return le_cbor_EncodePositiveInteger(bufferPtr, bufLen, value);
    }

    return le_cbor_EncodeNegativeInteger(bufferPtr, bufLen, -1 - value);
}


//--------------------------------------------------------------------------------------------------
/**
 * Encode a boolean value into a buffer, increment the buffer pointer if encoding is successful
 *
 * @return
 *      - true      if successfully encoded
 *      - false     otherwise
 */
//--------------------------------------------------------------------------------------------------
bool le_cbor_EncodeBool
(
    uint8_t **bufferPtr,            ///< [OUT] Pointer of buffer to store the encoded data
    size_t* bufLen,                 ///< [IN/OUT] Size of the buffer available
    bool value                      ///< [IN] Value to be encoded
)
{
    if (bufferPtr == NULL || *bufferPtr == NULL ||
        bufLen == NULL || *bufLen < LE_CBOR_BOOL_MAX_SIZE)
    {
        return false;
    }

    LE_CBOR_PACK_TINY_ITEM(_LE_CBOR_PRIMITVE, _LE_CBOR_PRIMITIVE_FALSE + (value ? 1 : 0));

    *bufLen -= LE_CBOR_BOOL_MAX_SIZE;

    return true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Encode a double value into a buffer, increment the buffer pointer if encoding is successful
 *
 * @return
 *      - true      if successfully encoded
 *      - false     otherwise
 */
//--------------------------------------------------------------------------------------------------
bool le_cbor_EncodeDouble
(
    uint8_t **bufferPtr,            ///< [OUT] Pointer of buffer to store the encoded data
    size_t* bufLen,                 ///< [IN/OUT] Size of the buffer available
    double value                    ///< [IN] Value to be encoded
)
{
    uint64_t    intValue;
    if (bufferPtr == NULL || *bufferPtr == NULL ||
        bufLen == NULL || *bufLen < LE_CBOR_DOUBLE_MAX_SIZE)
    {
        return false;
    }

    memcpy(&intValue, &value, sizeof(intValue));
    intValue = htobe64(intValue);
    LE_CBOR_PACK_TINY_ITEM(_LE_CBOR_PRIMITVE, _LE_CBOR_PRIMITIVE_DOUBLE);
    LE_CBOR_PACK_SIMPLE_BUFFER(&intValue, sizeof(intValue));

    *bufLen -= LE_CBOR_DOUBLE_MAX_SIZE;

    return true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Encode a string into a buffer, increment the buffer pointer if encoding is successful
 *
 * @return
 *      - true      if successfully encoded
 *      - false     otherwise
 */
//--------------------------------------------------------------------------------------------------
bool le_cbor_EncodeString
(
    uint8_t **bufferPtr,            ///< [OUT] Pointer of buffer to store the encoded data
    size_t* bufLen,                 ///< [IN/OUT] Size of the buffer available
    const char *stringPtr,          ///< [IN] String to be encoded
    uint32_t maxStringCount         ///< [IN] Max size of the string
)
{
    size_t numBytes = 0;
    if (bufferPtr == NULL || *bufferPtr == NULL ||
        bufLen == NULL ||
        stringPtr == NULL)
    {
        return false;
    }

    uint32_t stringLen = strnlen(stringPtr, maxStringCount);
    // String was too long to fit in the buffer -- return false.
    if (stringPtr[stringLen] != '\0')
    {
        return false;
    }

    numBytes = *bufLen;
    if (!EncodeInteger(bufferPtr, bufLen, stringLen, _LE_CBOR_TEXT_STRING))
    {
        return false;
    }

    if (*bufLen < stringLen)
    {
        // Restore the buffer pointer and available buffer size
        *bufferPtr -= (numBytes - *bufLen);
        *bufLen = numBytes;
        return false;
    }

    LE_CBOR_PACK_SIMPLE_BUFFER(stringPtr, stringLen);

    *bufLen -= stringLen;

    return true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Encode a byte string header into a buffer, increment the buffer pointer if encoding is successful
 *
 * @return
 *      - true      if successfully encoded
 *      - false     otherwise
 */
//--------------------------------------------------------------------------------------------------
bool le_cbor_EncodeByteStringHeader
(
    uint8_t **bufferPtr,            ///< [OUT] Pointer of buffer to store the encoded data
    size_t* bufLen,                 ///< [IN/OUT] Size of the buffer available
    size_t stringLen                ///< [IN] Byte string length
)
{
    return EncodeInteger(bufferPtr, bufLen, stringLen, _LE_CBOR_BYTE_STRING);
}


//--------------------------------------------------------------------------------------------------
/**
 * Encode a string header into a buffer, increment the buffer pointer if encoding is successful
 *
 * @return
 *      - true      if successfully encoded
 *      - false     otherwise
 */
//--------------------------------------------------------------------------------------------------
bool le_cbor_EncodeStringHeader
(
    uint8_t **bufferPtr,            ///< [OUT] Pointer of buffer to store the encoded data
    size_t* bufLen,                 ///< [IN/OUT] Size of the buffer available
    size_t stringLen                ///< [IN] String length
)
{
    return EncodeInteger(bufferPtr, bufLen, stringLen, _LE_CBOR_TEXT_STRING);
}


//--------------------------------------------------------------------------------------------------
/**
 * Encode an array header into a buffer, increment the buffer pointer if encoding is successful
 *
 * @return
 *      - true      if successfully encoded
 *      - false     otherwise
 */
//--------------------------------------------------------------------------------------------------
bool le_cbor_EncodeArrayHeader
(
    uint8_t **bufferPtr,            ///< [OUT] Pointer of buffer to store the encoded data
    size_t* bufLen,                 ///< [IN/OUT] Size of the buffer available
    size_t arraySize                ///< [IN] Size of the array
)
{
    return EncodeInteger(bufferPtr, bufLen, arraySize, _LE_CBOR_ITEM_ARRAY);
}


//--------------------------------------------------------------------------------------------------
/**
 * Encode the header of an indefinite length array into a buffer, increment the buffer pointer if
 * encoding is successful
 *
 * @return
 *      - true      if successfully encoded
 *      - false     otherwise
 */
//--------------------------------------------------------------------------------------------------
bool le_cbor_EncodeIndefArrayHeader
(
    uint8_t** bufferPtr,            ///< [OUT] Pointer of buffer to store the encoded data
    size_t* bufLen                  ///< [IN/OUT] Size of the buffer available
)
{
    if (bufferPtr == NULL || *bufferPtr == NULL ||
        bufLen == NULL || *bufLen < LE_CBOR_INDEF_ARRAY_HEADER_MAX_SIZE)
    {
        return false;
    }

    LE_CBOR_PACK_TINY_ITEM(_LE_CBOR_ITEM_ARRAY, _LE_CBOR_PRIMITIVE_INDEFINITE);

    *bufLen -= LE_CBOR_INDEF_ARRAY_HEADER_MAX_SIZE;

    return true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Encode the end mark of an indefinite length array into a buffer, increment the buffer pointer if
 * encoding is successful
 *
 * @return
 *      - true      if successfully encoded
 *      - false     otherwise
 */
//--------------------------------------------------------------------------------------------------
size_t le_cbor_EncodeEndOfIndefArray
(
    uint8_t** bufferPtr,            ///< [OUT] Pointer of buffer to store the encoded data
    size_t* bufLen                  ///< [IN/OUT] Size of the buffer available
)
{
    if (*bufferPtr == NULL || bufferPtr == NULL ||
        bufLen == NULL || *bufLen < LE_CBOR_INDEF_END_MAX_SIZE)
    {
        return false;
    }

    LE_CBOR_PACK_TINY_ITEM(_LE_CBOR_PRIMITVE, _LE_CBOR_PRIMITIVE_BREAK);

    *bufLen -= LE_CBOR_INDEF_END_MAX_SIZE;

    return true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Encode the header of an indefinite length map into a buffer, increment the buffer pointer if
 * encoding is successful
 *
 * @return
 *      - true      if successfully encoded
 *      - false     otherwise
 */
//--------------------------------------------------------------------------------------------------
size_t le_cbor_EncodeIndefMapHeader
(
    uint8_t** bufferPtr,            ///< [OUT] Pointer of buffer to store the encoded data
    size_t* bufLen                  ///< [IN/OUT] Size of the buffer available
)
{
    if (bufferPtr == NULL || *bufferPtr == NULL ||
        bufLen == NULL || *bufLen < LE_CBOR_INDEF_MAP_HEADER_MAX_SIZE)
    {
        return false;
    }

    LE_CBOR_PACK_TINY_ITEM(_LE_CBOR_PAIR_MAP, _LE_CBOR_PRIMITIVE_INDEFINITE);

    *bufLen -= LE_CBOR_INDEF_MAP_HEADER_MAX_SIZE;

    return true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Encode the end mark of an indefinite length map into a buffer, increment the buffer pointer if
 * encoding is successful
 *
 * @return
 *      - true      if successfully encoded
 *      - false     otherwise
 */
//--------------------------------------------------------------------------------------------------
bool le_cbor_EncodeEndOfIndefMap
(
    uint8_t** bufferPtr,            ///< [OUT] Pointer of buffer to store the encoded data
    size_t* bufLen                  ///< [IN/OUT] Size of the buffer available
)
{
    if (bufferPtr == NULL || *bufferPtr == NULL ||
        bufLen == NULL || *bufLen < LE_CBOR_INDEF_END_MAX_SIZE)
    {
        return false;
    }

    LE_CBOR_PACK_TINY_ITEM(_LE_CBOR_PRIMITVE, _LE_CBOR_PRIMITIVE_BREAK);

    *bufLen -= LE_CBOR_INDEF_END_MAX_SIZE;

    return true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Encode NULL into a buffer, increment the buffer pointer if encoding is successful
 *
 * @return
 *      - true      if successfully encoded
 *      - false     otherwise
 */
//--------------------------------------------------------------------------------------------------
bool le_cbor_EncodeNull
(
    uint8_t** bufferPtr,            ///< [OUT] Pointer of buffer to store the encoded data
    size_t* bufLen                  ///< [IN/OUT] Size of the buffer available
)
{
    if (bufferPtr == NULL || *bufferPtr == NULL ||
        bufLen == NULL || *bufLen < LE_CBOR_NULL_MAX_SIZE)
    {
        return false;
    }

    LE_CBOR_PACK_TINY_ITEM(_LE_CBOR_PRIMITVE, _LE_CBOR_PRIMITIVE_NULL);

    *bufLen -= LE_CBOR_NULL_MAX_SIZE;

    return true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Decode an integer from a buffer, increment the buffer pointer if decoding is successful
 *
 * @return
 *      - true      if successfully decoded
 *      - false     failed to decode
 */
//--------------------------------------------------------------------------------------------------
bool le_cbor_DecodeInteger
(
    uint8_t** bufferPtr,            ///< [IN/OUT] Pointer of buffer for decoding
    int64_t* valuePtr               ///< [OUT] Decoded value
)
{
    int additional;
    unsigned int major;

    if (bufferPtr == NULL || *bufferPtr == NULL || valuePtr == NULL)
    {
        return false;
    }

    LE_CBOR_UNPACK_TINY_ITEM();
    if (major != _LE_CBOR_POS_INTEGER && major != _LE_CBOR_NEG_INTEGER)
    {
       return false;
    }
    else
    {
        bool sign = (major == _LE_CBOR_POS_INTEGER);
        int length;
        if (additional < _LE_CBOR_COMPLEX_THRESHOLD)
        {
            *valuePtr = (sign)? additional: (-1 - additional);
        }
        else
        {
            length = (additional == _LE_CBOR_COMPLEX_THRESHOLD)?1:
                2 << (additional-_LE_CBOR_COMPLEX_THRESHOLD-1);
            LE_CBOR_UNPACK_SIMPLE_BUFFER(((uint8_t*)valuePtr) + sizeof(int64_t)-length, length);
            *valuePtr = be64toh(*valuePtr);
            *valuePtr = (sign)? *valuePtr: (-1 - (*valuePtr));
        }

        return true;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Decode a uint8_t value from a buffer, increment the buffer pointer if decoding is successful
 *
 * @return
 *      - true      if successfully decoded
 *      - false     failed to decode
 */
//--------------------------------------------------------------------------------------------------
bool le_cbor_DecodeUint8
(
    uint8_t** bufferPtr,            ///< [IN/OUT] Pointer of buffer for decoding
    uint8_t* valuePtr               ///< [OUT] Decoded value
)
{
    bool result;
    uint64_t tmpValue = 0;

    if (bufferPtr == NULL || *bufferPtr == NULL || valuePtr == NULL)
    {
        return false;
    }

    result = DecodePositiveInteger(bufferPtr, &tmpValue, _LE_CBOR_POS_INTEGER);
    if (!result || tmpValue > UINT8_MAX)
    {
        return false;
    }

    *valuePtr = tmpValue;
    return true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Decode a uint16_t value from a buffer, increment the buffer pointer if decoding is successful
 *
 * @return
 *      - true      if successfully decoded
 *      - false     failed to decode
 */
//--------------------------------------------------------------------------------------------------
bool le_cbor_DecodeUint16
(
    uint8_t** bufferPtr,            ///< [IN/OUT] Pointer of buffer for decoding
    uint16_t* valuePtr              ///< [OUT] Decoded value
)
{
    bool result;
    uint64_t tmpValue = 0;

    if (bufferPtr == NULL || *bufferPtr == NULL || valuePtr == NULL)
    {
        return false;
    }

    result = DecodePositiveInteger(bufferPtr, &tmpValue, _LE_CBOR_POS_INTEGER);
    if (!result || tmpValue > UINT16_MAX)
    {
        return false;
    }

    *valuePtr = tmpValue;
    return true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Decode a uint32_t value from a buffer, increment the buffer pointer if decoding is successful
 *
 * @return
 *      - true      if successfully decoded
 *      - false     failed to decode
 */
//--------------------------------------------------------------------------------------------------
bool le_cbor_DecodeUint32
(
    uint8_t** bufferPtr,            ///< [IN/OUT] Pointer of buffer for decoding
    uint32_t* valuePtr              ///< [OUT] Decoded value
)
{
    bool result;
    uint64_t tmpValue = 0;

    if (bufferPtr == NULL || *bufferPtr == NULL || valuePtr == NULL)
    {
        return false;
    }

    result = DecodePositiveInteger(bufferPtr, &tmpValue, _LE_CBOR_POS_INTEGER);
    if (!result || tmpValue > UINT32_MAX)
    {
        return false;
    }

    *valuePtr = tmpValue;
    return true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Decode a uint64_t value from a buffer, increment the buffer pointer if decoding is successful
 *
 * @return
 *      - true      if successfully decoded
 *      - false     failed to decode
 */
//--------------------------------------------------------------------------------------------------
bool le_cbor_DecodeUint64
(
    uint8_t** bufferPtr,            ///< [IN/OUT] Pointer of buffer for decoding
    uint64_t* valuePtr              ///< [OUT] Decoded value
)
{
    bool result;
    uint64_t tmpValue = 0;

    if (bufferPtr == NULL || *bufferPtr == NULL || valuePtr == NULL)
    {
        return false;
    }

    result = DecodePositiveInteger(bufferPtr, &tmpValue, _LE_CBOR_POS_INTEGER);
    if (!result || tmpValue > UINT64_MAX)
    {
        return false;
    }
    *valuePtr = tmpValue;
    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Decode a int8_t value from a buffer, increment the buffer pointer if decoding is successful
 *
 * @return
 *      - true      if successfully decoded
 *      - false     failed to decode
 */
//--------------------------------------------------------------------------------------------------
bool le_cbor_DecodeInt8
(
    uint8_t** bufferPtr,            ///< [IN/OUT] Pointer of buffer for decoding
    int8_t* valuePtr                ///< [OUT] Decoded value
)
{
    bool result;
    int64_t  tmpValue = 0;

    if (bufferPtr == NULL || *bufferPtr == NULL || valuePtr == NULL)
    {
        return false;
    }

    result = le_cbor_DecodeInteger(bufferPtr, &tmpValue);
    if (!result || tmpValue > INT8_MAX || tmpValue < INT8_MIN)
    {
        return false;
    }

    *valuePtr = tmpValue;
    return true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Decode a int16_t value from a buffer, increment the buffer pointer if decoding is successful
 *
 * @return
 *      - true      if successfully decoded
 *      - false     failed to decode
 */
//--------------------------------------------------------------------------------------------------
bool le_cbor_DecodeInt16
(
    uint8_t** bufferPtr,            ///< [IN/OUT] Pointer of buffer for decoding
    int16_t* valuePtr               ///< [OUT] Decoded value
)
{
    bool result;
    int64_t  tmpValue = 0;

    if (bufferPtr == NULL || *bufferPtr == NULL || valuePtr == NULL)
    {
        return false;
    }

    result = le_cbor_DecodeInteger(bufferPtr, &tmpValue);
    if (!result || tmpValue > INT16_MAX || tmpValue < INT16_MIN)
    {
        return false;
    }

    *valuePtr = tmpValue;
    return true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Decode a int32_t value from a buffer, increment the buffer pointer if decoding is successful
 *
 * @return
 *      - true      if successfully decoded
 *      - false     failed to decode
 */
//--------------------------------------------------------------------------------------------------
bool le_cbor_DecodeInt32
(
    uint8_t** bufferPtr,            ///< [IN/OUT] Pointer of buffer for decoding
    int32_t* valuePtr               ///< [OUT] Decoded value
)
{
    bool result;
    int64_t  tmpValue = 0;

    if (bufferPtr == NULL || *bufferPtr == NULL || valuePtr == NULL)
    {
        return false;
    }

    result = le_cbor_DecodeInteger(bufferPtr, &tmpValue);
    if (!result || tmpValue > INT32_MAX || tmpValue < INT32_MIN)
    {
        return false;
    }

    *valuePtr = tmpValue;
    return true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Decode a int64_t value from a buffer, increment the buffer pointer if decoding is successful
 *
 * @return
 *      - true      if successfully decoded
 *      - false     failed to decode
 */
//--------------------------------------------------------------------------------------------------
bool le_cbor_DecodeInt64
(
    uint8_t** bufferPtr,            ///< [IN/OUT] Pointer of buffer for decoding
    int64_t* valuePtr               ///< [OUT] Decoded value
)
{
    bool result;
    int64_t  tmpValue = 0;

    if (bufferPtr == NULL || *bufferPtr == NULL || valuePtr == NULL)
    {
        return false;
    }

    result = le_cbor_DecodeInteger(bufferPtr, &tmpValue);
    if (!result || tmpValue > INT64_MAX || tmpValue < INT64_MIN)
    {
        return false;
    }

    *valuePtr = tmpValue;
    return true;
}

//--------------------------------------------------------------------------------------------------
/**
 * Decode a boolean value from a buffer, increment the buffer pointer if decoding is successful
 *
 * @return
 *      - true      if successfully decoded
 *      - false     failed to decode
 */
//--------------------------------------------------------------------------------------------------
bool le_cbor_DecodeBool
(
    uint8_t** bufferPtr,            ///< [IN/OUT] Pointer of buffer for decoding
    bool* valuePtr                  ///< [OUT] Decoded value
)
{
    // Treat boolean as uint8_t for packing, regardless of underlying OS type.
    // Underlying type has been int on some platforms in the past.
    uint8_t simpleValue;

    if (bufferPtr == NULL || *bufferPtr == NULL || valuePtr == NULL)
    {
        return false;
    }

    memcpy(&simpleValue, *bufferPtr, sizeof(simpleValue));

    *bufferPtr = ((uint8_t* )*bufferPtr) + sizeof(simpleValue);

    if (simpleValue == (_LE_CBOR_PRIMITVE << 5 | _LE_CBOR_PRIMITIVE_FALSE))
    {
        *valuePtr = false;
    }
    else if (simpleValue == (_LE_CBOR_PRIMITVE << 5 | _LE_CBOR_PRIMITIVE_TRUE))
    {
        *valuePtr = true;
    }
    else
    {
        return false;
    }

    return true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Decode a char type value from a buffer, increment the buffer pointer if decoding is successful
 *
 * @return
 *      - true      if successfully decoded
 *      - false     failed to decode
 */
//--------------------------------------------------------------------------------------------------
bool le_cbor_DecodeChar
(
    uint8_t** bufferPtr,            ///< [IN/OUT] Pointer of buffer for decoding
    char* valuePtr                  ///< [OUT] Decoded value
)
{
    bool result;
    uint64_t tmpValue = 0;

    if (bufferPtr == NULL || *bufferPtr == NULL || valuePtr == NULL)
    {
        return false;
    }

    result = DecodePositiveInteger(bufferPtr, &tmpValue, _LE_CBOR_POS_INTEGER);
    if (!result || tmpValue > UINT8_MAX)
    {
        return false;
    }
    *valuePtr = tmpValue;
    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Decode a float type value from a buffer, increment the buffer pointer if decoding is successful
 *
 * @return
 *      - true      if successfully decoded
 *      - false     failed to decode
 */
//--------------------------------------------------------------------------------------------------
bool le_cbor_DecodeFloat
(
    uint8_t** bufferPtr,            ///< [IN/OUT] Pointer of buffer for decoding
    float* valuePtr                 ///< [OUT] Decoded value
)
{
    unsigned int major;
    unsigned int additional;
    uint32_t rawValue;

    if (bufferPtr == NULL || *bufferPtr == NULL || valuePtr == NULL)
    {
        return false;
    }

    LE_CBOR_UNPACK_TINY_ITEM();
    if ((major != _LE_CBOR_PRIMITVE) ||
        (additional != _LE_CBOR_PRIMITIVE_FLOAT))
    {
        return false;
    }
    else
    {
        LE_CBOR_UNPACK_SIMPLE_BUFFER(&rawValue, sizeof(uint32_t));
        rawValue = be32toh(rawValue);
        memcpy(valuePtr, &rawValue, sizeof(float));
    }
    return true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Decode a double type value from a buffer, increment the buffer pointer if decoding is successful
 *
 * @return
 *      - true      if successfully decoded
 *      - false     failed to decode
 */
//--------------------------------------------------------------------------------------------------
bool le_cbor_DecodeDouble
(
    uint8_t** bufferPtr,            ///< [IN/OUT] Pointer of buffer for decoding
    double* valuePtr                ///< [OUT] Decoded value
)
{
    unsigned int major;
    unsigned int additional;
    uint64_t rawValue;

    if (bufferPtr == NULL || *bufferPtr == NULL || valuePtr == NULL)
    {
        return false;
    }

    LE_CBOR_UNPACK_TINY_ITEM();
    if ((major != _LE_CBOR_PRIMITVE) ||
        (additional != _LE_CBOR_PRIMITIVE_DOUBLE))
    {
        return false;
    }
    else
    {
        LE_CBOR_UNPACK_SIMPLE_BUFFER(&rawValue, sizeof(uint64_t));
        rawValue = be64toh(rawValue);
        memcpy(valuePtr, &rawValue, sizeof(double));
    }
    return true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Decode a string from a buffer, increment the buffer pointer if decoding is successful and given
 * string pointer is valid, otherwise only update the buffer pointer
 *
 * @return
 *      - true      if successfully decoded
 *      - false     failed to decode
 */
//--------------------------------------------------------------------------------------------------
bool le_cbor_DecodeString
(
    uint8_t** bufferPtr,            ///< [IN/OUT] Pointer of buffer for decoding
    char *stringPtr,                ///< [IN/OUT] Pointer of string buffer for storing decoded data
    size_t bufferSize               ///< [IN] String buffer size
)
{
    size_t stringSize;
    uint64_t value = 0;

    if (bufferPtr == NULL || *bufferPtr == NULL)
    {
        return false;
    }

    if (!DecodePositiveInteger(bufferPtr, &value, _LE_CBOR_TEXT_STRING))
    {
        return false;
    }
    else
    {
        stringSize = (size_t)value;
    }

    if (stringSize > bufferSize)
    {
        return false;
    }

    if (!stringPtr)
    {
        // Only unpack into string buffer when the string is not zero sized
        if (stringSize)
        {
            return false;
        }
        else
        {
            return true;
        }
    }

    memcpy(stringPtr, *bufferPtr, stringSize);
    stringPtr[stringSize] = '\0';

    *bufferPtr = *bufferPtr + stringSize;

    return true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Decode a string header from a buffer, increment the buffer pointer if decoding is successful
 *
 * @return
 *      - true      if successfully decoded
 *      - false     failed to decode
 */
//--------------------------------------------------------------------------------------------------
bool le_cbor_DecodeStringHeader
(
    uint8_t** bufferPtr,            ///< [IN/OUT] Pointer of buffer for decoding
    size_t* stringSizePtr           ///< [OUT] Decoded value
)
{
    uint64_t value = 0;

    if (bufferPtr == NULL || *bufferPtr == NULL || stringSizePtr == NULL)
    {
        return false;
    }

    if (!DecodePositiveInteger(bufferPtr, &value, _LE_CBOR_TEXT_STRING))
    {
        return false;
    }
    else
    {
        *stringSizePtr = (size_t)value;
    }
    return true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Decode an array header from a buffer, increment the buffer pointer if decoding is successful
 *
 * @return
 *      - true      if successfully decoded
 *      - false     failed to decode
 */
//--------------------------------------------------------------------------------------------------
bool le_cbor_DecodeArrayHeader
(
    uint8_t **bufferPtr,            ///< [IN/OUT] Pointer of buffer for decoding
    size_t *arrayCountPtr           ///< [OUT] Decoded value
)
{
    uint64_t value = 0;

    if (bufferPtr == NULL || *bufferPtr == NULL || arrayCountPtr == NULL)
    {
        return false;
    }

    if (!DecodePositiveInteger(bufferPtr, &value, _LE_CBOR_ITEM_ARRAY))
    {
        return false;
    }
    *arrayCountPtr = value;

    return true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Decode the header of indefinite length array from a buffer, increment the buffer pointer if
 * decoding is successful
 *
 * @return
 *      - true      if successfully decoded
 *      - false     failed to decode
 */
//--------------------------------------------------------------------------------------------------
bool le_cbor_DecodeIndefArrayHeader
(
    uint8_t** bufferPtr             ///< [IN/OUT] Pointer of buffer for decoding
)
{
    unsigned int major;
    unsigned int additional;

    if (bufferPtr == NULL || *bufferPtr == NULL)
    {
        return false;
    }

    LE_CBOR_UNPACK_TINY_ITEM();
    if (major == _LE_CBOR_ITEM_ARRAY && additional == _LE_CBOR_PRIMITIVE_INDEFINITE)
    {
        return true;
    }
    else
    {
        return false;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Decode the end mark of indefinite length array from a buffer, increment the buffer pointer if
 * decoding is successful
 *
 * @return
 *      - true      if successfully decoded
 *      - false     failed to decode
 */
//--------------------------------------------------------------------------------------------------
bool le_cbor_DecodeEndOfIndefArray
(
    uint8_t** bufferPtr             ///< [IN/OUT] Pointer of buffer for decoding
)
{
    unsigned int major;
    unsigned int additional;

    if (bufferPtr == NULL || *bufferPtr == NULL)
    {
        return false;
    }

    LE_CBOR_UNPACK_TINY_ITEM();
    if (major == _LE_CBOR_PRIMITVE && additional == _LE_CBOR_PRIMITIVE_BREAK)
    {
        return true;
    }
    else
    {
        return false;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Decode the byte string header from a buffer, increment the buffer pointer if decoding is
 * successful
 *
 * @return
 *      - true      if successfully decoded
 *      - false     failed to decode
 */
//--------------------------------------------------------------------------------------------------
bool le_cbor_DecodeByteStringHeader
(
    uint8_t** bufferPtr,            ///< [IN/OUT] Pointer of buffer for decoding
    size_t* lengthPtr               ///< [OUT] Decoded value
)
{
    uint64_t value = 0;

    if (bufferPtr == NULL || *bufferPtr == NULL || lengthPtr == NULL)
    {
        return false;
    }

    if (!DecodePositiveInteger(bufferPtr, &value, _LE_CBOR_BYTE_STRING))
    {
        return false;
    }
    else
    {
        *lengthPtr = (size_t) value;
    }

    return true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Decode the byte string from a buffer, increment the buffer pointer if decoding is successful
 *
 * @return
 *      - true      if successfully decoded
 *      - false     failed to decode
 */
//--------------------------------------------------------------------------------------------------
bool le_cbor_DecodeByteString
(
    uint8_t** bufferPtr,            ///< [IN/OUT] Pointer of buffer for decoding
    void *arrayPtr,                 ///< [OUT] Output buffer to store the decoded data
    size_t *arrayCountPtr,          ///< [OUT] Number of bytes encoded
    size_t arrayMaxCount            ///< [IN] Size of output buffer
)
{
    uint64_t value = 0;

    if (bufferPtr == NULL || *bufferPtr == NULL || arrayCountPtr == NULL)
    {
        return false;
    }

    if (!DecodePositiveInteger(bufferPtr, &value, _LE_CBOR_BYTE_STRING))
    {
        return false;
    }
    else
    {
        *arrayCountPtr = (size_t)value;
    }

    if (*arrayCountPtr > arrayMaxCount)
    {
        return false;
    }
    else if (!arrayPtr)
    {
        // Missing array pointer must match zero sized array.
        return (*arrayCountPtr == 0);
    }
    else
    {
        LE_CBOR_UNPACK_SIMPLE_BUFFER(arrayPtr, *arrayCountPtr);
        return true;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Decode a tag from a buffer, increment the buffer pointer if decoding is successful
 *
 * @return
 *      - true      if successfully decoded
 *      - false     failed to decode
 */
//--------------------------------------------------------------------------------------------------
bool le_cbor_DecodeTag
(
    uint8_t** bufferPtr,                ///< [IN/OUT] Pointer of buffer for decoding
    le_cbor_Tag_t* tagIdPtr             ///< [OUT] Decoded value
)
{
    return DecodePositiveInteger(bufferPtr, tagIdPtr, _LE_CBOR_TAG);
}

//--------------------------------------------------------------------------------------------------
/**
 * Decode a map header from a buffer, increment the buffer pointer if decoding is successful
 *
 * @return
 *      - true      if successfully decoded
 *      - false     failed to decode
 */
//--------------------------------------------------------------------------------------------------
bool le_cbor_DecodeMapHeader
(
    uint8_t **bufferPtr,            ///< [IN/OUT] Pointer of buffer for decoding
    size_t *mapCountPtr             ///< [OUT] Decoded value
)
{
    uint64_t value = 0;

    if (bufferPtr == NULL || *bufferPtr == NULL || mapCountPtr == NULL)
    {
        return false;
    }

    if (!DecodePositiveInteger(bufferPtr, &value, _LE_CBOR_PAIR_MAP))
    {
        return false;
    }
    *mapCountPtr = value;

    return true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Decode one item from cbor stream in buffer, increment the buffer pointer and decrease the
 * available buffer size if decoding is successful
 *
 * @return
 *      - true      if successfully decoded
 *      - false     failed to decode
 */
//--------------------------------------------------------------------------------------------------
bool le_cbor_DecodeStream
(
    uint8_t** bufferPtr,                ///< [IN/OUT] Pointer of buffer for decoding
    size_t* bufferSize,                 ///< [IN/OUT] Available size of the buffer
    const le_cbor_Handlers_t* callbacks,///< [IN] Callback handlers
    void* context                       ///< [OUT] Context used for callbacks to store decoded data
)
{
    if (bufferPtr == NULL || *bufferPtr == NULL || bufferSize == NULL || *bufferSize < 1)
    {
        return false;
    }

    // Parse the initial MTB byte
    uint8_t mtb = **bufferPtr;

    switch (mtb)
    {
        case 0x00:
        case 0x01:
        case 0x02:
        case 0x03:
        case 0x04:
        case 0x05:
        case 0x06:
        case 0x07:
        case 0x08:
        case 0x09:
        case 0x0A:
        case 0x0B:
        case 0x0C:
        case 0x0D:
        case 0x0E:
        case 0x0F:
        case 0x10:
        case 0x11:
        case 0x12:
        case 0x13:
        case 0x14:
        case 0x15:
        case 0x16:
        case 0x17:
            // Embedded one byte unsigned integer
            callbacks->uint8(context, mtb);
            *bufferPtr += 1;
            *bufferSize -= 1;
            return true;
        case 0x18:
            // One byte unsigned integer
            {
                uint8_t val;
                if ((*bufferSize < LE_CBOR_UINT8_MAX_SIZE) ||
                    (!le_cbor_DecodeUint8(bufferPtr, &val)))
                {
                    return false;
                }

                callbacks->uint8(context, val);

                *bufferSize -= LE_CBOR_UINT8_MAX_SIZE;
            }
            return true;
        case 0x19:
            // Two bytes unsigned integer
            {
                uint16_t val;

                if ((*bufferSize < LE_CBOR_UINT16_MAX_SIZE) ||
                    (!le_cbor_DecodeUint16(bufferPtr, &val)))
                {
                    return false;
                }

                callbacks->uint16(context, val);

                *bufferSize -= LE_CBOR_UINT16_MAX_SIZE;
            }
            return true;
        case 0x1A:
            // Four bytes unsigned integer
            {
                uint32_t val;

                if ((*bufferSize < LE_CBOR_UINT32_MAX_SIZE) ||
                    (!le_cbor_DecodeUint32(bufferPtr, &val)))
                {
                    return false;
                }

                callbacks->uint32(context, val);

                *bufferSize -= LE_CBOR_UINT32_MAX_SIZE;
            }
            return true;
        case 0x1B:
            // Eight bytes unsigned integer
            {
                uint64_t val;

                if ((*bufferSize < LE_CBOR_UINT64_MAX_SIZE) ||
                    (!le_cbor_DecodeUint64(bufferPtr, &val)))
                {
                    return false;
                }

                callbacks->uint64(context, val);

                *bufferSize -= LE_CBOR_UINT64_MAX_SIZE;
            }
            return true;
        case 0x1C:
        case 0x1D:
        case 0x1E:
        case 0x1F:
            // Reserved
            return false;
        case 0x20:
        case 0x21:
        case 0x22:
        case 0x23:
        case 0x24:
        case 0x25:
        case 0x26:
        case 0x27:
        case 0x28:
        case 0x29:
        case 0x2A:
        case 0x2B:
        case 0x2C:
        case 0x2D:
        case 0x2E:
        case 0x2F:
        case 0x30:
        case 0x31:
        case 0x32:
        case 0x33:
        case 0x34:
        case 0x35:
        case 0x36:
        case 0x37:
            // Embedded one byte negative integer
            callbacks->negInt8(context, mtb - 0x20); /* 0x20 offset */
            *bufferPtr += 1;
            *bufferSize -= 1;
            return true;
        case 0x38:
            // One byte negative integer
            {
                int8_t val;
                if (*bufferSize < LE_CBOR_INT8_MAX_SIZE)
                {
                    return false;
                }
                else if (!le_cbor_DecodeInt8(bufferPtr, &val))
                {
                    // If le_cbor_DecodeInt8 returns an error, try to encode the value in int16
                    // ex: -256 in cbor is sent by 0x38 FF and FF is considered as negative in
                    // le_cbor_DecodeInt8 and false is returned
                    uint8_t* tmp;
                    uint16_t tempU16;

                    // Because le_cbor_DecodeInt16 updated the bufferPtr address, come back to
                    // initial address
                    *bufferPtr -= LE_CBOR_INT8_MAX_SIZE;

                    tmp = *bufferPtr;
                    tempU16 = tmp[1];

                    callbacks->negInt16(context, tempU16 + 1);
                }
                else
                {
                    callbacks->negInt8(context, val);
                }
                *bufferSize -= LE_CBOR_INT8_MAX_SIZE;
            }
            return true;
        case 0x39:
            // Two bytes negative integer
            {
                int16_t val;
                if (*bufferSize < LE_CBOR_INT16_MAX_SIZE)
                {
                    return false;
                }
                else if(!le_cbor_DecodeInt16(bufferPtr, &val))
                {
                    // If le_cbor_DecodeInt16 returns an error, try to encode the value in int16
                    // ex: -65536 in cbor is sent by 0x39 FF FF and FF FF is considered as negative
                    // in le_cbor_DecodeInt16 and false is returned
                    int additional;
                    unsigned int major;
                    int length;
                    uint64_t tempU64 = 0;

                    // Because le_cbor_DecodeInt16 updated the bufferPtr address, come back to
                    // initial address
                    *bufferPtr -= LE_CBOR_INT16_MAX_SIZE;

                    LE_CBOR_UNPACK_TINY_ITEM();

                    if (major != _LE_CBOR_NEG_INTEGER)
                    {
                        return false;
                    }

                    length = (additional == _LE_CBOR_COMPLEX_THRESHOLD)?1:
                        2 << (additional-_LE_CBOR_COMPLEX_THRESHOLD-1);
                    LE_CBOR_UNPACK_SIMPLE_BUFFER(((uint8_t*)&tempU64) + sizeof(uint64_t)-length, length);
                    tempU64 = (uint64_t)be64toh(tempU64);

                    callbacks->negInt64(context, tempU64 + 1);
                }
                else
                {
                    callbacks->negInt16(context, val);
                }
                *bufferSize -= LE_CBOR_INT16_MAX_SIZE;
            }
            return true;
        case 0x3A:
            // Four bytes negative integer
            {
                int32_t val;
                if (*bufferSize < LE_CBOR_INT32_MAX_SIZE)
                {
                    return false;
                }
                else if(!le_cbor_DecodeInt32(bufferPtr, &val))
                {
                    // If le_cbor_DecodeInt16 returns an error, try to encode the value in int16
                    // ex: -65536 in cbor is sent by 0x39 FF FF and FF FF is considered as negative
                    // in le_cbor_DecodeInt16 and false is returned
                    int additional;
                    unsigned int major;
                    int length;
                    uint64_t tempU64 = 0;

                    // Because le_cbor_DecodeInt32 updated the bufferPtr address, come back to
                    // initial address
                    *bufferPtr -= LE_CBOR_INT32_MAX_SIZE;

                    LE_CBOR_UNPACK_TINY_ITEM();

                    if (major != _LE_CBOR_NEG_INTEGER)
                    {
                        return false;
                    }

                    length = (additional == _LE_CBOR_COMPLEX_THRESHOLD)?1:
                        2 << (additional-_LE_CBOR_COMPLEX_THRESHOLD-1);
                    LE_CBOR_UNPACK_SIMPLE_BUFFER(((uint8_t*)&tempU64) + sizeof(uint64_t)-length, length);
                    tempU64 = (uint64_t)be64toh(tempU64);

                    callbacks->negInt64(context, tempU64 + 1);
                }
                else
                {
                    callbacks->negInt32(context, val);
                }

                *bufferSize -= LE_CBOR_INT32_MAX_SIZE;
            }
            return true;
        case 0x3B:
            // Eight bytes negative integer
            {
                int64_t val;
                if ((*bufferSize < LE_CBOR_INT64_MAX_SIZE) ||
                    (!le_cbor_DecodeInt64(bufferPtr, &val)))
                {
                    return false;
                }

                callbacks->negInt64(context, val);

                *bufferSize -= LE_CBOR_INT64_MAX_SIZE;
            }
            return true;
        case 0x3C:
        case 0x3D:
        case 0x3E:
        case 0x3F:
            // Reserved
            return false;
        case 0x40:
        case 0x41:
        case 0x42:
        case 0x43:
        case 0x44:
        case 0x45:
        case 0x46:
        case 0x47:
        case 0x48:
        case 0x49:
        case 0x4A:
        case 0x4B:
        case 0x4C:
        case 0x4D:
        case 0x4E:
        case 0x4F:
        case 0x50:
        case 0x51:
        case 0x52:
        case 0x53:
        case 0x54:
        case 0x55:
        case 0x56:
        case 0x57: // Embedded length byte string
        case 0x58: // One byte length byte string
        case 0x59: // Two bytes length byte string
        case 0x5A: // Four bytes length byte string
        case 0x5B: // Eight bytes length byte string
            {
                size_t strLen;
                int headerLen;
                uint8_t* bufPtr = *bufferPtr;

                if (!le_cbor_DecodeByteStringHeader(bufferPtr, &strLen))
                {
                    return false;
                }

                headerLen = *bufferPtr - bufPtr;
                if (strLen + headerLen > *bufferSize)
                {
                    // Restore the pointer
                    *bufferPtr -= headerLen;
                    return false;
                }

                callbacks->byteString(context, *bufferPtr, strLen);

                *bufferPtr += strLen;
                *bufferSize -= (strLen + headerLen);
            }
            return true;
        case 0x5C:
        case 0x5D:
        case 0x5E:
            // Reserved
            return false;
        case 0x5F:
            // Indefinite byte string
            callbacks->byteStringStart(context);
            *bufferPtr += LE_CBOR_INDEF_BYTE_STR_HEADER_MAX_SIZE;
            *bufferSize -= LE_CBOR_INDEF_BYTE_STR_HEADER_MAX_SIZE;
            return true;
        case 0x60:
        case 0x61:
        case 0x62:
        case 0x63:
        case 0x64:
        case 0x65:
        case 0x66:
        case 0x67:
        case 0x68:
        case 0x69:
        case 0x6A:
        case 0x6B:
        case 0x6C:
        case 0x6D:
        case 0x6E:
        case 0x6F:
        case 0x70:
        case 0x71:
        case 0x72:
        case 0x73:
        case 0x74:
        case 0x75:
        case 0x76:
        case 0x77: // Embedded one byte length string
        case 0x78: // One byte length string
        case 0x79: // Two bytes length string
        case 0x7A: // Four bytes length string
        case 0x7B: // Eight bytes length string
            {
                size_t strLen;
                int headerLen;
                uint8_t* bufPtr = *bufferPtr;

                if (!le_cbor_DecodeStringHeader(bufferPtr, &strLen))
                {
                    return false;
                }

                headerLen = *bufferPtr - bufPtr;
                if (strLen + headerLen > *bufferSize)
                {
                    // Restore the pointer
                    *bufferPtr -= headerLen;
                    return false;
                }

                callbacks->string(context, *bufferPtr, strLen);

                *bufferPtr += strLen;
                *bufferSize -= (strLen + headerLen);
            }
            return true;
        case 0x7C:
        case 0x7D:
        case 0x7E:
            // Reserved
            return false;
        case 0x7F:
            // Indefinite length string
            callbacks->stringStart(context);
            *bufferPtr += LE_CBOR_INDEF_STR_HEADER_MAX_SIZE;
            *bufferSize -= LE_CBOR_INDEF_STR_HEADER_MAX_SIZE;
            return true;
        case 0x80:
        case 0x81:
        case 0x82:
        case 0x83:
        case 0x84:
        case 0x85:
        case 0x86:
        case 0x87:
        case 0x88:
        case 0x89:
        case 0x8A:
        case 0x8B:
        case 0x8C:
        case 0x8D:
        case 0x8E:
        case 0x8F:
        case 0x90:
        case 0x91:
        case 0x92:
        case 0x93:
        case 0x94:
        case 0x95:
        case 0x96:
        case 0x97: // Embedded one byte length array
        case 0x98: // One byte length array
        case 0x99: // Two bytes length array
        case 0x9A: // Four bytes length array
        case 0x9B: // Eight bytes length array
            {
                size_t arrayLen;
                int headerLen;
                uint8_t* bufPtr = *bufferPtr;

                if (!le_cbor_DecodeArrayHeader(bufferPtr, &arrayLen))
                {
                    return false;
                }

                headerLen = *bufferPtr - bufPtr;
                if (arrayLen + headerLen > *bufferSize)
                {
                    // Restore the pointer
                    *bufferPtr -= headerLen;
                    return false;
                }

                callbacks->arrayStart(context, arrayLen);

                *bufferPtr += arrayLen;
                *bufferSize -= (arrayLen + headerLen);
            }
            return true;
        case 0x9C:
        case 0x9D:
        case 0x9E:
            // Reserved
            return false;
        case 0x9F:
            // Indefinite length array
            callbacks->indefArrayStart(context);
            *bufferPtr += LE_CBOR_INDEF_ARRAY_HEADER_MAX_SIZE;
            *bufferSize -= LE_CBOR_INDEF_ARRAY_HEADER_MAX_SIZE;
            return true;
        case 0xA0:
        case 0xA1:
        case 0xA2:
        case 0xA3:
        case 0xA4:
        case 0xA5:
        case 0xA6:
        case 0xA7:
        case 0xA8:
        case 0xA9:
        case 0xAA:
        case 0xAB:
        case 0xAC:
        case 0xAD:
        case 0xAE:
        case 0xAF:
        case 0xB0:
        case 0xB1:
        case 0xB2:
        case 0xB3:
        case 0xB4:
        case 0xB5:
        case 0xB6:
        case 0xB7: // Embedded one byte length map
        case 0xB8: // One byte length map
        case 0xB9: // Two bytes length map
        case 0xBA: // Four bytes length map
        case 0xBB: // Eight bytes length map
            {
                size_t mapLen;
                int headerLen;
                uint8_t* bufPtr = *bufferPtr;

                if (!le_cbor_DecodeMapHeader(bufferPtr, &mapLen))
                {
                    return false;
                }

                headerLen = *bufferPtr - bufPtr;
                if (mapLen + headerLen > *bufferSize)
                {
                    // Restore the pointer
                    *bufferPtr -= headerLen;
                    return false;
                }

                callbacks->mapStart(context, mapLen);

                *bufferPtr += mapLen;
                *bufferSize -= (mapLen + headerLen);
            }
            return true;
        case 0xBC:
        case 0xBD:
        case 0xBE:
            // Reserved
            return false;
        case 0xBF:
            // Indefinite length map
            callbacks->indefMapStart(context);
            *bufferPtr += LE_CBOR_INDEF_MAP_HEADER_MAX_SIZE;
            *bufferSize -= LE_CBOR_INDEF_MAP_HEADER_MAX_SIZE;
            return true;

        // As per tags registry: https://www.iana.org/assignments/cbor-tags/cbor-tags.xhtml
        case 0xC0: // Text date/time - RFC 3339 tag
        case 0xC1: // Epoch date tag, fallthrough
        case 0xC2: // Positive bignum tag, fallthrough
        case 0xC3: // Negative bignum tag, fallthrough
        case 0xC4: // Fraction, fallthrough
        case 0xC5: // Big float
        case 0xC6:
        case 0xC7:
        case 0xC8:
        case 0xC9:
        case 0xCA:
        case 0xCB:
        case 0xCC:
        case 0xCD:
        case 0xCE:
        case 0xCF: // Unassigned tag value
        case 0xD0: // COSE_Encrypt0
        case 0xD1: // COSE_Mac0
        case 0xD2: // COSE_Sign1
        case 0xD3:
        case 0xD4: // Unassigned tag value
        case 0xD5: // Expected b64url conversion tag
        case 0xD6: // Expected b64 conversion tag
        case 0xD7: // Expected b16 conversion tag
        case 0xD8: /* 1B tag */
        case 0xD9: /* 2B tag */
        case 0xDA: /* 4B tag */
        case 0xDB: /* 8B tag */
            {
                le_cbor_Tag_t tag;
                uint8_t* bufPtr = *bufferPtr;
                int tagLen;
                if (!le_cbor_DecodeTag(bufferPtr, &tag))
                {
                    return false;
                }

                tagLen = *bufferPtr - bufPtr;

                if (*bufferSize < tagLen)
                {
                    return false;
                }

                callbacks->tag(context, tag);

                *bufferSize -= tagLen;
            }
            return true;
        case 0xDC:
        case 0xDD:
        case 0xDE:
        case 0xDF: // Reserved
            return false;
        case 0xE0:
        case 0xE1:
        case 0xE2:
        case 0xE3:
        case 0xE4:
        case 0xE5:
        case 0xE6:
        case 0xE7:
        case 0xE8:
        case 0xE9:
        case 0xEA:
        case 0xEB:
        case 0xEC:
        case 0xED:
        case 0xEE:
        case 0xEF:
        case 0xF0:
        case 0xF1:
        case 0xF2:
        case 0xF3: // Simple value - unassigned
            return false;
        case 0xF4:
            // False
            callbacks->boolean(context, false);
            *bufferPtr += LE_CBOR_BOOL_MAX_SIZE;
            *bufferSize -= LE_CBOR_BOOL_MAX_SIZE;
            return true;
        case 0xF5:
            // True
            callbacks->boolean(context, true);
            *bufferPtr += LE_CBOR_BOOL_MAX_SIZE;
            *bufferSize -= LE_CBOR_BOOL_MAX_SIZE;
            return true;
        case 0xF6:
            // Null
            callbacks->null(context);
            *bufferPtr += LE_CBOR_NULL_MAX_SIZE;
            *bufferSize -= LE_CBOR_NULL_MAX_SIZE;
            return true;
        case 0xF7:
            // Undefined, consider as successful decoding
            callbacks->undefined(context);
            *bufferPtr += 1;
            *bufferSize -= 1;
            return true;
        case 0xF8:
            // 1B simple value, unassigned
            return false;
        case 0xF9:
            // 2B float
            {
                float val;
                if (*bufferSize < LE_CBOR_HALF_FLOAT_MAX_SIZE ||
                    !DecodeHalfFloat(bufferPtr, &val))
                {
                    return false;
                }

                callbacks->float2(context, val);
                *bufferSize -= LE_CBOR_HALF_FLOAT_MAX_SIZE;
            }
            return true;
        case 0xFA:
            // 4B float
            {
                float val;
                if (*bufferSize < LE_CBOR_FLOAT_MAX_SIZE ||
                    !le_cbor_DecodeFloat(bufferPtr, &val))
                {
                    return false;
                }

                callbacks->float4(context, val);
                *bufferSize -= LE_CBOR_FLOAT_MAX_SIZE;
            }
            return true;
        case 0xFB:
            // 8B float
            {
                double val;
                if (*bufferSize < LE_CBOR_DOUBLE_MAX_SIZE ||
                    !le_cbor_DecodeDouble(bufferPtr, &val))
                {
                    return false;
                }

                callbacks->float8(context, val);
                *bufferSize -= LE_CBOR_DOUBLE_MAX_SIZE;
            }
            return true;
        case 0xFC:
        case 0xFD:
        case 0xFE:
            // Reserved
            return false;
        case 0xFF:
            // Break
            callbacks->indefBreak(context);
            *bufferPtr += LE_CBOR_INDEF_END_MAX_SIZE;
            *bufferSize -= LE_CBOR_INDEF_END_MAX_SIZE;
            return true;
        default:
            // Never happens
            break;
    }

    return false;
}
