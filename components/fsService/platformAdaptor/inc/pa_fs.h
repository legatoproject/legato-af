/**
 * @page c_pa_fs File System Platform Adapter API
 *
 * @ref pa_fs.h "API Reference"
 *
 * <HR>
 *
 * @section pa_fs_toc Table of Contents
 *
 *  - @ref pa_fs_intro
 *  - @ref pa_fs_rationale
 *
 *
 * @section pa_fs_intro Introduction
 *
 * These APIs are on the top of the platform-dependent adapter layer. They are independent of the
 * implementation. They guarantee the portability on different kind of platform without any changes
 * for the components developed upon these APIs.
 *
 *
 * @section pa_fs_rationale Rationale
 * These functions are all blocking functions, so that they return when the modem has answered or
 * when a timeout has occurred due to an interrupted communication with the modem.
 *
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc.
 */

//--------------------------------------------------------------------------------------------------
/** @file pa_fs.h
 *
 * Legato @ref c_pa_fs include file.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_PA_FS_INCLUDE_GUARD
#define LEGATO_PA_FS_INCLUDE_GUARD

#include "legato.h"
#include "interfaces.h"

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
LE_SHARED le_result_t pa_fs_Open
(
    const char* pathPtr,            ///< [IN] File path
    le_fs_AccessMode_t accessMode,  ///< [IN] Access mode for the file
    int32_t* fileHandlerPtr         ///< [OUT] File handler for the file (if successful)
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
LE_SHARED le_result_t pa_fs_Close
(
    int32_t fileHandler     ///< [IN] File handler
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
LE_SHARED le_result_t pa_fs_Read
(
    int32_t  fileHandler,   ///< [IN] File handler
    uint8_t* bufPtr,        ///< [OUT] Buffer to store the data read in the file
    size_t*  bufSizePtr     ///< [IN]  Number of bytes to read when this function is called
                            ///< [OUT] Number of bytes read when this function returns
);

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
LE_SHARED le_result_t pa_fs_Write
(
    int32_t        fileHandler, ///< [IN] File handler
    const uint8_t* bufPtr,      ///< [IN] Buffer to write in the file
    size_t         bufSize      ///< [IN] Number of bytes to write
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
LE_SHARED le_result_t pa_fs_Seek
(
    int32_t fileHandler,        ///< [IN]  File handler
    off_t* offsetPtr,           ///< [IN]  Offset to apply when called
                                ///< [OUT] Offset from the beginning of the file when returns
    le_fs_Position_t position   ///< [IN]  Offset is relative to this position
);

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
LE_SHARED le_result_t pa_fs_GetSize
(
    const char* pathPtr,    ///< [IN] File path
    size_t*     sizePtr     ///< [IN] File size (if successful)
);

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
LE_SHARED le_result_t pa_fs_Delete
(
    const char* pathPtr     ///< [IN] File path
);

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
LE_SHARED le_result_t pa_fs_Move
(
    const char* srcPathPtr,     ///< [IN] Path to file to rename
    const char* destPathPtr     ///< [IN] New path to file
);

#endif // LEGATO_PA_FS_INCLUDE_GUARD
