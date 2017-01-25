
//--------------------------------------------------------------------------------------------------
/** @file atomFile.h
 *
 * Legato atomic file access inter-module include file.
 *
 * This file exposes interfaces that are for use by other modules inside the framework
 * implementation, but must not be used outside of the framework implementation.
 *
 * Copyright (C) Sierra Wireless Inc.
 * license.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_SRC_ATOM_FILE_INCLUDE_GUARD
#define LEGATO_SRC_ATOM_FILE_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Initialize the atomic file access internal memory pools.  This function is meant to be called
 * from Legato's internal init.
 */
//--------------------------------------------------------------------------------------------------
void atomFile_Init
(
    void
);


#endif  // LEGATO_SRC_ATOM_FILE_INCLUDE_GUARD
