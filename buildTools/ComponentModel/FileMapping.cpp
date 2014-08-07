//--------------------------------------------------------------------------------------------------
/**
 * Implementation of select methods of the FileMapping class.
 *
 * Copyright (C) 2014 Sierra Wireless Inc.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "LegatoObjectModel.h"


namespace legato
{

//--------------------------------------------------------------------------------------------------
/**
 * Comparison operator needed so this can be used inside sets.
 **/
//--------------------------------------------------------------------------------------------------
bool FileMapping::operator<
(
    const FileMapping& other
)
const
//--------------------------------------------------------------------------------------------------
{
    return (   (m_SourcePath < other.m_SourcePath)
            || (m_DestPath < other.m_DestPath) );
}


}
