/**
 * @file le_pack.c
 *
 * Definitions for all le_pack functions.  Causes the compiler to emit definitions which will
 * be used if the function cannot be inlined.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"

//
// This is a simple list of inline functions which should be defined here.
// For a description, see the comments with the implementation in le_pack.h
//

LE_DEFINE_INLINE bool le_pack_PackUint8(uint8_t** bufferPtr, uint8_t value);
LE_DEFINE_INLINE bool le_pack_PackUint16(uint8_t** bufferPtr, uint16_t value);
LE_DEFINE_INLINE bool le_pack_PackUint32(uint8_t** bufferPtr, uint32_t value);
LE_DEFINE_INLINE bool le_pack_PackUint64(uint8_t** bufferPtr, uint64_t value);
LE_DEFINE_INLINE bool le_pack_PackInt8(uint8_t** bufferPtr, int8_t value);
LE_DEFINE_INLINE bool le_pack_PackInt16(uint8_t** bufferPtr, int16_t value);
LE_DEFINE_INLINE bool le_pack_PackInt32(uint8_t** bufferPtr, int32_t value);
LE_DEFINE_INLINE bool le_pack_PackInt64(uint8_t** bufferPtr, int64_t value);
LE_DEFINE_INLINE bool le_pack_PackSize(uint8_t **bufferPtr, size_t value);
LE_DEFINE_INLINE bool le_pack_PackBool(uint8_t** bufferPtr, bool value);
LE_DEFINE_INLINE bool le_pack_PackChar(uint8_t** bufferPtr, char value);
LE_DEFINE_INLINE bool le_pack_PackDouble(uint8_t** bufferPtr, double value);
LE_DEFINE_INLINE bool le_pack_PackResult(uint8_t** bufferPtr, le_result_t value);
LE_DEFINE_INLINE bool le_pack_PackOnOff(uint8_t** bufferPtr, le_onoff_t value);
LE_DEFINE_INLINE bool le_pack_PackReference(uint8_t** bufferPtr, const void* ref);
LE_DEFINE_INLINE bool le_pack_PackStringHeader(uint8_t** bufferPtr, size_t stringLen);
LE_DEFINE_INLINE bool le_pack_PackString(uint8_t** bufferPtr,
                                         const char *stringPtr,
                                         uint32_t maxStringCount);
LE_DEFINE_INLINE bool le_pack_PackArrayHeader(uint8_t **bufferPtr,
                                              const void *arrayPtr,
                                              size_t elementSize,
                                              size_t arrayCount,
                                              size_t arrayMaxCount);
LE_DEFINE_INLINE bool le_pack_PackByteStringHeader(uint8_t** bufferPtr, size_t length);
LE_DEFINE_INLINE bool le_pack_PackByteString(uint8_t** bufferPtr, const void* byteStringPtr,
                                             uint32_t byteStringCount);
LE_DEFINE_INLINE bool le_pack_PackIndefArrayHeader(uint8_t** bufferPtr);
LE_DEFINE_INLINE bool le_pack_PackEndOfIndefArray(uint8_t** bufferPtr);
LE_DEFINE_INLINE bool le_pack_PackSemanticTag(uint8_t** bufferPtr, le_pack_SemanticTag_t value);
LE_DEFINE_INLINE bool le_pack_PackTaggedUint8(uint8_t** bufferPtr, uint8_t value,
                                              le_pack_SemanticTag_t tagId);
LE_DEFINE_INLINE bool le_pack_PackTaggedUint16(uint8_t** bufferPtr,
                                               uint16_t value,
                                               le_pack_SemanticTag_t tagId);
LE_DEFINE_INLINE bool le_pack_PackTaggedUint32(uint8_t** bufferPtr,
                                               uint32_t value,
                                               le_pack_SemanticTag_t tagId);
LE_DEFINE_INLINE bool le_pack_PackTaggedUint64(uint8_t** bufferPtr,
                                               uint64_t value,
                                               le_pack_SemanticTag_t tagId);
LE_DEFINE_INLINE bool le_pack_PackTaggedInt8(uint8_t** bufferPtr,
                                             int8_t value,
                                             le_pack_SemanticTag_t tagId);
LE_DEFINE_INLINE bool le_pack_PackTaggedInt16(uint8_t** bufferPtr,
                                              int16_t value,
                                              le_pack_SemanticTag_t tagId);
LE_DEFINE_INLINE bool le_pack_PackTaggedInt32(uint8_t** bufferPtr,
                                              int32_t value,
                                              le_pack_SemanticTag_t tagId);
LE_DEFINE_INLINE bool le_pack_PackTaggedInt64(uint8_t** bufferPtr,
                                              int64_t value,
                                              le_pack_SemanticTag_t tagId);
LE_DEFINE_INLINE bool le_pack_PackTaggedSizeUint32Tuple(uint8_t** bufferPtr,
                                                        size_t size,
                                                        uint32_t value,
                                                       le_pack_SemanticTag_t tagId);
LE_DEFINE_INLINE bool le_pack_PackTaggedSizeUint64Tuple(uint8_t** bufferPtr,
                                                        size_t size,
                                                        uint64_t value,
                                                        le_pack_SemanticTag_t tagId);
LE_DEFINE_INLINE bool le_pack_PackTaggedSize(uint8_t **bufferPtr,
                                             size_t value,
                                             le_pack_SemanticTag_t tagId);
LE_DEFINE_INLINE bool le_pack_PackTaggedBool(uint8_t** bufferPtr, bool value,
                                             le_pack_SemanticTag_t tagId);
LE_DEFINE_INLINE bool le_pack_PackTaggedChar(uint8_t** bufferPtr, char value,
                                             le_pack_SemanticTag_t tagId);
LE_DEFINE_INLINE bool le_pack_PackTaggedDouble(uint8_t** bufferPtr,
                                               double value,
                                               le_pack_SemanticTag_t tagId);
LE_DEFINE_INLINE bool le_pack_PackTaggedResult(uint8_t** bufferPtr,
                                               le_result_t value,
                                               le_pack_SemanticTag_t tagId);
LE_DEFINE_INLINE bool le_pack_PackTaggedOnOff(uint8_t** bufferPtr,
                                              le_onoff_t value,
                                              le_pack_SemanticTag_t tagId);
LE_DEFINE_INLINE bool le_pack_PackTaggedReference(uint8_t** bufferPtr,
                                                  const void* ref,
                                                  le_pack_SemanticTag_t tagId);
LE_DEFINE_INLINE bool le_pack_PackTaggedString(uint8_t** bufferPtr,
                                               const char *stringPtr,
                                               uint32_t maxStringCount,
                                               le_pack_SemanticTag_t tagId);
LE_DEFINE_INLINE bool le_pack_PackTaggedArrayHeader(uint8_t **bufferPtr,
                                                    const void *arrayPtr,
                                                    size_t elementSize,
                                                    size_t arrayCount,
                                                    size_t arrayMaxCount,
                                                    le_pack_SemanticTag_t tagId);
LE_DEFINE_INLINE bool le_pack_PackTaggedSizePointerTuple(uint8_t** bufferPtr,
                                                         size_t size,
                                                         void* value,
                                                         le_pack_SemanticTag_t tagId);
LE_DEFINE_INLINE bool le_pack_PackTaggedIndefArrayHeader(uint8_t** bufferPtr,
                                                         le_pack_SemanticTag_t tagId);
LE_DEFINE_INLINE bool le_pack_PackTaggedByteString(uint8_t** bufferPtr, void* byteStringPtr,
                                                   uint32_t byteStringCount);

LE_DEFINE_INLINE bool le_pack_CheckSemanticTag(uint8_t** bufferPtr,
                                                le_pack_SemanticTag_t expectedTagId);
LE_DEFINE_INLINE bool le_pack_UnpackUint8(uint8_t** bufferPtr, uint8_t* valuePtr);
LE_DEFINE_INLINE bool le_pack_UnpackUint16(uint8_t** bufferPtr, uint16_t* valuePtr);
LE_DEFINE_INLINE bool le_pack_UnpackUint32(uint8_t** bufferPtr, uint32_t* valuePtr);
LE_DEFINE_INLINE bool le_pack_UnpackUint64(uint8_t** bufferPtr, uint64_t* valuePtr);
LE_DEFINE_INLINE bool le_pack_UnpackInt8(uint8_t** bufferPtr, int8_t* valuePtr);
LE_DEFINE_INLINE bool le_pack_UnpackInt16(uint8_t** bufferPtr, int16_t* valuePtr);
LE_DEFINE_INLINE bool le_pack_UnpackInt32(uint8_t** bufferPtr, int32_t* valuePtr);
LE_DEFINE_INLINE bool le_pack_UnpackInt64(uint8_t** bufferPtr, int64_t* valuePtr);
LE_DEFINE_INLINE bool le_pack_UnpackSize(uint8_t **bufferPtr, size_t* valuePtr);
LE_DEFINE_INLINE bool le_pack_UnpackTaggedUint8(uint8_t** bufferPtr, uint8_t* valuePtr,
                                                le_pack_SemanticTag_t tagId);
LE_DEFINE_INLINE bool le_pack_UnpackTaggedUint16(uint8_t** bufferPtr, uint16_t* valuePtr,
                                                 le_pack_SemanticTag_t tagId);
LE_DEFINE_INLINE bool le_pack_UnpackTaggedUint32(uint8_t** bufferPtr, uint32_t* valuePtr,
                                                 le_pack_SemanticTag_t tagId);
LE_DEFINE_INLINE bool le_pack_UnpackTaggedUint64(uint8_t** bufferPtr, uint64_t* valuePtr,
                                                 le_pack_SemanticTag_t tagId);
LE_DEFINE_INLINE bool le_pack_UnpackTaggedInt8(uint8_t** bufferPtr, int8_t* valuePtr,
                                               le_pack_SemanticTag_t tagId);
LE_DEFINE_INLINE bool le_pack_UnpackTaggedInt16(uint8_t** bufferPtr, int16_t* valuePtr,
                                                le_pack_SemanticTag_t tagId);
LE_DEFINE_INLINE bool le_pack_UnpackTaggedInt32(uint8_t** bufferPtr, int32_t* valuePtr,
                                                le_pack_SemanticTag_t tagId);
LE_DEFINE_INLINE bool le_pack_UnpackTaggedInt64(uint8_t** bufferPtr, int64_t* valuePtr,
                                                le_pack_SemanticTag_t tagId);
LE_DEFINE_INLINE bool le_pack_UnpackTaggedSize(uint8_t **bufferPtr, size_t* valuePtr,
                                               le_pack_SemanticTag_t tagId);
LE_DEFINE_INLINE bool le_pack_UnpackBool(uint8_t** bufferPtr, bool* valuePtr);
LE_DEFINE_INLINE bool le_pack_UnpackChar(uint8_t** bufferPtr, char* valuePtr);
LE_DEFINE_INLINE bool le_pack_UnpackDouble(uint8_t** bufferPtr, double* valuePtr);
LE_DEFINE_INLINE bool le_pack_UnpackResult(uint8_t** bufferPtr, le_result_t* valuePtr);
LE_DEFINE_INLINE bool le_pack_UnpackOnOff(uint8_t** bufferPtr, le_onoff_t* valuePtr);
LE_DEFINE_INLINE bool le_pack_UnpackReference(uint8_t** bufferPtr, void* refPtr,
                                              le_pack_SemanticTag_t* semanticTagPtr);
LE_DEFINE_INLINE bool le_pack_UnpackString(uint8_t** bufferPtr, char *stringPtr,
                                           uint32_t bufferSize,
                                           uint32_t maxStringCount);
LE_DEFINE_INLINE bool le_pack_UnpackStringHeader(uint8_t** bufferPtr, size_t* stringSizePtr);
LE_DEFINE_INLINE bool le_pack_UnpackArrayHeader(uint8_t **bufferPtr,
                                                const void *arrayPtr,
                                                size_t elementSize,
                                                size_t *arrayCountPtr,
                                                size_t arrayMaxCount);
LE_DEFINE_INLINE bool le_pack_UnpackIndefArrayHeader(uint8_t** bufferPtr);
LE_DEFINE_INLINE bool le_pack_UnpackEndOfIndefArray(uint8_t** bufferPtr);
LE_DEFINE_INLINE bool le_pack_UnpackByteString(uint8_t** bufferPtr, void *arrayPtr,
                                               size_t *arrayCountPtr, size_t arrayMaxCount);
LE_DEFINE_INLINE bool le_pack_UnpackByteStringHeader(uint8_t** bufferPtr, size_t* lengthPtr);
LE_DEFINE_INLINE bool le_pack_UnpackSemanticTag(uint8_t** bufferPtr,
                                                le_pack_SemanticTag_t* tagIdPtr);
LE_DEFINE_INLINE bool le_pack_UnpackSizeUint32Tuple(uint8_t** bufferPtr,
                                                    size_t* sizePtr,
                                                    uint32_t* valuePtr,
                                                    le_pack_SemanticTag_t* semanticTagPtr);
LE_DEFINE_INLINE bool le_pack_UnpackSizeUint64Tuple(uint8_t** bufferPtr,
                                                    size_t* sizePtr,
                                                    uint64_t* valuePtr,
                                                    le_pack_SemanticTag_t* semanticTagPtr);
LE_DEFINE_INLINE bool le_pack_UnpackSizePointerTuple(uint8_t** bufferPtr,
                                                     size_t* sizePtr,
                                                     void** valuePtr,
                                                     le_pack_SemanticTag_t* semanticTagPtr);

//--------------------------------------------------------------------------------------------------
/**
 * Private functions:
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pack an integer:
 */
