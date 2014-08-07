 /** @file files.c
 *
 * Implementation of the files manipulations API.
 *
 * Copyright (C) Sierra Wireless, Inc. 2014. All rights reserved. Use of this work is subject to license.
 */

#include "legato.h"
#include "files.h"


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
)
{
    if ( (bufSize == 0) || (buf == NULL) )
    {
        return LE_FAULT;
    }

    size_t index = 0;

    for (; index < bufSize - 1; index++)
    {
        // read one byte at a time.
        char c;
        int result;

        do
        {
            result = read(fd, &c, 1);
        }
        while ( (result == -1) && (errno == EINTR) );

        if (result == 1)
        {
            if (c == '\n')
            {
                // This is the end of the line.  Terminate the string and return.
                buf[index] = '\0';
                return LE_OK;
            }

            // Store char.
            buf[index] = c;
        }
        else if (result == 0)
        {
            // End of file nothing else to read.  Terminate the string and return.
            buf[index] = '\0';

            if (index == 0)
            {
                return LE_OUT_OF_RANGE;
            }
            else
            {
                return LE_OK;
            }
        }
        else
        {
            LE_ERROR("Could not read file.  %m.");
            return LE_FAULT;
        }
    }

    // No more buffer space.  Terminate the string and return.
    LE_ERROR("Buffer too small.");
    buf[index] = '\0';
    return LE_OVERFLOW;
}


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
)
{
    if (lseek(fd, offset, SEEK_SET) == -1)
    {
        LE_ERROR("Could not seek to address %zx.  %m.", (size_t)offset);
        return LE_FAULT;
    }

    int result;

    do
    {
        result = read(fd, bufPtr, bufSize);
    }
    while ( (result == -1) && (errno == EINTR) );

    if (result == -1)
    {
        LE_ERROR("Could not read file.  %m.");
        return LE_FAULT;
    }
    else if (result != bufSize)
    {
        LE_ERROR("Unexpected end of file.");
        return LE_FAULT;
    }

    return LE_OK;
}
