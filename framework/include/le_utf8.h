/**
 * @page c_utf8 UTF-8 String Handling API
 *
 * @subpage le_utf8.h "API Reference"
 *
 * <HR>
 *
 * This module implements safe and easy to use string handling functions for null-terminated strings
 * with UTF-8 encoding.
 *
 * UTF-8 is a variable length character encoding that supports every character in the Unicode
 * character set. UTF-8 has become the dominant character encoding because it is self synchronizing,
 * compatible with ASCII, and avoids the endian issues that other encodings face.
 *
 *  @section utf8_encoding UTF-8 Encoding
 *
 * UTF-8 uses between one and four bytes to encode a character as illustrated in the following
 * table.
 *
 * <table>
 * <tr> <th> Byte 1   </th> <th> Byte 2   </th> <th> Byte 3   </th> <th> Byte 4   </th> </tr>
 * <tr> <td> 0xxxxxxx </td> <td>          </td> <td>          </td> <td>          </td> </tr>
 * <tr> <td> 110xxxxx </td> <td> 10xxxxxx </td> <td>          </td> <td>          </td> </tr>
 * <tr> <td> 1110xxxx </td> <td> 10xxxxxx </td> <td> 10xxxxxx </td> <td>          </td> </tr>
 * <tr> <td> 11110xxx </td> <td> 10xxxxxx </td> <td> 10xxxxxx </td> <td> 10xxxxxx </td> </tr>
 * </table>
 *
 * Single byte codes are used only for the ASCII values 0 through 127.  In this case, UTF-8 has the
 * same binary value as ASCII, making ASCII text valid UTF-8 encoded Unicode.  All ASCII strings are
 * UTF-8 compatible.
 *
 * Character codes larger than 127 have a multi-byte encoding consisting of a leading byte and one
 * or more continuation bytes.
 *
 * The leading byte has two or more high-order 1's followed by a 0 that can be used to determine the
 * number bytes in the character without examining the continuation bytes.
 *
 * The continuation bytes have '10' in the high-order position.
 *
 * Single bytes, leading bytes and continuation bytes can't have the same values. This means that
 * UTF-8 strings are self-synchronized, allowing the start of a character to be found by backing up
 * at most three bytes.
 *
 * @c le_utf8_EncodeUnicodeCodePoint() provides a function that is able to encode any unicode code
 * point into a sequence of bytes that represents the utf-8 encoding of the codepoint.  The function
 * @c le_utf8_DecodeUnicodeCodePoint() implements the inverse function.  It converts a UTF-8 encoded
 * character into the corresponding unicode code point.
 *
 *  @section utf8_copy Copy and Append
 *
 * @c le_utf8_Copy() copies a string to a specified buffer location.
 *
 * @c le_utf8_Append() appends a string to the end of another string by copying the source string to
 * the destination string's buffer starting at the null-terminator of the destination string.
 *
 * The @c le_uft8_CopyUpToSubStr() function is like le_utf8_Copy() except it copies only up to, but
 * not including, a specified string.
 *
 *  @section utf8_trunc Truncation
 *
 * Because UTF-8 is a variable length encoding, the number of characters in a string is not
 * necessarily the same as the number bytes in the string.  When using functions like le_utf8_Copy()
 * and le_utf8_Append(), the size of the destination buffer, in bytes, must be provided to avoid
 * buffer overruns.
 *
 * The copied string is truncated because of limited space in the destination buffer, and the
 * destination buffer may not be completely filled.  This can occur during the copy process if the
 * last character to copy is more than one byte long and will not fit within the buffer.
 *
 * The character is not copied and a null-terminator is added.  Even though we have not filled the
 * destination buffer, we have truncated the copied string.  Essentially, functions like
 * le_utf8_Copy() and le_utf8_Append() only copy complete characters, not partial characters.
 *
 * For le_utf8_Copy(), the number of bytes actually copied is returned in the numBytesPtr parameter.
 * This parameter can be set to NULL if the number of bytes copied is not needed.  le_utf8_Append()
 * and le_uft8_CopyUpToSubStr() work similarly.
 *
 * @code
 * // In this code sample, we need the number of bytes actually copied:
 * size_t numBytes;
 *
 * if (le_utf8_Copy(destStr, srcStr, sizeof(destStr), &numBytes) == LE_OVERFLOW)
 * {
 *     LE_WARN("'%s' was truncated when copied.  Only %d bytes were copied.", srcStr, numBytes);
 * }
 *
 * // In this code sample, we don't care about the number of bytes copied:
 * LE_ASSERT(le_utf8_Copy(destStr, srcStr, sizeof(destStr), NULL) != LE_OVERFLOW);
 * @endcode
 *
 *  @section utf8_stringLength String Lengths
 *
 * String length may mean either the number of characters in the string or the number of bytes in
 * the string.  These two meanings are often used interchangeably because in ASCII-only encodings
 * the number of characters in a string is equal to the number of bytes in a string. But this is not
 * necessarily true with variable length encodings such as UTF-8. Legato provides both a
 * le_utf8_NumChars() function and a le_utf8_NumBytes() function.
 *
 * @c le_utf8_NumBytes() must be used when determining the memory size of a string.
 * @c le_utf8_NumChars() is useful for counting the number of characters in a string (ie. for
 * display purposes).
 *
 *  @section utf8_charLength Character Lengths
 *
 * The function le_utf8_NumBytesInChar() can be used to determine the number of bytes in a character
 * by looking at its first byte.  This is handy when reading a UTF-8 string from an input stream.
 * When the first byte is read, it can be passed to le_utf8_NumBytesInChar() to determine how many
 * more bytes need to be read to get the rest of the character.
 *
 *  @section utf8_format Checking UTF-8 Format
 *
 * As can be seen in the @ref utf8_encoding section, UTF-8 strings have a specific byte sequence.
 * The @c le_utf8_IsFormatCorrect() function can be used to check if a string conforms to UTF-8
 * encoding.  Not all valid UTF-8 characters are valid for a given character set;
 * le_utf8_IsFormatCorrect() does not check for this.
 *
 *  @section utf8_parsing String Parsing
 *
 * To assist with converting integer values from UTF-8 strings to binary numerical values,
 * le_utf8_ParseInt() is provided.
 *
 * More parsing functions may be added as required in the future.
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
*/

