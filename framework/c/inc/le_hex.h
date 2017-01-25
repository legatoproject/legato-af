/**
 * @page c_hex Hex string API
 *
 *
 * @ref le_hex.h "API Reference"
 *
 * <HR>
 *
 *
 * This API provides convertion tools to switch between:
 *  - @ref le_hex_StringToBinary  Hex-String to binary
 *  - @ref le_hex_BinaryToString  Binary to Hex-String
 *
 *
 * @section hex_conversion Conversion
 *
 * Code sample:
 *
 * @code
 * char     HexString[] = "136ABC";
 * uint8_t  binString[] = {0x13,0x6A,0xBC};
 * @endcode
 *
 * So @ref le_hex_StringToBinary will convert HexString to binString.
 *
 * and @ref le_hex_BinaryToString will convert binString to HexString.
 *
 *
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc.
 */


/** @file le_hex.h
 *
 * Legato @ref c_hex include file.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_HEX_INCLUDE_GUARD
#define LEGATO_HEX_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Convert a string of valid hexadecimal characters [0-9a-fA-F] into a byte array where each
 * element of the byte array holds the value corresponding to a pair of hexadecimal characters.
 *
 * @return
 *      - number of bytes written into binaryPtr
 *      - -1 if the binarySize is too small or stringLength is odd or stringPtr contains an invalid
 *        character
 *
 * @note The input string is not required to be NULL terminated.
 */
//--------------------------------------------------------------------------------------------------
int32_t le_hex_StringToBinary
(
    const char *stringPtr,     ///< [IN] string to convert
    uint32_t    stringLength,  ///< [IN] string length
    uint8_t    *binaryPtr,     ///< [OUT] binary result
    uint32_t    binarySize     ///< [IN] size of the binary table.  Must be >= stringLength / 2
);

//--------------------------------------------------------------------------------------------------
/**
 * Convert a byte array into a string of uppercase hexadecimal characters.
 *
 * @return number of characters written to stringPtr or -1 if stringSize is too small for
 *         binarySize
 *
 * @note the string written to stringPtr will be NULL terminated.
 */
//--------------------------------------------------------------------------------------------------
int32_t le_hex_BinaryToString
(
    const uint8_t *binaryPtr,  ///< [IN] binary array to convert
    uint32_t       binarySize, ///< [IN] size of binary array
    char          *stringPtr,  ///< [OUT] hex string array, terminated with '\0'.
    uint32_t       stringSize  ///< [IN] size of string array.  Must be >= (2 * binarySize) + 1
);

//--------------------------------------------------------------------------------------------------
/**
 * Convert a NULL terminated string of valid hexadecimal characters [0-9a-fA-F] into an integer.
 *
 * @return
 *      - Positive integer corresponding to the hexadecimal input string
 *      - -1 if the input contains an invalid character or the value will not fit in an integer
 */
//--------------------------------------------------------------------------------------------------
int le_hex_HexaToInteger
(
    const char *stringPtr ///< [IN] string of hex chars to convert into an int
);

#endif // LEGATO_HEX_INCLUDE_GUARD
