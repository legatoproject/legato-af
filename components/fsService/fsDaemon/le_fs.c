/**
 * @file le_fs.c
 *
 * This file contains the data structures and the source code of the File System (FS) service.
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"
#include "pa_fs.h"


//--------------------------------------------------------------------------------------------------
/**
 * Type casting of file reference. Only support 32 and 64 bit wide reference.
 */
//--------------------------------------------------------------------------------------------------
#define FS_CAST(x) (int32_t)((uintptr_t)(x) & 0xFFFFFFFF)

//--------------------------------------------------------------------------------------------------
// APIs
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * This function is called to create or open an existing file.
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_BAD_PARAMETER  A parameter is invalid.
 *  - LE_OVERFLOW       The file path is too long.
 *  - LE_FAULT          The function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_fs_Open
(
    const char* filePathPtr,        ///< [IN]  File path
    le_fs_AccessMode_t accessMode,  ///< [IN]  Access mode for the file
    le_fs_FileRef_t* fileRefPtr     ///< [OUT] File reference (if successful)
)
{
    // Check if the pointers are set
    if (NULL == filePathPtr)
    {
        LE_ERROR("NULL file path pointer!");
        return LE_BAD_PARAMETER;
    }
    if (NULL == fileRefPtr)
    {
        LE_ERROR("NULL file handler pointer!");
        return LE_BAD_PARAMETER;
    }

    // Check if the file path starts with '/'
    if ('/' != *filePathPtr)
    {
        LE_ERROR("File path should start with '/'");
        return LE_BAD_PARAMETER;
    }

    // Check if the access mode is correct
    if ((accessMode & (uint16_t)(~(uint16_t)LE_FS_ACCESS_MODE_MAX)) || (0 == accessMode))
    {
        LE_ERROR("Unable to open file, wrong access mode 0x%04X", accessMode);
        return LE_BAD_PARAMETER;
    }

    return pa_fs_Open(filePathPtr, accessMode, (int32_t *)fileRefPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is called to close an opened file.
 *
 * @return
 *  - LE_OK         The function succeeded.
 *  - LE_FAULT      The function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_fs_Close
(
    le_fs_FileRef_t fileRef     ///< [IN] File reference
)
{
    return pa_fs_Close(FS_CAST(fileRef));
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is called to read the requested data length from an opened file.
 * The data is read at the current file position.
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_BAD_PARAMETER  A parameter is invalid.
 *  - LE_FAULT          The function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_fs_Read
(
    le_fs_FileRef_t fileRef,     ///< [IN]  File reference
    uint8_t* bufPtr,             ///< [OUT] Buffer to store the data read in the file
    size_t* bufNumElementsPtr    ///< [IN]  Number of bytes to read when this function is called
                                 ///< [OUT] Number of bytes read when this function returns
)
{
    // Check if the pointers are set
    if (NULL == bufPtr)
    {
        LE_ERROR("NULL buffer pointer!");
        return LE_BAD_PARAMETER;
    }
    if (NULL == bufNumElementsPtr)
    {
        LE_ERROR("NULL bytes number pointer!");
        return LE_BAD_PARAMETER;
    }

    // Check buffer size
    if (*bufNumElementsPtr > LE_FS_DATA_MAX_SIZE)
    {
        LE_ERROR("Requested length to read is too big, %zu > %d bytes",
                 *bufNumElementsPtr, LE_FS_DATA_MAX_SIZE);
        return LE_BAD_PARAMETER;
    }

    // Check the number of bytes to read
    if (0 == *bufNumElementsPtr)
    {
        // No need to read 0 bytes
        return LE_OK;
    }

    return pa_fs_Read(FS_CAST(fileRef), bufPtr, bufNumElementsPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is called to write the requested data length to an opened file.
 * The data is written at the current file position.
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_BAD_PARAMETER  A parameter is invalid.
 *  - LE_FAULT          The function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_fs_Write
(
    le_fs_FileRef_t fileRef,  ///< [IN] File reference
    const uint8_t* bufPtr,    ///< [IN] Buffer to write in the file
    size_t bufNumElements     ///< [IN] Number of bytes to write
)
{
    // Check if the pointer is set
    if (NULL == bufPtr)
    {
        LE_ERROR("NULL buffer pointer!");
        return LE_BAD_PARAMETER;
    }

    // Check buffer size
    if (bufNumElements > LE_FS_DATA_MAX_SIZE)
    {
        LE_ERROR("Requested length to write is too big, %zu > %d bytes",
                 bufNumElements, LE_FS_DATA_MAX_SIZE);
        return LE_BAD_PARAMETER;
    }

    // Check the number of bytes to write
    if (0 == bufNumElements)
    {
        // No need to write 0 bytes
        return LE_OK;
    }

    return pa_fs_Write(FS_CAST(fileRef), bufPtr, bufNumElements);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is called to change the file position of an opened file.
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_BAD_PARAMETER  A parameter is invalid.
 *  - LE_FAULT          The function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_fs_Seek
(
    le_fs_FileRef_t fileRef,    ///< [IN]  File reference
    int32_t offset,             ///< [IN]  Offset to apply when this function is called
    le_fs_Position_t position,  ///< [IN]  Offset is relative to this position
    int32_t* currentOffsetPtr   ///< [OUT] Offset from the beginning after the seek operation
)
{
    // Check if the pointer is set
    if (NULL == currentOffsetPtr)
    {
        LE_ERROR("NULL current offset pointer!");
        return LE_BAD_PARAMETER;
    }
    if (position > LE_FS_SEEK_END)
    {
        LE_ERROR("Wrong seek position!");
        return LE_BAD_PARAMETER;
    }

    le_result_t result = pa_fs_Seek(FS_CAST(fileRef), (off_t*)&offset, position);
    *currentOffsetPtr = offset;
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is called to get the size of a file.
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_BAD_PARAMETER  A parameter is invalid.
 *  - LE_OVERFLOW       The file path is too long.
 *  - LE_FAULT          The function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_fs_GetSize
(
    const char* filePathPtr,    ///< [IN]  File path
    uint32_t* sizePtr           ///< [OUT] File size (if successful)
)
{
    // Check if the pointers are set
    if (NULL == filePathPtr)
    {
        LE_ERROR("NULL file path pointer!");
        return LE_BAD_PARAMETER;
    }
    if (NULL == sizePtr)
    {
        LE_ERROR("NULL size pointer!");
        return LE_BAD_PARAMETER;
    }

    // Check if the file path starts with '/'
    if ('/' != *filePathPtr)
    {
        LE_ERROR("File path should start with '/'");
        return LE_BAD_PARAMETER;
    }

    return pa_fs_GetSize(filePathPtr, (size_t*)sizePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is called to delete a file.
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_BAD_PARAMETER  A parameter is invalid.
 *  - LE_OVERFLOW       The file path is too long.
 *  - LE_FAULT          The function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_fs_Delete
(
    const char* filePathPtr     ///< [IN] File path
)
{
    // Check if the pointer is set
    if (NULL == filePathPtr)
    {
        LE_ERROR("NULL file path pointer!");
        return LE_BAD_PARAMETER;
    }

    // Check if the file path starts with '/'
    if ('/' != *filePathPtr)
    {
        LE_ERROR("File path should start with '/'");
        return LE_BAD_PARAMETER;
    }

    return pa_fs_Delete(filePathPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is called to rename an existing file.
 * If rename fails, the file will keep its original name.
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_BAD_PARAMETER  A parameter is invalid.
 *  - LE_OVERFLOW       A file path is too long.
 *  - LE_FAULT          The function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_fs_Move
(
    const char* srcPathPtr,     ///< [IN] Path to file to rename
    const char* destPathPtr     ///< [IN] New path to file
)
{
    // Check if the pointers are set
    if (NULL == srcPathPtr)
    {
        LE_ERROR("NULL source path pointer!");
        return LE_BAD_PARAMETER;
    }
    if (NULL == destPathPtr)
    {
        LE_ERROR("NULL destination path pointer!");
        return LE_BAD_PARAMETER;
    }

    // Check if the file paths start with '/'
    if ('/' != *srcPathPtr)
    {
        LE_ERROR("Source file path should start with '/'");
        return LE_BAD_PARAMETER;
    }
    if ('/' != *destPathPtr)
    {
        LE_ERROR("Destination file path should start with '/'");
        return LE_BAD_PARAMETER;
    }

    // Check if the paths are different
    if (0 == strcmp(srcPathPtr, destPathPtr))
    {
        LE_ERROR("Same path for source and destination!");
        return LE_BAD_PARAMETER;
    }

    return pa_fs_Move(srcPathPtr, destPathPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialize this component
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    //No further initialization needed
}