//--------------------------------------------------------------------------------------------------
/** @file le_utf8.h
 *
 * Legato @ref c_utf8 include file.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#ifndef LEGATO_UTF8_INCLUDE_GUARD
#define LEGATO_UTF8_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Returns the number of characters in string.
 *
 * UTF-8 encoded characters may be larger than 1 byte so the number of characters is not necessarily
 * equal to the the number of bytes in the string.
 *
 * @return
 *      - Number of characters in string if successful.
 *      - LE_FORMAT_ERROR if the string is not UTF-8.
 */
//--------------------------------------------------------------------------------------------------
ssize_t le_utf8_NumChars
(
    const char* string      ///< [IN] Pointer to the string.
);


//--------------------------------------------------------------------------------------------------
/**
 * Returns the number of bytes in string (not including the null-terminator).
 *
 * @return
 *      Number of bytes in string (not including the null-terminator).
 */
//--------------------------------------------------------------------------------------------------
size_t le_utf8_NumBytes
(
    const char* string      ///< [IN] The string.
);


//--------------------------------------------------------------------------------------------------
/**
 * Returns the number of bytes in the character that starts with a given byte.
 *
 * @return
 *      Number of bytes in the character, or 0 if the byte provided is not a valid starting byte.
 */
//--------------------------------------------------------------------------------------------------
size_t le_utf8_NumBytesInChar
(
    const char firstByte    ///< [IN] The first byte in the character.
);


//--------------------------------------------------------------------------------------------------
/**
 * Determines whether a given byte is a continuation (not the first byte) of a multi-byte UTF-8
 * character.
 *
 * @return  True if a continuation byte or false otherwise.
 */
