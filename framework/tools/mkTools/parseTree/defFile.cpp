//--------------------------------------------------------------------------------------------------
/**
 * @file defFile.cpp
 *
 * Copyright (C) Sierra Wireless Inc.  Use of this work is subject to license.
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
    path(filePath),
    version(0),
    firstTokenPtr(NULL),
    lastTokenPtr(NULL)
//--------------------------------------------------------------------------------------------------
{
}



} // namespace parseTree
