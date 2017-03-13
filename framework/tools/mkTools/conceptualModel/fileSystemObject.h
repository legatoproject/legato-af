//--------------------------------------------------------------------------------------------------
/**
 * @file fileSystemObject.h
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_MKTOOLS_MODEL_FILE_SYSTEM_OBJECT_H_INCLUDE_GUARD
#define LEGATO_MKTOOLS_MODEL_FILE_SYSTEM_OBJECT_H_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Represents a file system object, such as a file or directory.
 */
//--------------------------------------------------------------------------------------------------
struct FileSystemObject_t
{
    /// Constructor
    FileSystemObject_t(const parseTree::TokenList_t* tokenListPtr): parseTreePtr(tokenListPtr) {}

    FileSystemObject_t
    (
        const std::string& newSrcPath,
        const std::string& destPath,
        const Permissions_t& permissions
    )
    :   srcPath(newSrcPath),
        destPath(destPath),
        permissions(permissions)
    {
    }

    const parseTree::TokenList_t* parseTreePtr;   ///< Pointer to related object in the parse tree.

    std::string srcPath;        ///< File system path where the object is found.
    std::string destPath;       ///< Path to where the object will be put on target.
    Permissions_t permissions;  ///< Read, write, and/or execute permissions on the object.
};


#endif // LEGATO_MKTOOLS_MODEL_FILE_SYSTEM_OBJECT_H_INCLUDE_GUARD