//--------------------------------------------------------------------------------------------------
static inline bool le_utf8_IsContinuationByte
(
    const char byte     ///< [IN] The byte to check.
)
{
    return ( (byte & 0xC0) == 0x80 );
}


//--------------------------------------------------------------------------------------------------
/**
 * Copies the string in srcStr to the start of destStr and returns the number of bytes copied (not
 * including the NULL-terminator) in numBytesPtr.  Null can be passed into numBytesPtr if the number
 * of bytes copied is not needed.  The srcStr must be in UTF-8 format.
 *
 * If the size of srcStr is less than or equal to the destination buffer size then the entire srcStr
 * will be copied including the null-character.  The rest of the destination buffer is not modified.
 *
 * If the size of srcStr is larger than the destination buffer then the maximum number of characters
 * (from srcStr) plus a null-character that will fit in the destination buffer is copied.
 *
 * UTF-8 characters may be more than one byte long and this function will only copy whole characters
 * not partial characters. Therefore, even if srcStr is larger than the destination buffer, the
 * copied characters may not fill the entire destination buffer because the last character copied
 * may not align exactly with the end of the destination buffer.
 *
 * The destination string will always be Null-terminated, unless destSize is zero.
 *
 * If destStr and srcStr overlap the behaviour of this function is undefined.
 *
 * @return
 *      - LE_OK if srcStr was completely copied to the destStr.
 *      - LE_OVERFLOW if srcStr was truncated when it was copied to destStr.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_utf8_Copy
(
    char* destStr,          ///< [IN] Destination where the srcStr is to be copied.
    const char* srcStr,     ///< [IN] UTF-8 source string.
    const size_t destSize,  ///< [IN] Size of the destination buffer in bytes.
    size_t* numBytesPtr     ///< [OUT] Number of bytes copied not including the NULL-terminator.
                            ///        Parameter can be set to NULL if the number of bytes copied is
                            ///        not needed.
);


//--------------------------------------------------------------------------------------------------
/**
 * Appends srcStr to destStr by copying characters from srcStr to the end of destStr.  The srcStr
 * must be in UTF-8 format.  The number of bytes in the resultant destStr (not including the
 * NULL-terminator) is returned in destStrLenPtr.
 *
 * A null-character is always added to the end of destStr after all srcStr characters have been
 * copied.
 *
 * This function will copy as many characters as possible from srcStr to destStr while ensuring that
 * the resultant string (including the null-character) will fit within the destination buffer.
 *
 * UTF-8 characters may be more than one byte long and this function will only copy whole characters
 * not partial characters.
 *
 * The destination string will always be Null-terminated, unless destSize is zero.
 *
 * If destStr and srcStr overlap the behaviour of this function is undefined.
 *
 * @return
 *      - LE_OK if srcStr was completely copied to the destStr.
 *      - LE_OVERFLOW if srcStr was truncated when it was copied to destStr.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_utf8_Append
(
    char* destStr,          ///< [IN] Destination string.
    const char* srcStr,     ///< [IN] UTF-8 source string.
    const size_t destSize,  ///< [IN] Size of the destination buffer in bytes.
    size_t* destStrLenPtr   ///< [OUT] Number of bytes in the resultant destination string (not
                            ///        including the NULL-terminator).  Parameter can be set to
                            ///        NULL if the destination string size is not needed.
);


//--------------------------------------------------------------------------------------------------
/**
 * Copies all characters from the srcStr to destStr up to the first occurrence of subStr.  The
 * subStr is not copied and instead a null-terminator is added to the destStr.  The number of bytes
 * copied (not including the null-terminator) is returned in numBytesPtr.
 *
 * The srcStr and subStr must be in null-terminated UTF-8 strings.
 *
 * The destination string will always be null-terminated.
 *
 * If subStr is not found in the srcStr then this function behaves just like le_utf8_Copy().
 *
 * @return
 *      - LE_OK if srcStr was completely copied to the destStr.
 *      - LE_OVERFLOW if srcStr was truncated when it was copied to destStr.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_utf8_CopyUpToSubStr
(
    char* destStr,          ///< [IN] Destination where the srcStr is to be copied.
    const char* srcStr,     ///< [IN] UTF-8 source string.
    const char* subStr,     ///< [IN] Sub-string to copy up to.
    const size_t destSize,  ///< [IN] Size of the destination buffer in bytes.
    size_t* numBytesPtr     ///< [OUT] Number of bytes copied not including the NULL-terminator.
                            ///        Parameter can be set to NULL if the number of bytes
                            ///        copied is not needed.
);


//--------------------------------------------------------------------------------------------------
/**
 * Checks to see if the string is indeed a UTF-8 encoded, null-terminated string.
 *
 * @return
 *      true if the format is correct or false otherwise
 */
