/**
 * @page c_cbor CBOR string API
 *
 *
 * @subpage le_cbor.h "API Reference"
 *
 * <HR>
 *
 * A CBOR encoding and decoding interface for Legato.
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc.
 */


/** @file le_cbor.h
 *
 * Legato @ref c_cbor include file.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_CBOR_INCLUDE_GUARD
#define LEGATO_CBOR_INCLUDE_GUARD


// Callback prototypes
typedef void (*le_cbor_Uint8Handler_t)(void *, uint8_t);

typedef void (*le_cbor_Uint16Handler_t)(void *, uint16_t);

typedef void (*le_cbor_Uint32Handler_t)(void *, uint32_t);

typedef void (*le_cbor_Uint64Handler_t)(void *, uint64_t);

typedef void (*le_cbor_Int8Handler_t)(void *, int8_t);

typedef void (*le_cbor_Int16Handler_t)(void *, int16_t);

typedef void (*le_cbor_Int32Handler_t)(void *, int32_t);

typedef void (*le_cbor_Int64Handler_t)(void *, int64_t);

typedef void (*le_cbor_SimpleHandler_t)(void *);

typedef void (*le_cbor_StringHandler_t)(void *, const uint8_t*, size_t);

typedef void (*le_cbor_CollectionHandler_t)(void *, size_t);

typedef void (*le_cbor_FloatHandler_t)(void *, float);

typedef void (*le_cbor_DoubleHandler_t)(void *, double);

typedef void (*le_cbor_BoolHandler_t)(void *, bool);

// Callback handlers for decoding stream
typedef struct le_cbor_Handlers
{
  // Unsigned int
  le_cbor_Uint8Handler_t uint8;
  le_cbor_Uint16Handler_t uint16;
  le_cbor_Uint32Handler_t uint32;
  le_cbor_Uint64Handler_t uint64;

  // Negative int
  le_cbor_Int64Handler_t negInt64;
  le_cbor_Int32Handler_t negInt32;
  le_cbor_Int16Handler_t negInt16;
  le_cbor_Int8Handler_t negInt8;

  // Definite byte string
  le_cbor_SimpleHandler_t byteStringStart;

  // Indefinite byte string start
  le_cbor_StringHandler_t byteString;

  // Definite string
  le_cbor_StringHandler_t string;

  // Indefinite string start
  le_cbor_SimpleHandler_t stringStart;

  // Definite array
  le_cbor_SimpleHandler_t indefArrayStart;

  // Indefinite array
  le_cbor_CollectionHandler_t arrayStart;

  // Definite map
  le_cbor_SimpleHandler_t indefMapStart;

  // Indefinite map
  le_cbor_CollectionHandler_t mapStart;

  // Tags
  le_cbor_Uint64Handler_t tag;

  // Half float
  le_cbor_FloatHandler_t float2;

  // Single float
  le_cbor_FloatHandler_t float4;

  // Double float
  le_cbor_DoubleHandler_t float8;

  // Undef
  le_cbor_SimpleHandler_t undefined;

  // Null
  le_cbor_SimpleHandler_t null;

  // Bool
  le_cbor_BoolHandler_t boolean;

  // Indefinite item break
  le_cbor_SimpleHandler_t indefBreak;
}
le_cbor_Handlers_t;

// Tag type
typedef uint64_t le_cbor_Tag_t;

// Required sizes for encoding different types of data
#define LE_CBOR_UINT8_MAX_SIZE                  (1 + sizeof(uint8_t))
#define LE_CBOR_UINT16_MAX_SIZE                 (1 + sizeof(uint16_t))
#define LE_CBOR_UINT32_MAX_SIZE                 (1 + sizeof(uint32_t))
#define LE_CBOR_UINT64_MAX_SIZE                 (1 + sizeof(uint64_t))
#define LE_CBOR_INT8_MAX_SIZE                   (1 + sizeof(int8_t))
#define LE_CBOR_INT16_MAX_SIZE                  (1 + sizeof(int16_t))
#define LE_CBOR_INT32_MAX_SIZE                  (1 + sizeof(int32_t))
#define LE_CBOR_INT64_MAX_SIZE                  (1 + sizeof(int64_t))
#define LE_CBOR_POS_INTEGER_MAX_SIZE            (1 + sizeof(uint64_t))
#define LE_CBOR_NEG_INTEGER_MAX_SIZE            (1 + sizeof(int64_t))
#define LE_CBOR_TAG_MAX_SIZE                    (1 + sizeof(le_cbor_Tag_t))
#define LE_CBOR_BOOL_MAX_SIZE                   (1)
#define LE_CBOR_DOUBLE_MAX_SIZE                 (1 + sizeof(double))
#define LE_CBOR_FLOAT_MAX_SIZE                  (1 + sizeof(float))
#define LE_CBOR_HALF_FLOAT_MAX_SIZE             (1 + 2)
#define LE_CBOR_INDEF_END_MAX_SIZE              (1)
#define LE_CBOR_STR_HEADER_MAX_SIZE             (1 + sizeof(uint32_t))
#define LE_CBOR_ARRAY_HEADER_MAX_SIZE           (1 + sizeof(uint32_t))
#define LE_CBOR_INDEF_ARRAY_HEADER_MAX_SIZE     (1)
#define LE_CBOR_INDEF_MAP_HEADER_MAX_SIZE       (1)
#define LE_CBOR_INDEF_STR_HEADER_MAX_SIZE       (1)
#define LE_CBOR_INDEF_BYTE_STR_HEADER_MAX_SIZE  (1)
#define LE_CBOR_NULL_MAX_SIZE                   (1)

// CBOR data types
typedef enum le_cbor_Type
{
    LE_CBOR_TYPE_POS_INTEGER = 0,
    LE_CBOR_TYPE_NEG_INTEGER = 1,
    LE_CBOR_TYPE_BYTE_STRING = 2,
    LE_CBOR_TYPE_TEXT_STRING = 3,
    LE_CBOR_TYPE_ITEM_ARRAY = 4,
    LE_CBOR_TYPE_TAG = 5,
    LE_CBOR_TYPE_BOOLEAN = 6,
    LE_CBOR_TYPE_DOUBLE = 7,
    LE_CBOR_TYPE_INDEF_END = 8,
    LE_CBOR_TYPE_NULL = 9,
    LE_CBOR_TYPE_INVALID_TYPE = 10,
} le_cbor_Type_t;


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
);


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
);


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
);


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
);


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
);


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
);


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
);


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
);


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
);


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
);


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
);


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
);


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
);


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
);


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
);


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
);


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
);


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
);


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
);


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
);


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
);


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
);


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
);


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
);


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
);


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
);


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
);


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
);


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
);


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
);


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
);


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
);


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
);


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
);


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
);


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
);


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
);


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
);


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
);


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
);


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
);


//--------------------------------------------------------------------------------------------------
/**
 * Decode a tag ID from a buffer, increment the buffer pointer if decoding is successful
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
);


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
);

#endif // LEGATO_CBOR_INCLUDE_GUARD
