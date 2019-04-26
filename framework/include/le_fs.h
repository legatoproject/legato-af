/**
 * @page c_fs File System service
 *
 * @subpage le_fs.h
 *
 * <HR>
 *
 * The File System (FS) service allows accessing the file system on different platforms:
 * - providing a Read/Write space to create/store/read files.
 * - on Read Only platforms, the /tmp/data/le_fs is used as base of the FS sevice.
 *
 * The File System service offers the following file operations:
 * - open a file with le_fs_Open()
 * - close a file with le_fs_Close()
 * - read in a file with le_fs_Read()
 * - write in a file with le_fs_Write()
 * - change the current position in a file with le_fs_Seek()
 * - get the size of a file with le_fs_GetSize()
 * - delete a file with le_fs_Delete()
 * - move a file with le_fs_Move()
 * - recursively deletes a folder with le_fs_RemoveDirRecursive()
 * - checks whether a regular file exists le_fs_Exists()
 *
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LE_FS_H_INCLUDE_GUARD
#define LE_FS_H_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Define the maximal bit mask for file access mode.
 *
 * @note This maximal value should be coherent with @ref le_fs_AccessMode_t
 */
//--------------------------------------------------------------------------------------------------
#define LE_FS_ACCESS_MODE_MAX 127

//--------------------------------------------------------------------------------------------------
/**
 * File access modes used when opening a file.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_FS_RDONLY = 0x1,        ///< Read only
    LE_FS_WRONLY = 0x2,        ///< Write only
    LE_FS_RDWR   = 0x4,        ///< Read/Write
    LE_FS_CREAT  = 0x8,        ///< Create a new file
    LE_FS_TRUNC  = 0x10,       ///< Truncate
    LE_FS_APPEND = 0x20,       ///< Append
    LE_FS_SYNC   = 0x40        ///< Synchronized
}
le_fs_AccessMode_t;


//--------------------------------------------------------------------------------------------------
/**
 * Offset position used when seeking in a file.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_FS_SEEK_SET = 0, ///< From the current position
    LE_FS_SEEK_CUR = 1, ///< From the beginning of the file
    LE_FS_SEEK_END = 2  ///< From the end of the file
}
le_fs_Position_t;


//--------------------------------------------------------------------------------------------------
/**
 * Reference of a file
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_fs_File* le_fs_FileRef_t;


//--------------------------------------------------------------------------------------------------
/**
 * This function is called to create or open an existing file.
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_BAD_PARAMETER  A parameter is invalid.
 *  - LE_OVERFLOW       The file path is too long.
 *  - LE_NOT_FOUND      The file does not exists or a directory in the path does not exist
 *  - LE_NOT_PERMITTED  Access denied to the file or to a directory in the path
 *  - LE_UNSUPPORTED    The prefix cannot be added and the function is unusable
 *  - LE_FAULT          The function failed.
 */
//--------------------------------------------------------------------------------------------------
LE_API_FILESYSTEM le_result_t le_fs_Open
(
    const char* filePath,          ///< [IN] File path
    le_fs_AccessMode_t accessMode, ///< [IN] Access mode for the file
    le_fs_FileRef_t* fileRefPtr    ///< [OUT] File reference (if successful)
);

//--------------------------------------------------------------------------------------------------
/**
 * This function is called to close an opened file.
 *
 * @return
 *  - LE_OK         The function succeeded.
 *  - LE_FAULT      The function failed.
 */
//--------------------------------------------------------------------------------------------------
LE_API_FILESYSTEM le_result_t le_fs_Close
(
    le_fs_FileRef_t fileRef ///< [IN] File reference
);

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
LE_API_FILESYSTEM le_result_t le_fs_Read
(
    le_fs_FileRef_t fileRef, ///< [IN] File reference
    uint8_t* bufPtr,         ///< [OUT] Buffer to store the data read in the file
    size_t* bufSizePtr       ///< [INOUT] Size to read and size really read on file
);

//--------------------------------------------------------------------------------------------------
/**
 * This function is called to write the requested data length to an opened file.
 * The data is written at the current file position.
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_BAD_PARAMETER  A parameter is invalid.
 *  - LE_UNDERFLOW      The write succeed but was not able to write all bytes
 *  - LE_FAULT          The function failed.
 */
//--------------------------------------------------------------------------------------------------
LE_API_FILESYSTEM le_result_t le_fs_Write
(
    le_fs_FileRef_t fileRef, ///< [IN] File reference
    const uint8_t* bufPtr,   ///< [IN] Buffer to write in the file
    size_t bufSize           ///< [IN] Size of the buffer to write
);

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
LE_API_FILESYSTEM le_result_t le_fs_Seek
(
    le_fs_FileRef_t fileRef,   ///< [IN] File reference
    int32_t offset,            ///< [IN] Offset to apply
    le_fs_Position_t position, ///< [IN] Offset is relative to this position
    int32_t* currentOffsetPtr  ///< [OUT] Offset from the beginning after the seek operation
);

//--------------------------------------------------------------------------------------------------
/**
 * This function is called to get the size of a file.
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_BAD_PARAMETER  A parameter is invalid.
 *  - LE_OVERFLOW       The file path is too long.
 *  - LE_UNSUPPORTED    The prefix cannot be added and the function is unusable
 *  - LE_FAULT          The function failed.
 */
//--------------------------------------------------------------------------------------------------
LE_API_FILESYSTEM le_result_t le_fs_GetSize
(
    const char* filePath, ///< [IN] File path
    size_t* sizePtr       ///< [OUT] File size (if successful)
);

//--------------------------------------------------------------------------------------------------
/**
 * This function is called to delete a file.
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_BAD_PARAMETER  A parameter is invalid.
 *  - LE_OVERFLOW       The file path is too long.
 *  - LE_NOT_FOUND      The file does not exist or a directory in the path does not exist
 *  - LE_NOT_PERMITTED  The access right fails to delete the file or access is not granted to a
 *                      a directory in the path
 *  - LE_UNSUPPORTED    The prefix cannot be added and the function is unusable
 *  - LE_FAULT          The function failed.
 */
//--------------------------------------------------------------------------------------------------
LE_API_FILESYSTEM le_result_t le_fs_Delete
(
    const char* filePath ///< [IN] File path
);

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
LE_API_FILESYSTEM le_result_t le_fs_Move
(
    const char* srcPath, ///< [IN] Path to file to rename
    const char* destPath ///< [IN] New path to file
);

//--------------------------------------------------------------------------------------------------
/**
 * Removes a directory located at storage managed by file system service by first recursively
 * removing sub-directories, files, symlinks, hardlinks, devices, etc.  Symlinks are not followed,
 * only the links themselves are deleted.
 *
 * A file or device may not be able to be removed if it is busy, in which case an error message
 * is logged and LE_FAULT is returned.
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_BAD_PARAMETER  A parameter is invalid.
 *  - LE_UNSUPPORTED    The prefix cannot be added and the function is unusable
 *  - LE_FAULT          There is an error.
 */
//--------------------------------------------------------------------------------------------------
LE_API_FILESYSTEM le_result_t le_fs_RemoveDirRecursive
(
    const char* dirPathPtr     ///< [IN] Directory path
);

//--------------------------------------------------------------------------------------------------
/**
 * This function checks whether a regular file exists at the provided path under file system service
 * storage.
 *
 * @return
 *  - true              If file exists and it is a regular file.
 *  - false             Otherwise.
 */
//--------------------------------------------------------------------------------------------------
LE_API_FILESYSTEM bool le_fs_Exists
(
    const char* filePathPtr     ///< [IN] File path
);

#endif // LE_FS_H_INCLUDE_GUARD
