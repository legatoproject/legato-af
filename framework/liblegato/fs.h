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
 * Default prefix path for RW if nothing is defined in the config tree
 */
//--------------------------------------------------------------------------------------------------
#define FS_PREFIX_DATA_PATH      "/data/le_fs/"


//--------------------------------------------------------------------------------------------------
/**
 * Alternative prefix path for RW if nothing is defined in the config tree
 */
//--------------------------------------------------------------------------------------------------
#define ALT_FS_PREFIX_DATA_PATH      ("/tmp" FS_PREFIX_DATA_PATH)


//--------------------------------------------------------------------------------------------------
/**
 * Initializes the file system service.
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