//--------------------------------------------------------------------------------------------------
bool le_utf8_IsFormatCorrect
(
    const char* string      ///< [IN] The string.
);


//--------------------------------------------------------------------------------------------------
/**
 * Parse an integer value from a string.
 *
 * @return
 *      - LE_OK = Success.
 *      - LE_FORMAT_ERROR = The argument string was not an integer value.
 *      - LE_OUT_OF_RANGE = Value is too large to be stored in an int variable.
 **/
//--------------------------------------------------------------------------------------------------
le_result_t le_utf8_ParseInt
(
    int* valuePtr,   ///< [OUT] Ptr to where the value will be stored if successful.
    const char* arg  ///< [IN] The string to parse.
);


//--------------------------------------------------------------------------------------------------
/**
 * Encode a unicode code point as UTF-8 into a buffer.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OUT_OF_RANGE if the code point supplied is outside the range of unicode code points
 *      - LE_OVERFLOW if the out buffer is not large enough to store the UTF-8 encoding of the code
 *        point
 *
 * @note
 *      Not all code point values are valid unicode. This function does not validate whether the
 *      code point is valid unicode.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_utf8_EncodeUnicodeCodePoint
(
    uint32_t codePoint, ///< [IN] Code point to encode as UTF-8
    char* out,          ///< [OUT] Buffer to store the UTF-8 encoded value in.
    size_t* outSize     ///< [IN/OUT] As an input, this value is interpreted as the size of the out
                        ///  buffer.  As an output, it is updated to hold the size of the UTF-8
                        ///  encoded value (in the case of an LE_OK return value) or size that would
                        ///  be required to encode the code point (in the case or an LE_OVERFLOW
                        ///  return value).
);


//--------------------------------------------------------------------------------------------------
/**
 * Decode the first unicode code point from the UTF-8 string src.
 *
 * @return
 *      - LE_OK on success
 *      - LE_BAD_PARAMETER if byteLength points to 0
 *      - LE_UNDERFLOW if src appears to be the beginning of a UTF-8 character which extends beyond
 *        the end of the string as specified by byteLength.
 *      - LE_FORMAT_ERROR if src is not valid UTF-8 encoded string data.
 *
 * @note
 *      Not all code point values are valid unicode. This function does not validate whether the
 *      code point is valid unicode.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_utf8_DecodeUnicodeCodePoint
(
    const char* src,     ///< [IN] UTF-8 encoded data to extract a code point from.
    size_t* byteLength,  ///< [IN/OUT] As an input parameter, the value pointed to represents the
                         ///  number of bytes in src. As an output parameter, the value pointed to
                         ///  is the number of bytes from src that were consumed to decode the code
                         ///  point (in the case of an LE_OK return value) or the number of bytes
                         ///  that would have been consumed had src been long enough (in the case of
                         ///  an LE_UNDERFLOW return value).
    uint32_t* codePoint  ///< [OUT] Code point that was decoded from src.  This value is only valid
                         ///  when the function returns LE_OK.
);

#endif  // LEGATO_UTF8_INCLUDE_GUARD
