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
LE_DEFINE_INLINE bool le_pack_PackString(uint8_t** bufferPtr,
                                      const char *stringPtr,
                                      uint32_t maxStringCount);
LE_DEFINE_INLINE bool le_pack_PackArrayHeader(uint8_t **bufferPtr,
                                           const void *arrayPtr,
                                           size_t elementSize,
                                           size_t arrayCount,
                                           size_t arrayMaxCount);

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
LE_DEFINE_INLINE bool le_pack_UnpackReference(uint8_t** bufferPtr, void* refPtr);
LE_DEFINE_INLINE bool le_pack_UnpackString(uint8_t** bufferPtr,
                                        char *stringPtr,
                                        uint32_t bufferSize,
                                        uint32_t maxStringCount);
LE_DEFINE_INLINE bool le_pack_UnpackArrayHeader(uint8_t **bufferPtr,
                                             const void *arrayPtr,
                                             size_t elementSize,
                                             size_t *arrayCountPtr,
                                             size_t arrayMaxCount);

#ifdef LE_CONFIG_RPC
LE_DEFINE_INLINE bool le_pack_PackTagID(uint8_t** bufferPtr, TagID_t value);
LE_DEFINE_INLINE bool le_pack_PackTaggedUint8(uint8_t** bufferPtr, uint8_t value, TagID_t tagId);
LE_DEFINE_INLINE bool le_pack_PackTaggedUint16(uint8_t** bufferPtr,
                                               uint16_t value,
                                               TagID_t tagId);
LE_DEFINE_INLINE bool le_pack_PackTaggedUint32(uint8_t** bufferPtr,
                                               uint32_t value,
                                               TagID_t tagId);
LE_DEFINE_INLINE bool le_pack_PackTaggedUint64(uint8_t** bufferPtr,
                                               uint64_t value,
                                               TagID_t tagId);
LE_DEFINE_INLINE bool le_pack_PackTaggedInt8(uint8_t** bufferPtr,
                                             int8_t value,
                                             TagID_t tagId);
LE_DEFINE_INLINE bool le_pack_PackTaggedInt16(uint8_t** bufferPtr,
                                              int16_t value,
                                              TagID_t tagId);
LE_DEFINE_INLINE bool le_pack_PackTaggedInt32(uint8_t** bufferPtr,
                                              int32_t value,
                                              TagID_t tagId);
LE_DEFINE_INLINE bool le_pack_PackTaggedInt64(uint8_t** bufferPtr,
                                              int64_t value,
                                              TagID_t tagId);
LE_DEFINE_INLINE bool le_pack_PackTaggedUint32Tuple(uint8_t** bufferPtr,
                                                    size_t size,
                                                    uint32_t value,
                                                    TagID_t tagId);
LE_DEFINE_INLINE bool le_pack_PackTaggedUint64Tuple(uint8_t** bufferPtr,
                                                    size_t size,
                                                    uint64_t value,
                                                    TagID_t tagId);
LE_DEFINE_INLINE bool le_pack_PackTaggedSize(uint8_t **bufferPtr,
                                             size_t value,
                                             TagID_t tagId);
LE_DEFINE_INLINE bool le_pack_PackTaggedBool(uint8_t** bufferPtr, bool value, TagID_t tagId);
LE_DEFINE_INLINE bool le_pack_PackTaggedChar(uint8_t** bufferPtr, char value, TagID_t tagId);
LE_DEFINE_INLINE bool le_pack_PackTaggedDouble(uint8_t** bufferPtr,
                                               double value,
                                               TagID_t tagId);
LE_DEFINE_INLINE bool le_pack_PackTaggedResult(uint8_t** bufferPtr,
                                               le_result_t value,
                                               TagID_t tagId);
LE_DEFINE_INLINE bool le_pack_PackTaggedOnOff(uint8_t** bufferPtr,
                                              le_onoff_t value,
                                              TagID_t tagId);
LE_DEFINE_INLINE bool le_pack_PackTaggedReference(uint8_t** bufferPtr,
                                                  const void* ref,
                                                  TagID_t tagId);
LE_DEFINE_INLINE bool le_pack_PackTaggedString(uint8_t** bufferPtr,
                                               const char *stringPtr,
                                               uint32_t maxStringCount,
                                               TagID_t tagId);
LE_DEFINE_INLINE bool le_pack_PackTaggedArrayHeader(uint8_t **bufferPtr,
                                                    const void *arrayPtr,
                                                    size_t elementSize,
                                                    size_t arrayCount,
                                                    size_t arrayMaxCount,
                                                    TagID_t tagId);

LE_DEFINE_INLINE bool le_pack_UnpackTagID(uint8_t** bufferPtr, TagID_t* tagIdPtr);
LE_DEFINE_INLINE bool le_pack_UnpackUint32Tuple(uint8_t** bufferPtr,
                                                size_t* sizePtr,
                                                uint32_t* valuePtr);
LE_DEFINE_INLINE bool le_pack_UnpackUint64Tuple(uint8_t** bufferPtr,
                                                size_t* sizePtr,
                                                uint64_t* valuePtr);
#endif