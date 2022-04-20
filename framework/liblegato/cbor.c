
/**
 * @file cbor.c
 *
 * Implementation of CBOR encoding and decoding interface for Legato.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"


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
#define _LE_CBOR_SEMANTIC_TAG       6
#define _LE_CBOR_PRIMITVE           7

#define _LE_CBOR_COMPLEX_THRESHOLD      24

// CBOR short count
#define _LE_CBOR_PRIMITIVE_FALSE        20
#define _LE_CBOR_PRIMITIVE_TRUE         21
#define _LE_CBOR_PRIMITIVE_NULL         22
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
 * Encode a TagID into a buffer, increment the buffer pointer if encoding is successful
 *
 * @return
 *      - true      if successfully encoded
 *      - false     otherwise
 */
//--------------------------------------------------------------------------------------------------
bool le_cbor_EncodeSemanticTag
(
    uint8_t** bufferPtr,            ///< [OUT] Pointer of buffer to store the encoded data
    size_t* bufLen,                 ///< [IN/OUT] Size of the buffer available
    le_cbor_SemanticTag_t value     ///< [IN] TagID to be encoded
)
{
    if ((bufferPtr == NULL) || (*bufferPtr == NULL) ||
        (bufLen == NULL) || (*bufLen < LE_CBOR_SEMANTIC_TAG_MAX_SIZE))
    {
        return false;
    }

    LE_CBOR_PACK_TINY_ITEM(_LE_CBOR_SEMANTIC_TAG, _LE_CBOR_COMPLEX_THRESHOLD + 1);
    value = htobe16(value);
    LE_CBOR_PACK_SIMPLE_BUFFER(&value, sizeof(le_cbor_SemanticTag_t));

    *bufLen -= LE_CBOR_SEMANTIC_TAG_MAX_SIZE;

    return true;
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
        case (_LE_CBOR_SEMANTIC_TAG): ret = LE_CBOR_TYPE_SEMANTIC_TAG; break;
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
            *valuePtr = (sign)? additional: (-1 * additional - 1);
        }
        else
        {
            length = (additional == _LE_CBOR_COMPLEX_THRESHOLD)?1:
                2 << (additional-_LE_CBOR_COMPLEX_THRESHOLD-1);
            LE_CBOR_UNPACK_SIMPLE_BUFFER(((uint8_t*)valuePtr) + sizeof(int64_t)-length, length);
            *valuePtr = be64toh(*valuePtr);
            *valuePtr = (sign)? *valuePtr: (-1 * (*valuePtr) - 1);
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
 * Decode TagID from a buffer, increment the buffer pointer if decoding is successful
 *
 * @return
 *      - true      if successfully decoded
 *      - false     failed to decode
 */
//--------------------------------------------------------------------------------------------------
bool le_cbor_DecodeSemanticTag
(
    uint8_t** bufferPtr,                ///< [IN/OUT] Pointer of buffer for decoding
    le_cbor_SemanticTag_t* tagIdPtr     ///< [OUT] Decoded value
)
{
    uint64_t value = 0 ;

    if (bufferPtr == NULL || *bufferPtr == NULL || tagIdPtr == NULL)
    {
        return false;
    }

    if (DecodePositiveInteger(bufferPtr, &value, _LE_CBOR_SEMANTIC_TAG))
    {
        *tagIdPtr = (le_cbor_SemanticTag_t)value;
        return true;
    }

    return false;
}
