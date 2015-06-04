//--------------------------------------------------------------------------------------------------
/**
 * @file system.h
 *
 * Copyright (C) Sierra Wireless Inc.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_MKTOOLS_MODEL_SYSTEM_H_INCLUDE_GUARD
#define LEGATO_MKTOOLS_MODEL_SYSTEM_H_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Represents a single system.
 */
//--------------------------------------------------------------------------------------------------
struct System_t
{
    System_t(parseTree::AdefFile_t* filePtr): defFilePtr(filePtr) {}

    parseTree::AdefFile_t* defFilePtr;  ///< Pointer to root of parse tree for the .adef file.

    std::list<App_t*> apps;  ///< List of apps in this system.
};


#endif // LEGATO_MKTOOLS_MODEL_SYSTEM_H_INCLUDE_GUARD
