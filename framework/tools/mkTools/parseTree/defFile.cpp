//--------------------------------------------------------------------------------------------------
/**
 * @file defFile.cpp
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"


namespace parseTree
{


//--------------------------------------------------------------------------------------------------
/**
 * Constructor
 */
//--------------------------------------------------------------------------------------------------
DefFile_t::DefFile_t
(
    Type_t fileType,            ///< The type of file CDEF, ADEF, etc.
    const std::string& filePath ///< The file system path to the file.
)
//--------------------------------------------------------------------------------------------------
:   type(fileType),
    path(path::MakeAbsolute(filePath)),
    pathMd5(md5(path::MakeCanonical(path))),
    version(0),
    firstTokenPtr(NULL),
    lastTokenPtr(NULL)
//--------------------------------------------------------------------------------------------------
{
}



} // namespace parseTree
