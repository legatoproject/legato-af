/** @file files.h
 *
 * Files manipulations API.
 *
 * Copyright (C) Sierra Wireless, Inc. 2014. All rights reserved. Use of this work is subject to license.
 */

#ifndef LE_FILES_H_INCLUDE_GUARD
#define LE_FILES_H_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Reads a line of text from the opened file descriptor specified up to the first newline or eof
 * character.  The output buffer will always be NULL-terminated and will not include the newline or
 * eof character.
 *
 * @return
 *      LE_OK if successful.
 *      LE_OVERFLOW if the buffer is too small.  As much of the line as possible will be copied to
 *                  buf.
 *      LE_OUT_OF_RANGE if there is nothing else to read from the file.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t files_ReadLine
(
    int fd,             ///< [IN] File to read from.
    char* buf,          ///< [OUT] Buffer to store the line.
    size_t bufSize      ///< [IN] Buffer size.
);


//--------------------------------------------------------------------------------------------------
/**
 * Reads bufSize bytes from the open file descriptor specified by fd, starting at offset, and stores
 * the bytes in the provided buffer.  This function will fail and return LE_FAULT if fewer than
 * bufSize bytes are available from the file.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t files_ReadFromOffset
(
    int fd,                 ///< [IN] File to read.
    off_t offset,           ///< [IN] Offset from the beginning of the file to start reading from.
    void* bufPtr,           ///< [OUT] Buffer to store the read bytes in.
    size_t bufSize          ///< [IN] Size of the buffer.
);


#endif // LE_FILES_H_INCLUDE_GUARD
