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
// This is a simple list of functions which should be defined here.
// For a description, see the comments with the implementation in le_pack.h
//
LE_DEFINE_INLINE le_pack_Type_t le_pack_GetType(uint8_t* buffer, ssize_t* additionalBytes);

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


LE_DEFINE_INLINE bool le_pack_UnpackUint8(uint8_t** bufferPtr, uint8_t* valuePtr);
LE_DEFINE_INLINE bool le_pack_UnpackUint16(uint8_t** bufferPtr, uint16_t* valuePtr);
LE_DEFINE_INLINE bool le_pack_UnpackUint32(uint8_t** bufferPtr, uint32_t* valuePtr);
LE_DEFINE_INLINE bool le_pack_UnpackUint64(uint8_t** bufferPtr, uint64_t* valuePtr);
LE_DEFINE_INLINE bool le_pack_UnpackInt8(uint8_t** bufferPtr, int8_t* valuePtr);
LE_DEFINE_INLINE bool le_pack_UnpackInt16(uint8_t** bufferPtr, int16_t* valuePtr);
LE_DEFINE_INLINE bool le_pack_UnpackInt32(uint8_t** bufferPtr, int32_t* valuePtr);
LE_DEFINE_INLINE bool le_pack_UnpackInt64(uint8_t** bufferPtr, int64_t* valuePtr);
LE_DEFINE_INLINE bool le_pack_UnpackSize(uint8_t **bufferPtr, size_t* valuePtr);
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
LE_DEFINE_INLINE void _le_pack_packNegativeInteger(uint8_t **bufferPtr, uint64_t value);
LE_DEFINE_INLINE void _le_pack_packPositiveInteger(uint8_t **bufferPtr, uint64_t value);

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
