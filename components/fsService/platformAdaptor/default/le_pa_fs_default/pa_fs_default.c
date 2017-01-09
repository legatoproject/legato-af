//--------------------------------------------------------------------------------------------------
/**
 * @file pa_fs_default.c
 *
 * Default implementation of @ref pa_fs interface
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "pa_fs.h"


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
le_result_t pa_fs_Open
(
    const char* pathPtr,            ///< [IN] File path
    le_fs_AccessMode_t accessMode,  ///< [IN] Access mode for the file
    int32_t* fileHandlerPtr         ///< [OUT] File handler for the file (if successful)
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
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
le_result_t pa_fs_Close
(
    int32_t fileHandler     ///< [IN] File handler
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
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
le_result_t pa_fs_Read
(
    int32_t  fileHandler,   ///< [IN] File handler
    uint8_t* bufPtr,        ///< [OUT] Buffer to store the data read in the file
    size_t*  bufSizePtr     ///< [IN]  Number of bytes to read when this function is called
                            ///< [OUT] Number of bytes read when this function returns
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
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
le_result_t pa_fs_Write
(
    int32_t        fileHandler, ///< [IN] File handler
    const uint8_t* bufPtr,      ///< [IN] Buffer to write in the file
    size_t         bufSize      ///< [IN] Number of bytes to write
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
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
le_result_t pa_fs_Seek
(
    int32_t fileHandler,        ///< [IN]  File handler
    off_t* offsetPtr,           ///< [IN]  Offset to apply when called
                                ///< [OUT] Offset from the beginning of the file when returns
    le_fs_Position_t position   ///< [IN]  Offset is relative to this position
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is called to get the size of an file.
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_BAD_PARAMETER  A parameter is invalid.
 *  - LE_OVERFLOW       The file path is too long.
 *  - LE_FAULT          The function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_fs_GetSize
(
    const char* pathPtr,    ///< [IN] File path
    size_t*     sizePtr     ///< [IN] File size (if successful)
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is called to delete an file.
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_BAD_PARAMETER  A parameter is invalid.
 *  - LE_OVERFLOW       The file path is too long.
 *  - LE_FAULT          The function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_fs_Delete
(
    const char* pathPtr     ///< [IN] File path
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is called to rename an existing file.
 * If rename fails, the file will keep its original name.
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_BAD_PARAMETER  A parameter is invalid.
 *  - LE_OVERFLOW       The file path is too long.
 *  - LE_FAULT          The function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_fs_Move
(
    const char* srcPathPtr,     ///< [IN] Path to file to rename
    const char* destPathPtr     ///< [IN] New path to file
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Component initialization.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // No initialization needed
}

