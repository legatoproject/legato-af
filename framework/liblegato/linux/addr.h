//--------------------------------------------------------------------------------------------------
/** @file addr.h
 *
 * Utility functions for examining a process's virtual memory address space.
 *
 * Copyright (C) Sierra Wireless Inc.
 */


#ifndef LEGATO_SRC_ADDR_INCLUDE_GUARD
#define LEGATO_SRC_ADDR_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Gets the address of the .data section of the specified library for the specified process.
 * The address is in the address space of the specified process.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NOT_FOUND if the library was not found.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t addr_GetLibDataSection
(
    pid_t pid,              ///< [IN] Pid to get the address for.  Zero means the calling process.
    const char* libNamePtr, ///< [IN] Name of the library.
    off_t* libAddressPtr    ///< [OUT] Address of the .data section of the library.
);


#endif  // LEGATO_SRC_ADDR_INCLUDE_GUARD
