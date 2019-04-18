/**
 * @page c_base64 Base64 encoding/decoding API
 *
 * @subpage le_base64.h "API Reference"
 *
 * <HR>
 *
 * This module implements encoding binary data into base64-encoded string, which contains only
 * the characters that belong to the base64 alphabet:
 * ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=
 *
 * This allows to convert the binary data into a form that is suitable for serialization
 * into a text file (without conflicting with special characters and markup elements),
 * and for sending through the channels that do not support arbitrary binary data.
 *
 * Encoded data is about 33% larger in size than the original binary data. Padding characters '='
 * are added to the end of the encoded string, to make the string length multiple of 4.
 *
 * The following sample performs base64 encoding and decoding:
 *
 * @code
 *
 * void Base64EncodeDecodeExample
 * (
 *     void
 * )
 * {
 *     // allocate 4-byte array and fill it with some data
 *     uint8_t binBuf[4] = {1, 2, 3, 4};
 *     // use macro to calculate the encoded size, add 1 byte for terminating 0
 *     char encodedBuf[LE_BASE64_ENCODED_SIZE(sizeof(binBuf)) + 1];
 *     uint8_t decodedBuf[4] = {0};
 *
 *     // perform encoding
 *     size_t len = sizeof(encodedBuf);
 *     le_result_t result = le_base64_Encode(binBuf,
 *                                           sizeof(binBuf),
 *                                           encodedBuf,
 *                                           &len);
 *     if (LE_OK != result)
 *     {
 *         LE_ERROR("Error %d encoding data!", result);
 *     }
 *     LE_INFO("Encoded string [%s] size %zd", encodedBuf, len);
 *
 *     // perform decoding
 *     len = sizeof(decodedBuf);
 *     result = le_base64_Decode(encodedBuf,
 *                               strlen(encodedBuf),
 *                               decodedBuf,
 *                               &len);
 *     if (LE_OK != result)
 *     {
 *         LE_ERROR("Error %d decoding data!", result);
 *     }
 *     LE_INFO("Decoded length: %zd", len);
 *     LE_INFO("Decoded data: %u %u %u %u",
 *              decodedBuf[0], decodedBuf[1],
 *              decodedBuf[2], decodedBuf[3]);
 * }
 * @endcode
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

//--------------------------------------------------------------------------------------------------
/** @file le_base64.h
 *
 * Legato @ref c_base64 include file.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_BASE64_INCLUDE_GUARD
#define LEGATO_BASE64_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Calculate the encoded string length (including padding, not including terminating zero)
 * for a given binary data size.
 */
//--------------------------------------------------------------------------------------------------
#define LE_BASE64_ENCODED_SIZE(x) (4 * ((x + 2) / 3))

//--------------------------------------------------------------------------------------------------
/**
 * Perform base64 data encoding.
 *
 * @return
 *      - LE_OK if succeeds
 *      - LE_BAD_PARAMETER if NULL pointer provided
 *      - LE_OVERFLOW if provided buffer is not large enough
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_base64_Encode
(
    const uint8_t *dataPtr,     ///< [IN] Data to be encoded
    size_t dataLength,          ///< [IN] Data length
    char *resultPtr,            ///< [OUT] Base64-encoded string buffer
    size_t *resultSizePtr       ///< [INOUT] Length of the base64-encoded string buffer
);

//--------------------------------------------------------------------------------------------------
/**
 * Decode base64-encoded data.
 *
 * @return
 *      - LE_OK if succeeds
 *      - LE_BAD_PARAMETER if NULL pointer provided
 *      - LE_FORMAT_ERROR if data contains invalid (non-base64) characters
 *      - LE_OVERFLOW if provided buffer is not large enough
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_base64_Decode
(
    const char *srcPtr, ///< [IN] Encoded string
    size_t srcLen,      ///< [IN] Encoded string length
    uint8_t *dstPtr,    ///< [OUT] Binary data buffer
    size_t *dstLenPtr   ///< [INOUT] Binary data buffer size / decoded data size
);

#endif // LEGATO_BASE64_INCLUDE_GUARD
