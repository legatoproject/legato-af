//--------------------------------------------------------------------------------------------------
/** @file fs.h
 *
 * Legato file system service module's inter-module include file.
 *
 * This file exposes interfaces that are for use by other modules inside the framework
 * implementation, but must not be used outside of the framework implementation.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */
#ifndef FS_INCLUDE_GUARD
#define FS_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Initializes the file system service.  This function must be called before any other memory pool
 * functions are called.
 *
 * @note
 *      On failure, the process exits.
 */
//--------------------------------------------------------------------------------------------------
void fs_Init
(
    void
);


#endif  // FS_INCLUDE_GUARD
