//--------------------------------------------------------------------------------------------------
/**
 * Definition of the File Mapping class, which is used to hold the information regarding a mapping
 * of a file system object from one place to another, either in the same or another file system.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 **/
//--------------------------------------------------------------------------------------------------

#ifndef FILE_MAPPING_H_INCLUDE_GUARD
#define FILE_MAPPING_H_INCLUDE_GUARD

namespace legato
{


//--------------------------------------------------------------------------------------------------
/**
 * A File Mapping defines a mapping of a file from one place in a file system to another place
 * in the same or another file system.
 *
 * Usually they are used to map files from somewhere outside of a sandbox to somewhere inside
 * of a sandbox.
 **/
//--------------------------------------------------------------------------------------------------
class FileMapping
{
    public:

        int m_PermissionFlags;    ///< See Permissions.h.

        std::string m_SourcePath; ///< File path to find the file at.

        std::string m_DestPath;   ///< Path at which the file will appear.

    public:
        bool IsReadable();
};



}

#endif // FILE_MAPPING_H_INCLUDE_GUARD