//--------------------------------------------------------------------------------------------------
void _le_pack_packInteger
(
    uint8_t **bufferPtr,
    uint64_t value,
    unsigned int major
)
{
    if (value <= UINT16_MAX)
    {
        if (value <= UINT8_MAX)
        {
            if (value < _LE_PACK_CBOR_COMPLEX_THRESHOLD)
            {
                LE_PACK_PACK_TINY_ITEM(major, value);
            }
            else
            {
                uint8_t value2 = (uint8_t) value;
                LE_PACK_PACK_TINY_ITEM(major, _LE_PACK_CBOR_COMPLEX_THRESHOLD);
                LE_PACK_PACK_SIMPLE_BUFFER(&value2, 1);
            }
        }
        else
        {
            uint16_t value2 = htobe16((uint16_t)value);
            LE_PACK_PACK_TINY_ITEM(major, _LE_PACK_CBOR_COMPLEX_THRESHOLD + 1);
            LE_PACK_PACK_SIMPLE_BUFFER(&value2, 2);
        }
    }
    else
    {
        if (value <= UINT32_MAX)
        {
            uint32_t value2 = htobe32((uint32_t)value);
            LE_PACK_PACK_TINY_ITEM(major, _LE_PACK_CBOR_COMPLEX_THRESHOLD + 2);
            LE_PACK_PACK_SIMPLE_BUFFER(&value2, 4);
        }
        else
        {
            uint64_t value2 = htobe64((uint64_t)value);
            LE_PACK_PACK_TINY_ITEM(major, _LE_PACK_CBOR_COMPLEX_THRESHOLD + 3);
            LE_PACK_PACK_SIMPLE_BUFFER(&value2, 8);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Pack a negative integer
 */
//--------------------------------------------------------------------------------------------------
void _le_pack_packNegativeInteger
(
    uint8_t **bufferPtr,
    uint64_t value
)
{
    _le_pack_packInteger(bufferPtr, value, _LE_PACK_CBOR_NEG_INTEGER);
}

//--------------------------------------------------------------------------------------------------
/**
 * Pack a positive integer
 */
//--------------------------------------------------------------------------------------------------
void _le_pack_packPositiveInteger
(
    uint8_t **bufferPtr,
    uint64_t value
)
{
    _le_pack_packInteger(bufferPtr, value, _LE_PACK_CBOR_POS_INTEGER);
}

//--------------------------------------------------------------------------------------------------
/**
 *  unpack integer(may be positive or negative)
 */
//--------------------------------------------------------------------------------------------------
bool _le_pack_unpackInteger
(
    uint8_t** bufferPtr,
    int64_t* valuePtr
)
{
    int additional;
    unsigned int major;
    LE_PACK_UNPACK_TINY_ITEM();
    if (major != _LE_PACK_CBOR_POS_INTEGER && major != _LE_PACK_CBOR_NEG_INTEGER)
    {
       return false;
    }
    else
    {
        bool sign = (major == _LE_PACK_CBOR_POS_INTEGER);
        int length;
        if (additional < _LE_PACK_CBOR_COMPLEX_THRESHOLD)
        {
            *valuePtr = (sign)? additional: (-1 * additional - 1);
        }
        else
        {
            length = (additional == _LE_PACK_CBOR_COMPLEX_THRESHOLD)?1:
                2 << (additional-_LE_PACK_CBOR_COMPLEX_THRESHOLD-1);
            LE_PACK_UNPACK_SIMPLE_BUFFER(((uint8_t*)valuePtr) + sizeof(int64_t)-length, length);
            *valuePtr = be64toh(*valuePtr);
            *valuePtr = (sign)? *valuePtr: (-1 * (*valuePtr) - 1);
        }
        return true;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 *  unpack positive integer
 */
//--------------------------------------------------------------------------------------------------
bool  _le_pack_unpackPositiveInteger
(
    uint8_t** bufferPtr,
    uint64_t* valuePtr,
    unsigned int expectedMajor
)
{
    unsigned int additional;
    unsigned int major;
    LE_PACK_UNPACK_TINY_ITEM();
    if (major != expectedMajor)
    {
       return false;
    }
    else
    {
        if (additional < _LE_PACK_CBOR_COMPLEX_THRESHOLD)
        {
            *valuePtr = additional;
        }
        else
        {
            int length = (additional == _LE_PACK_CBOR_COMPLEX_THRESHOLD)?1:
                2 << (additional-_LE_PACK_CBOR_COMPLEX_THRESHOLD-1);
            LE_PACK_UNPACK_SIMPLE_BUFFER(((uint8_t*)valuePtr) + sizeof(uint64_t)-length, length);
            *valuePtr = be64toh(*valuePtr);
        }
        return true;
    }
}

#if LE_CONFIG_RPC
//--------------------------------------------------------------------------------------------------
/**
 * Pack a TagID into a buffer, incrementing the buffer pointer and decrementing the
 * available size, as appropriate.
 *
 * @note By making this an inline function, gcc can often optimize out the size check if the buffer
 * size is known at compile time.
 */
//--------------------------------------------------------------------------------------------------
bool le_pack_PackSemanticTag
(
    uint8_t** bufferPtr,
    le_pack_SemanticTag_t value
)
{
    LE_PACK_PACK_TINY_ITEM(_LE_PACK_CBOR_SEMANTIC_TAG, _LE_PACK_CBOR_COMPLEX_THRESHOLD + 1);
    value = htobe16(value);
    LE_PACK_PACK_SIMPLE_BUFFER(&value, sizeof(le_pack_SemanticTag_t));
    return true;
}

//--------------------------------------------------------------------------------------------------
/**
 * Unpack a TagID from a buffer, incrementing the buffer pointer and decrementing the
 * available size, as appropriate.
 *
 * @note By making this an inline function, gcc can often optimize out the size check if the buffer
 * size is known at compile time.
 */
//--------------------------------------------------------------------------------------------------
bool le_pack_UnpackSemanticTag
(
    uint8_t** bufferPtr,
    le_pack_SemanticTag_t* tagIdPtr
)
{
    uint64_t value = 0 ;
    _le_pack_unpackPositiveInteger(bufferPtr, &value, _LE_PACK_CBOR_SEMANTIC_TAG);
    *tagIdPtr = (le_pack_SemanticTag_t) value;
    return true;
}
#else
LE_DEFINE_INLINE bool le_pack_PackSemanticTag(uint8_t **bufferPtr, le_pack_SemanticTag_t value);
LE_DEFINE_INLINE bool le_pack_UnpackSemanticTag(uint8_t **bufferPtr,
        le_pack_SemanticTag_t *tagIdPtr);
#endif


//--------------------------------------------------------------------------------------------------
/**
 * Get type.
 *
 * @return
 *      - an le_pack_Type_t enum.
 */
//--------------------------------------------------------------------------------------------------
le_pack_Type_t le_pack_GetType
(
    uint8_t* buffer,                      ///< [IN] buffer pointer
    ssize_t* additionalBytes         ///< [OUT] number of additional bytes
)
{
    unsigned int major = (buffer[0] >> 5) & 0x7;
    unsigned int additional = buffer[0] & 0x1F;

    if (additional < _LE_PACK_CBOR_COMPLEX_THRESHOLD)
    {
        *additionalBytes = 0;
    }
    else if (additional < _LE_PACK_CBOR_PRIMITIVE_INDEFINITE)
    {
        *additionalBytes = (additional == _LE_PACK_CBOR_COMPLEX_THRESHOLD)?1:
            2 << (additional-_LE_PACK_CBOR_COMPLEX_THRESHOLD-1);
    }
    else
    {
        *additionalBytes = -1;
    }
    le_pack_Type_t ret;
    switch(major)
    {
        case (_LE_PACK_CBOR_POS_INTEGER):  ret = LE_PACK_TYPE_POS_INTEGER;  break;
        case (_LE_PACK_CBOR_NEG_INTEGER):  ret = LE_PACK_TYPE_NEG_INTEGER;  break;
        case (_LE_PACK_CBOR_BYTE_STRING):  ret = LE_PACK_TYPE_BYTE_STRING;  break;
        case (_LE_PACK_CBOR_TEXT_STRING):  ret = LE_PACK_TYPE_TEXT_STRING;  break;
        case (_LE_PACK_CBOR_ITEM_ARRAY):   ret = LE_PACK_TYPE_ITEM_ARRAY;   break;
        case (_LE_PACK_CBOR_SEMANTIC_TAG): ret = LE_PACK_TYPE_SEMANTIC_TAG; break;
        case (_LE_PACK_CBOR_PRIMITVE):
        {
            if (additional == _LE_PACK_CBOR_PRIMITIVE_TRUE ||
                additional == _LE_PACK_CBOR_PRIMITIVE_FALSE)
            {
                ret = LE_PACK_TYPE_BOOLEAN;
            }
            else if (additional == _LE_PACK_CBOR_PRIMITIVE_DOUBLE)
            {
                ret = LE_PACK_TYPE_DOUBLE;
            }
            else if (additional == _LE_PACK_CBOR_PRIMITIVE_BREAK)
            {
                ret = LE_PACK_TYPE_INDEF_END;
            }
            else
            {
                ret = LE_PACK_TYPE_INVALID_TYPE;
            }
            break;
        }
        default:
            ret = LE_PACK_TYPE_INVALID_TYPE;
    }
    return ret;
}


#ifdef LE_CONFIG_RPC
//--------------------------------------------------------------------------------------------------
/**
 *  implementation of cbor specific functions:
 */
//--------------------------------------------------------------------------------------------------

bool le_pack_PackInt8_rpc
(
    uint8_t** bufferPtr,
    int8_t value
)
{
    if (value >= 0)
    {
        _le_pack_packPositiveInteger(bufferPtr, value);
    }
    else
    {
        _le_pack_packNegativeInteger(bufferPtr, -1 * value - 1);
    }
    return true;
}

bool le_pack_PackInt16_rpc
(
    uint8_t** bufferPtr,
    int16_t value
)
{
    if (value >= 0)
    {
        _le_pack_packPositiveInteger(bufferPtr, value);
    }
    else
    {
        _le_pack_packNegativeInteger(bufferPtr, -1 * value - 1);
    }
    return true;
}

bool le_pack_PackInt32_rpc
(
    uint8_t** bufferPtr,
    int32_t value
)
{
    if (value >= 0)
    {
        _le_pack_packPositiveInteger(bufferPtr, value);
    }
    else
    {
        _le_pack_packNegativeInteger(bufferPtr, -1 * value - 1);
    }
    return true;
}

bool le_pack_PackInt64_rpc
(
    uint8_t** bufferPtr,
    int64_t value
)
{
    if (value >= 0)
    {
        _le_pack_packPositiveInteger(bufferPtr, value);
    }
    else
    {
        _le_pack_packNegativeInteger(bufferPtr, -1 * value - 1);
    }
    return true;
}

bool le_pack_PackBool_rpc
(
    uint8_t** bufferPtr,
    bool value
)
{
    LE_PACK_PACK_TINY_ITEM(_LE_PACK_CBOR_PRIMITVE, _LE_PACK_CBOR_PRIMITIVE_FALSE + (value ? 1 : 0));
    return true;
}

bool le_pack_PackDouble_rpc
(
    uint8_t** bufferPtr,
    double value
)
{
    uint64_t    intValue;
    memcpy(&intValue, &value, sizeof(intValue));
    intValue = htobe64(intValue);
    LE_PACK_PACK_TINY_ITEM(_LE_PACK_CBOR_PRIMITVE, _LE_PACK_CBOR_PRIMITIVE_DOUBLE);
    LE_PACK_PACK_SIMPLE_BUFFER(&intValue, sizeof(intValue));
    return true;
}

bool le_pack_PackResult_rpc
(
    uint8_t** bufferPtr,
    le_result_t value
)
{
    if (value >= 0)
    {
        _le_pack_packPositiveInteger(bufferPtr, value);
    }
    else
    {
        _le_pack_packNegativeInteger(bufferPtr, -1 * value - 1);
    }
    return true;
}


bool le_pack_PackTaggedSizeUint32Tuple_rpc
(
    uint8_t** bufferPtr,
    size_t size,
    uint32_t value,
    le_pack_SemanticTag_t tagId
)
{
    bool result;
    result = le_pack_PackSemanticTag(bufferPtr, tagId);
    LE_PACK_PACK_TINY_ITEM(_LE_PACK_CBOR_ITEM_ARRAY, 2);
    if (result)
    {
        result = le_pack_PackSize(bufferPtr, size);
    }
    if (result)
    {
        result = le_pack_PackUint32(bufferPtr, value);
    }
    return result;
}

bool le_pack_PackTaggedSizeUint64Tuple_rpc
(
    uint8_t** bufferPtr,
    size_t size,
    uint64_t value,
    le_pack_SemanticTag_t tagId
)
{
    bool result;
    result = le_pack_PackSemanticTag(bufferPtr, tagId);
    LE_PACK_PACK_TINY_ITEM(_LE_PACK_CBOR_ITEM_ARRAY, 2);
    if (result)
    {
        result = le_pack_PackSize(bufferPtr, size);
    }
    if (result)
    {
        result = le_pack_PackUint64(bufferPtr, value);
    }
    return result;
}

bool le_pack_PackString_rpc
(
    uint8_t** bufferPtr,
    const char *stringPtr,
    uint32_t maxStringCount
)
{
    if (!stringPtr)
    {
        return false;
    }

    uint32_t stringLen = strnlen(stringPtr, maxStringCount);
    // String was too long to fit in the buffer -- return false.
    if (stringPtr[stringLen] != '\0')
    {
        return false;
    }
    _le_pack_packInteger(bufferPtr, stringLen, _LE_PACK_CBOR_TEXT_STRING);

    LE_PACK_PACK_SIMPLE_BUFFER(stringPtr, stringLen);
    return true;
}

bool le_pack_PackIndefArrayHeader_rpc
(
    uint8_t** bufferPtr
)
{
    if (*bufferPtr == NULL)
    {
        return false;
    }
    else
    {
        LE_PACK_PACK_TINY_ITEM(_LE_PACK_CBOR_ITEM_ARRAY, _LE_PACK_CBOR_PRIMITIVE_INDEFINITE);
    }
    return true;
}

bool le_pack_PackEndOfIndefArray_rpc
(
        uint8_t** bufferPtr
)
{
    if (*bufferPtr == NULL)
    {
        return false;
    }
    else
    {
        LE_PACK_PACK_TINY_ITEM(_LE_PACK_CBOR_PRIMITVE, _LE_PACK_CBOR_PRIMITIVE_BREAK);
    }
    return true;
}


bool le_pack_UnpackUint8_rpc
(
    uint8_t** bufferPtr,
    uint8_t* valuePtr
)
{
    bool result;
    uint64_t tmpValue = 0;
    result = _le_pack_unpackPositiveInteger(bufferPtr, &tmpValue, _LE_PACK_CBOR_POS_INTEGER);
    if (!result || tmpValue > UINT8_MAX)
    {
        return false;
    }
    *valuePtr = tmpValue;
    return result;
}

bool le_pack_UnpackUint16_rpc
(
    uint8_t** bufferPtr,
    uint16_t* valuePtr
)
{
    bool result;
    uint64_t tmpValue = 0;
    result = _le_pack_unpackPositiveInteger(bufferPtr, &tmpValue, _LE_PACK_CBOR_POS_INTEGER);
    if (!result || tmpValue > UINT16_MAX)
    {
        return false;
    }
    *valuePtr = tmpValue;
    return result;
}

bool le_pack_UnpackUint32_rpc
(
    uint8_t** bufferPtr,
    uint32_t* valuePtr
)
{
    bool result;
    uint64_t tmpValue = 0;
    result = _le_pack_unpackPositiveInteger(bufferPtr, &tmpValue, _LE_PACK_CBOR_POS_INTEGER);
    if (!result || tmpValue > UINT32_MAX)
    {
        return false;
    }
    *valuePtr = tmpValue;
    return result;
}

bool le_pack_UnpackUint64_rpc
(
    uint8_t** bufferPtr,
    uint64_t* valuePtr
)
{
    bool result;
    uint64_t tmpValue = 0;
    result = _le_pack_unpackPositiveInteger(bufferPtr, &tmpValue, _LE_PACK_CBOR_POS_INTEGER);
    if (!result || tmpValue > UINT64_MAX)
    {
        return false;
    }
    *valuePtr = tmpValue;
    return result;
}

bool le_pack_UnpackInt8_rpc
(
    uint8_t** bufferPtr,
    int8_t* valuePtr
)
{
    bool result;
    int64_t  tmpValue = 0;
    result = _le_pack_unpackInteger(bufferPtr, &tmpValue);
    if (!result || tmpValue > INT8_MAX || tmpValue < INT8_MIN)
    {
        return false;
    }
    *valuePtr = tmpValue;
    return result;
}

bool le_pack_UnpackInt16_rpc
(
    uint8_t** bufferPtr,
    int16_t* valuePtr
)
{
    bool result;
    int64_t  tmpValue = 0;
    result = _le_pack_unpackInteger(bufferPtr, &tmpValue);
    if (!result || tmpValue > INT16_MAX || tmpValue < INT16_MIN)
    {
        return false;
    }
    *valuePtr = tmpValue;
    return result;
}

bool le_pack_UnpackInt32_rpc
(
    uint8_t** bufferPtr,
    int32_t* valuePtr
)
{
    bool result;
    int64_t  tmpValue = 0;
    result = _le_pack_unpackInteger(bufferPtr, &tmpValue);
    if (!result || tmpValue > INT32_MAX || tmpValue < INT32_MIN)
    {
        return false;
    }
    *valuePtr = tmpValue;
    return result;
}

bool le_pack_UnpackInt64_rpc
(
    uint8_t** bufferPtr,
    int64_t* valuePtr
)
{
    bool result;
    int64_t  tmpValue = 0;
    result = _le_pack_unpackInteger(bufferPtr, &tmpValue);
    if (!result || tmpValue > INT64_MAX || tmpValue < INT64_MIN)
    {
        return false;
    }
    *valuePtr = tmpValue;
    return result;
}

bool le_pack_UnpackBool_rpc
(
    uint8_t** bufferPtr,
    bool* valuePtr
)
{
    // Treat boolean as uint8_t for packing, regardless of underlying OS type.
    // Underlying type has been int on some platforms in the past.
    uint8_t simpleValue;

    memcpy(&simpleValue, *bufferPtr, sizeof(simpleValue));

    *bufferPtr = ((uint8_t* )*bufferPtr) + sizeof(simpleValue);

    if (simpleValue == (_LE_PACK_CBOR_PRIMITVE << 5 | _LE_PACK_CBOR_PRIMITIVE_FALSE))
    {
        *valuePtr = false;
    }
    else if (simpleValue == (_LE_PACK_CBOR_PRIMITVE << 5 | _LE_PACK_CBOR_PRIMITIVE_TRUE))
    {
        *valuePtr = true;
    }
    else
    {
        return false;
    }

    return true;
}

bool le_pack_UnpackChar_rpc
(
    uint8_t** bufferPtr,
    char* valuePtr
)
{
    bool result;
    uint64_t tmpValue = 0;
    result = _le_pack_unpackPositiveInteger(bufferPtr, &tmpValue, _LE_PACK_CBOR_POS_INTEGER);
    if (!result || tmpValue > UINT8_MAX)
    {
        return false;
    }
    *valuePtr = tmpValue;
    return result;
}

bool le_pack_UnpackDouble_rpc
(
    uint8_t** bufferPtr,
    double* valuePtr
)
{
    unsigned int major;
    unsigned int additional;
    uint64_t rawValue;
    LE_PACK_UNPACK_TINY_ITEM();
    if (major != _LE_PACK_CBOR_PRIMITVE || additional != _LE_PACK_CBOR_PRIMITIVE_DOUBLE)
    {
        return false;
    }
    else
    {
        LE_PACK_UNPACK_SIMPLE_BUFFER(&rawValue, sizeof(uint64_t));
        rawValue = be64toh(rawValue);
        memcpy(valuePtr, &rawValue, sizeof(double));
    }
    return true;
}

bool le_pack_UnpackResult_rpc
(
    uint8_t** bufferPtr,
    le_result_t* valuePtr
)
{
    bool result;
    int64_t  tmpValue = 0;
    result = _le_pack_unpackInteger(bufferPtr, &tmpValue);
    if (!result)
    {
        return false;
    }
    *valuePtr = (le_result_t)tmpValue;
    return result;
}

bool le_pack_UnpackOnOff_rpc
(
    uint8_t** bufferPtr,
    le_onoff_t* valuePtr
)
{
    bool result;
    uint64_t tmpValue = 0;
    result = _le_pack_unpackPositiveInteger(bufferPtr, &tmpValue, _LE_PACK_CBOR_POS_INTEGER);
    if (!result)
    {
        return false;
    }
    *valuePtr = (tmpValue == 0)? LE_OFF : LE_ON;
    return result;
}

bool le_pack_UnpackReference_rpc
(
    uint8_t** bufferPtr,
    void* refPtr,
    le_pack_SemanticTag_t* semanticTagPtr
)
{
    uint32_t refAsInt;

    le_pack_SemanticTag_t tagId;
    le_pack_UnpackSemanticTag(bufferPtr, &tagId);
    if (tagId != LE_PACK_REFERENCE &&
        tagId != LE_PACK_CONTEXT_PTR_REFERENCE &&
        tagId != LE_PACK_ASYNC_HANDLER_REFERENCE)
    {
        return false;
    }

    if (semanticTagPtr)
    {
        *semanticTagPtr = tagId;
    }
    if (!le_pack_UnpackUint32(bufferPtr, &refAsInt))
    {
        return false;
    }

    //  All references passed through an API must be safe references, so
    // 0-bit will be set.  Check that here to be safe.
    if ((refAsInt & 0x01) ||
        (!refAsInt))
    {
        // Double cast to avoid warnings.
        *(void **)refPtr = (void *)(size_t)refAsInt;
        return true;
    }
    else
    {
        return false;
    }
}

bool le_pack_UnpackString_rpc
(
    uint8_t** bufferPtr,
    char *stringPtr,
    uint32_t bufferSize,
    uint32_t maxStringCount
)
{
    uint32_t stringSize;

    // First get string size
    uint64_t value = 0;
    if (!_le_pack_unpackPositiveInteger(bufferPtr, &value, _LE_PACK_CBOR_TEXT_STRING))
    {
        return false;
    }
    else
    {
        stringSize = value;
    }
    if ((stringSize > maxStringCount) ||
        (stringSize > bufferSize))
    {
        return false;
    }

    if (!stringPtr)
    {
        // Only allow unpacking into no output buffer if the string is zero sized.
        // Otherwise an output buffer is required.
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

bool le_pack_UnpackStringHeader_rpc
(
    uint8_t** bufferPtr,
    size_t* stringSizePtr
)
{
    uint64_t value = 0;
    if (!_le_pack_unpackPositiveInteger(bufferPtr, &value, _LE_PACK_CBOR_TEXT_STRING))
    {
        return false;
    }
    else
    {
        *stringSizePtr = (size_t) value;
    }
    return true;
}

bool le_pack_UnpackArrayHeader_rpc
(
    uint8_t **bufferPtr,
    const void *arrayPtr,
    size_t elementSize,
    size_t *arrayCountPtr,
    size_t arrayMaxCount
)
{
    LE_UNUSED(elementSize);
    uint64_t    value = 0;
    _le_pack_unpackPositiveInteger(bufferPtr, &value, _LE_PACK_CBOR_ITEM_ARRAY);
    *arrayCountPtr = value;
    if (*arrayCountPtr > arrayMaxCount)
    {
        return false;
    }
    else if (!arrayPtr)
    {
        // Missing array pointer must match zero sized array.
        return (*arrayCountPtr == 0);
    }

    return true;
}

bool le_pack_UnpackIndefArrayHeader_rpc
(
    uint8_t** bufferPtr
)
{
    unsigned int major;
    unsigned int additional;
    LE_PACK_UNPACK_TINY_ITEM();
    if (major == _LE_PACK_CBOR_ITEM_ARRAY && additional == _LE_PACK_CBOR_PRIMITIVE_INDEFINITE)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool le_pack_UnpackEndOfIndefArray_rpc
(
    uint8_t** bufferPtr
)
{
    unsigned int major;
    unsigned int additional;
    LE_PACK_UNPACK_TINY_ITEM();
    if (major == _LE_PACK_CBOR_PRIMITVE && additional == _LE_PACK_CBOR_PRIMITIVE_BREAK)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool le_pack_UnpackByteStringHeader_rpc
(
    uint8_t** bufferPtr,
    size_t* lengthPtr
)
{
    uint64_t value = 0;
    if (!_le_pack_unpackPositiveInteger(bufferPtr, &value, _LE_PACK_CBOR_BYTE_STRING))
    {
        return false;
    }
    else
    {
        *lengthPtr = (size_t) value;
    }
    return true;
}

bool le_pack_UnpackByteString_rpc
(
    uint8_t** bufferPtr,
    void *arrayPtr,
    size_t *arrayCountPtr,
    size_t arrayMaxCount
)

{
    uint64_t value = 0;
    if (!_le_pack_unpackPositiveInteger(bufferPtr, &value, _LE_PACK_CBOR_BYTE_STRING))
    {
        return false;
    }
    else
    {
        *arrayCountPtr = (size_t) value;
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
        LE_PACK_UNPACK_SIMPLE_BUFFER(arrayPtr, *arrayCountPtr);
        return true;
    }
}



bool le_pack_UnpackSizeUint32Tuple_rpc
(
    uint8_t** bufferPtr,
    size_t* sizePtr,
    uint32_t* valuePtr,
    le_pack_SemanticTag_t* semanticTagPtr
)
{
    le_pack_SemanticTag_t tagId;
    le_pack_UnpackSemanticTag(bufferPtr, &tagId);
    if (tagId != LE_PACK_IN_STRING_POINTER && tagId != LE_PACK_OUT_STRING_POINTER &&
        tagId != LE_PACK_IN_BYTE_STR_POINTER && tagId != LE_PACK_OUT_BYTE_STR_POINTER)
    {
        return false;
    }

    if (semanticTagPtr)
    {
        *semanticTagPtr = tagId;
    }
    uint64_t tupleCount = 0;
    if (!_le_pack_unpackPositiveInteger(bufferPtr, &tupleCount, _LE_PACK_CBOR_ITEM_ARRAY) ||
                                        tupleCount != 2)
    {
        return false;
    }
    le_pack_UnpackSize(bufferPtr, sizePtr);
    le_pack_UnpackUint32(bufferPtr, valuePtr);
    return true;
}

bool le_pack_UnpackSizeUint64Tuple_rpc
(
    uint8_t** bufferPtr,
    size_t* sizePtr,
    uint64_t* valuePtr,
    le_pack_SemanticTag_t* semanticTagPtr
)
{
    le_pack_SemanticTag_t tagId;
    le_pack_UnpackSemanticTag(bufferPtr, &tagId);
    if (tagId != LE_PACK_IN_STRING_POINTER && tagId != LE_PACK_OUT_STRING_POINTER &&
        tagId != LE_PACK_IN_BYTE_STR_POINTER && tagId != LE_PACK_OUT_BYTE_STR_POINTER)
    {
        return false;
    }

    if (semanticTagPtr)
    {
        *semanticTagPtr = tagId;
    }

    uint64_t tupleCount = 0;
    if (!_le_pack_unpackPositiveInteger(bufferPtr, &tupleCount, _LE_PACK_CBOR_ITEM_ARRAY) ||
                                        tupleCount != 2)
    {
        return false;
    }
    le_pack_UnpackSize(bufferPtr, sizePtr);
    le_pack_UnpackUint64(bufferPtr, valuePtr);
    return true;
}
#endif
