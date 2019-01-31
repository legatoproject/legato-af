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
