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
    size_t sz = LE_PACK_CBOR_BUFFER_LENGTH;
    return le_cbor_EncodeSemanticTag(bufferPtr, &sz, value);
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
    return le_cbor_DecodeSemanticTag(bufferPtr, tagIdPtr);
}

#else
LE_DEFINE_INLINE bool le_pack_PackSemanticTag(uint8_t **bufferPtr, le_pack_SemanticTag_t value);
LE_DEFINE_INLINE bool le_pack_UnpackSemanticTag(uint8_t **bufferPtr,
        le_pack_SemanticTag_t *tagIdPtr);
#endif


#ifdef LE_CONFIG_RPC

bool le_pack_PackResult_rpc
(
    uint8_t** bufferPtr,
    le_result_t value
)
{
    size_t sz = LE_PACK_CBOR_BUFFER_LENGTH;
    return le_cbor_EncodeInteger(bufferPtr, &sz, value);
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
    size_t sz = LE_PACK_CBOR_BUFFER_LENGTH;

    result = le_cbor_EncodeSemanticTag(bufferPtr, &sz, tagId);
    if (result)
    {
        result = le_cbor_EncodeArrayHeader(bufferPtr, &sz, 2);
    }
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
    size_t sz = LE_PACK_CBOR_BUFFER_LENGTH;

    result = le_cbor_EncodeSemanticTag(bufferPtr, &sz, tagId);
    if (result)
    {
        result = le_cbor_EncodeArrayHeader(bufferPtr, &sz, 2);
    }
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

bool le_pack_UnpackResult_rpc
(
    uint8_t** bufferPtr,
    le_result_t* valuePtr
)
{
    bool result;
    int64_t  tmpValue = 0;
    result = le_cbor_DecodeInteger(bufferPtr, &tmpValue);
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
    result = le_cbor_DecodeUint64(bufferPtr, &tmpValue);
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
    le_cbor_DecodeSemanticTag(bufferPtr, &tagId);
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

bool le_pack_UnpackSizeUint32Tuple_rpc
(
    uint8_t** bufferPtr,
    size_t* sizePtr,
    uint32_t* valuePtr,
    le_pack_SemanticTag_t* semanticTagPtr
)
{
    le_pack_SemanticTag_t tagId;
    le_cbor_DecodeSemanticTag(bufferPtr, &tagId);
    if (tagId != LE_PACK_IN_STRING_POINTER && tagId != LE_PACK_OUT_STRING_POINTER &&
        tagId != LE_PACK_IN_BYTE_STR_POINTER && tagId != LE_PACK_OUT_BYTE_STR_POINTER)
    {
        return false;
    }

    if (semanticTagPtr)
    {
        *semanticTagPtr = tagId;
    }
    size_t tupleCount = 0;
    if (!le_cbor_DecodeArrayHeader(bufferPtr, &tupleCount) || tupleCount != 2)
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
    le_cbor_DecodeSemanticTag(bufferPtr, &tagId);
    if (tagId != LE_PACK_IN_STRING_POINTER && tagId != LE_PACK_OUT_STRING_POINTER &&
        tagId != LE_PACK_IN_BYTE_STR_POINTER && tagId != LE_PACK_OUT_BYTE_STR_POINTER)
    {
        return false;
    }

    if (semanticTagPtr)
    {
        *semanticTagPtr = tagId;
    }

    size_t tupleCount = 0;
    if (!le_cbor_DecodeArrayHeader(bufferPtr, &tupleCount) || tupleCount != 2)
    {
        return false;
    }
    le_pack_UnpackSize(bufferPtr, sizePtr);
    le_pack_UnpackUint64(bufferPtr, valuePtr);
    return true;
}
#endif
